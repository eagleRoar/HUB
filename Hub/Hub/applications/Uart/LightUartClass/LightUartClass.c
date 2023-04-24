/*
#include <LightUartClass.h>
#include <DeviceUartClass.h>
#include <DeviceUartClass/UartClass.h>
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-04     Administrator       the first version
 */
#include "SeqList.h"
#include "UartEventType.h"
#include "Command.h"
#include "Module.h"
#include "LightUartClass.h"
#include "UartDataLayer.h"

//设备类型
type_uart_class lightObject;
static uart_send_line sendMoni[LINE_MAX]; //优化设备发送

static void GenerateVuleByCtrl(line_t *line, u8 port, u8 state, u8 value, u16 *res)
{
    if(MANUAL_HAND_ON == line->port[port]._manual.manual)
    {
        *res = 0x0164;
    }
    else if(MANUAL_HAND_OFF == line->port[port]._manual.manual)
    {
        *res = 0x0000;
    }
    else
    {
        *res = (state << 8) | value;
    }
}

//生成发送的数据
static void GenerateSendData(line_t *light, u8 port, u16 ctrl, u8 *data)
{
    rt_memset(data, 0, 8);

    data[0] = light->addr;
    data[1] = WRITE_SINGLE;
    data[2] = (light->ctrl_addr + port) >> 8;
    data[3] = (light->ctrl_addr + port);
    data[4] = ctrl >> 8;
    data[5] = ctrl;
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

//生成可以加载到keyValue 的数据
static rt_err_t GenerateKVData(KV *kv, line_t light, u8 port, u8 *data, u8 len)
{
    seq_key_t   seq_key;

    seq_key.addr = light.addr;
    seq_key.regH = (light.ctrl_addr + port) >> 8;
    seq_key.regL = (light.ctrl_addr + port);
    seq_key.regSize = 1;
    kv->key = SeqKeyToLong(seq_key);
    kv->dataSegment.len = len;
    kv->dataSegment.data = rt_malloc(kv->dataSegment.len);

    if(kv->dataSegment.data)
    {
        rt_memcpy(kv->dataSegment.data, data, kv->dataSegment.len);
        return RT_EOK;
    }

    return RT_ERROR;
}

static void GenerateAskData(line_t line, u16 reg, u8 *data)
{
    rt_memset(data, 0, 8);

    data[0] = line.addr;
    data[1] = READ_MUTI;
    data[2] = reg >> 8;
    data[3] = reg;
    data[4] = 0x00;
    data[5] = 1;
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

/**
 * Determine whether to send data
 *
 * @return
 */
static u8 IsCtrlChange(u8 addr, u8 port, u16 ctrl)
{
    u8              i           = 0;
    u8              ret         = NO;

    //2.如果控制内容发送变化
    for(i = 0; i < LINE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            //控制内容发生变化
            if(ctrl != sendMoni[i].ctrl[port])
            {
                ret = YES;
                break;
            }
            else
            {
                ret = NO;
                break;
            }
        }
    }

    if(i == LINE_MAX)
    {
        ret = YES;
    }

    return ret;
}

static void SignLightSendFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < LINE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].SendCnt++;
            sendMoni[i].sendTime = getTimerRun();
            return;
        }
    }
}

static void SignLightRecvFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < LINE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].SendCnt = 0;
            return;
        }
    }
}

static void AddLastCtrl(u8 addr, u8 port, u16 ctrl)
{
    u8              i           = 0;

    for(i = 0; i < LINE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].ctrl[port] = ctrl;
            break;
        }
    }

    if(i == LINE_MAX)
    {
        for(i = 0; i < LINE_MAX; i++)
        {
            if(0x00 == sendMoni[i].addr)
            {
                sendMoni[i].addr = addr;
                sendMoni[i].ctrl[port] = ctrl;
                break;
            }
        }
    }
}

static u8 IsNeedSendCtrToConnect(u8 addr, u8 port, u16 *ctrl)
{
    u8 ret = NO;

    //2.如果控制内容发送变化
    for(u8 i = 0; i < LINE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            //控制内容发生变化
            timer_t timeOut = 0;
            if(sendMoni[i].SendCnt > 1)
            {
                timeOut = UART_DISCON_TIME;
            }
            else
            {
                timeOut = UART_LONG_CONN_TIME;
            }

            if(getTimerRun() > (sendMoni[i].sendTime + timeOut))
            {
                ret = YES;
                *ctrl = sendMoni[i].ctrl[port];
            }
        }
    }

    return ret;
}

static void Optimization(type_monitor_t *monitor)
{
    u8 i = 0;

    for(i = 0; i < LINE_MAX; i++)
    {
        //1.如果已经被删除了剔除队列
        if(0x00 != sendMoni[i].addr)
        {
            if(RT_NULL == GetLineByAddr(monitor, sendMoni[i].addr))
            {
                rt_memset(&sendMoni[i], 0, sizeof(uart_send_line));
            }
        }
    }

    for(i = 0; i < monitor->line_size; i++)
    {
        u8 j = 0;
        for(j = 0; j < LINE_MAX; j++)
        {
            if(sendMoni[j].addr == monitor->line[i].addr)
            {
                break;
            }
        }

        if(j == LINE_MAX)
        {
            for(j = 0; j < LINE_MAX; j++)
            {
                if(0x00 == sendMoni[j].addr)
                {
                    sendMoni[j].addr = monitor->line[i].addr;
                    sendMoni[j].sendTime = 0;
                    rt_memset( sendMoni[j].ctrl, 0, LINE_PORT_MAX);

                    break;
                }
            }
        }
    }
}
/****************Check List START***************************************************************/
static Node *GetRecvList (void)
{
    return &lightObject.recvList.list;
}

//加入到关注列表
static void AddToRecvList(KV keyData, u8 keyUnique)
{
    CreatTail(&lightObject.recvList.list, keyData, keyUnique);
}

static void DeleteRecvList(KV keyData)
{
    if(YES == KeyExist(&lightObject.recvList.list, keyData))
    {
        DeleteNode(&lightObject.recvList.list, keyData);
    }
}

static u8 KeyHasExistInRecvList(KV keyData)
{
    return KeyExist(&lightObject.recvList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInRecvList(KV keyData)
{
    if(YES == KeyExist(&lightObject.recvList.list, keyData))
    {
        if(YES == DataCorrect(&lightObject.recvList.list, keyData))
        {
            return YES;
        }
    }

    return NO;
}

/****************Check List END*****************************************************************/

/****************Task List START****************************************************************/
static Node *GetTaskList (void)
{
    return &lightObject.taskList.list;
}

//加入到关注列表
static void AddToTaskList(KV keyData, u8 keyUnique)
{
    CreatTail(&lightObject.taskList.list, keyData, keyUnique);
}

static void DeleteToTaskList(KV keyData)
{
    if(YES == KeyExist(&lightObject.taskList.list, keyData))
    {
        DeleteNode(&lightObject.taskList.list, keyData);
    }
}

static u8 KeyHasExistInTaskList(KV keyData)
{
    return KeyExist(&lightObject.taskList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInTaskList(KV keyData)
{
    if(YES == KeyExist(&lightObject.taskList.list, keyData))
    {
        if(YES == DataCorrect(&lightObject.taskList.list, keyData))
        {
            return YES;
        }
    }

    return NO;
}

/****************Task List END******************************************************************/

static void ConfigureUart2(rt_device_t *dev)
{
    lightObject.dev = dev;
}

/**传入参数
 *
 * @param addr      设备485地址
 *        reg_addr  寄存器地址
 *        reg_data  寄存器实际值
 *        reg_size  控制的寄存器值的数量
 */
static void LineCtrl(line_t *line, u8 port, u8 state, u8 value)
{
    u8          data[8];
    KV          keyValue;
    seq_key_t   seq_key;

    if(RT_NULL == lightObject.dev)
    {
        return;
    }

    //3.加入到发送列表
    u16 ctrl = 0x0000;
    GenerateVuleByCtrl(line, port, state, value, &ctrl);

    GenerateSendData(line, port, ctrl, data);

    if(YES == IsCtrlChange(line->addr, port, ctrl))
    {
        //4.添加到taskList 之后会在统一接口中实现数据的发送
        seq_key.addr = line->addr;
        seq_key.regH = (line->ctrl_addr + port) >> 8;
        seq_key.regL = (line->ctrl_addr + port);
        seq_key.regSize = 1;
        keyValue.key = SeqKeyToLong(seq_key);
        keyValue.dataSegment.len = 8;
        keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
        if(keyValue.dataSegment.data)
        {
            //5.复制实际数据
            rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

            lightObject.taskList.AddToList(keyValue, NO);

            //6.回收空间
            rt_free(keyValue.dataSegment.data);
        }

    }
}

static void AskLine(line_t line, u16 regAddr)
{
    u8          data[8];
    KV          keyValue;
    seq_key_t   seq_key;
    //1.生成数据
    GenerateAskData(line, regAddr, data);

    //2.
    seq_key.addr = line.addr;
    seq_key.regH = regAddr >> 8;
    seq_key.regL = regAddr;
    seq_key.regSize = 1;

    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = 8;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //3.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        lightObject.taskList.AddToList(keyValue, NO);

        //4.回收空间
        rt_free(keyValue.dataSegment.data);
    }
}

//发送数据保存底下终端使能
static void KeepConnect(type_monitor_t *monitor)
{
    KV          keyValue;
    u16         lastCtrl    = 0;
    u8          data[8];
    static u8   i           = 0;

    if(YES == IsNeedSendCtrToConnect(monitor->line[i].addr, 0, &lastCtrl))
    {
        //1.如果是在线 则发送前一次的数据保持连续
        GenerateSendData(&monitor->line[i], 0, lastCtrl, data);
        if(RT_EOK == GenerateKVData(&keyValue, monitor->line[i], 0, data, 8))
        {
            lightObject.taskList.AddToList(keyValue, NO);

            //3.回收空间
            rt_free(keyValue.dataSegment.data);
        }
    }

    if((i + 1) < monitor->line_size)
    {
        i++;
    }
    else
    {
        i = 0;
    }
}

static void RecvCmd(u8* data, u8 len)
{
    KV          keyValue;
    seq_key_t   key;
    line_t      *line;

    line = GetLineByAddr(GetMonitor(), data[0]);

    if((REGISTER_CODE == data[0]) || (line))
    {
        //1.判断是否是已经注册的设备
        if(line)
        {
            key.addr = data[0];
            key.regH = data[2];
            key.regL = data[3];
            key.regSize = 1;

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
            lightObject.recvList.AddToList(keyValue, NO);
            //4.回收空间
            rt_free(keyValue.dataSegment.data);
        }
    }
}

/**
 * 注意: 头节点为空节点，需要从头节点的next节点开始处理
 */
static void SendCmd(void)
{
    Node        *first;
    line_t      *line   = RT_NULL;

    first = lightObject.taskList.list.next;

    if(RT_NULL == first)
    {
        return;
    }

    //3.将任务列表的任务发送出去
    rt_device_write(*lightObject.dev,
                    0,
                    first->keyData.dataSegment.data,
                    first->keyData.dataSegment.len);

    //4.标记已经发送
    line = GetLineByAddr(GetMonitor(), LongToSeqKey(first->keyData.key).addr);
    if(line)
    {
        //添加到记录控制内容数组
        if(WRITE_SINGLE == first->keyData.dataSegment.data[1])
        {
            u16 value = (first->keyData.dataSegment.data[4] << 8) | first->keyData.dataSegment.data[5];
            u16 stoa_addr = (first->keyData.dataSegment.data[2] << 8) | first->keyData.dataSegment.data[3];

            AddLastCtrl(LongToSeqKey(first->keyData.key).addr, stoa_addr - line->ctrl_addr, value);
            SignLightSendFlag(LongToSeqKey(first->keyData.key).addr);
        }
    }

    /*rt_kprintf("sendCmd : ");
    for(int i = 0; i < first->keyData.dataSegment.len; i++)
    {
        rt_kprintf(" %x",first->keyData.dataSegment.data[i]);

    }
    rt_kprintf("\r\n");*/

    //5.将这个任务从任务列表中移出去
    lightObject.taskList.DeleteToList(first->keyData);
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
    for(i = 0; i < monitor->line_size; i++)
    {
        if(uuid == monitor->line[i].uuid)
        {
            buffer[2] = monitor->line[i].uuid >> 24;
            buffer[3] = monitor->line[i].uuid >> 16;
            buffer[4] = monitor->line[i].uuid >> 8;
            buffer[5] = monitor->line[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->line[i].addr;
            buffer[8] = monitor->line[i].type;
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

        lightObject.taskList.AddToList(keyValue, NO);

        rt_free(keyValue.dataSegment.data);
    }
}

//处理返回数据的列表,解析数据
static void RecvListHandle(void)
{
    Node        *tail = &lightObject.recvList.list;
    line_t      *line;
    type_monitor_t *monitor = GetMonitor();
    u32         uuid;
    u8          addr;
    static u8 connect[LINE_MAX];

    while(tail)
    {
        if(REGISTER_CODE == tail->keyData.key)
        {
            //响应注册事件
            //1.先查看这个uuid + addr是否存在
            uuid = (tail->keyData.dataSegment.data[9] << 24) | (tail->keyData.dataSegment.data[10] << 16) |
                    (tail->keyData.dataSegment.data[11] << 8) | tail->keyData.dataSegment.data[12];

            if(RT_EOK == CheckLineExist(monitor, uuid))
            {
                //2.如果已经注册了忽略,否则删除之后重新注册
                if(RT_ERROR == CheckLineCorrect(monitor, uuid, tail->keyData.dataSegment.data[7], tail->keyData.dataSegment.data[8]))
                {
                    //3.删除原来已经注册的
                    DeleteModule(monitor, uuid);
                    //4.通过type 设置对应的默认值
                    addr = getAllocateAddress(GetMonitor());
                    SetLineDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                    //5.发送重新分配的地址给模块
                    SendReplyRegister(uuid, addr);
                    LOG_I("-----------RecvListHandle, register again");
                }
                else
                {
                    LOG_I("----------RecvListHandle, addr %x has exist",tail->keyData.dataSegment.data[7]);
                }
            }
            else
            {
                //6.之前没有注册过的直接注册
                addr = getAllocateAddress(GetMonitor());
                SetLineDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                //7.发送重新分配的地址
                SendReplyRegister(uuid, addr);
                LOG_I("----------RecvListHandle, register ");
            }
        }
        else
        {
            line = GetLineByAddr(monitor, LongToSeqKey(tail->keyData.key).addr);
            if(line)
            {
                u8 rwType = tail->keyData.dataSegment.data[1];
                u16 reg = (LongToSeqKey(tail->keyData.key).regH << 8) | (LongToSeqKey(tail->keyData.key).regL);

                SignLightRecvFlag(line->addr);

                //1.判断返回是否是读取功能
                if(WRITE_SINGLE == rwType)
                {
                    //判断是否是开关寄存器
                    if(reg == line->ctrl_addr)
                    {
                        line->port[0].ctrl.d_state = tail->keyData.dataSegment.data[4];
                        line->port[0].ctrl.d_value = tail->keyData.dataSegment.data[5];
                    }
                }
                //针对设置4路调光  //Justin debug
                else if(WRITE_MUTI == rwType)
                {

                }
            }
        }

        //5.数据已经处理了,删除数据
        DeleteRecvList(tail->keyData);

        tail = tail->next;
    }

    //6.判断失联情况
    for(u8 i = 0; i < monitor->line_size; i++)
    {
        //1.已经发送数据了 但是数据接收超时判断为失联
        if(sendMoni[i].SendCnt > 2)
        {
            GetLineByAddr(monitor, sendMoni[i].addr)->conn_state = CON_FAIL;
        }
        else
        {
            GetLineByAddr(monitor, sendMoni[i].addr)->conn_state = CON_SUCCESS;
        }

        if(connect[i] != GetLineByAddr(monitor, sendMoni[i].addr)->conn_state)
        {
            connect[i] = GetLineByAddr(monitor, sendMoni[i].addr)->conn_state;

            if(CON_FAIL == connect[i])
            {
                LOG_E("line addr = %d disconnect, sendCnt = %d",
                        sendMoni[i].addr,sendMoni[i].SendCnt);
            }
            else if(CON_SUCCESS == connect[i])
            {
                LOG_W("line addr = %d reconnect, sendCnt = %d",
                        sendMoni[i].addr,sendMoni[i].SendCnt);
            }
        }
    }
}

void InitLightObject(void)
{
    //1.初始化记录发送情况
    rt_memset(sendMoni, 0, sizeof(uart_send_line) * LINE_MAX);

    //2.初始化相关数据
    lightObject.dev = RT_NULL;
    lightObject.recvList.list.next = RT_NULL;
    lightObject.recvList.list.keyData.key = 0x00;
    lightObject.taskList.list.next = RT_NULL;
    lightObject.taskList.list.keyData.key = 0x00;

    //3.实例化相关接口
    lightObject.ConfigureUart = ConfigureUart2;
    lightObject.DeviceCtrl = RT_NULL;
    lightObject.LineCtrl = LineCtrl;
    lightObject.AskDevice = RT_NULL;
    lightObject.AskSensor = RT_NULL;
    lightObject.AskLine = AskLine;
    lightObject.SendCmd = SendCmd;
    lightObject.RecvCmd = RecvCmd;
    lightObject.RecvListHandle = RecvListHandle;
    lightObject.KeepConnect = KeepConnect;
    lightObject.Optimization = Optimization;

    lightObject.recvList.GetList = GetRecvList;       //获取核对关注列表
    lightObject.recvList.AddToList = AddToRecvList;   //添加到关注列表中,数据接收的时候就
    lightObject.recvList.DeleteToList = DeleteRecvList;
    lightObject.recvList.CheckDataCorrect = CheckDataInRecvList;
    lightObject.recvList.KeyHasExist = KeyHasExistInRecvList;

    lightObject.taskList.GetList = GetTaskList;       //获取核对关注列表
    lightObject.taskList.AddToList = AddToTaskList;   //添加到关注列表中,数据接收的时候就
    lightObject.taskList.DeleteToList = DeleteToTaskList;
    lightObject.taskList.CheckDataCorrect = CheckDataInTaskList;
    lightObject.taskList.KeyHasExist = KeyHasExistInTaskList;
}

type_uart_class *GetLightObject(void)
{
    return &lightObject;
}

