/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-23     Administrator       the first version
 */
#include "ButtonTask.h"
#include "Gpio.h"
#include "Oled1309.h"

type_button_t buttonList[BUTTON_MAX];

static void Buttonprogram(u8 period)
{
    u8                  index       = 0;
    u16                 short_max   = 0;

    for(index = 0; index < BUTTON_MAX; index++)
    {
        if(rt_pin_read(buttonList[index].pin) == buttonList[index].on_set)
        {
            buttonList[index].continue_time += period;
            buttonList[index].press_state = ON;
        }
        else
        {
            buttonList[index].press_state = OFF;
        }

        //通过时间计算类型

        if(ON == buttonList[index].press_state)     //还在持续点击
        {
            if(buttonList[index].continue_time >= LONG_TIME)
            {
                if(NO == buttonList[index].have_read)
                {
                    buttonList[index].type = LONG_PRESS;
                }
                else
                {
                    //按键已读的话不再重复赋值
                    buttonList[index].type = NULL_PRESS;
                }
                buttonList[index].have_read = YES;
            }
        }
        else
        {
            short_max = LONG_TIME - BAND_TIME;
            if ((buttonList[index].continue_time >= SHORT_TIME) &&
                (buttonList[index].continue_time < short_max))
            {
                buttonList[index].type = SHORT_PRESS;
            }
            else
            {
                buttonList[index].type = NULL_PRESS;
            }

            buttonList[index].have_read = NO;
            buttonList[index].continue_time = 0;
        }

        buttonList[index].call_back(buttonList[index].type);
    }
}

static void ButtonAdd(rt_base_t pin, u8 on_set, Call_Back call_back)
{
    static u8      index        = 0;

    if(index < BUTTON_MAX)
    {
        buttonList[index].pin = pin;
        buttonList[index].on_set = on_set;
        buttonList[index].call_back = call_back;
        buttonList[index].press_state = OFF;
        buttonList[index].have_read = NO;
        index += 1;
    }
    else
    {
        LOG_E("add button fail");
    }
}

static void ButtonRegister(void)
{
    ButtonAdd(BUTTON_MENU, KEY_ON, MenuBtnCallBack);
    ButtonAdd(BUTTON_ENTER, KEY_ON, EnterBtnCallBack);
}

static void ButtonTaskEntry(void* parameter)
{
    //注册按键信息，告诉我，按键的pin，按键0/1认为是已经点击
    ButtonRegister();

    while(1)
    {

        Buttonprogram(BUTTON_TASK_PERIOD);

        rt_thread_mdelay(BUTTON_TASK_PERIOD);
    }
}

/**
 * @brief  : 传感器类串口线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void ButtonTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("button task", ButtonTaskEntry, RT_NULL, 1024, BUTTON_PRIORITY, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("button task start failed");
        }
    } else {
        LOG_E("button task create failed");
    }
}
