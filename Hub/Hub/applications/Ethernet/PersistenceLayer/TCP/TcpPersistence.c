/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-01     Administrator       the first version
 */
#include "TcpPersistence.h"
#include "Ethernet.h"
#include "Uart.h"
#include "Device.h"
#include "InformationMonitor.h"
#include "Uart.h"
#include "SDCard.h"

extern struct sdCardState      sdCard;

extern type_action_t   Action[3];
type_condition_t    Condition;
type_excute_t       Excute;
type_dotask_t       Dotask;

void RegisterHub(rt_tcpclient_t *handle)
{
    u32                 id              = 0x00000000;
    type_hubreg_t       hubReg;
    type_package_t      pack;

    ReadUniqueId(&id);
    pack.package_top.checkId = CHECKID;                                                         //从机识别码
    pack.package_top.answer = ASK_ACTIVE;                                                       //应答,主动发送
    pack.package_top.function = F_HUB_REGSTER;                                                  //功能码
    pack.package_top.id = id;                                                                   //发送者id
    pack.package_top.serialNum = 0x00000000;                                                    //序列号

    hubReg.type = ENV_TYPE;                                                                     //环境控制
    hubReg.version = 0x01;                                                                      //版本号 BCD8421编码 需要注意
    //hubReg.config_id = 0x00;                                                                    //配置ID
    hubReg.heart = 0x10;                                                                        //心跳间隔
    rt_memcpy(hubReg.name, HUB_NAME, HUB_NAME_SIZE);
    rt_memcpy(pack.buffer, &hubReg, sizeof(type_hubreg_t));
    pack.package_top.crc = CRC16((u16*)&pack+3, 6+sizeof(type_hubreg_t)/2, 0);
    pack.package_top.length = sizeof(struct packTop) + sizeof(type_hubreg_t);

    rt_tcpclient_send(handle, &pack, pack.package_top.length);
}

void RegisterModule(rt_tcpclient_t *handle)
{
    u8                  small           = 0x00;
    u16                 index           = 0x00;
    u16                 temp            = 0x00;
    u32                 id              = 0x00000000;
    type_package_t      pack;
    type_module_t       module;                                                                 //只读
    type_sen_reg_t      sensorReg;
    type_dev_reg_t      deviceReg;


    ReadUniqueId(&id);

    if(0 >= GetMonitor()->monitorDeviceTable.deviceManageLength)
    {
        return;
    }
    //以下顺序不能变
    pack.package_top.checkId = CHECKID;                                                         //从机识别码
    pack.package_top.answer = ASK_ACTIVE;                                                       //应答,主动发送
    pack.package_top.id = id;                                                                   //发送者id

    for(index = 0; index < GetMonitor()->monitorDeviceTable.deviceManageLength; index++)
    {
        module = GetMonitor()->monitorDeviceTable.deviceTable[index];

        if(RECV_OK != module.registerAnswer)                                                    //如果还没有注册就发送注册
        {
            pack.package_top.function = module.function;                                        //功能码
            pack.package_top.serialNum = module.address;                                        //序列号 将该序列号用作注册时候检验是否注册成功的地址

            if(SENSOR_TYPE == module.s_or_d)
            {
                sensorReg.uuid = module.uuid;                                                   //传感器ID
                rt_memcpy(sensorReg.name, module.module_name, MODULE_NAMESZ);                   //名称
                itoa(sensorReg.uuid, sensorReg.product_type, 16);//16指的是16进制                               //产品型号,自己管控,只有同型号的产品才能分到同一组
                sensorReg.second = 0x0001;                                                      //发送频率时间，单位s
                sensorReg.parameter = module.storage_size;                                      //传感器参数，指的是有几个传感器，比如四合一，那就填写4

                for(small = 0; small < sensorReg.parameter; small++)
                {
                    sensorReg.scale_group[small].small_scale = module.module_t[small].small_scale;
                    rt_memcpy(sensorReg.scale_group[small].scale_name, module.module_t[small].name, MODULE_NAMESZ);
                }

                temp = sizeof(type_sen_reg_t) - (SENSOR_STR_MAX - sensorReg.parameter)*sizeof(struct scaleGroup);
                rt_memcpy(pack.buffer, &sensorReg, temp);
                pack.package_top.crc = CRC16((u16*)&pack+3, sizeof(struct packTop)/2 - 3 + temp/2, 0);
                pack.package_top.length = sizeof(struct packTop) + temp;

//                LOG_D("register sensor id = %x",sensorReg.uuid);//Justin debug
            }
            else if(DEVICE_TYPE == module.s_or_d)
            {
                deviceReg.uuid = module.uuid;
                rt_memcpy(deviceReg.name, module.module_name, MODULE_NAMESZ);
                itoa(deviceReg.uuid, deviceReg.product_type, 16);//16指的是16进制
                deviceReg.second = 0x0001;
                deviceReg.parameter = module.storage_size;

                for(small = 0; small < deviceReg.parameter; small++)
                {
                    deviceReg.scale_group[small].small_scale = module.module_t[small].small_scale;
                    rt_memcpy(deviceReg.scale_group[small].scale_name, module.module_t[small].name, MODULE_NAMESZ);
                    deviceReg.scale_group[small].fuction = module.module_t[small].scale_fuction;
                    deviceReg.scale_group[small].parameter_min = module.module_t[small].parameter_min;
                    deviceReg.scale_group[small].parameter_max = module.module_t[small].parameter_max;
                }

                temp = sizeof(type_dev_reg_t) - (SENSOR_STR_MAX - deviceReg.parameter)*sizeof(struct deviceScaleGroup);
                rt_memcpy(&(pack.buffer[0]), &deviceReg, temp);
                pack.buffer[temp] = 0x0000; //Justin debug 仅仅测试 需要添加该数据是因为后续还有内容，参照协议,还应该有一个小结构体写支持的设备功能
                temp += 2;//Justin debug 仅仅测试
                pack.package_top.crc = CRC16((u16*)&pack+3, sizeof(struct packTop)/2 - 3 + temp/2, 0);
                pack.package_top.length = sizeof(struct packTop) + temp;

//                LOG_D("register device id = %x",deviceReg.uuid);//Justin debug
            }

            rt_tcpclient_send(handle, &pack, pack.package_top.length);
            GetMonitor()->monitorDeviceTable.deviceTable[index].registerAnswer = SEND_OK;
        }
    }

}

void ReplyModuleReg(type_monitor_t *monitor, type_package_t data)
{
    type_module_t       module;
    u8                 index       = 0;

    for(index = 0; index < monitor->monitorDeviceTable.deviceManageLength; index++)
    {
        module = monitor->monitorDeviceTable.deviceTable[index];   //注意:module为只读，方便简写而已，复制的话不能直接对module赋值

        if(module.address == data.package_top.serialNum)
        {
            if(ASK_REPLY != data.package_top.answer)
            {
                monitor->monitorDeviceTable.deviceTable[index].registerAnswer = REG_ERR;
                LOG_D("sensor %d name %s register fail", index, monitor->monitorDeviceTable.deviceTable[index].module_name);
            }
            else
            {
                monitor->monitorDeviceTable.deviceTable[index].registerAnswer = RECV_OK;
                LOG_D("sensor %d name %s register OK", index, monitor->monitorDeviceTable.deviceTable[index].module_name);
            }
        }
    }
}

/**
 * 触发与动作，执行任务
 * @param handle
 * @param monitor
 * @param data
 */
void SetDotask(rt_tcpclient_t *handle, type_monitor_t *monitor, type_package_t data)
{
    type_dotask_t       dotask;
    u8                  ret                 = ASK_REPLY;
    u16                 dotaskSize          = sizeof(type_dotask_t);

    rt_memcpy((u8 *)&dotask + 2, (u8 *)data.buffer, dotaskSize - 2);    //最开始的crc占两位
    dotask.crc = usModbusRTU_CRC((u8 *)&dotask + 2, dotaskSize - 2);

    Dotask = dotask;
    PrintDotask(Dotask);
    //Justin debug 仅仅测试
//    if(YES == sdCard.init)
//    {
//        //新增新的dotask到SD卡
//        sdCard.sd_operate.dotask_op.AddDotaskToSD(dotask);
//    }
//    else
//    {
//        ret = UNKNOWN_ERR;
//    }

    /* 回复主机 */
    ReplyMaster(handle, data.package_top.function, ret, data.package_top.serialNum);
}

/**
 * 设置执行动作
 * @param handle
 * @param monitor
 * @param data
 */
void SetExcute(rt_tcpclient_t *handle, type_monitor_t *monitor, type_package_t data)
{
    type_excute_t       excute;
    u8                  ret                 = ASK_REPLY;

    rt_memcpy((u8 *)&excute + 2, (u8 *)data.buffer, sizeof(type_excute_t) - 2);   //最开始的crc占2位
    excute.crc = usModbusRTU_CRC((u8 *)&excute + 2, sizeof(type_excute_t) - 2);


    //Justin debug 仅仅测试
//    if(YES == sdCard.init)
//    {
//        //新增新的excute到SD卡
//        sdCard.sd_operate.excute_op.AddExcuteToSD(excute);
//    }
//    else
//    {
//        ret = UNKNOWN_ERR;
//    }
    Excute = excute;
    PrintExcute(Excute);
    /* 回复主机 */
    ReplyMaster(handle, data.package_top.function, ret, data.package_top.serialNum);
}

/**
 * 解析等量关系
 * @param data
 */
static u8 AnalyzeEqual(char *data)
{
    char    *p;
    u8      ret = 0;

    LOG_D("AnalyzeEqual data = %s",data);//Justin debug仅仅测试

    switch (*data) {
        case '<':
            ret = LESS_THAN;
            break;
        case '>':
            ret = GREATER_THAN;
            break;
        case '=':
            ret = EQUAL_TO;
            break;
        default:
            break;
    }

//    if((p = strchr(data, '<')) != NULL)
//    {
//        ret = LESS_THAN;
//        if(strchr(p+1, '=') != NULL)
//        {
//            ret = LESSTHAN_EQUAL;
//        }
//    }
//    else if((p = strchr(data, '>')) != NULL)
//    {
//        ret = GREATER_THAN;
//        if(strchr(p+1, '=') != NULL)
//        {
//            ret = GREATERTHAN_EQUAL;
//        }
//    }
//    else if((p = strchr(data, '=')) != NULL)
//    {
//        ret = EQUAL_TO;
//        if(strchr(p+1, '=') != NULL)
//        {
//            ret = EQUAL_TO;
//        }
//    }

    return ret;
}

/**
 * 接收触发条件
 * @param handle
 * @param monitor
 * @param data
 */
void SetCondition(rt_tcpclient_t *handle, type_monitor_t *monitor, type_package_t data)
{
    char                *delims             = { "()$#:" };
    char                *p                  = RT_NULL;
    u8                  ret                 = ASK_REPLY;
    type_condition_t    condition;

    rt_memcpy(&(condition.id), (u8 *)data.buffer, 4);   //buffer是u16类型
    rt_memcpy(&(condition.priority), (u8 *)&(data.buffer[2]), 2);
    //发送过来的可能是寄存器的条件的和梯形曲线ID的
    if(NULL != strchr((char *)&(data.buffer[3]), ':'))
    {
//        LOG_D("0-----------------------data length= %s",(char *)&(data.buffer[3]));
        //($24:1$)<(梯形曲线ID 5)
        condition.analyze_type = STORAGE_TYPE;
        p = strtok((char *)&(data.buffer[3]), delims);
//        LOG_D("1-----------------------data length= %s",p);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.value = 0;
        condition.module_uuid = atol(p);
        p = strtok(NULL, delims);
//        LOG_D("2-----------------------data length= %s",p);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.storage = atoi(p);
        p = strtok(NULL, delims);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.condition = AnalyzeEqual(p);

        p = strtok(NULL, delims);
//        LOG_D("3-----------------------data length= %s",p);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.action.action_id = atol(p);
    }
    else
    {
        //(1)==(梯形曲线ID 3)
        condition.analyze_type = VALUE_TYPE;
        p = strtok((char *)&(data.buffer[3]), delims);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.value = atoi(p);
        condition.module_uuid = 0;
        condition.storage = 0;
        p = strtok(NULL, delims);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.condition = AnalyzeEqual(p);
        p = strtok(NULL, delims);
        if(NULL == p)
        {
            ret = UNKNOWN_ERR;
        }
        condition.action.action_id = atol(p);
    }

    condition.crc = usModbusRTU_CRC((u8 *)&condition + 2, sizeof(type_condition_t) - 2);
    Condition = condition;
    PrintCondition(Condition);
    //Justin debug仅仅为了测试
//    if(YES == sdCard.init)
//    {
//        //新增新的condition到SD卡
//        sdCard.sd_operate.condition_op.AddConditionToSD(condition);
//    }
//    else
//    {
//        ret = UNKNOWN_ERR;
//    }

    /* 回复主机 */
    ReplyMaster(handle, data.package_top.function, ret, data.package_top.serialNum);

}

/** 梯形曲线设置
 * 发送过来的数据为:
 * 梯形曲线ID        4byte
 * 梯形曲线表达式        256byte
 * 梯形曲线表达式为:[曲线区间数目,(起始值，终点值，持续时长),(起始值，终点值，持续时长),…,(起始值，终点值，持续时长)]
 *
 * @param monitor
 * @param data
 */
void Set_Action(rt_tcpclient_t *handle, type_monitor_t *monitor, type_package_t data)
{
    char            *delims             = { "[],()" };
    char            *p                  = RT_NULL;
    u8              index               = 0x00;
    u8              ret                 = ASK_REPLY;
    u8              *temp               = RT_NULL;
    u16             length              = 0x0000;       //需要存储的长度
    u16             actionSize          = sizeof(type_action_t) - sizeof(type_curve_t *);
    u16             curveLength         = 0x0000;       //curve 的长度
    type_action_t   action;

    p = strtok((char *)&(data.buffer[2]), delims);
    if(p != NULL)
    {
        curveLength = atol(p);
        LOG_D("curve_length = %d",curveLength);
    }

    //设置的长度不能超过规定存储的长度
    length = actionSize + sizeof(type_curve_t) * curveLength;

    if(length > SAVE_ACTION_MAX_ZISE)
    {
        return;
    }

    action.curve = rt_malloc(SAVE_ACTION_MAX_ZISE - actionSize);

    if(RT_NULL != action.curve)
    {
        rt_memset((u8 *)action.curve, 0, SAVE_ACTION_MAX_ZISE - actionSize);

        rt_memcpy(&(action.id), (u8 *)data.buffer, 4);
        action.curve_length = curveLength;
        for(index = 0; index < action.curve_length; index++)
        {
            p = strtok(NULL, delims);
            action.curve[index].start_value = atoi(p);
            p = strtok(NULL, delims);
            action.curve[index].end_value = atoi(p);
            p = strtok(NULL, delims);
            action.curve[index].time = atol(p);
        }

        temp = rt_malloc(SAVE_ACTION_MAX_ZISE);
        if(RT_NULL != temp)
        {
            rt_memcpy(temp, (u8 *)&action, actionSize);
            rt_memcpy(&temp[actionSize], (u8 *)action.curve, SAVE_ACTION_MAX_ZISE - actionSize);
            action.crc = usModbusRTU_CRC(temp + 2, SAVE_ACTION_MAX_ZISE - 2);//crc 占两位，crc不加入校验

            rt_free(temp);
            temp = RT_NULL;
        }
    }

    Action[2] = action;
    PrintAction(Action[2]);
    //Justin debug仅仅测试
//    if(YES == sdCard.init)
//    {
//        //新增新的action到SD卡
//        sdCard.sd_operate.action_op.AddActionToSD(action);
//    }
//    else
//    {
//        ret = UNKNOWN_ERR;
//    }

    /* 回复主机 */
    ReplyMaster(handle, data.package_top.function, ret, data.package_top.serialNum);

    rt_free(action.curve);
    action.curve = RT_NULL;
}

void ReplyMaster(rt_tcpclient_t *handle, u16 func, u16 answer, u32 serialNum)
{
    type_package_t *pack = RT_NULL;

    pack = rt_malloc(sizeof(struct packTop));
    if(RT_NULL != pack)
    {
        pack->package_top.checkId = CHECKID;
        pack->package_top.answer = answer;
        pack->package_top.function = func;
        ReadUniqueId(&(pack->package_top.id));
        pack->package_top.serialNum = serialNum;

        pack->package_top.crc = CRC16((u16 *)pack + 3, sizeof(struct packTop)/2 - 3, 0);
        pack->package_top.length = sizeof(struct packTop);
    }

    rt_tcpclient_send(handle, pack, pack->package_top.length);

    rt_free(pack);
    pack = RT_NULL;
}
