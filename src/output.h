// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: The Xbox Object Directory List authors

#pragma once

#include <xboxkrnl/xboxkrnl.h>

void print(char* str, ...);

void open_output_file(char*);
BOOL close_output_file();
