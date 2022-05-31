/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_
#define APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_

#include "Gpio.h"

void InsertModuleToTable(type_monitor_t *, type_module_t , u8);
u8 FindModule(type_monitor_t *, type_module_t , u8*);
u8 FindModuleByAddr(type_monitor_t *, u8);
void initModuleConState(type_monitor_t *);

#endif /* APPLICATIONS_INFORMATIONMANAGE_MODULE_MODULE_H_ */
