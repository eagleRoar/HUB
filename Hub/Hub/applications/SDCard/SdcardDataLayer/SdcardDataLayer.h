/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-27     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_SDCARDDATALAYER_SDCARDDATALAYER_H_
#define APPLICATIONS_SDCARD_SDCARDDATALAYER_SDCARDDATALAYER_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "SDCard.h"
#include "SdcardBusiness.h"
#include <dfs_posix.h>

int sd_card_is_vaild(void);
void SdDirInit(void);
u8 CheckDirectory(char* name);
u8 CheckFileAndMark(char*);
u32 GetFileLength(char* name);
u8 ReadSdData(char* name, /*u8*/void* text, u32 offset,u32 l);
u8 WriteSdData(char* name, void* text, u32 offset, u32 l);


#endif /* APPLICATIONS_SDCARD_SDCARDDATALAYER_SDCARDDATALAYER_H_ */
