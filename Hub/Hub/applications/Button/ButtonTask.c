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
#include "button.h"

#define KEY_ON                                   0

static rt_mutex_t buttonMutex = RT_NULL;    //指向互斥量的指针

Button_t Button1;
Button_t Button2;

rt_uint8_t ButtonMenuSet(void)
{
    return rt_pin_read(BUTTON_MENU);
}

rt_uint8_t ButtonEnterSet(void)
{
    return rt_pin_read(BUTTON_ENTER);
}

/**
 * menu 按键回调函数
 * @param btn
 */
void BtnMenuShortCallBack(void *btn)
{

}

/**
 * enter 按键回调函数
 * @param btn
 */
void BtnEnterShortCallBack(void *btn)
{

}

/**
  ******************************************************************
  * @brief   main
  * @author  jiejie
  * @version V1.0
  * @date    2018-xx-xx
  ******************************************************************
  */
void ButtonTaskEntry(void* parameter)
{
  /* 注册按键 */
  Button_Create("Button1",
              &Button1,
              ButtonMenuSet,
              KEY_ON);
  Button_Attach(&Button1,BUTTON_DOWM,BtnMenuShortCallBack);                       //Click

  Button_Create("Button2",
              &Button2,
              ButtonEnterSet,
              KEY_ON);
  Button_Attach(&Button2,BUTTON_DOWM,BtnEnterShortCallBack);                     //Click

  /* 设置按键相关设置 */
  Get_Button_Event(&Button1);
  Get_Button_Event(&Button2);

  while(1)
  {

    Button_Process();     //Need to call the button handler function periodically

    rt_thread_mdelay(20);
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

    /* 创建一个动态互斥锁 */
    buttonMutex = rt_mutex_create("buttonMutex", RT_IPC_FLAG_FIFO);
    if (buttonMutex == RT_NULL)
    {
        LOG_E("create dynamic mutex failed.\n");
    }

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

