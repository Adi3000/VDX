# XInput/x360ce to ViGEm sample application

Adds ViGEm support to Xbox 360/One controllers and unmodified x360ce application.

[![Build status](https://ci.appveyor.com/api/projects/status/5bsea7nmggf08w2n?svg=true)](https://ci.appveyor.com/project/nefarius/vdx) [![Discord](https://img.shields.io/discord/346756263763378176.svg)](https://discord.vigem.org) [![Website](https://img.shields.io/website-up-down-green-red/https/vigem.org.svg?label=ViGEm.org)](https://vigem.org/) [![PayPal Donate](https://img.shields.io/badge/paypal-donate-blue.svg)](<https://paypal.me/NefariusMaximus>) [![Support on Patreon](https://img.shields.io/badge/patreon-donate-orange.svg)](<https://www.patreon.com/nefarius>) [![GitHub followers](https://img.shields.io/github/followers/nefarius.svg?style=social&label=Follow)](https://github.com/nefarius) [![Twitter Follow](https://img.shields.io/twitter/follow/nefariusmaximus.svg?style=social&label=Follow)](https://twitter.com/nefariusmaximus)

![explorer_2018-08-31_21-56-18.png](https://lh3.googleusercontent.com/-Ib62elVsRtQ/W4mdcwQj0uI/AAAAAAAABuM/ajU1PiueT7IRHfO68wTKhuaAdgTsPZFEgCHMYCw/s0/explorer_2018-08-31_21-56-18.png)

## How to use

👉 [Documentation](https://forums.vigem.org/topic/272/x360ce-to-vigem)

## How to build

### Prerequisites

- Visual Studio **2019** ([Community Edition](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) is just fine)
- [.NET Core SDK 2.1](https://www.microsoft.com/net/download/dotnet-core/2.1) (or greater, required for building only)

Then simply run the build from the command line:

```PowerShell
powershell.exe .\build.ps1 -configuration release
```

```PowerShell
.\vcpkg.exe install sfml:x86-windows-static sfml:x64-windows-static imgui:x86-windows-static imgui:x64-windows-static imgui-sfml:x86-windows-static imgui-sfml:x64-windows-static hidapi:x86-windows-static hidapi:x64-windows-static
```

## 3rd party stuff used

- [Simple and Fast Multimedia Library](https://www.sfml-dev.org/)
- [dear imgui](https://github.com/ocornut/imgui)
- [ImGui + SFML](https://github.com/eliasdaler/imgui-sfml)
