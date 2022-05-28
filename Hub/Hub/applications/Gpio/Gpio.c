/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */

#include "Gpio.h"


/**
 * @brief  : GPIO口初始化
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void GpioInit(void)
{
    /* led引脚设置*/
    rt_pin_mode(LED_SENSOR, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_DEVICE, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_LIGHT, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_COMMS, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_BLUETOOTH, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_POWER, PIN_MODE_OUTPUT);

    /* oled 1309屏设置引脚*/
    rt_pin_mode(LCD_DB0, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB1, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB2, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB3, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB4, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB5, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB6, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB7, PIN_MODE_OUTPUT);
    //rt_pin_mode(RST_CTR, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_WR, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_RD, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_CS, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DC, PIN_MODE_OUTPUT);

    //手动启动OLCD的PIN脚
    rt_pin_write(LCD_RD, PIN_HIGH);

    //SD卡检测脚
    rt_pin_mode(SD_CHK_PIN, PIN_MODE_INPUT);

    //SPI CS脚
    rt_pin_mode(SPI1_CS_PIN, PIN_MODE_OUTPUT);

    //BLE RESET脚
//    rt_pin_mode(BLE_NRST_PIN, PIN_MODE_OUTPUT);
//    rt_pin_write(BLE_NRST_PIN, PIN_HIGH);

    //Button 设置
    rt_pin_mode(BUTTON_MENU, PIN_MODE_INPUT);
    rt_pin_mode(BUTTON_ENTER, PIN_MODE_INPUT);
}

int sd_card_is_vaild(void)
{
    return (rt_pin_read(SD_CHK_PIN) == PIN_LOW) ? (1) : (0);
}


/**
 * @brief  : led 灯光控制
 * @para   : pin 控制的IO口
 * @para   : state 状态
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void Ctrl_LED(rt_base_t pin, rt_base_t state)
{
    rt_pin_write(pin, state);
}

/**
 * @brief  : led 灯光线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void LedTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建led 线程 */
    rt_thread_t thread = rt_thread_create("led task", LedTaskEntry, RT_NULL, 256, LED_PRIORITY, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("led task start failed");
        }
    } else {
        LOG_E("led task create failed");
    }
}

/**
 * @brief  : led 灯光线程入口函数，周期性闪烁效果
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void LedTaskEntry(void* parameter)
{
    static u8 ledState = 0;

    while(1)
    {
//        Ctrl_LED(LED_SENSOR,0);//Justin debug
//        Ctrl_LED(LED_DEVICE,0);
        Ctrl_LED(LED_LIGHT,ledState++ % 2);
//        Ctrl_LED(LED_COMMS,0);
//        Ctrl_LED(LED_BLUETOOTH,0);
//        Ctrl_LED(LED_POWER,0);

        rt_thread_mdelay(500);
    }
}

