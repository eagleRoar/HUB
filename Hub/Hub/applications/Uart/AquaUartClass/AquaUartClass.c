/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-06-27     Administrator       the first version
 */
#include "SeqList.h"
#include "UartEventType.h"
#include "Command.h"
#include "Module.h"
#include "AquaUartClass.h"
#include "UartDataLayer.h"
#include "InformationMonitor.h"

#if(HUB_SELECT == HUB_IRRIGSTION)
//设备类型
type_uart_class aquaObject;
static uart_send_aqua sendMoni[TANK_LIST_MAX]; //优化设备发送
static uart_cache_t sendCache[TANK_LIST_MAX];
static monitor_ask askState;
extern u8 saveAquaInfoFlag;

void setAskStateOK(void)
{
    askState.flag = NO;
    askState.askType = 0;
    askState.length = 0;
}

monitor_ask *GetAquaAskState(void)
{
    return &askState;
}

void InitAquaCache(void)
{
    for(int i = 0; i < TANK_LIST_MAX; i++)
    {
        sendCache[i].addr = 0;
        sendCache[i].txFlag = OFF;
        sendCache[i].cnt = 0;
        sendCache[i].TimeOut = 3;
        sendCache[i].info = RT_NULL;
        sendCache[i].recipe_no = TANK_LIST_MAX;
    }
}

void SetAquaSendCacheHaveSend(u8 addr, aqua_info_t *info, u8 recipe_no)
{
    int i = 0;

    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        if(sendCache[i].addr == addr)
        {
            break;
        }
    }

    if(i == TANK_LIST_MAX)
    {
        for(i = 0; i < TANK_LIST_MAX; i++)
        {
            if(0 == sendCache[i].addr)
            {
                break;
            }
        }
    }

    if(i < TANK_LIST_MAX)
    {
        sendCache[i].addr = addr;
        sendCache[i].txFlag = YES;
        sendCache[i].info = info;
        sendCache[i].recipe_no = recipe_no;
    }
}

void SetAquaSendCacheRecv(u8 addr)
{
    int i = 0;

    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        if(addr == sendCache[i].addr)
        {
            sendCache[i].addr = 0;
            sendCache[i].txFlag = NO;
            sendCache[i].cnt = 0;
            sendCache[i].info = RT_NULL;
            sendCache[i].recipe_no = 0;
            break;
        }
    }
}

uart_cache_t *GetAquaSendCache(u8 index)
{
    if(index > 0 && index < TANK_LIST_MAX)
    {
        return &sendCache[index];
    }

    return RT_NULL;
}

static void GenerateVuleByCtrl(aqua_t *aqua, u8 state, u8 recipe_i, u16 *res)
{
    *res= state;
    *(res + 1) = recipe_i;
}

//生成发送的数据
static void GenerateSendMonitor(aqua_t *aqua, u16 *ctrl, u8 *data)
{
    rt_memset(data, 0, 11);

    data[0] = aqua->addr;
    data[1] = WRITE_MUTI;
    data[2] = AQUA_MONITOR_ADDR >> 8;
    data[3] = AQUA_MONITOR_ADDR;
    data[4] = 0x00;
    data[5] = 1;
    data[6] = 2;
    data[7] = *ctrl >> 8;
    data[8] = *ctrl;
    data[9] = usModbusRTU_CRC(data, 9);
    data[10] = usModbusRTU_CRC(data, 9) >> 8;
}

//生成发送的数据
static void GenerateSendData(aqua_t *aqua, u16 *ctrl, u8 *data)
{
    rt_memset(data, 0, 15);

    data[0] = aqua->addr;
    data[1] = WRITE_MUTI;
    data[2] = AQUA_WORK_ADDR >> 8;
    data[3] = AQUA_WORK_ADDR;
    data[4] = 0x00;
    data[5] = 2;
    data[6] = 4;
    data[7] = ctrl[0] >> 8;
    data[8] = ctrl[0];
    data[9] = ctrl[1] >> 8;
    data[10] = ctrl[1];
    data[11] = usModbusRTU_CRC(data, 11);
    data[12] = usModbusRTU_CRC(data, 11) >> 8;
}

void changeBigToLittle(u16 src, u8 *data)
{
    *data = src >> 8;
    *(data+1) = src;
}

//生成发送aqua 配方的数据
static void GenerateSendAqueRecipeData(u8 addr,aqua_info_t *info, u8 recipe_no, u8 *data, u8 data_len)
{
    //data 长度要大于74 + 9
    rt_memset(data, 0, data_len);
    if(recipe_no >= AQUA_RECIPE_MX)
    {
        return;
    }

    if(data_len >= 83)
    {
        data[0] = addr;
        data[1] = WRITE_MUTI;
        data[2] = AQUA_RECIPE_ADDR >> 8;
        data[3] = AQUA_RECIPE_ADDR;
        data[4] = 0x00;
        data[5] = 37;//连续写31个寄存器
        data[6] = 74;
        data[7] = 0x00;
        data[8] = recipe_no + 1;
        u8 index = 9;
        changeBigToLittle(info->list[recipe_no].ecTarget, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].ecDeadband, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].ecLow, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].ecHigh, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].ecDosingTime, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].ecMixingTime, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].ecMaxDosingCycles, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phTarget, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phDeadband, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phLow, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phHigh, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phDosingTime, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phMixingTime, &data[index]);
        index += 2;
        changeBigToLittle(info->list[recipe_no].phMaxDosingCycles, &data[index]);
        index += 2;
        u16 en = 0x00;
        for(int i = 0; i < AQUA_RECIPE_PUMP_MX; i++)
        {
            if(YES == info->list[recipe_no].pumpList[i].state)
            {
                en |= (1 << i);
            }
        }
        changeBigToLittle(en, &data[index]);
        index += 2;
        for(int i = 0; i < AQUA_RECIPE_PUMP_MX; i++)
        {
            changeBigToLittle(info->list[recipe_no].pumpList[i].type, &data[index]);
            index += 2;
        }
        for(int i = 0; i < AQUA_RECIPE_PUMP_MX; i++)
        {
            changeBigToLittle(info->list[recipe_no].pumpList[i].ratio, &data[index]);
            index += 2;
        }

        rt_memcpy(&data[index], (u8 *)&info->list[recipe_no].formName[0], 2);
        index += 2;
        rt_memcpy(&data[index], (u8 *)&info->list[recipe_no].formName[2], 2);
        index += 2;
        rt_memcpy(&data[index], (u8 *)&info->list[recipe_no].formName[4], 2);
        index += 2;
        rt_memcpy(&data[index], (u8 *)&info->list[recipe_no].formName[6], 2);
        index += 2;
        rt_memcpy(&data[index], (u8 *)&info->list[recipe_no].formName[8], 2);
        index += 2;

        data[index] = usModbusRTU_CRC(data, index);
        data[index + 1] = usModbusRTU_CRC(data, index) >> 8;
    }
}

static void GenerateAskStateData(aqua_t *aqua, u16 reg, u8 *data, u8 regLen)
{
    rt_memset(data, 0, 8);

    data[0] = aqua->addr;
    data[1] = READ_MUTI;
    data[2] = reg >> 8;
    data[3] = reg;
    data[4] = 0x00;
    data[5] = regLen;
    data[6] = usModbusRTU_CRC(data, 6);
    data[7] = usModbusRTU_CRC(data, 6) >> 8;
}

//生成可以加载到keyValue 的数据
static rt_err_t GenerateKVData(KV *kv, aqua_t aqua, u8 *data, u8 len)
{
    seq_key_t   seq_key;

    seq_key.addr = aqua.addr;
    seq_key.regH = aqua.ctrl_addr >> 8;
    seq_key.regL = aqua.ctrl_addr;
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


/**
 * Determine whether to send data
 *
 * @return
 */
static u8 IsCtrlChange(u8 addr, u16 ctrl, u8 recipe_i)
{
    u8              i           = 0;
    u8              ret         = NO;

    //2.如果控制内容发送变化
    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            //控制内容发生变化
            if(ctrl != sendMoni[i].ctrl ||
               recipe_i != sendMoni[i].recipe_i)
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

    if(i == TANK_LIST_MAX)
    {
        ret = YES;
    }

    return ret;
}

static void SignAquaSendFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < TANK_LIST_MAX; i++)
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

static void SignAquaRecvFlag(u8 addr)
{
    u8              i           = 0;

    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].SendCnt = 0;
            return;
        }
    }
}

static void AddLastCtrl(u8 addr, u16 ctrl, u8 recipe_i)
{
    u8              i           = 0;

    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        if(addr == sendMoni[i].addr)
        {
            sendMoni[i].ctrl = ctrl;
            sendMoni[i].recipe_i = recipe_i;
            break;
        }
    }

    if(i == TANK_LIST_MAX)
    {
        for(i = 0; i < TANK_LIST_MAX; i++)
        {
            if(0x00 == sendMoni[i].addr)
            {
                sendMoni[i].addr = addr;
                sendMoni[i].ctrl = ctrl;
                sendMoni[i].recipe_i = recipe_i;
                break;
            }
        }
    }
}

static u8 IsNeedSendCtrToConnect(u8 addr, u16 *ctrl)
{
    u8 ret = NO;

    //2.如果控制内容发送变化
    for(u8 i = 0; i < GetMonitor()->aqua_size; i++)
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
                *(ctrl+1) = sendMoni[i].recipe_i;
            }
        }
    }

    return ret;
}

static void Optimization(type_monitor_t *monitor)
{
    u8 i = 0;

    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        //1.如果已经被删除了剔除队列
        if(0x00 != sendMoni[i].addr)
        {
            if(RT_NULL == GetAquaByAddr(monitor, sendMoni[i].addr))
            {
                rt_memset(&sendMoni[i], 0, sizeof(uart_send_aqua));
            }
        }
    }

    for(i = 0; i < monitor->aqua_size; i++)
    {
        u8 j = 0;
        for(j = 0; j < TANK_LIST_MAX; j++)
        {
            if(sendMoni[j].addr == monitor->aqua[i].addr)
            {
                break;
            }
        }

        if(j == TANK_LIST_MAX)
        {
            for(j = 0; j < TANK_LIST_MAX; j++)
            {
                if(0x00 == sendMoni[j].addr)
                {
                    sendMoni[j].addr = monitor->aqua[i].addr;
                    sendMoni[j].sendTime = 0;
                    sendMoni[j].ctrl = 0;

                    break;
                }
            }
        }
    }

    //重发机制
    for(i = 0; i < TANK_LIST_MAX; i++)
    {
        if(YES == sendCache[i].txFlag)
        {
            if(sendCache[i].cnt < sendCache[i].TimeOut)
            {
                sendCache[i].cnt++;
            }
            else
            {
                aqua_t *aqua = GetAquaByAddr(monitor, sendCache[i].addr);
                if(aqua)
                {
                    if((RT_NULL != sendCache[i].info) && (sendCache[i].recipe_no < TANK_LIST_MAX))
                    {
                        aquaObject.aquaSendInfo(aqua, sendCache[i].info, sendCache[i].recipe_no);
                        LOG_W("--------------------send cache again ok");
                        sendCache[i].cnt = 0;
                    }
                }
                else
                {
                    LOG_W("Optimization  aqua is NULL -----------------------");
                }
            }
        }
    }
}
/****************Check List START***************************************************************/
static Node *GetRecvList (void)
{
    return &aquaObject.recvList.list;
}

//加入到关注列表
static void AddToRecvList(KV keyData, u8 keyUnique)
{
    CreatTail(&aquaObject.recvList.list, keyData, keyUnique);
}

static void DeleteRecvList(KV keyData)
{
    if(YES == KeyExist(&aquaObject.recvList.list, keyData))
    {
        DeleteNode(&aquaObject.recvList.list, keyData);
    }
}

static u8 KeyHasExistInRecvList(KV keyData)
{
    return KeyExist(&aquaObject.recvList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInRecvList(KV keyData)
{
    if(YES == KeyExist(&aquaObject.recvList.list, keyData))
    {
        if(YES == DataCorrect(&aquaObject.recvList.list, keyData))
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
    return &aquaObject.taskList.list;
}

//加入到关注列表
static void AddToTaskList(KV keyData, u8 keyUnique)
{
    CreatTail(&aquaObject.taskList.list, keyData, keyUnique);
}

static void DeleteToTaskList(KV keyData)
{
    if(YES == KeyExist(&aquaObject.taskList.list, keyData))
    {
        DeleteNode(&aquaObject.taskList.list, keyData);
    }
}

static u8 KeyHasExistInTaskList(KV keyData)
{
    return KeyExist(&aquaObject.taskList.list, keyData);
}

//检查数据是否存在关注列表中
static u8 CheckDataInTaskList(KV keyData)
{
    if(YES == KeyExist(&aquaObject.taskList.list, keyData))
    {
        if(YES == DataCorrect(&aquaObject.taskList.list, keyData))
        {
            return YES;
        }
    }

    return NO;
}

/****************Task List END******************************************************************/

static void ConfigureUart3(rt_device_t *dev)
{
    aquaObject.dev = dev;
}

/**传入参数
 *
 * @param addr      设备485地址
 *        reg_addr  寄存器地址
 *        reg_data  寄存器实际值
 *        reg_size  控制的寄存器值的数量
 */
static void AquaSendMonitor(aqua_t *aqua, u8 monitor)
{
    u8          data[11];
    KV          keyValue;
    seq_key_t   seq_key;

    if(RT_NULL == aquaObject.dev)
    {
        return;
    }

    //3.加入到发送列表
    u16 ctrl = monitor;
    GenerateSendMonitor(aqua, &ctrl, data);

    {
        //4.添加到taskList 之后会在统一接口中实现数据的发送
        seq_key.addr = aqua->addr;
        seq_key.regH = AQUA_MONITOR_ADDR >> 8;
        seq_key.regL = AQUA_MONITOR_ADDR;
        seq_key.regSize = 1;
        keyValue.key = SeqKeyToLong(seq_key);
        keyValue.dataSegment.len = 11;
        keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
        if(keyValue.dataSegment.data)
        {
            //5.复制实际数据
            rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

            aquaObject.taskList.AddToList(keyValue, NO);

            //6.回收空间
            rt_free(keyValue.dataSegment.data);
        }

    }
}


/**传入参数
 *
 * @param addr      设备485地址
 *        reg_addr  寄存器地址
 *        reg_data  寄存器实际值
 *        reg_size  控制的寄存器值的数量
 */
static void aquaCtrl(aqua_t *aqua, u8 state, u8 recipe_i)
{
    u8          data[13];
    KV          keyValue;
    seq_key_t   seq_key;
    u8          maintain = GetSysSet()->sysPara.maintain;

    if(RT_NULL == aquaObject.dev)
    {
        return;
    }

    //3.加入到发送列表
    u16 ctrl[2] = {0,0};
    if(YES == maintain)
    {
        state = OFF;
    }
    GenerateVuleByCtrl(aqua, state, recipe_i, ctrl);

    GenerateSendData(aqua, ctrl, data);
    if(YES == IsCtrlChange(aqua->addr, state, recipe_i))
    {
        //4.添加到taskList 之后会在统一接口中实现数据的发送
        seq_key.addr = aqua->addr;
        seq_key.regH = aqua->ctrl_addr >> 8;
        seq_key.regL = aqua->ctrl_addr;
        seq_key.regSize = 1;
        keyValue.key = SeqKeyToLong(seq_key);
        keyValue.dataSegment.len = 13;
        keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
        if(keyValue.dataSegment.data)
        {
            //5.复制实际数据
            rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

            aquaObject.taskList.AddToList(keyValue, NO);

            //6.回收空间
            rt_free(keyValue.dataSegment.data);
        }

    }
}

static void aquaSendInfo(aqua_t *aqua, aqua_info_t *info, u8 recipe_no)
{
    u8          data[83];
    KV          keyValue;
    seq_key_t   seq_key;

    if(RT_NULL == aquaObject.dev)
    {
        return;
    }

    //1.生成要发送的数据
    GenerateSendAqueRecipeData(aqua->addr, info, recipe_no, data, sizeof(data));

    //添加到数据缓存
    SetAquaSendCacheHaveSend(aqua->addr, info, recipe_no);

    //2.添加到taskList 之后会在统一接口中实现数据的发送
    seq_key.addr = aqua->addr;
    seq_key.regH = AQUA_RECIPE_ADDR >> 8;
    seq_key.regL = AQUA_RECIPE_ADDR;
    seq_key.regSize = 1;
    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = sizeof(data);
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //5.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        aquaObject.taskList.AddToList(keyValue, NO);

        //6.回收空间
        rt_free(keyValue.dataSegment.data);
    }

}

void AskAquaState(aqua_t *aqua, int index)
{
    u8          data[8];
    KV          keyValue;
    seq_key_t   seq_key;
    u8          dataLen = 0;
    u8          regLen = 0;
    static u8   askList[TANK_LIST_MAX] = {0,0,0,0};//数据最大是（1 + AQUA_RECIPE_MX ）-1

    if(RT_NULL == aquaObject.dev)
    {
        return;
    }

    //只有接收数据了才能往下循环
    if(NO == askState.flag)
    {
        if(askList[index] < (3 + AQUA_RECIPE_MX) - 1)
        {
            askList[index]++;
        }
        else
        {
            askList[index] = 0;
        }
    }

    switch (askList[index])
    {
        case 0:
            //1.生成要发送的数据
            regLen = 8;
            GenerateAskStateData(aqua, AQUA_PUMPMAX_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_STATE;
            askState.length = regLen * 2;
            break;
        case 1:
            //1.生成要发送的数据
            regLen = 3;
            GenerateAskStateData(aqua, AQUA_RUNNING_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_VER;
            askState.length = regLen * 2;
            break;
        case 2:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE0_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_0;
            askState.length = regLen * 2;
            break;
        case 3:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE1_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_1;
            askState.length = regLen * 2;
            break;
        case 4:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE2_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_2;
            askState.length = regLen * 2;
            break;
        case 5:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE3_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_3;
            askState.length = regLen * 2;
            break;
        case 6:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE4_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_4;
            askState.length = regLen * 2;
            break;
        case 7:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE5_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_5;
            askState.length = regLen * 2;
            break;
        case 8:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE6_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_6;
            askState.length = regLen * 2;
            break;
        case 9:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE7_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_7;
            askState.length = regLen * 2;
            break;
        case 10:
            //1.生成要发送的数据
            regLen = 36;
            GenerateAskStateData(aqua, AQUA_RECIPE8_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_RECIPE_8;
            askState.length = regLen * 2;
            break;
        case 11:
            //1.生成要发送的数据
            regLen = 1;
            GenerateAskStateData(aqua, AQUA_MONITOR_ADDR, data, regLen);
            dataLen = 8;
            askState.askType = ASK_MONITOR;
            askState.length = regLen * 2;
            break;
        default:
            break;
    }
    askState.flag = YES;

    //2.添加到taskList 之后会在统一接口中实现数据的发送
    seq_key.addr = aqua->addr;
    seq_key.regH = AQUA_EC_ADDR >> 8;
    seq_key.regL = AQUA_EC_ADDR;
    seq_key.regSize = 1;
    keyValue.key = SeqKeyToLong(seq_key);
    keyValue.dataSegment.len = dataLen;
    keyValue.dataSegment.data = rt_malloc(keyValue.dataSegment.len);
    if(keyValue.dataSegment.data)
    {
        //5.复制实际数据
        rt_memcpy(keyValue.dataSegment.data, data, keyValue.dataSegment.len);

        aquaObject.taskList.AddToList(keyValue, NO);

        //6.回收空间
        rt_free(keyValue.dataSegment.data);
    }
}

//发送数据保存底下终端使能
static void KeepConnect(type_monitor_t *monitor)
{
    KV          keyValue;
    u16         lastCtrl[2];
    u8          data[17];
    static u8   i           = 0;

    if(YES == IsNeedSendCtrToConnect(monitor->aqua[i].addr, lastCtrl))
    {
        //1.如果是在线 则发送前一次的数据保持连续
        GenerateSendData(&monitor->aqua[i], lastCtrl, data);
        if(RT_EOK == GenerateKVData(&keyValue, monitor->aqua[i], data, 13))
        {
            aquaObject.taskList.AddToList(keyValue, NO);

            //3.回收空间
            rt_free(keyValue.dataSegment.data);
        }
    }

    if((i + 1) < monitor->aqua_size)
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
    aqua_t      *aqua;

    aqua = GetAquaByAddr(GetMonitor(), data[0]);

    if((REGISTER_CODE == data[0]) || (aqua))
    {
        //1.判断是否是已经注册的设备
        if(aqua)
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
            aquaObject.recvList.AddToList(keyValue, NO);
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
    aqua_t      *aqua   = RT_NULL;

    first = aquaObject.taskList.list.next;

    if(RT_NULL == first)
    {
        return;
    }

    //3.将任务列表的任务发送出去
    rt_device_write(*aquaObject.dev,
                    0,
                    first->keyData.dataSegment.data,
                    first->keyData.dataSegment.len);

    //4.标记已经发送
    aqua = GetAquaByAddr(GetMonitor(), LongToSeqKey(first->keyData.key).addr);
    if(aqua)
    {
        //添加到记录控制内容数组
        if(WRITE_SINGLE == first->keyData.dataSegment.data[1])
        {
            SignAquaSendFlag(LongToSeqKey(first->keyData.key).addr);
        }
        else if(WRITE_MUTI == first->keyData.dataSegment.data[1])
        {
            u16 reg = LongToSeqKey(first->keyData.key).regH << 8 |
                    LongToSeqKey(first->keyData.key).regL;
            if(aqua->ctrl_addr == reg)
            {
                u16 value = (first->keyData.dataSegment.data[7] << 8) | first->keyData.dataSegment.data[8];
                u16 recipe_i = (first->keyData.dataSegment.data[9] << 8) | first->keyData.dataSegment.data[10];

                AddLastCtrl(LongToSeqKey(first->keyData.key).addr, value, recipe_i);
                SignAquaSendFlag(LongToSeqKey(first->keyData.key).addr);
            }

        }
    }

    LOG_I("aqua sendCmd : ");
    for(int i = 0; i < first->keyData.dataSegment.len; i++)
    {
        rt_kprintf(" %02x",first->keyData.dataSegment.data[i]);

    }
    rt_kprintf("\r\n");//Justin

    //5.将这个任务从任务列表中移出去
    aquaObject.taskList.DeleteToList(first->keyData);
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
    for(i = 0; i < monitor->aqua_size; i++)
    {
        if(uuid == monitor->aqua[i].uuid)
        {
            buffer[2] = monitor->aqua[i].uuid >> 24;
            buffer[3] = monitor->aqua[i].uuid >> 16;
            buffer[4] = monitor->aqua[i].uuid >> 8;
            buffer[5] = monitor->aqua[i].uuid;
            buffer[6] = 0x06;
            buffer[7] = monitor->aqua[i].addr;
            buffer[8] = monitor->aqua[i].type;
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

        aquaObject.taskList.AddToList(keyValue, NO);

        rt_free(keyValue.dataSegment.data);
    }
}

//处理返回数据的列表,解析数据
static void RecvListHandle(void)
{
    Node        *tail = &aquaObject.recvList.list;
    aqua_t      *aqua;
    type_monitor_t *monitor = GetMonitor();
    u32         uuid;
    u8          addr;
    static u8 connect[TANK_LIST_MAX];

    while(tail)
    {
        if(REGISTER_CODE == tail->keyData.key)
        {
            //响应注册事件
            //1.先查看这个uuid + addr是否存在
            uuid = (tail->keyData.dataSegment.data[9] << 24) | (tail->keyData.dataSegment.data[10] << 16) |
                    (tail->keyData.dataSegment.data[11] << 8) | tail->keyData.dataSegment.data[12];

            if(RT_EOK == CheckAquaExist(monitor, uuid))
            {
                //2.如果已经注册了忽略,否则删除之后重新注册
                if(RT_ERROR == CheckAquaCorrect(monitor, uuid, tail->keyData.dataSegment.data[7]))
                {
                    //3.删除原来已经注册的
                    DeleteModule(monitor, uuid);
                    //4.通过type 设置对应的默认值
                    addr = getAllocateAddress(GetMonitor());
                    rt_err_t ret = SetAquaDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                    LOG_E("--------------------------------- RecvListHandle 1");
                    if(RT_EOK == ret)
                    {
                        //5.发送重新分配的地址给模块
                        SendReplyRegister(uuid, addr);
                        LOG_I("-----------RecvListHandle, register again");
                    }
                    else if(RT_ERROR == ret)
                    {
                        monitor->allocateStr.address[addr] = 0;
                    }
                }
                else
                {
                    for(int i = 0; i < monitor->aqua_size; i++)
                    {
                        if(uuid == monitor->aqua[i].uuid)
                        {
                            SendReplyRegister(uuid, monitor->aqua[i].addr);
                            rt_kprintf("----------aqua has exist, send addr = %d\r\n",monitor->aqua[i].addr);
                            break;
                        }
                    }
                }
            }
            else
            {
                //7.之前没有注册过的直接注册
                addr = getAllocateAddress(GetMonitor());
                rt_err_t ret = SetAquaDefault(monitor, uuid, tail->keyData.dataSegment.data[8], addr);
                LOG_E("--------------------------------- RecvListHandle 2, ret = %d",ret);
                if(RT_EOK == ret)
                {
                    //8.发送重新分配的地址
                    SendReplyRegister(uuid, addr);
                    addNewAquaSetAndInfo(uuid);
                    saveAquaInfoFlag = YES;
                    LOG_I("----------RecvListHandle, register ");
                }
                else if(RT_ERROR == ret)
                {
                    monitor->allocateStr.address[addr] = 0;
                }
            }
        }
        else
        {
            aqua = GetAquaByAddr(monitor, LongToSeqKey(tail->keyData.key).addr);
            if(aqua)
            {
                u8 rwType = tail->keyData.dataSegment.data[1];
                u16 reg = (LongToSeqKey(tail->keyData.key).regH << 8) | (LongToSeqKey(tail->keyData.key).regL);

                SignAquaRecvFlag(aqua->addr);

                //1.判断返回是否是读取功能
                if(WRITE_SINGLE == rwType)
                {
                    //判断是否是开关寄存器
                    if((reg >= aqua->ctrl_addr) && (reg < aqua->ctrl_addr + aqua->storage_size))
                    {
                        aqua->ctrl.d_state = tail->keyData.dataSegment.data[4];
                        aqua->ctrl.d_value = tail->keyData.dataSegment.data[5];
                    }
                }
                else if(WRITE_MUTI == rwType)
                {
                    u16 reg = tail->keyData.dataSegment.data[2] << 8 | tail->keyData.dataSegment.data[3];
                    if(AQUA_RECIPE_ADDR == reg)
                    {
                        SetAquaSendCacheRecv(aqua->addr);
                    }
                }
                else if(READ_MUTI == rwType)
                {
                    aqua_state_t newState;
                    aqua_state_t *state;
                    monitor_ask *ask = GetAquaAskState();
                    aqua_info_t *info = RT_NULL;
                    aqua_recipe *recipe = RT_NULL;
                    if(YES == ask->flag)
                    {
                        if(tail->keyData.dataSegment.len > 5)
                        {
                            if(ask->length == (tail->keyData.dataSegment.len - 5))//扣除地址 类型 长度 两个crc位
                            {

                                info = GetAquaInfoByUUID(aqua->uuid);

                                if(info)
                                {
                                    switch (ask->askType) {
                                        case ASK_STATE:

                                            if(RT_NULL == GetAquaWarnById(tail->keyData.dataSegment.data[0]))
                                            {
                                                state = &newState;
                                            }
                                            else
                                            {
                                                state = GetAquaWarnById(tail->keyData.dataSegment.data[0]);
                                            }
                                            state->id = tail->keyData.dataSegment.data[0];
                                            state->pumpSize = (tail->keyData.dataSegment.data[3] << 8) | tail->keyData.dataSegment.data[4];
                                            state->ec = (tail->keyData.dataSegment.data[5] << 8) | tail->keyData.dataSegment.data[6];
                                            state->ph = (tail->keyData.dataSegment.data[7] << 8) | tail->keyData.dataSegment.data[8];
                                            state->wt = (tail->keyData.dataSegment.data[9] << 8) | tail->keyData.dataSegment.data[10];
                                            state->pumpState = (tail->keyData.dataSegment.data[11] << 8) | tail->keyData.dataSegment.data[12];
                                            state->warn = (tail->keyData.dataSegment.data[13] << 8) | tail->keyData.dataSegment.data[14];
                                            state->work = (tail->keyData.dataSegment.data[15] << 8) | tail->keyData.dataSegment.data[16];

                                            aqua->pumpSize = state->pumpSize;

                                            if(RT_NULL == GetAquaWarnById(tail->keyData.dataSegment.data[0]))
                                            {
                                                SetAquaWarn(state);
                                            }
                                            break;
                                        case ASK_VER:

                                            if(RT_NULL == GetAquaWarnById(tail->keyData.dataSegment.data[0]))
                                            {
                                                state = &newState;
                                            }
                                            else
                                            {
                                                state = GetAquaWarnById(tail->keyData.dataSegment.data[0]);
                                            }

                                            state->id = tail->keyData.dataSegment.data[0];
                                            state->isAquaRunning = (tail->keyData.dataSegment.data[3] << 8) | tail->keyData.dataSegment.data[4];
                                            state->aqua_hmi_ver = (tail->keyData.dataSegment.data[5] << 8) | tail->keyData.dataSegment.data[6];
                                            state->aqua_firm_ver = (tail->keyData.dataSegment.data[7] << 8) | tail->keyData.dataSegment.data[8];

                                            if(RT_NULL == GetAquaWarnById(tail->keyData.dataSegment.data[0]))
                                            {
                                                SetAquaWarn(state);
                                            }

                                            break;
                                        case ASK_RECIPE_0:
                                            recipe = &info->list[0];
                                            break;
                                        case ASK_RECIPE_1:
                                            recipe = &info->list[1];
                                            break;
                                        case ASK_RECIPE_2:
                                            recipe = &info->list[2];
                                            break;
                                        case ASK_RECIPE_3:
                                            recipe = &info->list[3];
                                            break;
                                        case ASK_RECIPE_4:
                                            recipe = &info->list[4];
                                            break;
                                        case ASK_RECIPE_5:
                                            recipe = &info->list[5];
                                            break;
                                        case ASK_RECIPE_6:
                                            recipe = &info->list[6];
                                            break;
                                        case ASK_RECIPE_7:
                                            recipe = &info->list[7];
                                            break;
                                        case ASK_RECIPE_8:
                                            recipe = &info->list[8];
                                            break;
                                        case ASK_MONITOR:
                                            //同步monitor状态
                                            if(RT_NULL != GetAquaSetByUUID(aqua->uuid))
                                            {
                                                GetAquaSetByUUID(aqua->uuid)->monitor = (tail->keyData.dataSegment.data[3] << 8) | tail->keyData.dataSegment.data[4];

                                            }
                                            break;
                                        default:
                                            break;
                                    }

                                    if(ask->askType >= ASK_RECIPE_0 && ask->askType <= ASK_RECIPE_8)
                                    {
                                        recipe->ecTarget = tail->keyData.dataSegment.data[3] << 8 | tail->keyData.dataSegment.data[4];
                                        recipe->ecDeadband = tail->keyData.dataSegment.data[5] << 8 | tail->keyData.dataSegment.data[6];
                                        recipe->ecLow = tail->keyData.dataSegment.data[7] << 8 | tail->keyData.dataSegment.data[8];
                                        recipe->ecHigh = tail->keyData.dataSegment.data[9] << 8 | tail->keyData.dataSegment.data[10];
                                        recipe->ecDosingTime = tail->keyData.dataSegment.data[11] << 8 | tail->keyData.dataSegment.data[12];
                                        recipe->ecMixingTime = tail->keyData.dataSegment.data[13] << 8 | tail->keyData.dataSegment.data[14];
                                        recipe->ecMaxDosingCycles = tail->keyData.dataSegment.data[15] << 8 | tail->keyData.dataSegment.data[16];
                                        recipe->phTarget = tail->keyData.dataSegment.data[17] << 8 | tail->keyData.dataSegment.data[18];
                                        recipe->phDeadband = tail->keyData.dataSegment.data[19] << 8 | tail->keyData.dataSegment.data[20];
                                        recipe->phLow = tail->keyData.dataSegment.data[21] << 8 | tail->keyData.dataSegment.data[22];
                                        recipe->phHigh = tail->keyData.dataSegment.data[23] << 8 | tail->keyData.dataSegment.data[24];
                                        recipe->phDosingTime = tail->keyData.dataSegment.data[25] << 8 | tail->keyData.dataSegment.data[26];
                                        recipe->phMixingTime = tail->keyData.dataSegment.data[27] << 8 | tail->keyData.dataSegment.data[28];
                                        recipe->phMaxDosingCycles = tail->keyData.dataSegment.data[29] << 8 | tail->keyData.dataSegment.data[30];
                                        for(int i = 0; i < AQUA_RECIPE_PUMP_MX; i++)
                                        {
                                            u16 state = tail->keyData.dataSegment.data[31] << 8 | tail->keyData.dataSegment.data[32];
                                            if(state & (1 << i))
                                            {
                                                recipe->pumpList[i].state = 1;
                                            }
                                            else
                                            {
                                                recipe->pumpList[i].state = 0;
                                            }

                                            recipe->pumpList[i].type = tail->keyData.dataSegment.data[33 + 2 * i] << 8 | tail->keyData.dataSegment.data[34 + 2 * i];
                                            recipe->pumpList[i].ratio = tail->keyData.dataSegment.data[49 + 2 * i] << 8 | tail->keyData.dataSegment.data[50 + 2 * i];

                                        }
                                        u8 recipeName[10];
                                        rt_memcpy(recipeName, &tail->keyData.dataSegment.data[65], 9);
                                        recipeName[9] = 0;
                                        rt_memcpy(recipe->formName, recipeName, AQUA_RECIPE_NAMESZ - 1);
                                        recipe->formName[AQUA_RECIPE_NAMESZ - 1] = '\0';
                                        LOG_W("num %d, recipe name = %s",
                                                ask->askType - ASK_RECIPE_0, recipeName);

                                    }
                                }

                                setAskStateOK();
                            }
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
    for(u8 i = 0; i < monitor->aqua_size; i++)
    {
        //1.已经发送数据了 但是数据接收超时判断为失联
        if(sendMoni[i].SendCnt > 2)
        {
            GetAquaByAddr(monitor,  sendMoni[i].addr)->conn_state = CON_FAIL;
        }
        else
        {
            GetAquaByAddr(monitor, sendMoni[i].addr)->conn_state = CON_SUCCESS;
        }

        if(connect[i] != GetAquaByAddr(monitor, sendMoni[i].addr)->conn_state)
        {
            connect[i] = GetAquaByAddr(monitor, sendMoni[i].addr)->conn_state;

            if(CON_FAIL == connect[i])
            {
                LOG_E("aqua addr = %d disconnect, sendCnt = %d",
                        sendMoni[i].addr,sendMoni[i].SendCnt);
            }
            else if(CON_SUCCESS == connect[i])
            {
                LOG_W("aqua addr = %d reconnect, sendCnt = %d",
                        sendMoni[i].addr,sendMoni[i].SendCnt);
            }
        }
    }

}

void InitAquaObject(void)
{
    //1.初始化记录发送情况
    InitAquaCache();
    rt_memset(sendMoni, 0, sizeof(uart_send_aqua) * TANK_LIST_MAX);

    //2.初始化相关数据
    aquaObject.dev = RT_NULL;
    aquaObject.recvList.list.next = RT_NULL;
    aquaObject.recvList.list.keyData.key = 0x00;
    aquaObject.taskList.list.next = RT_NULL;
    aquaObject.taskList.list.keyData.key = 0x00;

    //3.实例化相关接口
    aquaObject.ConfigureUart = ConfigureUart3;
    aquaObject.DeviceCtrl = RT_NULL;
    aquaObject.LineCtrl = RT_NULL;
    aquaObject.Line4Ctrl = RT_NULL;
    aquaObject.AquaCtrl = aquaCtrl;
    aquaObject.AquaSendMonitor = AquaSendMonitor;
    aquaObject.aquaSendInfo = aquaSendInfo;
    aquaObject.AskDevice = RT_NULL;
    aquaObject.AskSensor = RT_NULL;
    aquaObject.AskLine = RT_NULL;
    aquaObject.AskAquaState = AskAquaState;
    aquaObject.SendCmd = SendCmd;
    aquaObject.RecvCmd = RecvCmd;
    aquaObject.RecvListHandle = RecvListHandle;
    aquaObject.KeepConnect = KeepConnect;
    aquaObject.Optimization = Optimization;

    aquaObject.recvList.GetList = GetRecvList;       //获取核对关注列表
    aquaObject.recvList.AddToList = AddToRecvList;   //添加到关注列表中,数据接收的时候就
    aquaObject.recvList.DeleteToList = DeleteRecvList;
    aquaObject.recvList.CheckDataCorrect = CheckDataInRecvList;
    aquaObject.recvList.KeyHasExist = KeyHasExistInRecvList;

    aquaObject.taskList.GetList = GetTaskList;       //获取核对关注列表
    aquaObject.taskList.AddToList = AddToTaskList;   //添加到关注列表中,数据接收的时候就
    aquaObject.taskList.DeleteToList = DeleteToTaskList;
    aquaObject.taskList.CheckDataCorrect = CheckDataInTaskList;
    aquaObject.taskList.KeyHasExist = KeyHasExistInTaskList;
}

type_uart_class *GetAquaObject(void)
{
    return &aquaObject;
}

#endif
