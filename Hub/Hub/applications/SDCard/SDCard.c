/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */

#include "SDCard.h"

#define SD_DEVICE_NAME    "sd0"

rt_uint8_t *write_buff, *read_buff;                  //Justin debug
extern struct rt_messagequeue uartSendMsg;           //串口发送数据消息队列

void fill_buffer1(rt_uint8_t *buff, rt_uint32_t buff_length)
{
    rt_uint32_t index;
    /* 往缓冲区填充随机数 */
    for (index = 0; index < buff_length; index++)
    {
        buff[index] = ((rt_uint8_t)rand()) & 0xff;
    }
}

static int sd_sample1(void)
{
    rt_err_t ret;
    rt_device_t sd_device;
    char sd_name[RT_NAME_MAX];
    //rt_uint8_t *write_buff, *read_buff;
    struct rt_device_blk_geometry geo;
    rt_uint8_t block_num;
    /* 判断命令行参数是否给定了设备名称 */
//    if (argc == 2)
//    {
//        rt_strncpy(sd_name, argv[1], RT_NAME_MAX);
//    }
//    else
//    {
//        rt_strncpy(sd_name, SD_DEVICE_NAME, RT_NAME_MAX);
//    }
    /* 查找设备获取设备句柄 */
    sd_device = rt_device_find(/*sd_name*/SD_DEVICE_NAME);
    if (sd_device == RT_NULL)
    {
        rt_kprintf("find device %s failed!\n", sd_name);
        return RT_ERROR;
    }
    /* 打开设备 */
    ret = rt_device_open(sd_device, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open device %s failed!\n", sd_name);
        return ret;
    }

    rt_memset(&geo, 0, sizeof(geo));
    /* 获取块设备信息 */
    ret = rt_device_control(sd_device, RT_DEVICE_CTRL_BLK_GETGEOME, &geo);
    if (ret != RT_EOK)
    {
        rt_kprintf("control device %s failed!\n", sd_name);
        return ret;
    }
    rt_kprintf("device information:\n");
    rt_kprintf("sector  size : %d byte\n", geo.bytes_per_sector);
    rt_kprintf("sector count : %d \n", geo.sector_count);
    rt_kprintf("block   size : %d byte\n", geo.block_size);
    /* 准备读写缓冲区空间，大小为一个块 */
    read_buff = rt_malloc(geo.block_size);
    if (read_buff == RT_NULL)
    {
        rt_kprintf("no memory for read buffer!\n");
        return RT_ERROR;
    }
    write_buff = rt_malloc(geo.block_size);
    if (write_buff == RT_NULL)
    {
        rt_kprintf("no memory for write buffer!\n");
        rt_free(read_buff);
        return RT_ERROR;
    }

    /* 填充写数据缓冲区，为写操作做准备 */
    //fill_buffer1(write_buff, geo.block_size);

    /* 把写数据缓冲的数据写入SD卡中，大小为一个块，size参数以块为单位 */
    /*block_num = rt_device_write(sd_device, 0, write_buff, 1);
    if (1 != block_num)
    {
        rt_kprintf("write device %s failed!\n", sd_name);
    }*/

    /* 从SD卡中读出数据，并保存在读数据缓冲区中 */
    block_num = rt_device_read(sd_device, 0, read_buff, 1);

    if (1 != block_num)
    {
        rt_kprintf("read %s device failed!\n", sd_name);
    }

    /* 比较写数据缓冲区和读数据缓冲区的内容是否完全一致 */
    if (rt_memcmp(write_buff, read_buff, geo.block_size) == 0)
    {
        rt_kprintf("Block test OK!\n");
    }
    else
    {
        rt_kprintf("Block test Fail!\n");
    }
    /* 释放缓冲区空间 */
    //rt_free(read_buff);    //Justin debug
    rt_free(write_buff);

    return RT_EOK;
}

/*
 * @brief  : SD卡处理线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.24
 */
void SDCardTaskEntry(void* parameter)
{
    static u8 timeCnt = 0;

    sd_sample1();

    while(1)
    {
        /* 1s定时触发 */
        if(0 == (timeCnt % 20))
        {
            rt_mq_send(&uartSendMsg, read_buff, 8);
        }

        timeCnt++;
        rt_thread_mdelay(50);
    }
}

/*
 * @brief  : SD卡处理线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.24
 */
void SDCardTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("sd card task", SDCardTaskEntry, RT_NULL, 1024, 26, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("sensor task start failed");
        }
    } else {
        LOG_E("sensor task create failed");
    }
}
