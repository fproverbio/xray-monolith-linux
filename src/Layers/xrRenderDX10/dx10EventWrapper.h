#pragma once

// L#Name (producing a single L"Name" wide-string-literal token straight from
// stringizing a macro parameter) is an MSVC-only preprocessor extension -
// standard C++ only token-pastes with `##`, and `#` (stringize) doesn't
// combine with an adjacent `L` the same way. The portable equivalent is the
// well-known two-level widen-macro indirection: stringize Name first, then
// paste L onto the resulting string-literal token in a separate expansion
// step (pasting must happen in a macro whose argument is already the
// stringized literal, not the raw identifier).
#define PIX_EVENT_WIDEN_(x) L##x
#define PIX_EVENT_WIDEN(x) PIX_EVENT_WIDEN_(x)
#define PIX_EVENT(Name) dxPixEventWrapper pixEvent##Name(PIX_EVENT_WIDEN(#Name))

class dxPixEventWrapper
{
public:
    dxPixEventWrapper(LPCWSTR wszName);
    ~dxPixEventWrapper();
}; 