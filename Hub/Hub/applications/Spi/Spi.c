/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-05     Administrator       the first version
 */

#include "Spi.h"

#define W25Q_SPI_DEVICE_NAME     "spi1"

void rtcTest(void)
{

    rt_err_t ret = RT_EOK;
    time_t now;

    /* 设置日期 */
    ret = set_date(2022, 3, 5);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC date failed\n");
    }

    /* 设置时间 */
    ret = set_time(15, 15, 50);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC time failed\n");
    }

    /* 延时3秒 */
    rt_thread_mdelay(3000);

//    /* 获取时间 */
//    now = time(RT_NULL);
//    LOG_D("%s\n", ctime(&now));
}

void spiTest(void)
{
    struct rt_spi_device *spi_dev_w25q;
    rt_uint8_t w25x_read_id = 0x90;
    rt_uint8_t id[5] = {0};

    /* 查找 spi 设备获取设备句柄 */
    spi_dev_w25q = (struct rt_spi_device *)rt_device_find(W25Q_SPI_DEVICE_NAME);
    if (!spi_dev_w25q)
    {
        rt_kprintf("spi sample run failed! can't find %s device!\n", W25Q_SPI_DEVICE_NAME);
    }
    else
    {
//        rt_spi_take(spi_dev_w25q);//Justin debug

//        /* 方式1：使用 rt_spi_send_then_recv()发送命令读取ID */
//        rt_spi_send_then_recv(spi_dev_w25q, &w25x_read_id, 1, id, 5);
//        rt_kprintf("use rt_spi_send_then_recv() read w25q ID is:%x%x\n", id[3], id[4]);
//
//        /* 方式2：使用 rt_spi_transfer_message()发送命令读取ID */
//        struct rt_spi_message msg1, msg2;
//
//        msg1.send_buf   = &w25x_read_id;
//        msg1.recv_buf   = RT_NULL;
//        msg1.length     = 1;
//        msg1.cs_take    = 1;
//        msg1.cs_release = 0;
//        msg1.next       = &msg2;
//
//        msg2.send_buf   = RT_NULL;
//        msg2.recv_buf   = id;
//        msg2.length     = 5;
//        msg2.cs_take    = 0;
//        msg2.cs_release = 1;
//        msg2.next       = RT_NULL;
//
//        rt_spi_transfer_message(spi_dev_w25q, &msg1);
//        rt_kprintf("use rt_spi_transfer_message() read w25q ID is:%x%x\n", id[3], id[4]);

    }
}

/**
 * @brief  : spi flash线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.03.05
 */
void SpiTaskEntry(void* parameter)
{
    static u8 timeCnt = 0;
    time_t now;

//    spiTest();
    rtcTest();
    while(1)
    {
        if(timeCnt < 10)
        {
            timeCnt++;
        }
        else
        {
            timeCnt = 0;

            /* 获取时间 */
            now = time(RT_NULL);
            LOG_D("%s\n", ctime(&now));
        }

        rt_thread_mdelay(1000);
    }
}

/**
 * @brief  : spi flash线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.03.05
 */
void SpiTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建led 线程 */
    rt_thread_t thread = rt_thread_create("spi task", SpiTaskEntry, RT_NULL, 1024, 26, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("spi task start failed");
        }
    } else {
        LOG_E("spi task create failed");
    }
}
