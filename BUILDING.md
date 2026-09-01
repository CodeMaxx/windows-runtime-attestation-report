# Building the sample from scratch

Everything you need to compile and run this sample on a clean Windows machine.
Nothing here assumes any pre-installed developer tooling.

## 1. Prerequisites

### 1.1 C++ toolchain

Install either **Visual Studio 2022** (Community is free) or the **Build Tools
for Visual Studio 2022** (no IDE, command-line only), with:

- Workload: **Desktop development with C++**, for the MSVC v143 compiler and
  MSBuild.
- Component: **MSVC v143 - VS 2022 C++ ARM64 build tools**, needed only for the
  ARM64 configuration. The workload on its own installs just the x86/x64
  compiler.

You do **not** need a Windows SDK component; the SDK comes from NuGet (§1.2).
Any Windows 10 or 11 host will do, x64 or ARM64, whichever target you build.

### 1.2 Windows SDK 10.0.29648

`GetRuntimeAttestationReport` (`sysinfoapi.h`) and the runtime-report structures
(`winnt.h`) need Windows SDK **10.0.29648**. That is a build-time requirement;
see §4 for what it takes to *run* the sample.

The SDK ships as a NuGet package rather than an installer, pinned by
`src\packages.config` to **`10.0.29648.1000-preview`**:

| Package | Provides |
|---|---|
| `Microsoft.Windows.SDK.CPP` | Headers and shared build props |
| `Microsoft.Windows.SDK.CPP.x64` | x64 import libraries |
| `Microsoft.Windows.SDK.CPP.arm64` | ARM64 import libraries |

A restore (§3) downloads them into `packages\`. Command-line builds need
`nuget.exe` on your `PATH` — from <https://www.nuget.org/downloads>, or
`winget install Microsoft.NuGet`. Visual Studio restores on its own.

### 1.3 Unattended install

To set up a fresh machine without the installer UI:

```powershell
Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile vs_BuildTools.exe

# VC.Tools.x86.x64 is the compiler itself and must be listed explicitly; the
# workload alone does not pull it in. Drop the ARM64 line if you only need x64.
.\vs_BuildTools.exe --quiet --wait --norestart `
  --add Microsoft.VisualStudio.Workload.VCTools `
  --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
  --add Microsoft.VisualStudio.Component.VC.Tools.ARM64
```

> With Build Tools rather than the full IDE, `vswhere` hides the installation
> unless you pass **`-products *`**:
> ```powershell
> $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
> $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
> ```

## 2. Get the code

```powershell
git clone https://github.com/CodeMaxx/windows-runtime-attestation-report.git
cd windows-runtime-attestation-report
```

Or download a ZIP from the repository page.

## 3. Build

In Visual Studio, open `WindowsRuntimeAttestationReport.sln`, select **Release**
and either **x64** or **ARM64**, and build. If the SDK packages do not restore
by themselves, right-click the solution and choose **Restore NuGet Packages**.

From a **Developer Command Prompt for VS 2022**:

```powershell
nuget restore WindowsRuntimeAttestationReport.sln
msbuild WindowsRuntimeAttestationReport.sln /p:Configuration=Release /p:Platform=x64
msbuild WindowsRuntimeAttestationReport.sln /p:Configuration=Release /p:Platform=ARM64
```

The restore only has to run once. The executable lands in
`<Platform>\Release\WindowsRuntimeAttestationReport.exe`.

## 4. Run

```powershell
x64\Release\WindowsRuntimeAttestationReport.exe [--type driver|hotpatch|all]
```

- **Windows 11 25H2 or later** for the driver report.
- **Windows 11 Insider Preview build 29591.1000 or later** for the hotpatch
  report; earlier builds return `ERROR_INVALID_PARAMETER` for that type.
- **VBS** enabled, plus **HVCI** for the driver report. Without HVCI that call
  returns `ERROR_NOT_SUPPORTED` (50).

The sample links the C runtime statically, so the executable is self-contained:
copy it to a clean machine and it runs, with no redistributable to install.

## 5. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `TRK0005: Failed to locate: "CL.exe"` / `MSB8003: VCToolsInstallDir ... not defined` | The MSVC compiler for that platform is not installed. For x64 add **MSVC v143 - VS 2022 C++ x64/x86 build tools**; for ARM64 add **MSVC v143 - VS 2022 C++ ARM64 build tools** (see §1.1). |
| `This project references NuGet package(s) that are missing on this computer` | The SDK packages have not been restored. Run `nuget restore WindowsRuntimeAttestationReport.sln` (see §3). |
| `error: size query failed (50)` / `error: GetRuntimeAttestationReport failed (50)` | Windows returned `ERROR_NOT_SUPPORTED`. Verify that the Windows build and security configuration meet the §4 requirements for the requested report. |
| `error: one or more parameters passed is invalid, or the requested report type is not supported on this version of Windows` | The requested report type is not supported on this build of Windows. |
