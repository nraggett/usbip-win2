#pragma once

#include <wdm.h>

struct vpdo_dev_t;
NTSTATUS send_to_server(vpdo_dev_t *vpdo, IRP *irp);