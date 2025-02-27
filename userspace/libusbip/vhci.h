/*
 * Copyright (C) 2021 - 2023 Vadym Hrynchyshyn <vadimgrn@gmail.com>
 */

#pragma once

#include "dllspec.h"
#include "win_handle.h"
#include <usbspec.h>

#include <string>
#include <vector>

/*
 * Strings encoding is UTF8. 
 */

namespace usbip
{

struct device_location
{
        std::string hostname;
        std::string service; // TCP/IP port number or symbolic name
        std::string busid;
};

struct imported_device
{
        device_location location;
        int port; // hub port number, >= 1

        UINT32 devid;
        USB_DEVICE_SPEED speed;

        UINT16 vendor;
        UINT16 product;
};

} // namespace usbip


namespace usbip::vhci
{

/**
 * Open driver's device interface
 * @return handle, call GetLastError() if it is invalid
 */
USBIP_API Handle open();

/**
 * @param dev handle of the driver device
 * @param success call GetLastError() if false is returned
 * @return imported devices
 */
USBIP_API std::vector<imported_device> get_imported_devices(_In_ HANDLE dev, _Out_ bool &success);

/**
 * @param dev handle of the driver device
 * @param location remote device to attach to
 * @return hub port number, >= 1. Call GetLastError() if zero is returned. 
 */
USBIP_API int attach(_In_ HANDLE dev, _In_ const device_location &location);

/**
 * @param dev handle of the driver device
 * @param port hub port number, <= 0 means detach all ports
 * @return call GetLastError() if false is returned
 */
USBIP_API bool detach(_In_ HANDLE dev, _In_ int port);

} // namespace usbip::vhci
