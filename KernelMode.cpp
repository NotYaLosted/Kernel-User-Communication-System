#include <ntifs.h>

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(
		PEPROCESS SourceProcess,
		PVOID SourceAddress,
		PEPROCESS TargetProcess,
		PVOID TargetAddress,
		SIZE_T BufferSize,
		KPROCESSOR_MODE PreviousMode,
		PSIZE_T ReturnSize
	);
}

void debug_kernel(PCSTR text) {
	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));
}

namespace driver {

	namespace codes {
		const ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x775, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x776, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		const ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x777, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct Request {
		HANDLE process_id;
		PVOID target;
		PVOID buffer;
		SIZE_T size;
		SIZE_T return_size;
	};

	PEPROCESS target_process = nullptr;

	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return irp->IoStatus.Status;
	}

	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return irp->IoStatus.Status;
	}

	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		auto stack = IoGetCurrentIrpStackLocation(irp);
		auto request = (Request*)irp->AssociatedIrp.SystemBuffer;

		if (!stack || !request) {
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_INVALID_PARAMETER;
		}

		NTSTATUS status = STATUS_UNSUCCESSFUL;
		const ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

		switch (code) {

		case codes::attach:
			status = PsLookupProcessByProcessId(request->process_id, &target_process);
			break;

		case codes::read:
			if (target_process) {
				status = MmCopyVirtualMemory(
					target_process,
					request->target,
					PsGetCurrentProcess(),
					request->buffer,
					request->size,
					KernelMode,
					&request->return_size
				);
			}
			break;

		case codes::write:
			if (target_process) {
				status = MmCopyVirtualMemory(
					PsGetCurrentProcess(),
					request->buffer,
					target_process,
					request->target,
					request->size,
					KernelMode,
					&request->return_size
				);
			}
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(Request);

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return status;
	}
}

NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING device_name;
	RtlInitUnicodeString(&device_name, L"\\Device\\AssholeDriver");

	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(
		driver_object,
		0,
		&device_name,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&device_object
	);

	if (status != STATUS_SUCCESS)
		return status;

	UNICODE_STRING sym_link;
	RtlInitUnicodeString(&sym_link, L"\\DosDevices\\AssholeDriver");

	status = IoCreateSymbolicLink(&sym_link, &device_name);
	if (status != STATUS_SUCCESS)
		return status;

	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		driver_object->MajorFunction[i] = [](PDEVICE_OBJECT dev, PIRP irp) {
			UNREFERENCED_PARAMETER(dev);
			irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return irp->IoStatus.Status;
			};
	}

	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	debug_kernel("[AssholeDriver] Loaded\n");

	return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry() {
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, L"\\Driver\\AssholeDriver");
	return IoCreateDriver(&name, &driver_main);
}
