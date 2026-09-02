// D3D-facing side of the D3DCompiler compat shim - implements D3DCompile,
// D3DDisassemble, D3DGetInputSignatureBlob and D3DReflect on top of the
// vendored vkd3d-shader library via d3dcompiler_vkd3d_bridge.h. Per the
// architecture-boundary rule documented there, this translation unit must
// only ever see dxvk's own D3D11/d3dcompiler headers - never vkd3d-shader's.
#include <d3dcompiler.h>
#include "d3d11shader_uuid_compat.h"
#include "d3dcompiler_vkd3d_bridge.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace {

//-----------------------------------------------------------------------------
// ID3D10Blob/ID3DBlob backed by a bridge-allocated (malloc'd) buffer. Mirrors
// D3DX11CompatBlob's QueryInterface/refcounting idiom (d3dx11tex_compat.h) -
// direct REFIID `==` comparison rather than __uuidof, matching this
// codebase's established style - but owns a raw bridge buffer instead of a
// std::vector, since the data always originates from xr_vkd3d_bridge_* calls
// and must be released with xr_vkd3d_bridge_free().
class Blob : public ID3DBlob
{
public:
    Blob(void* data, size_t size) : m_ref(1), m_data(data), m_size(size) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;
        if (riid == IID_ID3D10Blob || riid == IID_IUnknown)
        {
            *ppvObject = static_cast<ID3DBlob*>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ref = --m_ref;
        if (ref == 0)
            delete this;
        return ref;
    }

    void* STDMETHODCALLTYPE GetBufferPointer() override { return m_data; }
    SIZE_T STDMETHODCALLTYPE GetBufferSize() override { return m_size; }

private:
    ~Blob() { xr_vkd3d_bridge_free(m_data); }

    ULONG m_ref;
    void* m_data;
    size_t m_size;
};

void set_messages_blob(char* messages, ID3DBlob** out_blob)
{
    *out_blob = messages ? new Blob(messages, strlen(messages) + 1) : nullptr;
}

//-----------------------------------------------------------------------------
// ID3DInclude -> bridge trampoline, mirroring r4.cpp's `includer` class so
// vkd3d-shader's preprocessor can call back into an arbitrary caller-supplied
// ID3DInclude without vkd3d headers ever needing to know ID3DInclude exists.
//-----------------------------------------------------------------------------
struct IncludeTrampolineContext
{
    ID3DInclude* include;
};

int shim_open_include(const char* filename, int local, const char* parent_data,
    void* context, const void** out_data, size_t* out_size)
{
    IncludeTrampolineContext* ctx = static_cast<IncludeTrampolineContext*>(context);
    const void* data = nullptr;
    UINT bytes = 0;
    HRESULT hr = ctx->include->Open(
        local ? D3D_INCLUDE_LOCAL : D3D_INCLUDE_SYSTEM,
        filename, parent_data, &data, &bytes);
    if (FAILED(hr))
        return -1;
    *out_data = data;
    *out_size = bytes;
    return 0;
}

void shim_close_include(const void* data, size_t /*size*/, void* context)
{
    IncludeTrampolineContext* ctx = static_cast<IncludeTrampolineContext*>(context);
    ctx->include->Close(data);
}

//-----------------------------------------------------------------------------
// RDEF parsing + ID3D11ShaderReflection* COM wrapper classes.
//
// Byte layout below is taken directly from vkd3d-shader's own RDEF writer
// (libs/vkd3d-shader/hlsl_codegen.c, sm4_generate_rdef()) rather than from
// any Microsoft documentation, since that writer is the actual producer of
// every RDEF chunk this parser will ever see:
//
//   Base header (7 u32s, always present):
//     [0] buffer_count            [1] offset to buffer-desc array
//     [2] resource_count          [3] offset to resource(binding)-desc array
//     [4] (minor<<0)|(major<<8) in the low word, target_type in the high word
//     [5] flags (always 0)        [6] offset to creator string
//
//   RD11 extension (+8 u32s at absolute byte offsets 28..56), present only
//   when major_version (byte 1 of word[4]) >= 5:
//     byte28 "RD11" tag, byte32 header size, byte36 buffer-desc size,
//     byte40 binding-desc size, byte44 variable-desc size,
//     byte48 type-desc size, byte52 member-desc size, byte56 unknown.
//   Pre-SM5 (no RD11 extension) desc sizes are fixed: 8 words per binding,
//   6 words per variable, 4 words per type.
//
//   Binding (resource) desc - on-disk word order does NOT match
//   D3D11_SHADER_INPUT_BIND_DESC's member order and must be remapped:
//     word0 name-offset, word1 Type, word2 ReturnType, word3 Dimension,
//     word4 NumSamples, word5 BindPoint, word6 BindCount, word7 uFlags
//     [, word8 space, word9 id - SM5.1 only, unused here].
//
//   Buffer (cbuffer) desc - 6 words / 24 bytes always:
//     word0 name-offset, word1 Variables (count), word2 offset to this
//     buffer's variable-desc array, word3 Size, word4 uFlags, word5 Type.
//
//   Variable desc - 6 words (pre-SM5) or 10 words (SM5+, extra 4 words are
//     texture/sampler start/count, always 0, unused here) - word order
//     matches D3D11_SHADER_VARIABLE_DESC positionally for the first 6:
//     word0 name-offset, word1 StartOffset, word2 Size, word3 uFlags,
//     word4 offset to type-desc, word5 default-value offset (0 if none).
//
//   Type desc - 4 words (pre-SM5) or 9 words (SM5+, name field added):
//     word0 Class (low16) | Type (high16), word1 Rows (low16) | Columns
//     (high16), word2 Elements (low16) | Members (high16), word3 Offset
//     (member-desc-array offset - unused, GetMemberTypeByIndex is out of
//     scope), word8 (byte offset +32, SM5+ only) name-offset.
//
// Scope is intentionally narrow: only the methods/fields actually read by
// this codebase's real reflection consumer chain (dx10r_constants.cpp,
// dx10ConstantBuffer.cpp) are implemented for real; everything else on
// ID3D11ShaderReflection & friends is a harmless stub, since it is never
// called. In particular no struct-member recursion is needed anywhere.
//-----------------------------------------------------------------------------

uint32_t read_u32(const uint8_t* base, size_t offset)
{
    uint32_t v;
    memcpy(&v, base + offset, sizeof(v));
    return v;
}

const char* read_string(const uint8_t* base, uint32_t offset)
{
    return reinterpret_cast<const char*>(base + offset);
}

class ConstantBuffer;

class Type : public ID3D11ShaderReflectionType
{
public:
    D3D11_SHADER_TYPE_DESC desc{};

    HRESULT STDMETHODCALLTYPE GetDesc(D3D11_SHADER_TYPE_DESC* out) override
    {
        if (!out)
            return E_POINTER;
        *out = desc;
        return S_OK;
    }
    ID3D11ShaderReflectionType* STDMETHODCALLTYPE GetMemberTypeByIndex(UINT) override { return nullptr; }
    ID3D11ShaderReflectionType* STDMETHODCALLTYPE GetMemberTypeByName(const char*) override { return nullptr; }
    const char* STDMETHODCALLTYPE GetMemberTypeName(UINT) override { return nullptr; }
    HRESULT STDMETHODCALLTYPE IsEqual(ID3D11ShaderReflectionType*) override { return S_FALSE; }
    ID3D11ShaderReflectionType* STDMETHODCALLTYPE GetSubType() override { return nullptr; }
    ID3D11ShaderReflectionType* STDMETHODCALLTYPE GetBaseClass() override { return nullptr; }
    UINT STDMETHODCALLTYPE GetNumInterfaces() override { return 0; }
    ID3D11ShaderReflectionType* STDMETHODCALLTYPE GetInterfaceByIndex(UINT) override { return nullptr; }
    HRESULT STDMETHODCALLTYPE IsOfType(ID3D11ShaderReflectionType*) override { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE ImplementsInterface(ID3D11ShaderReflectionType*) override { return S_FALSE; }
};

class Variable : public ID3D11ShaderReflectionVariable
{
public:
    D3D11_SHADER_VARIABLE_DESC desc{};
    Type type;

    HRESULT STDMETHODCALLTYPE GetDesc(D3D11_SHADER_VARIABLE_DESC* out) override
    {
        if (!out)
            return E_POINTER;
        *out = desc;
        return S_OK;
    }
    ID3D11ShaderReflectionType* STDMETHODCALLTYPE GetType() override { return &type; }
    ID3D11ShaderReflectionConstantBuffer* STDMETHODCALLTYPE GetBuffer() override { return nullptr; }
    UINT STDMETHODCALLTYPE GetInterfaceSlot(UINT) override { return 0; }
};

class ConstantBuffer : public ID3D11ShaderReflectionConstantBuffer
{
public:
    D3D11_SHADER_BUFFER_DESC desc{};
    std::vector<Variable> variables;

    HRESULT STDMETHODCALLTYPE GetDesc(D3D11_SHADER_BUFFER_DESC* out) override
    {
        if (!out)
            return E_POINTER;
        *out = desc;
        return S_OK;
    }
    ID3D11ShaderReflectionVariable* STDMETHODCALLTYPE GetVariableByIndex(UINT index) override
    {
        if (index >= variables.size())
            return nullptr;
        return &variables[index];
    }
    ID3D11ShaderReflectionVariable* STDMETHODCALLTYPE GetVariableByName(const char* name) override
    {
        if (!name)
            return nullptr;
        for (auto& v : variables)
            if (v.desc.Name && strcmp(v.desc.Name, name) == 0)
                return &v;
        return nullptr;
    }
};

class Reflection : public ID3D11ShaderReflection
{
public:
    Reflection() : m_ref(1), m_rdef_data(nullptr) {}

    void* m_rdef_data;
    std::vector<ConstantBuffer> buffers;
    std::vector<D3D11_SHADER_INPUT_BIND_DESC> bindings;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** out) override
    {
        if (!out)
            return E_POINTER;
        if (riid == IID_ID3D11ShaderReflection || riid == IID_IUnknown)
        {
            *out = static_cast<ID3D11ShaderReflection*>(this);
            AddRef();
            return S_OK;
        }
        *out = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ref = --m_ref;
        if (ref == 0)
            delete this;
        return ref;
    }

    HRESULT STDMETHODCALLTYPE GetDesc(D3D11_SHADER_DESC* out) override
    {
        if (!out)
            return E_POINTER;
        memset(out, 0, sizeof(*out));
        out->ConstantBuffers = (UINT)buffers.size();
        out->BoundResources = (UINT)bindings.size();
        return S_OK;
    }
    ID3D11ShaderReflectionConstantBuffer* STDMETHODCALLTYPE GetConstantBufferByIndex(UINT index) override
    {
        if (index >= buffers.size())
            return nullptr;
        return &buffers[index];
    }
    ID3D11ShaderReflectionConstantBuffer* STDMETHODCALLTYPE GetConstantBufferByName(const char* name) override
    {
        if (!name)
            return nullptr;
        for (auto& b : buffers)
            if (b.desc.Name && strcmp(b.desc.Name, name) == 0)
                return &b;
        return nullptr;
    }
    HRESULT STDMETHODCALLTYPE GetResourceBindingDesc(UINT index, D3D11_SHADER_INPUT_BIND_DESC* out) override
    {
        if (!out)
            return E_POINTER;
        if (index >= bindings.size())
            return E_INVALIDARG;
        *out = bindings[index];
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetInputParameterDesc(UINT, D3D11_SIGNATURE_PARAMETER_DESC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetOutputParameterDesc(UINT, D3D11_SIGNATURE_PARAMETER_DESC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPatchConstantParameterDesc(UINT, D3D11_SIGNATURE_PARAMETER_DESC*) override { return E_NOTIMPL; }
    ID3D11ShaderReflectionVariable* STDMETHODCALLTYPE GetVariableByName(const char* name) override
    {
        if (!name)
            return nullptr;
        for (auto& b : buffers)
        {
            ID3D11ShaderReflectionVariable* v = b.GetVariableByName(name);
            if (v)
                return v;
        }
        return nullptr;
    }
    HRESULT STDMETHODCALLTYPE GetResourceBindingDescByName(const char* name, D3D11_SHADER_INPUT_BIND_DESC* out) override
    {
        if (!out)
            return E_POINTER;
        if (!name)
            return E_INVALIDARG;
        for (auto& b : bindings)
            if (b.Name && strcmp(b.Name, name) == 0)
            {
                *out = b;
                return S_OK;
            }
        return E_INVALIDARG;
    }
    UINT STDMETHODCALLTYPE GetMovInstructionCount() override { return 0; }
    UINT STDMETHODCALLTYPE GetMovcInstructionCount() override { return 0; }
    UINT STDMETHODCALLTYPE GetConversionInstructionCount() override { return 0; }
    UINT STDMETHODCALLTYPE GetBitwiseInstructionCount() override { return 0; }
    D3D_PRIMITIVE STDMETHODCALLTYPE GetGSInputPrimitive() override { return D3D_PRIMITIVE_UNDEFINED; }
    WINBOOL STDMETHODCALLTYPE IsSampleFrequencyShader() override { return FALSE; }
    UINT STDMETHODCALLTYPE GetNumInterfaceSlots() override { return 0; }
    HRESULT STDMETHODCALLTYPE GetMinFeatureLevel(D3D_FEATURE_LEVEL* level) override
    {
        if (!level)
            return E_POINTER;
        *level = D3D_FEATURE_LEVEL_11_0;
        return S_OK;
    }
    UINT STDMETHODCALLTYPE GetThreadGroupSize(UINT* x, UINT* y, UINT* z) override
    {
        if (x) *x = 0;
        if (y) *y = 0;
        if (z) *z = 0;
        return 0;
    }
    UINT64 STDMETHODCALLTYPE GetRequiresFlags() override { return 0; }

private:
    ~Reflection() { xr_vkd3d_bridge_free(m_rdef_data); }

    ULONG m_ref;
};

void read_type_desc(const uint8_t* base, uint32_t type_offset, bool has_rd11, D3D11_SHADER_TYPE_DESC* out)
{
    memset(out, 0, sizeof(*out));
    uint32_t w0 = read_u32(base, type_offset + 0);
    uint32_t w1 = read_u32(base, type_offset + 4);
    uint32_t w2 = read_u32(base, type_offset + 8);
    uint32_t w3 = read_u32(base, type_offset + 12);
    out->Class = static_cast<D3D_SHADER_VARIABLE_CLASS>(w0 & 0xFFFF);
    out->Type = static_cast<D3D_SHADER_VARIABLE_TYPE>((w0 >> 16) & 0xFFFF);
    out->Rows = w1 & 0xFFFF;
    out->Columns = (w1 >> 16) & 0xFFFF;
    out->Elements = w2 & 0xFFFF;
    out->Members = (w2 >> 16) & 0xFFFF;
    out->Offset = w3;
    out->Name = has_rd11 ? read_string(base, read_u32(base, type_offset + 32)) : nullptr;
}

bool parse_rdef(const uint8_t* data, size_t size, Reflection* refl)
{
    if (size < 28)
        return false;

    uint32_t buffer_count = read_u32(data, 0);
    uint32_t buffer_offset = read_u32(data, 4);
    uint32_t resource_count = read_u32(data, 8);
    uint32_t resource_offset = read_u32(data, 12);
    uint32_t version_field = read_u32(data, 16);
    uint32_t major_version = (version_field >> 8) & 0xFF;

    bool has_rd11 = major_version >= 5;
    uint32_t binding_desc_size, variable_desc_size;
    if (has_rd11)
    {
        if (size < 60)
            return false;
        binding_desc_size = read_u32(data, 40);
        variable_desc_size = read_u32(data, 44);
    }
    else
    {
        binding_desc_size = 8 * 4;
        variable_desc_size = 6 * 4;
    }

    refl->bindings.resize(resource_count);
    for (uint32_t i = 0; i < resource_count; ++i)
    {
        uint32_t base = resource_offset + i * binding_desc_size;
        D3D11_SHADER_INPUT_BIND_DESC& bd = refl->bindings[i];
        memset(&bd, 0, sizeof(bd));
        bd.Name = read_string(data, read_u32(data, base + 0));
        bd.Type = static_cast<D3D_SHADER_INPUT_TYPE>(read_u32(data, base + 4));
        bd.ReturnType = static_cast<D3D_RESOURCE_RETURN_TYPE>(read_u32(data, base + 8));
        bd.Dimension = static_cast<D3D_SRV_DIMENSION>(read_u32(data, base + 12));
        bd.NumSamples = read_u32(data, base + 16);
        bd.BindPoint = read_u32(data, base + 20);
        bd.BindCount = read_u32(data, base + 24);
        bd.uFlags = read_u32(data, base + 28);
    }

    refl->buffers.resize(buffer_count);
    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        uint32_t base = buffer_offset + i * 24;
        uint32_t var_count = read_u32(data, base + 4);
        uint32_t vars_offset = read_u32(data, base + 8);

        ConstantBuffer& cb = refl->buffers[i];
        cb.desc.Name = read_string(data, read_u32(data, base + 0));
        cb.desc.Variables = var_count;
        cb.desc.Size = read_u32(data, base + 12);
        cb.desc.uFlags = read_u32(data, base + 16);
        cb.desc.Type = static_cast<D3D_CBUFFER_TYPE>(read_u32(data, base + 20));

        cb.variables.resize(var_count);
        for (uint32_t v = 0; v < var_count; ++v)
        {
            uint32_t vbase = vars_offset + v * variable_desc_size;
            uint32_t type_offset = read_u32(data, vbase + 16);
            uint32_t default_offset = read_u32(data, vbase + 20);

            Variable& var = cb.variables[v];
            var.desc.Name = read_string(data, read_u32(data, vbase + 0));
            var.desc.StartOffset = read_u32(data, vbase + 4);
            var.desc.Size = read_u32(data, vbase + 8);
            var.desc.uFlags = read_u32(data, vbase + 12);
            var.desc.DefaultValue = default_offset ? const_cast<uint8_t*>(data + default_offset) : nullptr;
            var.desc.StartTexture = 0;
            var.desc.TextureSize = 0;
            var.desc.StartSampler = 0;
            var.desc.SamplerSize = 0;

            read_type_desc(data, type_offset, has_rd11, &var.type.desc);
        }
    }

    return true;
}

} // namespace

extern "C" HRESULT WINAPI D3DCompile(
    const void* data, SIZE_T data_size, const char* filename,
    const D3D_SHADER_MACRO* defines, ID3DInclude* include, const char* entrypoint,
    const char* target, UINT sflags, UINT eflags, ID3DBlob** shader, ID3DBlob** error_messages)
{
    (void)eflags;

    if (shader)
        *shader = nullptr;
    if (error_messages)
        *error_messages = nullptr;

    std::vector<xr_vkd3d_bridge_macro> macros;
    if (defines)
        for (const D3D_SHADER_MACRO* m = defines; m->Name; ++m)
            macros.push_back({m->Name, m->Definition});

    IncludeTrampolineContext include_ctx{include};

    unsigned int bridge_flags = 0;
    if (sflags & D3DCOMPILE_PACK_MATRIX_ROW_MAJOR)
        bridge_flags |= XR_VKD3D_BRIDGE_PACK_MATRIX_ROW_MAJOR;
    else if (sflags & D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR)
        bridge_flags |= XR_VKD3D_BRIDGE_PACK_MATRIX_COLUMN_MAJOR;

    void* out_code = nullptr;
    size_t out_size = 0;
    char* messages = nullptr;

    int rc = xr_vkd3d_bridge_compile_hlsl(
        static_cast<const char*>(data), data_size, filename,
        macros.empty() ? nullptr : macros.data(), (unsigned int)macros.size(),
        include ? shim_open_include : nullptr,
        include ? shim_close_include : nullptr,
        &include_ctx,
        entrypoint, target, bridge_flags,
        &out_code, &out_size, &messages);

    if (error_messages)
        set_messages_blob(messages, error_messages);
    else if (messages)
        xr_vkd3d_bridge_free_messages(messages);

    if (rc != 0)
    {
        xr_vkd3d_bridge_free(out_code);
        return E_FAIL;
    }

    if (shader)
        *shader = new Blob(out_code, out_size);
    else
        xr_vkd3d_bridge_free(out_code);

    return S_OK;
}

extern "C" HRESULT WINAPI D3DDisassemble(
    const void* data, SIZE_T data_size, UINT flags, const char* comments, ID3DBlob** disassembly)
{
    (void)flags;
    (void)comments;

    if (!disassembly)
        return E_POINTER;
    *disassembly = nullptr;

    void* out_code = nullptr;
    size_t out_size = 0;
    char* messages = nullptr;

    int rc = xr_vkd3d_bridge_disassemble(data, data_size, &out_code, &out_size, &messages);
    if (messages)
        xr_vkd3d_bridge_free_messages(messages);

    if (rc != 0)
    {
        xr_vkd3d_bridge_free(out_code);
        return E_FAIL;
    }

    *disassembly = new Blob(out_code, out_size + 1);
    return S_OK;
}

extern "C" HRESULT WINAPI D3DGetInputSignatureBlob(const void* data, SIZE_T data_size, ID3DBlob** blob)
{
    if (!blob)
        return E_POINTER;
    *blob = nullptr;

    void* out_code = nullptr;
    size_t out_size = 0;

    if (xr_vkd3d_bridge_extract_dxbc_section(data, data_size, XR_VKD3D_BRIDGE_TAG_ISGN, &out_code, &out_size) != 0)
        return E_FAIL;

    *blob = new Blob(out_code, out_size);
    return S_OK;
}

extern "C" HRESULT WINAPI D3DReflect(const void* data, SIZE_T data_size, REFIID riid, void** reflector)
{
    if (!reflector)
        return E_POINTER;
    *reflector = nullptr;

    if (riid != IID_ID3D11ShaderReflection && riid != IID_IUnknown)
        return E_NOINTERFACE;

    void* rdef_data = nullptr;
    size_t rdef_size = 0;
    if (xr_vkd3d_bridge_find_dxbc_section(data, data_size, XR_VKD3D_BRIDGE_TAG_RDEF, &rdef_data, &rdef_size) != 0)
        return E_FAIL;

    Reflection* refl = new Reflection();
    refl->m_rdef_data = rdef_data;

    if (!parse_rdef(static_cast<const uint8_t*>(rdef_data), rdef_size, refl))
    {
        refl->Release();
        return E_FAIL;
    }

    *reflector = refl;
    return S_OK;
}
