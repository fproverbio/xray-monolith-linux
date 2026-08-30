#pragma once

// Real implementation (NotificationClient.cpp) is Windows COM/WASAPI audio-
// device hotplug detection (IMMNotificationClient, <mmdeviceapi.h> - doesn't
// exist on Linux at all, and isn't a portability-shim job: it's a real,
// per-backend feature, not something with a generic Win32 stand-in).
// SoundRender_Core::bPendingDefaultDeviceSwitch/bPendingDeviceListRefresh
// (SoundRender_Core.h) already exist and are already polled every frame by
// the already-wired-in SoundRender_Core_Processor.cpp - this stub just never
// sets them, so device hotplug is silently unimplemented rather than half-
// wired. A real ALSA/PulseAudio-specific hotplug listener would be the
// eventual replacement, same category as xr_input_xinput.cpp's "reports no
// controller" treatment or xrEngine's SDL2 monitor-hotplug handling (which
// DID get a real implementation, since SDL2 already models it portably) -
// OpenAL's own device enumeration (OpenALDeviceList.cpp, already wired in)
// already covers the primary "which device" question regardless.
class CNotificationClient
{
public:
	CNotificationClient() {}
	~CNotificationClient() {}
};
