#pragma once

// Reimplementation for the Linux/GCC port (see
// playground/xray-monolith-vulkan-port-notes.md section 13, CMake skeleton
// bring-up).
//
// The original (Don Clugston's CodeProject "Member Function Pointers and
// the Fastest Possible C++ Delegates") relies on reinterpreting member
// function pointers through unions ("horrible_cast") to get raw-pointer-call
// performance without std::function's overhead. That's technically
// undefined behaviour that happened to work under the MSVC/GCC ABIs it was
// written against (2004-era GCC, workarounds up to GCC bug #8271); it does
// not parse under GCC 15's stricter template handling (the errors are
// coming from FASTDELEGATE_USESTATICFUNCTIONHACK's macro-generated code,
// not from anything project-specific).
//
// OpenXRay (a portable fork of the same X-Ray engine lineage) solved this
// by trimming fastdelegate.h down to just the modern FastDelegate<Sig>
// syntax - but Monolith's codebase uses the old numbered-class API
// (FastDelegate0/1/2/3, ~70 call sites) pervasively, so that trim isn't a
// safe drop-in here. This file instead reimplements the same public
// surface (both syntaxes) on top of std::function: fully portable, no
// pointer-punning, same call sites work unchanged.
//
// Known behavioural difference: equality/ordering (operator==/</>) compares
// bound-object identity, not (object, member function) identity - two
// different delegates bound to the *same* object compare equal. The
// original's UB-based comparison distinguished these; nothing found in this
// codebase's actual usage relies on that distinction, but it's a real gap
// if a future call site needs it.

#include <functional>

#define xr_stdcall // no calling-convention distinction on the Linux x86-64 SysV ABI

namespace fastdelegate
{
namespace detail
{

template <typename RetType, typename... Args>
class FastDelegateImpl
{
public:
	FastDelegateImpl() { clear(); }

	template <typename Y, typename X>
	FastDelegateImpl(Y* pthis, RetType (X::*function_to_bind)(Args...))
	{
		bind(pthis, function_to_bind);
	}

	template <typename Y, typename X>
	FastDelegateImpl(const Y* pthis, RetType (X::*function_to_bind)(Args...) const)
	{
		bind(pthis, function_to_bind);
	}

	FastDelegateImpl(RetType (*function_to_bind)(Args...)) { bind(function_to_bind); }

	template <typename Y, typename X>
	void bind(Y* pthis, RetType (X::*function_to_bind)(Args...))
	{
		m_identity = pthis;
		m_func = [pthis, function_to_bind](Args... args) -> RetType
		{
			return (pthis->*function_to_bind)(args...);
		};
	}

	template <typename Y, typename X>
	void bind(const Y* pthis, RetType (X::*function_to_bind)(Args...) const)
	{
		m_identity = const_cast<void*>(static_cast<const void*>(pthis));
		m_func = [pthis, function_to_bind](Args... args) -> RetType
		{
			return (pthis->*function_to_bind)(args...);
		};
	}

	void bind(RetType (*function_to_bind)(Args...))
	{
		m_identity = reinterpret_cast<void*>(function_to_bind);
		m_func = function_to_bind;
	}

	RetType operator()(Args... args) const { return m_func(args...); }

	void clear()
	{
		m_func = nullptr;
		m_identity = nullptr;
	}

	bool operator==(const FastDelegateImpl& x) const
	{
		return m_identity == x.m_identity && static_cast<bool>(m_func) == static_cast<bool>(x.m_func);
	}
	bool operator!=(const FastDelegateImpl& x) const { return !(*this == x); }
	bool operator<(const FastDelegateImpl& x) const { return m_identity < x.m_identity; }
	bool operator>(const FastDelegateImpl& x) const { return x < *this; }

	explicit operator bool() const { return static_cast<bool>(m_func); }
	bool empty() const { return !static_cast<bool>(m_func); }

private:
	std::function<RetType(Args...)> m_func;
	void* m_identity = nullptr;
};

} // namespace detail

// Old numbered-class syntax (arities 0-8, matching the original library's
// scope) - the syntax actually used throughout this codebase.
template <typename RetType = void>
using FastDelegate0 = detail::FastDelegateImpl<RetType>;

template <typename Param1, typename RetType = void>
using FastDelegate1 = detail::FastDelegateImpl<RetType, Param1>;

template <typename Param1, typename Param2, typename RetType = void>
using FastDelegate2 = detail::FastDelegateImpl<RetType, Param1, Param2>;

template <typename Param1, typename Param2, typename Param3, typename RetType = void>
using FastDelegate3 = detail::FastDelegateImpl<RetType, Param1, Param2, Param3>;

template <typename Param1, typename Param2, typename Param3, typename Param4, typename RetType = void>
using FastDelegate4 = detail::FastDelegateImpl<RetType, Param1, Param2, Param3, Param4>;

template <typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename RetType = void>
using FastDelegate5 = detail::FastDelegateImpl<RetType, Param1, Param2, Param3, Param4, Param5>;

template <typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename Param6,
          typename RetType = void>
using FastDelegate6 = detail::FastDelegateImpl<RetType, Param1, Param2, Param3, Param4, Param5, Param6>;

template <typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename Param6,
          typename Param7, typename RetType = void>
using FastDelegate7 = detail::FastDelegateImpl<RetType, Param1, Param2, Param3, Param4, Param5, Param6, Param7>;

template <typename Param1, typename Param2, typename Param3, typename Param4, typename Param5, typename Param6,
          typename Param7, typename Param8, typename RetType = void>
using FastDelegate8 =
    detail::FastDelegateImpl<RetType, Param1, Param2, Param3, Param4, Param5, Param6, Param7, Param8>;

// Modern function-type syntax: FastDelegate<RetType(Args...)>
template <typename Signature>
class FastDelegate; // primary template intentionally undefined

template <typename RetType, typename... Args>
class FastDelegate<RetType(Args...)> : public detail::FastDelegateImpl<RetType, Args...>
{
public:
	using detail::FastDelegateImpl<RetType, Args...>::FastDelegateImpl;
	FastDelegate() = default;
};

template <typename Y, typename RetType, typename... Args>
inline detail::FastDelegateImpl<RetType, Args...> MakeDelegate(Y* pthis, RetType (Y::*func)(Args...))
{
	return detail::FastDelegateImpl<RetType, Args...>(pthis, func);
}

template <typename Y, typename RetType, typename... Args>
inline detail::FastDelegateImpl<RetType, Args...> MakeDelegate(const Y* pthis, RetType (Y::*func)(Args...) const)
{
	return detail::FastDelegateImpl<RetType, Args...>(pthis, func);
}

} // namespace fastdelegate
