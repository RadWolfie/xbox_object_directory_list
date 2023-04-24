// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: The Xbox Object Directory List authors

#include <pbkit/pbkit.h>
#include <hal/debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>
#include "output.h"

static HANDLE hOutputLog = INVALID_HANDLE_VALUE;

void open_output_file(char* file_path)
{
	debugPrint("Creating file %s\n", file_path);
	hOutputLog = CreateFile(
		file_path,
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hOutputLog == INVALID_HANDLE_VALUE) {
		debugPrint("ERROR: Could not create file %s\n", file_path);
	}
}

int write_to_output_file(char* buffer_str, DWORD buffer_len)
{
	DWORD bytes_written;
	BOOL ret = WriteFile(
		hOutputLog,
		buffer_str,
		buffer_len,
		&bytes_written,
		NULL);
	if (!ret) {
		debugPrint("ERROR: Could not write to output file\n");
	}
	if (bytes_written != buffer_len) {
		debugPrint("ERROR: Bytes written = %lu, bytes expected to write = %lu\n",
		           bytes_written, buffer_len);
		ret = 1;
	}
	return ret;
}

BOOL close_output_file()
{
	BOOL ret = CloseHandle(hOutputLog);
	if (!ret) {
		debugPrint("ERROR: Could not close output file\n");
	}
	return ret;
}


void print(char* str, ...)
{
	va_list args;
	char buffer[500];
	va_start(args, str);
	vsnprintf(buffer, 499, str, args);
	va_end(args);
	// Print on screen
	debugPrint("%s\n", buffer);

	// Print on attached debugger window
	DbgPrint("%s", buffer);

	// Write message to log file
	strncat(buffer, "\n", 500);
	write_to_output_file(
		buffer,
		strlen(buffer));
}
