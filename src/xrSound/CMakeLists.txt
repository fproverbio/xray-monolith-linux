add_library(xrSound STATIC)

target_include_directories(xrSound PUBLIC .)

# guids.cpp intentionally excluded: exists purely to instantiate
# DirectSound GUID constants (`#define INITGUID` + `#include <dsound.h>`
# in exactly one .cpp, the classic Windows trick to get extern GUID
# declarations to resolve at link time). Dead weight now that the
# renderer backend is OpenAL, not DirectSound - dsound.h doesn't exist
# on Linux and nothing in this build references these GUIDs anyway.
#
# SoundRender_Core.cpp/SoundRender_CoreA.cpp (2026-08-30): wired in for
# real. Both were excluded pending xrEngine's console subsystem
# (SoundRender_CoreA.cpp's Console->Execute(...) call, ../xrEngine/
# XR_IOConsole.h) - that's long since wired in (xr_ioc_cmd.cpp already
# registers all of the psReverb*/psSound*/snd_efx_* console commands
# these two files define the backing globals for, which is exactly why
# their absence surfaced as undefined-symbol link errors, not a compile
# gap). SoundRender_Core.cpp's ../xrEngine/xrLevel.h include turned out
# to compile fine as-is (xrEngine now exists in full). The one real fix
# needed: NotificationClient.h/.cpp (Windows COM/WASAPI audio-device
# hotplug detection, IMMNotificationClient/<mmdeviceapi.h> - a real,
# per-backend feature with no portable Win32-shim equivalent, not
# something to blanket-port) is replaced by a trivial stub class with the
# same name and a no-op constructor/destructor - SoundRender_Core.h's
# own bPendingDefaultDeviceSwitch/bPendingDeviceListRefresh flags (already
# polled every frame by the already-wired-in SoundRender_Core_Processor.cpp)
# simply never get set, so hotplug is silently unimplemented rather than
# half-wired; OpenAL's own device enumeration (OpenALDeviceList.cpp)
# already covers the primary "which device" question regardless. The real
# NotificationClient.cpp (COM-based) stays on disk, unbuilt.
target_sources_grouped(
  TARGET xrSound
  NAME "Sound"
  FILES
  OpenALDeviceList.cpp
  sound.cpp
  SoundRender_Cache.cpp
  SoundRender_Core.cpp
  SoundRender_Core_Processor.cpp
  SoundRender_Core_SourceManager.cpp
  SoundRender_Core_StartStop.cpp
  SoundRender_CoreA.cpp
  SoundRender_Emitter.cpp
  SoundRender_Emitter_FSM.cpp
  SoundRender_Emitter_StartStop.cpp
  SoundRender_Emitter_streamer.cpp
  SoundRender_Environment.cpp
  SoundRender_Source.cpp
  SoundRender_Source_loader.cpp
  SoundRender_Target.cpp
  SoundRender_TargetA.cpp
  stdafx.cpp
)

target_link_libraries(xrSound PUBLIC xrCore xrCDB vorbisfile OpenAL)
