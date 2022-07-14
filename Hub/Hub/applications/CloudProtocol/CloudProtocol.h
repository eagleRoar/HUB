/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-08     Administrator       the first version
 */
#ifndef APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_
#define APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_

#include "Gpio.h"
#include "mqtt_client.h"
#include "CloudProtocolBusiness.h"

#pragma pack(4)//因为cjson 不能使用1字节对齐

//sys_set_t *GetSysSet(void);//为什么识别不了sys_set_t
void initCloudProtocol(void);
char *GetSnName(char *);
void tempProgram(type_monitor_t *);
void timmerProgram(type_monitor_t *);
void co2Program(type_monitor_t *);
void humiProgram(type_monitor_t *);
void analyzeCloudData(char *);
void ReplyDataToCloud(mqtt_client *);
hub_t *GetHub(void);

#endif /* APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_ */
