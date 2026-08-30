#include "stdafx.h"
#pragma hdrstop

#include "ttapi.h"
#include <thread>

typedef struct TTAPI_WORKER_PARAMS
{
	volatile LONG vlFlag;
	LPPTTAPI_WORKER_FUNC lpWorkerFunc;
	LPVOID lpvWorkerFuncParams;
	DWORD dwPadding[13];
}* PTTAPI_WORKER_PARAMS;

typedef PTTAPI_WORKER_PARAMS LPTTAPI_WORKER_PARAMS;

// Real OS threads, not the eventfd/inotify-backed HANDLEs posix_sync.h's
// WaitForMultipleObjects() models - a Windows thread HANDLE becomes
// "signaled" on thread exit, semantically a join(), which std::thread
// models directly and far more simply than faking up pollable fds per
// worker thread would.
static std::thread* ttapi_threads = NULL;
static BOOL ttapi_initialized = FALSE;
static DWORD ttapi_workers_count = 0;
static DWORD ttapi_threads_count = 0;
static DWORD ttapi_assigned_workers = 0;
static LPTTAPI_WORKER_PARAMS ttapi_worker_params = NULL;

static DWORD ttapi_dwFastIter = 0;
static DWORD ttapi_dwSlowIter = 0;

struct
{
	volatile LONG size;
	DWORD dwPadding[15];
} ttapi_queue_size;

DWORD ttapiThreadProc(LPVOID lpParameter)
{
	LPTTAPI_WORKER_PARAMS pParams = (LPTTAPI_WORKER_PARAMS)lpParameter;
	DWORD i, dwFastIter = ttapi_dwFastIter, dwSlowIter = ttapi_dwSlowIter;

	while (TRUE)
	{
		// Wait

		// Fast
		for (i = 0; i < dwFastIter; ++i)
		{
			if (pParams->vlFlag == 0)
			{
				// Msg( "0x%8.8X Fast %u" , dwId , i );
				goto process;
			}
			_mm_pause();
		}

		// Moderate
		for (i = 0; i < dwSlowIter; ++i)
		{
			if (pParams->vlFlag == 0)
			{
				// Msg( "0x%8.8X Moderate %u" , dwId , i );
				goto process;
			}
			std::this_thread::yield();
		}

		// Slow
		while (pParams->vlFlag)
		{
			Sleep(100);
			//Msg( "Shit" );
		}

	process:

		pParams->vlFlag = 1;

		if (pParams->lpWorkerFunc)
			pParams->lpWorkerFunc(pParams->lpvWorkerFuncParams);
		else
			break;

		_InterlockedDecrement(&ttapi_queue_size.size);
	} // while

	return 0;
}

DWORD ttapi_Init(_processor_info* ID)
{
	if (ttapi_initialized)
		return ttapi_workers_count;

	// 1. Use standard library to detect cores (More reliable than legacy cpuid structs)
	ttapi_workers_count = std::thread::hardware_concurrency();

	// Fallback if detection fails
	if (ttapi_workers_count == 0) ttapi_workers_count = 4;

	// 2. Handle Command Line Override (-max-threads)
	// Kept for backward compatibility with launcher/user configs
	char szSearchFor[] = "-max-threads";
	char* pszTemp = strstr(GetCommandLine(), szSearchFor);
	DWORD dwOverride = 0;
	if (pszTemp)
		if (sscanf_s(pszTemp + strlen(szSearchFor), "%u", &dwOverride))
			if ((dwOverride >= 1) && (dwOverride < ttapi_workers_count))
				ttapi_workers_count = dwOverride;

	// 3. Set Spin-Loop Constants directly
	// OLD: Calibrated at runtime by freezing the PC for 1 second.
	// NEW: Set to reasonable constants for modern architectures.
	// 4096 is a standard spin count for _mm_pause loops before yielding.
	ttapi_dwFastIter = 4096;
	ttapi_dwSlowIter = 0; // Modern schedulers prefer immediate yield over slow-spinning

	// 4. Allocate Control Structures
	// Helper threads count
	ttapi_threads_count = ttapi_workers_count - 1;

	ttapi_threads = new std::thread[ttapi_threads_count];
	if ((ttapi_worker_params = (PTTAPI_WORKER_PARAMS)malloc(sizeof(TTAPI_WORKER_PARAMS) * ttapi_workers_count)) == NULL)
		return 0;

	// Clear params
	memset(ttapi_worker_params, 0, sizeof(TTAPI_WORKER_PARAMS) * ttapi_workers_count);

	// 5. Create Threads WITHOUT Affinity Masking
	// let the Windows Scheduler decide where to put threads.
	char szThreadName[64];

	for (DWORD i = 0; i < ttapi_threads_count; i++)
	{
		ttapi_worker_params[i].vlFlag = 1;

		ttapi_threads[i] = std::thread(ttapiThreadProc, &ttapi_worker_params[i]);

		// Modern Thread Naming: pthread_setname_np is the direct portable
		// equivalent of the old RaiseException(0x406D1388, ...) "magic
		// exception" trick MSVC debuggers used to sniff for - much
		// cleaner, no SEH involved, and it works whether or not a
		// debugger is attached. Linux thread names are capped at 15
		// bytes + NUL (TASK_COMM_LEN) - "Worker #%u" instead of the
		// original "Helper Thread #%u" to stay under that even for
		// large worker counts.
		sprintf_s(szThreadName, "Worker #%u", i);
		pthread_setname_np(ttapi_threads[i].native_handle(), szThreadName);
	}

	ttapi_initialized = TRUE;
	return ttapi_workers_count;
}

DWORD ttapi_GetWorkersCount()
{
	return ttapi_workers_count;
}

// We do not check for overflow here to be faster
// Assume that caller is smart enough to use ttapi_GetWorkersCount() to get number of available slots
VOID ttapi_AddWorker(LPPTTAPI_WORKER_FUNC lpWorkerFunc, LPVOID lpvWorkerFuncParams)
{
	// Assigning parameters
	ttapi_worker_params[ttapi_assigned_workers].lpWorkerFunc = lpWorkerFunc;
	ttapi_worker_params[ttapi_assigned_workers].lpvWorkerFuncParams = lpvWorkerFuncParams;

	ttapi_assigned_workers++;
}

VOID ttapi_RunAllWorkers()
{
	DWORD ttapi_thread_workers = (ttapi_assigned_workers - 1);
	//unsigned __int64 Start,Stop;

	if (ttapi_thread_workers)
	{
		// Setting queue size
		ttapi_queue_size.size = ttapi_thread_workers;

		// Starting all workers except the last
		for (DWORD i = 0; i < ttapi_thread_workers; ++i)
			_InterlockedExchange(&ttapi_worker_params[i].vlFlag, 0);

		// Running last worker in current thread
		ttapi_worker_params[ttapi_thread_workers].lpWorkerFunc(
			ttapi_worker_params[ttapi_thread_workers].lpvWorkerFuncParams);

		// Waiting task queue to become empty
		//Start = __rdtsc();
		while (ttapi_queue_size.size)
			_mm_pause();
		//Stop = __rdtsc();
		//Msg( "Wait: %u ticks" , Stop - Start );
	}
	else
		// Running the only worker in current thread
		ttapi_worker_params[ttapi_thread_workers].lpWorkerFunc(
			ttapi_worker_params[ttapi_thread_workers].lpvWorkerFuncParams);

	// Cleaning active workers count
	ttapi_assigned_workers = 0;
}

VOID ttapi_Done()
{
	if (! ttapi_initialized)
		return;

	// Asking helper threads to terminate
	for (DWORD i = 0; i < ttapi_threads_count; i++)
	{
		ttapi_worker_params[i].lpWorkerFunc = NULL;
		_InterlockedExchange(&ttapi_worker_params[i].vlFlag, 0);
	}

	// Waiting threads for completion
	for (DWORD i = 0; i < ttapi_threads_count; i++)
		ttapi_threads[i].join();

	// Freeing resources
	delete[] ttapi_threads;
	ttapi_threads = NULL;
	free(ttapi_worker_params);
	ttapi_worker_params = NULL;

	ttapi_workers_count = 0;
	ttapi_threads_count = 0;
	ttapi_assigned_workers = 0;

	ttapi_initialized = FALSE;
}
