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
#include "Uart.h"
#include "Oled1309.h"

static u8 alarm_flag = NO;

u8 getAlarmFlag(void)
{
    return alarm_flag;
}

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
#if(HUB_SELECT == HUB_ENVIRENMENT)
    rt_pin_mode(LED_LINE, PIN_MODE_OUTPUT);
#endif
    rt_pin_mode(LED_ALAEM, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_HEART, PIN_MODE_OUTPUT);
    rt_pin_mode(ALARM_OUT, PIN_MODE_OUTPUT);
//    rt_pin_mode(LED_POWER, PIN_MODE_OUTPUT);

    /* oled 1309屏设置引脚*/
    rt_pin_mode(LCD_DB0, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB1, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DB2, PIN_MODE_OUTPUT);
    rt_pin_write(LCD_DB2, 1);
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
    rt_pin_mode(LCD_RST, PIN_MODE_OUTPUT);

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
    rt_pin_mode(BUTTON_DOWN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(BUTTON_ENTER, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(BUTTON_UP, PIN_MODE_INPUT_PULLUP);
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
    static      u8              ledState            = 0;
    static      u8              Timer500msTouch     = OFF;
    static      u8              Timer1sTouch        = OFF;
    static      u16             time500ms           = 0;
    static      u16             time1s              = 0;
    while(1)
    {
        time500ms = TimerTask(&time500ms, 500/100, &Timer500msTouch);                       //1s定时任务
        time1s = TimerTask(&time1s, 1000/100, &Timer1sTouch);                       //1s定时任务

        //100ms
        {
            LedProgram();
        }

        //500ms 定时器
        if(ON == Timer500msTouch)
        {
            if(NO == getFactoryMode())
            {
#if(HUB_SELECT == HUB_ENVIRENMENT)
                AlarmLedProgram();
#endif
            }
        }

        //1s 定时器
        if(ON == Timer1sTouch)
        {
            //呼吸灯
            Ctrl_LED(LED_HEART,ledState++ % 2);
        }

        rt_thread_mdelay(100);
    }
}
#if(HUB_SELECT == HUB_ENVIRENMENT)
//报警逻辑: 如果是有标志报警的话就一直报 否则关闭
void AlarmLedProgram(void)
{
    if((ON == GetSysSet()->sysWarn.dayCo2Buzz && DAY_TIME == GetSysSet()->dayOrNight) ||
       (ON == GetSysSet()->sysWarn.nightCo2Buzz && NIGHT_TIME == GetSysSet()->dayOrNight))
    {
        if((ON == GetSysSet()->warn[WARN_CO2_LOW - 1]) ||
           (ON == GetSysSet()->warn[WARN_CO2_HIGHT - 1]))
        {
            rt_pin_write(ALARM_OUT, ON);
        }
        else
        {
            rt_pin_write(ALARM_OUT, OFF);
        }
    }
    else
    {
        rt_pin_write(ALARM_OUT, OFF);
    }
}
#endif

void LedProgram(void)
{
    type_monitor_t  *monitor        = RT_NULL;
    static u8       sensor_s        = 0;
    static u8       device_s        = 0;
    static u8       sensor_cnt      = 0;
    static u8       device_cnt      = 0;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    static u8       line_s          = 0;
    static u8       line_cnt        = 0;
#endif

    monitor = GetMonitor();

    if(sensor_s != monitor->sensor_size)
    {
        //如果删除或者增加sensor 之后就会闪烁一下
        if(sensor_cnt < 2)
        {
            sensor_cnt++;
            Ctrl_LED(LED_SENSOR, 0 == rt_pin_read(LED_SENSOR) ? 1 : 0);
        }
        else
        {
            sensor_cnt = 0;
            sensor_s = monitor->sensor_size;
        }
    }
    else
    {
        if(monitor->sensor_size > 0)
        {
            //1.如果有sensor 的话就亮灯
            Ctrl_LED(LED_SENSOR,0);
        }
        else
        {
            //2.如果没有sensor 的时候就灭灯
            Ctrl_LED(LED_SENSOR,1);
        }
    }

    if(device_s != monitor->device_size)
    {
        //如果删除或者增加device 之后就会闪烁一下
        if(device_cnt < 2)
        {
            device_cnt++;
            Ctrl_LED(LED_DEVICE, 0 == rt_pin_read(LED_DEVICE) ? 1 : 0);
        }
        else
        {
            device_cnt = 0;
            device_s = monitor->device_size;
        }
    }
    else
    {
        if(monitor->device_size > 0)
        {
            //1.如果有device 的话就亮灯
            Ctrl_LED(LED_DEVICE,0);
        }
        else
        {
            //2.如果没有device 的时候就灭灯
            Ctrl_LED(LED_DEVICE,1);
        }
    }

#if(HUB_SELECT == HUB_ENVIRENMENT)
    if(line_s != monitor->line_size)
    {
        //如果删除或者增加line 之后就会闪烁一下
        if(line_cnt < 2)
        {
            line_cnt++;
            Ctrl_LED(LED_LINE, 0 == rt_pin_read(LED_LINE) ? 1 : 0);
        }
        else
        {
            line_cnt = 0;
            line_s = monitor->line_size;
        }
    }
    else
    {
        if(monitor->line_size > 0)
        {
            //1.如果有line 的话就亮灯
            Ctrl_LED(LED_LINE,0);
        }
        else
        {
            //2.如果没有line 的时候就灭灯
            Ctrl_LED(LED_LINE,1);
        }
    }
#endif
}

