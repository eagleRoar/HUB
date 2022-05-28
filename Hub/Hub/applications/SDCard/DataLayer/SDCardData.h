/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_DATALAYER_SDCARDDATA_H_
#define APPLICATIONS_SDCARD_DATALAYER_SDCARDDATA_H_


#include "Gpio.h"

u32 GetFileLength(char*);
u8 WriteSdData(char* , void* , u32 , u32);
u8 ReadSdData(char* , void* , u32 , u32);

#endif /* APPLICATIONS_SDCARD_DATALAYER_SDCARDDATA_H_ */
