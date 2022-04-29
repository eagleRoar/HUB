/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-27     Administrator       the first version
 */
#include <dfs_posix.h>
#include "SdcardBusiness.h"
#include "rt_cjson_tools.h"
#include "Uart.h"

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
    rt_access_dir(MODULEFILE);
    rt_access_dir(TEST_FILE);
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
u8 write_data(char* name, void* text, u32 offset, u32 l)
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

void PrintModule(type_module_t module)
{
    LOG_D("address              = %x",module.address);
    LOG_D("module_name          = %s",module.module_name);
    LOG_D("module_t[0].name     = %s",module.module_t[0].name);
    LOG_D("module_t[1].name     = %s",module.module_t[1].name);
    LOG_D("module_t[2].name     = %s",module.module_t[2].name);
    LOG_D("module_t[3].name     = %s",module.module_t[3].name);
}

/** 开机时赋值给monitor **/
void GetMonitorFromSdCard(type_monitor_t *monitor)
{
    u8      index               = 0;

    read_data(MODULEDOCUMENT, (u8 *)monitor, 0, sizeof(struct allocateStruct));

    if(RT_ERROR == read_data(MODULEDOCUMENT, &(monitor->monitorDeviceTable.deviceManageLength), sizeof(struct allocateStruct), 1))     //该位置存储的是module长度
    {
        LOG_D("GetMonitor Err");
    }
    else
    {
        LOG_D("moduleLength = %d", monitor->monitorDeviceTable.deviceManageLength);//Justin debug

        monitor->monitorDeviceTable.deviceTable = RT_NULL;

        monitor->monitorDeviceTable.deviceTable = rt_malloc(monitor->monitorDeviceTable.deviceManageLength * sizeof(type_module_t));

        if(RT_NULL == monitor->monitorDeviceTable.deviceTable)
        {
            LOG_D("GetMonitor Err1");
        }
        else
        {
            if(RT_ERROR == read_data(MODULEDOCUMENT, monitor->monitorDeviceTable.deviceTable, sizeof(struct allocateStruct) + 1,
                    monitor->monitorDeviceTable.deviceManageLength * sizeof(type_module_t)))
            {
                LOG_D("GetMonitor Err2");
            }
            else
            {
                for(index = 0; index < monitor->monitorDeviceTable.deviceManageLength; index++)
                {
                    PrintModule(monitor->monitorDeviceTable.deviceTable[index]);
                }
            }
        }
    }
}

void SaveAddrAndLenToFile(type_monitor_t *monitor)
{
    if(RT_ERROR == write_data(MODULEDOCUMENT, &(monitor->allocateStr), 0, sizeof(struct allocateStruct)))
    {
        LOG_D("SaveModuleToFile err1");
    }
    else
    {
        LOG_D("SaveModuleToFile OK1");
    }

    if(RT_ERROR == write_data(MODULEDOCUMENT, &(monitor->monitorDeviceTable.deviceManageLength), sizeof(struct allocateStruct), 1))
    {
        LOG_D("SaveModuleToFile err2");
    }
    else
    {
        LOG_D("SaveModuleToFile OK2");
    }
}

void SaveModuleToFile(type_module_t *module, u8 index)
{
    if(RT_ERROR == write_data(MODULEDOCUMENT, module, sizeof(struct allocateStruct) + 1 + index * sizeof(type_module_t), sizeof(type_module_t)))
    {
        LOG_D("SaveModuleToFile err1");
    }
    else
    {
        LOG_D("SaveModuleToFile OK1");
    }
}

