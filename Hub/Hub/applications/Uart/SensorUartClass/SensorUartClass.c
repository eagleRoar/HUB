/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-04     Administrator       the first version
 */
#include "Command.h"
#include "SensorUartClass.h"
#include "Module.h"
#include "UartEventType.h"
#include "UartDataLayer.h"

type_uart_class SensorObject;
static uart_send_monitor sendMoni[SENSOR_MAX]; //优化设备发送


static void GenerateAskData(sensor_t sensor, u16 reg, u8 *data)
{
    rt_memset(data, 0, 8);

    data[0] = sensor.addr;
    data[1] = READ_MUTI;
    data[2] = reg >> 8;
    data[3] = reg;
    data[4] = sensor.storage_size >> 8;
    data[5] = sensor.storage_size;
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

static void SignSensorSendFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < SENSOR_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            if(sendMoni[i].SendCnt < 255)
            {
                sendMoni[i].SendCnt++;
            }
            sendMoni[i].sendTime = getTimerRun();
            return;
        }
    }
}

static void SignSensorRecvFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < SENSOR_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].SendCnt = 0;
            return;
        }
    }
}

static void AddLastCtrl(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < SENSOR_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            break;
        }
    }

    if(i == SENSOR_MAX)
    {
        for(i = 0; i < SENSOR_MAX; i++)
        {
            if(0x00 == sendMoni[i].addr)
            {
                sendMoni[i].addr = addr;
                break;
            }
        }
    }
}

static void SendReplyRegister(u32 uuid, u8 addr)
{
    u8      buffer[15];
    u16     i = 0;
    u16     crc16Result = 0x0000;
    u32     id;
    KV      keyValue;
    type_monitor_t *monitor = GetMonitor();

    buffer[0] = REGISTER_CODE;
    buffer[1] = 0x80;
    for(i = 0; i < monitor->sensor_size; i++)
    {
        if(uuid == monitor->sensor[i].uuid)
        {
            buffer[2] = monitor->sensor[i].uuid >> 24;
            buffer[3] = monitor->sensor[i].uuid >> 16;
            buffer[4] = monitor->sensor[i].uuid >> 8;
            buffer[5] = monitor->sensor[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->sensor[i].addr;
            buffer[8] = monitor->sensor[i].type;
        }
    }

    ReadUniqueId(&id);
    buffer[9] = id >> 24;
    buffer[10] = id >> 16;
    buffer[11] = id >> 8;
    buffer[12] = id;

    crc16Result = usModbusRTU_CRC(buffer, 13);
    buffer[13] = crc16Result;                         //CRC16低位
    buffer[14] = (crc16Result>>8);                    //CRC16高位

    keyValue.key = REGISTER_CODE;
    keyValue.dataSegment.len = 15;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        rt_memcpy(keyValue.dataSegment.data, buffer, keyValue.dataSegment.len);

        SensorObject.taskList.AddToList(keyValue, NO);

        rt_free(keyValue.dataSegment.data);
    }
}


static void ConfigureUart1(rt_device_t *dev)
{
    SensorObject.dev = dev;
}

static void UartAsk(sensor_t sensor, u16 regAddr)
{
    u8          data[8];
    KV          keyValue;
    seq_key_t   seq_key;
    //1.生成数据
    GenerateAskData(sensor, regAddr, data);

    //2.
    seq_key.addr = sensor.addr;
    seq_key.regH = regAddr >> 8;
    seq_key.regL = regAddr;
    seq_key.regSize = sensor.storage_size;

    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = 8;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //3.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        SensorObject.taskList.AddToList(keyValue, NO);

        //4.回收空间
        rt_free(keyValue.dataSegment.data);
    }
}

/**
 * 注意: 头节点为空节点，需要从头节点的next节点开始处理
 */
static void SendCmd(void)
{
    Node        *first;

    first = SensorObject.taskList.list.next;

    if(RT_NULL == first)
    {
        return;
    }

    //3.将任务列表的任务发送出去
    rt_device_write(*SensorObject.dev,
                    0,
                    first->keyData.dataSegment.data,
                    first->keyData.dataSegment.len);

    //4.标记已经发送
    SignSensorSendFlag(LongToSeqKey(first->keyData.key).addr);
    AddLastCtrl(LongToSeqKey(first->keyData.key).addr);

//    rt_kprintf("sendcmd : ");
//    for(int i = 0; i < first->keyData.dataSegment.len; i++)
//    {
//        rt_kprintf(" %x",first->keyData.dataSegment.data[i]);
//    }
//    rt_kprintf("\r\n");

    //5.将这个任务从任务列表中移出去
    SensorObject.taskList.DeleteToList(first->keyData);
}

static void RecvCmd(u8* data, u8 len)
{
    KV          keyValue;
    seq_key_t   key;
    sensor_t    *sensor;

    sensor = GetSensorByAddr(GetMonitor(), data[0]);

    if((REGISTER_CODE == data[0]) || (sensor))
    {
        //1.判断是否是已经注册的设备
        if(sensor)
        {
            key.addr = data[0];
            key.regH = sensor->ctrl_addr >> 8;
            key.regL = sensor->ctrl_addr;
            key.regSize = sensor->storage_size;

            keyValue.key = SeqKeyToLong(key);
        }
        //2.是否是注册信息
        else if(REGISTER_CODE == data[0])
        {
            keyValue.key = REGISTER_CODE;
        }

        keyValue.dataSegment.len = len;
        keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
        if(keyValue.dataSegment.data)
        {
            rt_memcpy(keyValue.dataSegment.data, data, len);
            //3.加入到获取回复的接口列表中,在该列表中处理事件
            SensorObject.recvList.AddToList(keyValue, NO);
            //4.回收空间
            rt_free(keyValue.dataSegment.data);
        }
    }
}


/********************************************************************************/
static Node *GetRecvList (void)
{
    return &SensorObject.recvList.list;
}

//加入到关注列表
static void AddToRecvList(KV keyData, u8 keyUnique)
{
    CreatTail(&SensorObject.recvList.list, keyData, keyUnique);
}

static void DeleteRecvList(KV keyData)
{
    if(YES == KeyExist(&SensorObject.recvList.list, keyData))
    {
        DeleteNode(&SensorObject.recvList.list, keyData);
    }
}

static u8 KeyHasExistInRecvList(KV keyData)
{
    return KeyExist(&SensorObject.recvList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInRecvList(KV keyData)
{
    if(YES == KeyExist(&SensorObject.recvList.list, keyData))
    {
        if(YES == DataCorrect(&SensorObject.recvList.list, keyData))
        {
            return YES;
        }
    }

    return NO;
}

/********************************************************************************/
static Node *GetTaskList (void)
{
    return &SensorObject.taskList.list;
}

//加入到关注列表
static void AddToTaskList(KV keyData, u8 keyUnique)
{
    CreatTail(&SensorObject.taskList.list, keyData, keyUnique);
}

static void DeleteToTaskList(KV keyData)
{
    if(YES == KeyExist(&SensorObject.taskList.list, keyData))
    {
        DeleteNode(&SensorObject.taskList.list, keyData);
    }
}

static u8 KeyHasExistInTaskList(KV keyData)
{
    return KeyExist(&SensorObject.taskList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInTaskList(KV keyData)
{
    if(YES == KeyExist(&SensorObject.taskList.list, keyData))
    {
        if(YES == DataCorrect(&SensorObject.taskList.list, keyData))
        {
            return YES;
        }
    }

    return NO;
}
/********************************************************************************/

//处理返回数据的列表,解析数据
static void RecvListHandle(void)
{
    Node        *tail = &SensorObject.recvList.list;
    sensor_t    *sensor;
    type_monitor_t *monitor = GetMonitor();
    u32         uuid;
    u8          addr;
    static u8 connect[SENSOR_MAX];

    while(tail)
    {
        if(REGISTER_CODE == tail->keyData.key)
        {
            //响应注册事件
            //1.先查看这个uuid + addr是否存在
            uuid = (tail->keyData.dataSegment.data[9] << 24) | (tail->keyData.dataSegment.data[10] << 16) |
                    (tail->keyData.dataSegment.data[11] << 8) | tail->keyData.dataSegment.data[12];

            if(RT_EOK == CheckSensorExist(monitor, uuid))
            {
                //2.如果已经注册了忽略,否则删除之后重新注册
                if(RT_ERROR == CheckSensorCorrect(monitor, uuid, tail->keyData.dataSegment.data[7], tail->keyData.dataSegment.data[8]))
                {
                    //3.删除原来已经注册的
                    DeleteModule(monitor, uuid);
                    //4.通过type 设置对应的默认值
                    addr = getAllocateAddress(monitor);
                    rt_err_t ret = SetSensorDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                    if(RT_EOK == ret)
                    {
                        //5.发送重新分配的地址给模块
                        SendReplyRegister(uuid, addr);
                        LOG_I("-----------RecvListHandle sensor, register again");
                    }
                    else if(RT_ERROR == ret)
                    {
                        monitor->allocateStr.address[addr] = 0;
                    }
                }
                else
                {
                    LOG_I("----------RecvListHandle sensor, addr %x has exist",tail->keyData.dataSegment.data[7]);
                }
            }
            else
            {
                //6.之前没有注册过的直接注册
                addr = getAllocateAddress(monitor);
                rt_err_t ret = SetSensorDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                if(RT_EOK == ret)
                {
                    //7.发送重新分配的地址
                    SendReplyRegister(uuid, addr);
                    LOG_I("----------RecvListHandle sensor, register ");
                }
                else if(RT_ERROR == ret)
                {
                    monitor->allocateStr.address[addr] = 0;
                }
            }
        }
        else
        {
            sensor = GetSensorByAddr(monitor, LongToSeqKey(tail->keyData.key).addr);
            if(sensor)
            {
                u8 rwType = tail->keyData.dataSegment.data[1];
                u8 size = tail->keyData.dataSegment.data[2];

                SignSensorRecvFlag(sensor->addr);

                if(READ_MUTI == rwType)//接收读取数据
                {
                    //1.检测寄存器类型
                    if(size == sensor->storage_size * 2)
                    {
                        for(int port = 0; port < sensor->storage_size; port++)
                        {
                            s16 value = (tail->keyData.dataSegment.data[3 + port * 2] << 8) |
                                    tail->keyData.dataSegment.data[4 + port * 2];
                            sensor->__stora[port].value = value;
                        }

                    }
                }
            }
        }

        //5.数据已经处理了,删除数据
        DeleteRecvList(tail->keyData);

        tail = tail->next;
    }

    //6.判断失联情况
    for(u8 i = 0; i < monitor->sensor_size; i++)
    {
        //1.已经发送数据了 但是数据接收超时判断为失联

        if(sendMoni[i].SendCnt > 2)
        {
            GetSensorByAddr(monitor, sendMoni[i].addr)->conn_state = CON_FAIL;
        }
        else
        {
            GetSensorByAddr(monitor, sendMoni[i].addr)->conn_state = CON_SUCCESS;
        }

        if(connect[i] != GetSensorByAddr(monitor, sendMoni[i].addr)->conn_state)
        {
            connect[i] = GetSensorByAddr(monitor, sendMoni[i].addr)->conn_state;

            if(CON_FAIL == connect[i])
            {
                LOG_E("sensor addr = %d disconnect, sendT = %d",
                        sendMoni[i].addr,sendMoni[i].SendCnt);
            }
            else if(CON_SUCCESS == connect[i])
            {
                LOG_W("sensor addr = %d reconnect, sendT = %d",
                        sendMoni[i].addr,sendMoni[i].SendCnt);
            }
        }
    }
}

static void Optimization(type_monitor_t *monitor)
{
    u8 i = 0;

    for(i = 0; i < SENSOR_MAX; i++)
    {
        //1.如果已经被删除了剔除队列
        if(0x00 != sendMoni[i].addr)
        {
            if(RT_NULL == GetSensorByAddr(monitor, sendMoni[i].addr))
            {
                rt_memset(&sendMoni[i], 0, sizeof(uart_send_monitor));
            }
        }
    }

    for(i = 0; i < monitor->sensor_size; i++)
    {
        u8 j = 0;
        for(j = 0; j < SENSOR_MAX; j++)
        {
            if(sendMoni[j].addr == monitor->sensor[i].addr)
            {
                break;
            }
        }

        if(j == SENSOR_MAX)
        {
            for(j = 0; j < SENSOR_MAX; j++)
            {
                if(0x00 == sendMoni[j].addr)
                {
                    sendMoni[j].addr = monitor->sensor[i].addr;
                    sendMoni[j].sendTime = 0;
                    sendMoni[j].ctrl = 0;

                    break;
                }
            }
        }
    }
}

static void KeepConnect(type_monitor_t *monitor)
{
    static int i = 0;
    sensor_t sensor;

    sensor = monitor->sensor[i];

    for(int item = 0; item < monitor->sensor_size; item++)
    {
        if(sendMoni[item].addr == sensor.addr)
        {
            if(getTimerRun() >= sendMoni[item].sendTime + 2000)
            {
                if(sensor.addr)
                {
                    SensorObject.AskSensor(sensor, sensor.ctrl_addr);
                }
            }
        }
    }

    if((i + 1) < monitor->sensor_size)
    {
        i++;
    }
    else
    {
        i = 0;
    }
}

void InitSensorObject(void)
{
    //1.初始化记录发送情况
    rt_memset(sendMoni, 0, sizeof(uart_send_monitor) * SENSOR_MAX);

    //2.初始化相关数据
    SensorObject.dev = RT_NULL;
    SensorObject.recvList.list.next = RT_NULL;
    SensorObject.recvList.list.keyData.key = 0x00;
    SensorObject.taskList.list.next = RT_NULL;
    SensorObject.taskList.list.keyData.key = 0x00;

    //3.实例化相关接口
    SensorObject.ConfigureUart = ConfigureUart1;
    SensorObject.DeviceCtrl = RT_NULL;
    SensorObject.AskDevice = RT_NULL;
    SensorObject.AskSensor = UartAsk;
    SensorObject.SendCmd = SendCmd;
    SensorObject.RecvCmd = RecvCmd;
    SensorObject.RecvListHandle = RecvListHandle;
    SensorObject.KeepConnect = KeepConnect;
    SensorObject.Optimization = Optimization;

    SensorObject.recvList.GetList = GetRecvList;       //获取核对关注列表
    SensorObject.recvList.AddToList = AddToRecvList;   //添加到关注列表中,数据接收的时候就
    SensorObject.recvList.DeleteToList = DeleteRecvList;
    SensorObject.recvList.CheckDataCorrect = CheckDataInRecvList;
    SensorObject.recvList.KeyHasExist = KeyHasExistInRecvList;

    SensorObject.taskList.GetList = GetTaskList;       //获取核对关注列表
    SensorObject.taskList.AddToList = AddToTaskList;   //添加到关注列表中,数据接收的时候就
    SensorObject.taskList.DeleteToList = DeleteToTaskList;
    SensorObject.taskList.CheckDataCorrect = CheckDataInTaskList;
    SensorObject.taskList.KeyHasExist = KeyHasExistInTaskList;
}

type_uart_class *GetSensorObject(void)
{
    return &SensorObject;
}
