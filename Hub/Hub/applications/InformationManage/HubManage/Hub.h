/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-01     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_HUBMANAGE_HUB_H_
#define APPLICATIONS_INFORMATIONMANAGE_HUBMANAGE_HUB_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "InformationMonitor.h"

/* 该结构体不能随便增加或者更改 */
struct hubRegister
{
    u16 type;
    u32 version;
    u32 config_id;
    u16 heart;
    char name[HUB_NAME_SIZE];
};

struct hubRegInterface
{
    void    (*init)(u16, u32, u32, u16, char*);
    void    (*setRegisterAnswer)(u8);
    u8      (*getRegisterAnswer)(void);

    type_hubreg_t hub_reg;
    u8 registerAnswer;
};

void hubRegisterInit(void);
hubreg_interface GetHubReg(void);

#endif /* APPLICATIONS_INFORMATIONMANAGE_HUBMANAGE_HUB_H_ */
