#include "busenum.h"
#include <wdmguid.h>
#include <usb.h>
#include <usbioctl.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, Bus_EvtDeviceAdd)
#pragma alloc_text (PAGE, Bus_EvtIoDefault)
#pragma alloc_text (PAGE, Bus_FileCleanup)
#pragma alloc_text (PAGE, Bus_PlugInDevice)
#pragma alloc_text (PAGE, Bus_UnPlugDevice)
#pragma alloc_text (PAGE, Bus_EjectDevice)
#endif

//
// Driver entry routine.
// 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDFDRIVER driver;

    KdPrint(("Virtual Gamepad Emulation Bus Driver [built: %s %s]\n", __DATE__, __TIME__));

    WDF_DRIVER_CONFIG_INIT(&config, Bus_EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfDriverCreate failed with status 0x%x\n", status));
    }

    return status;
}

//
// Bus-device creation routine.
// 
NTSTATUS Bus_EvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
    WDF_CHILD_LIST_CONFIG config;
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    PNP_BUS_INFORMATION busInfo;
    WDFQUEUE queue;
    WDF_FILEOBJECT_CONFIG foConfig;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint(("Bus_EvtDeviceAdd: 0x%p\n", Driver));

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
    // More than one process may talk to the bus at the same time
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    // Prepare child list
    {
        WDF_CHILD_LIST_CONFIG_INIT(&config, sizeof(PDO_IDENTIFICATION_DESCRIPTION), Bus_EvtDeviceListCreatePdo);

        config.EvtChildListIdentificationDescriptionCompare = Bus_EvtChildListIdentificationDescriptionCompare;

        WdfFdoInitSetDefaultChildListConfig(DeviceInit, &config, WDF_NO_OBJECT_ATTRIBUTES);
    }

    // Assign File Object Configuration
    {
        WDF_FILEOBJECT_CONFIG_INIT(&foConfig, NULL, NULL, Bus_FileCleanup);

        WdfDeviceInitSetFileObjectConfig(DeviceInit, &foConfig, WDF_NO_OBJECT_ATTRIBUTES);
    }

    // Create FDO
    {
        status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("Error creating device 0x%x\n", status));
            return status;
        }
    }

    // Create default I/O queue for FDO
    {
        WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

        queueConfig.EvtIoDeviceControl = Bus_EvtIoDeviceControl;
        queueConfig.EvtIoDefault = Bus_EvtIoDefault;

        __analysis_assume(queueConfig.EvtIoStop != 0);
        status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
        __analysis_assume(queueConfig.EvtIoStop == 0);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfIoQueueCreate failed status 0x%x\n", status));
            return status;
        }
    }

    // Expose FDO interface
    {
        status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_BUSENUM_VIGEM, NULL);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfDeviceCreateDeviceInterface failed status 0x%x\n", status));
            return status;
        }
    }

    busInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
    busInfo.LegacyBusType = PNPBus;
    busInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(device, &busInfo);

    return status;
}

//
// Gets called when the user-land process exits or closes the handle.
// 
_Use_decl_annotations_
VOID
Bus_FileCleanup(
    WDFFILEOBJECT FileObject
)
{
    WDFDEVICE device, hChild;
    NTSTATUS status;
    WDFCHILDLIST list;
    WDF_CHILD_LIST_ITERATOR iterator;
    WDF_CHILD_RETRIEVE_INFO childInfo;
    PDO_IDENTIFICATION_DESCRIPTION description;

    PAGED_CODE();


    KdPrint(("Bus_FileCleanup called\n"));

    device = WdfFileObjectGetDevice(FileObject);

    list = WdfFdoGetDefaultChildList(device);

    WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

    WdfChildListBeginIteration(list, &iterator);

    for (;;)
    {
        WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        status = WdfChildListRetrieveNextDevice(list, &iterator, &hChild, &childInfo);
        if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        // Only unplug owned children
        if (childInfo.Status == WdfChildListRetrieveDeviceSuccess
            && description.OwnerProcessId == CURRENT_PROCESS_ID())
        {
            // "Unplug" child
            status = WdfChildListUpdateChildDescriptionAsMissing(list, &description.Header);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("WdfChildListUpdateChildDescriptionAsMissing failed with status 0x%X\n", status));
            }
        }
    }

    WdfChildListEndIteration(list, &iterator);
}

//
// Responds to I/O control requests sent to the FDO.
// 
VOID Bus_EvtIoDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    WDFDEVICE hDevice;
    size_t length = 0;

    hDevice = WdfIoQueueGetDevice(Queue);

    KdPrint(("Bus_EvtIoDeviceControl: 0x%p\n", hDevice));

    switch (IoControlCode)
    {
    case IOCTL_VIGEM_PLUGIN_TARGET:
    {
        PVIGEM_PLUGIN_TARGET plugIn = NULL;

        KdPrint(("IOCTL_BUSENUM_PLUGIN_HARDWARE\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_PLUGIN_TARGET), (PVOID)&plugIn, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(VIGEM_PLUGIN_TARGET) == plugIn->Size) && (length == InputBufferLength))
        {
            if (plugIn->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_PlugInDevice(hDevice, plugIn->SerialNo, plugIn->TargetType);
        }

        break;
    }

    case IOCTL_VIGEM_UNPLUG_TARGET:
    {
        PVIGEM_UNPLUG_TARGET unPlug = NULL;

        KdPrint(("IOCTL_BUSENUM_UNPLUG_HARDWARE\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_UNPLUG_TARGET), (PVOID)&unPlug, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(VIGEM_UNPLUG_TARGET) == unPlug->Size) && (length == InputBufferLength))
        {
            status = Bus_UnPlugDevice(hDevice, unPlug->SerialNo);
        }

        break;
    }

    case IOCTL_VIGEM_EJECT_TARGET:
    {
        PVIGEM_EJECT_TARGET eject = NULL;

        KdPrint(("IOCTL_BUSENUM_EJECT_HARDWARE\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_EJECT_TARGET), (PVOID)&eject, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if (eject->Size == InputBufferLength)
        {
            status = Bus_EjectDevice(hDevice, eject->SerialNo);
        }

        break;
    }

    case IOCTL_XUSB_SUBMIT_REPORT:
    {
        PXUSB_SUBMIT_REPORT xusbSubmit = NULL;

        KdPrint(("IOCTL_XUSB_SUBMIT_REPORT\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XUSB_SUBMIT_REPORT), (PVOID)&xusbSubmit, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(XUSB_SUBMIT_REPORT) == xusbSubmit->Size) && (length == InputBufferLength))
        {
            // This request only supports a single PDO at a time
            if (xusbSubmit->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_XusbSubmitReport(hDevice, xusbSubmit->SerialNo, xusbSubmit);
        }

        break;
    }

    case IOCTL_XUSB_REQUEST_NOTIFICATION:
    {
        PXUSB_REQUEST_NOTIFICATION xusbNotify = NULL;

        KdPrint(("IOCTL_XUSB_REQUEST_NOTIFICATION\n"));

        // Don't accept the request if the output buffer can't hold the results
        if (OutputBufferLength < sizeof(XUSB_REQUEST_NOTIFICATION))
        {
            KdPrint(("IOCTL_XUSB_REQUEST_NOTIFICATION: output buffer too small: %ul\n", OutputBufferLength));
            break;
        }

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XUSB_REQUEST_NOTIFICATION), (PVOID)&xusbNotify, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(XUSB_REQUEST_NOTIFICATION) == xusbNotify->Size) && (length == InputBufferLength))
        {
            // This request only supports a single PDO at a time
            if (xusbNotify->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_XusbQueueNotification(hDevice, xusbNotify->SerialNo, Request);
        }

        break;
    }

    case IOCTL_DS4_SUBMIT_REPORT:
    {
        PDS4_SUBMIT_REPORT ds4Submit = NULL;

        KdPrint(("IOCTL_DS4_SUBMIT_REPORT\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(DS4_SUBMIT_REPORT), (PVOID)&ds4Submit, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(DS4_SUBMIT_REPORT) == ds4Submit->Size) && (length == InputBufferLength))
        {
            // This request only supports a single PDO at a time
            if (ds4Submit->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_Ds4SubmitReport(hDevice, ds4Submit->SerialNo, ds4Submit);
        }

        break;
    }

    default:
        KdPrint(("UNKNOWN IOCTL CODE 0x%x\n", IoControlCode));
        break; // default status is STATUS_INVALID_PARAMETER
    }

    if (status != STATUS_PENDING)
    {
        WdfRequestCompleteWithInformation(Request, status, length);
    }
}

//
// Catches unsupported requests.
// 
VOID Bus_EvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(Request);

    KdPrint(("Bus_EvtIoDefault called\n"));

    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

//
// Simulates a device plug-in event.
// 
NTSTATUS Bus_PlugInDevice(_In_ WDFDEVICE Device, _In_ ULONG SerialNo, _In_ VIGEM_TARGET_TYPE TargetType)
{
    PDO_IDENTIFICATION_DESCRIPTION description;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the description with the information about the newly
    // plugged in device.
    //
    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

    description.SerialNo = SerialNo;
    description.TargetType = TargetType;
    description.OwnerProcessId = CURRENT_PROCESS_ID();

    status = WdfChildListAddOrUpdateChildDescriptionAsPresent(WdfFdoGetDefaultChildList(Device), &description.Header, NULL);

    if (status == STATUS_OBJECT_NAME_EXISTS)
    {
        //
        // The description is already present in the list, the serial number is
        // not unique, return error.
        //
        status = STATUS_INVALID_PARAMETER;
    }

    KdPrint(("Bus_PlugInDevice exiting with 0x%x\n", status));

    return status;
}

//
// Simulates a device unplug event.
// 
NTSTATUS Bus_UnPlugDevice(WDFDEVICE Device, ULONG SerialNo)
{
    NTSTATUS status;
    WDFDEVICE hChild;
    WDFCHILDLIST list;
    WDF_CHILD_LIST_ITERATOR iterator;
    WDF_CHILD_RETRIEVE_INFO childInfo;
    PDO_IDENTIFICATION_DESCRIPTION description;
    BOOLEAN unplugAll = (SerialNo == 0);

    PAGED_CODE();

    KdPrint(("Entered Bus_UnPlugDevice\n"));

    list = WdfFdoGetDefaultChildList(Device);

    WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

    WdfChildListBeginIteration(list, &iterator);

    for (;;)
    {
        WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);
        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        status = WdfChildListRetrieveNextDevice(list, &iterator, &hChild, &childInfo);

        // Error or no more children, end loop
        if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

        // Child isn't the one we looked for, skip
        if (!unplugAll && description.SerialNo != SerialNo)
        {
            continue;
        }

        // Only unplug owned children
        if (childInfo.Status == WdfChildListRetrieveDeviceSuccess
            && description.OwnerProcessId == CURRENT_PROCESS_ID())
        {
            // Unplug child
            status = WdfChildListUpdateChildDescriptionAsMissing(list, &description.Header);
            if (!NT_SUCCESS(status))
            {
                KdPrint(("WdfChildListUpdateChildDescriptionAsMissing failed with status 0x%X\n", status));
            }
        }
    }

    WdfChildListEndIteration(list, &iterator);

    return status;
}

//
// Simulates a device ejection event.
// 
NTSTATUS Bus_EjectDevice(WDFDEVICE Device, ULONG SerialNo)
{
    WDFDEVICE hChild;
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    WDFCHILDLIST list;

    PAGED_CODE();

    list = WdfFdoGetDefaultChildList(Device);

    //
    // A zero serial number means eject all children
    //
    if (0 == SerialNo)
    {
        WDF_CHILD_LIST_ITERATOR iterator;

        WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrievePresentChildren);

        WdfChildListBeginIteration(list, &iterator);

        for (; ;)
        {
            WDF_CHILD_RETRIEVE_INFO childInfo;
            PDO_IDENTIFICATION_DESCRIPTION description;
            BOOLEAN ret;

            //
            // Init the structures.
            //
            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));
            WDF_CHILD_RETRIEVE_INFO_INIT(&childInfo, &description.Header);

            WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

            //
            // Get the device identification description
            //
            status = WdfChildListRetrieveNextDevice(list, &iterator, &hChild, &childInfo);

            if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_ENTRIES)
            {
                break;
            }

            ASSERT(childInfo.Status == WdfChildListRetrieveDeviceSuccess);

            //
            // Use that description to request an eject.
            //
            ret = WdfChildListRequestChildEject(list, &description.Header);

            if (!ret)
            {
                WDFVERIFY(ret);
            }
        }

        WdfChildListEndIteration(list, &iterator);

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        PDO_IDENTIFICATION_DESCRIPTION description;

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        description.SerialNo = SerialNo;

        if (WdfChildListRequestChildEject(list, &description.Header))
        {
            status = STATUS_SUCCESS;
        }
    }

    return status;
}

//
// Sends a report update to an XUSB PDO.
// 
NTSTATUS Bus_XusbSubmitReport(WDFDEVICE Device, ULONG SerialNo, PXUSB_SUBMIT_REPORT Report)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFCHILDLIST list;
    WDF_CHILD_RETRIEVE_INFO info;
    WDFDEVICE hChild;
    PPDO_DEVICE_DATA pdoData;
    PXUSB_DEVICE_DATA xusbData;
    WDFREQUEST usbRequest;
    PIRP pendingIrp;
    PIO_STACK_LOCATION irpStack;
    BOOLEAN changed;


    KdPrint(("Entered Bus_XusbSubmitReport\n"));

    // Get child
    {
        list = WdfFdoGetDefaultChildList(Device);

        PDO_IDENTIFICATION_DESCRIPTION description;

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        description.SerialNo = SerialNo;

        WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

        hChild = WdfChildListRetrievePdo(list, &info);
    }

    // Validate child
    if (hChild == NULL)
    {
        KdPrint(("Bus_XusbSubmitReport: PDO with serial %d not found\n", SerialNo));
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        KdPrint(("Bus_XusbSubmitReport: PDO context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check XUSB context
    xusbData = XusbGetData(hChild);
    if (xusbData == NULL)
    {
        KdPrint(("Bus_XusbSubmitReport: XUSB context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!IS_OWNER(pdoData))
    {
        KdPrint(("Bus_XusbSubmitReport: PID mismatch: %d != %d\n", pdoData->OwnerProcessId, CURRENT_PROCESS_ID()));
        return STATUS_ACCESS_DENIED;
    }

    // Check if input is different from previous value
    changed = (RtlCompareMemory(xusbData->Report, &Report->Report, sizeof(XUSB_REPORT)) != sizeof(XUSB_REPORT));

    // Don't waste pending IRP if input hasn't changed
    if (changed)
    {
        KdPrint(("Bus_XusbSubmitReport: received new report\n"));

        // Get pending USB request
        status = WdfIoQueueRetrieveNextRequest(xusbData->PendingUsbInRequests, &usbRequest);

        if (NT_SUCCESS(status))
        {
            KdPrint(("Bus_XusbSubmitReport: pending IRP found\n"));

            // Get pending IRP
            pendingIrp = WdfRequestWdmGetIrp(usbRequest);
            irpStack = IoGetCurrentIrpStackLocation(pendingIrp);

            // Get USB request block
            PURB urb = (PURB)irpStack->Parameters.Others.Argument1;

            // Get transfer buffer
            PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;
            // Set buffer length to report size
            urb->UrbBulkOrInterruptTransfer.TransferBufferLength = XUSB_REPORT_SIZE;

            /* Copy report to cache and transfer buffer
             * The first two bytes are always the same, so we skip them */
            RtlCopyBytes(xusbData->Report + 2, &Report->Report, sizeof(XUSB_REPORT));
            RtlCopyBytes(Buffer, xusbData->Report, XUSB_REPORT_SIZE);

            // Complete pending request
            WdfRequestComplete(usbRequest, status);
        }
    }

    return status;
}

//
// Queues an inverted call to receive XUSB-specific updates.
// 
NTSTATUS Bus_XusbQueueNotification(WDFDEVICE Device, ULONG SerialNo, WDFREQUEST Request)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    WDFCHILDLIST list;
    WDF_CHILD_RETRIEVE_INFO info;
    WDFDEVICE hChild;
    PPDO_DEVICE_DATA pdoData;
    PXUSB_DEVICE_DATA xusbData;


    KdPrint(("Entered Bus_XusbQueueNotification\n"));

    // Get child
    {
        list = WdfFdoGetDefaultChildList(Device);

        PDO_IDENTIFICATION_DESCRIPTION description;

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        description.SerialNo = SerialNo;

        WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

        hChild = WdfChildListRetrievePdo(list, &info);
    }

    // Validate child
    if (hChild == NULL)
    {
        KdPrint(("Bus_XusbQueueNotification: PDO with serial %d not found\n", SerialNo));
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        KdPrint(("Bus_XusbQueueNotification: PDO context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!IS_OWNER(pdoData))
    {
        KdPrint(("Bus_XusbQueueNotification: PID mismatch: %d != %d\n", pdoData->OwnerProcessId, CURRENT_PROCESS_ID()));
        return STATUS_ACCESS_DENIED;
    }

    xusbData = XusbGetData(hChild);

    if (xusbData != NULL)
    {
        // Queue the request for later completion by the PDO and return STATUS_PENDING
        status = WdfRequestForwardToIoQueue(Request, xusbData->PendingNotificationRequests);
        if (!NT_SUCCESS(status))
        {
            KdPrint(("WdfRequestForwardToIoQueue failed with status 0x%X\n", status));
        }
    }

    return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
}

//
// Sends a report update to a DS4 PDO.
// 
NTSTATUS Bus_Ds4SubmitReport(WDFDEVICE Device, ULONG SerialNo, PDS4_SUBMIT_REPORT Report)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDFCHILDLIST list;
    WDF_CHILD_RETRIEVE_INFO info;
    WDFDEVICE hChild;
    PPDO_DEVICE_DATA pdoData;
    PDS4_DEVICE_DATA ds4Data;
    WDFREQUEST usbRequest;
    PIRP pendingIrp;
    PIO_STACK_LOCATION irpStack;


    KdPrint(("Entered Bus_Ds4SubmitReport\n"));

    // Get child
    {
        list = WdfFdoGetDefaultChildList(Device);

        PDO_IDENTIFICATION_DESCRIPTION description;

        WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

        description.SerialNo = SerialNo;

        WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

        hChild = WdfChildListRetrievePdo(list, &info);
    }

    // Validate child
    if (hChild == NULL)
    {
        KdPrint(("Bus_Ds4SubmitReport: PDO with serial %d not found\n", SerialNo));
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        KdPrint(("Bus_Ds4SubmitReport: PDO context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check XUSB context
    ds4Data = Ds4GetData(hChild);
    if (ds4Data == NULL)
    {
        KdPrint(("Bus_Ds4SubmitReport: DS4 context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!IS_OWNER(pdoData))
    {
        KdPrint(("Bus_Ds4SubmitReport: PID mismatch: %d != %d\n", pdoData->OwnerProcessId, CURRENT_PROCESS_ID()));
        return STATUS_ACCESS_DENIED;
    }

    // Get pending USB request
    status = WdfIoQueueRetrieveNextRequest(ds4Data->PendingUsbRequests, &usbRequest);

    if (NT_SUCCESS(status))
    {
        KdPrint(("Bus_Ds4SubmitReport: pending IRP found\n"));

        // Get pending IRP
        pendingIrp = WdfRequestWdmGetIrp(usbRequest);
        irpStack = IoGetCurrentIrpStackLocation(pendingIrp);

        // Get USB request block
        PURB urb = (PURB)irpStack->Parameters.Others.Argument1;

        // Get transfer buffer
        PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;
        // Set buffer length to report size
        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DS4_HID_REPORT_SIZE;

        /* Copy report to cache and transfer buffer 
         * Skip first byte as it contains the never changing report id */
        RtlCopyBytes(ds4Data->HidReport + 1, &Report->HidReport, Report->Size);
        RtlCopyBytes(Buffer + 1, &Report->HidReport, Report->Size);

        // Complete pending request
        WdfRequestComplete(usbRequest, status);
    }

    return status;
}

