/*
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
#include "UartDataLayer.h"
#include "DeviceUartClass.h"
#include "UartAction.h"

//设备类型
type_uart_class uart2Object;
//获取软件定时器，避免使用getTimeStamp(),因为如果时间被修改的话会导致该值被改变
static time_t timerRun;
static uart_send_monitor sendMoni[DEVICE_MAX]; //优化设备发送

//模拟软件定时器//ms
time_t getTimerRun(void)
{
    return timerRun;
}

//模拟软件定时器在跑,要在1s定时器里面跑
//返回ms
void TimerRunning(u16 data)
{
    timerRun += data;
}

seq_key_t LongToSeqKey(long a)
{
    static seq_key_t key;

    key.addr = a >> 24;
    key.regH = a >> 16;
    key.regL = a >> 8;
    key.regSize = a;

    return key;
}

long SeqKeyToLong(seq_key_t a)
{
    static long b;

    b = *(long *)&a;

    b = ((b & 0xFF000000) >> 24) | ((b & 0x00FF0000) >> 8)
             | ((b & 0x0000FF00) << 8) | ((b & 0x000000FF) << 24);

    return *(long *)&b;
}

//生成发送的数据
static void GenerateSendData(device_t device, u16 ctrl, u8 *data)
{
    rt_memset(data, 0, 8);

    data[0] = device.addr;
    data[1] = WRITE_SINGLE;
    data[2] = device.ctrl_addr >> 8;
    data[3] = device.ctrl_addr;
    data[4] = ctrl >> 8;
    data[5] = ctrl;
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

static void GenerateAskData(device_t *device, u16 reg, u8 *data)
{
    rt_memset(data, 0, 8);

    data[0] = device->addr;
    data[1] = READ_MUTI;
    data[2] = reg >> 8;
    data[3] = reg;
    if(UART_FINDLOCATION_REG == reg)
    {
        data[4] = 0x00;
        data[5] = 0x01;
    }
    else
    {
        data[4] = device->storage_size >> 8;
        data[5] = device->storage_size;
    }
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

static void GenerateChangeType(device_t *device, u8 port, u8 type, u8 *data)
{
    u16 reg = 0;

    rt_memset(data, 0, 8);

    data[0] = device->addr;
    data[1] = WRITE_SINGLE;

    GetReadRegAddrByType(device->type, &reg);

    reg += port;
    data[2] = reg >> 8;
    data[3] = reg;
    data[4] = 0x00;
    data[5] = type;
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

#if(HUB_ENVIRENMENT == HUB_SELECT)
//获取红外空调命令
static void GenerateIrAirCtrlData(u8 state, u16 *res)
{
    u16         temp        = 0;

    proTempSet_t    tempSet;
    GetNowSysSet(&tempSet, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    if(ON == state)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            temp = tempSet.dayCoolingTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            temp = tempSet.nightCoolingTarget;
        }

        if(temp > tempSet.tempDeadband)
        {
            temp -= tempSet.tempDeadband;
        }
        changeIrAirCode(temp, res);
    }
    else
    {
        *res = 0x6000;
    }
}
#endif

static void GenarateHvacCtrData(u8 HvacMode, u8 cool, u8 heat, u16 *value)
{
    //只允许AC的不允许端口中为hvac型的
    if((ON == heat) && (OFF == cool))
    {
        if(HVAC_CONVENTIONAL == HvacMode)
        {
            *value = 0x14;
        }
        else if(HVAC_PUM_O == HvacMode)
        {
            *value = 0x1C;
        }
        else if(HVAC_PUM_B == HvacMode)
        {
            *value = 0x0C;
        }
    }
    else if((ON == cool) && (OFF == heat))
    {
        if(HVAC_CONVENTIONAL == HvacMode)
        {
            *value = 0x0C;
        }
        else if(HVAC_PUM_O == HvacMode)
        {
            *value = 0x0C;
        }
        else if(HVAC_PUM_B == HvacMode)
        {
            *value = 0x1C;
        }
    }
    else if((ON == cool) && (ON == heat))
    {
        *value = 0x1F;
    }
    else
    {
        *value = 0x00;
    }
}

static void GenerateVuleBySingleCtrl(device_t device, u8 port, u8 state, u16 *value)
{

    if (1 == device.storage_size)//一个寄存器
    {
        //1.如果是非手动模式
        if(MANUAL_HAND_ON == device.port[0].manual.manual)
        {
#if(HUB_ENVIRENMENT == HUB_SELECT)
            if(HVAC_6_TYPE == device.type)
            {
                GenarateHvacCtrData(device._hvac.hvacMode, ON, ON, value);
            }
            else if(IR_AIR_TYPE == device.type)
            {
                GenerateIrAirCtrlData(ON, value);
            }
            else
#endif
            {
                *value = 0x0100;//开启
            }
        }
        else if(MANUAL_HAND_OFF == device.port[0].manual.manual)
        {
#if(HUB_ENVIRENMENT == HUB_SELECT)
            if(HVAC_6_TYPE == device.type)
            {
                GenarateHvacCtrData(device._hvac.hvacMode, OFF, OFF, value);
            }
            else if(IR_AIR_TYPE == device.type)
            {
                GenerateIrAirCtrlData(OFF, value);
            }
            else
#endif
            {
                *value = 0x0000;
            }
        }
        else
        {
            if(ON == state)
            {
#if(HUB_ENVIRENMENT == HUB_SELECT)
                if(IR_AIR_TYPE == device.type)
                {
                    GenerateIrAirCtrlData(ON, value);
                }
                else if(HVAC_6_TYPE == device.type)
                {
                    GenarateHvacCtrData(device._hvac.hvacMode, ON, ON, value);
                }
                else
#endif
                {
                    *value = 0x0100;//开启
                }
            }
            else if(OFF == state)
            {
#if(HUB_ENVIRENMENT == HUB_SELECT)
                if(HVAC_6_TYPE == device.type)
                {
                    GenarateHvacCtrData(device._hvac.hvacMode, OFF, OFF, value);
                }
                else if(IR_AIR_TYPE == device.type)
                {
                    GenerateIrAirCtrlData(OFF, value);
                }
                else
#endif
                {
                    *value = 0x0000;
                }
            }
        }
    }
    else//多个寄存器
    {
        for(u8 i = 0; i < device.storage_size; i++)
        {
            //如果先前是开着的就制1
            if(MANUAL_HAND_ON == device.port[i].manual.manual)
            {
                *value |= (1 << i);
            }
            else if(MANUAL_HAND_OFF == device.port[i].manual.manual)
            {
                *value &= ~(1 << i);
            }
            else
            {
                if(ON == device.port[i].ctrl.d_state)
                {
                    *value |= (1 << i);
                }

                if(port == i)
                {
                    if(ON == state)
                    {
                        *value |= (1 << i);
                    }
                    else if(OFF == state)
                    {
                        *value &= ~(1 << i);
                    }
                }
            }
        }
    }
}

static void GenerateVuleByCtrl(device_t device, u8 func, u8 state, u16 *value)
{

    if (1 == device.storage_size)//一个寄存器
    {
        if(func == device.port[0].func)
        {
            //1.如果是非手动模式
            if(MANUAL_HAND_ON == device.port[0].manual.manual)
            {
#if(HUB_ENVIRENMENT == HUB_SELECT)
                if(HVAC_6_TYPE == device.type)
                {
                    GenarateHvacCtrData(device._hvac.hvacMode, ON, ON, value);
                }
                else if(IR_AIR_TYPE == device.type)
                {
                    GenerateIrAirCtrlData(ON, value);
                }
                else
#endif
                {
                    *value = 0x0100;//开启
                }
            }
            else if(MANUAL_HAND_OFF == device.port[0].manual.manual)
            {
#if(HUB_ENVIRENMENT == HUB_SELECT)
                if(HVAC_6_TYPE == device.type)
                {
                    GenarateHvacCtrData(device._hvac.hvacMode, OFF, OFF, value);
                }
                else if(IR_AIR_TYPE == device.type)
                {
                    GenerateIrAirCtrlData(OFF, value);
                }
                else
#endif
                {
                    *value = 0x0000;
                }
            }
            else
            {
                if(ON == state)
                {
#if(HUB_ENVIRENMENT == HUB_SELECT)
                    if(IR_AIR_TYPE == device.type)
                    {
                        GenerateIrAirCtrlData(ON, value);
                    }
                    else if(HVAC_6_TYPE == device.type)
                    {
                        if(F_COOL == func)
                        {
                            GenarateHvacCtrData(device._hvac.hvacMode, ON, OFF, value);
                        }
                        else if(F_HEAT)
                        {
                            GenarateHvacCtrData(device._hvac.hvacMode, OFF, ON, value);
                        }
                    }
                    else
#endif
                    {
                        *value = 0x0100;//开启
                    }
                }
                else if(OFF == state)
                {
#if(HUB_ENVIRENMENT == HUB_SELECT)
                    if(IR_AIR_TYPE == device.type)
                    {
                        GenerateIrAirCtrlData(OFF, value);
                    }
                    else if(HVAC_6_TYPE == device.type)
                    {
                        GenarateHvacCtrData(device._hvac.hvacMode, ON, ON, value);
                    }
                    else
#endif
                    {
                        *value = 0x0000;
                    }
                }
            }
        }
    }
    else//多个寄存器
    {
        for(u8 port = 0; port < device.storage_size; port++)
        {
            //如果先前是开着的就制1
            if(MANUAL_HAND_ON == device.port[port].manual.manual)
            {
                *value |= (1 << port);
            }
            else if(MANUAL_HAND_OFF == device.port[port].manual.manual)
            {
                *value &= ~(1 << port);
            }
            else
            {
                if(ON == device.port[port].ctrl.d_state)
                {
                    *value |= (1 << port);
                }

                if(func == device.port[port].func)
                {
                    if(ON == state)
                    {
                        *value |= (1 << port);
                    }
                    else if(OFF == state)
                    {
                        *value &= ~(1 << port);
                    }
                }
            }
        }
    }
}

////生成发送的数据
//static void GenerateRegisterData(u32 destUuid, u8 newAddr, u8 type, u32 souceUuid, u8 *data)
//{
//    rt_memset(data, 0, 15);
//
//    data[0] = REGISTER_CODE;
//    data[1] = 0x80;
//    data[2] = destUuid >> 24;
//    data[3] = destUuid >> 16;
//    data[4] = destUuid >> 8;
//    data[5] = destUuid;
//    data[6] = 6;
//    data[7] = newAddr;
//    data[8] = type;
//    data[9] = souceUuid >> 24;
//    data[10] = souceUuid >> 16;
//    data[11] = souceUuid >> 8;
//    data[12] = souceUuid;
//    data[13] = usModbusRTU_CRC(data, 13);
//    data[14] = usModbusRTU_CRC(data, 13) >> 8;
//}

//生成可以加载到keyValue 的数据
static rt_err_t GenerateKVData(KV *kv, device_t device, u8 *data, u8 len)
{
    seq_key_t   seq_key;

    seq_key.addr = device.addr;
    seq_key.regH = device.ctrl_addr >> 8;
    seq_key.regL = device.ctrl_addr;
    seq_key.regSize = device.storage_size;
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

/**
 * Determine whether to send data
 *
 * @return
 */
static u8 IsCtrlChange(u8 addr, u16 ctrl)
{
    u8              i           = 0;
    u8              ret         = NO;

    //2.如果控制内容发送变化
    for(i = 0; i < DEVICE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            //控制内容发生变化
            if(ctrl != sendMoni[i].ctrl)
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

    if(i == DEVICE_MAX)
    {
        ret = YES;
    }

    return ret;
}

static void SignDeviceSendFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < DEVICE_MAX; i++)
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

static void SignDeviceRecvFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < DEVICE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].SendCnt = 0;
            return;
        }
    }
}

static void AddLastCtrl(u8 addr, u16 ctrl)
{
    u8              i           = 0;

    for(i = 0; i < DEVICE_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].ctrl = ctrl;
            break;
        }
    }

    if(i == DEVICE_MAX)
    {
        for(i = 0; i < DEVICE_MAX; i++)
        {
            if(0x00 == sendMoni[i].addr)
            {
                sendMoni[i].addr = addr;
                sendMoni[i].ctrl = ctrl;
                break;
            }
        }
    }
}

static u8 IsNeedSendCtrToConnect(u8 addr, u16 *ctrl)
{
    u8 ret = NO;

    //2.如果控制内容发送变化
    for(u8 i = 0; i < GetMonitor()->device_size; i++)
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
                *ctrl = sendMoni[i].ctrl;
            }
        }
    }

    return ret;
}

static void Optimization(type_monitor_t *monitor)
{
    u8 i = 0;

    for(i = 0; i < DEVICE_MAX; i++)
    {
        //1.如果已经被删除了剔除队列
        if(0x00 != sendMoni[i].addr)
        {
            if(RT_NULL == GetDeviceByAddr(monitor, sendMoni[i].addr))
            {
                rt_memset(&sendMoni[i], 0, sizeof(uart_send_monitor));
            }
        }
    }

    for(i = 0; i < monitor->device_size; i++)
    {
        u8 j = 0;
        for(j = 0; j < DEVICE_MAX; j++)
        {
            if(sendMoni[j].addr == monitor->device[i].addr)
            {
                break;
            }
        }

        if(j == DEVICE_MAX)
        {
            for(j = 0; j < DEVICE_MAX; j++)
            {
                if(0x00 == sendMoni[j].addr)
                {
                    sendMoni[j].addr = monitor->device[i].addr;
                    sendMoni[j].sendTime = 0;
                    sendMoni[j].ctrl = 0;

                    break;
                }
            }
        }

    }
}
/****************Check List START***************************************************************/
static Node *GetRecvList (void)
{
    return &uart2Object.recvList.list;
}

//加入到关注列表
static void AddToRecvList(KV keyData, u8 keyUnique)
{
    CreatTail(&uart2Object.recvList.list, keyData, keyUnique);
}

static void DeleteRecvList(KV keyData)
{
    if(YES == KeyExist(&uart2Object.recvList.list, keyData))
    {
        DeleteNode(&uart2Object.recvList.list, keyData);
    }
}

static u8 KeyHasExistInRecvList(KV keyData)
{
    return KeyExist(&uart2Object.recvList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInRecvList(KV keyData)
{
    if(YES == KeyExist(&uart2Object.recvList.list, keyData))
    {
        if(YES == DataCorrect(&uart2Object.recvList.list, keyData))
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
    return &uart2Object.taskList.list;
}

//加入到关注列表
static void AddToTaskList(KV keyData, u8 keyUnique)
{
    CreatTail(&uart2Object.taskList.list, keyData, keyUnique);
}

static void DeleteToTaskList(KV keyData)
{
    if(YES == KeyExist(&uart2Object.taskList.list, keyData))
    {
        DeleteNode(&uart2Object.taskList.list, keyData);
    }
}

static u8 KeyHasExistInTaskList(KV keyData)
{
    return KeyExist(&uart2Object.taskList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInTaskList(KV keyData)
{
    if(YES == KeyExist(&uart2Object.taskList.list, keyData))
    {
        if(YES == DataCorrect(&uart2Object.taskList.list, keyData))
        {
            return YES;
        }
    }

    return NO;
}

/****************Task List END******************************************************************/

static void ConfigureUart2(rt_device_t *dev)
{
    uart2Object.dev = dev;
}

/**传入参数
 *
 * @param addr      设备485地址
 *        reg_addr  寄存器地址
 *        reg_data  寄存器实际值
 *        reg_size  控制的寄存器值的数量
 */
static void DeviceCtrl(type_monitor_t *monitor, u8 func, u8 state)
{
    u8          data[8];
    u16         value       = 0;
    KV          keyValue;
    seq_key_t   seq_key;
    device_t    device;

//    PrintNode(&uart2Object.taskList.list);

    if(RT_NULL == uart2Object.dev)
    {
        return;
    }

    //1.遍历当前设备列表 控制设备
    for(u8 i = 0; i < monitor->device_size; i++)
    {

        //2.组合发送的数据
        value = 0x0000;
        device = GetMonitor()->device[i];

        GenerateVuleByCtrl(device, func, state, &value);

        //3.加入到发送列表
        GenerateSendData(monitor->device[i], value, data);

        if(YES == IsCtrlChange(monitor->device[i].addr, value))
        {
            //4.0 判断是否是需要控制的
            if(YES == IsExistFunc(monitor, monitor->device[i].addr, func))
            {
                //4.添加到taskList 之后会在统一接口中实现数据的发送
                seq_key.addr = monitor->device[i].addr;
                seq_key.regH = monitor->device[i].ctrl_addr >> 8;
                seq_key.regL = monitor->device[i].ctrl_addr;
                seq_key.regSize = monitor->device[i].storage_size;
                keyValue.key = SeqKeyToLong(seq_key);
                keyValue.dataSegment.len = 8;
                keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
                if(keyValue.dataSegment.data)
                {
                    //5.复制实际数据
                    rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

                    uart2Object.taskList.AddToList(keyValue, NO);

                    //6.回收空间
                    rt_free(keyValue.dataSegment.data);
                    keyValue.dataSegment.data = RT_NULL;
                }
            }
        }
    }
}

static void DeviceCtrlSingle(device_t *device, u8 port, u8 state)
{
    u8          data[8];
    u16         value       = 0;
    KV          keyValue;
    seq_key_t   seq_key;

    if(RT_NULL == uart2Object.dev)
    {
        return;
    }

    //2.组合发送的数据
    value = 0x0000;

    GenerateVuleBySingleCtrl(*device, port, state, &value);

    //3.加入到发送列表
    GenerateSendData(*device, value, data);

    if(YES == IsCtrlChange(device->addr, value))
    {
        //4.0 判断是否是需要控制的
        //4.添加到taskList 之后会在统一接口中实现数据的发送
        seq_key.addr = device->addr;
        seq_key.regH = device->ctrl_addr >> 8;
        seq_key.regL = device->ctrl_addr;
        seq_key.regSize = device->storage_size;
        keyValue.key = SeqKeyToLong(seq_key);
        keyValue.dataSegment.len = 8;
        keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
        if(keyValue.dataSegment.data)
        {
            //5.复制实际数据
            rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

            uart2Object.taskList.AddToList(keyValue, NO);

            //6.回收空间
            rt_free(keyValue.dataSegment.data);
            keyValue.dataSegment.data = RT_NULL;
        }
    }
}

//询问device 寄存器 regAddr为寄存器地址
static void UartAsk(device_t *device, u16 regAddr)
{
    u8          data[8];
    KV          keyValue;
    seq_key_t   seq_key;
    //1.生成数据
    GenerateAskData(device, regAddr, data);

    //2.
    seq_key.addr = device->addr;
    seq_key.regH = regAddr >> 8;
    seq_key.regL = regAddr;
    seq_key.regSize = device->storage_size;

    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = 8;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //3.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        uart2Object.taskList.AddToList(keyValue, NO);

        //4.回收空间
        rt_free(keyValue.dataSegment.data);
    }
}

static void DeviceChgType(type_monitor_t *monitor, u16 id, u8 type)
{
    u8          addr        = 0;
    u8          port        = 0;
    u8          data[8];
    u16         reg         = 0;
    KV          keyValue;
    seq_key_t   seq_key;
    device_t    *device     = RT_NULL;

    //1 判断是哪个device的端口
    if(id > 0xFF)
    {
        addr = id >> 8;
        port = id;

        device = GetDeviceByAddr(monitor, addr);
        if(RT_NULL == device)
        {
            return;
        }
    }
    else
    {
        return;
    }
    //2 生成数据
    GenerateChangeType(device, port, type, data);
    //2.
    GetReadRegAddrByType(device->type, &reg);
    seq_key.addr = device->addr;
    seq_key.regH = reg >> 8;
    seq_key.regL = reg;
    seq_key.regSize = port;
    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = 8;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //3.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        uart2Object.taskList.AddToList(keyValue, NO);

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

    if(YES == IsNeedSendCtrToConnect(monitor->device[i].addr, &lastCtrl))
    {
//        LOG_E(".................KeepConnect, addr = %x",monitor->device[i].addr);
        //1.如果是在线 则发送前一次的数据保持连续

//            if((CON_SUCCESS == monitor->device[i].conn_state) ||
//               (CON_NULL == monitor->device[i].conn_state))

        {
            GenerateSendData(monitor->device[i], lastCtrl, data);
            if(RT_EOK == GenerateKVData(&keyValue, monitor->device[i], data, 8))
            {
                uart2Object.taskList.AddToList(keyValue, NO);

                //3.回收空间
                rt_free(keyValue.dataSegment.data);
            }
        }
//            else if(CON_FAIL == monitor->device[i].conn_state)//要考虑刚开始的时候全部都是失联的情况
//            {
//
//            }
    }

    if((i + 1) < monitor->device_size)
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
    device_t    *device;

    device = GetDeviceByAddr(GetMonitor(), data[0]);

    if((REGISTER_CODE == data[0]) || (device))
    {
        //1.判断是否是已经注册的设备
        if(device)
        {
            key.addr = data[0];
            if(WRITE_SINGLE == data[1])
            {
                key.regH = data[2];
                key.regL = data[3];
            }
            else if(READ_MUTI == data[1])
            {
                u16 reg;
                GetReadRegAddrByType(device->type, &reg);

                key.regH = reg >> 8;
                key.regL = reg;
            }
            key.regSize = device->storage_size;

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
            uart2Object.recvList.AddToList(keyValue, NO);
            //4.回收空间
            rt_free(keyValue.dataSegment.data);
            keyValue.dataSegment.data = RT_NULL;
        }
    }
}

//在该函数中做发送优化,即如果和上次发送的数据相同的，
//那么直接移除不做发送,如果是状态不一样的话那么马上响应,但是如果状态一样的话也要在10秒之内发送,否则device会将状态置零
/**
 * 注意: 头节点为空节点，需要从头节点的next节点开始处理
 */
static void SendCmd(void)
{
    Node        *first;
    device_t    *device = RT_NULL;

    first = uart2Object.taskList.list.next;

    if(RT_NULL == first)
    {
        return;
    }

    //3.将任务列表的任务发送出去
    rt_device_write(*uart2Object.dev,
                    0,
                    first->keyData.dataSegment.data,
                    first->keyData.dataSegment.len);

    //4.标记已经发送
    device = GetDeviceByAddr(GetMonitor(), LongToSeqKey(first->keyData.key).addr);
    if(device)
    {
        u16 regAddr = (first->keyData.dataSegment.data[2] << 8) | first->keyData.dataSegment.data[3];

        //添加到记录控制内容数组
        if((WRITE_SINGLE == first->keyData.dataSegment.data[1]) &&
           (regAddr == device->ctrl_addr))
        {
            u16 value = (first->keyData.dataSegment.data[4] << 8) | first->keyData.dataSegment.data[5];

            AddLastCtrl(LongToSeqKey(first->keyData.key).addr, value);
            SignDeviceSendFlag(LongToSeqKey(first->keyData.key).addr);
        }
    }

//    rt_kprintf("sendCmd : ");
//    for(int i = 0; i < first->keyData.dataSegment.len; i++)
//    {
//        rt_kprintf(" %x",first->keyData.dataSegment.data[i]);
//
//    }
//    rt_kprintf("\r\n");

    //5.将这个任务从任务列表中移出去
    uart2Object.taskList.DeleteToList(first->keyData);
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
    for(i = 0; i < monitor->device_size; i++)
    {
        if(uuid == monitor->device[i].uuid)
        {
            buffer[2] = monitor->device[i].uuid >> 24;
            buffer[3] = monitor->device[i].uuid >> 16;
            buffer[4] = monitor->device[i].uuid >> 8;
            buffer[5] = monitor->device[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->device[i].addr;
            buffer[8] = monitor->device[i].type;
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

        uart2Object.taskList.AddToList(keyValue, NO);

        rt_free(keyValue.dataSegment.data);
        keyValue.dataSegment.data = RT_NULL;
    }
}

//处理返回数据的列表,解析数据
static void RecvListHandle(void)
{
    Node        *tail = &uart2Object.recvList.list;
    device_t    *device;
    type_monitor_t *monitor = GetMonitor();
    u32         uuid;
    u8          addr;
    static u8 connect[DEVICE_MAX];

//    PrintNode(tail);

    while(tail)
    {
        if(REGISTER_CODE == tail->keyData.key)
        {
            //响应注册事件
            //1.先查看这个uuid + addr是否存在
            uuid = (tail->keyData.dataSegment.data[9] << 24) | (tail->keyData.dataSegment.data[10] << 16) |
                    (tail->keyData.dataSegment.data[11] << 8) | tail->keyData.dataSegment.data[12];

            if(RT_EOK == CheckDeviceExist(monitor, uuid))
            {
                //2.如果已经注册了忽略,否则删除之后重新注册
                if(RT_ERROR == CheckDeviceCorrect(monitor, uuid, tail->keyData.dataSegment.data[7], tail->keyData.dataSegment.data[8]))
                {
                    //3.删除原来已经注册的
                    DeleteModule(monitor, uuid);
                    //4.通过type 设置对应的默认值
                    addr = getAllocateAddress(GetMonitor());
                    rt_err_t ret = SetDeviceDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                    //5.发送重新分配的地址给模块
                    if(RT_EOK == ret)
                    {
                        SendReplyRegister(uuid, addr);
                        rt_kprintf("-----------RecvListHandle, uuid %x %x %x %x register again\r\n",
                                tail->keyData.dataSegment.data[9],tail->keyData.dataSegment.data[10],
                                tail->keyData.dataSegment.data[11],tail->keyData.dataSegment.data[12]);
                    }
                    else if(RT_ERROR == ret)
                    {
                        monitor->allocateStr.address[addr] = 0;
                    }
                }
                else
                {
                    for(int i = 0; i < monitor->device_size; i++)
                    {
                        if(uuid == monitor->device[i].uuid)
                        {
                            SendReplyRegister(uuid, monitor->device[i].addr);
                            rt_kprintf("device has exist, send addr = %d\r\n",monitor->device[i].addr);
                            break;
                        }
                    }
                }
            }
            else
            {
                //6.之前没有注册过的直接注册
                addr = getAllocateAddress(GetMonitor());
                rt_err_t ret = SetDeviceDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                if(RT_EOK == ret)
                {
                    //7.发送重新分配的地址
                    SendReplyRegister(uuid, addr);
                    rt_kprintf("----------RecvListHandle, register addr = %d, uuid %x %x %x %x\r\n",
                            addr,
                            tail->keyData.dataSegment.data[9],tail->keyData.dataSegment.data[10],
                            tail->keyData.dataSegment.data[11],tail->keyData.dataSegment.data[12]);
                }
                else if(RT_ERROR == ret)
                {
                    monitor->allocateStr.address[addr] = 0;
                }
            }
        }
        else
        {
            device = GetDeviceByAddr(monitor, LongToSeqKey(tail->keyData.key).addr);
            if(device)
            {
                u8 rwType = tail->keyData.dataSegment.data[1];
                u16 reg = (LongToSeqKey(tail->keyData.key).regH << 8) | (LongToSeqKey(tail->keyData.key).regL);

                SignDeviceRecvFlag(device->addr);

                //1.判断返回是否是读取功能
                if(WRITE_SINGLE == rwType)
                {
                    //判断是否是开关寄存器
                    if(reg == device->ctrl_addr)
                    {
                        int regSize = LongToSeqKey(tail->keyData.key).regSize;
                        //判断寄存器的数量
                        if(1 == regSize)
                        {
                            if(IR_AIR_TYPE == device->type)
                            {
                                if(tail->keyData.dataSegment.data[4] & 0x80)
                                {
                                    device->port[0].ctrl.d_state = ON;
                                }
                                else
                                {
                                    device->port[0].ctrl.d_state = OFF;
                                }
                            }
                            else
                            {
                                device->port[0].ctrl.d_state = tail->keyData.dataSegment.data[4];
                                device->port[0].ctrl.d_value = tail->keyData.dataSegment.data[5];
                            }
                        }
                        else
                        {
                            u16 value = (tail->keyData.dataSegment.data[4] << 8) | tail->keyData.dataSegment.data[5];

                            for(int port = 0; port < (regSize > device->storage_size ? device->storage_size : regSize); port++)
                            {
                                if((value >> port) & 0x0001)
                                {
                                    device->port[port].ctrl.d_state = ON;
                                }
                                else
                                {
                                    device->port[port].ctrl.d_state = OFF;
                                }
                            }
                        }
                    }
                    //还要判断其他寄存器，比如类型寄存器
                    else
                    {
                        u16 regAddr = 0;
                        //注意 接收时将regSize 赋值为端口
                        GetReadRegAddrByType(device->type, &regAddr);
                        for(int port = 0; port < device->storage_size; port++)
                        {
                            if(reg == (regAddr + port))
                            {
                                device->port[port].type = tail->keyData.dataSegment.data[5];
                                device->port[port].func = GetFuncByType(device->port[port].type);
                            }
                        }
                    }
                }
                else if(READ_MUTI == rwType)//接收读取数据
                {
                    //1.检测寄存器类型
                    u16 regA = 0;
                    GetReadRegAddrByType(device->type, &regA);
                    int size = tail->keyData.dataSegment.data[2];

                    if(size == device->storage_size * 2)//改位置有问题
                    {
                        for(int port = 0; port < device->storage_size; port++)
                        {
                            u8 type = tail->keyData.dataSegment.data[4 + port * 2];
                            device->port[port].type = type;
                            device->port[port].func = GetFuncByType(type);
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
    for(u8 i = 0; i < monitor->device_size; i++)
    {
        //1.已经发送数据了 但是数据接收超时判断为失联
        if(sendMoni[i].SendCnt > 2)
        {
            GetDeviceByAddr(monitor, sendMoni[i].addr)->conn_state = CON_FAIL;
        }
        else
        {
            GetDeviceByAddr(monitor, sendMoni[i].addr)->conn_state = CON_SUCCESS;
        }

        if(connect[i] != GetDeviceByAddr(monitor, sendMoni[i].addr)->conn_state)
        {
            connect[i] = GetDeviceByAddr(monitor, sendMoni[i].addr)->conn_state;

            if(CON_FAIL == connect[i])
            {
                LOG_E("addr = %d disconnect, sendTimes = %d", sendMoni[i].addr,sendMoni[i].SendCnt);
            }
            else if(CON_SUCCESS == connect[i])
            {
                LOG_W("-----------addr = %d reconnect, sendTimes = %d", sendMoni[i].addr,sendMoni[i].SendCnt);
            }
        }
    }
}

void InitUart2Object(void)
{
    //1.初始化记录发送情况
    rt_memset(sendMoni, 0, sizeof(uart_send_monitor) * DEVICE_MAX);

    //2.初始化相关数据
    uart2Object.dev = RT_NULL;
    uart2Object.recvList.list.next = RT_NULL;
    uart2Object.recvList.list.keyData.key = 0x00;
    uart2Object.taskList.list.next = RT_NULL;
    uart2Object.taskList.list.keyData.key = 0x00;

    //3.实例化相关接口
    uart2Object.ConfigureUart = ConfigureUart2;
    uart2Object.DeviceCtrl = DeviceCtrl;
    uart2Object.DeviceCtrlSingle = DeviceCtrlSingle;
    uart2Object.DeviceChgType = DeviceChgType;
    uart2Object.AskDevice = UartAsk;
    uart2Object.SendCmd = SendCmd;
    uart2Object.RecvCmd = RecvCmd;
    uart2Object.RecvListHandle = RecvListHandle;
    uart2Object.KeepConnect = KeepConnect;
    uart2Object.Optimization = Optimization;

    uart2Object.recvList.GetList = GetRecvList;       //获取核对关注列表
    uart2Object.recvList.AddToList = AddToRecvList;   //添加到关注列表中,数据接收的时候就
    uart2Object.recvList.DeleteToList = DeleteRecvList;
    uart2Object.recvList.CheckDataCorrect = CheckDataInRecvList;
    uart2Object.recvList.KeyHasExist = KeyHasExistInRecvList;

    uart2Object.taskList.GetList = GetTaskList;       //获取核对关注列表
    uart2Object.taskList.AddToList = AddToTaskList;   //添加到关注列表中,数据接收的时候就
    uart2Object.taskList.DeleteToList = DeleteToTaskList;
    uart2Object.taskList.CheckDataCorrect = CheckDataInTaskList;
    uart2Object.taskList.KeyHasExist = KeyHasExistInTaskList;
}

type_uart_class *GetDeviceObject(void)
{
    return &uart2Object;
}

void printSendMoni(void)
{
    for(int i = 0; i < GetMonitor()->device_size; i++)
    {
        LOG_W("addr = %x, last ctrl = %x",sendMoni[i].addr, sendMoni[i].ctrl);
    }
}

