/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-12     Qiuyijie     the first version
 */

#include "uart.h"


static rt_mutex_t recvUartMutex = RT_NULL;    //指向互斥量的指针
static rt_device_t serial;
static struct rt_messagequeue uartRecvMsg;    //串口接收数据消息队列
struct rt_messagequeue uartSendMsg;           //串口发送数据消息队列
extern struct rt_messagequeue ethMsg;         //接收网口数据消息队列

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;


    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&uartRecvMsg, &msg, sizeof(msg));
    if ( result == -RT_EFULL)
    {
        /* 消息队列满 */
        rt_kprintf("message queue full！\n");
    }



    return result;
}

/**
 * @brief  : 传感器类串口线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.12
 */
void sensorUart2TaskEntry(void* parameter)
{
    char recvEthBuf[30];                  //接收以太网数据缓存
    struct rx_msg msg;                    //接收串口数据结构体
    rt_err_t result;
    rt_err_t sendMsgRes;                  //发送消息队列结果
    rt_uint32_t recvUartLength = 0;       //接收串口数据长度
    u16 crc16Result = 0;                  //CRC16校验码
    static char recvUartBuf[65];          //接收串口数据缓存，从数据结构体转化到接收缓存
    static u8 sendUartBuf[30];            //发送串口数据缓存
    static u8 timeCnt = 0;

    /* 查找串口设备 */
    serial = rt_device_find("uart2");
    if (!serial)
    {
        rt_kprintf("find uart2 failed!\n");
    }

    /* 以 DMA 接收及轮询发送方式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_DMA_RX);
    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(serial, uart_input);
    while (1)
    {
        rt_mutex_take(recvUartMutex, RT_WAITING_FOREVER);       //加锁保护
        timeCnt++;
        /* 清空接收串口数据结构体 */
        rt_memset(&msg, 0, sizeof(msg));
        /* 从串口消息队列中读取消息 */
        result = rt_mq_recv(&uartRecvMsg, &msg, sizeof(msg), RT_WAITING_NO);//Justin debug 需要将等待时间改为可以设置的
        if (result == RT_EOK)
        {
            /* 从串口读取数据*/
            recvUartLength = rt_device_read(msg.dev, 0, recvUartBuf, msg.size);
            recvUartBuf[recvUartLength] = '\0';

            if(13 == recvUartLength)
            {
                sendUartBuf[0] = recvUartBuf[7];
                sendUartBuf[1] = recvUartBuf[8];
                sendUartBuf[2] = recvUartBuf[5];
                sendUartBuf[3] = recvUartBuf[6];
                sendUartBuf[4] = recvUartBuf[3];
                sendUartBuf[5] = recvUartBuf[4];
                sendUartBuf[6] = recvUartBuf[9];
                sendUartBuf[7] = recvUartBuf[10];
                /* 通过串口设备 serial 输出读取到的消息到以太网线程 */
                sendMsgRes = rt_mq_send(&uartSendMsg, sendUartBuf, /*recvUartLength*/8);
                if(RT_EOK != sendMsgRes)
                {
                    //LOG_E("Uart send message to Ethernet Error");
                }
                else
                {
                    //rt_device_write(serial, 0, recvUartBuf, recvUartLength);
                }
            }
        }

        /* 周期性1s向四合一模块发送询问指令 */
        if(0 == (timeCnt % 20))
        {
            sendUartBuf[0] = 0x01;
            sendUartBuf[1] = 0x03;
            sendUartBuf[2] = 0x00;
            sendUartBuf[3] = 0x10;
            sendUartBuf[4] = 0x00;
            sendUartBuf[5] = 0x04;
            crc16Result = usModbusRTU_CRC(sendUartBuf,6);
            sendUartBuf[6] = crc16Result;                       //CRC16低位
            sendUartBuf[7] = (crc16Result>>8);                  //CRC16高位

            rt_device_write(serial, 0, sendUartBuf, 8);
//            rt_mq_send(&uartSendMsg, sendUartBuf, 8);//Justin debug
        }

        /* 从以太网消息队列中接收消息 */
        if (rt_mq_recv(&ethMsg, &recvEthBuf[0], sizeof(recvEthBuf), RT_WAITING_NO) == RT_EOK)
        {
            rt_device_write(serial, 0, &recvEthBuf[0], (sizeof(recvEthBuf) - 1));
        }
        rt_mutex_release(recvUartMutex);                        //解锁

        rt_thread_mdelay(50);//Justin debug
    }
}

/**
 * @brief  : 传感器类串口线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.01.17
 */
void sensorUart2TaskInit(void)
{
    rt_err_t threadStart = RT_NULL;
    static char recvMsgPool[256];
    static char sendMsgPool[256];


    /* 创建一个动态互斥量 */
    recvUartMutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
    if (recvUartMutex == RT_NULL)
    {
        LOG_E("create dynamic mutex failed.\n");
    }

    /* 初始化接收串口数据消息队列 */
    rt_mq_init(&uartRecvMsg, "uartrecv_msg",
               recvMsgPool,                     // 存放消息的缓冲区
               sizeof(struct rx_msg),           // 一条消息的最大长度
               sizeof(recvMsgPool),             // 存放消息的缓冲区大小
               RT_IPC_FLAG_FIFO);               // 如果有多个线程等待，按照先来先得到的方法分配消息

    /* 初始化发送串口数据消息队列 */
    rt_mq_init(&uartSendMsg, "uartsend_msg",
               sendMsgPool,                     // 存放消息的缓冲区
               30,                              // 一条消息的最大长度
               sizeof(sendMsgPool),             // 存放消息的缓冲区大小
               RT_IPC_FLAG_FIFO);               // 如果有多个线程等待，按照先来先得到的方法分配消息

    /* 创建串口 线程 */
    rt_thread_t thread = rt_thread_create("sensor task", sensorUart2TaskEntry, RT_NULL, 2048, 25, 10);

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


/*
 * INPUT:
 * pucData: input the data for CRC16
 * ulLen : the length of the data
 *
 * OUTPUT: the value for (CRC16)
*/
u16 usModbusRTU_CRC(const u8* pucData, u32 ulLen)
{
    u8 ucIndex = 0U;
    u16 usCRC = 0xFFFFU;

    while (ulLen > 0U) {
        usCRC ^= *pucData++;
        while (ucIndex < 8U) {
            if (usCRC & 0x0001U) {
                usCRC >>= 1U;
                usCRC ^= 0xA001U;
            } else {
                usCRC >>= 1U;
            }
            ucIndex++;
        }
        ucIndex = 0U;
        ulLen--;
    }
    return usCRC;
}

#if 0
//-----------------------------------------------------------------
// CRC_16校验码表(由上面的比特型CRC16校验函数生成，共256项，512字节)
//-----------------------------------------------------------------
u16 CRC_T16[] =
{
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 0xC601, 0x06C0,
    0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0, 0x0D80, 0xCD41,
    0x0F00, 0xCFC1, 0xCE81, 0x0E40, 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0,
    0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 0x1400, 0xD4C1,
    0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341,
    0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1,
    0xF281, 0x3240, 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0,
    0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 0x2800, 0xE8C1, 0xE981, 0x2940,
    0xEB01, 0x2BC0, 0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1,
    0xEC81, 0x2C40, 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 0xA001, 0x60C0,
    0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1, 0xA781, 0x6740,
    0xA501, 0x65C0, 0x6480, 0xA441, 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0,
    0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 0xBE01, 0x7EC0,
    0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541,
    0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0,
    0x7080, 0xB041, 0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0,
    0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 0x5A00, 0x9AC1, 0x9B81, 0x5B40,
    0x9901, 0x59C0, 0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1,
    0x8A81, 0x4A40, 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 0x8201, 0x42C0,
    0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};
//-----------------------------------------------------------------
// CRC_16校验函数(查表法)
// uCRC16 = 0xFFFF;                                 校验码初值统一约定为0xFFFF;
//-----------------------------------------------------------------
void CRC_16(u8 d) //CRC_16校验函数,查表法
{
    uCRC16 = (uCRC16 >> 8) ^ CRC_T16[(uCRC16 & 0xFF) ^ d];
}
#endif
