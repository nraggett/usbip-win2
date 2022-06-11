#include "irp.h"
#include "trace.h"
#include "irp.tmh"

#include "dev.h"
#include "usbip_network.h"

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

void complete_canceled_irp(IRP *irp)
{
	TraceMsg("%04x", ptr4log(irp));
	usbip::free_transfer_buffer_mdl(irp);

	irp->IoStatus.Status = STATUS_CANCELLED;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
}

void set_context(IRP *irp, seqnum_t seqnum, irp_status_t status, USBD_PIPE_HANDLE hpipe)
{
	auto &flags = get_flags(irp);

	auto irql = KeGetCurrentIrql();
	flags = irql;

	NT_ASSERT((flags & F_IRQL_MASK) == irql);
	NT_ASSERT(!(flags & F_FREE_MDL));

	get_seqnum(irp) = seqnum;
	*get_status(irp) = status;
	get_pipe_handle(irp) = hpipe;
}
