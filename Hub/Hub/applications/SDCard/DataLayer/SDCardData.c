/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */


#include "SDCardData.h"

#define         BUFF_MAX_SZ        1024

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
        LOG_E("%s file not found");
        return 0;
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
    int     fd;
    int     size;
    u8      index       = 0;
    u8      ret         = RT_EOK;

    /*生成文件名称*/

    fd = open(name, O_RDONLY | O_CREAT);
    if (fd >= 0) {
        lseek(fd,offset,SEEK_SET);//设置偏移地址

        if(l <= BUFF_MAX_SZ)
        {
            size = read(fd, text, l);
        }
        else
        {
            for(index = 0; index < l/BUFF_MAX_SZ; index++)
            {
                size = read(fd, text + index * BUFF_MAX_SZ, BUFF_MAX_SZ);
//                LOG_D("-----------read %d 1024",index+1);
            }

            if((l % BUFF_MAX_SZ) > 0)
            {
                size = read(fd, text + index * BUFF_MAX_SZ, l % BUFF_MAX_SZ);
//                LOG_D("-----------read %d %d",index+1,l % BUFF_MAX_SZ);
            }
        }

        if(0 == close(fd))
        {
            LOG_D("read fd close ok");
        }
        else
        {
            LOG_E("read fd close fail");
        }

        if (size > 0) {
            //LOG_D("read done[%d].", size);
            ret = RT_EOK;
        }
    }
    else
    {
        ret = RT_ERROR;
        LOG_E("ReadSdData ERR, fd = %d",fd);
    }

    return ret;
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
    int     fd;
    u8      index       = 0;
    u8      ret         = RT_EOK;


    if (text != NULL) {
        /*生成文件名称*/
        /* 以创建和读写模式打开 name 文件，如果该文件不存在则创建该文件*/
        fd = open(name, O_WRONLY | O_CREAT);
        if (fd >= 0) {
            lseek(fd,offset,SEEK_SET);//设置偏移地址
//            write(fd, text, l);
            if(l <= BUFF_MAX_SZ)
            {
                write(fd, text, l);
            }
            else
            {
                for(index = 0; index < l/BUFF_MAX_SZ; index++)
                {
                    write(fd, text + index * BUFF_MAX_SZ, BUFF_MAX_SZ);
//                    LOG_D("-----------write %d 1024",index+1);
                }

                if((l % BUFF_MAX_SZ) > 0)
                {
                    write(fd, text + index * BUFF_MAX_SZ, l % BUFF_MAX_SZ);
//                    LOG_D("-----------write %d %d",index+1,l % BUFF_MAX_SZ);
                }
            }

            if(0 == close(fd))
            {
                LOG_D("write fd close ok");
            }
            else
            {
                LOG_E("write fd close fail");
            }

            ret = RT_EOK;
        }
        else
        {
            LOG_E(" WriteSdData ERR 1");
        }
    }
    else
    {
        ret = RT_ERROR;
        LOG_E(" WriteSdData ERR 2");
    }


    return ret;
}
