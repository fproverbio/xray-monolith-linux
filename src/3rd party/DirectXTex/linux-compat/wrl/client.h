// Minimal Microsoft::WRL::ComPtr replacement for the Linux/dxvk-native build
// of DirectXTex. Real WRL isn't available outside Windows, and DirectXTex is
// steered away from DirectX-Headers' own wsl/wrladapter.h (see DirectXTexP.h
// and DirectXTex.h: XR_DIRECTXTEX_DXVK_NATIVE branch) so that the COM
// interface pointers it hands back (ID3D11Device*, ID3D11Texture2D*, ...) are
// the exact same types dxvk-native's headers declare elsewhere in this engine,
// not a second, incompatible reimplementation.
#pragma once

namespace Microsoft { namespace WRL {

template <typename T>
class ComPtr
{
public:
    ComPtr() noexcept : ptr_(nullptr) {}
    ComPtr(std::nullptr_t) noexcept : ptr_(nullptr) {}

    explicit ComPtr(T* p) noexcept : ptr_(p)
    {
        if (ptr_) ptr_->AddRef();
    }

    ComPtr(const ComPtr& other) noexcept : ptr_(other.ptr_)
    {
        if (ptr_) ptr_->AddRef();
    }

    ComPtr(ComPtr&& other) noexcept : ptr_(other.ptr_)
    {
        other.ptr_ = nullptr;
    }

    ~ComPtr()
    {
        if (ptr_) ptr_->Release();
    }

    ComPtr& operator=(const ComPtr& other) noexcept
    {
        if (this != &other)
        {
            if (other.ptr_) other.ptr_->AddRef();
            if (ptr_) ptr_->Release();
            ptr_ = other.ptr_;
        }
        return *this;
    }

    ComPtr& operator=(ComPtr&& other) noexcept
    {
        if (this != &other)
        {
            if (ptr_) ptr_->Release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ComPtr& operator=(std::nullptr_t) noexcept
    {
        Reset();
        return *this;
    }

    void Reset() noexcept
    {
        if (ptr_)
        {
            ptr_->Release();
            ptr_ = nullptr;
        }
    }

    T* Get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T* const* GetAddressOf() const noexcept { return &ptr_; }
    T** GetAddressOf() noexcept { return &ptr_; }

    T** ReleaseAndGetAddressOf() noexcept
    {
        Reset();
        return &ptr_;
    }

    // Matches the &comPtr idiom used throughout DirectXTexD3D11.cpp to pass a
    // ComPtr<T> as a T** out-parameter (e.g. CreateTexture2D(&desc, ..., &tex)).
    T** operator&() noexcept
    {
        Reset();
        return &ptr_;
    }

    template <typename U>
    HRESULT As(ComPtr<U>* out) const noexcept
    {
        out->Reset();
        return ptr_->QueryInterface(__uuidof(U), reinterpret_cast<void**>(out->ReleaseAndGetAddressOf()));
    }

private:
    T* ptr_;
};

} } // namespace Microsoft::WRL
