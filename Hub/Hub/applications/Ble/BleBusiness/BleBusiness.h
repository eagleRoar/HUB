/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-24     Administrator       the first version
 */
#ifndef APPLICATIONS_BLE_BLEBUSINESS_BLEBUSINESS_H_
#define APPLICATIONS_BLE_BLEBUSINESS_BLEBUSINESS_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "Gpio.h"

#include "InformationMonitor.h"

#define         AT_ENTER            "+++a"
#define         AT_ENTER_RET        "a+ok"
#define         TEST_TO_BLE         "AT+HELLO?"
#define         SET_NAME            "AT+NAME=Hub_Justin"
#define         ASK_NAME            "AT+NAME?"
#define         ENTER_TO_BUF        "AT+ENTM"
#define         ENTER_BUF_OK        "+ENTM:OK"
#define         ASK_MAX_PUT         "AT+MAXPUT?"
#define         SET_MAX_PUT         "AT+MAXPUT=OFF"
#define         SET_MAX_PUT_OK      "+MAXPUT:OFF"

enum{

    MODE_AT = 0x01,                 //AT指令模式
    MODE_BUFFER,                    //透传模式
};

struct bleState{

    u8 connect;
    u8 mode;
    u8 max_buf_enable;              //是否使能最大输出
    u8 init;
};

void ConnectToBle(rt_device_t);
void SetBleMode(rt_device_t);
rt_err_t AnalyzePack(type_blepack_t *);
void SetMaxput(rt_device_t);
void TransmitPack(type_blepack_t *);

#endif /* APPLICATIONS_BLE_BLEBUSINESS_BLEBUSINESS_H_ */
