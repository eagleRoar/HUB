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

#pragma pack(4)//因为cjson 不能使用1字节对齐

void initCloudProtocol(void);
char *GetSnName(char *);
void tempProgram(type_monitor_t *);
void analyzeCloudData(char *);
void ReplyDataToCloud(mqtt_client *);

#endif /* APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_ */
