/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_BUSINESSLAYER_SDCARDBUSINESS_H_
#define APPLICATIONS_SDCARD_BUSINESSLAYER_SDCARDBUDDNESS_H_

#include "Gpio.h"
#include "SDCardData.h"
#include "Recipe.h"

#pragma pack(4)//因为cjson 不能使用1字节对齐

rt_err_t TackSysSetFromSD(sys_set_t *);
rt_err_t SaveSysSet(sys_set_t *);
#endif /* APPLICATIONS_SDCARD_BUSINESSLAYER_SDCARDBUSINESS_H_ */
