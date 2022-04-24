/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */

//rt-include-------------------------------------------------------
#include <dfs_posix.h>
#include <rtdevice.h>
#include <rtthread.h>
//user-include-------------------------------------------------------
#include "SDCard.h"
#include "drv_flash.h"
#define DBG_TAG "u.sd"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>
//extern---------------------------------------------------------------
static rt_mutex_t sd_dfs_mutex = RT_NULL;

//---------------------------------------------------------------------

#ifndef bool
typedef enum {FALSE = 0, TRUE = !FALSE} bool;
#endif


/**
 * @brief SD处理线程初始化
 * @return
 */
int SDCardTaskInit(void)
{
    rt_err_t ret = RT_EOK;

    /* 创建一个SD-DFS互斥量 */
    sd_dfs_mutex = rt_mutex_create("sd_dfs", RT_IPC_FLAG_FIFO);

    /* 创建 SD卡线程 */
    rt_thread_t thread = rt_thread_create(SD_CARD_TASK, sd_dfs_event_entry, RT_NULL, 1024*2, SDCARD_PRIORITY, 10);

    /* 创建成功则启动线程 */
    if (thread != RT_NULL) {
        rt_thread_startup(thread);
        LOG_I("start Thread [event dfs] sucess");
    } else {
        LOG_E("start Thread [event dfs] failed");
        ret = RT_ERROR;
    }

    return ret;
}

//INIT_APP_EXPORT(SDCardTaskInit);

/**
 * @brief SD卡相关处理处理事件
 *
 * @param parameter
 */

void sd_dfs_event_entry(void* parameter)
{
    rt_device_t dev;

    static int dfsMountFlag = RT_ERROR;

    while (1) {

        rt_mutex_take(sd_dfs_mutex, RT_WAITING_FOREVER);

        /* 检查SD卡是否存在 */
        if(sd_card_is_vaild())
        {
            /* 寻找SD设备 */
            dev = rt_device_find("sd0");

            if (dev != RT_NULL)
            {
                /* 将SD卡挂载在根目录下 */
                if(RT_EOK != dfsMountFlag)//如果还没挂载，尝试挂载
                {
                    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)//SD挂载在根目录下
                    {
                        dfsMountFlag = RT_EOK;

                        sd_file_init();//寻找文件夹，不存在则创建

                        LOG_I("sd card mount to / success!\r\n");
                    }
                    else //挂载失败
                    {
                        LOG_E("sd card mount to / failed!\r\n");
                    }
                }
            }
            else
            {

                LOG_E("sd card find failed!\r\n");
            }
        }
        else
        {

            LOG_E("The SD card slot is empty!\r\n");
        }

        rt_mutex_release(sd_dfs_mutex);
        rt_thread_mdelay(50);
    }
}

/**
 * @brief 检测SD是否存在
 * @return 返回SD卡读取的电平，高为卡有效，低为卡无效
 */
int sd_card_is_vaild(void)
{
    return (rt_pin_read(SD_CHK_PIN) == PIN_LOW) ? (1) : (0);
}

/**
 * @brief SD内文件和文件夹有效性
 */
void sd_file_init(void)
{
    //检测文件夹可读性
    rt_access_dir(DOWNLOADFILE);
}

/**
 * @brief 检测文件是否存在,并获取文件长度
 * @param name:相关文件名称
 * @return 返回相关文件长度
 */
u32 length_file(char* name)
{
    int ret;
    struct stat buf;

    ret = stat(name, &buf);

    if (ret == 0) {
        LOG_D("%s file size = %d", name, buf.st_size);
        return buf.st_size;
    } else {
        LOG_E("%s file not fonud");
        return 0;
    }
}

/**
 * @brief 检测文件夹是否存在
 *
 * @param name:文件夹名称
 * @return 返回是否有效,成功为RT_EOK
 */
u8 rt_access_dir(char* name)
{
    int ret;

    ret = access(name, 0);
    if (ret < 0) {
        LOG_E("\"%s\" error, reset the dir", name);
        //创建文件夹
        ret = mkdir(name, 0x777);
        if (ret < 0) {
            LOG_E("mkdir \"%s\" error", name);
            return RT_ERROR;
        } else {
            return RT_EOK;
        }
    } else {
        return RT_EOK;
    }
}

/**
 * @brief 从文件中读取到固定位置的数据
 *
 * @param name 需要读取的文件
 * @param text 返回的数据
 * @param offset 偏移量
 * @param l 文件读取的长度
 * @return u8
 */
u8 read_data(char* name, void* text, u32 offset,u32 l)
{
    int fd;
    int size;

    /*生成文件名称*/
    /* 以创建和读写模式打开 /text.txt 文件，如果该文件不存在则创建该文件*/
    fd = open(name, O_WRONLY | O_CREAT);
    if (fd >= 0) {
        lseek(fd,offset,SEEK_SET);//设置偏移地址
        size = read(fd, text, l);
        close(fd);
        if (size > 0) {
            LOG_D("read done[%d].", size);
            return RT_EOK;
        }
    }

    return RT_ERROR;
}

/**
 * @brief 将数据写入相应文件
 *
 * @param name 写入的文件名称
 * @param offset 偏移量
 * @param text 需要写入的数据内容
 * @param l 写入长度
 * @return
 */
u8 write_data(char* name, u8* text, u32 offset, u32 l)
{
    int fd;

    if (text != NULL) {
        /*生成文件名称*/
        /* 以创建和读写模式打开 name 文件，如果该文件不存在则创建该文件*/
        fd = open(name, O_WRONLY | O_CREAT);
        if (fd >= 0) {
            lseek(fd,offset,SEEK_SET);//设置偏移地址
            write(fd, text, l);
            close(fd);

            return RT_EOK;
        }
    }
    return RT_ERROR;
}


