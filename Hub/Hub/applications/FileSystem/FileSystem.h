/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-09     Administrator       the first version
 */
#ifndef APPLICATIONS_FILESYSTEM_FILESYSTEM_H_
#define APPLICATIONS_FILESYSTEM_FILESYSTEM_H_

#include "Gpio.h"
#include "InformationMonitor.h"

rt_err_t CheckDirectory(char*);
rt_err_t CreateDirectory(char*);
void FileSystemInit(void);
u8 GetFileSystemState(void);
u8 WriteFileData(char* name, void* text, u32 offset, u32 l);
u8 ReadFileData(char* name, void* text, u32 offset, u32 l);
void DataExport(void);
void DataImport(void);
void setMqttUrlFileFlag(u8 flag);
void SaveConsole(char *data, size_t len);
void RestoreFactorySettings(void);
#endif /* APPLICATIONS_FILESYSTEM_FILESYSTEM_H_ */
