#include "HotpatchReport.h"
#include "Helpers.h"

#include <cstdio>

void
PrintHotpatchReport(const BYTE* report, UINT32 reportSize)
{
    if (reportSize < FIELD_OFFSET(HOTPATCH_RUNTIME_REPORT, Entries)) {
        std::printf("  (hotpatch report too small)\n");
        return;
    }

    const HOTPATCH_RUNTIME_REPORT UNALIGNED* hotpatchReport =
        Overlay<HOTPATCH_RUNTIME_REPORT>(report, 0);

    static_assert(sizeof(hotpatchReport->NumberOfEntries) == sizeof(UINT16),
                  "loop counter width assumes a 16-bit entry count");

    std::printf("  %u patched image(s)\n", hotpatchReport->NumberOfEntries);

    for (UINT16 i = 0; i < hotpatchReport->NumberOfEntries; i += 1) {
        size_t offset = FIELD_OFFSET(HOTPATCH_RUNTIME_REPORT, Entries) +
                        static_cast<size_t>(i) * sizeof(HOTPATCH_INFO_ENTRY);

        if (!InRange(offset, sizeof(HOTPATCH_INFO_ENTRY), reportSize)) {
            std::printf("  (entry %u out of range)\n", i);
            return;
        }

        const HOTPATCH_INFO_ENTRY UNALIGNED* entry =
            Overlay<HOTPATCH_INFO_ENTRY>(report, offset);

        std::printf("\n  [%u] %.*s\n",
                    i, HOTPATCH_REPORT_NAME_MAX_LENGTH, entry->BaseImageName);
        std::printf("      %-14s: 0x%016llx\n", "base address",
                    static_cast<unsigned long long>(entry->BaseAddress));
        std::printf("      %-14s: %u bytes\n", "image size", entry->ImageSize);
        std::printf("      %-14s: 0x%08x\n", "checksum", entry->BaseCheckSum);
        std::printf("      %-14s: 0x%08x\n", "timedatestamp",
                    entry->BaseTimeDateStamp);
        std::printf("      %-14s: %u\n", "patch sequence",
                    entry->LatestSequenceNumber);
    }
}
