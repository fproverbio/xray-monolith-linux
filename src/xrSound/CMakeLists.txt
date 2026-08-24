add_library(xrSound STATIC)

target_include_directories(xrSound PUBLIC .)

# guids.cpp intentionally excluded: exists purely to instantiate
# DirectSound GUID constants (`#define INITGUID` + `#include <dsound.h>`
# in exactly one .cpp, the classic Windows trick to get extern GUID
# declarations to resolve at link time). Dead weight now that the
# renderer backend is OpenAL, not DirectSound - dsound.h doesn't exist
# on Linux and nothing in this build references these GUIDs anyway.
#
# SoundRender_Core.cpp/SoundRender_CoreA.cpp intentionally excluded for
# this pass: SoundRender_CoreA.cpp needs the real CConsole (extern
# CConsole* Console; Console->Execute(...)) from ../xrEngine/XR_IOConsole.h
# to register reverb/EAX console commands - xrEngine's console subsystem
# isn't ported yet. SoundRender_Core.cpp's ../xrEngine/xrLevel.h include
# looks vestigial (no symbol from it is actually referenced in the file)
# but excluded alongside CoreA for now since they're the two halves of
# the same CSoundRender_Core class. Revisit alongside xrEngine.
#
# NotificationClient.cpp/.h intentionally excluded: a Windows COM/WASAPI
# IMMNotificationClient implementation (audio-device hotplug detection),
# built on <mmdeviceapi.h> which doesn't exist on Linux at all - not a
# portability shim job, a real feature that only makes sense per-backend
# (OpenAL's own device enumeration already covers the "which device"
# question; hotplug notification would be a real ALSA/PulseAudio-specific
# feature to add later). Only referenced from SoundRender_Core(A), both
# already excluded above, so nothing else needs it.
target_sources_grouped(
  TARGET xrSound
  NAME "Sound"
  FILES
  OpenALDeviceList.cpp
  sound.cpp
  SoundRender_Cache.cpp
  SoundRender_Core_Processor.cpp
  SoundRender_Core_SourceManager.cpp
  SoundRender_Core_StartStop.cpp
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
