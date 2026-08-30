#include "stdafx.h"
#pragma hdrstop

#ifdef DEBUG

// Named-mailslot IPC (CreateMailslot/GetMailslotInfo/WriteFile) is a Windows-
// only mechanism with no portable equivalent, used here only to let an
// external process inject console commands into a running DEBUG build via
// msParse(). Not worth a real IPC port for a debug-only remote-console
// feature, so this is stubbed to a no-op rather than ported; msCreate()/
// msRead() are kept as functions (not removed) since Engine.cpp/EventAPI.cpp
// call them unconditionally under their own #ifdef DEBUG guards. msWrite()
// (the write side of the same mechanism, meant to run in the external
// process) had no callers anywhere in this source tree and was dropped.
void msCreate(LPCSTR name)
{
}

void msRead(void)
{
}

#endif
