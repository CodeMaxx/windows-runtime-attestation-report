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

The driver report structures come from the Windows SDK's `winnt.h`, and the API
is declared in `sysinfoapi.h`. The hotpatch report structures have not shipped
in the Windows SDK yet — that update is expected in the coming weeks — so until
then the sample defines them in
[`src/HotpatchReport.h`](src/HotpatchReport.h). Once they ship, you can remove
that header and use the SDK definitions instead.

> Scope: this sample **retrieves, parses, and prints** the report. It does not
> perform remote attestation or verify the package's RSA-PSS signature.

## Requirements

To **run**:

- Windows 11 **25H2** or later.
- **VBS** enabled (required to produce any signed report).
- **HVCI** enabled — required for the *driver* report. The *hotpatch* report is
  HVCI-independent.

To **build**:

- Visual Studio 2022 or the Build Tools for Visual Studio 2022, with the
  **Desktop development with C++** workload (MSVC v143). Builds for **x64** and
  **ARM64**.
- **Windows 11 SDK 10.0.26100.7705** or later.

## Project layout

```
src\
  main.cpp                Entry point: argument parsing, report acquisition,
                          package envelope walk, and dispatch to each decoder.
  Helpers.h/.cpp          Bounds checks, structure overlay, hash-algorithm
                          table, and hex/blob/banner printing.
  DriverReport.h/.cpp     Driver report decoder.
  HotpatchReport.h/.cpp   Hotpatch report decoder, including the hotpatch
                          report structure definitions used by the sample.
```

## Building

Open `WindowsRuntimeAttestationReport.sln` in Visual Studio and build the `x64`
or `ARM64` configuration, or from a Developer Command Prompt:

```
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
- `all`      — both of the above.

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

A series of blog posts explaining the concepts behind this sample is coming over
the next few weeks at [akashtrehan.com](https://akashtrehan.com).

## License

MIT — see [LICENSE](LICENSE).
