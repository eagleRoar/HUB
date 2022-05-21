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

//Justin debug 定义以下全局变量为了测试
type_action_t   Action[3];

/**
 * 初始化设置类的文件
 * @param file_name
 */
void SettingFileInit(char* file_name)
{
    u8 text = 0;
    //校验文件里面的数据,相应初始化sd卡文件标记
    if(NO == CheckFileAndMark(file_name))
    {
        WriteSdData(file_name, &text, SD_HEAD_SIZE, 1);
    }
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
    if(RT_ERROR == ReadSdData(ACTION_FILE, (u8 *)&action_sum, SD_INFOR_SIZE - 1, 1))
    {
        LOG_D("GetActionSum read sd err");//Justin debug
        action_sum = 0;
    }
    LOG_D("GetActionSum = %d",action_sum);
    return action_sum;
}

static u8 GetConditionSum(void)
{
    if(RT_ERROR == ReadSdData(CONDITION_FILE, (u8 *)&condition_sum, SD_INFOR_SIZE - 1, 1))
    {
        condition_sum = 0;
    }

    return condition_sum;
}

static u8 GetExcuteSum(void)
{
    if(RT_ERROR == ReadSdData(EXCUTE_FILE, (u8 *)&excute_sum, SD_INFOR_SIZE - 1, 1))
    {
        excute_sum = 0;
    }

    return excute_sum;
}

static u8 GetDotaskSum(void)
{
    if(RT_ERROR == ReadSdData(DOTASK_FILE, (u8 *)&dotask_sum, SD_INFOR_SIZE - 1, 1))
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
    WriteSdData(ACTION_FILE, (u8 *)&action_sum, SD_INFOR_SIZE - 1, 1);
}

static void SetConditionSum(u8 sum)
{
    condition_sum = sum;
    WriteSdData(CONDITION_FILE, (u8 *)&condition_sum, SD_INFOR_SIZE - 1, 1);
}

static void SetExcuteSum(u8 sum)
{
    excute_sum = sum;
    WriteSdData(EXCUTE_FILE, (u8 *)&excute_sum, SD_INFOR_SIZE - 1, 1);
}

static void SetDotaskSum(u8 sum)
{
    dotask_sum = sum;
    WriteSdData(DOTASK_FILE, (u8 *)&dotask_sum, SD_INFOR_SIZE - 1, 1);
}

/**
 * find action
 * @param id
 * @return 如果存在则返回该位置，否则返回最后位置
 */
static u8 FindActionById(u32 id, u8 *index)
{
    //u8      temp            = 0;
    u8      ret             = RT_EOK;
    u32     find_id         = 0;
    u8      actionSum       = 0;

    actionSum = GetActionSum();
    //index = &temp;

    LOG_D("FindActionById actionSum = %d",actionSum);//Justin debug

    if(0 == actionSum)
    {
        *index = 0;
        ret = RT_ERROR;
    }
    else
    {
        for(*index = 0; *index < actionSum; *index++)
        {
           ReadSdData(ACTION_FILE, (u8 *)&find_id, SD_INFOR_SIZE + *index * (sizeof(type_action_t) + 2), 4);//crc 为2

           if(id == find_id)
           {
               break;
           }
        }

        if(*index == actionSum)
        {
            ret = RT_ERROR;
        }
        else
        {
            ret = RT_EOK;
        }
    }

    return ret;
}

static u8 FindConditionById(u32 id, u8 *index)
{
    u8      temp            = 0;
    u8      ret             = RT_EOK;
    u32     find_id         = 0;
    u8      conditionSum    = GetConditionSum();

    index = &temp;

    if(0 == conditionSum)
    {
        temp = 0;
        ret = RT_ERROR;
    }
    else
    {
        for(temp = 0; temp < conditionSum; temp++)
        {
           ReadSdData(CONDITION_FILE, (u8 *)&find_id, SD_INFOR_SIZE + temp * (sizeof(type_condition_t) + 2), 4);

           if(id == find_id)
           {
               break;
           }
        }

        if(temp == conditionSum)
        {
           ret = RT_ERROR;
        }
        else
        {
           ret = RT_EOK;
        }
    }

    return ret;
}

static u8 FindExcuteById(u32 id, u8 *index)
{
    u8      temp            = 0;
    u8      ret             = RT_EOK;
    u32     find_id         = 0;
    u8      excuteSum       = GetExcuteSum();

    index = &temp;

    if(0 == excuteSum)
    {
        temp = 0;
        ret = RT_ERROR;
    }
    else
    {
        for(temp = 0; temp < excuteSum; temp++)
        {
           ReadSdData(EXCUTE_FILE, (u8 *)&find_id, SD_INFOR_SIZE + temp * (sizeof(type_excute_t) + 2) , 4);

           if(id == find_id)
           {
               break;
           }
        }

        if(temp == excuteSum)
        {
            ret = RT_ERROR;
        }
        else
        {
            ret = RT_EOK;
        }
    }

    return ret;
}

static u8 FindDotaskById(u32 id, u8 *index)
{
    u8      temp            = 0;
    u8      ret             = RT_EOK;
    u32     find_id         = 0;
    u8      dotaskSum       = GetDotaskSum();

    index = &temp;

    if(0 == dotaskSum)
    {
        temp = 0;
        ret = RT_ERROR;
    }
    else
    {
        for(temp = 0; temp < dotaskSum; temp++)
        {
           ReadSdData(DOTASK_FILE, (u8 *)&find_id, SD_INFOR_SIZE + temp * (sizeof(type_dotask_t) + 2), 4);

           if(id == find_id)
           {
               break;
           }
        }

        if(temp == dotaskSum)
        {
            ret = RT_ERROR;
        }
        else
        {
            ret = RT_EOK;
        }
    }

    return ret;
}

/**
  * 增加action 结构体,该结构体对应的是梯形曲线设置
 */
static void AddActionToSD(type_action_t action)
{
    u8          index           = 0;
    u16         actionSize      = sizeof(type_action_t) - sizeof(type_curve_t*);
    u16         curveSize       = SAVE_ACTION_MAX_ZISE - actionSize;
    u8          actionSum       = 0;

    actionSum       = GetActionSum();

    FindActionById(action.id, &index);
    LOG_D("-------------AddActionToSD index = %d, action sum = %d",index,actionSum);//Justin debug

    WriteSdData(ACTION_FILE, (u8 *)&action, SD_INFOR_SIZE + index * SAVE_ACTION_MAX_ZISE, actionSize);
    WriteSdData(ACTION_FILE, (u8 *)action.curve, SD_INFOR_SIZE + index * SAVE_ACTION_MAX_ZISE + actionSize, curveSize);

    if(index == actionSum)
    {
        SetActionSum(actionSum + 1);
    }
}

static void AddConditionToSD(type_condition_t condition)
{
    u8          index           = 0;
    u16         conditionSize   = sizeof(type_condition_t);


    FindConditionById(condition.id, &index);

    WriteSdData(CONDITION_FILE, (u8 *)&condition, SD_INFOR_SIZE + index * conditionSize, conditionSize);

    //Justin debug 仅仅测试
//    ReadSdData(CONDITION_FILE, (u8 *)&condition, SD_INFOR_SIZE + index * conditionSize, conditionSize);
//
//    LOG_D("crc = %x",condition.crc);
//    LOG_D("id = %x",condition.id);
//    LOG_D("priority = %x",condition.priority);
//    LOG_D("analyze_type = %x",condition.analyze_type);
//    LOG_D("value = %x",condition.value);
//    LOG_D("module_uuid = %x",condition.module_uuid);
//    LOG_D("storage = %x",condition.storage);
//    LOG_D("condition = %x",condition.condition);
//    LOG_D("action_id = %x",condition.action.action_id);

    if(index == GetConditionSum())
    {
        SetConditionSum(GetConditionSum() + 1);
    }
}

static void AddExcuteToSD(type_excute_t excute)
{
    u8          index           = 0;
    u16         excuteSize      = sizeof(type_excute_t);

    FindExcuteById(excute.id, &index);

    WriteSdData(EXCUTE_FILE, (u8 *)&excute, SD_INFOR_SIZE + index * excuteSize, excuteSize);
    //Justin debug 仅仅测试
//    LOG_D("id = %x",excute.id);
//    LOG_D("device_id = %x",excute.device_id);
//    LOG_D("storage = %x",excute.storage);
//    LOG_D("action_id_v = %x",excute.action_id_v);

    if(index == GetExcuteSum())
    {
        SetExcuteSum(GetExcuteSum() + 1);
    }
    LOG_D("GetExcuteSum = %d",GetExcuteSum());
}

static void AddDotaskToSD(type_dotask_t dotask)
{
    u8          index           = 0;
    u16         dotaskSize      = sizeof(type_dotask_t);

    FindDotaskById(dotask.id, &index);

    WriteSdData(DOTASK_FILE, (u8 *)&dotask, SD_INFOR_SIZE + index * dotaskSize, dotaskSize);
//    rt_memset((u8 *)&dotask, 0, dotaskSize);
//    ReadSdData(DOTASK_FILE, (u8 *)&dotask, SD_INFOR_SIZE + index * dotaskSize, dotaskSize);
//
//    //Justin debug
//    LOG_D("id = %x",dotask.id);
//    LOG_D("condition_id = %x",dotask.condition_id);
//    LOG_D("excuteAction_id = %x",dotask.excuteAction_id);
//    LOG_D("start = %x",dotask.start);
//    LOG_D("continue_t = %x",dotask.continue_t);
//    LOG_D("delay = %x",dotask.delay);//Justin debug 仅仅测试

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
static rt_err_t TakeAction(type_action_t *action, u8 index)
{
    u8          *data           = RT_NULL;
    u16         crc             = 0x0000;
    u16         actionSize      = 0x0000;
    u16         curveSize       = 0x0000;
    rt_err_t    ret             = RT_EOK;

    data = rt_malloc(SAVE_ACTION_MAX_ZISE);
    //获取实际的长度

    if(RT_NULL != data)
    {
        rt_memset(data, 0, SAVE_ACTION_MAX_ZISE);
        //将第index位置的数据取出来，数据长度固定为SAVE_ACTION_MAX_ZISE
        ReadSdData(ACTION_FILE, data, SD_INFOR_SIZE + index * SAVE_ACTION_MAX_ZISE, SAVE_ACTION_MAX_ZISE);

        actionSize = sizeof(type_action_t) - sizeof(type_curve_t *);

        //先取出来curve长度才能知道该给action多大空间，该操作并非全部复制action
        rt_memcpy((u8 *)action, data, actionSize);
        curveSize = sizeof(type_curve_t) * action->curve_length;

        action->curve = rt_malloc(curveSize);

        if(RT_NULL != action->curve)
        {
            rt_memcpy((u8 *)action->curve, &data[actionSize], curveSize);
        }
        else
        {
            action = RT_NULL;
            ret = RT_ERROR;
        }

        crc = usModbusRTU_CRC(data + 2, SAVE_ACTION_MAX_ZISE - 2);//crc 占两位，crc不加入校验

        //如果crc 错误的话action为RT_NULL
        if(action->crc != crc)
        {
            action = RT_NULL;
            ret = RT_ERROR;
        }
    }

    rt_free(data);
    data = RT_NULL;

    return ret;
}

static rt_err_t TakeCondition(type_condition_t *condition, u8 index)
{
    u16         conditionSize   = sizeof(type_condition_t);
    u16         crc             = 0x0000;
    rt_err_t    ret             = RT_EOK;

    //获取第index个值
    ReadSdData(CONDITION_FILE, (u8 *)condition, SD_INFOR_SIZE + index * conditionSize, conditionSize);

    crc = usModbusRTU_CRC((u8 *)condition + 2, conditionSize - 2);
    LOG_D("condition->crc = %x, crc = %x",condition->crc, crc);  //Justin debug
    if(condition->crc != crc)
    {
        condition = RT_NULL;
        ret = RT_ERROR;
    }

    return ret;
}

static rt_err_t TakeExcute(type_excute_t *excute, u8 index)
{
    u16         excuteSize      = sizeof(type_excute_t);
    u16         crc             = 0x0000;
    rt_err_t    ret             = RT_EOK;

    //获取第index个值
    ReadSdData(EXCUTE_FILE, (u8 *)excute, SD_INFOR_SIZE + index * excuteSize, excuteSize);

    crc = usModbusRTU_CRC((u8 *)excute + 2, excuteSize - 2);
    LOG_D("excute->crc = %x, crc = %x",excute->crc, crc);  //Justin debug
    if(excute->crc != crc)
    {
        excute = RT_NULL;
        ret = RT_ERROR;
    }

    return ret;
}

static rt_err_t TakeDotask(type_dotask_t *dotask, u8 index)
{
    u16         dotaskSize      = sizeof(type_dotask_t);
    u16         crc             = 0x0000;
    rt_err_t    ret             = RT_EOK;

    //获取第index个值
    ReadSdData(DOTASK_FILE, (u8 *)dotask, SD_INFOR_SIZE + index * dotaskSize, dotaskSize);

    crc = usModbusRTU_CRC((u8 *)dotask + 2, dotaskSize - 2);
    LOG_D("dotask->crc = %x, crc = %x",dotask->crc, crc);  //Justin debug
    if(dotask->crc != crc)
    {
        dotask = RT_NULL;
        ret = RT_ERROR;
    }

    return ret;
}

/**
 * @brief SD内文件和文件夹有效性
 */
void SdDirInit(void)
{
    type_action_t   action;
    u16             actionSize          = sizeof(type_action_t) - sizeof(type_curve_t *);
    u8              *data               = RT_NULL;

    //检测文件夹可读性
    CheckDirectory(DOWNLOAD_DIR);
    CheckDirectory(MODULE_DIR);
    CheckDirectory(TEST_DIR);
    CheckDirectory(SETTING_DIR);

    //校验文件里面的数据
    SettingFileInit(ACTION_FILE);
    //增加系统曲线
    data = rt_malloc(SAVE_ACTION_MAX_ZISE);
    if(RT_NULL != data)
    {
        action.curve = rt_malloc(sizeof(type_curve_t));
        rt_memset(data, 0, SAVE_ACTION_MAX_ZISE);//Justin debug 仅仅测试
        action.id = 0;
        action.curve_length = 1;
        action.current_value = 0;
        action.curve[0].start_value = 0;
        action.curve[0].end_value = 0;
        action.curve[0].time = 0;
        rt_memcpy(data, (u8 *)&action, actionSize);
        rt_memcpy(&data[actionSize], (u8 *)action.curve, sizeof(type_curve_t));
        action.crc = usModbusRTU_CRC(data + 2, SAVE_ACTION_MAX_ZISE - 2);//crc 占两位，crc不加入校验
//        AddActionToSD(action);
        Action[0] = action;

        rt_thread_mdelay(100);//Justin debug

        rt_memset(data, 0, SAVE_ACTION_MAX_ZISE);
        action.id = 1;
        action.curve_length = 1;
        action.current_value = 1;
        action.curve[0].start_value = 1;
        action.curve[0].end_value = 1;
        action.curve[0].time = 0;
        rt_memcpy(data, (u8 *)&action, actionSize);
        rt_memcpy(&data[actionSize], (u8 *)action.curve, sizeof(type_curve_t));
        action.crc = usModbusRTU_CRC(data + 2, SAVE_ACTION_MAX_ZISE - 2);//crc 占两位，crc不加入校验
//        AddActionToSD(action);
        Action[1] = action;
    }
    SettingFileInit(CONDITION_FILE);
    SettingFileInit(EXCUTE_FILE);
    SettingFileInit(DOTASK_FILE);

    rt_free(data);
    data = RT_NULL;
}

static void SdOperateInit(void)
{
    LOG_D("SdOperateInit function");

    operate.action_op.GetActionSum = GetActionSum;
    operate.action_op.AddActionToSD = AddActionToSD;
    operate.action_op.TakeAction = TakeAction;
    operate.action_op.FindActionById = FindActionById;

    operate.condition_op.GetConditionSum = GetConditionSum;
    operate.condition_op.AddConditionToSD = AddConditionToSD;
    operate.condition_op.TakeCondition = TakeCondition;
    operate.condition_op.FindConditionById = FindConditionById;

    operate.excute_op.GetExcuteSum = GetExcuteSum;
    operate.excute_op.AddExcuteToSD = AddExcuteToSD;
    operate.excute_op.TakeExcute = TakeExcute;
    operate.excute_op.FindExcuteById = FindExcuteById;

    operate.dotask_op.GetDotaskSum = GetDotaskSum;
    operate.dotask_op.AddDotaskToSD = AddDotaskToSD;
    operate.dotask_op.TakeDotask = TakeDotask;
    operate.dotask_op.FindDotaskById = FindDotaskById;
}

type_sdoperate_t GetSdOperate(void)
{
    static u8 init = NO;

    if(NO == init)
    {
        GetActionSum();
        GetExcuteSum();
        GetConditionSum();
        GetDotaskSum();

        SdOperateInit();
        init = YES;
    }

    return operate;
}
