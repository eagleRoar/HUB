/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-27     Administrator       the first version
 */

#include "SdcardBusiness.h"
#include "SdcardDataLayer.h"
#include "Uart.h"
#include "TcpPersistence.h"

static u8 action_sum = 0;                   //action 存储的数量
static u8 condition_sum = 0;                //condition 存储的数量
static u8 excute_sum = 0;                   //excute 存储的数量
static u8 dotask_sum = 0;                   //dotask 存储的数量

static type_sdoperate_t     operate;        //SD卡对外开放的操作

/**
 * @brief SD内文件和文件夹有效性
 */
void SdDirInit(void)
{
    //检测文件夹可读性
    CheckDirectory(DOWNLOAD_DIR);
    CheckDirectory(MODULE_DIR);
    CheckDirectory(TEST_DIR);
    CheckDirectory(SETTING_DIR);
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

    ReadSdData(MODULE_FILE, (u8 *)monitor, 0, sizeof(struct allocateStruct));

    if(RT_ERROR == ReadSdData(MODULE_FILE, &(monitor->monitorDeviceTable.deviceManageLength), sizeof(struct allocateStruct), 1))     //该位置存储的是module长度
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
            if(RT_ERROR == ReadSdData(MODULE_FILE, monitor->monitorDeviceTable.deviceTable, sizeof(struct allocateStruct) + 1,
                    monitor->monitorDeviceTable.deviceManageLength * sizeof(type_module_t)))
            {
                LOG_D("GetMonitor Err2");
            }
            else
            {
                for(index = 0; index < monitor->monitorDeviceTable.deviceManageLength; index++)
                {
                    /* 需要重新向主机发送注册命令 */
                    monitor->monitorDeviceTable.deviceTable[index].registerAnswer = SEND_NULL;

                    PrintModule(monitor->monitorDeviceTable.deviceTable[index]);
                }
            }
        }
    }
}

void SaveAddrAndLenToFile(type_monitor_t *monitor)
{
    if(RT_ERROR == WriteSdData(MODULE_FILE, &(monitor->allocateStr), 0, sizeof(struct allocateStruct)))
    {
        LOG_D("SaveModuleToFile err1");
    }
    else
    {
        LOG_D("SaveModuleToFile OK1");
    }

    if(RT_ERROR == WriteSdData(MODULE_FILE, &(monitor->monitorDeviceTable.deviceManageLength), sizeof(struct allocateStruct), 1))
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
    if(RT_ERROR == WriteSdData(MODULE_FILE, module, sizeof(struct allocateStruct) + 1 + index * sizeof(type_module_t), sizeof(type_module_t)))
    {
        LOG_D("SaveModuleToFile err1");
    }
    else
    {
        LOG_D("SaveModuleToFile OK1");
    }
}

/*******************************存储设置类***************************************************/
/**
 * 获取ActionSum
 * @return
 */
static u8 GetActionSum(void)
{
    if(RT_ERROR == ReadSdData(ACTION_FILE, (u8 *)&action_sum, 0, 1))
    {
        action_sum = 0;
    }

    return action_sum;
}

static u8 GetConditionSum(void)
{
    if(RT_ERROR == ReadSdData(CONDITION_FILE, (u8 *)&condition_sum, 0, 1))
    {
        condition_sum = 0;
    }

    return condition_sum;
}

static u8 GetExcuteSum(void)
{
    if(RT_ERROR == ReadSdData(EXCUTE_FILE, (u8 *)&excute_sum, 0, 1))
    {
        excute_sum = 0;
    }

    return excute_sum;
}

static u8 GetDotaskSum(void)
{
    if(RT_ERROR == ReadSdData(DOTASK_FILE, (u8 *)&dotask_sum, 0, 1))
    {
        dotask_sum = 0;
    }

    return dotask_sum;
}

/**
  *设置ActionSum
 * @param sum
 */
static void SetActionSum(u8 sum)
{
    action_sum = sum;
    WriteSdData(ACTION_FILE, (u8 *)&action_sum, 0, 1);
}

static void SetConditionSum(u8 sum)
{
    condition_sum = sum;
    WriteSdData(CONDITION_FILE, (u8 *)&condition_sum, 0, 1);
}

static void SetExcuteSum(u8 sum)
{
    excute_sum = sum;
    WriteSdData(EXCUTE_FILE, (u8 *)&excute_sum, 0, 1);
}

static void SetDotaskSum(u8 sum)
{
    dotask_sum = sum;
    WriteSdData(DOTASK_FILE, (u8 *)&dotask_sum, 0, 1);
}

/**
 * find action
 * @param id
 * @return 如果存在则返回该位置，否则返回最后位置
 */
static u8 FindActionById(u32 id)
{
    u8      index           = 0;
    u8      ret             = 0;
    u32     find_id         = 0;

   for(index = 0; index < GetActionSum(); index++)
   {
       ReadSdData(ACTION_FILE, (u8 *)&find_id, 1 + sizeof(type_action_t) * index, 4);

       if(id == find_id)
       {
           ret = index;
           break;
       }
   }

   if(index == GetActionSum())
   {
       return GetActionSum();
   }
   else
   {
       return ret;
   }
}

static u8 FindConditionById(u32 id)
{
    u8      index           = 0;
    u8      ret             = 0;
    u32     find_id         = 0;

   for(index = 0; index < GetConditionSum(); index++)
   {
       ReadSdData(CONDITION_FILE, (u8 *)&find_id, 1 + sizeof(type_condition_t) * index, 4);

       if(id == find_id)
       {
           ret = index;
           break;
       }
   }

   if(index == GetConditionSum())
   {
       return GetConditionSum();
   }
   else
   {
       return ret;
   }
}

static u8 FindExcuteById(u32 id)
{
    u8      index           = 0;
    u8      ret             = 0;
    u32     find_id         = 0;

   for(index = 0; index < GetExcuteSum(); index++)
   {
       ReadSdData(EXCUTE_FILE, (u8 *)&find_id, 1 + sizeof(type_excute_t) * index, 4);

       if(id == find_id)
       {
           ret = index;
           break;
       }
   }

   if(index == GetExcuteSum())
   {
       return GetExcuteSum();
   }
   else
   {
       return ret;
   }
}

static u8 FindDotaskById(u32 id)
{
    u8      index           = 0;
    u8      ret             = 0;
    u32     find_id         = 0;

   for(index = 0; index < GetDotaskSum(); index++)
   {
       ReadSdData(DOTASK_FILE, (u8 *)&find_id, 1 + sizeof(type_dotask_t) * index, 4);

       if(id == find_id)
       {
           ret = index;
           break;
       }
   }

   if(index == GetDotaskSum())
   {
       return GetDotaskSum();
   }
   else
   {
       return ret;
   }
}

static u8 ReadActionSum(void)
{
    return action_sum;
}

static u8 ReadConditionSum(void)
{
    return condition_sum;
}

static u8 ReadExcuteSum(void)
{
    return excute_sum;
}

static u8 ReadDotaskSum(void)
{
    return dotask_sum;
}
/**
  * 增加action 结构体,该结构体对应的是梯形曲线设置
 */
static void AddActionToSD(type_action_t action)
{
    u8          index           = FindActionById(action.id);
    u16         crc             = 0x0000;

    WriteSdData(ACTION_FILE, (u8 *)&action, 1 + index * (sizeof(type_action_t) + 2), sizeof(type_action_t));
    //加入crc校验
    crc = usModbusRTU_CRC((u8 *)&action, sizeof(type_action_t));
    WriteSdData(ACTION_FILE, (u8 *)&crc, 1 + index * (sizeof(type_action_t) + 2) + sizeof(type_action_t), 2);

    if(index == GetActionSum())
    {
        SetActionSum(GetActionSum() + 1);
    }
}

static void AddConditionToSD(type_condition_t condition)
{
    u8          index           = FindConditionById(condition.id);
    u16         crc             = 0x0000;

    WriteSdData(CONDITION_FILE, (u8 *)&condition, 1 + index * (sizeof(type_condition_t) + 2), sizeof(type_condition_t));
    //加入crc校验
    crc = usModbusRTU_CRC((u8 *)&condition, sizeof(type_condition_t));
    WriteSdData(CONDITION_FILE, (u8 *)&crc, 1 + index * (sizeof(type_condition_t) + 2) + sizeof(type_condition_t), 2);

    if(index == GetConditionSum())
    {
        SetConditionSum(GetConditionSum() + 1);
    }
}

static void AddExcuteToSD(type_excute_t excute)
{
    u8          index           = FindExcuteById(excute.id);
    u16         crc             = 0x0000;

    WriteSdData(EXCUTE_FILE, (u8 *)&excute, 1 + index * (sizeof(type_excute_t) + 2), sizeof(type_excute_t));
    //加入crc校验
    crc = usModbusRTU_CRC((u8 *)&excute, sizeof(type_excute_t));
    WriteSdData(EXCUTE_FILE, (u8 *)&crc, 1 + index * (sizeof(type_excute_t) + 2) + sizeof(type_excute_t), 2);

    if(index == GetExcuteSum())
    {
        SetExcuteSum(GetExcuteSum() + 1);
    }
}

static void AddDotaskToSD(type_dotask_t dotask)
{
    u8          index           = FindDotaskById(dotask.id);
    u16         crc             = 0x0000;

    WriteSdData(DOTASK_FILE, (u8 *)&dotask, 1 + index * (sizeof(type_dotask_t) + 2), sizeof(type_dotask_t));
    //加入crc校验
    crc = usModbusRTU_CRC((u8 *)&dotask, sizeof(type_dotask_t));
    WriteSdData(DOTASK_FILE, (u8 *)&crc, 1 + index * (sizeof(type_dotask_t) + 2) + sizeof(type_dotask_t), 2);

    if(index == GetDotaskSum())
    {
        SetDotaskSum(GetDotaskSum() + 1);
    }
}

/**
 * 获取SD 卡中的action
 * @param index
 * @return
 */
//Justin debug 未完待续 未验证
void TakeAction(type_action_t *action, u8 index)
{
    u8 *data = RT_NULL;
    u16 crc = 0x0000;

    data = rt_malloc(sizeof(type_action_t) + 2);

    if(RT_NULL != data)
    {
        ReadSdData(ACTION_FILE, data, 1 + index * (sizeof(type_action_t) + 2), sizeof(type_action_t) + 2);

        rt_memcpy((u8 *)action, data, sizeof(type_action_t));

        crc = usModbusRTU_CRC((u8 *)action, sizeof(type_action_t));

        //如果crc 错误的话action为RT_NULL
        if(0 != rt_memcmp(&data[sizeof(type_action_t)], (u8 *)&crc, 2))
        {
            action = RT_NULL;
        }
    }

    rt_free(data);
    data = RT_NULL;
}

static void SdOperateInit(void)
{
    operate.action_op.ReadActionSum = ReadActionSum;
    operate.action_op.AddActionToSD = AddActionToSD;

    operate.condition_op.ReadConditionSum = ReadConditionSum;
    operate.condition_op.AddConditionToSD = AddConditionToSD;

    operate.excute_op.ReadExcuteSum = ReadExcuteSum;
    operate.excute_op.AddExcuteToSD = AddExcuteToSD;

    operate.dotask_op.ReadDotaskSum = ReadDotaskSum;
    operate.dotask_op.AddDotaskToSD = AddDotaskToSD;
}

type_sdoperate_t GetSdOperate(void)
{
    static u8 init = NO;

    if(NO == init)
    {
        GetActionSum();
        GetExcuteSum();
        GetActionSum();
        GetDotaskSum();

        SdOperateInit();
        init = YES;
    }

    return operate;
}
