/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" H�glinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


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
#endif

//
// Driver entry routine.
// 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;
    WDFDRIVER driver;

    KdPrint((DRIVERNAME "Virtual Gamepad Emulation Bus Driver [built: %s %s]\n", __DATE__, __TIME__));

    WDF_DRIVER_CONFIG_INIT(&config, Bus_EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, &driver);

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfDriverCreate failed with status 0x%x\n", status));
    }

    return status;
}

//
// Bus-device creation routine.
// 
NTSTATUS Bus_EvtDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
    WDF_CHILD_LIST_CONFIG       config;
    NTSTATUS                    status;
    WDFDEVICE                   device;
    WDF_IO_QUEUE_CONFIG         queueConfig;
    PNP_BUS_INFORMATION         busInfo;
    WDFQUEUE                    queue;
    WDF_FILEOBJECT_CONFIG       foConfig;
    VIGEM_INTERFACE_STANDARD    VigemInterface;
    WDF_QUERY_INTERFACE_CONFIG  qiConfig;



    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint((DRIVERNAME "Bus_EvtDeviceAdd: 0x%p\n", Driver));

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
    // More than one process may talk to the bus at the same time
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);
    // Bus is power policy owner over all PDOs
    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, TRUE);

#pragma region Prepare child list

    WDF_CHILD_LIST_CONFIG_INIT(&config, sizeof(PDO_IDENTIFICATION_DESCRIPTION), Bus_EvtDeviceListCreatePdo);

    config.EvtChildListIdentificationDescriptionCompare = Bus_EvtChildListIdentificationDescriptionCompare;

    WdfFdoInitSetDefaultChildListConfig(DeviceInit, &config, WDF_NO_OBJECT_ATTRIBUTES);

#pragma endregion

#pragma region Assign File Object Configuration

    WDF_FILEOBJECT_CONFIG_INIT(&foConfig, NULL, NULL, Bus_FileCleanup);

    WdfDeviceInitSetFileObjectConfig(DeviceInit, &foConfig, WDF_NO_OBJECT_ATTRIBUTES);

#pragma endregion

#pragma region Assign object name

    status = WdfDeviceInitAssignName(DeviceInit, &VigemNtDeviceName);
    if (!NT_SUCCESS(status)) {
        KdPrint((DRIVERNAME "WdfDeviceInitAssignName failed with status 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Create FDO

    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &device);

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "Error creating device 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Create symbolic link

    status = WdfDeviceCreateSymbolicLink(
        device,
        &VigemDosDeviceName
    );
    if (!NT_SUCCESS(status)) {
        KdPrint((DRIVERNAME "WdfDeviceCreateSymbolicLink failed with status 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Create default I/O queue for FDO

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoDeviceControl = Bus_EvtIoDeviceControl;
    queueConfig.EvtIoDefault = Bus_EvtIoDefault;

    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfIoQueueCreate failed status 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Expose FDO interface

    status = WdfDeviceCreateDeviceInterface(device, &GUID_DEVINTERFACE_BUSENUM_VIGEM, NULL);

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfDeviceCreateDeviceInterface failed status 0x%x\n", status));
        return status;
    }

#pragma endregion

#pragma region Set bus information

    busInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
    busInfo.LegacyBusType = PNPBus;
    busInfo.BusNumber = 0;

    WdfDeviceSetBusInformationForChildren(device, &busInfo);

#pragma endregion

#pragma region Add query interface

    RtlZeroMemory(&VigemInterface, sizeof(VIGEM_INTERFACE_STANDARD));

    VigemInterface.Header.Size = sizeof(VigemInterface);
    VigemInterface.Header.Version = 1;
    VigemInterface.Header.Context = (PVOID)device;

    VigemInterface.Header.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    VigemInterface.Header.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    VigemInterface.PlugInTarget = BusIface_PlugInTarget;
    VigemInterface.UnPlugTarget = BufIface_UnplugTarget;
    VigemInterface.XusbSubmitReport = BufIface_XusbSubmitReport;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&qiConfig,
        (PINTERFACE)&VigemInterface,
        &GUID_VIGEM_INTERFACE_STANDARD,
        NULL);

    status = WdfDeviceAddQueryInterface(device, &qiConfig);

    if (!NT_SUCCESS(status)) {
        KdPrint((DRIVERNAME "WdfDeviceAddQueryInterface failed with status 0x%X", status));
        return status;
    }

#pragma endregion 

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
    WDFDEVICE                           device;
    WDFDEVICE                           hChild;
    NTSTATUS                            status;
    WDFCHILDLIST                        list;
    WDF_CHILD_LIST_ITERATOR             iterator;
    WDF_CHILD_RETRIEVE_INFO             childInfo;
    PDO_IDENTIFICATION_DESCRIPTION      description;

    PAGED_CODE();


    KdPrint((DRIVERNAME "Bus_FileCleanup called\n"));

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
                KdPrint((DRIVERNAME "WdfChildListUpdateChildDescriptionAsMissing failed with status 0x%X\n", status));
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
    NTSTATUS    status = STATUS_INVALID_PARAMETER;
    WDFDEVICE   hDevice;
    size_t      length = 0;

    hDevice = WdfIoQueueGetDevice(Queue);

    KdPrint((DRIVERNAME "Bus_EvtIoDeviceControl: 0x%p\n", hDevice));

    switch (IoControlCode)
    {
    case IOCTL_VIGEM_PLUGIN_TARGET:
    {
        PVIGEM_PLUGIN_TARGET plugIn = NULL;

        KdPrint((DRIVERNAME "IOCTL_VIGEM_PLUGIN_TARGET\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_PLUGIN_TARGET), (PVOID)&plugIn, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
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

        KdPrint((DRIVERNAME "IOCTL_VIGEM_UNPLUG_TARGET\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(VIGEM_UNPLUG_TARGET), (PVOID)&unPlug, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(VIGEM_UNPLUG_TARGET) == unPlug->Size) && (length == InputBufferLength))
        {
            status = Bus_UnPlugDevice(hDevice, unPlug->SerialNo);
        }

        break;
    }

    case IOCTL_XUSB_SUBMIT_REPORT:
    {
        PXUSB_SUBMIT_REPORT xusbSubmit = NULL;

        KdPrint((DRIVERNAME "IOCTL_XUSB_SUBMIT_REPORT\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XUSB_SUBMIT_REPORT), (PVOID)&xusbSubmit, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
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

        KdPrint((DRIVERNAME "IOCTL_XUSB_REQUEST_NOTIFICATION\n"));

        // Don't accept the request if the output buffer can't hold the results
        if (OutputBufferLength < sizeof(XUSB_REQUEST_NOTIFICATION))
        {
            KdPrint((DRIVERNAME "IOCTL_XUSB_REQUEST_NOTIFICATION: output buffer too small: %ul\n", OutputBufferLength));
            break;
        }

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XUSB_REQUEST_NOTIFICATION), (PVOID)&xusbNotify, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
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

            status = Bus_QueueNotification(hDevice, xusbNotify->SerialNo, Request);
        }

        break;
    }

    case IOCTL_DS4_SUBMIT_REPORT:
    {
        PDS4_SUBMIT_REPORT ds4Submit = NULL;

        KdPrint((DRIVERNAME "IOCTL_DS4_SUBMIT_REPORT\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(DS4_SUBMIT_REPORT), (PVOID)&ds4Submit, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
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

    case IOCTL_DS4_REQUEST_NOTIFICATION:
    {
        PDS4_REQUEST_NOTIFICATION ds4Notify = NULL;

        KdPrint((DRIVERNAME "IOCTL_DS4_REQUEST_NOTIFICATION\n"));

        // Don't accept the request if the output buffer can't hold the results
        if (OutputBufferLength < sizeof(DS4_REQUEST_NOTIFICATION))
        {
            KdPrint((DRIVERNAME "IOCTL_DS4_REQUEST_NOTIFICATION: output buffer too small: %ul\n", OutputBufferLength));
            break;
        }

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(DS4_REQUEST_NOTIFICATION), (PVOID)&ds4Notify, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(DS4_REQUEST_NOTIFICATION) == ds4Notify->Size) && (length == InputBufferLength))
        {
            // This request only supports a single PDO at a time
            if (ds4Notify->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_QueueNotification(hDevice, ds4Notify->SerialNo, Request);
        }

        break;
    }

    case IOCTL_XGIP_SUBMIT_REPORT:
    {
        PXGIP_SUBMIT_REPORT xgipSubmit = NULL;

        KdPrint((DRIVERNAME "IOCTL_XGIP_SUBMIT_REPORT\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XGIP_SUBMIT_REPORT), (PVOID)&xgipSubmit, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(XGIP_SUBMIT_REPORT) == xgipSubmit->Size) && (length == InputBufferLength))
        {
            // This request only supports a single PDO at a time
            if (xgipSubmit->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_XgipSubmitReport(hDevice, xgipSubmit->SerialNo, xgipSubmit);
        }

        break;
    }

    case IOCTL_XGIP_SUBMIT_INTERRUPT:
    {
        PXGIP_SUBMIT_INTERRUPT xgipSubmit = NULL;

        KdPrint((DRIVERNAME "IOCTL_XGIP_SUBMIT_INTERRUPT\n"));

        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XGIP_SUBMIT_INTERRUPT), (PVOID)&xgipSubmit, &length);

        if (!NT_SUCCESS(status))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed 0x%x\n", status));
            break;
        }

        if ((sizeof(XGIP_SUBMIT_INTERRUPT) == xgipSubmit->Size) && (length == InputBufferLength))
        {
            // This request only supports a single PDO at a time
            if (xgipSubmit->SerialNo == 0)
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            status = Bus_XgipSubmitInterrupt(hDevice, xgipSubmit->SerialNo, xgipSubmit);
        }

        break;
    }

    default:
        KdPrint((DRIVERNAME "UNKNOWN IOCTL CODE 0x%x\n", IoControlCode));
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

    KdPrint((DRIVERNAME "Bus_EvtIoDefault called\n"));

    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

//
// Simulates a device plug-in event.
// 
NTSTATUS Bus_PlugInDevice(WDFDEVICE Device, ULONG SerialNo, VIGEM_TARGET_TYPE TargetType)
{
    PDO_IDENTIFICATION_DESCRIPTION  description;
    NTSTATUS                        status;

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

    KdPrint((DRIVERNAME "Bus_PlugInDevice exiting with 0x%x\n", status));

    return status;
}

//
// Simulates a device unplug event.
// 
NTSTATUS Bus_UnPlugDevice(WDFDEVICE Device, ULONG SerialNo)
{
    NTSTATUS                            status;
    WDFDEVICE                           hChild;
    WDFCHILDLIST                        list;
    WDF_CHILD_LIST_ITERATOR             iterator;
    WDF_CHILD_RETRIEVE_INFO             childInfo;
    PDO_IDENTIFICATION_DESCRIPTION      description;
    BOOLEAN                             unplugAll = (SerialNo == 0);

    PAGED_CODE();

    KdPrint((DRIVERNAME "Entered Bus_UnPlugDevice\n"));

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
                KdPrint((DRIVERNAME "WdfChildListUpdateChildDescriptionAsMissing failed with status 0x%X\n", status));
            }
        }
    }

    WdfChildListEndIteration(list, &iterator);

    return STATUS_SUCCESS;
}

//
// Sends a report update to an XUSB PDO.
// 
NTSTATUS Bus_XusbSubmitReport(WDFDEVICE Device, ULONG SerialNo, PXUSB_SUBMIT_REPORT Report)
{
    return Bus_SubmitReport(Device, SerialNo, Report);
}

//
// Queues an inverted call to receive XUSB-specific updates.
// 
NTSTATUS Bus_QueueNotification(WDFDEVICE Device, ULONG SerialNo, WDFREQUEST Request)
{
    NTSTATUS                    status = STATUS_INVALID_PARAMETER;
    WDFCHILDLIST                list;
    WDF_CHILD_RETRIEVE_INFO     info;
    WDFDEVICE                   hChild;
    PPDO_DEVICE_DATA            pdoData;
    PXUSB_DEVICE_DATA           xusbData;
    PDS4_DEVICE_DATA            ds4Data;


    KdPrint((DRIVERNAME "Entered Bus_QueueNotification\n"));

#pragma region Get PDO from child list

    list = WdfFdoGetDefaultChildList(Device);

    PDO_IDENTIFICATION_DESCRIPTION description;

    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

    description.SerialNo = SerialNo;

    WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

    hChild = WdfChildListRetrievePdo(list, &info);

#pragma endregion

    // Validate child
    if (hChild == NULL)
    {
        KdPrint((DRIVERNAME "Bus_QueueNotification: PDO with serial %d not found\n", SerialNo));
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        KdPrint((DRIVERNAME "Bus_QueueNotification: PDO context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!IS_OWNER(pdoData))
    {
        KdPrint((DRIVERNAME "Bus_QueueNotification: PID mismatch: %d != %d\n", pdoData->OwnerProcessId, CURRENT_PROCESS_ID()));
        return STATUS_ACCESS_DENIED;
    }

    // Queue the request for later completion by the PDO and return STATUS_PENDING
    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        xusbData = XusbGetData(hChild);

        if (xusbData == NULL) break;

        status = WdfRequestForwardToIoQueue(Request, xusbData->PendingNotificationRequests);

        break;
    case DualShock4Wired:

        ds4Data = Ds4GetData(hChild);

        if (ds4Data == NULL) break;

        status = WdfRequestForwardToIoQueue(Request, ds4Data->PendingNotificationRequests);

        break;
    }

    if (!NT_SUCCESS(status))
    {
        KdPrint((DRIVERNAME "WdfRequestForwardToIoQueue failed with status 0x%X\n", status));
    }

    return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
}

//
// Sends a report update to a DS4 PDO.
// 
NTSTATUS Bus_Ds4SubmitReport(WDFDEVICE Device, ULONG SerialNo, PDS4_SUBMIT_REPORT Report)
{
    return Bus_SubmitReport(Device, SerialNo, Report);
}

NTSTATUS Bus_XgipSubmitReport(WDFDEVICE Device, ULONG SerialNo, PXGIP_SUBMIT_REPORT Report)
{
    return Bus_SubmitReport(Device, SerialNo, Report);
}

NTSTATUS Bus_XgipSubmitInterrupt(WDFDEVICE Device, ULONG SerialNo, PXGIP_SUBMIT_INTERRUPT Report)
{
    return Bus_SubmitReport(Device, SerialNo, Report);
}

NTSTATUS Bus_SubmitReport(WDFDEVICE Device, ULONG SerialNo, PVOID Report)
{
    NTSTATUS                    status = STATUS_SUCCESS;
    WDFCHILDLIST                list;
    WDF_CHILD_RETRIEVE_INFO     info;
    WDFDEVICE                   hChild;
    PPDO_DEVICE_DATA            pdoData;
    WDFREQUEST                  usbRequest;
    PIRP                        pendingIrp;
    BOOLEAN                     changed;


    KdPrint((DRIVERNAME "Entered Bus_SubmitReport\n"));

#pragma region Get PDO from child list

    list = WdfFdoGetDefaultChildList(Device);

    PDO_IDENTIFICATION_DESCRIPTION description;

    WDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER_INIT(&description.Header, sizeof(description));

    description.SerialNo = SerialNo;

    WDF_CHILD_RETRIEVE_INFO_INIT(&info, &description.Header);

    hChild = WdfChildListRetrievePdo(list, &info);

#pragma endregion

    // Validate child
    if (hChild == NULL)
    {
        KdPrint((DRIVERNAME "Bus_SubmitReport: PDO with serial %d not found\n", SerialNo));
        return STATUS_NO_SUCH_DEVICE;
    }

    // Check common context
    pdoData = PdoGetData(hChild);
    if (pdoData == NULL)
    {
        KdPrint((DRIVERNAME "Bus_SubmitReport: PDO context not found\n"));
        return STATUS_INVALID_PARAMETER;
    }

    // Check if caller owns this PDO
    if (!IS_OWNER(pdoData))
    {
        KdPrint((DRIVERNAME "Bus_SubmitReport: PID mismatch: %d != %d\n", pdoData->OwnerProcessId, CURRENT_PROCESS_ID()));
        return STATUS_ACCESS_DENIED;
    }

    // Check if input is different from previous value
    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        changed = (RtlCompareMemory(XusbGetData(hChild)->Report,
            &((PXUSB_SUBMIT_REPORT)Report)->Report,
            sizeof(XUSB_REPORT)) != sizeof(XUSB_REPORT));

        break;
    case DualShock4Wired:

        changed = TRUE;

        break;
    case XboxOneWired:

        // TODO: necessary?
        changed = TRUE;

        break;
    default:

        changed = FALSE;

        break;
    }

    // Don't waste pending IRP if input hasn't changed
    if (!changed)
        return status;

    KdPrint((DRIVERNAME "Bus_SubmitReport: received new report\n"));

    // Get pending USB request
    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        status = WdfIoQueueRetrieveNextRequest(XusbGetData(hChild)->PendingUsbInRequests, &usbRequest);

        break;
    case DualShock4Wired:

        status = WdfIoQueueRetrieveNextRequest(Ds4GetData(hChild)->PendingUsbInRequests, &usbRequest);

        break;
    case XboxOneWired:

        // Request is control data
        if (((PXGIP_SUBMIT_INTERRUPT)Report)->Size == sizeof(XGIP_SUBMIT_INTERRUPT))
        {
            PXGIP_DEVICE_DATA xgip = XgipGetData(hChild);
            PXGIP_SUBMIT_INTERRUPT interrupt = (PXGIP_SUBMIT_INTERRUPT)Report;
            WDFMEMORY memory;
            WDF_OBJECT_ATTRIBUTES memAttribs;
            WDF_OBJECT_ATTRIBUTES_INIT(&memAttribs);

            memAttribs.ParentObject = hChild;

            // Allocate kernel memory
            status = WdfMemoryCreate(&memAttribs, NonPagedPool, VIGEM_POOL_TAG,
                interrupt->InterruptLength, &memory, NULL);
            if (!NT_SUCCESS(status))
            {
                KdPrint((DRIVERNAME "WdfMemoryCreate failed with status 0x%X\n", status));
                return status;
            }

            // Copy interrupt buffer to memory object
            status = WdfMemoryCopyFromBuffer(memory, 0, interrupt->Interrupt, interrupt->InterruptLength);
            if (!NT_SUCCESS(status))
            {
                KdPrint((DRIVERNAME "WdfMemoryCopyFromBuffer failed with status 0x%X\n", status));
                return status;
            }

            // Add memory object to collection
            status = WdfCollectionAdd(xgip->XboxgipSysInitCollection, memory);
            if (!NT_SUCCESS(status))
            {
                KdPrint((DRIVERNAME "WdfCollectionAdd failed with status 0x%X\n", status));
                return status;
            }

            // Check if all packets have been received
            xgip->XboxgipSysInitReady =
                WdfCollectionGetCount(xgip->XboxgipSysInitCollection) == XGIP_SYS_INIT_PACKETS;

            // If all packets are cached, start initialization timer
            if (xgip->XboxgipSysInitReady)
            {
                WdfTimerStart(xgip->XboxgipSysInitTimer, XGIP_SYS_INIT_PERIOD);
            }

            return status;
        }

        status = WdfIoQueueRetrieveNextRequest(XgipGetData(hChild)->PendingUsbInRequests, &usbRequest);

        break;
    default:

        return STATUS_NOT_SUPPORTED;
    }

    if (!NT_SUCCESS(status))
        return status;

    KdPrint((DRIVERNAME "Bus_SubmitReport: pending IRP found\n"));

    // Get pending IRP
    pendingIrp = WdfRequestWdmGetIrp(usbRequest);

    // Get USB request block
    PURB urb = (PURB)URB_FROM_IRP(pendingIrp);

    // Get transfer buffer
    PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;

    switch (pdoData->TargetType)
    {
    case Xbox360Wired:

        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = XUSB_REPORT_SIZE;

        /* Copy report to cache and transfer buffer
         * The first two bytes are always the same, so we skip them */
        RtlCopyBytes(XusbGetData(hChild)->Report + 2, &((PXUSB_SUBMIT_REPORT)Report)->Report, sizeof(XUSB_REPORT));
        RtlCopyBytes(Buffer, XusbGetData(hChild)->Report, XUSB_REPORT_SIZE);

        break;
    case DualShock4Wired:

        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = DS4_REPORT_SIZE;

        /* Copy report to cache and transfer buffer
         * Skip first byte as it contains the never changing report id */
        RtlCopyBytes(Ds4GetData(hChild)->Report + 1, &((PDS4_SUBMIT_REPORT)Report)->Report, sizeof(DS4_REPORT));
        RtlCopyBytes(Buffer, Ds4GetData(hChild)->Report, DS4_REPORT_SIZE);

        break;
    case XboxOneWired:

        // Request is input report
        if (((PXGIP_SUBMIT_REPORT)Report)->Size == sizeof(XGIP_SUBMIT_REPORT))
        {
            urb->UrbBulkOrInterruptTransfer.TransferBufferLength = XGIP_REPORT_SIZE;

            // Increase event counter on every call (can roll-over)
            XgipGetData(hChild)->Report[2]++;

            /* Copy report to cache and transfer buffer
             * Skip first four bytes as they are not part of the report */
            RtlCopyBytes(XgipGetData(hChild)->Report + 4, &((PXGIP_SUBMIT_REPORT)Report)->Report, sizeof(XGIP_REPORT));
            RtlCopyBytes(Buffer, XgipGetData(hChild)->Report, XGIP_REPORT_SIZE);

            break;
        }

        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    // Complete pending request
    WdfRequestComplete(usbRequest, status);

    return status;
}

