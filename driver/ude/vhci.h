/*
 * Copyright (C) 2022 Vadym Hrynchyshyn <vadimgrn@gmail.com>
 */

#pragma once

#include <libdrv\codeseg.h>
#include <wdf.h>

#include <usb.h>
#include <wdfusb.h>
#include <UdeCx.h>

#include <libdrv\wdf_cpp.h>

namespace usbip
{

_Function_class_(EVT_WDF_DRIVER_DEVICE_ADD)
_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
PAGED NTSTATUS DriverDeviceAdd(_In_ WDFDRIVER, _Inout_ WDFDEVICE_INIT *DeviceInit);

} // namespace usbip


namespace usbip::vhci
{

_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
PAGED int claim_roothub_port(_In_ UDECXUSBDEVICE devs);

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
wdf::ObjectRef find_device(_In_ WDFDEVICE vhci, _In_ int port);

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
void reclaim_roothub_port(_In_ UDECXUSBDEVICE dev);

_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
PAGED void destroy_all_devices(_In_ WDFDEVICE vhci);

} // namespace usbip::vhci
