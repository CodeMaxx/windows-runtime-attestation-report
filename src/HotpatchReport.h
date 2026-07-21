//
// HotpatchReport.h
//
// Decoder for the hotpatch runtime report.
//

#pragma once

#include <windows.h>

// Hotpatch report type value passed to GetRuntimeAttestationReport.
#define RUNTIME_REPORT_TYPE_HOTPATCH 2

#define HOTPATCH_REPORT_NAME_MAX_LENGTH 32

typedef struct _HOTPATCH_INFO_ENTRY {
    UINT32 BaseCheckSum;
    UINT32 BaseTimeDateStamp;
    UINT64 BaseAddress;
    UINT32 ImageSize;
    UINT32 LatestSequenceNumber;
    CHAR BaseImageName[HOTPATCH_REPORT_NAME_MAX_LENGTH];
} HOTPATCH_INFO_ENTRY;

typedef struct _HOTPATCH_RUNTIME_REPORT {
    RUNTIME_REPORT_HEADER Header;
    UINT16 NumberOfEntries;
    UINT16 Reserved1;
    UINT32 Reserved2;
    HOTPATCH_INFO_ENTRY Entries[ANYSIZE_ARRAY];
} HOTPATCH_RUNTIME_REPORT;

// Prints a hotpatch runtime report. 'report' points at the report header.
void
PrintHotpatchReport(const BYTE* report, UINT32 reportSize);
