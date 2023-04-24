/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-09     Administrator       the first version
 */

#include "FileSystem.h"
#include "cJSON.h"
#include "CloudProtocolBusiness.h"
#include "SDCardData.h"
#include "Module.h"
#include "Uart.h"
#include "recipe.h"

#define         BUFF_MAX_SZ        1024
__attribute__((section(".ccmbss"))) u8 file_sys_task[1024 * 8];
__attribute__((section(".ccmbss"))) struct rt_thread file_sys_thread;
rt_mutex_t dynamic_mutex = RT_NULL;
static u8 fileSystemState = NO;

u8 GetFileSystemState(void)
{
    return fileSystemState;
}

static void SetFileSystemState(u8 state)
{
    fileSystemState = state;
}

/**
 * @brief 从文件中读取到固定位置的数据
 *
 * @param name 需要读取的文件
 * @param text 返回的数据
 * @param offset 偏移量
 * @param l 文件读取的长度
 * @return u8
 */
u8 ReadFileData(char* name, void* text, u32 offset, u32 l)
{
    int     fd;
    int     size;
    u8      index       = 0;
    u8      ret         = RT_EOK;

    /*生成文件名称*/

    fd = open(name, O_RDONLY | O_CREAT);
    if (fd >= 0) {
        lseek(fd,offset,SEEK_SET);//设置偏移地址

        if(l <= BUFF_MAX_SZ)
        {
            size = read(fd, text, l);
        }
        else
        {
            for(index = 0; index < l/BUFF_MAX_SZ; index++)
            {
                size = read(fd, text + index * BUFF_MAX_SZ, BUFF_MAX_SZ);
            }

            if((l % BUFF_MAX_SZ) > 0)
            {
                size = read(fd, text + index * BUFF_MAX_SZ, l % BUFF_MAX_SZ);
            }
        }

        if(0 == close(fd))
        {
        }
        else
        {
        }

        if (size > 0) {
            ret = RT_EOK;
        }
    }
    else
    {
        ret = RT_ERROR;
        LOG_E("ReadFileData ERR, fd = %d",fd);
    }

    return ret;
}

/**
 * @brief 将数据写入相应文件
 *
 * @param name 写入的文件名称
 * @param offset 偏移量
 * @param text 需要写入的数据内容
 * @param l 写入长度
 * @return
 */
u8 WriteFileData(char* name, void* text, u32 offset, u32 l)
{
    int     fd;
    u8      index       = 0;
    u8      ret         = RT_EOK;


    if (text != NULL) {
        /*生成文件名称*/
        /* 以创建和读写模式打开 name 文件，如果该文件不存在则创建该文件*/
        fd = open(name, O_WRONLY | O_CREAT);
        if (fd >= 0) {
            lseek(fd,offset,SEEK_SET);//设置偏移地址

            if(l <= BUFF_MAX_SZ)
            {
                write(fd, text, l);
            }
            else
            {
                for(index = 0; index < l/BUFF_MAX_SZ; index++)
                {
                    write(fd, text + index * BUFF_MAX_SZ, BUFF_MAX_SZ);
                }

                if((l % BUFF_MAX_SZ) > 0)
                {
                    write(fd, text + index * BUFF_MAX_SZ, l % BUFF_MAX_SZ);
                }
            }

            if(0 == close(fd))
            {
                LOG_D("write fd close ok");
            }
            else
            {
                LOG_E("write fd close fail");
            }

            ret = RT_EOK;
        }
        else
        {
            LOG_E(" WriteFileData ERR 1");
        }
    }
    else
    {
        ret = RT_ERROR;
        LOG_E(" WriteFileData ERR 2");
    }


    return ret;
}

void RemoveFileDirectory(char *fileName)
{
    rmdir(fileName);
}

/**
 * @brief 检测文件是否存在,并获取文件长度
 * @param name:相关文件名称
 * @return 返回相关文件长度
 */
u32 GetFileLength(char* name)
{
    int ret;
    struct stat buf;

    ret = stat(name, &buf);

    if (ret == 0) {
        //LOG_D("%s file size = %d", name, buf.st_size);
        return buf.st_size;
    } else {
        //LOG_E("%s file not found");
        return 0;
    }
}

rt_err_t CheckDirectory(char* name)
{
    int ret;

    ret = access(name, 0);
    if (ret < 0) {
        //LOG_E("\"%s\" no exist", name);
        return RT_ERROR;
    } else {
        return RT_EOK;
    }
}

rt_err_t CreateDirectory(char* name)
{
    int ret;

    ret = mkdir(name, 0x777);
    if (ret < 0) {
        //LOG_E("mkdir \"%s\" error", name);
        return RT_ERROR;
    } else {
        return RT_EOK;
    }
}

static void SaveMonitorSizeByJson(u8 sen_size, u8 dev_size, u8 line_size)
{
    char        reg_size_file[]     = "/main/devRegistry/reg_size.bin";//新版本设备注册表存储文件夹
    cJSON       *cjson              = RT_NULL;
    char        *str                = RT_NULL;

    LOG_I("SaveMonitorSizeByJson");
    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "sensor_size", sen_size);
        cJSON_AddNumberToObject(cjson, "device_size", dev_size);
        cJSON_AddNumberToObject(cjson, "line_size", line_size);

        str = cJSON_PrintUnformatted(cjson);
        //回收空间
        cJSON_Delete(cjson);
        if(str)
        {
            RemoveFileDirectory(reg_size_file);
            if(RT_ERROR == WriteFileData(reg_size_file, str, 0, strlen(str)))
            {
                LOG_E("write data to flash fail");
            }
            cJSON_free(str);
            str = RT_NULL;
        }
    }
}

static void SaveMonitorAddrByJson(struct allocate all_addr)
{
    char        reg_addr_file[]     = "/main/devRegistry/reg_addr.bin";//新版本设备注册表存储文件夹
    cJSON       *cjson              = RT_NULL;
    cJSON       *item               = RT_NULL;
    char        *str                = RT_NULL;

    LOG_I("SaveMonitorAddrByJson");
    //以Json 格式存储到主存储区
    cjson = cJSON_CreateArray();
    if(cjson)
    {
        for(int i = 0; i < 128; i++)
        {
            item = cJSON_CreateNumber(all_addr.address[i]);
            if(item)
            {
                cJSON_AddItemToArray(cjson, item);
            }
        }
        str = cJSON_PrintUnformatted(cjson);
        //回收空间
        cJSON_Delete(cjson);
        if(str)
        {
            RemoveFileDirectory(reg_addr_file);
            if(RT_ERROR == WriteFileData(reg_addr_file, str, 0, strlen(str)))
            {
                LOG_E("write data to flash fail");
            }
            cJSON_free(str);
            str = RT_NULL;
        }
        else
        {
            LOG_E("SaveMonitorAddrByJson str == NULL");

        }
    }
}

/*
 * 将旧数据device类转化为cjson格式
 * saveFile : 保存的文件的文件夹位置
 */
static void SaveSensorByJson(sensor_t sensor, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    cJSON       *port               = RT_NULL;
    cJSON       *item               = RT_NULL;
    char        file[50]            = "";
    char        *str                = RT_NULL;

    //1. 是否存在sensor 文件夹
    sprintf(file, "%s%s%d", saveFile, "/sensor", no);
    if(RT_ERROR == CheckDirectory(file))
    {
        //新建存储该文件的文件夹
        CreateDirectory(file);
    }

    //2.数据转化成json格式
    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "crc", sensor.crc);
        cJSON_AddNumberToObject(cjson, "uuid", sensor.uuid);
        cJSON_AddStringToObject(cjson, "name", sensor.name);
        cJSON_AddNumberToObject(cjson, "addr", sensor.addr);
        cJSON_AddNumberToObject(cjson, "type", sensor.type);
        cJSON_AddNumberToObject(cjson, "ctrl_addr", sensor.ctrl_addr);
        cJSON_AddNumberToObject(cjson, "conn_state", sensor.conn_state);
        cJSON_AddNumberToObject(cjson, "reg_state", sensor.reg_state);
        cJSON_AddNumberToObject(cjson, "save_state", sensor.save_state);
        cJSON_AddNumberToObject(cjson, "storage_size", sensor.storage_size);
        cJSON_AddNumberToObject(cjson, "isMainSensor", sensor.isMainSensor);

        port = cJSON_CreateArray();
        if(port)
        {
            for(u8 i = 0; i < 4; i++)
            {
                item = cJSON_CreateObject();

                if(item)
                {
                    cJSON_AddStringToObject(item, "name", sensor.__stora[i].name);
                    cJSON_AddNumberToObject(item, "func", sensor.__stora[i].func);
                    cJSON_AddNumberToObject(item, "value", sensor.__stora[i].value);

                    cJSON_AddItemToArray(port, item);
                }
            }
            cJSON_AddItemToObject(cjson, "port", port);
        }

        //3. 存储sensor数据
        str = cJSON_PrintUnformatted(cjson);
        //释放空间 否则JSON 占用的空间很大
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);
//            LOG_I("file size = %d",size);
            sprintf(file, "%s%s%d%s", saveFile, "/sensor", no, "/sensor.txt");
//            LOG_D("file name = %s",file);
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveSensorByJson, write to %s err",file);
            }
            else
            {
                LOG_I("SaveSensorByJson write to file ok");

            }

            cJSON_free(str);
            str = RT_NULL;
        }
    }
}

static void SaveDevPortByJson(device_t device, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        *str                = RT_NULL;
    char        file[50]            = "";

    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddStringToObject(cjson, "name", device.port[no].name);
        cJSON_AddNumberToObject(cjson, "addr", device.port[no].addr);
        cJSON_AddNumberToObject(cjson, "type", device.port[no].type);
        cJSON_AddNumberToObject(cjson, "hotStartDelay", device.port[no].hotStartDelay);
        cJSON_AddNumberToObject(cjson, "mode", device.port[no].mode);
        cJSON_AddNumberToObject(cjson, "func", device.port[no].func);

        cJSON_AddNumberToObject(cjson, "startAt", device.port[no].cycle.startAt);
        cJSON_AddNumberToObject(cjson, "duration", device.port[no].cycle.duration);
        cJSON_AddNumberToObject(cjson, "pauseTime", device.port[no].cycle.pauseTime);
        cJSON_AddNumberToObject(cjson, "times", device.port[no].cycle.times);

        cJSON_AddNumberToObject(cjson, "manual", device.port[no].manual.manual);
        cJSON_AddNumberToObject(cjson, "manual_on_time", device.port[no].manual.manual_on_time);
        cJSON_AddNumberToObject(cjson, "manual_on_time_save", device.port[no].manual.manual_on_time_save);

        cJSON_AddNumberToObject(cjson, "d_state", device.port[no].ctrl.d_state);
        cJSON_AddNumberToObject(cjson, "d_value", device.port[no].ctrl.d_value);
    }

    str = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);
    if(str)
    {
        int size = strlen(str);
        sprintf(file, "%s%s%d%s", saveFile, "/port", no, ".txt");
        RemoveFileDirectory(file);
        if(RT_ERROR == WriteFileData(file, str, 0, size))
        {
            LOG_E("SaveDevPortByJson, write to %s err",file);
        }

        cJSON_free(str);
        str = RT_NULL;
    }
}

/**
 * @param saveFile 存储文件位置的文件夹
 */
static void SaveDevTimerByJson(device_t device, u8 no, char *saveFile)
{
    u8          i           = 0;
    cJSON       *cjson      = RT_NULL;
    cJSON       *port_i     = RT_NULL;
    char        *str        = RT_NULL;
    char        file[50]    = "";

    cjson = cJSON_CreateArray();
    if(cjson)
    {
        for(i = 0; i < 12; i++)
        {
            port_i = cJSON_CreateObject();

            if(port_i)
            {
                cJSON_AddNumberToObject(port_i, "on_at", device.port[no].timer[i].on_at);
                cJSON_AddNumberToObject(port_i, "duration", device.port[no].timer[i].duration);
                cJSON_AddNumberToObject(port_i, "en", device.port[no].timer[i].en);

                cJSON_AddItemToArray(cjson, port_i);
            }
        }
    }

    str = cJSON_PrintUnformatted(cjson);
    cJSON_Delete(cjson);
    if(str)
    {
        int size = strlen(str);
        sprintf(file, "%s%s%d%s", saveFile, "/timer", no, ".txt");
        RemoveFileDirectory(file);
        if(RT_ERROR == WriteFileData(file, str, 0, size))
        {
            LOG_E("SaveDeviceByJson, write to %s err",file);
        }

        cJSON_free(str);
        str = RT_NULL;
    }
}

/*
 * 将旧数据device类转化为cjson格式
 * saveFile : 保存的文件的文件夹位置
 */
static void SaveDeviceByJson(device_t device, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

//    LOG_W("device_t size = %d",sizeof(device_t));

    //1.是否存在该文件夹 不存在的话新建文件夹
    sprintf(file, "%s%s%d", saveFile, "/device", no);
    if(RT_ERROR == CheckDirectory(file))
    {
        //新建存储该文件的文件夹
        CreateDirectory(file);
        //新建存储timer文件夹
        sprintf(file, "%s%s%d%s", saveFile, "/device", no, "/timer");
        CreateDirectory(file);
        //新建存储port文件夹
        sprintf(file, "%s%s%d%s", saveFile, "/device", no, "/port");
        CreateDirectory(file);
    }

    //2.数据转化成json格式
    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "crc", device.crc);
        cJSON_AddNumberToObject(cjson, "uuid", device.uuid);
        cJSON_AddStringToObject(cjson, "name", device.name);
        cJSON_AddNumberToObject(cjson, "addr", device.addr);
        cJSON_AddNumberToObject(cjson, "type", device.type);
        cJSON_AddNumberToObject(cjson, "ctrl_addr", device.ctrl_addr);
        cJSON_AddNumberToObject(cjson, "main_type", device.main_type);
        cJSON_AddNumberToObject(cjson, "conn_state", device.conn_state);
        cJSON_AddNumberToObject(cjson, "reg_state", device.reg_state);
        cJSON_AddNumberToObject(cjson, "save_state", device.save_state);
        cJSON_AddNumberToObject(cjson, "storage_size", device.storage_size);
        cJSON_AddNumberToObject(cjson, "color", device.color);
        //端口

        cJSON_AddNumberToObject(cjson, "fanNormallyOpen", device._hvac.fanNormallyOpen);
        cJSON_AddNumberToObject(cjson, "hvacMode", device._hvac.hvacMode);
        cJSON_AddNumberToObject(cjson, "manualOnMode", device._hvac.manualOnMode);

        //存储device数据
        str = cJSON_PrintUnformatted(cjson);
        //释放空间 否则JSON 占用的空间很大
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);
            //LOG_I("file size = %d",size);
            sprintf(file, "%s%s%d%s", saveFile, "/device", no, "/device.txt");
            //LOG_D("file name = %s",file);
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveDeviceByJson, write to %s err",file);
            }
            else
            {
                LOG_I("SaveDeviceByJson address %x,crc = %x write to file ok",
                        device.addr,device.crc);

            }

            cJSON_free(str);
            str = RT_NULL;
        }
        else
        {
            LOG_E("SaveDeviceByJson, apply str memory fail");
        }

        //保存port
        sprintf(file, "%s%s%d%s", saveFile, "/device", no, "/port");
        u8 port_size = 0;
        port_size = device.storage_size < DEVICE_PORT_MAX ? device.storage_size : DEVICE_PORT_MAX;

        for(int i = 0; i < port_size; i++)
        {
            SaveDevPortByJson(device, i, file);
        }

        //保存timer,(由于json很费空间，需要单独存储)
        sprintf(file, "%s%s%d%s", saveFile, "/device", no, "/timer");
        for(int i = 0; i < port_size; i++)
        {
            if((device.port[i].type == VALVE_TYPE) ||
               (device.port[i].type == PUMP_TYPE) ||
               (device.port[i].type == TIMER_TYPE))
            {
                SaveDevTimerByJson(device, i, file);
            }
        }
    }
    else
    {
        LOG_E("SaveDeviceByJson, apply json memory fail");
    }
}

#if(HUB_SELECT == HUB_ENVIRENMENT)
/*
 * 将旧数据device类转化为cjson格式
 * saveFile : 保存的文件的文件夹位置
 */
static void SaveLineByJson(line_t light, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        file[50]            = "";
    char        *str                = RT_NULL;

    //1. 是否存在sensor 文件夹
    sprintf(file, "%s%s%d", saveFile, "/line", no);
    if(RT_ERROR == CheckDirectory(file))
    {
        //新建存储该文件的文件夹
        CreateDirectory(file);
    }

    //2.数据转化成json格式
    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "crc", light.crc);
        cJSON_AddNumberToObject(cjson, "type", light.type);
        cJSON_AddNumberToObject(cjson, "uuid", light.uuid);
        cJSON_AddStringToObject(cjson, "name", light.name);
        cJSON_AddNumberToObject(cjson, "addr", light.addr);
        cJSON_AddNumberToObject(cjson, "ctrl_addr", light.ctrl_addr);
        cJSON_AddNumberToObject(cjson, "save_state", light.save_state);
        cJSON_AddNumberToObject(cjson, "conn_state", light.conn_state);
        cJSON_AddNumberToObject(cjson, "storage_size", light.storage_size);

        cJSON *portList = cJSON_CreateArray();
        if(portList)
        {
            for(int i = 0; i < LINE_PORT_MAX; i++)
            {
                cJSON *item = cJSON_CreateObject();
                if(item)
                {
                    cJSON_AddNumberToObject(item, "d_state", light.port[i].ctrl.d_state);
                    cJSON_AddNumberToObject(item, "d_value", light.port[i].ctrl.d_value);
                    cJSON_AddNumberToObject(item, "manual", light.port[i]._manual.manual);
                    cJSON_AddNumberToObject(item, "manual_on_time", light.port[i]._manual.manual_on_time);
                    cJSON_AddNumberToObject(item, "manual_on_time_save", light.port[i]._manual.manual_on_time_save);
                    cJSON_AddItemToArray(portList, item);
                }
            }
            cJSON_AddItemToObject(cjson, "port", portList);
        }

        //3. 存储sensor数据
        str = cJSON_PrintUnformatted(cjson);
        //释放空间 否则JSON 占用的空间很大
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);

            sprintf(file, "%s%s%d%s", saveFile, "/line", no, "/line.txt");

            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveLineByJson, write to %s err",file);
            }
            else
            {
                LOG_I("SaveLineByJson write to file ok");

            }

            cJSON_free(str);
            str = RT_NULL;
        }
    }
}

static void SaveSysCo2CalByJson(sys_set_t sys_set,char *saveFile)
{
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cJSON *co2Cal = cJSON_CreateArray();
    if(co2Cal)
    {
        for(int i = 0; i < SENSOR_MAX; i++)
        {
            cJSON_AddNumberToObject(co2Cal, "", sys_set.co2Cal[i]);
        }
    }

    str = cJSON_PrintUnformatted(co2Cal);
    //释放空间
    cJSON_Delete(co2Cal);
    if(str)
    {
        int size = strlen(str);
        sprintf(file, "%s%s", saveFile, "/co2Cal.txt");
        RemoveFileDirectory(file);
        if(RT_ERROR == WriteFileData(file, str, 0, size))
        {
            LOG_E("SaveSysCo2CalByJson, write to %s err",file);
        }
        cJSON_free(str);
    }
}

static void SaveRecipeByJson(recipe_t recipe, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    cJSON       *line               = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cjson = cJSON_CreateObject();

    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "id", recipe.id);
        cJSON_AddStringToObject(cjson, "name", recipe.name);
        cJSON_AddNumberToObject(cjson, "color", recipe.color);
        cJSON_AddNumberToObject(cjson, "dayCoolingTarget", recipe.dayCoolingTarget);
        cJSON_AddNumberToObject(cjson, "dayHeatingTarget", recipe.dayHeatingTarget);
        cJSON_AddNumberToObject(cjson, "nightCoolingTarget", recipe.nightCoolingTarget);
        cJSON_AddNumberToObject(cjson, "nightHeatingTarget", recipe.nightHeatingTarget);
        cJSON_AddNumberToObject(cjson, "dayHumidifyTarget", recipe.dayHumidifyTarget);
        cJSON_AddNumberToObject(cjson, "dayDehumidifyTarget", recipe.dayDehumidifyTarget);
        cJSON_AddNumberToObject(cjson, "nightHumidifyTarget", recipe.nightHumidifyTarget);
        cJSON_AddNumberToObject(cjson, "nightDehumidifyTarget", recipe.nightDehumidifyTarget);
        cJSON_AddNumberToObject(cjson, "dayCo2Target", recipe.dayCo2Target);
        cJSON_AddNumberToObject(cjson, "nightCo2Target", recipe.nightCo2Target);

        line = cJSON_CreateObject();
        if(line)
        {
            cJSON_AddNumberToObject(line, "brightMode", recipe.line_list[0].brightMode);
            cJSON_AddNumberToObject(line, "byPower", recipe.line_list[0].byPower);
            cJSON_AddNumberToObject(line, "byAutoDimming", recipe.line_list[0].byAutoDimming);
            cJSON_AddNumberToObject(line, "mode", recipe.line_list[0].mode);
            cJSON_AddNumberToObject(line, "lightOn", recipe.line_list[0].lightOn);
            cJSON_AddNumberToObject(line, "lightOff", recipe.line_list[0].lightOff);
            cJSON_AddNumberToObject(line, "firstCycleTime", recipe.line_list[0].firstCycleTime);
            cJSON_AddNumberToObject(line, "duration", recipe.line_list[0].duration);
            cJSON_AddNumberToObject(line, "pauseTime", recipe.line_list[0].pauseTime);
            cJSON_AddNumberToObject(line, "firstRuncycleTime", recipe.line_list[0].firstRuncycleTime);

            cJSON_AddItemToObject(cjson, "line1", line);
        }

        line = cJSON_CreateObject();
        if(line)
        {
            cJSON_AddNumberToObject(line, "brightMode", recipe.line_list[1].brightMode);
            cJSON_AddNumberToObject(line, "byPower", recipe.line_list[1].byPower);
            cJSON_AddNumberToObject(line, "byAutoDimming", recipe.line_list[1].byAutoDimming);
            cJSON_AddNumberToObject(line, "mode", recipe.line_list[1].mode);
            cJSON_AddNumberToObject(line, "lightOn", recipe.line_list[1].lightOn);
            cJSON_AddNumberToObject(line, "lightOff", recipe.line_list[1].lightOff);
            cJSON_AddNumberToObject(line, "firstCycleTime", recipe.line_list[1].firstCycleTime);
            cJSON_AddNumberToObject(line, "duration", recipe.line_list[1].duration);
            cJSON_AddNumberToObject(line, "pauseTime", recipe.line_list[1].pauseTime);
            cJSON_AddNumberToObject(line, "firstRuncycleTime", recipe.line_list[1].firstRuncycleTime);

            cJSON_AddItemToObject(cjson, "line2", line);
        }

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);
            sprintf(file, "%s%s%d%s", saveFile, "/recipe",no,".txt");
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveRecipeByJson, write to %s err",file);
            }
            //释放空间
            cJSON_free(str);
        }
    }
}

static void SaveRecipeSizeByJson(u8 recipe_size, u16 crc)
{
    char        recipe_size_file[]  = "/main/recipe/recipe_size.txt";
    cJSON       *cjson              = RT_NULL;
    char        *str                = RT_NULL;

    LOG_I("SaveRecipeSizeByJson");//Justin
    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "crc", crc);
        cJSON_AddNumberToObject(cjson, "recipe_size", recipe_size);

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);
        if(str)
        {
            RemoveFileDirectory(recipe_size_file);
            if(RT_ERROR == WriteFileData(recipe_size_file, str, 0, strlen(str)))
            {
                LOG_E("write data to flash fail");
            }
            else
            {
                LOG_I("write data to flash ok");
            }
            //释放空间
            cJSON_free(str);
        }
    }
}

static void SaveRecipeAddrByJson(u8 *addr, u8 size, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cjson = cJSON_CreateArray();
    if(cjson)
    {
        for(u8 i = 0; i < size; i++)
        {
            cJSON_AddItemToArray(cjson, cJSON_CreateNumber(addr[i]));
        }

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);
            sprintf(file, "%s%s", saveFile, "/recipe_addr.txt");
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveRecipeAddrByJson, write to %s err",file);
            }
            //释放空间
            cJSON_free(str);
        }
    }
}

static void SaveSysStageList(stage_t *stage)
{
    char        sys_StageList_dir[]         = "/main/sysSet/stageList.txt";
    char        *str                        = RT_NULL;
    char        file[20]                    = "";//存储device类设备
    cJSON       *stageSet                   = RT_NULL;
    cJSON       *item                       = RT_NULL;

    stageSet = cJSON_CreateObject();
    if(stageSet)
    {
        cJSON_AddNumberToObject(stageSet, "en", stage->en);
        cJSON_AddStringToObject(stageSet, "starts", stage->starts);
        for(int i = 0; i < STAGE_LIST_MAX; i++)
        {
            item = cJSON_CreateObject();
            if(item)
            {
                cJSON_AddNumberToObject(item, "recipeId", stage->_list[i].recipeId);
                cJSON_AddNumberToObject(item, "duration_day", stage->_list[i].duration_day);

                sprintf(file, "%s%d", "stage", i);
                cJSON_AddItemToObject(stageSet, file, item);
            }
        }
    }

    str = cJSON_PrintUnformatted(stageSet);
    //释放空间
    cJSON_Delete(stageSet);
    if(str)
    {
        int size = strlen(str);

        RemoveFileDirectory(sys_StageList_dir);
        if(RT_ERROR == WriteFileData(sys_StageList_dir, str, 0, size))
        {
            LOG_E("SaveSysStageList, write to %s err",file);
        }
        cJSON_free(str);
    }
}

static void SaveSysWarn(sys_warn_t *warn)
{
    char        sys_warn_dir[]         = "/main/sysSet/warn.txt";
    char        *str                        = RT_NULL;
    char        file[20]                    = "";//存储device类设备
    cJSON       *cjson                       = RT_NULL;
//    cJSON       *item                       = RT_NULL;

    cjson = cJSON_CreateObject();
    if(cjson)
    {
#if(HUB_SELECT == HUB_ENVIRENMENT)
        cJSON_AddNumberToObject(cjson, "dayTempMin", warn->dayTempMin);
        cJSON_AddNumberToObject(cjson, "dayTempMax", warn->dayTempMax);
        cJSON_AddNumberToObject(cjson, "dayTempEn", warn->dayTempEn);
        cJSON_AddNumberToObject(cjson, "dayhumidMin", warn->dayhumidMin);
        cJSON_AddNumberToObject(cjson, "dayhumidMax", warn->dayhumidMax);
        cJSON_AddNumberToObject(cjson, "dayhumidEn", warn->dayhumidEn);
        cJSON_AddNumberToObject(cjson, "dayCo2Min", warn->dayCo2Min);
        cJSON_AddNumberToObject(cjson, "dayCo2Max", warn->dayCo2Max);
        cJSON_AddNumberToObject(cjson, "dayCo2En", warn->dayCo2En);
        cJSON_AddNumberToObject(cjson, "dayCo2Buzz", warn->dayCo2Buzz);
        cJSON_AddNumberToObject(cjson, "dayVpdMin", warn->dayVpdMin);
        cJSON_AddNumberToObject(cjson, "dayVpdMax", warn->dayVpdMax);
        cJSON_AddNumberToObject(cjson, "dayVpdEn", warn->dayVpdEn);
        cJSON_AddNumberToObject(cjson, "dayParMin", warn->dayParMin);
        cJSON_AddNumberToObject(cjson, "dayParMax", warn->dayParMax);
        cJSON_AddNumberToObject(cjson, "dayParEn", warn->dayParEn);
        cJSON_AddNumberToObject(cjson, "nightTempMin", warn->nightTempMin);
        cJSON_AddNumberToObject(cjson, "nightTempMax", warn->nightTempMax);
        cJSON_AddNumberToObject(cjson, "nightTempEn", warn->nightTempEn);
        cJSON_AddNumberToObject(cjson, "nighthumidMin", warn->nighthumidMin);
        cJSON_AddNumberToObject(cjson, "nighthumidMax", warn->nighthumidMax);
        cJSON_AddNumberToObject(cjson, "nighthumidEn", warn->nighthumidEn);
        cJSON_AddNumberToObject(cjson, "nightCo2Min", warn->nightCo2Min);
        cJSON_AddNumberToObject(cjson, "nightCo2Max", warn->nightCo2Max);
        cJSON_AddNumberToObject(cjson, "nightCo2En", warn->nightCo2En);
        cJSON_AddNumberToObject(cjson, "nightCo2Buzz", warn->nightCo2Buzz);
        cJSON_AddNumberToObject(cjson, "nightVpdMin", warn->nightVpdMin);
        cJSON_AddNumberToObject(cjson, "nightVpdMax", warn->nightVpdMax);
        cJSON_AddNumberToObject(cjson, "nightVpdEn", warn->nightVpdEn);
        cJSON_AddNumberToObject(cjson, "co2TimeoutEn", warn->co2TimeoutEn);
        cJSON_AddNumberToObject(cjson, "co2Timeoutseconds", warn->co2Timeoutseconds);
        cJSON_AddNumberToObject(cjson, "tempTimeoutEn", warn->tempTimeoutEn);
        cJSON_AddNumberToObject(cjson, "tempTimeoutseconds", warn->tempTimeoutseconds);
        cJSON_AddNumberToObject(cjson, "humidTimeoutEn", warn->humidTimeoutEn);
        cJSON_AddNumberToObject(cjson, "humidTimeoutseconds", warn->humidTimeoutseconds);
        cJSON_AddNumberToObject(cjson, "lightEn", warn->lightEn);
        cJSON_AddNumberToObject(cjson, "o2ProtectionEn", warn->o2ProtectionEn);
#elif(HUB_SELECT == HUB_IRRIGSTION)
        cJSON_AddNumberToObject(cjson, "phEn", warn->phEn);
        cJSON_AddNumberToObject(cjson, "ecEn", warn->ecEn);
        cJSON_AddNumberToObject(cjson, "wtEn", warn->wtEn);
        cJSON_AddNumberToObject(cjson, "wlEn", warn->wlEn);
        cJSON_AddNumberToObject(cjson, "mmEn", warn->mmEn);
        cJSON_AddNumberToObject(cjson, "meEn", warn->meEn);
        cJSON_AddNumberToObject(cjson, "mtEn", warn->mtEn);
        cJSON_AddNumberToObject(cjson, "autoFillTimeout", warn->autoFillTimeout);
#endif
        cJSON_AddNumberToObject(cjson, "smokeEn", warn->smokeEn);
        cJSON_AddNumberToObject(cjson, "waterEn", warn->waterEn);
        cJSON_AddNumberToObject(cjson, "offlineEn", warn->offlineEn);
    }

    str = cJSON_PrintUnformatted(cjson);
    //释放空间
    cJSON_Delete(cjson);
    if(str)
    {
        int size = strlen(str);

        RemoveFileDirectory(sys_warn_dir);
        if(RT_ERROR == WriteFileData(sys_warn_dir, str, 0, size))
        {
            LOG_E("SaveSysWarn, write to %s err",file);
        }
        cJSON_free(str);
    }
}

#elif(HUB_SELECT == HUB_IRRIGSTION)

static void SaveSysPhCalByJson(sys_set_t sys_set,char *saveFile)
{
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cJSON *phCal = cJSON_CreateArray();
    if(phCal)
    {
        for(int i = 0; i < SENSOR_MAX; i++)
        {
            cJSON *item = cJSON_CreateObject();
            if (item) {
                cJSON_AddNumberToObject(item, "ph_a", sys_set.ph[i].ph_a);
                cJSON_AddNumberToObject(item, "ph_b", sys_set.ph[i].ph_b);
                cJSON_AddNumberToObject(item, "uuid", sys_set.ph[i].uuid);

                cJSON_AddItemToArray(phCal, item);
            }
        }
    }

    str = cJSON_PrintUnformatted(phCal);
    //释放空间
    cJSON_Delete(phCal);
    if(str)
    {
        int size = strlen(str);
        sprintf(file, "%s%s", saveFile, "/phCal.txt");
        RemoveFileDirectory(file);
        if(RT_ERROR == WriteFileData(file, str, 0, size))
        {
            LOG_E("SaveSysPhCalByJson, write to %s err",file);
        }
        cJSON_free(str);
    }
}

static void SaveSysEcCalByJson(sys_set_t sys_set,char *saveFile)
{
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cJSON *ecCal = cJSON_CreateArray();
    if(ecCal)
    {
        for(int i = 0; i < SENSOR_MAX; i++)
        {
            cJSON *item = cJSON_CreateObject();
            if (item) {
                cJSON_AddNumberToObject(item, "ec_a", sys_set.ec[i].ec_a);
                cJSON_AddNumberToObject(item, "ec_b", sys_set.ec[i].ec_b);
                cJSON_AddNumberToObject(item, "uuid", sys_set.ec[i].uuid);

                cJSON_AddItemToArray(ecCal, item);
            }
        }
    }

    str = cJSON_PrintUnformatted(ecCal);
    //释放空间
    cJSON_Delete(ecCal);
    if(str)
    {
        int size = strlen(str);
        sprintf(file, "%s%s", saveFile, "/ecCal.txt");
        RemoveFileDirectory(file);
        if(RT_ERROR == WriteFileData(file, str, 0, size))
        {
            LOG_E("SaveSysEcCalByJson, write to %s err",file);
        }
        cJSON_free(str);
    }
}

static void SaveTankSizeByJson(u8 size, u16 crc, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "crc", crc);
        cJSON_AddNumberToObject(cjson, "tank_size", size);

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);
            sprintf(file, "%s%s", saveFile, "/tank_size.txt");
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveTankSizeByJson, write to %s err",file);
            }
            //释放空间
            cJSON_free(str);
        }
    }
}

static void SaveTankByJson(tank_t tank, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cjson = cJSON_CreateObject();
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "tankNo", tank.tankNo);
        cJSON_AddStringToObject(cjson, "name", tank.name);
        cJSON_AddNumberToObject(cjson, "autoFillValveId", tank.autoFillValveId);
        cJSON_AddNumberToObject(cjson, "autoFillHeight", tank.autoFillHeight);
        cJSON_AddNumberToObject(cjson, "autoFillFulfilHeight", tank.autoFillFulfilHeight);
        cJSON_AddNumberToObject(cjson, "highEcProtection", tank.highEcProtection);
        cJSON_AddNumberToObject(cjson, "lowPhProtection", tank.lowPhProtection);
        cJSON_AddNumberToObject(cjson, "highPhProtection", tank.highPhProtection);
        cJSON_AddNumberToObject(cjson, "phMonitorOnly", tank.phMonitorOnly);
        cJSON_AddNumberToObject(cjson, "ecMonitorOnly", tank.ecMonitorOnly);
        cJSON_AddNumberToObject(cjson, "wlMonitorOnly", tank.wlMonitorOnly);
        cJSON_AddNumberToObject(cjson, "pumpId", tank.pumpId);
        cJSON_AddNumberToObject(cjson, "color", tank.color);
        cJSON_AddNumberToObject(cjson, "poolTimeout", tank.poolTimeout);

        cJSON *valve = cJSON_CreateArray();
        if(valve)
        {
            for(u8 i = 0; i < VALVE_MAX; i++)
            {
                cJSON_AddItemToArray(valve, cJSON_CreateNumber(tank.valve[i]));
            }

            cJSON_AddItemToObject(cjson, "valve", valve);
        }

        cJSON *sensorId = cJSON_CreateArray();
        if(sensorId)
        {
            for(u8 i = 0; i < 2; i++)
            {
                cJSON *item = cJSON_CreateArray();
                if(item)
                {
                    for(u8 j = 0; j < TANK_SENSOR_MAX; j++)
                    {
                        cJSON_AddItemToArray(item, cJSON_CreateNumber(tank.sensorId[i][j]));
                    }
                    cJSON_AddItemToArray(sensorId, item);
                }
            }
            cJSON_AddItemToObject(cjson, "sensorId", sensorId);
        }

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);
        if(str)
        {
            int size = strlen(str);
            sprintf(file, "%s%s%d%s", saveFile, "/tank", no, ".txt");
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveTankByJson, write to %s err",file);
            }
            //释放空间
            cJSON_free(str);
        }
    }
}

static void GetEcCalFromFile(sys_set_t *set)
{
    char        eccal_file[]       = "/main/sysSet/ecCal.txt";
    int         file_size           = 0;
    char        *str                = RT_NULL;

    if(RT_EOK == CheckDirectory(eccal_file))
    {
        file_size = GetFileLength(eccal_file);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(eccal_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);

                    if(cjson)
                    {
                        //获取数组大小
                        u8 num = cJSON_GetArraySize(cjson);
                        if(num)
                        {
                            num = num < SENSOR_MAX ? num : SENSOR_MAX;

                            for(u8 i = 0; i < num; i++)
                            {
                                GetValueByInt(cJSON_GetArrayItem(cjson, i), "ec_a", &set->ec[i].ec_a);
                                GetValueByInt(cJSON_GetArrayItem(cjson, i), "ec_b", &set->ec[i].ec_b);
                                GetValueByInt(cJSON_GetArrayItem(cjson, i), "uuid", &set->ec[i].uuid);
                            }
                        }

                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetPhCalFromFile(sys_set_t *set)
{
    char        phcal_file[]       = "/main/sysSet/phCal.txt";
    int         file_size           = 0;
    char        *str                = RT_NULL;

    if(RT_EOK == CheckDirectory(phcal_file))
    {
        file_size = GetFileLength(phcal_file);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(phcal_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);

                    if(cjson)
                    {
                        //获取数组大小
                        u8 num = cJSON_GetArraySize(cjson);
                        if(num)
                        {
                            num = num < SENSOR_MAX ? num : SENSOR_MAX;

                            for(u8 i = 0; i < num; i++)
                            {
                                GetValueByInt(cJSON_GetArrayItem(cjson, i), "ph_a", &set->ph[i].ph_a);
                                GetValueByInt(cJSON_GetArrayItem(cjson, i), "ph_b", &set->ph[i].ph_b);
                                GetValueByInt(cJSON_GetArrayItem(cjson, i), "uuid", &set->ph[i].uuid);
                            }
                        }

                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetTankSizeFromFile(sys_tank_t *list)
{
    char        tank_size_file[]          = "/main/tank/tank_size.txt";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    if(RT_EOK == CheckDirectory(tank_size_file))
    {
        file_size = GetFileLength(tank_size_file);

        if(file_size)
        {
            //1.1申请空间
            str = rt_malloc(file_size);
            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(tank_size_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                      GetValueByU16(cjson, "crc", &list->crc);
                      GetValueByU8(cjson, "tank_size", &list->tank_size);

                      cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetTankFromFile(tank_t *tank, u8 no)
{
    char        file_name[50]               = "";
    char        tank_dir[]                  = "/main/tank";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    sprintf(file_name, "%s%s%d%s", tank_dir, "/tank", no, ".txt");

    if(RT_EOK == CheckDirectory(file_name))
    {
        file_size = GetFileLength(file_name);

        if(file_size)
        {
            //1.1申请空间
            str = rt_malloc(file_size);
            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(file_name, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        GetValueByU8(cjson, "tankNo", &tank->tankNo);
                        GetValueByC16(cjson, "name", tank->name, TANK_NAMESZ);
                        GetValueByU16(cjson, "autoFillValveId", &tank->autoFillValveId);
                        GetValueByU8(cjson, "autoFillHeight", &tank->autoFillHeight);
                        GetValueByU8(cjson, "autoFillFulfilHeight", &tank->autoFillFulfilHeight);
                        GetValueByU16(cjson, "highEcProtection", &tank->highEcProtection);
                        GetValueByU16(cjson, "lowPhProtection", &tank->lowPhProtection);
                        GetValueByU16(cjson, "highPhProtection", &tank->highPhProtection);
                        GetValueByU8(cjson, "phMonitorOnly", &tank->phMonitorOnly);
                        GetValueByU8(cjson, "ecMonitorOnly", &tank->ecMonitorOnly);
                        GetValueByU8(cjson, "wlMonitorOnly", &tank->wlMonitorOnly);
                        GetValueByU16(cjson, "pumpId", &tank->pumpId);
                        GetValueByU8(cjson, "color", &tank->color);
                        GetValueByU16(cjson, "poolTimeout", &tank->poolTimeout);

                        cJSON *valve = cJSON_GetObjectItem(cjson, "valve");
                        u8 num = cJSON_GetArraySize(valve);
                        num = num < VALVE_MAX ? num : VALVE_MAX;
                        for(u8 i = 0; i < num; i++)
                        {
                            tank->valve[i] = cJSON_GetArrayItem(valve, i)->valueint;
                        }

                        cJSON *sensorId = cJSON_GetObjectItem(cjson, "sensorId");
                        u8 groud = cJSON_GetArraySize(sensorId);
                        groud = groud < TANK_SINGLE_GROUD ? groud : TANK_SINGLE_GROUD;
                        for(u8 i = 0; i < groud; i++)
                        {
                            u8 sen_n = cJSON_GetArraySize(cJSON_GetArrayItem(sensorId, i));
                            sen_n = sen_n < TANK_SENSOR_MAX ? sen_n : TANK_SENSOR_MAX;
                            for(u8 j = 0; j < sen_n; j++)
                            {
                                tank->sensorId[i][j] = cJSON_GetArrayItem(cJSON_GetArrayItem(sensorId, i), j)->valueint;
                            }
                        }

                      cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetTankListFromFile(sys_tank_t *list)
{
    //1.获取tank size
    GetTankSizeFromFile(list);
    //获取tank
    u8 num = list->tank_size < TANK_LIST_MAX ? list->tank_size : TANK_LIST_MAX;
    for(u8 i = 0; i < num; i++)
    {
        GetTankFromFile(&list->tank[i], i);
    }
}

static void CheckSysTankNeedSave(sys_tank_t *list)
{
    int             i = 0;
    u16             crc = 0;
    char            tank_dir[]          = "/main/tank";

    crc = usModbusRTU_CRC((u8 *)list + 2, sizeof(sys_tank_t) - 2);
    if(crc != list->crc)
    {
        LOG_I("CheckSysTankNeedSave");
        list->crc = crc;
        SaveTankSizeByJson(list->tank_size, list->crc, tank_dir);

        for(i = 0; i < list->tank_size; i++)
        {
            SaveTankByJson(list->tank[i], i, tank_dir);
        }
    }
}

#endif

static void saveSysParaByJson(sys_para_t *sysPara)
{
    cJSON       *cjson              = RT_NULL;
    char        sys_warn_dir[]      = "/main/sysSet/sysPara.txt";
    char        *str                = RT_NULL;

    cjson = cJSON_CreateObject();

    if(cjson)
    {
        cJSON_AddStringToObject(cjson, "ntpzone", sysPara->ntpzone);
        cJSON_AddNumberToObject(cjson, "tempUnit", sysPara->tempUnit);
        cJSON_AddNumberToObject(cjson, "ecUnit", sysPara->ecUnit);
        cJSON_AddNumberToObject(cjson, "timeFormat", sysPara->timeFormat);
        cJSON_AddNumberToObject(cjson, "dayNightMode", sysPara->dayNightMode);
        cJSON_AddNumberToObject(cjson, "photocellSensitivity", sysPara->photocellSensitivity);
        cJSON_AddNumberToObject(cjson, "dayTime", sysPara->dayTime);
        cJSON_AddNumberToObject(cjson, "nightTime", sysPara->nightTime);
        cJSON_AddNumberToObject(cjson, "maintain", sysPara->maintain);

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);

        if(str)
        {
            int size = strlen(str);

            RemoveFileDirectory(sys_warn_dir);
            if(RT_ERROR == WriteFileData(sys_warn_dir, str, 0, size))
            {
                LOG_E("saveSysParaByJson, write to %s err",sys_warn_dir);
            }
            else
            {
                LOG_I("saveSysParaByJson write to file ok");

            }
            //释放空间
            cJSON_free(str);
        }
        else
        {
            LOG_E("saveSysParaByJson reply memory err");
        }
    }
}

static void SaveSysSetByJson(sys_set_t sys_set,char *saveFile)//Justin 存储这个太大了 可能会失败
{
    cJSON       *cjson              = RT_NULL;
    cJSON       *line               = RT_NULL;
//    cJSON       *sysPara            = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    //1.生成json 格式
    cjson = cJSON_CreateObject();
    LOG_I("SaveSysSetByJson");//Justin
    if(cjson)
    {
        cJSON_AddNumberToObject(cjson, "crc", sys_set.crc);
        cJSON_AddNumberToObject(cjson, "dayCoolingTarget", sys_set.tempSet.dayCoolingTarget);
        cJSON_AddNumberToObject(cjson, "dayHeatingTarget", sys_set.tempSet.dayHeatingTarget);
        cJSON_AddNumberToObject(cjson, "nightCoolingTarget", sys_set.tempSet.nightCoolingTarget);
        cJSON_AddNumberToObject(cjson, "nightHeatingTarget", sys_set.tempSet.nightHeatingTarget);
        cJSON_AddNumberToObject(cjson, "coolingDehumidifyLock", sys_set.tempSet.coolingDehumidifyLock);
        cJSON_AddNumberToObject(cjson, "tempDeadband", sys_set.tempSet.tempDeadband);

        cJSON_AddNumberToObject(cjson, "dayCo2Target", sys_set.co2Set.dayCo2Target);
        cJSON_AddNumberToObject(cjson, "nightCo2Target", sys_set.co2Set.nightCo2Target);
        cJSON_AddNumberToObject(cjson, "isFuzzyLogic", sys_set.co2Set.isFuzzyLogic);
        cJSON_AddNumberToObject(cjson, "coolingLock", sys_set.co2Set.coolingLock);
        cJSON_AddNumberToObject(cjson, "dehumidifyLock", sys_set.co2Set.dehumidifyLock);
        cJSON_AddNumberToObject(cjson, "co2Deadband", sys_set.co2Set.co2Deadband);
        cJSON_AddNumberToObject(cjson, "co2Corrected", sys_set.co2Set.co2Corrected);

        cJSON_AddNumberToObject(cjson, "dayHumiTarget", sys_set.humiSet.dayHumiTarget);
        cJSON_AddNumberToObject(cjson, "dayDehumiTarget", sys_set.humiSet.dayDehumiTarget);
        cJSON_AddNumberToObject(cjson, "nightHumiTarget", sys_set.humiSet.nightHumiTarget);
        cJSON_AddNumberToObject(cjson, "nightDehumiTarget", sys_set.humiSet.nightDehumiTarget);
        cJSON_AddNumberToObject(cjson, "humidDeadband", sys_set.humiSet.humidDeadband);

        cJSON_AddNumberToObject(cjson, "sensorMainType", sys_set.sensorMainType);

        line = cJSON_CreateObject();
        if(line)
        {
            cJSON_AddNumberToObject(line, "lightsType", sys_set.line1Set.lightsType);
            cJSON_AddNumberToObject(line, "brightMode", sys_set.line1Set.brightMode);
            cJSON_AddNumberToObject(line, "byPower", sys_set.line1Set.byPower);
            cJSON_AddNumberToObject(line, "byAutoDimming", sys_set.line1Set.byAutoDimming);
            cJSON_AddNumberToObject(line, "mode", sys_set.line1Set.mode);
            cJSON_AddNumberToObject(line, "lightOn", sys_set.line1Set.lightOn);
            cJSON_AddNumberToObject(line, "lightOff", sys_set.line1Set.lightOff);
            cJSON_AddNumberToObject(line, "firstCycleTime", sys_set.line1Set.firstCycleTime);
            cJSON_AddNumberToObject(line, "duration", sys_set.line1Set.duration);
            cJSON_AddNumberToObject(line, "pauseTime", sys_set.line1Set.pauseTime);
            cJSON_AddNumberToObject(line, "hidDelay", sys_set.line1Set.hidDelay);
            cJSON_AddNumberToObject(line, "tempStartDimming", sys_set.line1Set.tempStartDimming);
            cJSON_AddNumberToObject(line, "tempOffDimming", sys_set.line1Set.tempOffDimming);
            cJSON_AddNumberToObject(line, "sunriseSunSet", sys_set.line1Set.sunriseSunSet);
            cJSON_AddNumberToObject(line, "timestamp", sys_set.line1Set.timestamp);
            cJSON_AddNumberToObject(line, "firstRuncycleTime", sys_set.line1Set.firstRuncycleTime);

            cJSON_AddItemToObject(cjson, "line1", line);
        }

        line = cJSON_CreateObject();
        if(line)
        {
            cJSON_AddNumberToObject(line, "lightsType", sys_set.line2Set.lightsType);
            cJSON_AddNumberToObject(line, "brightMode", sys_set.line2Set.brightMode);
            cJSON_AddNumberToObject(line, "byPower", sys_set.line2Set.byPower);
            cJSON_AddNumberToObject(line, "byAutoDimming", sys_set.line2Set.byAutoDimming);
            cJSON_AddNumberToObject(line, "mode", sys_set.line2Set.mode);
            cJSON_AddNumberToObject(line, "lightOn", sys_set.line2Set.lightOn);
            cJSON_AddNumberToObject(line, "lightOff", sys_set.line2Set.lightOff);
            cJSON_AddNumberToObject(line, "firstCycleTime", sys_set.line2Set.firstCycleTime);
            cJSON_AddNumberToObject(line, "duration", sys_set.line2Set.duration);
            cJSON_AddNumberToObject(line, "pauseTime", sys_set.line2Set.pauseTime);
            cJSON_AddNumberToObject(line, "hidDelay", sys_set.line2Set.hidDelay);
            cJSON_AddNumberToObject(line, "tempStartDimming", sys_set.line2Set.tempStartDimming);
            cJSON_AddNumberToObject(line, "tempOffDimming", sys_set.line2Set.tempOffDimming);
            cJSON_AddNumberToObject(line, "sunriseSunSet", sys_set.line2Set.sunriseSunSet);
            cJSON_AddNumberToObject(line, "timestamp", sys_set.line2Set.timestamp);
            cJSON_AddNumberToObject(line, "firstRuncycleTime", sys_set.line2Set.firstRuncycleTime);

            cJSON_AddItemToObject(cjson, "line2", line);
        }

        str = cJSON_PrintUnformatted(cjson);
        //释放空间
        cJSON_Delete(cjson);

        if(str)
        {
            int size = strlen(str);
            //LOG_I("file size = %d",size);
            sprintf(file, "%s%s", saveFile, "/sys_set.txt");
            //LOG_D("file name = %s",file);
            RemoveFileDirectory(file);
            if(RT_ERROR == WriteFileData(file, str, 0, size))
            {
                LOG_E("SaveSysSetByJson, write to %s err",file);
            }
            else
            {
                LOG_I("SaveSysSetByJson write to file ok");

            }
            //释放空间
            cJSON_free(str);
        }
        else
        {
            LOG_E("SaveSysSetByJson reply memory err");
        }
    }
    else
    {
        LOG_E("SaveSysSetByJson err");
    }

#if(HUB_SELECT == HUB_ENVIRENMENT)
        SaveSysCo2CalByJson(sys_set, saveFile);
        SaveSysStageList(&sys_set.stageSet);
        SaveSysWarn(&sys_set.sysWarn);
        saveSysParaByJson(&sys_set.sysPara);//Justin 未测试
#elif(HUB_SELECT == HUB_IRRIGSTION)

        SaveSysPhCalByJson(sys_set, saveFile);
        SaveSysEcCalByJson(sys_set, saveFile);
#endif
}

static void GetDevPortFromFile(device_t *device, u8 no, char *fromFile)
{
    //1.从文件读取数据
    char        file[50];
    int         fileSize            = 0;

    sprintf(file,"%s%s%d%s",fromFile, "/port", no, ".txt");

    if(RT_ERROR == CheckDirectory(file))
    {
        return;
    }

    fileSize = GetFileLength(file);
    if(fileSize)
    {
        char *str = rt_malloc(fileSize);
        if(str)
        {
            if(RT_EOK == ReadFileData(file, str, 0, fileSize))
            {
                //解析
                cJSON *cjson = cJSON_Parse(str);

                if(cjson)
                {
                    GetValueByC16(cjson, "name", device->port[no].name, STORAGE_NAMESZ);
                    GetValueByU16(cjson, "addr", &device->port[no].addr);
                    GetValueByU8(cjson, "type", &device->port[no].type);
                    GetValueByU8(cjson, "hotStartDelay", &device->port[no].hotStartDelay);
                    GetValueByU8(cjson, "mode", &device->port[no].mode);
                    GetValueByU8(cjson, "func", &device->port[no].func);

                    GetValueByU16(cjson, "startAt", &device->port[no].cycle.startAt);
                    GetValueByInt(cjson, "duration", &device->port[no].cycle.duration);
                    GetValueByInt(cjson, "pauseTime", &device->port[no].cycle.pauseTime);
                    GetValueByU16(cjson, "times", &device->port[no].cycle.times);

                    GetValueByU8(cjson, "manual", &device->port[no].manual.manual);
                    GetValueByU16(cjson, "manual_on_time", &device->port[no].manual.manual_on_time);
                    GetValueByU32(cjson, "manual_on_time_save", &device->port[no].manual.manual_on_time_save);

                    GetValueByU8(cjson, "d_state", &device->port[no].ctrl.d_state);
                    GetValueByU8(cjson, "d_value", &device->port[no].ctrl.d_value);

                    device->port[no].ctrl.d_state = 0x00;//Justin
                    device->port[no].ctrl.d_value = 0x00;

                    //释放空间
                    cJSON_Delete(cjson);
                }
            }

            //释放空间
            cJSON_free(str);
            str = RT_NULL;
        }
    }
}

static void GetDevTimerFromFile(device_t *device, u8 no, char *fromFile)
{
    //1.从文件读取数据
    char        file[50];
    int         fileSize            = 0;

    sprintf(file,"%s%s%d%s",fromFile, "/timer", no, ".txt");

    if(RT_ERROR == CheckDirectory(file))
    {
        return;
    }

    fileSize = GetFileLength(file);
    if(fileSize)
    {
        char *str = rt_malloc(fileSize);
        if(str)
        {
            if(RT_EOK == ReadFileData(file, str, 0, fileSize))
            {
                //解析
                cJSON *cjson = cJSON_Parse(str);

                if(cjson)
                {
                    u8 item_num = cJSON_GetArraySize(cjson);

                    if (item_num > TIMER_GROUP) {
                        item_num = TIMER_GROUP;
                    }

                    for(u8 i = 0; i < item_num; i++)
                    {
                        cJSON *item = cJSON_GetArrayItem(cjson, i);

                        if(item)
                        {
                            GetValueByInt(item, "on_at", &device->port[no].timer[i].on_at);
                            GetValueByInt(item, "duration", &device->port[no].timer[i].duration);
                            GetValueByU8(item, "en", &device->port[no].timer[i].en);
                        }
                    }
                    //释放空间
                    cJSON_Delete(cjson);
                }
            }

            //释放空间
            cJSON_free(str);
            str = RT_NULL;
        }
    }
}

//将文件数据按照json格式存储到device
static void GetDeviceFromFile(device_t *device, u8 no, char *fromFile)
{
    //1.从文件读取数据
    char        file[50];
    int         fileSize            = 0;

    //测试读取数据
    sprintf(file,"%s%s%d%s",fromFile, "/device", no, "/device.txt");

    if(RT_ERROR == CheckDirectory(file))
    {
        return;
    }

    fileSize = GetFileLength(file);

    if(fileSize)
    {
        //申请动态空间
        char *str = rt_malloc(fileSize);
        if(str)
        {
            if(RT_EOK == ReadFileData(file, str, 0, fileSize))
            {
                //解析
                cJSON *cjson = cJSON_Parse(str);
                //释放空间
                cJSON_free(str);
                str = RT_NULL;
                if(cjson)
                {
                    GetValueByU16(cjson, "crc", &device->crc);
                    GetValueByU32(cjson, "uuid", &device->uuid);
                    GetValueByC16(cjson, "name", device->name, MODULE_NAMESZ);
                    GetValueByU8(cjson, "addr", &device->addr);
                    GetValueByU8(cjson, "type", &device->type);
                    GetValueByU16(cjson, "ctrl_addr", &device->ctrl_addr);
                    GetValueByU8(cjson, "main_type", &device->main_type);
                    GetValueByU8(cjson, "storage_size", &device->storage_size);
                    GetValueByU8(cjson, "color", &device->color);

                    GetValueByU8(cjson, "fanNormallyOpen", &device->_hvac.fanNormallyOpen);
                    GetValueByU8(cjson, "hvacMode", &device->_hvac.hvacMode);
                    GetValueByU8(cjson, "manualOnMode", &device->_hvac.manualOnMode);

                    //释放空间
                    cJSON_Delete(cjson);
                }
            }
        }
    }

    //获取端口
    sprintf(file,"%s%s%d%s",fromFile, "/device", no, "/port");
    u8 port_num = device->storage_size;
    port_num = port_num < DEVICE_PORT_MAX ? port_num : DEVICE_PORT_MAX;
    for(u8 i = 0; i < port_num; i++)
    {
        GetDevPortFromFile(device, i, file);
    }
    //获取timer
    sprintf(file,"%s%s%d%s",fromFile, "/device", no, "/timer");
    for(u8 i = 0; i < port_num; i++)
    {
        GetDevTimerFromFile(device, i, file);
    }
}

static void GetSensorFromFile(sensor_t *sensor, u8 no, char *fromFile)
{
    //1.从文件读取数据
    char        file[50];
    int         fileSize            = 0;

    //测试读取数据
    sprintf(file,"%s%s%d%s",fromFile, "/sensor", no, "/sensor.txt");

    if(RT_ERROR == CheckDirectory(file))
    {
        return;
    }

    fileSize = GetFileLength(file);

    if(fileSize)
    {
        //申请动态空间
        char *str = rt_malloc(fileSize);
        if(str)
        {
            if(RT_EOK == ReadFileData(file, str, 0, fileSize))
            {
                //解析
                cJSON *cjson = cJSON_Parse(str);
                //释放空间
                cJSON_free(str);
                str = RT_NULL;
                if(cjson)
                {
                    GetValueByU16(cjson, "crc", &sensor->crc);
                    GetValueByU32(cjson, "uuid", &sensor->uuid);
                    GetValueByC16(cjson, "name", sensor->name, MODULE_NAMESZ);
                    GetValueByU8(cjson, "addr", &sensor->addr);
                    GetValueByU8(cjson, "type", &sensor->type);
                    GetValueByU16(cjson, "ctrl_addr", &sensor->ctrl_addr);
                    GetValueByU8(cjson, "conn_state", &sensor->conn_state);
                    GetValueByU8(cjson, "reg_state", &sensor->reg_state);
                    GetValueByU8(cjson, "save_state", &sensor->save_state);
                    GetValueByU8(cjson, "storage_size", &sensor->storage_size);
                    GetValueByU8(cjson, "isMainSensor", &sensor->isMainSensor);

                    //获取端口
                    cJSON *port = cJSON_GetObjectItem(cjson, "port");
                    if(port)
                    {
                        u8 port_size = cJSON_GetArraySize(port);
                        if(port_size)
                        {
                            for(u8 port_i = 0; port_i < port_size; port_i++)
                            {
                                cJSON *item = cJSON_GetArrayItem(port, port_i);

                                GetValueByC16(item, "name", sensor->__stora[port_i].name, STORAGE_NAMESZ);
                                GetValueByU8(item, "func", &sensor->__stora[port_i].func);
                                GetValueByInt(item, "value", &sensor->__stora[port_i].value);
                            }
                        }
                    }

                    //释放空间
                    cJSON_Delete(cjson);
                }
            }
        }
    }
}

static void GetLineFromFile(line_t *line, u8 no, char *fromFile)
{
    //1.从文件读取数据
    char        file[50];
    int         fileSize            = 0;

    //测试读取数据
    sprintf(file,"%s%s%d%s",fromFile, "/line", no, "/line.txt");

    if(RT_ERROR == CheckDirectory(file))
    {
        return;
    }

    fileSize = GetFileLength(file);

    if(fileSize)
    {
        //申请动态空间
        char *str = rt_malloc(fileSize);
        if(str)
        {
            if(RT_EOK == ReadFileData(file, str, 0, fileSize))
            {
                //解析
                cJSON *cjson = cJSON_Parse(str);
                //释放空间
                cJSON_free(str);
                str = RT_NULL;
                if(cjson)
                {
                    GetValueByU16(cjson, "crc", &line->crc);
                    GetValueByU8(cjson, "type", &line->type);
                    GetValueByU32(cjson, "uuid", &line->uuid);
                    GetValueByC16(cjson, "name", line->name, MODULE_NAMESZ);
                    GetValueByU8(cjson, "addr", &line->addr);
                    GetValueByU16(cjson, "ctrl_addr", &line->ctrl_addr);
                    GetValueByU8(cjson, "save_state", &line->save_state);
                    GetValueByU8(cjson, "conn_state", &line->conn_state);
                    GetValueByU8(cjson, "storage_size", &line->storage_size);

                    cJSON *portList = cJSON_GetObjectItem(cjson, "port");
                    if(portList)
                    {
                        u8 size = cJSON_GetArraySize(portList);
                        size = size < LINE_PORT_MAX ? size : LINE_PORT_MAX;
                        for(int i = 0; i < size; i++)
                        {
                            cJSON *item = cJSON_GetArrayItem(portList, i);

                            if(item)
                            {
                                GetValueByU8(item, "d_state", &line->port[i].ctrl.d_state);
                                GetValueByU8(item, "d_value", &line->port[i].ctrl.d_value);
                                GetValueByU8(item, "manual", &line->port[i]._manual.manual);
                                GetValueByU16(item, "manual_on_time", &line->port[i]._manual.manual_on_time);
                                GetValueByU32(item, "manual_on_time_save", &line->port[i]._manual.manual_on_time_save);
                            }
                        }
                    }

                    //释放空间
                    cJSON_Delete(cjson);
                }
            }
        }
    }
}

static void GetCo2CalFromFile(sys_set_t *set)
{
    char        co2cal_file[]       = "/main/sysSet/co2Cal.txt";
    int         file_size           = 0;
    char        *str                = RT_NULL;

    if(RT_EOK == CheckDirectory(co2cal_file))
    {
        file_size = GetFileLength(co2cal_file);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(co2cal_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);

                    if(cjson)
                    {
                        //获取数组大小
                        u8 num = cJSON_GetArraySize(cjson);
                        if(num)
                        {
                            num = num < SENSOR_MAX ? num : SENSOR_MAX;

                            for(u8 i = 0; i < num; i++)
                            {
                                set->co2Cal[i] = cJSON_GetArrayItem(cjson, i)->valueint;
                            }
                        }

                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetSysParaFromFile(sys_para_t *para)
{
    char        sys_para_dir[]              = "/main/sysSet/sysPara.txt";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    if(RT_EOK == CheckDirectory(sys_para_dir))
    {
        file_size = GetFileLength(sys_para_dir);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(sys_para_dir, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);

                    if(cjson)
                    {
                        GetValueByC16(cjson, "ntpzone", para->ntpzone, 9);
                        GetValueByU8(cjson, "tempUnit", &para->tempUnit);
                        GetValueByU8(cjson, "ecUnit", &para->ecUnit);
                        GetValueByU8(cjson, "timeFormat", &para->timeFormat);
                        GetValueByU8(cjson, "dayNightMode", &para->dayNightMode);
                        GetValueByU16(cjson, "photocellSensitivity", &para->photocellSensitivity);
                        GetValueByU16(cjson, "dayTime", &para->dayTime);
                        GetValueByU16(cjson, "nightTime", &para->nightTime);
                        GetValueByU8(cjson, "maintain", &para->maintain);
                    }
                }
            }
        }
    }
}

static void GetSysWarnFromFile(sys_warn_t *warn)
{
    char        sys_warn_dir[]              = "/main/sysSet/warn.txt";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    LOG_I("GetSysWarnFromFile");

    if(RT_EOK == CheckDirectory(sys_warn_dir))
    {
        file_size = GetFileLength(sys_warn_dir);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(sys_warn_dir, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);

                    if(cjson)
                    {
#if(HUB_SELECT == HUB_ENVIRENMENT)
                        GetValueByU16(cjson, "dayTempMin", &warn->dayTempMin);
                        LOG_I("warn->dayTempMin = %d",warn->dayTempMin);//Justin
                        GetValueByU16(cjson, "dayTempMax", &warn->dayTempMax);
                        GetValueByU8(cjson, "dayTempEn", &warn->dayTempEn);
                        GetValueByU16(cjson, "dayhumidMin", &warn->dayhumidMin);
                        GetValueByU16(cjson, "dayhumidMax", &warn->dayhumidMax);
                        GetValueByU8(cjson, "dayhumidEn", &warn->dayhumidEn);
                        GetValueByU16(cjson, "dayCo2Min", &warn->dayCo2Min);
                        GetValueByU16(cjson, "dayCo2Max", &warn->dayCo2Max);
                        GetValueByU8(cjson, "dayCo2En", &warn->dayCo2En);
                        GetValueByU8(cjson, "dayCo2Buzz", &warn->dayCo2Buzz);
                        GetValueByU16(cjson, "dayVpdMin", &warn->dayVpdMin);
                        GetValueByU16(cjson, "dayVpdMax", &warn->dayVpdMax);
                        GetValueByU8(cjson, "dayVpdEn", &warn->dayVpdEn);
                        GetValueByU16(cjson, "dayParMin", &warn->dayParMin);
                        GetValueByU16(cjson, "dayParMax", &warn->dayParMax);
                        GetValueByU8(cjson, "dayParEn", &warn->dayParEn);
                        GetValueByU16(cjson, "nightTempMin", &warn->nightTempMin);
                        GetValueByU16(cjson, "nightTempMax", &warn->nightTempMax);
                        GetValueByU8(cjson, "nightTempEn", &warn->nightTempEn);
                        GetValueByU16(cjson, "nighthumidMin", &warn->nighthumidMin);
                        GetValueByU16(cjson, "nighthumidMax", &warn->nighthumidMax);
                        GetValueByU8(cjson, "nighthumidEn", &warn->nighthumidEn);
                        GetValueByU16(cjson, "nightCo2Min", &warn->nightCo2Min);
                        GetValueByU16(cjson, "nightCo2Max", &warn->nightCo2Max);
                        GetValueByU8(cjson, "nightCo2En", &warn->nightCo2En);
                        GetValueByU8(cjson, "nightCo2Buzz", &warn->nightCo2Buzz);
                        GetValueByU16(cjson, "nightVpdMin", &warn->nightVpdMin);
                        GetValueByU16(cjson, "nightVpdMax", &warn->nightVpdMax);
                        GetValueByU8(cjson, "nightVpdEn", &warn->nightVpdEn);
                        GetValueByU8(cjson, "co2TimeoutEn", &warn->co2TimeoutEn);
                        GetValueByU16(cjson, "co2Timeoutseconds", &warn->co2Timeoutseconds);
                        GetValueByU8(cjson, "tempTimeoutEn", &warn->tempTimeoutEn);
                        GetValueByU16(cjson, "tempTimeoutseconds", &warn->tempTimeoutseconds);
                        GetValueByU8(cjson, "humidTimeoutEn", &warn->humidTimeoutEn);
                        GetValueByU16(cjson, "humidTimeoutseconds", &warn->humidTimeoutseconds);
                        GetValueByU8(cjson, "lightEn", &warn->lightEn);
                        GetValueByU8(cjson, "o2ProtectionEn", &warn->o2ProtectionEn);
    #elif(HUB_SELECT == HUB_IRRIGSTION)
                        GetValueByU8(cjson, "phEn", &warn->phEn);
                        GetValueByU8(cjson, "ecEn", &warn->ecEn);
                        GetValueByU8(cjson, "wtEn", &warn->wtEn);
                        GetValueByU8(cjson, "wlEn", &warn->wlEn);
                        GetValueByU8(cjson, "mmEn", &warn->mmEn);
                        GetValueByU8(cjson, "meEn", &warn->meEn);
                        GetValueByU8(cjson, "mtEn", &warn->mtEn);
                        GetValueByU8(cjson, "autoFillTimeout", &warn->autoFillTimeout);
    #endif
                        GetValueByU8(cjson, "smokeEn", &warn->smokeEn);
                        GetValueByU8(cjson, "waterEn", &warn->waterEn);
                        GetValueByU8(cjson, "offlineEn", &warn->offlineEn);
                        cJSON_Delete(cjson);
                    }
                    else
                    {
                        LOG_E("GetSysWarnFromFile err2");
                    }
                }
                else
                {
                    LOG_E("GetSysWarnFromFile err1");
                }
            }
            else
            {
                LOG_E("GetSysWarnFromFile err");
            }
        }
        else {

            LOG_E("GetSysWarnFromFile err4");
        }
    }
    else
    {
        LOG_E("GetSysWarnFromFile err3");
    }
}

static void GetSysStageListFromFile(stage_t *stage)
{
    char        sys_StageList_dir[]         = "/main/sysSet/stageList.txt";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;
    char        name[20];

    LOG_I("GetSysStageListFromFile");

    if(RT_EOK == CheckDirectory(sys_StageList_dir))
    {
        file_size = GetFileLength(sys_StageList_dir);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(sys_StageList_dir, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);

                    if(cjson)
                    {
                        GetValueByU8(cjson, "en", &stage->en);
                        GetValueByC16(cjson, "starts", stage->starts, 16);

                        for(int i = 0; i < STAGE_LIST_MAX; i++)
                        {
                            sprintf(name, "%s%d", "stage", i);

                            cJSON *item = cJSON_GetObjectItem(cjson, name);

                            GetValueByU8(item, "recipeId", &stage->_list[i].recipeId);
                            GetValueByU8(item, "duration_day",  &stage->_list[i].duration_day);
                        }

                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetSysRecipeFromFile(recipe_t *recipe, u8 no)
{
    char        file_name[50]       = "";
    char        recipe_dir[]        = "/main/recipe";
    int         file_size           = 0;
    char        *str                = RT_NULL;

    //1.检查是否存在文件
    sprintf(file_name, "%s%s%d%s", recipe_dir, "/recipe", no, ".txt");
    if(RT_EOK == CheckDirectory(file_name))
    {
        file_size = GetFileLength(file_name);

        if(file_size)
        {
            //1.1 申请空间
            str = rt_malloc(file_size);
            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(file_name, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        GetValueByU8(cjson, "id", &recipe->id);
                        GetValueByC16(cjson, "name", recipe->name, RECIPE_NAMESZ);
                        GetValueByU8(cjson, "color", &recipe->color);
                        GetValueByU16(cjson, "dayCoolingTarget", &recipe->dayCoolingTarget);
                        GetValueByU16(cjson, "dayHeatingTarget", &recipe->dayHeatingTarget);
                        GetValueByU16(cjson, "nightCoolingTarget", &recipe->nightCoolingTarget);
                        GetValueByU16(cjson, "nightHeatingTarget", &recipe->nightHeatingTarget);
                        GetValueByU16(cjson, "dayHumidifyTarget", &recipe->dayHumidifyTarget);
                        GetValueByU16(cjson, "dayDehumidifyTarget", &recipe->dayDehumidifyTarget);
                        GetValueByU16(cjson, "nightHumidifyTarget", &recipe->nightHumidifyTarget);
                        GetValueByU16(cjson, "nightDehumidifyTarget", &recipe->nightDehumidifyTarget);
                        GetValueByU16(cjson, "dayCo2Target", &recipe->dayCo2Target);
                        GetValueByU16(cjson, "nightCo2Target", &recipe->nightCo2Target);

                        cJSON *line = cJSON_GetObjectItem(cjson, "line1");
                        if(line)
                        {
                            GetValueByU8(line, "brightMode", &recipe->line_list[0].brightMode);
                            GetValueByU8(line, "byPower", &recipe->line_list[0].byPower);
                            GetValueByU16(line, "byAutoDimming", &recipe->line_list[0].byAutoDimming);
                            GetValueByU8(line, "mode", &recipe->line_list[0].mode);
                            GetValueByU16(line, "lightOn", &recipe->line_list[0].lightOn);
                            GetValueByU16(line, "lightOff", &recipe->line_list[0].lightOff);
                            GetValueByU16(line, "firstCycleTime", &recipe->line_list[0].firstCycleTime);
                            GetValueByInt(line, "duration", &recipe->line_list[0].duration);
                            GetValueByInt(line, "pauseTime", &recipe->line_list[0].pauseTime);
                            GetValueByU32(line, "firstRuncycleTime", &recipe->line_list[0].firstRuncycleTime);
                        }

                        line = cJSON_GetObjectItem(cjson, "line2");
                        if(line)
                        {
                            GetValueByU8(line, "brightMode", &recipe->line_list[1].brightMode);
                            GetValueByU8(line, "byPower", &recipe->line_list[1].byPower);
                            GetValueByU16(line, "byAutoDimming", &recipe->line_list[1].byAutoDimming);
                            GetValueByU8(line, "mode", &recipe->line_list[1].mode);
                            GetValueByU16(line, "lightOn", &recipe->line_list[1].lightOn);
                            GetValueByU16(line, "lightOff", &recipe->line_list[1].lightOff);
                            GetValueByU16(line, "firstCycleTime", &recipe->line_list[1].firstCycleTime);
                            GetValueByInt(line, "duration", &recipe->line_list[1].duration);
                            GetValueByInt(line, "pauseTime", &recipe->line_list[1].pauseTime);
                            GetValueByU32(line, "firstRuncycleTime", &recipe->line_list[1].firstRuncycleTime);
                        }
                    }
                }
            }
        }
    }
}

static void GetSysRecipeAddrFromFile(sys_recipe_t *list)
{
    char        recipe_addr_file[]          = "/main/recipe/recipe_addr.txt";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    if(RT_EOK == CheckDirectory(recipe_addr_file))
    {
        file_size = GetFileLength(recipe_addr_file);

        if(file_size)
        {
            //1.1 申请空间
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(recipe_addr_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        //获取数组大小
                        u8 num = cJSON_GetArraySize(cjson);
                        if(num)
                        {
                            num = num < REC_ALLOT_ADDR ? num : REC_ALLOT_ADDR;

                            for(u8 i = 0; i < num; i++)
                            {
                                list->allot_add[i] = cJSON_GetArrayItem(cjson, i)->valueint;
                            }
                        }
                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetSysRecipeSizeFromFile(sys_recipe_t *list)
{
    char        recipe_size_file[]          = "/main/recipe/recipe_size.txt";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    if(RT_EOK == CheckDirectory(recipe_size_file))
    {
        file_size = GetFileLength(recipe_size_file);

        if(file_size)
        {
            //1.1申请空间
            str = rt_malloc(file_size);
            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(recipe_size_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        GetValueByU16(cjson, "crc", &list->crc);
                        GetValueByU8(cjson, "recipe_size", &list->recipe_size);

                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetSysRecipeListFromFile(sys_recipe_t *list)
{
    //1.获取配方地址
    GetSysRecipeAddrFromFile(list);
    //2.获取配方数量
    GetSysRecipeSizeFromFile(list);
    //3.获取配方
    u8 num = list->recipe_size;

    num = num < RECIPE_LIST_MAX ? num : RECIPE_LIST_MAX;
    for(u8 i = 0; i < num; i++)
    {
        GetSysRecipeFromFile(&list->recipe[i], i);
        //printRecipe(&list->recipe[i]);
    }
}

static void GetSysSetFromFile(sys_set_t *set)//Justin 存储这个太大了 可能会失败
{
    char        sysset_file[]       = "/main/sysSet/sys_set.txt";
    int         file_size           = 0;
    char        *str                = RT_NULL;
    //char        name[20];

    //1.检查是否存在文件
    if(RT_EOK == CheckDirectory(sysset_file))
    {
        file_size = GetFileLength(sysset_file);

        if(file_size)
        {
            //1.1申请内存
            str = rt_malloc(file_size);

            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(sysset_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        GetValueByU16(cjson, "crc", &set->crc);
                        rt_kprintf("GetSysSetFromFile crc = %x",set->crc);//Justin debug
                        GetValueByU16(cjson, "dayCoolingTarget", &set->tempSet.dayCoolingTarget);
                        GetValueByU16(cjson, "dayHeatingTarget", &set->tempSet.dayHeatingTarget);
                        GetValueByU16(cjson, "nightCoolingTarget", &set->tempSet.nightCoolingTarget);
                        GetValueByU16(cjson, "nightHeatingTarget", &set->tempSet.nightHeatingTarget);
                        GetValueByU8(cjson, "coolingDehumidifyLock", &set->tempSet.coolingDehumidifyLock);
                        GetValueByU16(cjson, "tempDeadband", &set->tempSet.tempDeadband);

                        GetValueByU16(cjson, "dayCo2Target", &set->co2Set.dayCo2Target);
                        GetValueByU16(cjson, "nightCo2Target", &set->co2Set.nightCo2Target);
                        GetValueByU8(cjson, "isFuzzyLogic", &set->co2Set.isFuzzyLogic);
                        GetValueByU8(cjson, "coolingLock", &set->co2Set.coolingLock);
                        GetValueByU8(cjson, "dehumidifyLock", &set->co2Set.dehumidifyLock);
                        GetValueByU16(cjson, "co2Deadband", &set->co2Set.co2Deadband);
                        GetValueByInt(cjson, "co2Corrected", &set->co2Set.co2Corrected);

                        GetValueByU16(cjson, "dayHumiTarget", &set->humiSet.dayHumiTarget);
                        GetValueByU16(cjson, "dayDehumiTarget", &set->humiSet.dayDehumiTarget);
                        GetValueByU16(cjson, "nightHumiTarget", &set->humiSet.nightHumiTarget);
                        GetValueByU16(cjson, "nightDehumiTarget", &set->humiSet.nightDehumiTarget);
                        GetValueByU16(cjson, "humidDeadband", &set->humiSet.humidDeadband);

                        GetValueByU8(cjson, "sensorMainType", &set->sensorMainType);

                        cJSON *line = cJSON_GetObjectItem(cjson, "line1");
                        if(line)
                        {
                            GetValueByU8(line, "lightsType", &set->line1Set.lightsType);
                            GetValueByU8(line, "brightMode", &set->line1Set.brightMode);
                            GetValueByU8(line, "byPower", &set->line1Set.byPower);
                            GetValueByU16(line, "byAutoDimming", &set->line1Set.byAutoDimming);
                            GetValueByU8(line, "mode", &set->line1Set.mode);
                            GetValueByU16(line, "lightOn", &set->line1Set.lightOn);
                            GetValueByU16(line, "lightOff", &set->line1Set.lightOff);
                            GetValueByU16(line, "firstCycleTime", &set->line1Set.firstCycleTime);
                            GetValueByInt(line, "duration", &set->line1Set.duration);
                            GetValueByInt(line, "pauseTime", &set->line1Set.pauseTime);
                            GetValueByU8(line, "hidDelay", &set->line1Set.hidDelay);
                            GetValueByU16(line, "tempStartDimming", &set->line1Set.tempStartDimming);
                            GetValueByU16(line, "tempOffDimming", &set->line1Set.tempOffDimming);
                            GetValueByU8(line, "sunriseSunSet", &set->line1Set.sunriseSunSet);
                            GetValueByU32(line, "timestamp", &set->line1Set.timestamp);
                            GetValueByU32(line, "firstRuncycleTime", &set->line1Set.firstRuncycleTime);
                        }

                        line = cJSON_GetObjectItem(cjson, "line2");
                        if(line)
                        {
                            GetValueByU8(line, "lightsType", &set->line2Set.lightsType);
                            GetValueByU8(line, "brightMode", &set->line2Set.brightMode);
                            GetValueByU8(line, "byPower", &set->line2Set.byPower);
                            GetValueByU16(line, "byAutoDimming", &set->line2Set.byAutoDimming);
                            GetValueByU8(line, "mode", &set->line2Set.mode);
                            GetValueByU16(line, "lightOn", &set->line2Set.lightOn);
                            GetValueByU16(line, "lightOff", &set->line2Set.lightOff);
                            GetValueByU16(line, "firstCycleTime", &set->line2Set.firstCycleTime);
                            GetValueByInt(line, "duration", &set->line2Set.duration);
                            GetValueByInt(line, "pauseTime", &set->line2Set.pauseTime);
                            GetValueByU8(line, "hidDelay", &set->line2Set.hidDelay);
                            GetValueByU16(line, "tempStartDimming", &set->line2Set.tempStartDimming);
                            GetValueByU16(line, "tempOffDimming", &set->line2Set.tempOffDimming);
                            GetValueByU8(line, "sunriseSunSet", &set->line2Set.sunriseSunSet);
                            GetValueByU32(line, "timestamp", &set->line2Set.timestamp);
                            GetValueByU32(line, "firstRuncycleTime", &set->line2Set.firstRuncycleTime);
                        }

                        cJSON_Delete(cjson);
                    }
                    else
                    {
                        LOG_E("GetSysSetFromFile 4");
                    }
                }
                else
                {
                    LOG_E("GetSysSetFromFile 3");
                }
            }
            else
            {
                LOG_E("GetSysSetFromFile 2");
            }
        }
        else
        {
            LOG_E("GetSysSetFromFile 1");
        }
    }

#if(HUB_SELECT == HUB_ENVIRENMENT)
    //2.读取co2 定标
    GetCo2CalFromFile(set);
    GetSysStageListFromFile(&set->stageSet);
    GetSysWarnFromFile(&set->sysWarn);
    GetSysParaFromFile(&set->sysPara);//Justin 未测试
#elif(HUB_SELECT == HUB_IRRIGSTION)
    GetPhCalFromFile(set);
    GetEcCalFromFile(set);
#endif
}

static void GetMonitorSizeByJson(type_monitor_t *monitor)
{
    char        monitor_size_file[]         = "/main/devRegistry/reg_size.bin";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    if(RT_EOK == CheckDirectory(monitor_size_file))
    {
        file_size = GetFileLength(monitor_size_file);
        if(file_size)
        {
            str = rt_malloc(file_size);
            //1.1 申请空间
            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(monitor_size_file, str, 0, file_size))
                {
                    //LOG_I("GetMonitorSizeByJson file size  = %d",file_size);
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        GetValueByU8(cjson, "sensor_size", &monitor->sensor_size);
                        GetValueByU8(cjson, "device_size", &monitor->device_size);
                        GetValueByU8(cjson, "line_size", &monitor->line_size);

                      cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetMonitorAadrByJson(type_monitor_t *monitor)
{
    char        monitor_addr_file[]         = "/main/devRegistry/reg_addr.bin";
    int         file_size                   = 0;
    char        *str                        = RT_NULL;

    if(RT_EOK == CheckDirectory(monitor_addr_file))
    {
        file_size = GetFileLength(monitor_addr_file);

        if(file_size)
        {
            //1.1申请空间
            str = rt_malloc(file_size);
            if(str)
            {
                //1.2读取文件
                if(RT_EOK == ReadFileData(monitor_addr_file, str, 0, file_size))
                {
                    //1.3解析数据
                    cJSON *cjson = cJSON_Parse(str);
                    rt_free(str);
                    if(cjson)
                    {
                        int addr_num = cJSON_GetArraySize(cjson);
                        if(addr_num > ALLOCATE_ADDRESS_SIZE)
                        {
                            addr_num = ALLOCATE_ADDRESS_SIZE;
                        }

                        if(addr_num)
                        {
                            for(int i = 0; i < addr_num; i++)
                            {
                                monitor->allocateStr.address[i] = cJSON_GetArrayItem(cjson, i)->valueint;
                            }
                        }

                        cJSON_Delete(cjson);
                    }
                }
            }
        }
    }
}

static void GetMonitorFromFile(type_monitor_t *monitor)
{
    //char        reg_addr_file[]     = "/main/devRegistry/reg_addr.bin";//新版本设备注册表存储文件夹
    //char        reg_size_file[]     = "/main/devRegistry/reg_size.bin";//新版本设备注册表存储文件夹
    char        reg_sensor[]        = "/main/devRegistry/sensor";//存储device类设备
    char        reg_device[]        = "/main/devRegistry/device";//存储device类设备
    char        reg_line[]          = "/main/devRegistry/line";//存储device类设备
    //char        *str;

    //1.读取地址分配表
    GetMonitorAadrByJson(monitor);

    //2.读取device、sensor、line数量
    GetMonitorSizeByJson(monitor);

    //3.读取sensor 数据
    u8 num = monitor->sensor_size;
    num = num < SENSOR_MAX ? num : SENSOR_MAX;
    for(u8 i = 0; i < num; i++)
    {
        GetSensorFromFile(&monitor->sensor[i], i, reg_sensor);
    }

    //4.读取device 数据
    num = monitor->device_size;
    num = num < DEVICE_MAX ? num : DEVICE_MAX;
    for(u8 i = 0; i < num; i++)
    {
        GetDeviceFromFile(&monitor->device[i], i, reg_device);

        //Justin
        printDevice(monitor->device[i]);
    }

    //5.读取line 数据
    num = monitor->line_size;
    num = num < LINE_MAX ? num : LINE_MAX;
    for(u8 i = 0; i < num; i++)
    {
        GetLineFromFile(&monitor->line[i], i, reg_line);
    }
}

//通过查看crc校验是否正确 如果不正确 则crc赋值并存入SD卡中
static void CheckDeviceNeedSave(type_monitor_t *monitor)
{
    int         i = 0;
    device_t    device;
    u16         crc = 0;
    char        reg_device[]    = "/main/devRegistry/device";//存储device类设备

    for(i = 0; i < monitor->device_size; i++)
    {
        rt_memset(&device, 0, sizeof(device_t));
        device = monitor->device[i];

        //以下不关注 不实时存到SD卡
        device.conn_state = CON_SUCCESS;
        device.reg_state = 0;
        device.save_state = 0;

        for(int port = 0; port < device.storage_size; port++)
        {
            device.port[port].ctrl.d_state = 0;
            device.port[port].ctrl.d_value = 0;
        }

        crc = usModbusRTU_CRC((u8 *)&device + 2, sizeof(device_t) - 2);

        if(crc != device.crc)
        {
            LOG_W("addr %x, crc = %x, device crc = %x",device.addr,crc, device.crc);
            //存储
            monitor->device[i].crc = crc;
            SaveDeviceByJson(monitor->device[i], i, reg_device);
        }
    }
}

static void CheckSensorNeedSave(type_monitor_t *monitor)
{
    int         i = 0;
    sensor_t    sensor;
    u16         crc = 0;
    char        reg_sensor[]    = "/main/devRegistry/sensor";//存储sensor类设备

    for(i = 0; i < monitor->sensor_size; i++)
    {
        rt_memset(&sensor, 0, sizeof(sensor_t));
        sensor = monitor->sensor[i];

        //以下不关注 不实时存到SD卡
        sensor.conn_state = CON_SUCCESS;
        sensor.reg_state = 0;
        sensor.save_state = 0;

        for(int port = 0; port < sensor.storage_size; port++)
        {
            sensor.__stora[port].value = 0;
        }

        crc = usModbusRTU_CRC((u8 *)&sensor + 2, sizeof(sensor_t) - 2);

        if(crc != sensor.crc)
        {
            LOG_W("addr %x, crc = %x, sensor crc = %x",sensor.addr,crc, sensor.crc);
            //存储
            monitor->sensor[i].crc = crc;
            SaveSensorByJson(monitor->sensor[i], i, reg_sensor);
        }
    }
}

static void CheckLineNeedSave(type_monitor_t *monitor)
{
    int         i = 0;
    line_t      line;
    u16         crc = 0;
    char        reg_line[]    = "/main/devRegistry/line";//存储line类设备

    for(i = 0; i < monitor->line_size; i++)
    {
        rt_memset(&line, 0, sizeof(line_t));
        line = monitor->line[i];

        //以下不关注 不实时存到SD卡
        line.conn_state = CON_SUCCESS;
        line.save_state = 0;

        for(int i = 0; i < LINE_PORT_MAX; i++)
        {
            line.port[i].ctrl.d_state = 0;
            line.port[i].ctrl.d_value = 0;
        }

        crc = usModbusRTU_CRC((u8 *)&line + 2, sizeof(line_t) - 2);

        if(crc != line.crc)
        {
            LOG_W("addr %x, crc = %x, sensor crc = %x",line.addr,crc, line.crc);
            //存储
            monitor->line[i].crc = crc;
            SaveLineByJson(monitor->line[i], i, reg_line);
        }
    }
}

static void CheckSysSetNeedSave(sys_set_t *set)
{
    char        sys_set_dir[]       = "/main/sysSet";

    if(YES == set->saveFlag)
    {
        LOG_I("CheckSysSetNeedSave");

        SaveSysSetByJson(*set, sys_set_dir);

        set->saveFlag = NO;
    }
}

static void CheckSysRecipeNeedSave(sys_recipe_t *list)
{
    int             i = 0;
    u16             crc = 0;
    char            recipe_dir[]        = "/main/recipe";

    crc = usModbusRTU_CRC((u8 *)list + 2, sizeof(sys_recipe_t) - 2);
    if(crc != list->crc)
    {
        LOG_I("CheckSysRecipeNeedSave");
        list->crc = crc;
        LOG_I("crc = %x, list.crc = %x",crc,list->crc);

        SaveRecipeSizeByJson(GetSysRecipt()->recipe_size, GetSysRecipt()->crc);
        SaveRecipeAddrByJson(list->allot_add, REC_ALLOT_ADDR, recipe_dir);

        for(i = 0; i < list->recipe_size; i++)
        {
            SaveRecipeByJson(list->recipe[i], i, recipe_dir);
        }
    }
}

void FileSystemEntry(void* parameter)
{
    static      u8              Timer1sTouch    = OFF;
    static      u16             time1S = 0;
    static      u8              Timer30sTouch    = OFF;
    static      u16             time30S = 0;
    static      u8              sensorSize = 0;
    static      u8              deviceSize = 0;
    static      u8              lineSize = 0;
    static      u16             monitorAddrCrc = 0;

    sensorSize = GetMonitor()->sensor_size;
    deviceSize = GetMonitor()->device_size;
    lineSize = GetMonitor()->line_size;
    monitorAddrCrc = usModbusRTU_CRC(GetMonitor()->allocateStr.address, sizeof(GetMonitor()->allocateStr.address));
    while(1)
    {
        time1S = TimerTask(&time1S, 1000/FILE_SYS_PERIOD, &Timer1sTouch);
        time30S = TimerTask(&time30S, 30000/FILE_SYS_PERIOD, &Timer30sTouch);

        //1s 任务
        if(ON == Timer1sTouch)
        {
            //2.存储系统信息
            CheckSysSetNeedSave(GetSysSet());//Justin debug 这个函数一直在存储 有问题
#if(HUB_SELECT == HUB_ENVIRENMENT)
            //3.存储配方
            CheckSysRecipeNeedSave(GetSysRecipt());
#elif(HUB_SELECT == HUB_IRRIGSTION)
            CheckSysTankNeedSave(GetSysTank());
#endif
        }

        //10s 任务
        if(ON == Timer30sTouch)
        {
            //存储分配的地址
            if(monitorAddrCrc !=
                    usModbusRTU_CRC(GetMonitor()->allocateStr.address, sizeof(GetMonitor()->allocateStr.address)))
            {
                monitorAddrCrc = usModbusRTU_CRC(GetMonitor()->allocateStr.address, sizeof(GetMonitor()->allocateStr.address));
                SaveMonitorAddrByJson(GetMonitor()->allocateStr);
            }

            //1.存储设备注册表
            if((sensorSize != GetMonitor()->sensor_size) ||
               (deviceSize != GetMonitor()->device_size) ||
               (lineSize != GetMonitor()->line_size))
            {
                sensorSize = GetMonitor()->sensor_size;
                deviceSize = GetMonitor()->device_size;
                lineSize = GetMonitor()->line_size;

                SaveMonitorSizeByJson(GetMonitor()->sensor_size, GetMonitor()->device_size, GetMonitor()->line_size);
            }

            CheckDeviceNeedSave(GetMonitor());
            CheckSensorNeedSave(GetMonitor());
#if(HUB_SELECT == HUB_ENVIRENMENT)
            CheckLineNeedSave(GetMonitor());
#endif
        }

        rt_thread_mdelay(FILE_SYS_PERIOD);
    }
}

/**
 * 文件系统以flash 为主存储区(因为sd卡容易被拔掉), SD卡为备份存储区
 * 主存储区存储设备注册信息、系统设置信息、桶信息等。备份存储区主要是存储firmware升级包、日志等。
 * 函数数据兼容性考虑1.旧版本迭代到新版本需要将旧结构数据存储为新结构数据；
 * 2.首先判断主存储区是否存在数据，存在的话去搜索备份存储区的旧结构数据，如果存在的话按1处理方式，不存在的话直接新建数据文件
 *
 * flash 数据存储在main文件夹下面,sd卡挂载在backup文件夹下
 */
void FileSystemInit(void)
{
    char        mainFile[]          = "main";
    char        backupFile[]        = "backup";
    char        main_dev_reg[]      = "/main/devRegistry";//新版本设备注册表存储文件夹
    char        old_dev_reg[]       = "/backup/download";
    char        reg_sensor[]        = "/main/devRegistry/sensor";//存储device类设备
    char        reg_device[]        = "/main/devRegistry/device";//存储device类设备
    char        reg_line[]          = "/main/devRegistry/line";//存储device类设备
    char        sys_set_dir[]       = "/main/sysSet";
    char        recipe_dir[]        = "/main/recipe";
    char        tank_dir[]          = "/main/tank";
    char        fileName[50];

    //1.首先将flash作为主存储区挂载到根文件夹 sd卡作为备份存储区挂载到根目录下的文件夹
    if (0 != dfs_mount(FLASH_MEMORY_NAME, "/", "elm", 0, 0))
    {
        //flash 格式化
        if(0 == dfs_mkfs("elm", FLASH_MEMORY_NAME))
        {
            dfs_mount(FLASH_MEMORY_NAME, "/", "elm", 0, 0);
        }
    }

    //flash 挂载到根目录成功,接着新建sd卡文件夹  让sd卡可以挂载
    if(RT_ERROR == CheckDirectory(mainFile))
    {
        CreateDirectory(mainFile);
    }

    if(RT_ERROR == CheckDirectory(backupFile))
    {
        CreateDirectory(backupFile);
    }
    else
    {
        //将sd挂载
        if (0 != dfs_mount(SDCARD_MEMORY_NAME, "/backup", "elm", 0, 0))
        {
            //flash 格式化
            if(0 == dfs_mkfs("elm", SDCARD_MEMORY_NAME))
            {
                dfs_mount(SDCARD_MEMORY_NAME, "/backup", "elm", 0, 0);
            }
        }
    }

    //2.检查是否存在主存储区文件夹
    if(RT_ERROR == CheckDirectory(main_dev_reg))
    {
        //2.1新建主存储区文件夹
        CreateDirectory(main_dev_reg);
        CreateDirectory(reg_sensor);
        CreateDirectory(reg_device);
        CreateDirectory(reg_line);
        CreateDirectory(sys_set_dir);
        CreateDirectory(recipe_dir);
        CreateDirectory(tank_dir);

        //2.2判断是否是由于旧版本升级来的 此时存在旧结构体数据,迁移旧数据
        if(RT_EOK == CheckDirectory(old_dev_reg))
        {
            OldDataMigration();
        }
    }
    else
    {
        //2.2如果不存在存储sensor device light的文件夹则新建
        if(RT_ERROR == CheckDirectory(reg_sensor))
        {
            CreateDirectory(reg_sensor);
            //新建sensor 文件夹
            for(u8 sen_i = 0; sen_i < SENSOR_MAX; sen_i++)
            {
                sprintf(fileName, "%s%s%d", reg_sensor, "/sensor", sen_i);
                //如果找不到文件夹 则新建文件夹
                if(RT_ERROR == CheckDirectory(fileName))
                {
                    CreateDirectory(fileName);
                }
            }
        }

        if(RT_ERROR == CheckDirectory(reg_device))
        {
            CreateDirectory(reg_device);

            for(u8 dev_i = 0; dev_i < DEVICE_MAX; dev_i++)
            {
                sprintf(fileName, "%s%s%d", reg_device, "/device", dev_i);
                //如果找不到文件夹 则新建文件夹
                if(RT_ERROR == CheckDirectory(fileName))
                {
                    CreateDirectory(fileName);
                }

                sprintf(fileName, "%s%s%d%s", reg_device, "/device", dev_i, "/timer");
                if(RT_ERROR == CheckDirectory(fileName))
                {
                    CreateDirectory(fileName);
                }

                sprintf(fileName, "%s%s%d%s", reg_device, "/device", dev_i, "/port");
                if(RT_ERROR == CheckDirectory(fileName))
                {
                    CreateDirectory(fileName);
                }
            }
        }

        if(RT_ERROR == CheckDirectory(reg_line))
        {
            CreateDirectory(reg_line);

            for(u8 line_i = 0; line_i < LINE_MAX; line_i++)
            {
                sprintf(fileName, "%s%s%d", reg_line, "/line", line_i);
                //如果找不到文件夹 则新建文件夹
                if(RT_ERROR == CheckDirectory(fileName))
                {
                    CreateDirectory(fileName);
                }
            }
        }

        if(RT_ERROR == CheckDirectory(sys_set_dir))
        {
            CreateDirectory(sys_set_dir);
        }

        if(RT_ERROR == CheckDirectory(recipe_dir))
        {
            CreateDirectory(recipe_dir);
        }

        if(RT_ERROR == CheckDirectory(tank_dir))
        {
            CreateDirectory(tank_dir);
        }
    }

    //3.读取主存储设备数据
    GetMonitorFromFile(GetMonitor());
    GetSysSetFromFile(GetSysSet());
    if(0x00 == GetSysSet()->crc)
    {
        initCloudProtocol();
    }
#if(HUB_SELECT == HUB_ENVIRENMENT)//Justin
    GetSysRecipeListFromFile(GetSysRecipt());
    if(0x00 == GetSysRecipt()->crc)
    {
        initSysRecipe();
    }
#elif(HUB_SELECT == HUB_IRRIGSTION)
    GetTankListFromFile(GetSysTank());
    if(0x00 == GetSysTank()->crc)
    {
        initSysTank();
    }
#endif

    //4.标记文件系统准备完成
    SetFileSystemState(YES);

    //5.文件系统线程
    if(RT_EOK != rt_thread_init(&file_sys_thread,
            FILE_SYS_TASK, FileSystemEntry, RT_NULL, file_sys_task, sizeof(file_sys_task), FILE_SYS_PRIORITY, 10))
    {
        LOG_E("uart thread fail");
    }
    else
    {
        dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
        rt_thread_startup(&file_sys_thread);
    }
}
