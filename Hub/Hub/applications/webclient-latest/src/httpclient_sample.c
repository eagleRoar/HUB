/*
 * File      : httpclient.c
 *
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date             Author      Notes
 * 2018-07-20     flybreak     first version
 * 2018-09-05     flybreak     Upgrade API to webclient latest version
 */

#include <webclient.h>  /* 使用 HTTP 协议与服务器通信需要包含此头文件 */
#include <sys/socket.h> /* 使用BSD socket，需要包含socket.h头文件 */
#include <netdb.h>
//#include <cJSON.h>
//#include <finsh.h>
#include "GlobalConfig.h"
#include "typedef.h"
#include <dfs_posix.h>
#include <rtdbg.h>
#include "SDCardBusiness.h"

//#define DOWNLOAD                "/download"
//#define DOWNLOAD_FILE           "/download/downloadFile.bin"

#define GET_HEADER_BUFSZ        1024        //头部大小
#define GET_RESP_BUFSZ          1024        //响应缓冲区大小

typedef struct {
    u8  type[4];
    u16 algo;
    u16 algo2;
    u32 time_stamp;
    u8  part_name[16];
    u8  fw_ver[24];//版本号
    u8  prod_code[24];
    u32 pkg_crc;
    u32 raw_crc;
    u32 raw_size;
    u32 pkg_size;
    u32 hdr_crc;
}fw_info_t;

/**
 * @brief  SD处理初始化
 * @return RT_ERROR 初始化失败
 *         RT_EOK   初始化成功
 */
//rt_err_t SDInit(void)
//{
//    rt_device_t dev;
//    rt_err_t ret;
//
//    //SD卡结构初始化
//    if (sd_card_is_vaild()) //检查SD卡是否有插
//    {
//        dev = rt_device_find("sd0");
//        if (dev != RT_NULL)
//        {
//            if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
//            {
//                LOG_E("sd card mount to / success!");
//                CheckDirectory(DOWNLOAD_DIR);
//
//                ret = RT_EOK;
//            }
//            else
//            {
//                LOG_E("sd card mount to / failed!");
//                ret = RT_ERROR;
//            }
//        }
//        else
//        {
//            LOG_E("find sd0 failed!");
//            ret =  RT_ERROR;
//        }
//    }
//    else
//    {
//        LOG_E("find sd card failed!");
//        ret =  RT_ERROR;
//    }
//
//    return ret;
//}

/**
 * @brief 将数据写入相应文件
 *
 * @param name 写入的文件名称
 * @param text 需要写入的数据内容
 * @param l 写入长度
 * @return
 */
rt_err_t writeData(char* name, void* text, u32 offset, u32 l)
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
            //rt_kprintf("Write done.\n");
            return RT_EOK;
        }
    }
    return RT_ERROR;
}

/**
 * @brief 从文件中读取到固定位置的数据
 *
 * @param name 需要读取的文件
 * @param text 返回的数据
 * @param offset 偏移量
 * @param l 文件读取的长度
 * @return
 */
rt_err_t readData(char* name, void* text, u32 offset,u32 l)
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
 * @brief 删除相应文件
 *
 * @param name 文件名称
 * @return RT_ERROR 删除失败
 * @return RT_EOK   删除成功
 */
rt_err_t deleteFile(char* name)
{
    int ret;
    ret = rmdir(name);
    if(0 == ret)
    {
        return RT_EOK;
    }
    else
    {
        return RT_ERROR;
    }

}

/**
 * @brief  识别是否需要升级
 *
 * @param  dest 当前的固件包信息
 * @param  src  需要下载的固件包信息
 * @return RT_ERROR 不需要升级
 * @return RT_EOK   需要升级
 */
rt_err_t checkUpdataApp(fw_info_t *dest, fw_info_t *src)
{
    /* 对比版本号是否一样 */
    if(!rt_memcmp(dest->fw_ver, src->fw_ver, 24)) //相同
    {
        LOG_D("app version is the last,is %d.%d.%d, no need to update"
                   , dest->fw_ver[1] - 48, dest->fw_ver[3] - 48, dest->fw_ver[4] - 48);
        return RT_ERROR;
    }
    else
    {
        LOG_D("current version is %d.%d.%d, the new update app version is %d.%d.%d"
                , dest->fw_ver[1]- 48, dest->fw_ver[3]- 48 , dest->fw_ver[4]- 48
                , src->fw_ver[1]- 48, src->fw_ver[3]- 48,src->fw_ver[4]- 48);
        return RT_EOK;
    }
}

void GetUpdataFileFromWeb(void)
{
    rt_uint8_t *buffer = RT_NULL;
    int resp_status;
    struct webclient_session *session = RT_NULL;
    char *weather_url = "http://pic.pro-leaf.com/down/beleaf_hub.bin"/*"http://192.168.0.195:8080/test.txt"*/;
    int content_length = -1, bytes_read = 0;
    u32 content_pos = 0;
    fw_info_t dest, src;
    static rt_err_t sdInitFlag = RT_ERROR;

    /* 创建会话并且设置响应的大小 */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        LOG_E("No memory for get header!");
        goto __exit;
    }

    /* 发送 GET 请求使用默认的头部 */
    if ((resp_status = webclient_get(session, weather_url)) != 200)
    {
        LOG_E("webclient GET request failed, response(%d) error.", resp_status);
        goto __exit;
    }

    /* 分配用于存放接收数据的缓冲 */
    buffer = rt_calloc(1, GET_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        LOG_E("No memory for data receive buffer!");
        goto __exit;
    }

    sdInitFlag = RT_EOK;//SDInit();//Justin debug

    content_length = webclient_content_length_get(session);
    LOG_I("get connect data length = %d-----------------",content_length);

    if(RT_EOK == sdInitFlag)
    {
        /* 识别是否需要下载 */
        webclient_read(session, &src, sizeof(fw_info_t));     //获取网络更新包的app信息, 读取之后偏移量会后移，
                                                              //因此如果为真,该内容需要保存到文本
        readData(DOWNLOAD_FILE, &dest,0,sizeof(fw_info_t));   //获取当前SD卡的app信息

        if(RT_EOK == checkUpdataApp(&dest, &src))
        {
            /* 删除原来位置的文件 */
            if(RT_EOK == deleteFile(DOWNLOAD_FILE))//Justin debug
            {
                LOG_I("delete file successful");
            }
            else
            {
                LOG_E("delete file faile");
            }

            content_pos = sizeof(fw_info_t);//当前偏移值
            writeData(DOWNLOAD_FILE,&src,0,content_pos); //将刚才读取的app数据存入升级包
            if (content_length < 0)
            {
                /* 返回的数据是分块传输的. */
                do
                {
                    bytes_read = webclient_read(session, buffer, GET_RESP_BUFSZ);

                    if (bytes_read <= 0)
                    {
                        break;
                    }

                        /* 将数据存入SD卡 */
                    writeData(DOWNLOAD_FILE,buffer,content_pos,bytes_read);
                }while (1);
            }
            else
            {
                do
                {
                    bytes_read = webclient_read(session, buffer,
                                                content_length - content_pos > GET_RESP_BUFSZ ?
                                                GET_RESP_BUFSZ : content_length - content_pos);

                    if (bytes_read <= 0)
                    {
                        break;
                    }

                    /* 将数据存入SD卡 */
                    writeData(DOWNLOAD_FILE,buffer,content_pos,bytes_read);

                    content_pos += bytes_read;
                    LOG_I("save file data %d",content_pos);
                }while (content_pos < content_length);
			}
        }
        else
        {
            LOG_E("no need to update\r\n");
        }
    }
    else
    {
        LOG_E("sd init fail");
    }

__exit:

    /* 关闭会话 */
    if (session != RT_NULL)
        webclient_close(session);
    /* 释放缓冲区空间 */
    if (buffer != RT_NULL)
        rt_free(buffer);
}


