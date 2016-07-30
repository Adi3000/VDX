/*
MIT License

Copyright (c) 2016 Benjamin "Nefarius" Höglinger

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


// ViGEmUM.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ViGEmUM.h"
#include <SetupAPI.h>
#include <stdlib.h>
#include <winioctl.h>
#include <thread>

HANDLE g_hViGEmBus = INVALID_HANDLE_VALUE;


VIGEM_API VIGEM_ERROR vigem_init()
{
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
    DWORD memberIndex = 0;
    DWORD requiredSize = 0;
    VIGEM_ERROR error = VIGEM_ERROR_BUS_NOT_FOUND;

    auto deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_BUSENUM_VIGEM, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_BUSENUM_VIGEM, memberIndex, &deviceInterfaceData))
    {
        // get required target buffer size
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, nullptr, 0, &requiredSize, nullptr);

        // allocate target buffer
        auto detailDataBuffer = static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));
        detailDataBuffer->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        // get detail buffer
        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, detailDataBuffer, requiredSize, &requiredSize, nullptr))
        {
            SetupDiDestroyDeviceInfoList(deviceInfoSet);
            free(detailDataBuffer);
            error = VIGEM_ERROR_BUS_NOT_FOUND;
            continue;
        }

        if (g_hViGEmBus != INVALID_HANDLE_VALUE)
        {
            CloseHandle(g_hViGEmBus);
        }

        // bus found, open it
        g_hViGEmBus = CreateFile(detailDataBuffer->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            nullptr);

        free(detailDataBuffer);

        error = VIGEM_ERROR_NONE;

        break;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return error;
}

VIGEM_API VOID vigem_shutdown()
{
    if (g_hViGEmBus != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_hViGEmBus);
        g_hViGEmBus = INVALID_HANDLE_VALUE;
    }
}

VIGEM_API VIGEM_ERROR vigem_register_xusb_notification(
    VIGEM_XUSB_NOTIFICATION Notification,
    VIGEM_TARGET Target)
{
    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target.SerialNo == 0 || Notification == nullptr)
    {
        return VIGEM_ERROR_INVALID_TARGET;
    }

    std::thread _async{ [](
        VIGEM_XUSB_NOTIFICATION _Notification,
        VIGEM_TARGET _Target)
    {
        DWORD error = ERROR_SUCCESS;
        DWORD transfered = 0;
        OVERLAPPED lOverlapped = { 0 };
        lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        XUSB_REQUEST_NOTIFICATION notify;
        XUSB_REQUEST_NOTIFICATION_INIT(&notify, _Target.SerialNo);

        do
        {
            DeviceIoControl(g_hViGEmBus, IOCTL_XUSB_REQUEST_NOTIFICATION, &notify, notify.Size, &notify, notify.Size, &transfered, &lOverlapped);

            if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) != 0)
            {
                _Notification(_Target, notify.LargeMotor, notify.SmallMotor, notify.LedNumber);
            }
            else
            {
                error = GetLastError();
            }
        } while (error != ERROR_OPERATION_ABORTED && error != ERROR_ACCESS_DENIED);

        CloseHandle(lOverlapped.hEvent);

    }, Notification, Target };

    _async.detach();

    return VIGEM_ERROR_NONE;
}

VIGEM_API VIGEM_ERROR vigem_target_plugin(
    VIGEM_TARGET_TYPE Type,
    PVIGEM_TARGET Target)
{
    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target->State == VigemTargetNew)
    {
        return VIGEM_ERROR_TARGET_UNINITIALIZED;
    }

    if (Target->State == VigemTargetConnected)
    {
        return VIGEM_ERROR_ALREADY_CONNECTED;
    }

    DWORD transfered = 0;
    VIGEM_PLUGIN_TARGET plugin;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    for (Target->SerialNo = 1; Target->SerialNo <= VIGEM_TARGETS_MAX; Target->SerialNo++)
    {
        VIGEM_PLUGIN_TARGET_INIT(&plugin, Target->SerialNo, Type);

        DeviceIoControl(g_hViGEmBus, IOCTL_VIGEM_PLUGIN_TARGET, &plugin, plugin.Size, nullptr, 0, &transfered, &lOverlapped);

        if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) != 0)
        {
            Target->State = VigemTargetConnected;
            CloseHandle(lOverlapped.hEvent);
            return VIGEM_ERROR_NONE;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NO_FREE_SLOT;
}

VIGEM_API VIGEM_ERROR vigem_target_unplug(PVIGEM_TARGET Target)
{
    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target->State == VigemTargetNew)
    {
        return VIGEM_ERROR_TARGET_UNINITIALIZED;
    }

    if (Target->State != VigemTargetConnected)
    {
        return VIGEM_ERROR_TARGET_NOT_PLUGGED_IN;
    }

    DWORD transfered = 0;
    VIGEM_UNPLUG_TARGET unplug;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    VIGEM_UNPLUG_TARGET_INIT(&unplug, Target->SerialNo);

    DeviceIoControl(g_hViGEmBus, IOCTL_VIGEM_UNPLUG_TARGET, &unplug, unplug.Size, nullptr, 0, &transfered, &lOverlapped);

    if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) != 0)
    {
        Target->State = VigemTargetDisconnected;
        CloseHandle(lOverlapped.hEvent);
        return VIGEM_ERROR_NONE;
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_REMOVAL_FAILED;
}

VIGEM_API VIGEM_ERROR vigem_register_ds4_notification(
    VIGEM_DS4_NOTIFICATION Notification,
    VIGEM_TARGET Target)
{
    // TODO: de-duplicate this section

    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target.SerialNo == 0 || Notification == nullptr)
    {
        return VIGEM_ERROR_INVALID_TARGET;
    }

    std::thread _async{ [](
        VIGEM_DS4_NOTIFICATION _Notification,
        VIGEM_TARGET _Target)
    {
        DWORD error = ERROR_SUCCESS;
        DWORD transfered = 0;
        OVERLAPPED lOverlapped = { 0 };
        lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        DS4_REQUEST_NOTIFICATION notify;
        DS4_REQUEST_NOTIFICATION_INIT(&notify, _Target.SerialNo);

        do
        {
            DeviceIoControl(g_hViGEmBus, IOCTL_DS4_REQUEST_NOTIFICATION, &notify, notify.Size, &notify, notify.Size, &transfered, &lOverlapped);

            if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) != 0)
            {
                _Notification(_Target, notify.Report.LargeMotor, notify.Report.SmallMotor, notify.Report.LightbarColor);
            }
            else
            {
                error = GetLastError();
            }
        } while (error != ERROR_OPERATION_ABORTED && error != ERROR_ACCESS_DENIED);

        CloseHandle(lOverlapped.hEvent);

    }, Notification, Target };

    _async.detach();

    return VIGEM_ERROR_NONE;
}

VIGEM_API VIGEM_ERROR vigem_xusb_submit_report(
    VIGEM_TARGET Target,
    XUSB_REPORT Report)
{
    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target.SerialNo == 0)
    {
        return VIGEM_ERROR_INVALID_TARGET;
    }

    DWORD transfered = 0;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    XUSB_SUBMIT_REPORT report;
    XUSB_SUBMIT_REPORT_INIT(&report, Target.SerialNo);

    report.Report = Report;

    DeviceIoControl(g_hViGEmBus, IOCTL_XUSB_SUBMIT_REPORT, &report, report.Size, nullptr, 0, &transfered, &lOverlapped);

    if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) == 0)
    {
        CloseHandle(lOverlapped.hEvent);

        switch (GetLastError())
        {
        case ERROR_ACCESS_DENIED:
            return VIGEM_ERROR_INVALID_TARGET;
            break;
        default:
            break;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NONE;
}

VIGEM_API VIGEM_ERROR vigem_ds4_submit_report(VIGEM_TARGET Target, DS4_REPORT Report)
{
    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target.SerialNo == 0)
    {
        return VIGEM_ERROR_INVALID_TARGET;
    }

    DWORD transfered = 0;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    DS4_SUBMIT_REPORT report;
    DS4_SUBMIT_REPORT_INIT(&report, Target.SerialNo);

    report.Report = Report;

    DeviceIoControl(g_hViGEmBus, IOCTL_DS4_SUBMIT_REPORT, &report, report.Size, nullptr, 0, &transfered, &lOverlapped);

    if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) == 0)
    {
        CloseHandle(lOverlapped.hEvent);

        switch (GetLastError())
        {
        case ERROR_ACCESS_DENIED:
            return VIGEM_ERROR_INVALID_TARGET;
            break;
        default:
            break;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NONE;
}

VIGEM_API VIGEM_ERROR vigem_xgip_submit_report(VIGEM_TARGET Target, XGIP_REPORT Report)
{
    if (g_hViGEmBus == nullptr)
    {
        return VIGEM_ERROR_BUS_NOT_FOUND;
    }

    if (Target.SerialNo == 0)
    {
        return VIGEM_ERROR_INVALID_TARGET;
    }

    DWORD transfered = 0;
    OVERLAPPED lOverlapped = { 0 };
    lOverlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    XGIP_SUBMIT_REPORT report;
    XGIP_SUBMIT_REPORT_INIT(&report, Target.SerialNo);

    report.Report = Report;

    DeviceIoControl(g_hViGEmBus, IOCTL_XGIP_SUBMIT_REPORT, &report, report.Size, nullptr, 0, &transfered, &lOverlapped);

    if (GetOverlappedResult(g_hViGEmBus, &lOverlapped, &transfered, TRUE) == 0)
    {
        CloseHandle(lOverlapped.hEvent);

        switch (GetLastError())
        {
        case ERROR_ACCESS_DENIED:
            return VIGEM_ERROR_INVALID_TARGET;
            break;
        default:
            break;
        }
    }

    CloseHandle(lOverlapped.hEvent);

    return VIGEM_ERROR_NONE;
}

