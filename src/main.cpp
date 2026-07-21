//
// WindowsRuntimeAttestationReport
//
// Sample that calls the GetRuntimeAttestationReport Win32 API and prints the
// returned signed runtime report package.
//
//   WindowsRuntimeAttestationReport [--type driver|hotpatch|all]
//
// Requesting the driver report requires HVCI to be enabled.
//

#include "Helpers.h"
#include "DriverReport.h"
#include "HotpatchReport.h"

#include <windows.h>
#include <bcrypt.h>
#include <vector>
#include <new>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "mincore.lib")   // GetRuntimeAttestationReport
#pragma comment(lib, "bcrypt.lib")    // BCryptGenRandom

// Selects which report type(s) to request. Defaults to the driver report.
// Returns false on a malformed argument.
static bool
ParseReportTypes(int argc, char** argv, UINT64* reportTypes)
{
    *reportTypes = RUNTIME_REPORT_TYPE_TO_MASK(RuntimeReportTypeDriver);

    for (int i = 1; i < argc; i += 1) {
        if (_stricmp(argv[i], "--type") != 0) {
            std::fprintf(stderr, "error: unknown argument '%s'\n", argv[i]);
            return false;
        }

        if (i + 1 >= argc) {
            std::fprintf(stderr, "error: --type requires driver|hotpatch|all\n");
            return false;
        }

        i += 1;
        if (_stricmp(argv[i], "driver") == 0) {
            *reportTypes = RUNTIME_REPORT_TYPE_TO_MASK(RuntimeReportTypeDriver);
        } else if (_stricmp(argv[i], "hotpatch") == 0) {
            *reportTypes = RUNTIME_REPORT_TYPE_TO_MASK(RUNTIME_REPORT_TYPE_HOTPATCH);
        } else if (_stricmp(argv[i], "all") == 0) {
            *reportTypes = RUNTIME_REPORT_TYPE_TO_MASK(RuntimeReportTypeDriver) |
                           RUNTIME_REPORT_TYPE_TO_MASK(RUNTIME_REPORT_TYPE_HOTPATCH);
        } else {
            std::fprintf(stderr, "error: unknown --type '%s'\n", argv[i]);
            return false;
        }
    }

    return true;
}

// Fetches the runtime report package for the given report-type mask using the
// two-step size-then-fetch pattern. Fills 'nonce' with fresh random bytes.
// Returns false and prints the error on failure.
static bool
GetReport(std::vector<BYTE>& package, UCHAR nonce[RUNTIME_REPORT_NONCE_SIZE],
          UINT64 reportTypes)
{
    NTSTATUS status = BCryptGenRandom(nullptr, nonce, RUNTIME_REPORT_NONCE_SIZE,
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!BCRYPT_SUCCESS(status)) {
        std::fprintf(stderr, "error: BCryptGenRandom failed (0x%08lX)\n",
                     static_cast<unsigned long>(status));
        return false;
    }

    UINT32 size = 0;
    if (!GetRuntimeAttestationReport(nullptr, RUNTIME_REPORT_PACKAGE_VERSION_CURRENT,
                                     reportTypes, nullptr, &size)) {
        DWORD error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER) {
            // ERROR_INVALID_PARAMETER can mean a genuinely invalid parameter or
            // that the requested report type is not supported on this version
            // of Windows.
            if (error == ERROR_INVALID_PARAMETER) {
                std::fprintf(stderr,
                             "error: one or more parameters passed is "
                             "invalid, or the requested report type is not "
                             "supported on this version of Windows\n");
            } else {
                std::fprintf(stderr, "error: size query failed (%lu)\n", error);
            }
            return false;
        }
    }

    // The required size can grow if drivers load or hotpatches are applied
    // between the size query and the fetch, so retry a bounded number of times.
    constexpr int kMaxFetchAttempts = 4;

    for (int attempt = 0; attempt < kMaxFetchAttempts; attempt += 1) {
        package.resize(size);
        if (GetRuntimeAttestationReport(nonce, RUNTIME_REPORT_PACKAGE_VERSION_CURRENT,
                                        reportTypes, package.data(), &size)) {
            package.resize(size);
            return true;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            std::fprintf(stderr, "error: GetRuntimeAttestationReport failed (%lu)\n",
                         GetLastError());
            return false;
        }
    }

    std::fprintf(stderr, "error: report size kept growing; giving up\n");
    return false;
}

// Human-readable name for a runtime report type.
static const char*
ReportTypeName(UINT16 reportType)
{
    switch (reportType) {
    case RuntimeReportTypeDriver:
        return "Driver report";
    case RUNTIME_REPORT_TYPE_HOTPATCH:
        return "Hotpatch report";
    default:
        return "Unknown report type";
    }
}

// Human-readable name for a runtime report signature scheme.
static const char*
SignatureSchemeName(UINT16 scheme)
{
    if (scheme == RUNTIME_REPORT_SIGNATURE_SCHEME_SHA512_RSA_PSS_SHA512) {
        return "RSA-PSS SHA-512";
    }
    return "unknown";
}

// Parses the report package envelope and dispatches each contained report to
// its decoder.
static void
PrintPackage(const std::vector<BYTE>& package)
{
    const BYTE* data = package.data();
    size_t total = package.size();

    if (total < sizeof(RUNTIME_REPORT_PACKAGE_HEADER)) {
        std::printf("package too small\n");
        return;
    }

    const RUNTIME_REPORT_PACKAGE_HEADER UNALIGNED* header =
        Overlay<RUNTIME_REPORT_PACKAGE_HEADER>(data, 0);

    if (header->Magic != RUNTIME_REPORT_PACKAGE_MAGIC) {
        std::printf("unexpected package magic 0x%08X\n", header->Magic);
        return;
    }

    if (header->PackageVersion != RUNTIME_REPORT_PACKAGE_VERSION_CURRENT) {
        std::printf("unsupported package version %u\n", header->PackageVersion);
        return;
    }

    const HashAlg* digestAlg = FindHashAlg(header->ReportDigestType);
    const char* digestName;

    if (digestAlg != nullptr) {
        digestName = digestAlg->name;
    } else {
        digestName = "unknown";
    }

    PrintBanner("Runtime Report Package");
    std::printf("  version   : %u\n", header->PackageVersion);
    std::printf("  reports   : %u\n", header->NumberOfReports);
    std::printf("  size      : %u bytes\n", header->PackageSize);
    std::printf("  digest    : %s\n", digestName);
    std::printf("  signature : %s\n", SignatureSchemeName(header->SignatureScheme));

    size_t nonceOffset = sizeof(RUNTIME_REPORT_PACKAGE_HEADER);
    size_t digestsOffset = nonceOffset + RUNTIME_REPORT_NONCE_SIZE;
    size_t signatureOffset = digestsOffset + header->TotalReportDigestsSize;
    size_t reportsOffset = signatureOffset + header->SignatureSize;

    if (!InRange(reportsOffset, header->TotalAuthenticatedReportsSize, total)) {
        std::printf("package sections exceed the buffer\n");
        return;
    }
    size_t reportsEnd = reportsOffset + header->TotalAuthenticatedReportsSize;

    size_t cursor = reportsOffset;
    for (UINT16 i = 0; i < header->NumberOfReports; i += 1) {
        if (!InRange(cursor, sizeof(RUNTIME_REPORT_HEADER), reportsEnd)) {
            std::printf("truncated report header\n");
            return;
        }

        const RUNTIME_REPORT_HEADER UNALIGNED* reportHeader =
            Overlay<RUNTIME_REPORT_HEADER>(data, cursor);
        UINT32 reportSize = reportHeader->ReportSize;

        if (reportSize < sizeof(RUNTIME_REPORT_HEADER) ||
            !InRange(cursor, reportSize, reportsEnd)) {
            std::printf("report %u has an invalid size\n", i);
            return;
        }

        char label[96];
        std::snprintf(label, sizeof(label), "Report %u of %u  -  %s",
                      static_cast<unsigned>(i + 1),
                      static_cast<unsigned>(header->NumberOfReports),
                      ReportTypeName(reportHeader->ReportType));
        PrintBanner(label);

        switch (reportHeader->ReportType) {
        case RuntimeReportTypeDriver:
            PrintDriverReport(data + cursor, reportSize);
            break;

        case RUNTIME_REPORT_TYPE_HOTPATCH:
            PrintHotpatchReport(data + cursor, reportSize);
            break;

        default:
            std::printf("  no decoder for report type %u (%u bytes)\n",
                        reportHeader->ReportType, reportSize);
            break;
        }

        char footer[128];
        std::snprintf(footer, sizeof(footer), "End of %s", label);
        PrintFooter(footer);

        cursor += reportSize;
    }
}

int
main(int argc, char** argv)
{
    std::vector<BYTE> package;
    UCHAR nonce[RUNTIME_REPORT_NONCE_SIZE];
    UINT64 reportTypes;

    if (!ParseReportTypes(argc, argv, &reportTypes)) {
        return 1;
    }

    try {
        if (!GetReport(package, nonce, reportTypes)) {
            return 1;
        }

        PrintPackage(package);
    } catch (const std::bad_alloc&) {
        std::fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    return 0;
}
