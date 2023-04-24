// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: The Xbox Object Directory List authors

#include <pbkit/pbkit.h>
#include <hal/video.h>
#include <hal/xbox.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <fileapi.h>
#include "output.h"

// Custom configuration can be edit below
unsigned limit_obj_directory = 3; // Query list of directory objects
unsigned limit_fs_directory = 1;  // Query list of directory's file objects (if set to 0, it will use limit_obj_directory counter instead)
// Custom configuration can be edit above

typedef struct _OBJECT_DIRECTORY_INFORMATION_EXTEND {
	OBJECT_DIRECTORY_INFORMATION dir_info;
	char buffer_name[MAX_PATH];
} OBJECT_DIRECTORY_INFORMATION_EXTEND;
OBJECT_DIRECTORY_INFORMATION_EXTEND obj_dir_info;

typedef struct _FILE_DIRECTORY_INFORMATION_EXTEND {
	FILE_DIRECTORY_INFORMATION dir_info;
	OCHAR filename_extend[MAX_PATH - 1];
} FILE_DIRECTORY_INFORMATION_EXTEND;

void output_symlink_full_path(OBJECT_ATTRIBUTES ObjAttrs, unsigned indent_length)
{
	char full_path_buffer[MAX_PATH];
	STRING full_path_str;
	full_path_str.Buffer = full_path_buffer;
	full_path_str.MaximumLength = sizeof(full_path_buffer);

	HANDLE symlinkHandle;
	NTSTATUS result = NtOpenSymbolicLinkObject(&symlinkHandle, &ObjAttrs);

	if (NT_SUCCESS(result)) {
		// If no error, then obtain symbolic link's file path
		result = NtQuerySymbolicLinkObject(symlinkHandle, &full_path_str, NULL);
		if (NT_SUCCESS(result)) {
			// If successfully obtain target object, print it!
			full_path_str.Buffer[full_path_str.Length] = 0;
			print("%*sfull path = %.*s", indent_length, "", full_path_str.Length, full_path_str.Buffer);
		}
		else {
			print("%*sNtQuerySymbolicLinkObject error: %08X", indent_length, "", result);
		}
	}
	else {
		print("%*sNtOpenSymbolicLinkObject error: %08X", indent_length, "", result);
	}
	CloseHandle(symlinkHandle);
}

void validate_pooltag(ULONG PoolTag, UCHAR* cPoolTag)
{
	cPoolTag[0] = (PoolTag & 0xFF);
	cPoolTag[1] = ((PoolTag >> 8) & 0xFF);
	cPoolTag[2] = ((PoolTag >> 16) & 0xFF);
	cPoolTag[3] = ((PoolTag >> 24) & 0xFF);
	for (unsigned i = 0; i < 4; i++) {
		if (!(cPoolTag[i] >= 'a' && cPoolTag[i] <= 'z') && !(cPoolTag[i] >= 'A' && cPoolTag[i] <= 'Z')) {
			cPoolTag[i] = '?';
		}
	}
}

void get_generic_object_type(OBJECT_ATTRIBUTES ObjAttrs, UCHAR* cPoolTag)
{
	PVOID object;
	HANDLE hObject;
	NTSTATUS result = ObOpenObjectByName(&ObjAttrs, NULL, NULL, &hObject);
	if (!NT_SUCCESS(result)) {
		validate_pooltag(0, cPoolTag);
		return;
	}

	result = ObReferenceObjectByHandle(hObject, NULL, &object);
	if (!NT_SUCCESS(result)) {
		validate_pooltag(0, cPoolTag);
		CloseHandle(hObject);
		return;
	}

	POBJECT_HEADER ObjHeader = OBJECT_TO_OBJECT_HEADER(object);
	validate_pooltag(ObjHeader->Type->PoolTag, cPoolTag);

	ObfDereferenceObject(object);
	CloseHandle(hObject);
}

void output_file_directory_list(OBJECT_ATTRIBUTES ObjAttrs, unsigned limit_child, unsigned indent_length)
{
	if (!limit_child) {
		return;
	}
	limit_child--;
	UCHAR cPoolTag[4];
	HANDLE hFileDirectory;
	IO_STATUS_BLOCK ioStatus;
	NTSTATUS result = NtOpenFile(&hFileDirectory, FILE_LIST_DIRECTORY | SYNCHRONIZE, &ObjAttrs, &ioStatus, FILE_SHARE_READ, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (NT_SUCCESS(result)) {
		FILE_DIRECTORY_INFORMATION_EXTEND fileInformation;
		result = NtQueryDirectoryFile(hFileDirectory, NULL, NULL, NULL, &ioStatus, &fileInformation, sizeof(fileInformation), FileDirectoryInformation, NULL, TRUE);
		while (NT_SUCCESS(result)) {
			if (NT_SUCCESS(result)) {
				ObjAttrs.RootDirectory = hFileDirectory;
				ObjAttrs.ObjectName->Buffer = fileInformation.dir_info.FileName;
				ObjAttrs.ObjectName->Length = fileInformation.dir_info.FileNameLength;
#ifdef DISABLED // Always ???? for PoolTag
				get_generic_object_type(ObjAttrs, cPoolTag);
				print("%*s\\%.*s - Type = %c%c%c%c", indent_length, "", fileInformation.dir_info.FileNameLength, fileInformation.dir_info.FileName,
				      cPoolTag[0], cPoolTag[1], cPoolTag[2], cPoolTag[3]);
#else
				print("%*s\\%.*s", indent_length, "", fileInformation.dir_info.FileNameLength, fileInformation.dir_info.FileName);
#endif
				output_file_directory_list(ObjAttrs, limit_child, indent_length + fileInformation.dir_info.FileNameLength + 1);
			}
			result = NtQueryDirectoryFile(hFileDirectory, NULL, NULL, NULL, &ioStatus, &fileInformation, sizeof(fileInformation), FileDirectoryInformation, NULL, FALSE);
		}
		CloseHandle(hFileDirectory);
	}
}

void output_disk_partition_list(OBJECT_ATTRIBUTES ObjAttrs, unsigned limit_child, unsigned indent_length)
{
	if (!limit_child) {
		return;
	}
	limit_child--;
	indent_length += ObjAttrs.ObjectName->Length + 1;
	USHORT orgLength = ObjAttrs.ObjectName->Length;
	ObjAttrs.ObjectName->Buffer[orgLength] = 0;

	STRING disk_name = *ObjAttrs.ObjectName;
	char path_name[MAX_PATH];
	UCHAR cPoolTag[4];

	// I think the actual limit of partitions xbox kernel support is 28... 20 for hard drive and another 8 for memory units
	// Hmm... doesn't make sense according to hardware dump but okay.
	// TODO: Find a replacement for this loop if possible. Perhaps partition table from Partition0?
	for (unsigned i = 0; i < 28; i++) {
		HANDLE hPartition;
		IO_STATUS_BLOCK ioStatus;
		ObjAttrs.ObjectName->Buffer = path_name;
		snprintf(ObjAttrs.ObjectName->Buffer, disk_name.MaximumLength, "%s\\Partition%u", disk_name.Buffer, i);
		ObjAttrs.ObjectName->Length = strlen(ObjAttrs.ObjectName->Buffer);
		ObjAttrs.ObjectName->MaximumLength = disk_name.MaximumLength;
		NTSTATUS result = NtOpenFile(&hPartition, GENERIC_READ | SYNCHRONIZE, &ObjAttrs, &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NO_INTERMEDIATE_BUFFERING);
		if (NT_SUCCESS(result)) {
			CloseHandle(hPartition);
			get_generic_object_type(ObjAttrs, cPoolTag);
			print("%*s%.*s - Type = %c%c%c%c", indent_length, "", ObjAttrs.ObjectName->Length - orgLength, &ObjAttrs.ObjectName->Buffer[orgLength],
			      cPoolTag[0], cPoolTag[1], cPoolTag[2], cPoolTag[3]);
			strncat(ObjAttrs.ObjectName->Buffer, "\\", ObjAttrs.ObjectName->MaximumLength);
			ObjAttrs.ObjectName->Length++;
			output_file_directory_list(ObjAttrs, (limit_fs_directory ? limit_fs_directory : limit_child), indent_length + (ObjAttrs.ObjectName->Length - orgLength - 1));
		}
	}
	*ObjAttrs.ObjectName = disk_name;
}

void query_path_dir_list(OBJECT_ATTRIBUTES ObjAttrs, unsigned limit_child, unsigned indent_length)
{
	if (!limit_child) {
		return;
	}
	limit_child--;
	HANDLE hDirectory;
	NTSTATUS result = NtOpenDirectoryObject(&hDirectory, &ObjAttrs);
	if (NT_SUCCESS(result)) {
		indent_length += ObjAttrs.ObjectName->Length + 1;
		ULONG Context = 0;
		// Store current directory path handle to pass down into loop call
		ObjAttrs.RootDirectory = hDirectory;
		do {
			result = NtQueryDirectoryObject(hDirectory, &obj_dir_info, sizeof(obj_dir_info), FALSE, &Context, NULL);
			if (NT_SUCCESS(result)) {
				obj_dir_info.dir_info.Name.MaximumLength = MAX_PATH;
				UCHAR cPoolTag[4];
				validate_pooltag(obj_dir_info.dir_info.Type, cPoolTag);
				print("%*s\\%.*s - Type = %c%c%c%c", indent_length, "",
				      obj_dir_info.dir_info.Name.Length, obj_dir_info.dir_info.Name.Buffer,
				      cPoolTag[0], cPoolTag[1], cPoolTag[2], cPoolTag[3]);
				*ObjAttrs.ObjectName = obj_dir_info.dir_info.Name;
				// If type is symbolic link, then just retrieve full path.
				if (obj_dir_info.dir_info.Type == 'bmyS') {
					output_symlink_full_path(ObjAttrs, indent_length + 1);
				}
				// If type is device, we can view filesystem directory
				else if (obj_dir_info.dir_info.Type == 'iveD') {
					strncat(ObjAttrs.ObjectName->Buffer, "\\", ObjAttrs.ObjectName->MaximumLength);
					ObjAttrs.ObjectName->Length++;
					output_file_directory_list(ObjAttrs, (limit_fs_directory ? limit_fs_directory : limit_child), indent_length + ObjAttrs.ObjectName->Length);
				}
				// If type is disk, we have to make a guess which partitions are mounted.
				else if (obj_dir_info.dir_info.Type == 'ksiD') {
					output_disk_partition_list(ObjAttrs, limit_child, indent_length);
				}
				// Otherwise, try get child directory if any exists.
				// This method only work for directory object.
				else if (obj_dir_info.dir_info.Type == 'eriD') {
					query_path_dir_list(ObjAttrs, limit_child, indent_length);
				}
			}
		} while (NT_SUCCESS(result));

		if (result != STATUS_NO_MORE_ENTRIES) {
			print("%*sNtQueryDirectoryObject return %08X", indent_length + 1, "", result);
		}
		CloseHandle(hDirectory);
	}
	else {
		print("%*sERROR: %08X | Unable to open \"%.*s\"", indent_length + 1, "", result, ObjAttrs.ObjectName->Length, ObjAttrs.ObjectName->Buffer);
	}
}

void main(void)
{
	XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

	switch (pb_init()) {
		case 0:
			break;
		default:
			Sleep(2000);
			XReboot();
			return;
	}

	pb_show_debug_screen();

	open_output_file("D:\\object_directory_list.log");

	// Tests to verify if they do show up in Win32NamedObjects' directory
	HANDLE mutex = CreateMutexA(NULL, FALSE, "TestMutexObjectName");
	HANDLE semaphore = CreateSemaphore(NULL, 1, 1, "TestSemaphoreObjectName");

	STRING path_name_str;
	path_name_str.MaximumLength = sizeof(char) * MAX_PATH;
	char path_name[MAX_PATH] = "\\";
	path_name_str.Buffer = path_name;
	path_name_str.Length = strlen(path_name);
	OBJECT_ATTRIBUTES ObjAttrs;
	InitializeObjectAttributes(&ObjAttrs, &path_name_str, OBJ_CASE_INSENSITIVE, NULL, NULL);

	query_path_dir_list(ObjAttrs, limit_obj_directory, 0);

	CloseHandle(mutex);
	CloseHandle(semaphore);

	close_output_file();

	Sleep(10000);
	pb_kill();
	XReboot();
}
