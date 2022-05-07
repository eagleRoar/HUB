/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-27     Administrator       the first version
 */
#include "SdcardDataLayer.h"

extern rt_mutex_t       sd_dfs_mutex;

/**
 * @brief 检测SD是否存在
 * @return 返回SD卡读取的电平，高为卡有效，低为卡无效
 */
int sd_card_is_vaild(void)
{
    return (rt_pin_read(SD_CHK_PIN) == PIN_LOW) ? (1) : (0);
}

/**
 * @brief 检测文件是否存在,并获取文件长度
 * @param name:相关文件名称
 * @return 返回相关文件长度
 */
u32 GetFileLength(char* name)
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
u8 CheckDirectory(char* name)
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
u8 ReadSdData(char* name, void* text, u32 offset, u32 l)
{
    int fd;
    int size;

    /*生成文件名称*/
    rt_mutex_take(sd_dfs_mutex, RT_WAITING_FOREVER);
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
    rt_mutex_release(sd_dfs_mutex);

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
u8 WriteSdData(char* name, void* text, u32 offset, u32 l)
{
    int fd;

    rt_mutex_take(sd_dfs_mutex, RT_WAITING_FOREVER);
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
    rt_mutex_release(sd_dfs_mutex);

    return RT_ERROR;
}
