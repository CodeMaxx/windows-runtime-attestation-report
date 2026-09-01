//
// HotpatchReport.h
//
// Decoder for the hotpatch runtime report.
//

#pragma once

#include <windows.h>

// Prints a hotpatch runtime report. 'report' points at the report header.
// Returns false if the report is malformed.
bool
PrintHotpatchReport(const BYTE* report, UINT32 reportSize);
