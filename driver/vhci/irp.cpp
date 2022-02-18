#include "irp.h"
#include "trace.h"
#include "irp.tmh"

#include "dev.h"

namespace
{

/*
 * If the lower driver didn't return STATUS_PENDING, we don't need to
 * set the event because we won't be waiting on it.
 * This optimization avoids grabbing the dispatcher lock and improves perf.
 */
NTSTATUS irp_completion_routine(__in PDEVICE_OBJECT, __in PIRP irp, __in PVOID Context)
{
	if (irp->PendingReturned) {
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	}

	return StopCompletion;
}

} // namespace


PAGEABLE NTSTATUS irp_pass_down(DEVICE_OBJECT *devobj, IRP *irp)
{
	PAGED_CODE();
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver(devobj, irp);
}

/*
 * Wait for lower drivers to be done with the Irp.
 * Important thing to note here is when you allocate the memory for an event in the stack 
 * you must do a KernelMode wait instead of UserMode to prevent the stack from getting paged out.
 * 
 * See: IoForwardIrpSynchronously
 */
PAGEABLE NTSTATUS irp_send_synchronously(DEVICE_OBJECT *devobj, IRP *irp)
{
	PAGED_CODE();

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(irp);
	IoSetCompletionRoutine(irp, irp_completion_routine, &event, TRUE, TRUE, TRUE);

	auto status = IoCallDriver(devobj, irp);

	if (status == STATUS_PENDING) {
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, nullptr);
		status = irp->IoStatus.Status;
	}

	return status;
}

NTSTATUS CompleteRequest(IRP *irp, NTSTATUS status)
{
	NT_ASSERT(status != STATUS_PENDING);
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

void complete_canceled_irp(vpdo_dev_t*, IRP *irp)
{
	TraceCall("irp %p", irp);

	auto &r = irp->IoStatus;
	r.Status = STATUS_CANCELLED;
	r.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
}
