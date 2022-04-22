/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-02     Administrator       the first version
 */
#include "Sensor.h"

sensor_interface sensor;

void sensorRegisterInit(void)
{
    sensor.registerAnswer = SEND_NULL;
}
