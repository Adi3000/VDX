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

NTSTATUS Xgip_PreparePdo(PWDFDEVICE_INIT DeviceInit, PUNICODE_STRING DeviceId, PUNICODE_STRING DeviceDescription)
{
    NTSTATUS status;
    UNICODE_STRING buffer;

    // prepare device description
    status = RtlUnicodeStringInit(DeviceDescription, L"Virtual Xbox One Controller");
    if (!NT_SUCCESS(status))
        return status;

    // Set hardware IDs
    RtlUnicodeStringInit(&buffer, L"USB\\VID_0E6F&PID_0139&REV_0650");

    status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringCopy(DeviceId, &buffer);

    RtlUnicodeStringInit(&buffer, L"USB\\VID_0E6F&PID_0139");

    status = WdfPdoInitAddHardwareID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    // Set compatible IDs
    RtlUnicodeStringInit(&buffer, L"USB\\MS_COMP_XGIP10");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringInit(&buffer, L"USB\\Class_FF&SubClass_47&Prot_D0");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringInit(&buffer, L"USB\\Class_FF&SubClass_47");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    RtlUnicodeStringInit(&buffer, L"USB\\Class_FF");

    status = WdfPdoInitAddCompatibleID(DeviceInit, &buffer);
    if (!NT_SUCCESS(status))
        return status;

    return STATUS_SUCCESS;
}

NTSTATUS Xgip_PrepareHardware(WDFDEVICE Device)
{
    NTSTATUS status;
    WDF_QUERY_INTERFACE_CONFIG ifaceCfg;

#ifdef NOPE

    INTERFACE dummyIface;

    dummyIface.Size = sizeof(INTERFACE);
    dummyIface.Version = 1;
    dummyIface.Context = (PVOID)Device;

    dummyIface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    dummyIface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    /* XGIP will query for the following three (unknown) interfaces
    * BUT WONT USE IT so we just expose them to satisfy initialization. */

    // Dummy 0

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XGIP_UNKNOWN_0, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data1,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data2,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data3,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[0],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[1],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[2],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[3],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[4],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[5],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[6],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_0.Data4[7],
            status));

        return status;
    }

    // Dummy 1

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XGIP_UNKNOWN_1, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data1,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data2,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data3,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[0],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[1],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[2],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[3],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[4],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[5],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[6],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_1.Data4[7],
            status));

        return status;
    }

    // Dummy 2

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XGIP_UNKNOWN_2, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data1,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data2,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data3,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[0],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[1],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[2],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[3],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[4],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[5],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[6],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_2.Data4[7],
            status));

        return status;
    }

    // Dummy 3

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XGIP_UNKNOWN_3, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data1,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data2,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data3,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[0],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[1],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[2],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[3],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[4],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[5],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[6],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_3.Data4[7],
            status));

        return status;
    }

    // Dummy 4

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&dummyIface, &GUID_DEVINTERFACE_XGIP_UNKNOWN_4, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Couldn't register unknown interface GUID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X (status 0x%x)\n",
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data1,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data2,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data3,
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[0],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[1],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[2],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[3],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[4],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[5],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[6],
            GUID_DEVINTERFACE_XGIP_UNKNOWN_4.Data4[7],
            status));

        return status;
    }

#endif

    // Expose USB_BUS_INTERFACE_USBDI_GUID

    // This interface actually IS used
    USB_BUS_INTERFACE_USBDI_V1 xgipInterface;

    xgipInterface.Size = sizeof(USB_BUS_INTERFACE_USBDI_V1);
    xgipInterface.Version = USB_BUSIF_USBDI_VERSION_1;
    xgipInterface.BusContext = (PVOID)Device;

    xgipInterface.InterfaceReference = WdfDeviceInterfaceReferenceNoOp;
    xgipInterface.InterfaceDereference = WdfDeviceInterfaceDereferenceNoOp;

    xgipInterface.SubmitIsoOutUrb = UsbPdo_SubmitIsoOutUrb;
    xgipInterface.GetUSBDIVersion = UsbPdo_GetUSBDIVersion;
    xgipInterface.QueryBusTime = UsbPdo_QueryBusTime;
    xgipInterface.QueryBusInformation = UsbPdo_QueryBusInformation;
    xgipInterface.IsDeviceHighSpeed = UsbPdo_IsDeviceHighSpeed;

    WDF_QUERY_INTERFACE_CONFIG_INIT(&ifaceCfg, (PINTERFACE)&xgipInterface, &USB_BUS_INTERFACE_USBDI_GUID, NULL);

    status = WdfDeviceAddQueryInterface(Device, &ifaceCfg);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfDeviceAddQueryInterface failed status 0x%x\n", status));
        return status;
    }

    // Start pending IRP queue flush timer
    //WdfTimerStart(XgipGetData(Device)->PendingUsbInRequestsTimer, XGIP_QUEUE_FLUSH_PERIOD);
    
    UCHAR DefaultReport[XGIP_REPORT_SIZE] =
    {
        0x20, 0x00, 0x10, 0x0e, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xa3, 0xfd, 0xed, 0x05, 0x5b, 0x03,
        0x6f, 0x02
    };

    RtlCopyBytes(XgipGetData(Device)->Report, DefaultReport, XGIP_REPORT_SIZE);

    return STATUS_SUCCESS;
}

NTSTATUS Xgip_AssignPdoContext(WDFDEVICE Device)
{
    NTSTATUS status;

    PXGIP_DEVICE_DATA xgip = XgipGetData(Device);

    KdPrint(("Initializing XGIP context...\n"));

    RtlZeroMemory(xgip, sizeof(XGIP_DEVICE_DATA));

    // Set fixed report id
    xgip->Report[0] = 0x20;
    xgip->Report[3] = 0x0E;

    // I/O Queue for pending IRPs
    WDF_IO_QUEUE_CONFIG pendingUsbQueueConfig, notificationsQueueConfig;

    // Create and assign queue for incoming interrupt transfer
    WDF_IO_QUEUE_CONFIG_INIT(&pendingUsbQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(Device, &pendingUsbQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &xgip->PendingUsbInRequests);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    // Initialize periodic timer
    WDF_TIMER_CONFIG timerConfig;
    WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, Xgip_PendingUsbRequestsTimerFunc, DS4_QUEUE_FLUSH_PERIOD);

    // Timer object attributes
    WDF_OBJECT_ATTRIBUTES timerAttribs;
    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttribs);

    // PDO is parent
    timerAttribs.ParentObject = Device;

    // Create timer
    status = WdfTimerCreate(&timerConfig, &timerAttribs, &xgip->PendingUsbInRequestsTimer);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfTimerCreate failed 0x%x\n", status));
        return status;
    }

    // Create and assign queue for user-land notification requests
    WDF_IO_QUEUE_CONFIG_INIT(&notificationsQueueConfig, WdfIoQueueDispatchManual);

    status = WdfIoQueueCreate(Device, &notificationsQueueConfig, WDF_NO_OBJECT_ATTRIBUTES, &xgip->PendingNotificationRequests);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
        return status;
    }

    return STATUS_SUCCESS;
}

VOID Xgip_GetConfigurationDescriptorType(PUCHAR Buffer, ULONG Length)
{
    UCHAR XgipDescriptorData[XGIP_DESCRIPTOR_SIZE] =
    {
        0x09,        //   bLength
        0x02,        //   bDescriptorType (Configuration)
        0x40, 0x00,  //   wTotalLength 64
        0x02,        //   bNumInterfaces 2
        0x01,        //   bConfigurationValue
        0x00,        //   iConfiguration (String Index)
        0xC0,        //   bmAttributes
        0xFA,        //   bMaxPower 500mA

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x00,        //   bInterfaceNumber 0
        0x00,        //   bAlternateSetting
        0x02,        //   bNumEndpoints 2
        0xFF,        //   bInterfaceClass
        0x47,        //   bInterfaceSubClass
        0xD0,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x81,        //   bEndpointAddress (IN/D2H)
        0x03,        //   bmAttributes (Interrupt)
        0x40, 0x00,  //   wMaxPacketSize 64
        0x04,        //   bInterval 4 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x01,        //   bEndpointAddress (OUT/H2D)
        0x03,        //   bmAttributes (Interrupt)
        0x40, 0x00,  //   wMaxPacketSize 64
        0x04,        //   bInterval 4 (unit depends on device speed)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x01,        //   bInterfaceNumber 1
        0x00,        //   bAlternateSetting
        0x00,        //   bNumEndpoints 0
        0xFF,        //   bInterfaceClass
        0x47,        //   bInterfaceSubClass
        0xD0,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x09,        //   bLength
        0x04,        //   bDescriptorType (Interface)
        0x01,        //   bInterfaceNumber 1
        0x01,        //   bAlternateSetting
        0x02,        //   bNumEndpoints 2
        0xFF,        //   bInterfaceClass
        0x47,        //   bInterfaceSubClass
        0xD0,        //   bInterfaceProtocol
        0x00,        //   iInterface (String Index)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x02,        //   bEndpointAddress (OUT/H2D)
        0x01,        //   bmAttributes (Isochronous, No Sync, Data EP)
        0xE0, 0x00,  //   wMaxPacketSize 224
        0x01,        //   bInterval 1 (unit depends on device speed)

        0x07,        //   bLength
        0x05,        //   bDescriptorType (Endpoint)
        0x83,        //   bEndpointAddress (IN/D2H)
        0x01,        //   bmAttributes (Isochronous, No Sync, Data EP)
        0x80, 0x00,  //   wMaxPacketSize 128
        0x01,        //   bInterval 1 (unit depends on device speed)

                     // 64 bytes

                     // best guess: USB Standard Descriptor
    };

    RtlCopyBytes(Buffer, XgipDescriptorData, Length);
}

VOID Xgip_PendingUsbRequestsTimerFunc(
    _In_ WDFTIMER Timer
)
{
    NTSTATUS status;
    WDFREQUEST usbRequest;
    WDFDEVICE hChild;
    PXGIP_DEVICE_DATA xgipData;
    PIRP pendingIrp;
    PIO_STACK_LOCATION irpStack;

    KdPrint(("Xgip_PendingUsbRequestsTimerFunc: Timer elapsed\n"));

    hChild = WdfTimerGetParentObject(Timer);
    xgipData = XgipGetData(hChild);

    WdfObjectAcquireLock(xgipData->PendingUsbInRequests);

    // Get pending USB request
    status = WdfIoQueueRetrieveNextRequest(xgipData->PendingUsbInRequests, &usbRequest);

    if (NT_SUCCESS(status))
    {
        // KdPrint(("Ds4_PendingUsbRequestsTimerFunc: pending IRP found\n"));

        // Get pending IRP
        pendingIrp = WdfRequestWdmGetIrp(usbRequest);
        irpStack = IoGetCurrentIrpStackLocation(pendingIrp);

        // Get USB request block
        PURB urb = (PURB)irpStack->Parameters.Others.Argument1;

        // Get transfer buffer
        PUCHAR Buffer = (PUCHAR)urb->UrbBulkOrInterruptTransfer.TransferBuffer;

        // Set buffer length to report size
        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = XGIP_REPORT_SIZE;

        // Increase event counter on every call (can roll-over)
        xgipData->Report[2]++;

        RtlCopyBytes(Buffer, xgipData->Report, XGIP_REPORT_SIZE);

        // Complete pending request
        WdfRequestComplete(usbRequest, status);
    }

    WdfObjectReleaseLock(xgipData->PendingUsbInRequests);
}

