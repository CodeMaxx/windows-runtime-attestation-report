#include "DriverReport.h"
#include "Helpers.h"

#include <cstdio>

bool
PrintDriverReport(const BYTE* report, UINT32 reportSize)
{
    if (reportSize < FIELD_OFFSET(DRIVER_RUNTIME_REPORT, DriverEntries)) {
        std::fprintf(stderr, "error: driver report too small\n");
        return false;
    }

    const DRIVER_RUNTIME_REPORT UNALIGNED* driverReport =
        Overlay<DRIVER_RUNTIME_REPORT>(report, 0);

    static_assert(sizeof(driverReport->NumberOfDrivers) == sizeof(UINT16),
                  "loop counter width assumes a 16-bit driver count");

    std::printf("  %u driver(s)\n", driverReport->NumberOfDrivers);

    if (driverReport->Flags.ReportOverflowed) {
        std::printf("  note: report truncated (driver limit reached)\n");
    }

    if (driverReport->Flags.IncludeBootDrivers) {
        std::printf("  note: includes boot-loaded drivers\n");
    }

    for (UINT16 i = 0; i < driverReport->NumberOfDrivers; i += 1) {
        size_t offset = FIELD_OFFSET(DRIVER_RUNTIME_REPORT, DriverEntries) +
                        static_cast<size_t>(i) * sizeof(DRIVER_INFO_ENTRY);

        if (!InRange(offset, sizeof(DRIVER_INFO_ENTRY), reportSize)) {
            std::fprintf(stderr, "error: entry %u out of range\n", i);
            return false;
        }

        const DRIVER_INFO_ENTRY UNALIGNED* entry =
            Overlay<DRIVER_INFO_ENTRY>(report, offset);

        std::printf("\n  [%u] %.*s (loads=%u",
                    i, DRIVER_REPORT_NAME_MAX_LENGTH, entry->InternalName,
                    entry->LoadCount);

        if (entry->Flags.Unloaded) {
            std::printf(" unloaded");
        }

        if (entry->Flags.BootDriver) {
            std::printf(" boot");
        }

        if (entry->Flags.HotPatch) {
            std::printf(" hotpatch");
        }

        std::printf(")\n");
        PrintBlob("image hash", report, reportSize,
                  entry->ImageHashAlgorithm, entry->ImageHashOffset);
        PrintBlob("cert thumbprint", report, reportSize,
                  entry->PublisherThumbprintHashAlgorithm,
                  entry->PublisherThumbprintOffset);

        if (entry->OemNameSize != 0 &&
            InRange(entry->OemNameOffset, entry->OemNameSize, reportSize)) {
            std::printf("      OEM name        : %.*s\n", entry->OemNameSize,
                        reinterpret_cast<const char*>(report + entry->OemNameOffset));
        }
    }

    return true;
}
