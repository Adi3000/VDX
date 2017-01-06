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


#include "driver.h"
#include "queue.tmh"
#include "XInputInternal.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, XnaGuardianQueueInitialize)
#endif

NTSTATUS
XnaGuardianQueueInitialize(
    _In_ WDFDEVICE Device
)
{
    WDFQUEUE                queue;
    NTSTATUS                status;
    WDF_IO_QUEUE_CONFIG     queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
    );

    queueConfig.EvtIoDefault = XnaGuardianEvtIoDefault;
    queueConfig.EvtIoDeviceControl = XnaGuardianEvtIoDeviceControl;

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &queue
    );

    if (!NT_SUCCESS(status))
    {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

//
// Forward everything we're not interested in.
// 
VOID XnaGuardianEvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request
)
{
    WDF_REQUEST_SEND_OPTIONS    options;
    NTSTATUS                    status;
    BOOLEAN                     ret;

    KdPrint((DRIVERNAME "XnaGuardianEvtIoDefault called\n"));

    WDF_REQUEST_SEND_OPTIONS_INIT(&options,
        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    ret = WdfRequestSend(Request, WdfDeviceGetIoTarget(WdfIoQueueGetDevice(Queue)), &options);

    if (ret == FALSE)
    {
        status = WdfRequestGetStatus(Request);
        KdPrint((DRIVERNAME "WdfRequestSend failed: 0x%x\n", status));
        WdfRequestComplete(Request, status);
    }
}

VOID XnaGuardianEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    WDF_REQUEST_SEND_OPTIONS        options;
    NTSTATUS                        status;
    BOOLEAN                         ret;
    size_t                          buflen;
    PDEVICE_CONTEXT                 pDeviceContext;
    PVOID                           pBuffer;
    PXINPUT_EXT_HIDE_GAMEPAD        pHidePad;
    PXINPUT_EXT_OVERRIDE_GAMEPAD    pOverride;
    WDFDEVICE                       Device;
    WDF_OBJECT_ATTRIBUTES           requestAttribs;
    PXINPUT_PAD_IDENTIFIER_CONTEXT  pXInputContext = NULL;

    KdPrint((DRIVERNAME "XnaGuardianEvtIoDeviceControl called with code 0x%X\n", IoControlCode));

    KdPrint((DRIVERNAME "InputBufferLength = 0x%X\n", InputBufferLength));
    KdPrint((DRIVERNAME "OutputBufferLength = 0x%X\n", OutputBufferLength));

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    Device = WdfIoQueueGetDevice(Queue);
    pDeviceContext = DeviceGetContext(Device);

    KdPrint((DRIVERNAME "Device = 0x%X\n", Device));

    //
    // Filter desired I/O-control codes
    // 
    switch (IoControlCode)
    {
#pragma region IOCTL_XINPUT_GET_INFORMATION
        //
        // Filter GetDeviceInfoFromInterface(...) call
        // 
    case IOCTL_XINPUT_GET_INFORMATION:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_GET_INFORMATION\n"));

        WdfRequestFormatRequestUsingCurrentType(Request);
        WdfRequestSetCompletionRoutine(Request, XInputGetInformationCompleted, Device);

        ret = WdfRequestSend(Request, WdfDeviceGetIoTarget(Device), WDF_NO_SEND_OPTIONS);

        if (!ret) {
            status = WdfRequestGetStatus(Request);
            KdPrint((DRIVERNAME "WdfRequestSend failed: 0x%x\n", status));
        }

        return;
#pragma endregion

    case IOCTL_XINPUT_GET_CAPABILITIES:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_GET_CAPABILITIES\n"));
        break;

    case IOCTL_XINPUT_GET_LED_STATE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_GET_LED_STATE\n"));
        break;

        //
        // Filter GetLatestDeviceInfo(...) call
        // 
    case IOCTL_XINPUT_GET_GAMEPAD_STATE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_GET_GAMEPAD_STATE\n"));

        status = WdfRequestRetrieveInputBuffer(Request, IO_GET_GAMEPAD_STATE_IN_SIZE, &pBuffer, &buflen);

        KdPrint((DRIVERNAME "[IOCTL_XINPUT_GET_GAMEPAD_STATE] [0x%X] [I] ", Device));

        if (NT_SUCCESS(status))
        {
            for (size_t i = 0; i < buflen; i++)
            {
                KdPrint(("%02X ", ((PUCHAR)pBuffer)[i]));
            }

            KdPrint(("\n"));
        }

        WdfRequestFormatRequestUsingCurrentType(Request);
        WdfRequestSetCompletionRoutine(Request, XInputGetGamepadStateCompleted, Device);

        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttribs, XINPUT_PAD_IDENTIFIER_CONTEXT);

        status = WdfObjectAllocateContext(
            Request,
            &requestAttribs,
            (PVOID)&pXInputContext
        );

        if (NT_SUCCESS(status))
        {
            pXInputContext->Index = ((PUCHAR)pBuffer)[2];
        }

        ret = WdfRequestSend(Request, WdfDeviceGetIoTarget(Device), WDF_NO_SEND_OPTIONS);

        if (!ret) {
            status = WdfRequestGetStatus(Request);
            KdPrint((DRIVERNAME "WdfRequestSend failed: 0x%x\n", status));
        }

        return;

        //
        // Filter SendLEDState(...) call
        // 
    case IOCTL_XINPUT_SET_GAMEPAD_STATE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_SET_GAMEPAD_STATE\n"));

        status = WdfRequestRetrieveInputBuffer(Request, IO_SET_GAMEPAD_STATE_IN_SIZE, &pBuffer, &buflen);

        KdPrint((DRIVERNAME "[IOCTL_XINPUT_SET_GAMEPAD_STATE] [0x%X] [I] ", Device));

        if (NT_SUCCESS(status))
        {
            //
            // Identify LED request
            // 
            if (((PUCHAR)pBuffer)[4] == 0x01)
            {
                pDeviceContext->LedValues[((PUCHAR)pBuffer)[0]] = ((PUCHAR)pBuffer)[1];
            }

            for (size_t i = 0; i < buflen; i++)
            {
                KdPrint(("%02X ", ((PUCHAR)pBuffer)[i]));
            }

            KdPrint(("\n"));
        }

        break;

    case IOCTL_XINPUT_WAIT_FOR_GUIDE_BUTTON:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_WAIT_FOR_GUIDE_BUTTON\n"));
        break;

    case IOCTL_XINPUT_GET_BATTERY_INFORMATION:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_GET_BATTERY_INFORMATION\n"));
        break;

    case IOCTL_XINPUT_POWER_DOWN_DEVICE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_POWER_DOWN_DEVICE\n"));
        break;

    case IOCTL_XINPUT_GET_AUDIO_INFORMATION:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_GET_AUDIO_INFORMATION\n"));
        break;

#pragma region IOCTL_XINPUT_EXT_HIDE_GAMEPAD
    case IOCTL_XINPUT_EXT_HIDE_GAMEPAD:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_EXT_HIDE_GAMEPAD\n"));

        // 
        // Retrieve input buffer
        // 
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XINPUT_EXT_HIDE_GAMEPAD), &pBuffer, &buflen);
        if (!NT_SUCCESS(status) || buflen < sizeof(XINPUT_EXT_HIDE_GAMEPAD))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed with status 0x%X\n", status));
            WdfRequestComplete(Request, status);
            return;
        }

        pHidePad = (PXINPUT_EXT_HIDE_GAMEPAD)pBuffer;

        //
        // Validate padding
        // 
        if (pHidePad->Size != sizeof(XINPUT_EXT_HIDE_GAMEPAD))
        {
            WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
            return;
        }

        //
        // Set pad state
        // 
        PadStates[pHidePad->UserIndex].IsGetStateForbidden = pHidePad->Hidden;

        //
        // Complete request
        // 
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
#pragma endregion

#pragma region IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE
    case IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE:

        KdPrint((DRIVERNAME ">> IOCTL_XINPUT_EXT_OVERRIDE_GAMEPAD_STATE\n"));

        // 
        // Retrieve input buffer
        // 
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD), &pBuffer, &buflen);
        if (!NT_SUCCESS(status) || buflen < sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD))
        {
            KdPrint((DRIVERNAME "WdfRequestRetrieveInputBuffer failed with status 0x%X\n", status));
            WdfRequestComplete(Request, status);
            return;
        }

        pOverride = (PXINPUT_EXT_OVERRIDE_GAMEPAD)pBuffer;

        //
        // Validate padding
        // 
        if (pOverride->Size != sizeof(XINPUT_EXT_OVERRIDE_GAMEPAD))
        {
            WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
            return;
        }

        // 
        // Validate range
        // 
        if (pOverride->UserIndex < 0 || pOverride->UserIndex > 3)
        {
            WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
            return;
        }

        //
        // Set pad overrides
        // 
        if (
            RtlCompareMemory(
                &PadStates[pOverride->UserIndex].Overrides,
                &pOverride->Overrides,
                sizeof(ULONG)
            ) != 0)
        {
            PadStates[pOverride->UserIndex].Overrides = pOverride->Overrides;
        }
        if (
            RtlCompareMemory(
                &PadStates[pOverride->UserIndex].Gamepad,
                &pOverride->Gamepad,
                sizeof(XINPUT_GAMEPAD_STATE)
            ) != 0)
        {
            PadStates[pOverride->UserIndex].Gamepad = pOverride->Gamepad;
        }

        //
        // Complete request
        // 
        WdfRequestComplete(Request, STATUS_SUCCESS);
        return;
#pragma endregion

    default:
        break;
    }

    //
    // Not our business, forward
    // 
    WDF_REQUEST_SEND_OPTIONS_INIT(&options,
        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    ret = WdfRequestSend(Request, WdfDeviceGetIoTarget(Device), &options);

    if (ret == FALSE)
    {
        status = WdfRequestGetStatus(Request);
        KdPrint((DRIVERNAME "WdfRequestSend failed: 0x%x\n", status));
        WdfRequestComplete(Request, status);
    }
}

//
// Filter GetDeviceInfoFromInterface(...) result
// 
void XInputGetInformationCompleted(
    _In_ WDFREQUEST                     Request,
    _In_ WDFIOTARGET                    Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT                     Context
)
{
    NTSTATUS            status;
    PVOID               buffer;
    size_t              buflen;
    PDEVICE_CONTEXT     pDeviceContext;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    status = WdfRequestGetStatus(Request);
    pDeviceContext = DeviceGetContext(Context);

    KdPrint((DRIVERNAME "IOCTL_XINPUT_GET_INFORMATION called with status 0x%x\n", status));

    status = WdfRequestRetrieveOutputBuffer(Request, IO_GET_INFORMATION_OUT_SIZE, &buffer, &buflen);

    if (NT_SUCCESS(status))
    {
        pDeviceContext->MaxDevices = (ULONG)((PUCHAR)buffer)[2];

        KdPrint((DRIVERNAME "pDeviceContext->MaxDevices = %d\n", pDeviceContext->MaxDevices));

        KdPrint((DRIVERNAME "IOCTL_XINPUT_GET_INFORMATION [O] "));

        for (size_t i = 0; i < buflen; i++)
        {
            KdPrint(("%02X ", ((PUCHAR)buffer)[i]));
        }

        KdPrint(("\n"));
    }
    else
    {
        KdPrint((DRIVERNAME "WdfRequestRetrieveOutputBuffer failed with status 0x%X", status));
    }

    WdfRequestComplete(Request, status);
}

void XInputGetGamepadStateCompleted(
    _In_ WDFREQUEST                     Request,
    _In_ WDFIOTARGET                    Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT                     Context
)
{
    NTSTATUS                        status;
    PVOID                           buffer;
    size_t                          buflen;
    PXINPUT_GAMEPAD_STATE           pGamepad;
    PDEVICE_CONTEXT                 pDeviceContext;
    PXINPUT_PAD_STATE_INTERNAL      pPad;
    LONG                            padIndex = 0;
    PXINPUT_PAD_IDENTIFIER_CONTEXT  pXInputContext;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    status = WdfRequestGetStatus(Request);

    KdPrint((DRIVERNAME "IOCTL_XINPUT_GET_GAMEPAD_STATE called with status 0x%x\n", status));

    pDeviceContext = DeviceGetContext(Context);
    pXInputContext = GetPadIdentifier(Request);

    KdPrint((DRIVERNAME "pXInputContext->Index = %d\n", pXInputContext->Index));

    if (pDeviceContext->MaxDevices == 0x01)
    {
        padIndex = pDeviceContext->LedValues[0] - 0x06;
    }

    if (pDeviceContext->MaxDevices > 0x01)
    {
        padIndex = pDeviceContext->LedValues[pXInputContext->Index] - 0x06;
    }

    KdPrint((DRIVERNAME "PAD INDEX: %d\n", padIndex));

    if (padIndex < 0 || padIndex > XINPUT_MAX_DEVICES)
    {
        WdfRequestComplete(Request, status);
        return;
    }

    pPad = &PadStates[padIndex];

    // TODO: test code!
    if (padIndex == 0x00)
    {
        pPad->Overrides |= XINPUT_GAMEPAD_OVERRIDE_A;
        pPad->Gamepad.wButtons |= XINPUT_GAMEPAD_A;
    }

    status = WdfRequestRetrieveOutputBuffer(Request, IO_GET_GAMEPAD_STATE_OUT_SIZE, &buffer, &buflen);

    if (NT_SUCCESS(status))
    {
        KdPrint(("[O] "));

        for (size_t i = 0; i < buflen; i++)
        {
            KdPrint(("%02X ", ((PUCHAR)buffer)[i]));
        }

        KdPrint(("\n"));

        pGamepad = GAMEPAD_FROM_BUFFER(buffer);

        KdPrint((DRIVERNAME "pDeviceContext->LedValue = %d\n", pDeviceContext->LedValues[pXInputContext->Index]));

        //
        // Override buttons
        // 

        // D-Pad
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_DPAD_UP)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_DPAD_DOWN)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_DPAD_LEFT)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_DPAD_RIGHT)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

        // Start, Back
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_START)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_START);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_BACK)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_BACK);

        // Thumbs
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

        // Shoulders
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_LEFT_SHOULDER)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_RIGHT_SHOULDER)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

        // Face
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_A)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_A);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_B)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_B);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_X)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_X);
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_Y)
            pGamepad->wButtons |= (pPad->Gamepad.wButtons & XINPUT_GAMEPAD_Y);

        //
        // Override axes
        //

        // Triggers
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_LEFT_TRIGGER)
            pGamepad->bLeftTrigger = pPad->Gamepad.bLeftTrigger;
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_RIGHT_TRIGGER)
            pGamepad->bRightTrigger = pPad->Gamepad.bRightTrigger;

        // Left Thumb
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB_X)
            pGamepad->sThumbLX = pPad->Gamepad.sThumbLX;
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_LEFT_THUMB_Y)
            pGamepad->sThumbLY = pPad->Gamepad.sThumbLY;

        // Right Thumb
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB_X)
            pGamepad->sThumbRX = pPad->Gamepad.sThumbRX;
        if (pPad->Overrides & XINPUT_GAMEPAD_OVERRIDE_RIGHT_THUMB_Y)
            pGamepad->sThumbRY = pPad->Gamepad.sThumbRY;
    }
    else
    {
        KdPrint((DRIVERNAME "WdfRequestRetrieveOutputBuffer failed with status 0x%x\n", status));
    }

    WdfRequestComplete(Request, status);
}


