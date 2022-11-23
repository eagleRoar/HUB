/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#include "Uart.h"
#include "Module.h"
#include "SDCardBusiness.h"
#include "CloudProtocol.h"

/**
 * @brief 检测文件夹是否存在
 * @return 返回是否有效,成功为RT_EOK
 */
static u8 CheckDirectory(char* name)
{
    int ret;

    ret = access(name, 0);
    if (ret < 0) {
        LOG_E("\"%s\" error, reset the dir", name);
        //创建文件夹
        ret = mkdir(name, 0x777);
        if (ret < 0) {
            LOG_E("mkdir \"%s\" error", name);
            return RT_ERROR;
        } else {
            return RT_EOK;
        }
    } else {
        return RT_EOK;
    }
}

/**
 * 如果该文件还没有使用的标记返回No ,否则Yes
 */
static u8 CheckFileAndMark(char* file_name)
{
    u8 ret = YES;
    u32 test = 0x00000000;

    if(RT_EOK == ReadSdData(file_name, (u8 *)&test, 0, SD_HEAD_SIZE))
    {
        LOG_D("CheckFileAndMark test = %x",test);
        if(SD_HEAD_CORE != test)
        {
            //如果该文件未写过，需要先写入标志
            test = SD_HEAD_CORE;
            WriteSdData(file_name, (u8 *)&test, 0, SD_HEAD_SIZE);
            ret = NO;
        }
    }
    else
    {
        LOG_E("CheckFileAndMark read fail");
    }

    return ret;
}

/**
 * 初始化sd卡相关设置
 */
void InitSDCard(void)
{
    u8              size            = 0;
    u16             monitorSize     = sizeof(type_monitor_t);
    u16             syssetSize      = sizeof(sys_set_t);
    u16             recipeSize      = sizeof(sys_recipe_t);
    u16             tankListSize    = sizeof(sys_tank_t);

    //检查文件是否存在
    CheckDirectory(MODULE_DIR);
    //检查存储app的文件夹
    CheckDirectory(DOWNLOAD_DIR);

    LOG_D("size of type_monitor_t = %d",monitorSize);

    //检查文件是否存在
    if(NO == CheckFileAndMark(MODULE_FILE))
    {
        if(RT_ERROR == WriteSdData(MODULE_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 1");
        }

        //初始化monitor文件存储monitor的空间,都置0
        rt_memset((u8 *)GetMonitor(), 0, monitorSize);
        GetMonitor()->crc = usModbusRTU_CRC((u8 *)GetMonitor(), monitorSize - 2);
        if(RT_ERROR == WriteSdData(MODULE_FILE, (u8 *)GetMonitor(), SD_INFOR_SIZE, monitorSize))
        {
            LOG_E("InitSDCard err 2");
        }

    }

    if(NO == CheckFileAndMark(SYSSET_FILE))
    {
        if(RT_ERROR == WriteSdData(SYSSET_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 1");
        }

        rt_memset((u8 *)GetSysSet(), 0, syssetSize);
        initCloudProtocol();
        GetSysSet()->crc = usModbusRTU_CRC((u8 *)GetSysSet() + 2, syssetSize - 2);
        if(RT_ERROR == WriteSdData(SYSSET_FILE, (u8 *)GetSysSet(), SD_INFOR_SIZE, syssetSize))
        {
            LOG_E("InitSDCard err 3");
        }

        LOG_D("GetSysSet()->crc = %x",GetSysSet()->crc);
    }

    if(NO == CheckFileAndMark(RECIPE_FILE))
    {
        if(RT_ERROR == WriteSdData(RECIPE_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 4");
        }

        initSysRecipe();
        GetSysRecipt()->crc = usModbusRTU_CRC((u8 *)GetSysRecipt() + 2, recipeSize - 2);
        if(RT_ERROR == WriteSdData(RECIPE_FILE, (u8 *)GetSysRecipt(), SD_INFOR_SIZE, recipeSize))
        {
            LOG_E("InitSDCard err 5");
        }
    }

    if(NO == CheckFileAndMark(TANK_FILE))
    {
        if(RT_ERROR == WriteSdData(TANK_FILE, &size, SD_HEAD_SIZE, SD_PAGE_SIZE))
        {
            LOG_E("InitSDCard err 6");
        }

        initSysTank();
        GetSysTank()->crc = usModbusRTU_CRC((u8 *)GetSysTank() + 2, tankListSize - 2);
        if(RT_ERROR == WriteSdData(TANK_FILE, (u8 *)GetSysTank(), SD_INFOR_SIZE, tankListSize))
        {
            LOG_E("InitSDCard err 7");
        }
    }
}

rt_err_t SaveModule(type_monitor_t *monitor)
{
    u8      index           = 0;
    u16     sensorSize      = sizeof(sensor_t);//为module结构体大小
    u16     deviceSize      = sizeof(device_t);//为module结构体大小
    u16     LineSize        = sizeof(line_t);
    u16     monitorSize     = sizeof(type_monitor_t);
    rt_err_t ret = RT_EOK;

    for(index = 0; index < monitor->sensor_size; index++)
    {
        if(NO == monitor->sensor[index].save_state)
        {
            monitor->sensor[index].save_state = YES;
            monitor->sensor[index].crc = usModbusRTU_CRC((u8 *)&(monitor->sensor[index]) + 2, sensorSize - 2);//前两位是crc
        }
    }

    for(index = 0; index < monitor->device_size; index++)
    {
        if(NO == monitor->device[index].save_state)
        {
            monitor->device[index].save_state = YES;
            monitor->device[index].crc = usModbusRTU_CRC((u8 *)&(monitor->device[index]) + 2, deviceSize - 2);//前两位是crc
        }
    }

    for(index = 0; index < monitor->line_size; index++)
    {
        if(NO == monitor->line[index].save_state)
        {
            monitor->line[index].save_state = YES;
            monitor->line[index].crc = usModbusRTU_CRC((u8 *)&(monitor->line[index]) + 2, LineSize - 2);//前两位是crc
        }
    }

    monitor->crc = usModbusRTU_CRC((u8 *)monitor, monitorSize - 2);

    if(RT_EOK != WriteSdData(MODULE_FILE, (u8 *)monitor, SD_INFOR_SIZE, monitorSize))
    {
        ret = RT_ERROR;
    }

    return ret;
}

void printsensor1(sensor_t sen)
{
    LOG_W("name = %s",sen.name);
    rt_kprintf("name = : ");
    for(int i = 0; i < MODULE_NAMESZ; i++)
    {
        rt_kprintf("%x ",sen.name[i]);
    }
    rt_kprintf("\r\n");
    LOG_W("addr = %x",sen.addr);
    LOG_W("type = %x",sen.type);
    LOG_W("ctrl_addr = %x",sen.ctrl_addr);
    LOG_W("conn_state = %x",sen.conn_state);
    LOG_W("reg_state = %x",sen.reg_state);
    LOG_W("save_state = %x",sen.save_state);
    LOG_W("storage_size = %x",sen.storage_size);
    for(int index = 0; index < SENSOR_VALUE_MAX; index++)
    {
        LOG_W("sto %d , name = %s",index,sen.__stora[index].name);
        LOG_W("sto %d , func = %x",index,sen.__stora[index].func);
        LOG_W("sto %d , value = %x",index,sen.__stora[index].value);
    }
}

void printline1(line_t line)
{
    LOG_W("name = %s",line.name);
    for(int i = 0; i < MODULE_NAMESZ; i++)
    {
        rt_kprintf("%x ",line.name[i]);
    }
    rt_kprintf("\r\n");
    LOG_W("type = %x",line.type);
    LOG_W("uuid = %x",line.uuid);
    LOG_W("addr = %x",line.addr);
    LOG_W("ctrl_addr = %x",line.ctrl_addr);
    LOG_W("d_state = %x",line.d_state);
    LOG_W("d_value = %x",line.d_value);
    LOG_W("save_state = %x",line.save_state);
    LOG_W("conn_state = %x",line.conn_state);
    LOG_W("manual = %x",line._manual.manual);
    LOG_W("manual_on_time = %x",line._manual.manual_on_time);
    LOG_W("manual_on_time_save = %x",line._manual.manual_on_time_save);

}


rt_err_t TakeMonitorFromSD(type_monitor_t *monitor)
{
    u16         monitorSize     = sizeof(type_monitor_t);
    u16         crc             = 0;
    rt_err_t    ret             = RT_ERROR;
    u16         deviceCrc       = 0;


    //1.初始化monitor
    initMonitor();

    if(RT_EOK == ReadSdData(MODULE_FILE, (u8 *)monitor, SD_INFOR_SIZE, monitorSize))
    {
        crc = usModbusRTU_CRC((u8 *)monitor, monitorSize - 2);//crc 在最后
        LOG_D("TakeMonitorFromSD crc = %x,monitor->crc = %x",crc,monitor->crc);

        if(monitor->crc == crc)
        {
            initModuleConState(monitor);
            ret = RT_EOK;
        }
        else
        {
            for(u8 index = 0; index < monitor->device_size; index++)
            {
                deviceCrc = usModbusRTU_CRC((u8 *)&monitor->device[index] + 2, sizeof(device_t) - 2);
                if(deviceCrc != monitor->device[index].crc)
                {
                    LOG_E("device name %s crc err, crc = %x, sd crc = %x",
                            monitor->device[index].name,deviceCrc,monitor->device[index].crc);
                }
            }

            for(u8 index = 0; index < monitor->sensor_size; index++)
            {
                deviceCrc = usModbusRTU_CRC((u8 *)&monitor->sensor[index] + 2, sizeof(sensor_t) - 2);
                if(deviceCrc != monitor->sensor[index].crc)
                {
                    LOG_E("sensor name %s crc err, crc = %x, sd crc = %x",
                            monitor->sensor[index].name,deviceCrc,monitor->sensor[index].crc);
                    printsensor1(monitor->sensor[index]);
                }
            }

            for(u8 index = 0; index < monitor->line_size; index++)
            {
                deviceCrc = usModbusRTU_CRC((u8 *)&monitor->line[index] + 2, sizeof(line_t) - 2);
                if(deviceCrc != monitor->line[index].crc)
                {
                    LOG_E("line name %s crc err, crc = %x, sd crc = %x",
                            monitor->line[index].name,deviceCrc,monitor->line[index].crc);
                    printline1(monitor->line[index]);
                }
            }

            initMonitor();
            ret = RT_ERROR;
        }
    }
    else
    {
        LOG_E("TakeMonitorFromSD err 1");
    }

   return ret;
}

rt_err_t SaveSysRecipe(sys_recipe_t *recipe)
{
    rt_err_t    ret             = RT_EOK;
    u16         sysRecipeSize   = sizeof(sys_recipe_t);

    recipe->crc = usModbusRTU_CRC((u8 *)recipe + 2, sysRecipeSize - 2);

    if(RT_EOK != WriteSdData(RECIPE_FILE, (u8 *)recipe, SD_INFOR_SIZE, sysRecipeSize))
    {
        LOG_E("SaveSysRecipe err");
        ret = RT_ERROR;
    }
    else
    {
        LOG_D("save recipe ok");
    }

    return ret;
}

rt_err_t TackRecipeFromSD(sys_recipe_t *rec)
{
    rt_err_t    ret             = RT_ERROR;
    u16         sysRecSize      = sizeof(sys_recipe_t);
    u16         crc             = 0;

    rt_memset((u8 *)rec, 0, sizeof(sys_recipe_t));

    if(RT_EOK == ReadSdData(RECIPE_FILE, (u8 *)rec, SD_INFOR_SIZE, sysRecSize))
    {
        crc = usModbusRTU_CRC((u8 *)rec + 2, sysRecSize - 2);  //crc 在最后

        if(crc == rec->crc)
        {
            ret = RT_EOK;
        }
        else
        {
            initSysRecipe();
            rec->saveFlag = YES;
            ret = RT_ERROR;
        }
    }

    return ret;
}

rt_err_t SaveSysTank(sys_tank_t *tank)
{
    rt_err_t    ret             = RT_EOK;
    u16         sysTankSize     = sizeof(sys_tank_t);

    tank->crc = usModbusRTU_CRC((u8 *)tank + 2, sysTankSize - 2);

    if(RT_EOK != WriteSdData(TANK_FILE, (u8 *)tank, SD_INFOR_SIZE, sysTankSize))
    {
        LOG_E("SaveSysTank err");
        ret = RT_ERROR;
    }
    else
    {
        LOG_D("save tank ok");
    }

    return ret;
}

rt_err_t TackSysTankFromSD(sys_tank_t *tank)
{
    rt_err_t    ret             = RT_ERROR;
    u16         sysTankSize     = sizeof(sys_tank_t);
    u16         crc             = 0;

    rt_memset((u8 *)tank, 0, sizeof(sys_tank_t));

    if(RT_EOK == ReadSdData(TANK_FILE, (u8 *)tank, SD_INFOR_SIZE, sysTankSize))
    {
        crc = usModbusRTU_CRC((u8 *)tank + 2, sysTankSize - 2);  //crc 在最后

        if(crc == tank->crc)
        {
            ret = RT_EOK;
        }
        else
        {
            initSysTank();
            ret = RT_ERROR;
        }
    }

    return ret;
}

