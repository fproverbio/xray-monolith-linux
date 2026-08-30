#ifndef xrDebugH
#define xrDebugH
#pragma once

// Needs LPCSTR - included before vector.h (the file that actually pulls in
// _types.h) in xrCore.h's own include order; relied on implicit MSVC
// precompiled-header ordering before.
#include "_types.h"

typedef void crashhandler(void);
typedef void on_dialog(bool before);

class XRCORE_API xrDebug
{
private:
	crashhandler* handler;
	on_dialog* m_on_dialog;

public:
	void _initialize(const bool& dedicated);
	void _destroy();

public:
	crashhandler* get_crashhandler()
	{
		return handler;
	};

	void set_crashhandler(crashhandler* _handler)
	{
		handler = _handler;
	};

	on_dialog* get_on_dialog()
	{
		return m_on_dialog;
	}

	void set_on_dialog(on_dialog* on_dialog)
	{
		m_on_dialog = on_dialog;
	}

	LPCSTR error2string(long code);

	void gather_info(const char* expression, const char* description, const char* argument0, const char* argument1,
	                 const char* file, int line, const char* function, LPSTR assertion_info,
	                 unsigned int assertion_info_size);

	template <int count>
	inline void gather_info(const char* expression, const char* description, const char* argument0,
	                        const char* argument1, const char* file, int line, const char* function,
	                        char (&assertion_info)[count])
	{
		gather_info(expression, description, argument0, argument1, file, line, function, assertion_info, count);
	}

	void fail(const char* e1, const char* file, int line, const char* function, bool& ignore_always);
	void fail(const char* e1, const std::string& e2, const char* file, int line, const char* function,
	          bool& ignore_always);
	void fail(const char* e1, const char* e2, const char* file, int line, const char* function, bool& ignore_always);
	void fail(const char* e1, const char* e2, const char* e3, const char* file, int line, const char* function,
	          bool& ignore_always);
	void fail(const char* e1, const char* e2, const char* e3, const char* e4, const char* file, int line,
	          const char* function, bool& ignore_always);
	//AVO: print, dont crash
	void soft_fail(LPCSTR e1, LPCSTR file, int line, LPCSTR function);
	void soft_fail(LPCSTR e1, const std::string& e2, LPCSTR file, int line, LPCSTR function);
	void soft_fail(LPCSTR e1, LPCSTR e2, LPCSTR file, int line, LPCSTR function);
	void soft_fail(LPCSTR e1, LPCSTR e2, LPCSTR e3, LPCSTR file, int line, LPCSTR function);
	void soft_fail(LPCSTR e1, LPCSTR e2, LPCSTR e3, LPCSTR e4, LPCSTR file, int line, LPCSTR function);
	void soft_fail(LPCSTR e1, LPCSTR e2, LPCSTR e3, LPCSTR e4, LPCSTR e5, LPCSTR file, int line, LPCSTR function);
	//-AVO
	void error(long code, const char* e1, const char* file, int line, const char* function, bool& ignore_always);
	void error(long code, const char* e1, const char* e2, const char* file, int line, const char* function,
	           bool& ignore_always);
	void _cdecl fatal(const char* file, int line, const char* function, const char* F, ...);
	void backend(const char* reason, const char* expression, const char* argument0, const char* argument1,
	             const char* file, int line, const char* function, bool& ignore_always);
	void do_exit(const std::string& message);
};

// warning
// this function can be used for debug purposes only
IC std::string __cdecl make_string(LPCSTR format, ...)
{
	va_list args;
	va_start(args, format);

	char temp[4096];
	vsprintf(temp, format, args);

	return std::string(temp);
}

extern XRCORE_API xrDebug Debug;

// The real definition (xrDebugNew.cpp) takes a 2nd `bool printStack`
// parameter this declaration was missing entirely - a genuine signature
// mismatch (not just a default-argument gap), so every real call site
// (which only ever passes 1 arg) was mangled against this declaration's
// 1-parameter symbol name, which xrDebugNew.cpp's actual 2-parameter
// definition can never satisfy.
XRCORE_API void LogStackTrace(LPCSTR header, bool printStack = false);

#include "xrDebug_macros.h"

#endif // xrDebugH
