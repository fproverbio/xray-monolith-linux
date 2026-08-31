/* Implementation side of the D3DCompiler-family bridge - see
 * d3dcompiler_vkd3d_bridge.h for the rationale and the boundary rule
 * (vkd3d-shader headers only, never dxvk's). */
#include "d3dcompiler_vkd3d_bridge.h"

#include <stdlib.h>
#include <string.h>

#include <vkd3d_shader.h>

/* Everything handed back across the bridge boundary is a plain
 * malloc()'d buffer, regardless of which allocator vkd3d-shader itself
 * used internally - this keeps the bridge's free contract
 * (xr_vkd3d_bridge_free/xr_vkd3d_bridge_free_messages, i.e. plain
 * free()) independent of vkd3d's own vkd3d_shader_free_shader_code() /
 * vkd3d_shader_free_messages(), which are still called here to release
 * vkd3d's originals after copying out. */
static int dup_to_malloc(const void *src, size_t size, void **out)
{
	void *copy;

	if (!size)
	{
		*out = NULL;
		return 0;
	}

	copy = malloc(size);
	if (!copy)
		return -1;

	memcpy(copy, src, size);
	*out = copy;
	return 0;
}

static char *dup_messages(char *vkd3d_messages)
{
	char *copy;
	size_t len;

	if (!vkd3d_messages)
		return NULL;

	len = strlen(vkd3d_messages) + 1;
	copy = malloc(len);
	if (copy)
		memcpy(copy, vkd3d_messages, len);

	vkd3d_shader_free_messages(vkd3d_messages);
	return copy;
}

void xr_vkd3d_bridge_free(void *p)
{
	free(p);
}

void xr_vkd3d_bridge_free_messages(char *messages)
{
	free(messages);
}

/* Trampoline context bridging vkd3d-shader's PFN_vkd3d_shader_open_include/
 * PFN_vkd3d_shader_close_include callbacks (which speak in terms of
 * struct vkd3d_shader_code) to this bridge's primitive-only callback
 * types, so the shim side (d3dcompiler_shim.cpp) never needs to see
 * vkd3d_shader.h to implement an include handler. */
struct include_trampoline_ctx
{
	xr_vkd3d_bridge_open_include_fn open_include;
	xr_vkd3d_bridge_close_include_fn close_include;
	void *user_context;
};

static int trampoline_open_include(const char *filename, bool local,
	const char *parent_data, void *context, struct vkd3d_shader_code *out)
{
	struct include_trampoline_ctx *ctx = context;
	const void *data = NULL;
	size_t size = 0;
	int rc;

	rc = ctx->open_include(filename, local ? 1 : 0, parent_data,
		ctx->user_context, &data, &size);
	if (rc)
		return VKD3D_ERROR;

	out->code = data;
	out->size = size;
	return VKD3D_OK;
}

static void trampoline_close_include(const struct vkd3d_shader_code *code, void *context)
{
	struct include_trampoline_ctx *ctx = context;

	ctx->close_include(code->code, code->size, ctx->user_context);
}

int xr_vkd3d_bridge_compile_hlsl(
	const char *source, size_t source_len, const char *source_name,
	const struct xr_vkd3d_bridge_macro *macros, unsigned int macro_count,
	xr_vkd3d_bridge_open_include_fn open_include,
	xr_vkd3d_bridge_close_include_fn close_include, void *include_context,
	const char *entry_point, const char *profile,
	void **out_code, size_t *out_size, char **out_messages)
{
	struct vkd3d_shader_hlsl_source_info hlsl_info;
	struct vkd3d_shader_preprocess_info preprocess_info;
	struct vkd3d_shader_compile_info compile_info;
	struct include_trampoline_ctx include_ctx;
	struct vkd3d_shader_macro *vkd3d_macros = NULL;
	struct vkd3d_shader_code out_dxbc = {0};
	char *messages = NULL;
	unsigned int i;
	int rc;

	*out_code = NULL;
	*out_size = 0;
	if (out_messages)
		*out_messages = NULL;

	if (macro_count)
	{
		vkd3d_macros = malloc(macro_count * sizeof(*vkd3d_macros));
		if (!vkd3d_macros)
			return -1;
		for (i = 0; i < macro_count; ++i)
		{
			vkd3d_macros[i].name = macros[i].name;
			vkd3d_macros[i].value = macros[i].value;
		}
	}

	memset(&preprocess_info, 0, sizeof(preprocess_info));
	preprocess_info.type = VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO;
	preprocess_info.next = NULL;
	preprocess_info.macros = vkd3d_macros;
	preprocess_info.macro_count = macro_count;

	if (open_include)
	{
		include_ctx.open_include = open_include;
		include_ctx.close_include = close_include;
		include_ctx.user_context = include_context;
		preprocess_info.pfn_open_include = trampoline_open_include;
		preprocess_info.pfn_close_include = trampoline_close_include;
		preprocess_info.include_context = &include_ctx;
	}

	memset(&hlsl_info, 0, sizeof(hlsl_info));
	hlsl_info.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO;
	hlsl_info.next = NULL;
	hlsl_info.entry_point = entry_point;
	hlsl_info.secondary_code.code = NULL;
	hlsl_info.secondary_code.size = 0;
	hlsl_info.profile = profile;

	preprocess_info.next = &hlsl_info;

	memset(&compile_info, 0, sizeof(compile_info));
	compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
	compile_info.next = &preprocess_info;
	compile_info.source.code = source;
	compile_info.source.size = source_len;
	compile_info.source_type = VKD3D_SHADER_SOURCE_HLSL;
	compile_info.target_type = VKD3D_SHADER_TARGET_DXBC_TPF;
	compile_info.options = NULL;
	compile_info.option_count = 0;
	compile_info.log_level = VKD3D_SHADER_LOG_INFO;
	compile_info.source_name = source_name;

	rc = vkd3d_shader_compile(&compile_info, &out_dxbc, &messages);

	free(vkd3d_macros);

	if (out_messages)
		*out_messages = dup_messages(messages);
	else if (messages)
		vkd3d_shader_free_messages(messages);

	if (rc < 0)
		return rc;

	if (dup_to_malloc(out_dxbc.code, out_dxbc.size, out_code))
	{
		vkd3d_shader_free_shader_code(&out_dxbc);
		return -1;
	}
	*out_size = out_dxbc.size;
	vkd3d_shader_free_shader_code(&out_dxbc);
	return 0;
}

int xr_vkd3d_bridge_disassemble(
	const void *dxbc, size_t dxbc_size,
	void **out_code, size_t *out_size, char **out_messages)
{
	struct vkd3d_shader_compile_info compile_info;
	struct vkd3d_shader_code out_text = {0};
	char *messages = NULL;
	int rc;

	*out_code = NULL;
	*out_size = 0;
	if (out_messages)
		*out_messages = NULL;

	memset(&compile_info, 0, sizeof(compile_info));
	compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
	compile_info.next = NULL;
	compile_info.source.code = dxbc;
	compile_info.source.size = dxbc_size;
	compile_info.source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
	compile_info.target_type = VKD3D_SHADER_TARGET_D3D_ASM;
	compile_info.options = NULL;
	compile_info.option_count = 0;
	compile_info.log_level = VKD3D_SHADER_LOG_INFO;
	compile_info.source_name = NULL;

	rc = vkd3d_shader_compile(&compile_info, &out_text, &messages);

	if (out_messages)
		*out_messages = dup_messages(messages);
	else if (messages)
		vkd3d_shader_free_messages(messages);

	if (rc < 0)
		return rc;

	/* vkd3d_shader_compile() appends a null terminator past out_text.size
	 * for textual targets like D3D_ASM (see struct vkd3d_shader_code's
	 * doc comment) - include it in the copy so the result is safe to
	 * treat as a C string. */
	if (dup_to_malloc(out_text.code, out_text.size + 1, out_code))
	{
		vkd3d_shader_free_shader_code(&out_text);
		return -1;
	}
	*out_size = out_text.size;
	vkd3d_shader_free_shader_code(&out_text);
	return 0;
}

static int find_section(const struct vkd3d_shader_dxbc_desc *desc, unsigned int tag,
	const struct vkd3d_shader_dxbc_section_desc **out_section)
{
	size_t i;

	for (i = 0; i < desc->section_count; ++i)
	{
		if (desc->sections[i].tag == tag)
		{
			*out_section = &desc->sections[i];
			return 0;
		}
	}
	return -1;
}

int xr_vkd3d_bridge_extract_dxbc_section(
	const void *dxbc, size_t dxbc_size, unsigned int tag,
	void **out_code, size_t *out_size)
{
	struct vkd3d_shader_code in = {dxbc, dxbc_size};
	struct vkd3d_shader_dxbc_desc desc;
	const struct vkd3d_shader_dxbc_section_desc *section;
	struct vkd3d_shader_code out_dxbc = {0};
	char *messages = NULL;
	int rc;

	*out_code = NULL;
	*out_size = 0;

	if (vkd3d_shader_parse_dxbc(&in, 0, &desc, &messages) < 0)
	{
		vkd3d_shader_free_messages(messages);
		return -1;
	}
	vkd3d_shader_free_messages(messages);

	if (find_section(&desc, tag, &section))
	{
		vkd3d_shader_free_dxbc(&desc);
		return -1;
	}

	messages = NULL;
	rc = vkd3d_shader_serialize_dxbc(1, section, &out_dxbc, &messages);
	vkd3d_shader_free_messages(messages);
	vkd3d_shader_free_dxbc(&desc);

	if (rc < 0)
		return rc;

	if (dup_to_malloc(out_dxbc.code, out_dxbc.size, out_code))
	{
		vkd3d_shader_free_shader_code(&out_dxbc);
		return -1;
	}
	*out_size = out_dxbc.size;
	vkd3d_shader_free_shader_code(&out_dxbc);
	return 0;
}

int xr_vkd3d_bridge_find_dxbc_section(
	const void *dxbc, size_t dxbc_size, unsigned int tag,
	void **out_data, size_t *out_size)
{
	struct vkd3d_shader_code in = {dxbc, dxbc_size};
	struct vkd3d_shader_dxbc_desc desc;
	const struct vkd3d_shader_dxbc_section_desc *section;
	char *messages = NULL;
	int result;

	*out_data = NULL;
	*out_size = 0;

	if (vkd3d_shader_parse_dxbc(&in, 0, &desc, &messages) < 0)
	{
		vkd3d_shader_free_messages(messages);
		return -1;
	}
	vkd3d_shader_free_messages(messages);

	if (find_section(&desc, tag, &section))
	{
		vkd3d_shader_free_dxbc(&desc);
		return -1;
	}

	result = dup_to_malloc(section->data.code, section->data.size, out_data);
	if (!result)
		*out_size = section->data.size;

	vkd3d_shader_free_dxbc(&desc);
	return result;
}
