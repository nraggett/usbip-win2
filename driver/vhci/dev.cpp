#include "dev.h"
#include "trace.h"
#include "dev.tmh"
#include "pageable.h"
#include "vhci.h"
#include "strutil.h"

#include <wdmsec.h>
#include <initguid.h> // required for GUID definitions

DEFINE_GUID(GUID_SD_USBIP_VHCI,
	0x9d3039dd, 0xcca5, 0x4b4d, 0xb3, 0x3d, 0xe2, 0xdd, 0xc8, 0xa8, 0xc5, 0x2f);

namespace
{

const ULONG ext_sizes_per_devtype[] = {
	sizeof(root_dev_t),
	sizeof(cpdo_dev_t),
	sizeof(vhci_dev_t),
	sizeof(hpdo_dev_t),
	sizeof(vhub_dev_t),
	sizeof(vpdo_dev_t)
};

} // namespace


/*
 * Use ExFreePool to free result because libdrv_strdupW uses different tag for memory allocation.
 */
LPWSTR GetDevicePropertyString(DEVICE_OBJECT *pdo, DEVICE_REGISTRY_PROPERTY prop, ULONG &ResultLength)
{
	wchar_t data[0xFF];
	ResultLength = sizeof(data);

	auto free = [data] (auto ptr) 
	{ 
		NT_ASSERT(ptr);
		if (ptr != data) {
			ExFreePoolWithTag(ptr, USBIP_VHCI_POOL_TAG);
		}
	};

	for (auto buf = data; buf; ) {

		switch (auto st = IoGetDeviceProperty(pdo, prop, ResultLength, buf, &ResultLength)) {
		case STATUS_SUCCESS:
			return buf == data ? libdrv_strdupW(PagedPool, buf) : buf;
		case STATUS_BUFFER_TOO_SMALL:
			free(buf);
			buf = (wchar_t*)ExAllocatePoolWithTag(PagedPool, ResultLength, USBIP_VHCI_POOL_TAG);
			break;
		default:
			Trace(TRACE_LEVEL_ERROR, "%!DEVICE_REGISTRY_PROPERTY! -> %!STATUS!", prop, st);
			free(buf);
			return nullptr;
		}
	}

	Trace(TRACE_LEVEL_ERROR, "%!DEVICE_REGISTRY_PROPERTY! -> insufficient resources", prop);
	return nullptr;
}

PAGEABLE PDEVICE_OBJECT vdev_create(DRIVER_OBJECT *drvobj, vdev_type_t type)
{
	PAGED_CODE();

	DEVICE_OBJECT *devobj{};
	auto extsize = ext_sizes_per_devtype[type];
	NTSTATUS status{};

	switch (type) {
	case VDEV_CPDO:
	case VDEV_HPDO:
	case VDEV_VPDO:
		status = IoCreateDeviceSecure(drvobj, extsize, nullptr,
				FILE_DEVICE_BUS_EXTENDER, FILE_AUTOGENERATED_DEVICE_NAME | FILE_DEVICE_SECURE_OPEN,
				FALSE, &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RWX_RES_RWX, // allow normal users to access the devices
				&GUID_SD_USBIP_VHCI, &devobj);
		break;
	default:
		status = IoCreateDevice(drvobj, extsize, nullptr,
					FILE_DEVICE_BUS_EXTENDER, FILE_DEVICE_SECURE_OPEN, TRUE, &devobj);
	}

	if (!NT_SUCCESS(status)) {
		Trace(TRACE_LEVEL_ERROR, "Failed to create vdev(%!vdev_type_t!): %!STATUS!", type, status);
		return nullptr;
	}

	auto vdev = to_vdev(devobj);

	vdev->PnPState = pnp_state::NotStarted;
	vdev->PreviousPnPState = pnp_state::NotStarted;

	vdev->type = type;
	vdev->Self = devobj;

	vdev->DevicePowerState = PowerDeviceUnspecified;
	vdev->SystemPowerState = PowerSystemWorking;

	devobj->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO;

	return devobj;
}

cpdo_dev_t *to_cpdo_or_null(DEVICE_OBJECT *devobj)
{
	auto vdev = to_vdev(devobj);
	NT_ASSERT(vdev);
	return vdev->type == VDEV_CPDO ? static_cast<cpdo_dev_t*>(vdev) : nullptr;
}

vhci_dev_t *to_vhci_or_null(DEVICE_OBJECT *devobj)
{
	auto vdev = to_vdev(devobj);
	NT_ASSERT(vdev);
	return vdev->type == VDEV_VHCI ? static_cast<vhci_dev_t*>(vdev) : nullptr;
}

hpdo_dev_t *to_hpdo_or_null(DEVICE_OBJECT *devobj)
{
	auto vdev = to_vdev(devobj);
	NT_ASSERT(vdev);
	return vdev->type == VDEV_HPDO ? static_cast<hpdo_dev_t*>(vdev) : nullptr;
}

vhub_dev_t *to_vhub_or_null(DEVICE_OBJECT *devobj)
{
	auto vdev = to_vdev(devobj);
	NT_ASSERT(vdev);
	return vdev->type == VDEV_VHUB ? static_cast<vhub_dev_t*>(vdev) : nullptr;
}

vpdo_dev_t *to_vpdo_or_null(DEVICE_OBJECT *devobj)
{
	auto vdev = to_vdev(devobj);
	NT_ASSERT(vdev);
	return vdev->type == VDEV_VPDO ? static_cast<vpdo_dev_t*>(vdev) : nullptr;
}
