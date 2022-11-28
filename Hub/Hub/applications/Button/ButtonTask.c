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

__attribute__((section(".ccmbss"))) type_button_t buttonList[BUTTON_MAX];
__attribute__((section(".ccmbss"))) u8 button_task[1024];
__attribute__((section(".ccmbss"))) struct rt_thread button_thread;

static void Buttonprogram(u8 period)
{
#define     BTN_COOL_TIME       2
    u8                  index       = 0;
    u16                 short_max   = 0;
    static u8           btn_cooling[BUTTON_MAX] = {0,0,0};        //按键冷却时间避免长按的时候判断了长按和短按
    static u8           on_set_pre[BUTTON_MAX] = {0,0,0};

    if(on_set_pre[index] != buttonList[index].on_set)
    {
        on_set_pre[index] = buttonList[index].on_set;

        //如果按键弹起 按键冷却时间清零
        if(rt_pin_read(buttonList[index].pin) != buttonList[index].on_set)
        {
            btn_cooling[index] = 0;
        }
    }

    for(index = 0; index < BUTTON_MAX; index++)
    {
        if(rt_pin_read(buttonList[index].pin) == buttonList[index].on_set)
        {
            if(btn_cooling[index] >= BTN_COOL_TIME)
            {
                buttonList[index].continue_time += period;
                buttonList[index].press_state = ON;
            }
        }
        else
        {
            buttonList[index].press_state = OFF;
            if(btn_cooling[index] < BTN_COOL_TIME)
            {
                btn_cooling[index]++;
            }
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
    ButtonAdd(BUTTON_ENTER, KEY_ON, EnterBtnCallBack);
    ButtonAdd(BUTTON_UP, KEY_ON, UpBtnCallBack);
    ButtonAdd(BUTTON_DOWN, KEY_ON, DowmBtnCallBack);
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
    if(RT_EOK != rt_thread_init(&button_thread, BUTTON_TASK, ButtonTaskEntry, RT_NULL, button_task, sizeof(button_task), BUTTON_PRIORITY, 10))
    {
        LOG_E("uart thread fail");
    }
    else
    {
        rt_thread_startup(&button_thread);
    }
}
