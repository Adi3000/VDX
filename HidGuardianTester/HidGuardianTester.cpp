// HidGuardianTester.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>
#include <initguid.h>
#include "../XnaGuardian/Public.h"

#pragma comment(lib, "hid.lib")

typedef void (WINAPI* HidGuardianOpen_t)();
typedef void (WINAPI* HidGuardianClose_t)();


int main()
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
    DWORD memberIndex = 0;
    DWORD requiredSize = 0;

    GUID hidClass;
    HidD_GetHidGuid(&hidClass);

#ifdef HIDGUARDIAN
    HMODULE cerberus = LoadLibrary(L"HidCerberus.Lib.dll");

    HidGuardianOpen_t fpOpen = reinterpret_cast<HidGuardianOpen_t>(GetProcAddress(cerberus, "HidGuardianOpen"));
    HidGuardianOpen_t fpClose = reinterpret_cast<HidGuardianOpen_t>(GetProcAddress(cerberus, "HidGuardianClose"));

    fpOpen();
#endif

    auto deviceInfoSet = SetupDiGetClassDevs(&XUSB_INTERFACE_CLASS_GUID, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    while (SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &XUSB_INTERFACE_CLASS_GUID, memberIndex++, &deviceInterfaceData))
    {
        printf("Found one!\n");

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

            continue;
        }

        printf("%ls\n", detailDataBuffer->DevicePath);

        if (hDevice != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hDevice);
        }

        // device found, open it
        hDevice = CreateFile(detailDataBuffer->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        printf("hDevice = 0x%p, error = %d\n", hDevice, GetLastError());

        DWORD retval;

        XINPUT_EXT_HIDE_GAMEPAD hidePad;
        XINPUT_EXT_HIDE_GAMEPAD_INIT(&hidePad, 0, TRUE);

        BOOLEAN ret = DeviceIoControl(
            hDevice,
            IOCTL_XINPUT_EXT_HIDE_GAMEPAD,
            static_cast<LPVOID>(&hidePad),
            hidePad.Size,
            nullptr,
            0,
            &retval,
            nullptr);

        printf("DeviceIoControl = %d, error = %d\n", ret, GetLastError());

        free(detailDataBuffer);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    printf("Done\n");
    getchar();

#ifdef HIDGUARDIAN
    fpClose();
#endif

    return 0;
}

