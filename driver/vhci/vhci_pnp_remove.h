#pragma once

#include "basetype.h"
#include "vhci_dev.h"

PAGEABLE NTSTATUS pnp_remove_device(vdev_t * vdev, PIRP irp);
