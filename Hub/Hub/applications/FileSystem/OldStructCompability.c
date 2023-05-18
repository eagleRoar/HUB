#include "FileSystem.h"
#include "cJSON.h"
#include "CloudProtocolBusiness.h"
#include "SDCardData.h"
#include "Module.h"
#include "Uart.h"
#include "recipe.h"

//按照原来的数据结构的
void OldDataMigration(void)
{
    char            old_dev_file[]      = "/backup/moduleInfo/module.bin";
    char            old_sysset_file[]   = "/backup/moduleInfo/sys_set.bin";
    char            old_recipe_file[]   = "/backup/moduleInfo/recipe.bin";
    char            old_tank_file[]     = "/backup/moduleInfo/tank.bin";
    static u8       FileHeadSpace       = 5;
    u8              temp                = 0;
    type_monitor_t  *newMonitor         = GetMonitor();
    u16             lenght              = 0;
    struct allocate_old     allocateStr;
    u8              size[3];
    sensorOld_t     sensor;
    deviceOld_t     device;
    lineOld_t       line;
    sys_set_t       *newSet             = GetSysSet();
    sys_setOld_t    oldSet;
    sys_recipe_t    *newRecipeList      = GetSysRecipt();
    recipeOld_t     oldRecipe;
    sys_tank_t      *newTankList        = GetSysTank();
    tankOld_t       oldTank;

    //读取旧数据
    //1.monitor
    lenght = FileHeadSpace;
    if(RT_EOK == ReadFileData(old_dev_file, &allocateStr, lenght, sizeof(struct allocate_old)))
    {
        rt_memcpy((u8 *)&newMonitor->allocateStr, (u8 *)&allocateStr, sizeof(struct allocate_old));
    }
    lenght += sizeof(struct allocate_old);

    if(RT_EOK == ReadFileData(old_dev_file, size, lenght, sizeof(size)))
    {
        newMonitor->sensor_size = size[0];
        newMonitor->device_size = size[1];
        newMonitor->line_size = size[2];
        //LOG_I("sensor_size %d, device_size %d, line_size %d",newMonitor->sensor_size,newMonitor->device_size,newMonitor->line_size);
    }
    lenght += sizeof(size);

    temp = newMonitor->sensor_size < SENSOR_MAX ? newMonitor->sensor_size : SENSOR_MAX;
    for(int i = 0; i < temp; i++)
    {
        if(RT_EOK == ReadFileData(old_dev_file, &sensor, lenght + sizeof(sensorOld_t) * i, sizeof(sensorOld_t)))
        {
            newMonitor->sensor[i].crc = sensor.crc;
            newMonitor->sensor[i].uuid = sensor.uuid;
            strncpy(newMonitor->sensor[i].name, sensor.name, MODULE_NAMESZ);
            newMonitor->sensor[i].addr = sensor.addr;
            newMonitor->sensor[i].type = sensor.type;
            newMonitor->sensor[i].ctrl_addr = sensor.ctrl_addr;
            newMonitor->sensor[i].conn_state = sensor.conn_state;
            newMonitor->sensor[i].reg_state = sensor.reg_state;
            newMonitor->sensor[i].save_state = sensor.save_state;
            newMonitor->sensor[i].storage_size = sensor.storage_size;
            newMonitor->sensor[i].isMainSensor = NO;
            rt_memcpy(newMonitor->sensor[i].__stora, sensor.__stora, sizeof(sen_storaOld_t) * 4);

            //printSensor(newMonitor->sensor[i]);
        }

    }
    lenght += sizeof(sensorOld_t) * 20;

    temp = newMonitor->device_size < DEVICE_MAX ? newMonitor->device_size : DEVICE_MAX;
    for(int i = 0; i < temp; i++)
    {
        if(RT_EOK == ReadFileData(old_dev_file, &device, lenght + sizeof(deviceOld_t) * i, sizeof(deviceOld_t)))
        {

            newMonitor->device[i].crc = device.crc;
            newMonitor->device[i].uuid = device.uuid;
            strncpy(newMonitor->device[i].name, device.name, MODULE_NAMESZ);
            newMonitor->device[i].addr = device.addr;
            newMonitor->device[i].type = device.type;
            newMonitor->device[i].ctrl_addr = device.ctrl_addr;
            newMonitor->device[i].main_type = device.main_type;
            newMonitor->device[i].conn_state = device.conn_state;
            newMonitor->device[i].reg_state = device.reg_state;
            newMonitor->device[i].save_state = device.save_state;
            newMonitor->device[i].storage_size = device.storage_size;
            newMonitor->device[i].color = device.color;
            for(int port = 0; port < 12; port++)
            {
                strncpy(newMonitor->device[i].port[port].name, device.port[port].name, 9);
                newMonitor->device[i].port[port].addr = device.port[port].addr;
                newMonitor->device[i].port[port].type = device.port[port].type;
                newMonitor->device[i].port[port].hotStartDelay = device.port[port].hotStartDelay;
                newMonitor->device[i].port[port].mode = device.port[port].mode;
                newMonitor->device[i].port[port].func = device.port[port].func;
                rt_memcpy(newMonitor->device[i].port[port].timer, device.port[port].timer, sizeof(type_timmerOld_t));
                rt_memcpy((u8 *)&newMonitor->device[i].port[port].cycle, (u8 *)&device.port[port].cycle, sizeof(type_cycleOld_t));
                rt_memset((u8 *)&newMonitor->device[i].port[port].cycle1, 0, sizeof(type_cycleOld_t));
                rt_memcpy((u8 *)&newMonitor->device[i].port[port].manual, (u8 *)&device.port[port].manual, sizeof(type_manualOld_t));
                rt_memcpy((u8 *)&newMonitor->device[i].port[port].ctrl, (u8 *)&device.port[port].ctrl, sizeof(type_ctrlOld_t));
            }
            newMonitor->device[i]._hvac.manualOnMode = device._hvac.manualOnMode;
            newMonitor->device[i]._hvac.fanNormallyOpen = device._hvac.fanNormallyOpen;
            newMonitor->device[i]._hvac.hvacMode = device._hvac.hvacMode;

            //printDevice(newMonitor->device[i]);
        }

    }
    lenght += sizeof(deviceOld_t) * 16;

    temp = newMonitor->line_size < LINE_MAX ? newMonitor->line_size : LINE_MAX;
    for(int i = 0; i < temp; i++)
    {
        if(RT_EOK == ReadFileData(old_dev_file, &line, lenght + sizeof(lineOld_t) * i, sizeof(lineOld_t)))
        {
            newMonitor->line[i].crc = line.crc;
            newMonitor->line[i].type = line.type;
            newMonitor->line[i].uuid = line.uuid;
            strncpy(newMonitor->line[i].name, line.name, MODULE_NAMESZ);
            newMonitor->line[i].addr = line.addr;
            newMonitor->line[i].ctrl_addr = line.ctrl_addr;
            newMonitor->line[i].port[0].ctrl.d_state = line.d_state;
            newMonitor->line[i].port[0].ctrl.d_value = line.d_value;
            rt_memcpy((u8 *)&newMonitor->line[i].port[0]._manual, (u8 *)&line._manual, sizeof(type_manual_t));
            newMonitor->line[i].storage_size = 1;
            newMonitor->line[i].save_state = line.save_state;
            newMonitor->line[i].conn_state = line.conn_state;
            newMonitor->line[i].lineNo = i + 1;
        }
    }
    lenght += sizeof(lineOld_t) * 2;

    ReadFileData(old_dev_file, &newMonitor->crc, lenght, 2);

    //2.获取sys_set
    lenght = FileHeadSpace;
    if(RT_EOK == ReadFileData(old_sysset_file, &oldSet, lenght, sizeof(sys_setOld_t)))
    {
        rt_memset((u8 *)newSet, 0, sizeof(sys_set_t));

        newSet->crc = oldSet.crc;
//        LOG_I("crc = %x %x", oldSet.crc,newSet->crc);
        rt_memcpy((u8 *)&newSet->tempSet, (u8 *)&oldSet.tempSet, sizeof(proTempSetOld_t));
//        LOG_I("temp setting %d %d %d %d %d %d %d %d",
//                oldSet.tempSet.dayCoolingTarget, newSet->tempSet.dayCoolingTarget,
//                oldSet.tempSet.dayHeatingTarget,newSet->tempSet.dayHeatingTarget,
//                oldSet.tempSet.nightCoolingTarget,newSet->tempSet.nightCoolingTarget,
//                oldSet.tempSet.nightHeatingTarget,newSet->tempSet.nightHeatingTarget);
        rt_memcpy((u8 *)&newSet->co2Set, (u8 *)&oldSet.co2Set, sizeof(proCo2SetOld_t));
//        LOG_I("co2 setting %d %d %d %d",
//                oldSet.co2Set.dayCo2Target,newSet->co2Set.dayCo2Target,
//                oldSet.co2Set.nightCo2Target,newSet->co2Set.nightCo2Target);
        rt_memcpy((u8 *)&newSet->humiSet, (u8 *)&oldSet.humiSet, sizeof(proHumiSetOld_t));
//        LOG_I("humiSet setting %d %d %d %d %d %d %d %d",
//                oldSet.humiSet.dayDehumiTarget,newSet->humiSet.dayDehumiTarget,
//                oldSet.humiSet.dayHumiTarget,newSet->humiSet.dayHumiTarget,
//                oldSet.humiSet.nightDehumiTarget,newSet->humiSet.nightDehumiTarget,
//                oldSet.humiSet.nightHumiTarget,newSet->humiSet.nightHumiTarget);
        rt_memcpy((u8 *)&newSet->line1Set, (u8 *)&oldSet.line1Set, sizeof(proLineOld_t));

        newSet->line1_4Set.brightMode = LINE_MODE_BY_POWER;
        newSet->line1_4Set.byAutoDimming = AUTO_DIMMING;
        newSet->line1_4Set.mode = 1;
        newSet->line1_4Set.tempStartDimming = 350;
        newSet->line1_4Set.tempOffDimming = 400;
        newSet->line1_4Set.sunriseSunSet = 10;
        rt_memcpy((u8 *)&newSet->line2Set, (u8 *)&oldSet.line2Set, sizeof(proLineOld_t));
        rt_memcpy((u8 *)&newSet->stageSet, (u8 *)&oldSet.stageSet, sizeof(stageOld_t));
//        LOG_I("---------- en = %d %d, stage start %s %s, duration_day = %d %d",
//                oldSet.stageSet.en , newSet->stageSet.en,
//                oldSet.stageSet.starts, newSet->stageSet.starts,
//                oldSet.stageSet._list[0].duration_day, newSet->stageSet._list[0].duration_day);
        rt_memcpy((u8 *)&newSet->sysPara, (u8 *)&oldSet.sysPara, sizeof(sys_paraOld_t));

        rt_memcpy(newSet->co2Cal, oldSet.co2Cal, sizeof(oldSet.co2Cal));
//        for(int port = 0; port < 20; port++)
//        {
//            printf("......co2 %d , value = %d\r\n",port,newSet->co2Cal[port]);
//        }
        for(int port = 0; port < 20; port++)
        {
            newSet->ph[port].ph_a = oldSet.ph[port].ph_a;
            newSet->ph[port].ph_b = oldSet.ph[port].ph_b;
            newSet->ph[port].uuid = oldSet.ph[port].uuid;
//            printf("ph %d a = %f, b = %f, uuid = %d\r\n",port,newSet->ph[port].ph_a,newSet->ph[port].ph_b,newSet->ph[port].uuid);
        }

        for(int port = 0; port < 20; port++)
        {
            newSet->ec[port].ec_a = oldSet.ec[port].ec_a;
            newSet->ec[port].ec_b = oldSet.ec[port].ec_b;
            newSet->ec[port].uuid = oldSet.ec[port].uuid;
//            printf("--ec %d a = %f, b = %f, uuid = %d\r\n",port,newSet->ph[port].ph_a,newSet->ph[port].ph_b,newSet->ph[port].uuid);
        }
        newSet->startCalFlg = oldSet.startCalFlg;
        rt_memcpy((u8 *)&newSet->hub_info, (u8 *)&oldSet.hub_info, sizeof(hubOld_t));
        newSet->ver = HUB_VER_NO;
//        printf("hub name = %s , %s\r\n",oldSet.hub_info.name, newSet->hub_info.name);
    }

    //3.获取recipe
    lenght = FileHeadSpace;
    ReadFileData(old_recipe_file, &newRecipeList->crc, lenght, 2);
    lenght += 2;

    ReadFileData(old_recipe_file, &newRecipeList->recipe_size, lenght, 1);
    lenght += 1;

    for(int i = 0; i < 10; i++)
    {
        rt_memset((u8 *)&newRecipeList->recipe[i], 0, sizeof(recipe_t));
        if(RT_EOK == ReadFileData(old_recipe_file, &oldRecipe, lenght, sizeof(recipeOld_t)))
        {
            rt_memcpy((u8 *)&newRecipeList->recipe[i], (u8 *)&oldRecipe, sizeof(recipeOld_t));
            newRecipeList->recipe[i].line_4.brightMode = LINE_MODE_BY_POWER;
            newRecipeList->recipe[i].line_4.byAutoDimming = AUTO_DIMMING;
            newRecipeList->recipe[i].line_4.mode = 1;
            newRecipeList->recipe[i].line_4.tempStartDimming = 350;
            newRecipeList->recipe[i].line_4.tempOffDimming = 400;
            newRecipeList->recipe[i].line_4.sunriseSunSet = 10;

//            printRecipe(&newRecipeList->recipe[i]);
        }

        lenght += sizeof(recipeOld_t);
    }

    ReadFileData(old_recipe_file, newRecipeList->allot_add, lenght, 10);
    lenght += 10;

    ReadFileData(old_recipe_file, &newRecipeList->saveFlag, lenght, 1);

    //4.获取tank
    lenght = FileHeadSpace;
    ReadFileData(old_tank_file, &newTankList->crc, lenght, 2);
    lenght += 2;

    ReadFileData(old_tank_file, &newTankList->tank_size, lenght, 1);
    lenght += 1;

    for(int i = 0; i < 4; i++)
    {
        if(RT_EOK == ReadFileData(old_tank_file, &oldTank, lenght, sizeof(tankOld_t)))
        {
            newTankList->tank[i].tankNo = oldTank.tankNo;
            strncpy(newTankList->tank[i].name, oldTank.name, TANK_NAMESZ);
            newTankList->tank[i].autoFillValveId = oldTank.autoFillValveId;
            newTankList->tank[i].autoFillHeight = oldTank.autoFillHeight;
            newTankList->tank[i].autoFillFulfilHeight = oldTank.autoFillFulfilHeight;
            newTankList->tank[i].highEcProtection = oldTank.highEcProtection;
            newTankList->tank[i].lowPhProtection = oldTank.lowPhProtection;
            newTankList->tank[i].highPhProtection = oldTank.highPhProtection;
            newTankList->tank[i].phMonitorOnly = oldTank.phMonitorOnly;
            newTankList->tank[i].ecMonitorOnly = oldTank.ecMonitorOnly;
            newTankList->tank[i].wlMonitorOnly = oldTank.wlMonitorOnly;
            newTankList->tank[i].pumpId = oldTank.pumpId;

            temp = 16 < VALVE_MAX ? 16 : VALVE_MAX;
            for(int port = 0; port < temp; port++)
            {
                newTankList->tank[i].valve[port] = oldTank.valve[port];
            }
            rt_memset(newTankList->tank[i].nopump_valve, 0, sizeof(newTankList->tank[i].nopump_valve));

            temp = 2 < TANK_SINGLE_GROUD ? 2 : TANK_SINGLE_GROUD;
            u8 temp1 = 4 < TANK_SENSOR_MAX ? 4 : TANK_SENSOR_MAX;
            for(int port = 0; port < temp; port++)
            {
                for(int port1 = 0; port1 < temp1; port1++)
                {
                    newTankList->tank[i].sensorId[port][port1] = oldTank.sensorId[port][port1];
                }
            }
            newTankList->tank[i].color = oldTank.color;
            newTankList->tank[i].poolTimeout = oldTank.poolTimeout;
        }

        lenght += sizeof(tankOld_t);
    }

    ReadFileData(old_tank_file, &newTankList->saveFlag, lenght, 1);
}
