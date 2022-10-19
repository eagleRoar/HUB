/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-15     Administrator       the first version
 */
#include "TcpProgram.h"
#include "Udp.h"

const u8    HEAD_CODE[4] = {0xA5,0xA5,0x5A,0x5A};      //该头部标识符需要修改
/**
 * 连接tcp
 * flg 是否是阻塞
 * @return RT_ROK 成功 RT_ERROR 失败
 */
rt_err_t ConnectToSever(int *sock, char *ip, uint32_t port)
{
    rt_err_t            ret                         = RT_EOK;
    struct hostent      *host;
    struct sockaddr_in  server_addr;

    host = gethostbyname(ip);
    /* 创建一个socket，类型是SOCKET_STREAM，TCP类型 */
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        ret = RT_ERROR;
    }

    /* 初始化预连接的服务端地址 */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* 连接到服务端 */
    if (connect(*sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        closesocket(*sock);
        ret = RT_ERROR;
        LOG_W("ConnectToSever close sock");
    }
    else
    {
        LOG_I("---------------- connect new sock = %d",*sock);
    }
    return ret;
}

rt_err_t TcpRecvMsg(int *sock, u8 *buff, u16 size, int *recLen)
{
    rt_err_t ret = RT_EOK;
    int bytes_received = 0;
    int state = 0;

    state = getSockState(*sock);
    if(state >= 0)
    {
        bytes_received = recv(*sock, buff, size, 0);
        *recLen = bytes_received;
        if (bytes_received <= 0)
        {
            closesocket(*sock);
            ret = RT_ERROR;
            LOG_W("TcpRecvMsg err, close sock, sock state = %d",state);
        }
    }

    return ret;
}

rt_err_t TcpSendMsg(int *sock, u8 *buff, u16 size)
{
    rt_err_t ret = RT_EOK;
    int state = 0;

//    if(0 != *sock)
    state = getSockState(*sock);
    if(state >= 0)
    {
        if(send(*sock, buff, size, 0) < 0)
        {
//            if(getSockState(*sock) >= 0)
            {
                closesocket(*sock);
            }
            //*sock = 0;
            ret = RT_ERROR;
            LOG_W("1 TcpSendMsg close sock, sock state = %d",state);
        }
    }

    return ret;
}

void SetIpAndPort(char *newIp, int newPort, struct ethDeviceStruct *masterInfo)
{
    masterInfo->SetIp(newIp);
    masterInfo->SetPort(newPort);
}

rt_err_t notifyTcpAndUdpSocket(char *newIp, int newPort, struct ethDeviceStruct *masterInfo)
{
    if(RT_NULL == masterInfo)
    {
        return RT_ERROR;
    }

    /* 设置新的Ip和port */
    SetIpAndPort(newIp, newPort, masterInfo);

    return RT_EOK;
}

//Justin debug 仅仅测试
/**
 * 本函数用于解决实际过程中碰到的粘包和分包问题
 *
 * 有可能出现情况
 * 1:整个完整包
 * 2:收到的包是上一个包的一个部分
 * 3:收到上一个包的尾包和新包的一部分
 * 4：收到上一个包的尾包和新包的完整包
 * @param data  原始数据
 * @param size  数据大小
 */
void analyzeTcpData(char *data, u16 size)
{
    u16             index               = 0;
    u16             start               = 0;

    LOG_I("-----------------------size = %d",size);

    //1.通过包头分割完整数据
    for(;index + 3 < size; index++)
    {
//        if(0 == strncmp(&data[index],HEAD_CODE,4))
        if(0 == rt_memcmp(&data[index],HEAD_CODE,4))
        {
            if(index != 0)
            {
                //LOG_I("%.*s",index - start,data + start);
                splitJointData(data + start, index - start);       //Justin debug
            }
            start = index;
        }
    }

    //标识后面如果还有数据的话需要解决
    if(start + 4 != size)//start + 3 == size - 1
    {
        //LOG_E("%.*s",size - start,data + start);
        splitJointData(data + start, size - start);       //Justin debug
    }
}

//拼包
/*
 * 还存在需要优化的,如果第一包发送过来的包头没有完整,即一部分包头在第一包,剩下在第二包的话
 */
void splitJointData(char *data, u16 size)
{
    static u8           nocompleted     = NO;       //非完整包标志
    static u16          page_size       = 0;        //标记总的包的大小
    static u16          now_size        = 0;        //累加当前的包大小
    static eth_page_t          page;

    //1.判断是否上一次是否还没有拼包完
    if(YES == nocompleted)
    {
        //1.1 判断是否存在 头部标识符，如果存在则错误
//        if(RT_NULL != strstr(data, HEAD_CODE))
        if(0 == rt_memcmp(data, HEAD_CODE, 4))
        {
            LOG_E("test 7");
            //1.上个包没有完整,但是没有收到后续包,收到了新包，那么抛弃上一包拼包
            now_size = 0;
            nocompleted = NO;       //放弃此包
            //2.释放空间
            if(RT_NULL != page.data)
            {
//                LOG_D("------------------ free");
                rt_free(page.data);
                page.data = RT_NULL;
            }
        }
        else
        {
            //1.3拼包,判断粘上这一包是否就是完整包
            if(now_size + size == page_size)
            {
                LOG_W("test 4");
                //1.继续将数据拼接上去
                //LOG_D("len = %d,size = %d",now_size - sizeof(eth_page_head),size);

                rt_memcpy(page.data + (now_size - sizeof(eth_page_head)), data, size);//Justin debug 仅仅测试

                //2.加上这一包就完成完整包
                changDataToEthPage(&page, page_size);
                now_size = 0;
                nocompleted = NO;       //标识为完整包
                //3.调用上层解析
                //LOG_W("recv buf = %.*s",page.head.length,page.data);
                analyzeCloudData(page.data, NO);//Justin debug 仅仅测试
                //4.注意解析完数据后要释放空间
                if(RT_NULL != page.data)
                {
                    //LOG_D("------------------ free");
                    rt_free(page.data);
                    page.data = RT_NULL;
                }
            }
            else if(now_size + size < page_size)
            {
                LOG_W("test 5");
                //1.数据拼接上去
                rt_memcpy(page.data + (now_size - sizeof(eth_page_head)), data, size);//Justin debug 仅仅测试
                //2.这一包不是尾包 继续等待粘包
                now_size += size;
                nocompleted = YES;       //标识为存在未完整包

            }
            else if(now_size + size > page_size)
            {
                LOG_E("test 6");
                //1.加上这一包就超过了,说明丢包,直接放弃粘包
                now_size = 0;
                nocompleted = NO;       //放弃此包
                //2.释放空间
                if(RT_NULL != page.data)
                {
//                    LOG_D("------------------ free");
                    rt_free(page.data);
                    page.data = RT_NULL;
                }
            }
        }
    }
    //2.处理新包
    else
    {
//        LOG_I("-------------------size = %d",size);
        //2.1 存在标识符才处理
//        if(0 == strncmp(data, HEAD_CODE, 4))
        if(0 == rt_memcmp(data, HEAD_CODE, 4))
        {
            //1.读取头部 查看需要多大的空间
            page.head.length = readePageLength(data, size);
            //1.申请动态空间
            if(page.head.length > 0)
            {
                LOG_D("length = %d",page.head.length);
                page.data = rt_malloc(page.head.length);
                //1.申请动态空间
                if(RT_NULL != page.data)
                {
                    //2.2判断是否是完整包,如果是完整包
                    if(size >= sizeof(eth_page_head))
                    {
                        //2.2.1 如果收到的包的大小 和 包里面的长度+包头大小一样的话,那么这一包刚好接收完成
                        if(page.head.length == size - sizeof(eth_page_head))
                        {
                            LOG_W("test 1");
                            rt_memcpy((u8 *)&page.head, data, sizeof(eth_page_head));
                            rt_memcpy(page.data, data + sizeof(eth_page_head), size - sizeof(eth_page_head));
                            changDataToEthPage(&page, size);
                            //1.将标志置零
                            nocompleted = NO;
                            page_size = 0;
                            now_size = 0;
                            //2.调用上层解析
                            LOG_I("len = %d ,recv buf = %.*s",page.head.length,page.head.length,page.data);
                            analyzeCloudData(page.data, NO);//Justin debug 仅仅测试
                            //3.注意解析完数据后要释放空间
                            if(RT_NULL != page.data)
                            {
//                                LOG_D("------------------ free");
                                rt_free(page.data);
                                page.data = RT_NULL;
                            }
                        }
                        //2.2.2 如果接收的包的大小小于 包里面的长度+包头大小的话,那么这包没接收完，等待接收完成
                        else if(size - sizeof(eth_page_head) < page.head.length)
                        {
                            LOG_W("test 2");
//                            LOG_D("size = %d,sizeof(eth_page_head) = %d,page.head.length = %d",
//                                    size,sizeof(eth_page_head),page.head.length);
                            if(size == sizeof(eth_page_head))
                            {
                                rt_memcpy((u8 *)&page.head, data, sizeof(eth_page_head));
                            }
                            else if(size > sizeof(eth_page_head))
                            {
                                rt_memcpy((u8 *)&page.head, data, sizeof(eth_page_head));
                                rt_memcpy(page.data, data + sizeof(eth_page_head), size - sizeof(eth_page_head));
                            }
                            changDataToEthPage(&page, size);
                            nocompleted = YES;
                            page_size = page.head.length + sizeof(eth_page_head);        //保存应该接收到的总的包的大小
                            now_size = size;
                        }
                        //2.2.3 如果接收的包的大小小于 包里面的长度+包头大小的话,认为包出错,直接舍弃
                        else
                        {
                            LOG_E("test 3");
                            //LOG_W("recv size = %d, page_len = %d",size,page.head.length);
                            //1.标志置为零
                            nocompleted = NO;
                            page_size = 0;
                            now_size = 0;
                            //2.释放空间
                            if(RT_NULL != page.data)
                            {
//                                LOG_D("------------------ free");
                                rt_free(page.data);
                                page.data = RT_NULL;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            //后续继续处理如果包头不完整的拼包

        }
    }
}

//由于网络是大端模式,stm32是小端模式
void changDataToEthPage(eth_page_t *page, u16 size)
{
    //1.大小必须大于包头大小
    if(size >= sizeof(eth_page_head))
    {
        //2.转化长度(后面app 那边转化了，长度就不转了)
//        page->head.length = (page->head.length >> 8) | (page->head.length << 8);
    }
}

//转化原始数据,读取包的大小
u16 readePageLength(char *data, u16 size)
{
    eth_page_head head;
    head.length = 0;

    //1.大小必须大于包头大小
    if(size >= sizeof(eth_page_head))
    {
        rt_memcpy(&head, data, sizeof(eth_page_head));
//        head.length = (head.length >> 8) | (head.length << 8);
    }

    return head.length;
}

