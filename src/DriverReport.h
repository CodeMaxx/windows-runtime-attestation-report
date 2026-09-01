//
// DriverReport.h
//
// Decoder for the driver runtime report.
//

#pragma once

#include <windows.h>

// Prints a driver runtime report. 'report' points at the report header; every
// entry offset is relative to it. Returns false if the report is malformed.
bool
PrintDriverReport(const BYTE* report, UINT32 reportSize);
