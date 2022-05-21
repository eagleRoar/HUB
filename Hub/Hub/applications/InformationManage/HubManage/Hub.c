/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-01     Administrator       the first version
 */
#include "Hub.h"

//hubreg_interface *hub_reg_int = RT_NULL;
hubreg_interface hub_reg_int;

static void init(u16 type, u32 version, /*u32 config_id,*/ u16 heart, char *name)
{
    hub_reg_int.hub_reg.type = type;
    hub_reg_int.hub_reg.version = version;
    //hub_reg_int.hub_reg.config_id = config_id;
    hub_reg_int.hub_reg.heart = heart;
    rt_memcpy(hub_reg_int.hub_reg.name, name, HUB_NAME_SIZE);
}

static void setRegisterAnswer(u8 state)
{
    hub_reg_int.registerAnswer = state;
}

static u8 getRegisterAnswer(void)
{
    return hub_reg_int.registerAnswer;
}

void hubRegisterInit(void)
{
//    if(RT_NULL == hub_reg_int)
    {
//        hub_reg_int = rt_malloc(sizeof(hubreg_interface));
        rt_memset(&hub_reg_int, OFF, sizeof(hubreg_interface));

        hub_reg_int.init = init;
        hub_reg_int.setRegisterAnswer = setRegisterAnswer;
        hub_reg_int.getRegisterAnswer = getRegisterAnswer;

        hub_reg_int.setRegisterAnswer(SEND_NULL);
    }
}

hubreg_interface GetHubReg(void)
{
    return hub_reg_int;
}
