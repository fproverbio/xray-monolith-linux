#ifndef _TTAPI_H_INCLUDED_
#define _TTAPI_H_INCLUDED_

// Declarations-only stub pass (see playground/xray-monolith-vulkan-port-
// notes.md section 21, Part 1 item 2) - only the header needs to exist so
// xrCPU_Pipe.h's includers can see these declarations; the real
// implementation (PLC.cpp/ttapi.cpp, SSE-based skinning) stays unported.
// Matches xrCore_platform.h's established _WIN32-vs-win32_compat.h split
// rather than pulling real <windows.h> on Linux.
#ifdef _WIN32
#include <windows.h>
#else
#include "../xrCore/win32_compat.h"
#endif

/*
	Trivial (and dumb) Threads API
*/

#ifdef _GPA_ENABLED
	#include <tal.h>
#endif // _GPA_ENABLED


typedef VOID (*PTTAPI_WORKER_FUNC)(LPVOID lpWorkerParameters);
typedef PTTAPI_WORKER_FUNC LPPTTAPI_WORKER_FUNC;

#ifdef XRCPU_PIPE_EXPORTS
#define TTAPI
//__declspec(dllexport)
#else // XRCPU_PIPE_EXPORTS
	#define TTAPI
//__declspec(dllimport)
#endif // XRCPU_PIPE_EXPORTS

extern "C" {

// Initializes subsystem
// Returns zero for error, and number of workers on success
DWORD TTAPI ttapi_Init(_processor_info* ID);

// Destroys subsystem
VOID TTAPI ttapi_Done();

// Return number of workers
DWORD TTAPI ttapi_GetWorkersCount();

// Adds new task
// No more than TTAPI_HARDCODED_THREADS should be added
VOID TTAPI ttapi_AddWorker(LPPTTAPI_WORKER_FUNC lpWorkerFunc, LPVOID lpvWorkerFuncParams);

// Runs and wait for all workers to complete job
VOID TTAPI ttapi_RunAllWorkers();

}

#endif // _TTAPI_H_INCLUDED_
