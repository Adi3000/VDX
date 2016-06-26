#include "busenum.h"
#include "ds4defs.h"


//
// Dummy function to satisfy USB interface
// 
BOOLEAN USB_BUSIFFN UsbPdo_IsDeviceHighSpeed(IN PVOID BusContext)
{
    UNREFERENCED_PARAMETER(BusContext);

    KdPrint(("UsbPdo_IsDeviceHighSpeed: TRUE\n"));

    return TRUE;
}

//
// Dummy function to satisfy USB interface
// 
NTSTATUS USB_BUSIFFN UsbPdo_QueryBusInformation(
    IN PVOID BusContext,
    IN ULONG Level,
    IN OUT PVOID BusInformationBuffer,
    IN OUT PULONG BusInformationBufferLength,
    OUT PULONG BusInformationActualLength
)
{
    UNREFERENCED_PARAMETER(BusContext);
    UNREFERENCED_PARAMETER(Level);
    UNREFERENCED_PARAMETER(BusInformationBuffer);
    UNREFERENCED_PARAMETER(BusInformationBufferLength);
    UNREFERENCED_PARAMETER(BusInformationActualLength);

    KdPrint(("UsbPdo_QueryBusInformation: STATUS_UNSUCCESSFUL\n"));
    return STATUS_UNSUCCESSFUL;
}

//
// Dummy function to satisfy USB interface
// 
NTSTATUS USB_BUSIFFN UsbPdo_SubmitIsoOutUrb(IN PVOID BusContext, IN PURB Urb)
{
    UNREFERENCED_PARAMETER(BusContext);
    UNREFERENCED_PARAMETER(Urb);

    KdPrint(("UsbPdo_SubmitIsoOutUrb: STATUS_UNSUCCESSFUL\n"));
    return STATUS_UNSUCCESSFUL;
}

//
// Dummy function to satisfy USB interface
// 
NTSTATUS USB_BUSIFFN UsbPdo_QueryBusTime(IN PVOID BusContext, IN OUT PULONG CurrentUsbFrame)
{
    UNREFERENCED_PARAMETER(BusContext);
    UNREFERENCED_PARAMETER(CurrentUsbFrame);

    KdPrint(("UsbPdo_QueryBusTime: STATUS_UNSUCCESSFUL\n"));
    return STATUS_UNSUCCESSFUL;
}

//
// Dummy function to satisfy USB interface
// 
VOID USB_BUSIFFN UsbPdo_GetUSBDIVersion(
    IN PVOID BusContext,
    IN OUT PUSBD_VERSION_INFORMATION VersionInformation,
    IN OUT PULONG HcdCapabilities
)
{
    UNREFERENCED_PARAMETER(BusContext);

    KdPrint(("UsbPdo_GetUSBDIVersion: 0x500, 0x200\n"));

    if (VersionInformation != NULL)
    {
        VersionInformation->USBDI_Version = 0x500; /* Usbport */
        VersionInformation->Supported_USB_Version = 0x200; /* USB 2.0 */
    }

    if (HcdCapabilities != NULL)
    {
        *HcdCapabilities = 0;
    }
}

//
// Set device descriptor to identify as wired Microsoft Xbox 360 Controller.
// 
NTSTATUS UsbPdo_GetDeviceDescriptorType(PURB urb, PPDO_DEVICE_DATA pCommon)
{
    PUSB_DEVICE_DESCRIPTOR pDescriptor = (PUSB_DEVICE_DESCRIPTOR)urb->UrbControlDescriptorRequest.TransferBuffer;

    switch (pCommon->TargetType)
    {
    case Xbox360Wired:
    {
        pDescriptor->bLength = 0x12;
        pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
        pDescriptor->bcdUSB = 0x0200; // USB v2.0
        pDescriptor->bDeviceClass = 0xFF;
        pDescriptor->bDeviceSubClass = 0xFF;
        pDescriptor->bDeviceProtocol = 0xFF;
        pDescriptor->bMaxPacketSize0 = 0x08;
        pDescriptor->idVendor = 0x045E; // Microsoft Corp.
        pDescriptor->idProduct = 0x028E; // Xbox360 Controller
        pDescriptor->bcdDevice = 0x0114;
        pDescriptor->iManufacturer = 0x01;
        pDescriptor->iProduct = 0x02;
        pDescriptor->iSerialNumber = 0x03;
        pDescriptor->bNumConfigurations = 0x01;
    }
    case DualShock4Wired:
    {
        pDescriptor->bLength = 0x12;
        pDescriptor->bDescriptorType = USB_DEVICE_DESCRIPTOR_TYPE;
        pDescriptor->bcdUSB = 0x0200; // USB v2.0
        pDescriptor->bDeviceClass = 0x00; // per Interface
        pDescriptor->bDeviceSubClass = 0x00;
        pDescriptor->bDeviceProtocol = 0x00;
        pDescriptor->bMaxPacketSize0 = 0x40;
        pDescriptor->idVendor = 0x054C; // Sony Corp.
        pDescriptor->idProduct = 0x05C4; // Wireless Controller
        pDescriptor->bcdDevice = 0x0100;
        pDescriptor->iManufacturer = 0x01;
        pDescriptor->iProduct = 0x02;
        pDescriptor->iSerialNumber = 0x00;
        pDescriptor->bNumConfigurations = 0x01;
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Set configuration descriptor to identify as HID and exposed endpoints.
// 
NTSTATUS UsbPdo_GetConfigurationDescriptorType(PURB urb, PPDO_DEVICE_DATA pCommon)
{
    /*
    0x09,        //   bLength
    0x02,        //   bDescriptorType (Configuration)
    0x99, 0x00,  //   wTotalLength 153
    0x04,        //   bNumInterfaces 4
    0x01,        //   bConfigurationValue
    0x00,        //   iConfiguration (String Index)
    0xA0,        //   bmAttributes Remote Wakeup
    0xFA,        //   bMaxPower 500mA

    0x09,        //   bLength
    0x04,        //   bDescriptorType (Interface)
    0x00,        //   bInterfaceNumber 0
    0x00,        //   bAlternateSetting
    0x02,        //   bNumEndpoints 2
    0xFF,        //   bInterfaceClass
    0x5D,        //   bInterfaceSubClass
    0x01,        //   bInterfaceProtocol
    0x00,        //   iInterface (String Index)

    0x11,        //   bLength
    0x21,        //   bDescriptorType (HID)
    0x00, 0x01,  //   bcdHID 1.00
    0x01,        //   bCountryCode
    0x25,        //   bNumDescriptors
    0x81,        //   bDescriptorType[0] (Unknown 0x81)
    0x14, 0x00,  //   wDescriptorLength[0] 20
    0x00,        //   bDescriptorType[1] (Unknown 0x00)
    0x00, 0x00,  //   wDescriptorLength[1] 0
    0x13,        //   bDescriptorType[2] (Unknown 0x13)
    0x01, 0x08,  //   wDescriptorLength[2] 2049
    0x00,        //   bDescriptorType[3] (Unknown 0x00)
    0x00,
    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x81,        //   bEndpointAddress (IN/D2H)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x04,        //   bInterval 4 (unit depends on device speed)

    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x01,        //   bEndpointAddress (OUT/H2D)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x08,        //   bInterval 8 (unit depends on device speed)

    0x09,        //   bLength
    0x04,        //   bDescriptorType (Interface)
    0x01,        //   bInterfaceNumber 1
    0x00,        //   bAlternateSetting
    0x04,        //   bNumEndpoints 4
    0xFF,        //   bInterfaceClass
    0x5D,        //   bInterfaceSubClass
    0x03,        //   bInterfaceProtocol
    0x00,        //   iInterface (String Index)

    0x1B,        //   bLength
    0x21,        //   bDescriptorType (HID)
    0x00, 0x01,  //   bcdHID 1.00
    0x01,        //   bCountryCode
    0x01,        //   bNumDescriptors
    0x82,        //   bDescriptorType[0] (Unknown 0x82)
    0x40, 0x01,  //   wDescriptorLength[0] 320
    0x02, 0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x82,        //   bEndpointAddress (IN/D2H)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x02,        //   bInterval 2 (unit depends on device speed)

    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x02,        //   bEndpointAddress (OUT/H2D)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x04,        //   bInterval 4 (unit depends on device speed)

    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x83,        //   bEndpointAddress (IN/D2H)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x40,        //   bInterval 64 (unit depends on device speed)

    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x03,        //   bEndpointAddress (OUT/H2D)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x10,        //   bInterval 16 (unit depends on device speed)

    0x09,        //   bLength
    0x04,        //   bDescriptorType (Interface)
    0x02,        //   bInterfaceNumber 2
    0x00,        //   bAlternateSetting
    0x01,        //   bNumEndpoints 1
    0xFF,        //   bInterfaceClass
    0x5D,        //   bInterfaceSubClass
    0x02,        //   bInterfaceProtocol
    0x00,        //   iInterface (String Index)

    0x09,        //   bLength
    0x21,        //   bDescriptorType (HID)
    0x00, 0x01,  //   bcdHID 1.00
    0x01,        //   bCountryCode
    0x22,        //   bNumDescriptors
    0x84,        //   bDescriptorType[0] (Unknown 0x84)
    0x07, 0x00,  //   wDescriptorLength[0] 7

    0x07,        //   bLength
    0x05,        //   bDescriptorType (Endpoint)
    0x84,        //   bEndpointAddress (IN/D2H)
    0x03,        //   bmAttributes (Interrupt)
    0x20, 0x00,  //   wMaxPacketSize 32
    0x10,        //   bInterval 16 (unit depends on device speed)

    0x09,        //   bLength
    0x04,        //   bDescriptorType (Interface)
    0x03,        //   bInterfaceNumber 3
    0x00,        //   bAlternateSetting
    0x00,        //   bNumEndpoints 0
    0xFF,        //   bInterfaceClass
    0xFD,        //   bInterfaceSubClass
    0x13,        //   bInterfaceProtocol
    0x04,        //   iInterface (String Index)

    0x06,        //   bLength
    0x41,        //   bDescriptorType (Unknown)
    0x00, 0x01, 0x01, 0x03,
    // 153 bytes

    // best guess: USB Standard Descriptor
    */
    UCHAR XusbDescriptorData[XUSB_DESCRIPTOR_SIZE] =
    {
        0x09, 0x02, 0x99, 0x00, 0x04, 0x01, 0x00, 0xA0, 0xFA, 0x09,
        0x04, 0x00, 0x00, 0x02, 0xFF, 0x5D, 0x01, 0x00, 0x11, 0x21,
        0x00, 0x01, 0x01, 0x25, 0x81, 0x14, 0x00, 0x00, 0x00, 0x00,
        0x13, 0x01, 0x08, 0x00, 0x00, 0x07, 0x05, 0x81, 0x03, 0x20,
        0x00, 0x04, 0x07, 0x05, 0x01, 0x03, 0x20, 0x00, 0x08, 0x09,
        0x04, 0x01, 0x00, 0x04, 0xFF, 0x5D, 0x03, 0x00, 0x1B, 0x21,
        0x00, 0x01, 0x01, 0x01, 0x82, 0x40, 0x01, 0x02, 0x20, 0x16,
        0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x05, 0x82, 0x03, 0x20,
        0x00, 0x02, 0x07, 0x05, 0x02, 0x03, 0x20, 0x00, 0x04, 0x07,
        0x05, 0x83, 0x03, 0x20, 0x00, 0x40, 0x07, 0x05, 0x03, 0x03,
        0x20, 0x00, 0x10, 0x09, 0x04, 0x02, 0x00, 0x01, 0xFF, 0x5D,
        0x02, 0x00, 0x09, 0x21, 0x00, 0x01, 0x01, 0x22, 0x84, 0x07,
        0x00, 0x07, 0x05, 0x84, 0x03, 0x20, 0x00, 0x10, 0x09, 0x04,
        0x03, 0x00, 0x00, 0xFF, 0xFD, 0x13, 0x04, 0x06, 0x41, 0x00,
        0x01, 0x01, 0x03
    };

    UCHAR Ds4DescriptorData[DS4_DESCRIPTOR_SIZE] =
    {
        0x09,        // bLength
        0x02,        // bDescriptorType (Configuration)
        0x29, 0x00,  // wTotalLength 41
        0x01,        // bNumInterfaces 1
        0x01,        // bConfigurationValue
        0x00,        // iConfiguration (String Index)
        0xC0,        // bmAttributes Self Powered
        0xFA,        // bMaxPower 500mA

        0x09,        // bLength
        0x04,        // bDescriptorType (Interface)
        0x00,        // bInterfaceNumber 0
        0x00,        // bAlternateSetting
        0x02,        // bNumEndpoints 2
        0x03,        // bInterfaceClass
        0x00,        // bInterfaceSubClass
        0x00,        // bInterfaceProtocol
        0x00,        // iInterface (String Index)

        0x09,        // bLength
        0x21,        // bDescriptorType (HID)
        0x11, 0x01,  // bcdHID 1.17
        0x00,        // bCountryCode
        0x01,        // bNumDescriptors
        0x22,        // bDescriptorType[0] (HID)
        0xD3, 0x01,  // wDescriptorLength[0] 467

        0x07,        // bLength
        0x05,        // bDescriptorType (Endpoint)
        0x84,        // bEndpointAddress (IN/D2H)
        0x03,        // bmAttributes (Interrupt)
        0x40, 0x00,  // wMaxPacketSize 64
        0x05,        // bInterval 5 (unit depends on device speed)

        0x07,        // bLength
        0x05,        // bDescriptorType (Endpoint)
        0x03,        // bEndpointAddress (OUT/H2D)
        0x03,        // bmAttributes (Interrupt)
        0x40, 0x00,  // wMaxPacketSize 64
        0x05,        // bInterval 5 (unit depends on device speed)
    };

    // First request just gets required buffer size back
    if (urb->UrbControlDescriptorRequest.TransferBufferLength == sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        PUSB_CONFIGURATION_DESCRIPTOR pDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)urb->UrbControlDescriptorRequest.TransferBuffer;

        switch (pCommon->TargetType)
        {
        case Xbox360Wired:
        {
            pDescriptor->bLength = 0x09;
            pDescriptor->bDescriptorType = USB_CONFIGURATION_DESCRIPTOR_TYPE;
            pDescriptor->wTotalLength = XUSB_DESCRIPTOR_SIZE;
            pDescriptor->bNumInterfaces = 0x04;
            pDescriptor->bConfigurationValue = 0x01;
            pDescriptor->iConfiguration = 0x00;
            pDescriptor->bmAttributes = 0xA0;
            pDescriptor->MaxPower = 0xFA;
        }
        case DualShock4Wired:
        {
            pDescriptor->bLength = 0x09;
            pDescriptor->bDescriptorType = USB_CONFIGURATION_DESCRIPTOR_TYPE;
            pDescriptor->wTotalLength = DS4_DESCRIPTOR_SIZE;
            pDescriptor->bNumInterfaces = 0x01;
            pDescriptor->bConfigurationValue = 0x01;
            pDescriptor->iConfiguration = 0x00;
            pDescriptor->bmAttributes = 0xC0;
            pDescriptor->MaxPower = 0xFA;
        }
        default:
            break;
        }
    }

    // Second request can store the whole descriptor
    switch (pCommon->TargetType)
    {
    case Xbox360Wired:
    {
        if (urb->UrbControlDescriptorRequest.TransferBufferLength >= XUSB_DESCRIPTOR_SIZE)
        {
            RtlCopyMemory(urb->UrbControlDescriptorRequest.TransferBuffer, XusbDescriptorData, XUSB_DESCRIPTOR_SIZE);
        }
    }
    case DualShock4Wired:
    {
        if (urb->UrbControlDescriptorRequest.TransferBufferLength >= DS4_DESCRIPTOR_SIZE)
        {
            RtlCopyMemory(urb->UrbControlDescriptorRequest.TransferBuffer, Ds4DescriptorData, DS4_DESCRIPTOR_SIZE);
        }
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Set device string descriptors (currently only used in DS4 emulation).
// 
NTSTATUS UsbPdo_GetStringDescriptorType(PURB urb, PPDO_DEVICE_DATA pCommon)
{
    KdPrint(("Index = %d\n", urb->UrbControlDescriptorRequest.Index));

    switch (pCommon->TargetType)
    {
    case DualShock4Wired:
    {
        switch (urb->UrbControlDescriptorRequest.Index)
        {
        case 0:
        {
            // "American English"
            UCHAR LangId[HID_LANGUAGE_ID_LENGTH] =
            {
                0x04, 0x03, 0x09, 0x04
            };

            urb->UrbControlDescriptorRequest.TransferBufferLength = HID_LANGUAGE_ID_LENGTH;
            RtlCopyBytes(urb->UrbControlDescriptorRequest.TransferBuffer, LangId, HID_LANGUAGE_ID_LENGTH);

            break;
        }
        case 1:
        {
            KdPrint(("LanguageId = 0x%X\n", urb->UrbControlDescriptorRequest.LanguageId));

            if (urb->UrbControlDescriptorRequest.TransferBufferLength < DS4_MANUFACTURER_NAME_LENGTH)
            {
                PUSB_STRING_DESCRIPTOR pDesc = (PUSB_STRING_DESCRIPTOR)urb->UrbControlDescriptorRequest.TransferBuffer;
                pDesc->bLength = DS4_MANUFACTURER_NAME_LENGTH;
                break;
            }

            // "Sony Computer Entertainment"
            UCHAR ManufacturerString[DS4_MANUFACTURER_NAME_LENGTH] =
            {
                0x38, 0x03, 0x53, 0x00, 0x6F, 0x00, 0x6E, 0x00,
                0x79, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6F, 0x00,
                0x6D, 0x00, 0x70, 0x00, 0x75, 0x00, 0x74, 0x00,
                0x65, 0x00, 0x72, 0x00, 0x20, 0x00, 0x45, 0x00,
                0x6E, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00,
                0x74, 0x00, 0x61, 0x00, 0x69, 0x00, 0x6E, 0x00,
                0x6D, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00
            };

            urb->UrbControlDescriptorRequest.TransferBufferLength = DS4_MANUFACTURER_NAME_LENGTH;
            RtlCopyBytes(urb->UrbControlDescriptorRequest.TransferBuffer, ManufacturerString, DS4_MANUFACTURER_NAME_LENGTH);

            break;
        }
        case 2:
        {
            KdPrint(("LanguageId = 0x%X\n", urb->UrbControlDescriptorRequest.LanguageId));

            if (urb->UrbControlDescriptorRequest.TransferBufferLength < DS4_PRODUCT_NAME_LENGTH)
            {
                PUSB_STRING_DESCRIPTOR pDesc = (PUSB_STRING_DESCRIPTOR)urb->UrbControlDescriptorRequest.TransferBuffer;
                pDesc->bLength = DS4_PRODUCT_NAME_LENGTH;
                break;
            }

            // "Wireless Controller"
            UCHAR ProductString[DS4_PRODUCT_NAME_LENGTH] =
            {
                0x28, 0x03, 0x57, 0x00, 0x69, 0x00, 0x72, 0x00,
                0x65, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x73, 0x00,
                0x73, 0x00, 0x20, 0x00, 0x43, 0x00, 0x6F, 0x00,
                0x6E, 0x00, 0x74, 0x00, 0x72, 0x00, 0x6F, 0x00,
                0x6C, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x72, 0x00
            };

            urb->UrbControlDescriptorRequest.TransferBufferLength = DS4_PRODUCT_NAME_LENGTH;
            RtlCopyBytes(urb->UrbControlDescriptorRequest.TransferBuffer, ProductString, DS4_PRODUCT_NAME_LENGTH);
        }
        default:
            break;
        }
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Fakes a successfully selected configuration.
// 
NTSTATUS UsbPdo_SelectConfiguration(PURB urb, PPDO_DEVICE_DATA pCommon)
{
    PUSBD_INTERFACE_INFORMATION pInfo;

    pInfo = &urb->UrbSelectConfiguration.Interface;

    KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: TotalLength %d, Size %d\n", urb->UrbHeader.Length, XUSB_CONFIGURATION_SIZE));

    if (urb->UrbHeader.Length == sizeof(struct _URB_SELECT_CONFIGURATION))
    {
        KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: NULL ConfigurationDescriptor\n"));
        return STATUS_SUCCESS;
    }

    switch (pCommon->TargetType)
    {
    case Xbox360Wired:
    {
        if (urb->UrbHeader.Length < XUSB_CONFIGURATION_SIZE)
        {
            KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Invalid ConfigurationDescriptor\n"));
            return STATUS_INVALID_PARAMETER;
        }

        KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
            (int)pInfo->Length,
            (int)pInfo->InterfaceNumber,
            (int)pInfo->AlternateSetting,
            pInfo->NumberOfPipes));

        pInfo->Class = 0xFF;
        pInfo->SubClass = 0x5D;
        pInfo->Protocol = 0x01;

        pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

        pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[0].MaximumPacketSize = 0x20;
        pInfo->Pipes[0].EndpointAddress = 0x81;
        pInfo->Pipes[0].Interval = 0x04;
        pInfo->Pipes[0].PipeType = 0x03;
        pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0081;
        pInfo->Pipes[0].PipeFlags = 0x00;

        pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[1].MaximumPacketSize = 0x20;
        pInfo->Pipes[1].EndpointAddress = 0x01;
        pInfo->Pipes[1].Interval = 0x08;
        pInfo->Pipes[1].PipeType = 0x03;
        pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0001;
        pInfo->Pipes[1].PipeFlags = 0x00;

        pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

        KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
            (int)pInfo->Length,
            (int)pInfo->InterfaceNumber,
            (int)pInfo->AlternateSetting,
            pInfo->NumberOfPipes));

        pInfo->Class = 0xFF;
        pInfo->SubClass = 0x5D;
        pInfo->Protocol = 0x03;

        pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

        pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[0].MaximumPacketSize = 0x20;
        pInfo->Pipes[0].EndpointAddress = 0x82;
        pInfo->Pipes[0].Interval = 0x04;
        pInfo->Pipes[0].PipeType = 0x03;
        pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0082;
        pInfo->Pipes[0].PipeFlags = 0x00;

        pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[1].MaximumPacketSize = 0x20;
        pInfo->Pipes[1].EndpointAddress = 0x02;
        pInfo->Pipes[1].Interval = 0x08;
        pInfo->Pipes[1].PipeType = 0x03;
        pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0002;
        pInfo->Pipes[1].PipeFlags = 0x00;

        pInfo->Pipes[2].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[2].MaximumPacketSize = 0x20;
        pInfo->Pipes[2].EndpointAddress = 0x83;
        pInfo->Pipes[2].Interval = 0x08;
        pInfo->Pipes[2].PipeType = 0x03;
        pInfo->Pipes[2].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0083;
        pInfo->Pipes[2].PipeFlags = 0x00;

        pInfo->Pipes[3].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[3].MaximumPacketSize = 0x20;
        pInfo->Pipes[3].EndpointAddress = 0x03;
        pInfo->Pipes[3].Interval = 0x08;
        pInfo->Pipes[3].PipeType = 0x03;
        pInfo->Pipes[3].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
        pInfo->Pipes[3].PipeFlags = 0x00;

        pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

        KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
            (int)pInfo->Length,
            (int)pInfo->InterfaceNumber,
            (int)pInfo->AlternateSetting,
            pInfo->NumberOfPipes));

        pInfo->Class = 0xFF;
        pInfo->SubClass = 0x5D;
        pInfo->Protocol = 0x02;

        pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

        pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[0].MaximumPacketSize = 0x20;
        pInfo->Pipes[0].EndpointAddress = 0x84;
        pInfo->Pipes[0].Interval = 0x04;
        pInfo->Pipes[0].PipeType = 0x03;
        pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
        pInfo->Pipes[0].PipeFlags = 0x00;

        pInfo = (PUSBD_INTERFACE_INFORMATION)((PCHAR)pInfo + pInfo->Length);

        KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
            (int)pInfo->Length,
            (int)pInfo->InterfaceNumber,
            (int)pInfo->AlternateSetting,
            pInfo->NumberOfPipes));

        pInfo->Class = 0xFF;
        pInfo->SubClass = 0xFD;
        pInfo->Protocol = 0x13;

        pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;
    }
    case DualShock4Wired:
    {
        if (urb->UrbHeader.Length < DS4_CONFIGURATION_SIZE)
        {
            KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Invalid ConfigurationDescriptor\n"));
            return STATUS_INVALID_PARAMETER;
        }

        KdPrint((">> >> >> URB_FUNCTION_SELECT_CONFIGURATION: Length %d, Interface %d, Alternate %d, Pipes %d\n",
            (int)pInfo->Length,
            (int)pInfo->InterfaceNumber,
            (int)pInfo->AlternateSetting,
            pInfo->NumberOfPipes));

        pInfo->Class = 0x03; // HID
        pInfo->SubClass = 0x00;
        pInfo->Protocol = 0x00;

        pInfo->InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

        pInfo->Pipes[0].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[0].MaximumPacketSize = 0x40;
        pInfo->Pipes[0].EndpointAddress = 0x84;
        pInfo->Pipes[0].Interval = 0x05;
        pInfo->Pipes[0].PipeType = 0x03;
        pInfo->Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
        pInfo->Pipes[0].PipeFlags = 0x00;

        pInfo->Pipes[1].MaximumTransferSize = 0x00400000;
        pInfo->Pipes[1].MaximumPacketSize = 0x40;
        pInfo->Pipes[1].EndpointAddress = 0x03;
        pInfo->Pipes[1].Interval = 0x05;
        pInfo->Pipes[1].PipeType = 0x03;
        pInfo->Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
        pInfo->Pipes[1].PipeFlags = 0x00;
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Fakes a successfully selected interface.
// 
NTSTATUS UsbPdo_SelectInterface(PURB urb, PPDO_DEVICE_DATA pCommon)
{
    PUSBD_INTERFACE_INFORMATION pInfo = &urb->UrbSelectInterface.Interface;

    KdPrint((">> >> >> URB_FUNCTION_SELECT_INTERFACE: Length %d, Interface %d, Alternate %d, Pipes %d\n",
        (int)pInfo->Length,
        (int)pInfo->InterfaceNumber,
        (int)pInfo->AlternateSetting,
        pInfo->NumberOfPipes));

    KdPrint((">> >> >> URB_FUNCTION_SELECT_INTERFACE: Class %d, SubClass %d, Protocol %d\n",
        (int)pInfo->Class,
        (int)pInfo->SubClass,
        (int)pInfo->Protocol));

    switch (pCommon->TargetType)
    {
    case Xbox360Wired:
    {
        if (pInfo->InterfaceNumber == 1)
        {
            pInfo[0].Class = 0xFF;
            pInfo[0].SubClass = 0x5D;
            pInfo[0].Protocol = 0x03;
            pInfo[0].NumberOfPipes = 0x04;

            pInfo[0].InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

            pInfo[0].Pipes[0].MaximumTransferSize = 0x00400000;
            pInfo[0].Pipes[0].MaximumPacketSize = 0x20;
            pInfo[0].Pipes[0].EndpointAddress = 0x82;
            pInfo[0].Pipes[0].Interval = 0x04;
            pInfo[0].Pipes[0].PipeType = 0x03;
            pInfo[0].Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0082;
            pInfo[0].Pipes[0].PipeFlags = 0x00;

            pInfo[0].Pipes[1].MaximumTransferSize = 0x00400000;
            pInfo[0].Pipes[1].MaximumPacketSize = 0x20;
            pInfo[0].Pipes[1].EndpointAddress = 0x02;
            pInfo[0].Pipes[1].Interval = 0x08;
            pInfo[0].Pipes[1].PipeType = 0x03;
            pInfo[0].Pipes[1].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0002;
            pInfo[0].Pipes[1].PipeFlags = 0x00;

            pInfo[0].Pipes[2].MaximumTransferSize = 0x00400000;
            pInfo[0].Pipes[2].MaximumPacketSize = 0x20;
            pInfo[0].Pipes[2].EndpointAddress = 0x83;
            pInfo[0].Pipes[2].Interval = 0x08;
            pInfo[0].Pipes[2].PipeType = 0x03;
            pInfo[0].Pipes[2].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0083;
            pInfo[0].Pipes[2].PipeFlags = 0x00;

            pInfo[0].Pipes[3].MaximumTransferSize = 0x00400000;
            pInfo[0].Pipes[3].MaximumPacketSize = 0x20;
            pInfo[0].Pipes[3].EndpointAddress = 0x03;
            pInfo[0].Pipes[3].Interval = 0x08;
            pInfo[0].Pipes[3].PipeType = 0x03;
            pInfo[0].Pipes[3].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0003;
            pInfo[0].Pipes[3].PipeFlags = 0x00;

            return STATUS_SUCCESS;
        }

        if (pInfo->InterfaceNumber == 2)
        {
            pInfo[0].Class = 0xFF;
            pInfo[0].SubClass = 0x5D;
            pInfo[0].Protocol = 0x02;
            pInfo[0].NumberOfPipes = 0x01;

            pInfo[0].InterfaceHandle = (USBD_INTERFACE_HANDLE)0xFFFF0000;

            pInfo[0].Pipes[0].MaximumTransferSize = 0x00400000;
            pInfo[0].Pipes[0].MaximumPacketSize = 0x20;
            pInfo[0].Pipes[0].EndpointAddress = 0x84;
            pInfo[0].Pipes[0].Interval = 0x04;
            pInfo[0].Pipes[0].PipeType = 0x03;
            pInfo[0].Pipes[0].PipeHandle = (USBD_PIPE_HANDLE)0xFFFF0084;
            pInfo[0].Pipes[0].PipeFlags = 0x00;

            return STATUS_SUCCESS;
        }
    }
    case DualShock4Wired:
    {
        KdPrint(("Warning: not implemented\n"));
        break;
    }
    default:
        break;
    }

    return STATUS_INVALID_PARAMETER;
}

//
// Dispatch bulk (or interrupt, if any) transfers.
// 
NTSTATUS UsbPdo_BulkOrInterruptTransfer(PURB urb, WDFDEVICE Device, WDFREQUEST Request)
{
    struct _URB_BULK_OR_INTERRUPT_TRANSFER* pTransfer = &urb->UrbBulkOrInterruptTransfer;

    NTSTATUS status;
    PPDO_DEVICE_DATA pdoData = PdoGetData(Device);
    PXUSB_DEVICE_DATA xusb = XusbGetData(Device);
    WDFREQUEST notifyRequest;

    if (pdoData == NULL)
    {
        KdPrint((">> >> >> Invalid common context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    switch (pdoData->TargetType)
    {
    case Xbox360Wired:
    {
        // Check context
        if (xusb == NULL)
        {
            KdPrint(("No XUSB context found on device %p\n", Device));

            return STATUS_UNSUCCESSFUL;
        }

        // Data coming FROM us TO higher driver
        if (pTransfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
        {
            KdPrint((">> >> >> Incoming request, queuing...\n"));

            /* This request is sent periodically and relies on data the "feeder"
            * has to supply, so we queue this request and return with STATUS_PENDING.
            * The request gets completed as soon as the "feeder" sent an update. */
            status = WdfRequestForwardToIoQueue(Request, xusb->PendingUsbRequests);

            return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
        }

        // Data coming FROM the higher driver TO us
        KdPrint((">> >> >> URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER: Handle %p, Flags %X, Length %d\n",
            pTransfer->PipeHandle,
            pTransfer->TransferFlags,
            pTransfer->TransferBufferLength));

        if (pTransfer->TransferBufferLength == XUSB_LEDSET_SIZE) // Led
        {
            PUCHAR Buffer = pTransfer->TransferBuffer;

            KdPrint(("-- LED Buffer: %02X %02X %02X", Buffer[0], Buffer[1], Buffer[2]));

            // extract LED byte to get controller slot
            if (Buffer[0] == 0x01 && Buffer[1] == 0x03 && Buffer[2] >= 0x02)
            {
                if (Buffer[2] == 0x02) xusb->LedNumber = 0;
                if (Buffer[2] == 0x03) xusb->LedNumber = 1;
                if (Buffer[2] == 0x04) xusb->LedNumber = 2;
                if (Buffer[2] == 0x05) xusb->LedNumber = 3;

                KdPrint(("-- LED Number: %d", xusb->LedNumber));
            }
        }

        // Extract rumble (vibration) information
        if (pTransfer->TransferBufferLength == XUSB_RUMBLE_SIZE)
        {
            PUCHAR Buffer = pTransfer->TransferBuffer;

            KdPrint(("-- Rumble Buffer: %02X %02X %02X %02X %02X %02X %02X %02X",
                Buffer[0],
                Buffer[1],
                Buffer[2],
                Buffer[3],
                Buffer[4],
                Buffer[5],
                Buffer[6],
                Buffer[7]));

            RtlCopyBytes(xusb->Rumble, Buffer, pTransfer->TransferBufferLength);
        }

        // Notify user-mode process that new data is available
        status = WdfIoQueueRetrieveNextRequest(xusb->PendingNotificationRequests, &notifyRequest);

        if (NT_SUCCESS(status))
        {
            PXUSB_REQUEST_NOTIFICATION notify = NULL;

            status = WdfRequestRetrieveOutputBuffer(notifyRequest, sizeof(XUSB_REQUEST_NOTIFICATION), (PVOID)&notify, NULL);

            if (NT_SUCCESS(status))
            {
                // Assign values to output buffer
                notify->Size = sizeof(XUSB_REQUEST_NOTIFICATION);
                notify->SerialNo = pdoData->SerialNo;
                notify->LedNumber = xusb->LedNumber;
                notify->LargeMotor = xusb->Rumble[3];
                notify->SmallMotor = xusb->Rumble[4];

                WdfRequestCompleteWithInformation(notifyRequest, status, notify->Size);
            }
            else
            {
                KdPrint(("WdfRequestRetrieveOutputBuffer failed with status 0x%X\n", status));
            }
        }
    }
    case DualShock4Wired:
    {
        PDS4_DEVICE_DATA ds4Data = Ds4GetData(Device);

        // Data coming FROM us TO higher driver
        if (pTransfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN
            && pTransfer->PipeHandle == (USBD_PIPE_HANDLE)0xFFFF0084)
        {
            KdPrint((">> >> >> Incoming request, queuing...\n"));

            /* This request is sent periodically and relies on data the "feeder"
            * has to supply, so we queue this request and return with STATUS_PENDING.
            * The request gets completed as soon as the "feeder" sent an update. */
            status = WdfRequestForwardToIoQueue(Request, ds4Data->PendingUsbRequests);

            return (NT_SUCCESS(status)) ? STATUS_PENDING : status;
        }
        else
        {
            KdPrint(("Warning: unimplemented pipe or transfer direction\n"));
        }

        // TODO: implement force-feedback requests
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Clean-up actions on shutdown.
// 
NTSTATUS UsbPdo_AbortPipe(WDFDEVICE Device)
{
    PPDO_DEVICE_DATA pdoData = PdoGetData(Device);

    switch (pdoData->TargetType)
    {
    case Xbox360Wired:
    {
        PXUSB_DEVICE_DATA xusb = XusbGetData(Device);

        // Check context
        if (xusb == NULL)
        {
            KdPrint(("No XUSB context found on device %p\n", Device));

            return STATUS_UNSUCCESSFUL;
        }

        // Higher driver shutting down, emptying PDOs queues
        WdfIoQueuePurge(xusb->PendingUsbRequests, NULL, NULL);
        WdfIoQueuePurge(xusb->PendingNotificationRequests, NULL, NULL);
    }
    case DualShock4Wired:
    {
        PDS4_DEVICE_DATA ds4 = Ds4GetData(Device);

        // Check context
        if (ds4 == NULL)
        {
            KdPrint(("No DS4 context found on device %p\n", Device));

            return STATUS_UNSUCCESSFUL;
        }

        // Higher driver shutting down, emptying PDOs queues
        WdfTimerStop(ds4->PendingUsbRequestsTimer, TRUE);
        WdfIoQueuePurge(ds4->PendingUsbRequests, NULL, NULL);
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Processes URBs containing HID-related requests.
// 
NTSTATUS UsbPdo_ClassInterface(PURB urb)
{
    UNREFERENCED_PARAMETER(urb);

    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST* pRequest = &urb->UrbControlVendorClassRequest;

    KdPrint((">> >> >> URB_FUNCTION_CLASS_INTERFACE\n"));
    KdPrint((">> >> >> TransferFlags = 0x%X, Request = 0x%X, Value = 0x%X, Index = 0x%X, BufLen = %d\n",
        pRequest->TransferFlags,
        pRequest->Request,
        pRequest->Value,
        pRequest->Index,
        pRequest->TransferBufferLength));

    switch (pRequest->Request)
    {
    case HID_REQUEST_GET_REPORT:
    {
        UCHAR reportId = (pRequest->Value) & 0xFF;
        UCHAR reportType = (pRequest->Value >> 8) & 0xFF;

        KdPrint((">> >> >> >> GET_REPORT(%d): %d\n", reportType, reportId));

        switch (reportType)
        {
        case HID_REPORT_TYPE_FEATURE:
        {
            switch (reportId)
            {
            case HID_REPORT_ID_0:
            {
                // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
                UCHAR Response[HID_GET_FEATURE_REPORT_SIZE_0] =
                {
                    0xA3, 0x41, 0x75, 0x67, 0x20, 0x20, 0x33, 0x20,
                    0x32, 0x30, 0x31, 0x33, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x30, 0x37, 0x3A, 0x30, 0x31, 0x3A, 0x31,
                    0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x01, 0x00, 0x31, 0x03, 0x00, 0x00,
                    0x00, 0x49, 0x00, 0x05, 0x00, 0x00, 0x80, 0x03,
                    0x00
                };

                pRequest->TransferBufferLength = HID_GET_FEATURE_REPORT_SIZE_0;
                RtlCopyBytes(pRequest->TransferBuffer, Response, HID_GET_FEATURE_REPORT_SIZE_0);

                break;
            }
            case HID_REPORT_ID_1:
            {
                // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
                UCHAR Response[HID_GET_FEATURE_REPORT_SIZE_1] =
                {
                    0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87,
                    0x22, 0x7B, 0xDD, 0xB2, 0x22, 0x47, 0xDD, 0xBD,
                    0x22, 0x43, 0xDD, 0x1C, 0x02, 0x1C, 0x02, 0x7F,
                    0x1E, 0x2E, 0xDF, 0x60, 0x1F, 0x4C, 0xE0, 0x3A,
                    0x1D, 0xC6, 0xDE, 0x08, 0x00
                };

                pRequest->TransferBufferLength = HID_GET_FEATURE_REPORT_SIZE_1;
                RtlCopyBytes(pRequest->TransferBuffer, Response, HID_GET_FEATURE_REPORT_SIZE_1);

                break;
            }
            case HID_REPORT_ID_2:
            {
                // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
                UCHAR Response[HID_GET_FEATURE_REPORT_SIZE_2] =
                {
                    0x12, 0x8B, 0x09, 0x07, 0x6D, 0x66, 0x1C, 0x08,
                    0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                };

                pRequest->TransferBufferLength = HID_GET_FEATURE_REPORT_SIZE_2;
                RtlCopyBytes(pRequest->TransferBuffer, Response, HID_GET_FEATURE_REPORT_SIZE_2);

                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        break;
    }
    case HID_REQUEST_SET_REPORT:
    {
        UCHAR reportId = (pRequest->Value) & 0xFF;
        UCHAR reportType = (pRequest->Value >> 8) & 0xFF;

        KdPrint((">> >> >> >> SET_REPORT(%d): %d\n", reportType, reportId));

        switch (reportType)
        {
        case HID_REPORT_TYPE_FEATURE:
        {
            switch (reportId)
            {
            case HID_REPORT_ID_3:
            {
                // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
                UCHAR Response[HID_SET_FEATURE_REPORT_SIZE_0] =
                {
                    0x13, 0xAC, 0x9E, 0x17, 0x94, 0x05, 0xB0, 0x56,
                    0xE8, 0x81, 0x38, 0x08, 0x06, 0x51, 0x41, 0xC0,
                    0x7F, 0x12, 0xAA, 0xD9, 0x66, 0x3C, 0xCE
                };

                pRequest->TransferBufferLength = HID_SET_FEATURE_REPORT_SIZE_0;
                RtlCopyBytes(pRequest->TransferBuffer, Response, HID_SET_FEATURE_REPORT_SIZE_0);

                break;
            }
            case HID_REPORT_ID_4:
            {
                // Source: http://eleccelerator.com/wiki/index.php?title=DualShock_4#Class_Requests
                UCHAR Response[HID_SET_FEATURE_REPORT_SIZE_1] =
                {
                    0x14, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00
                };

                pRequest->TransferBufferLength = HID_SET_FEATURE_REPORT_SIZE_1;
                RtlCopyBytes(pRequest->TransferBuffer, Response, HID_SET_FEATURE_REPORT_SIZE_1);

                break;
            }
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        break;
    }
    default:
        break;
    }

    return STATUS_SUCCESS;
}

//
// Returns interface HID report descriptor.
// 
NTSTATUS UsbPdo_GetDescriptorFromInterface(PURB urb, PPDO_DEVICE_DATA pCommon)
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    UCHAR Ds4HidReportDescriptor[DS4_HID_REPORT_DESCRIPTOR_SIZE] =
    {
        0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
        0x09, 0x05,        // Usage (Game Pad)
        0xA1, 0x01,        // Collection (Application)
        0x85, 0x01,        //   Report ID (1)
        0x09, 0x30,        //   Usage (X)
        0x09, 0x31,        //   Usage (Y)
        0x09, 0x32,        //   Usage (Z)
        0x09, 0x35,        //   Usage (Rz)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x04,        //   Report Count (4)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x09, 0x39,        //   Usage (Hat switch)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x07,        //   Logical Maximum (7)
        0x35, 0x00,        //   Physical Minimum (0)
        0x46, 0x3B, 0x01,  //   Physical Maximum (315)
        0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
        0x75, 0x04,        //   Report Size (4)
        0x95, 0x01,        //   Report Count (1)
        0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
        0x65, 0x00,        //   Unit (None)
        0x05, 0x09,        //   Usage Page (Button)
        0x19, 0x01,        //   Usage Minimum (0x01)
        0x29, 0x0E,        //   Usage Maximum (0x0E)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)
        0x75, 0x01,        //   Report Size (1)
        0x95, 0x0E,        //   Report Count (14)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
        0x09, 0x20,        //   Usage (0x20)
        0x75, 0x06,        //   Report Size (6)
        0x95, 0x01,        //   Report Count (1)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x7F,        //   Logical Maximum (127)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
        0x09, 0x33,        //   Usage (Rx)
        0x09, 0x34,        //   Usage (Ry)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x02,        //   Report Count (2)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
        0x09, 0x21,        //   Usage (0x21)
        0x95, 0x36,        //   Report Count (54)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
        0x85, 0x05,        //   Report ID (5)
        0x09, 0x22,        //   Usage (0x22)
        0x95, 0x1F,        //   Report Count (31)
        0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x04,        //   Report ID (4)
        0x09, 0x23,        //   Usage (0x23)
        0x95, 0x24,        //   Report Count (36)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x02,        //   Report ID (2)
        0x09, 0x24,        //   Usage (0x24)
        0x95, 0x24,        //   Report Count (36)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x08,        //   Report ID (8)
        0x09, 0x25,        //   Usage (0x25)
        0x95, 0x03,        //   Report Count (3)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x10,        //   Report ID (16)
        0x09, 0x26,        //   Usage (0x26)
        0x95, 0x04,        //   Report Count (4)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x11,        //   Report ID (17)
        0x09, 0x27,        //   Usage (0x27)
        0x95, 0x02,        //   Report Count (2)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x12,        //   Report ID (18)
        0x06, 0x02, 0xFF,  //   Usage Page (Vendor Defined 0xFF02)
        0x09, 0x21,        //   Usage (0x21)
        0x95, 0x0F,        //   Report Count (15)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x13,        //   Report ID (19)
        0x09, 0x22,        //   Usage (0x22)
        0x95, 0x16,        //   Report Count (22)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x14,        //   Report ID (20)
        0x06, 0x05, 0xFF,  //   Usage Page (Vendor Defined 0xFF05)
        0x09, 0x20,        //   Usage (0x20)
        0x95, 0x10,        //   Report Count (16)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x15,        //   Report ID (21)
        0x09, 0x21,        //   Usage (0x21)
        0x95, 0x2C,        //   Report Count (44)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x06, 0x80, 0xFF,  //   Usage Page (Vendor Defined 0xFF80)
        0x85, 0x80,        //   Report ID (128)
        0x09, 0x20,        //   Usage (0x20)
        0x95, 0x06,        //   Report Count (6)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x81,        //   Report ID (129)
        0x09, 0x21,        //   Usage (0x21)
        0x95, 0x06,        //   Report Count (6)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x82,        //   Report ID (130)
        0x09, 0x22,        //   Usage (0x22)
        0x95, 0x05,        //   Report Count (5)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x83,        //   Report ID (131)
        0x09, 0x23,        //   Usage (0x23)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x84,        //   Report ID (132)
        0x09, 0x24,        //   Usage (0x24)
        0x95, 0x04,        //   Report Count (4)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x85,        //   Report ID (133)
        0x09, 0x25,        //   Usage (0x25)
        0x95, 0x06,        //   Report Count (6)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x86,        //   Report ID (134)
        0x09, 0x26,        //   Usage (0x26)
        0x95, 0x06,        //   Report Count (6)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x87,        //   Report ID (135)
        0x09, 0x27,        //   Usage (0x27)
        0x95, 0x23,        //   Report Count (35)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x88,        //   Report ID (136)
        0x09, 0x28,        //   Usage (0x28)
        0x95, 0x22,        //   Report Count (34)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x89,        //   Report ID (137)
        0x09, 0x29,        //   Usage (0x29)
        0x95, 0x02,        //   Report Count (2)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x90,        //   Report ID (144)
        0x09, 0x30,        //   Usage (0x30)
        0x95, 0x05,        //   Report Count (5)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x91,        //   Report ID (145)
        0x09, 0x31,        //   Usage (0x31)
        0x95, 0x03,        //   Report Count (3)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x92,        //   Report ID (146)
        0x09, 0x32,        //   Usage (0x32)
        0x95, 0x03,        //   Report Count (3)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0x93,        //   Report ID (147)
        0x09, 0x33,        //   Usage (0x33)
        0x95, 0x0C,        //   Report Count (12)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA0,        //   Report ID (160)
        0x09, 0x40,        //   Usage (0x40)
        0x95, 0x06,        //   Report Count (6)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA1,        //   Report ID (161)
        0x09, 0x41,        //   Usage (0x41)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA2,        //   Report ID (162)
        0x09, 0x42,        //   Usage (0x42)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA3,        //   Report ID (163)
        0x09, 0x43,        //   Usage (0x43)
        0x95, 0x30,        //   Report Count (48)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA4,        //   Report ID (164)
        0x09, 0x44,        //   Usage (0x44)
        0x95, 0x0D,        //   Report Count (13)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA5,        //   Report ID (165)
        0x09, 0x45,        //   Usage (0x45)
        0x95, 0x15,        //   Report Count (21)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA6,        //   Report ID (166)
        0x09, 0x46,        //   Usage (0x46)
        0x95, 0x15,        //   Report Count (21)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xF0,        //   Report ID (240)
        0x09, 0x47,        //   Usage (0x47)
        0x95, 0x3F,        //   Report Count (63)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xF1,        //   Report ID (241)
        0x09, 0x48,        //   Usage (0x48)
        0x95, 0x3F,        //   Report Count (63)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xF2,        //   Report ID (242)
        0x09, 0x49,        //   Usage (0x49)
        0x95, 0x0F,        //   Report Count (15)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA7,        //   Report ID (167)
        0x09, 0x4A,        //   Usage (0x4A)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA8,        //   Report ID (168)
        0x09, 0x4B,        //   Usage (0x4B)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xA9,        //   Report ID (169)
        0x09, 0x4C,        //   Usage (0x4C)
        0x95, 0x08,        //   Report Count (8)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xAA,        //   Report ID (170)
        0x09, 0x4E,        //   Usage (0x4E)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xAB,        //   Report ID (171)
        0x09, 0x4F,        //   Usage (0x4F)
        0x95, 0x39,        //   Report Count (57)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xAC,        //   Report ID (172)
        0x09, 0x50,        //   Usage (0x50)
        0x95, 0x39,        //   Report Count (57)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xAD,        //   Report ID (173)
        0x09, 0x51,        //   Usage (0x51)
        0x95, 0x0B,        //   Report Count (11)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xAE,        //   Report ID (174)
        0x09, 0x52,        //   Usage (0x52)
        0x95, 0x01,        //   Report Count (1)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xAF,        //   Report ID (175)
        0x09, 0x53,        //   Usage (0x53)
        0x95, 0x02,        //   Report Count (2)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0x85, 0xB0,        //   Report ID (176)
        0x09, 0x54,        //   Usage (0x54)
        0x95, 0x3F,        //   Report Count (63)
        0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
        0xC0,              // End Collection
    };

    struct _URB_CONTROL_DESCRIPTOR_REQUEST* pRequest = &urb->UrbControlDescriptorRequest;

    KdPrint((">> >> >> _URB_CONTROL_DESCRIPTOR_REQUEST: Buffer Length %d\n", pRequest->TransferBufferLength));

    switch (pCommon->TargetType)
    {
    case DualShock4Wired:
    {
        if (pRequest->TransferBufferLength >= DS4_HID_REPORT_DESCRIPTOR_SIZE)
        {
            RtlCopyMemory(pRequest->TransferBuffer, Ds4HidReportDescriptor, DS4_HID_REPORT_DESCRIPTOR_SIZE);
            status = STATUS_SUCCESS;
        }
    }
    default:
        break;
    }

    return status;
}

