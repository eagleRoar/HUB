/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-23     Administrator       the first version
 */

#ifndef APPLICATIONS_BUTTON_BUTTONTASK_H_
#define APPLICATIONS_BUTTON_BUTTONTASK_H_


#include "Gpio.h"


#define     BUTTON_MAX              3

#define     KEY_ON                  0

//定义按键持续时间
#define     BAND_TIME               40  //对于区别短按和长按的区间
#define     SHORT_TIME              60
#define     LONG_TIME               600

typedef     void (*Call_Back)(u8);

struct buttonInfo{
    u8          on_set;
    u8          type;
    u8          press_state;
    u8          have_read;
    u16         continue_time;
    rt_base_t   pin;
    Call_Back   call_back;
};

enum BUTTTON_TYPE{
    NULL_PRESS = 0x00,
    SHORT_PRESS,
    LONG_PRESS,
};

void ButtonTaskInit(void);

#endif /* APPLICATIONS_BUTTON_BUTTONTASK_H_ */
