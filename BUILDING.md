# Building the sample from scratch

This guide lists **everything** you need to compile and run this sample on a
clean machine — nothing here assumes any pre-installed developer tooling. If
you follow it top to bottom on a fresh Windows install, it will build.

## 1. Prerequisites

### 1.1 Build machine

- **Windows 10 or 11, x64.** You can *build* on Windows 10; you can only
  *run* the sample meaningfully on a Windows 11 build that ships the API (see
  §4).

### 1.2 C++ toolchain (required)

Install **one** of the following. Both give you the MSVC v143 compiler and
MSBuild:

- **Visual Studio 2022** (Community is free), **or**
- **Build Tools for Visual Studio 2022** (no IDE, command-line only).

When installing, you **must** select:

- Workload: **Desktop development with C++**
- Individual component: **Windows 11 SDK (10.0.26100)** — you need servicing
  build **10.0.26100.7705 or later** (see §1.3 for why).

### 1.3 Windows 11 SDK 10.0.26100.7705 or later (required)

The `GetRuntimeAttestationReport` prototype and the runtime-report structures
(`RUNTIME_REPORT_PACKAGE_HEADER`, `DRIVER_INFO_ENTRY`, etc.) were added in the
**10.0.26100.7705** servicing update of the Windows 11 SDK (`sysinfoapi.h`,
`winnt.h`).

The project file targets `WindowsTargetPlatformVersion` `10.0.26100.0`, the
26100 platform folder that the servicing update (10.0.26100.7705 or later)
installs into. If you use a different SDK version, retarget the project
(right-click the project in Visual Studio → **Retarget** to your installed SDK).

### 1.4 Git (optional)

Only needed if you want to `git clone` the repository. You can also download a
ZIP from the repository page.

### 1.5 Scripted install (for a fresh VM)

On a clean machine, the entire toolchain can be installed unattended. Use the VS
Build Tools bootstrapper directly (this does not require `winget`):

```powershell
# Download the Build Tools bootstrapper
Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile vs_BuildTools.exe

# Install: C++ compiler + Windows 11 SDK. The VC.Tools.x86.x64 component is the
# MSVC compiler itself and MUST be listed explicitly -- adding only the VCTools
# workload does NOT pull the compiler unless you also pass --includeRecommended.
.\vs_BuildTools.exe --quiet --wait --norestart `
  --add Microsoft.VisualStudio.Workload.VCTools `
  --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
  --add Microsoft.VisualStudio.Component.Windows11SDK.26100
```

If you have `winget`, you can instead install the full IDE
(`winget install --id Microsoft.VisualStudio.2022.Community`) and select the
**Desktop development with C++** workload plus the **Windows 11 SDK
(10.0.26100)** component (servicing build 10.0.26100.7705 or later) in the
installer UI.

> **Locating MSBuild with Build Tools:** when you install *Build Tools* (not the
> full IDE), `vswhere` excludes it by default. Pass **`-products *`**:
> ```powershell
> $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
> $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
> ```

## 2. Get the code

```powershell
git clone https://github.com/CodeMaxx/windows-runtime-attestation-report.git
cd windows-runtime-attestation-report
```

## 3. Build

### Option A — Visual Studio IDE

1. Open `WindowsRuntimeAttestationReport.sln`.
2. Set the configuration to **Release** and the platform to **x64** or **ARM64**.
3. **Build → Build Solution** (Ctrl+Shift+B).

### Option B — command line

Open a **Developer Command Prompt for VS 2022** (or a **Developer PowerShell**),
`cd` to the repository, then:

```powershell
msbuild WindowsRuntimeAttestationReport.sln /p:Configuration=Release /p:Platform=x64
msbuild WindowsRuntimeAttestationReport.sln /p:Configuration=Release /p:Platform=ARM64
```

The executable is written to `<Platform>\Release\WindowsRuntimeAttestationReport.exe`
(e.g. `x64\Release\...` or `ARM64\Release\...`).

## 4. Run

The program accepts an optional `--type` argument selecting which report(s) to
request (`driver` is the default):

```powershell
x64\Release\WindowsRuntimeAttestationReport.exe [--type driver|hotpatch|all]
```

**Runtime requirements:**

- **Windows 11 25H2 or later.**
- **VBS** enabled, plus **HVCI** for the driver report. Without HVCI the driver
  report call returns `ERROR_NOT_SUPPORTED` (50).

## 5. Linkage

`GetRuntimeAttestationReport` comes from `mincore.lib` and `BCryptGenRandom`
from `bcrypt.lib`. `main.cpp` pulls both in automatically with
`#pragma comment(lib, ...)` directives, so you do not need to add them to the
project's linker settings.

## 6. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `TRK0005: Failed to locate: "CL.exe"` / `MSB8003: VCToolsInstallDir ... not defined` | The MSVC compiler component is not installed. Add **MSVC v143 - VS 2022 C++ x64/x86 build tools** (see §1). |
| Compile error: unknown type `RUNTIME_REPORT_PACKAGE_HEADER` | Your 26100 SDK is older than servicing build 10.0.26100.7705. Install 10.0.26100.7705 or later. |
| Runtime `ERROR_NOT_SUPPORTED` (50) | HVCI / VBS is not enabled. Turn on memory integrity (HVCI). |
| The API cannot be found at runtime | The runtime attestation report requires Windows 11 25H2 or later. |
