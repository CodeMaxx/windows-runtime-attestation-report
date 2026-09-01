# Windows Runtime Attestation Report — Sample

A small C++ console sample that calls the
[`GetRuntimeAttestationReport`](https://learn.microsoft.com/windows/win32/api/sysinfoapi/nf-sysinfoapi-getruntimeattestationreport)
Win32 API and prints the returned signed **Runtime Report Package**: the package
header (digest algorithm and signature scheme) and one or more decoded reports:

- **Driver report** — every loaded kernel driver with its image hash,
  publisher certificate thumbprint, and flags.
- **Hotpatch report** — every kernel-mode image that currently has a
  Microsoft-signed hotpatch applied, with its base address, image size, PE
  checksum/timestamp, and latest applied patch sequence number.

Which reports are requested and printed is controlled by `--type` (see
[Running](#running)).

The report structures come from the Windows SDK's `winnt.h`, and the API is
declared in `sysinfoapi.h`.

> Scope: this sample **retrieves, parses, and prints** the report. It does not
> perform remote attestation or verify the package's RSA-PSS signature.

> **New here?**
> [Patch or attack? Inside the Windows Secure Hotpatch Report](https://www.akashtrehan.com/secure-hotpatch-report/)
> explains what the hotpatch report is, what it proves, what it doesn't, and
> how security software should use it. This sample is the code behind it.

## Requirements

To **run**:

- Windows 11 **25H2** or later for the *driver* report.
- A Windows 11 **Insider Preview build 29591.1000** or later for the *hotpatch*
  report. Asking for it on an earlier build returns `ERROR_INVALID_PARAMETER`
  rather than an empty report.
- **VBS** enabled (required to produce any signed report).
- **HVCI** enabled — required for the *driver* report. The *hotpatch* report is
  HVCI-independent.

To **build**:

- Visual Studio 2022 or the Build Tools for Visual Studio 2022, with the
  **Desktop development with C++** workload (MSVC v143). Builds for **x64** and
  **ARM64**.
- **Windows SDK 10.0.29648**, consumed as a NuGet package.

The sample links the C runtime statically, so the executable it produces has no
dependency on the Visual C++ Redistributable and runs as a single self-contained
file.

## Project layout

```
src\
  main.cpp                Entry point: argument parsing, report acquisition,
                          package envelope walk, and dispatch to each decoder.
  Helpers.h/.cpp          Bounds checks, structure overlay, hash-algorithm
                          table, and hex/blob/banner printing.
  DriverReport.h/.cpp     Driver report decoder.
  HotpatchReport.h/.cpp   Hotpatch report decoder.
  packages.config         Windows SDK NuGet package reference.
```

## Building

Open `WindowsRuntimeAttestationReport.sln` in Visual Studio and build the `x64`
or `ARM64` configuration, or from a Developer Command Prompt:

```
nuget restore WindowsRuntimeAttestationReport.sln
msbuild WindowsRuntimeAttestationReport.sln /p:Configuration=Release /p:Platform=x64
msbuild WindowsRuntimeAttestationReport.sln /p:Configuration=Release /p:Platform=ARM64
```

See [BUILDING.md](BUILDING.md) for a from-scratch toolchain setup.

## Running

```
WindowsRuntimeAttestationReport.exe [--type driver|hotpatch|all]
```

`--type` selects which report types to request and decode. It defaults to
`driver` when omitted.

- `driver`   — the loaded-driver report (default).
- `hotpatch` — the kernel-mode hotpatch report.
- `all`      — both of the above. The SDK also defines a Code Integrity report
  type, which this sample does not request.

### Driver report

```
WindowsRuntimeAttestationReport.exe --type driver
```

```

========================================================================
  Runtime Report Package
========================================================================
  version   : 1
  reports   : 1
  size      : 12800 bytes
  digest    : SHA-512
  signature : RSA-PSS SHA-512

========================================================================
  Report 1 of 1  -  Driver report
========================================================================
  128 driver(s)

  [0] crashdmp.sys (loads=1)
      image hash      (SHA-256): 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
      cert thumbprint (SHA-1): 0123456789abcdef0123456789abcdef01234567

  ...

------------------------------------------------------------------------
  End of Report 1 of 1  -  Driver report
------------------------------------------------------------------------
```

Each driver's `image hash` is printed with the digest algorithm Code Integrity
used to authenticate that image, so a mix of SHA-256 and (legacy) SHA-1 hashes
across drivers is expected. The certificate thumbprint is a SHA-1 hash by
definition.

### Hotpatch report

```
WindowsRuntimeAttestationReport.exe --type hotpatch
```

```

========================================================================
  Runtime Report Package
========================================================================
  version   : 1
  reports   : 1
  size      : 512 bytes
  digest    : SHA-512
  signature : RSA-PSS SHA-512

========================================================================
  Report 1 of 1  -  Hotpatch report
========================================================================
  1 patched image(s)

  [0] ntoskrnl.exe
      base address  : 0xfffff80000000000
      image size    : 16777216 bytes
      checksum      : 0x01234567
      timedatestamp : 0x89abcdef
      patch sequence: 1

------------------------------------------------------------------------
  End of Report 1 of 1  -  Hotpatch report
------------------------------------------------------------------------
```

The hotpatch report is *active-only*: an image appears only while a hotpatch is
applied to it, and its entry is removed when the image is unloaded. On a system
with no active hotpatches the report lists 0 images.

## Further reading

A series of posts at [akashtrehan.com](https://www.akashtrehan.com) explains the
concepts behind this sample.

- [Patch or attack? Inside the Windows Secure Hotpatch Report](https://www.akashtrehan.com/secure-hotpatch-report/)
  — what a secure hotpatch looks like in memory, why it is indistinguishable
  from an inline hook at the byte level, and how the signed report settles it.

More on the trust model, the package format, the driver report, and verifying
the signature are on the way.

## License

MIT — see [LICENSE](LICENSE).
