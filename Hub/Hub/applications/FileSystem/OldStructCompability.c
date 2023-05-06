#include "FileSystem.h"
#include "cJSON.h"
#include "CloudProtocolBusiness.h"
#include "SDCardData.h"
#include "Module.h"
#include "Uart.h"
#include "recipe.h"

static void SaveOldMonitorAddrByJson(struct allocate_old all_addr)
{
    char        reg_addr_file[]     = "/main/devRegistry/reg_addr.bin";//新版本设备注册表存储文件夹
    cJSON       *cjson              = RT_NULL;
    cJSON       *item               = RT_NULL;
    char        *str                = RT_NULL;

    LOG_I("SaveOldMonitorAddrByJson");
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
            LOG_E("SaveOldMonitorAddrByJson str == NULL");

        }
    }
}

static void SaveOldMonitorSizeByJson(u8 sen_size, u8 dev_size, u8 line_size)
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

/*
 * 将旧数据device类转化为cjson格式
 * saveFile : 保存的文件的文件夹位置
 */
static void SaveOldSensorByJson(sensorOld_t sensor, u8 no, char *saveFile)
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

static void SaveOldDevPortByJson(deviceOld_t device, u8 no, char *saveFile)
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
static void SaveOldDevTimerByJson(deviceOld_t device, u8 no, char *saveFile)
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
static void SaveOldDeviceByJson(deviceOld_t device, u8 no, char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

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
            SaveOldDevPortByJson(device, i, file);
        }

        //保存timer,(由于json很费空间，需要单独存储)
        sprintf(file, "%s%s%d%s", saveFile, "/device", no, "/timer");
        for(int i = 0; i < port_size; i++)
        {
            if((device.port[i].type == VALVE_TYPE) ||
               (device.port[i].type == PUMP_TYPE) ||
               (device.port[i].type == TIMER_TYPE))
            {
                SaveOldDevTimerByJson(device, i, file);
            }
        }
    }
    else
    {
        LOG_E("SaveDeviceByJson, apply json memory fail");
    }
}

/*
 * 将旧数据device类转化为cjson格式
 * saveFile : 保存的文件的文件夹位置
 */
static void SaveOldLineByJson(lineOld_t light, u8 no, char *saveFile)
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
        cJSON_AddNumberToObject(cjson, "storage_size", 1);

        cJSON *portList = cJSON_CreateArray();
        if(portList)
        {
            cJSON *item = cJSON_CreateObject();
            if(item)
            {
                cJSON_AddNumberToObject(item, "d_state", light.d_state);
                cJSON_AddNumberToObject(item, "d_value", light.d_value);
                cJSON_AddNumberToObject(item, "manual", light._manual.manual);
                cJSON_AddNumberToObject(item, "manual_on_time", light._manual.manual_on_time);
                cJSON_AddNumberToObject(item, "manual_on_time_save", light._manual.manual_on_time_save);
                cJSON_AddItemToArray(portList, item);
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
                LOG_E("SaveOldLineByJson, write to %s err",file);
            }
            else
            {
                LOG_I("SaveOldLineByJson write to file ok");

            }

            cJSON_free(str);
            str = RT_NULL;
        }
    }
}

static void SaveOldSysCo2CalByJson(sys_setOld_t sys_set,char *saveFile)
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

static void SaveOldSysStageList(stageOld_t *stage)
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
        for(int i = 0; i < 5; i++)
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

static void SaveOldSysWarn(sys_warnOld_t *warn)
{
    char        sys_warn_dir[]         = "/main/sysSet/warn.txt";
    char        *str                        = RT_NULL;
    char        file[20]                    = "";//存储device类设备
    cJSON       *cjson                       = RT_NULL;
//    cJSON       *item                       = RT_NULL;

    cjson = cJSON_CreateObject();
    if(cjson)
    {

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
//        cJSON_AddNumberToObject(cjson, "o2ProtectionEn", warn->o2ProtectionEn);
        cJSON_AddNumberToObject(cjson, "phEn", warn->phEn);
        cJSON_AddNumberToObject(cjson, "ecEn", warn->ecEn);
        cJSON_AddNumberToObject(cjson, "wtEn", warn->wtEn);
        cJSON_AddNumberToObject(cjson, "wlEn", warn->wlEn);
//        cJSON_AddNumberToObject(cjson, "mmEn", warn->mmEn);
//        cJSON_AddNumberToObject(cjson, "meEn", warn->meEn);
//        cJSON_AddNumberToObject(cjson, "mtEn", warn->mtEn);
        cJSON_AddNumberToObject(cjson, "autoFillTimeout", warn->autoFillTimeout);
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

static void SaveOldSysPhCalByJson(sys_setOld_t sys_set,char *saveFile)
{
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cJSON *phCal = cJSON_CreateArray();
    if(phCal)
    {
        for(int i = 0; i < 20; i++)
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

static void SaveOldSysEcCalByJson(sys_setOld_t sys_set,char *saveFile)
{
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    cJSON *ecCal = cJSON_CreateArray();
    if(ecCal)
    {
        for(int i = 0; i < 20; i++)
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

static void saveOldSysParaByJson(sys_paraOld_t *sysPara)
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

static void SaveOldSysSetByJson(sys_setOld_t sys_set,char *saveFile)
{
    cJSON       *cjson              = RT_NULL;
    cJSON       *line               = RT_NULL;
//    cJSON       *sysPara            = RT_NULL;
    char        file[50]            = "";//存储device类设备
    char        *str                = RT_NULL;

    //1.生成json 格式
    cjson = cJSON_CreateObject();

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

#if(HUB_SELECT == HUB_ENVIRENMENT)
        SaveOldSysCo2CalByJson(sys_set, saveFile);
        SaveOldSysStageList(&sys_set.stageSet);
        SaveOldSysWarn(&sys_set.sysWarn);
        saveOldSysParaByJson(&sys_set.sysPara);//Justin debug 未测试
#elif(HUB_SELECT == HUB_IRRIGSTION)
        SaveOldSysPhCalByJson(sys_set, saveFile);
        SaveOldSysEcCalByJson(sys_set, saveFile);
#endif
}
#if(HUB_SELECT == HUB_ENVIRENMENT)
static void SaveOldRecipeSizeByJson(u8 recipe_size, u16 crc)
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

static void SaveOldRecipeByJson(recipeOld_t recipe, u8 no, char *saveFile)
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

static void SaveOldRecipeAddrByJson(u8 *addr, u8 size, char *saveFile)
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

#elif(HUB_SELECT == HUB_IRRIGSTION)
static void SaveOldTankSizeByJson(u8 size, u16 crc, char *saveFile)
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

static void SaveOldTankByJson(tankOld_t tank, u8 no, char *saveFile)
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
            for(u8 i = 0; i < 16; i++)
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
                    for(u8 j = 0; j < 4; j++)
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
#endif
//旧数据迁移
void OldDataMigration(void)
{
    u8              FileHeadSpace       = 5;//旧结构文件头部预留4Byte 0x5A5AA5A5作为文件已写的标识 + 1byte为标记为
    char            old_dev_reg[]       = "/backup/moduleInfo";
    char            old_dev_file[]      = "/backup/moduleInfo/module.bin";
    char            old_sysset_file[]   = "/backup/moduleInfo/sys_set.bin";
    char            reg_sensor[]        = "/main/devRegistry/sensor";//存储device类设备
    char            reg_device[]        = "/main/devRegistry/device";//存储device类设备
    char            reg_line[]          = "/main/devRegistry/line";//存储device类设备
    char            sys_set_dir[]       = "/main/sysSet";
//#if(HUB_SELECT == HUB_ENVIRENMENT)
    char            old_recipe_file[]   = "/backup/moduleInfo/recipe.bin";
    char            recipe_dir[]        = "/main/recipe";
//#elif(HUB_SELECT == HUB_IRRIGSTION)
    char            tank_dir[]          = "/main/tank";
    char            old_tank_file[]     = "/backup/moduleInfo/tank.bin";
    char            recipe_file[]       = "/main/recipe/recipe.txt";
    tankOld_t          tank;
//#endif
    struct          allocate_old all_addr;
    sensorOld_t        sensor;
    deviceOld_t        device;
    lineOld_t          light;
    u8              num[3];
    sys_setOld_t       sys_set;
    recipeOld_t        recipe;

    //1.读取旧的设备注册信息
    if(RT_EOK == CheckDirectory(old_dev_reg))
    {
        //读取地址配置列表
        if(RT_EOK == ReadFileData(old_dev_file, &all_addr, FileHeadSpace, sizeof(struct allocate_old)))
        {
            SaveOldMonitorAddrByJson(all_addr);
        }
        //读取sensor size
        if(RT_EOK == ReadFileData(old_dev_file, &num, FileHeadSpace + sizeof(struct allocate_old), 3))
        {
            SaveOldMonitorSizeByJson(num[0],num[1],num[2]);
        }

        //读取sensor
        for(u8 i = 0; i < 20; i++)
        {
            if(RT_EOK == ReadFileData(old_dev_file, &sensor,
                    FileHeadSpace + sizeof(struct allocate_old) + 3 + sizeof(sensorOld_t) *i,
                    sizeof(sensorOld_t)))
            {
                if(sensor.addr)//只存入有效值
                {
                    SaveOldSensorByJson(sensor, i, reg_sensor);
                }
            }
        }

        for(u8 i = 0; i < 16; i++)//旧数据结构的最大device数量为16
        {
            if(RT_EOK == ReadFileData(old_dev_file, &device,
                    FileHeadSpace + sizeof(struct allocate_old) + 3 + sizeof(sensorOld_t) * 20 + sizeof(deviceOld_t)*i,
                    sizeof(deviceOld_t)))
            {
                if(device.addr)
                {
                    SaveOldDeviceByJson(device, i, reg_device);
                }
            }
        }

        for(u8 i = 0; i < 2; i++)
        {
            if(RT_EOK == ReadFileData(old_dev_file, &light,
                    FileHeadSpace + sizeof(struct allocate_old) + 3 + sizeof(sensorOld_t) * 20 + sizeof(deviceOld_t) * 16 +
                    sizeof(lineOld_t) * i,
                    sizeof(lineOld_t)))
            {
                if(light.addr)
                {
                    SaveOldLineByJson(light, i, reg_line);
                }
            }
        }

        //2.读取旧的系统设置
        if(RT_EOK == ReadFileData(old_sysset_file, &sys_set,
            FileHeadSpace,
            sizeof(sys_set_t)))
        {
            SaveOldSysSetByJson(sys_set, sys_set_dir);
        }
//
//        //3.如果是环控的则需要保存配方
#if(HUB_SELECT == HUB_ENVIRENMENT)
        u8 recipe_size = 0;
        u16 crc = 0;
        //保存配方数据
        if(RT_EOK == ReadFileData(old_recipe_file, &recipe_size, FileHeadSpace + 2, 1))
        {
            if(ReadFileData(old_recipe_file, &crc, FileHeadSpace, 2))
            {
                SaveOldRecipeSizeByJson(recipe_size, crc);
            }
        }
        //保存配方
        recipe_size = recipe_size < 10 ? recipe_size : 10;
        for(u8 i = 0; i < recipe_size; i++)
        {
            if(RT_EOK == ReadFileData(old_recipe_file, &recipe, FileHeadSpace + 3 + sizeof(recipe_t) * i, sizeof(recipe_t)))
            {
                SaveOldRecipeByJson(recipe, i, recipe_dir);
            }
        }
        //保存配方地址
        u8 addr[10];
        if(RT_EOK == ReadFileData(old_recipe_file, addr, FileHeadSpace + 3 + sizeof(recipe_t) * 10, 10))
        {
            SaveOldRecipeAddrByJson(addr, 10, recipe_dir);
        }
        //4.如果是灌溉则要保存水桶信息
#elif(HUB_SELECT == HUB_IRRIGSTION)
        u8 tank_size = 0;
        u16 crc = 0;
        if(RT_EOK == ReadFileData(old_tank_file, &tank_size, FileHeadSpace + 2, 1))
        {
            if(RT_EOK == ReadFileData(old_tank_file, &crc, FileHeadSpace, 2))
            {
                SaveOldTankSizeByJson(tank_size, crc, tank_dir);
            }
        }

        tank_size = tank_size < 4 ? tank_size : 4;
        for(u8 i = 0; i < tank_size; i++)
        {
            if(RT_EOK == ReadFileData(old_tank_file, &tank, FileHeadSpace + 3 + sizeof(tank_t) * i, sizeof(tank_t)))
            {
                SaveOldTankByJson(tank, i, tank_dir);
            }
        }
#endif
    }
}

//按照原来的数据结构的
//未完待续 Justin debug
void OldDataMigration1(void)
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

    //读取旧数据
    //1.monitor
    lenght = FileHeadSpace;
    if(RT_EOK == ReadFileData(old_dev_file, &allocateStr, lenght, sizeof(struct allocate_old)))
    {
        rt_memcpy(newMonitor->allocateStr, allocateStr, sizeof(struct allocate_old));
    }
    lenght += sizeof(struct allocate_old);

    if(RT_EOK == ReadFileData(old_dev_file, size, lenght, sizeof(size)))
    {
        newMonitor->sensor_size = size[0];
        newMonitor->device_size = size[1];
        newMonitor->line_size = size[2];
    }
    lenght += sizeof(size);

    temp = newMonitor->sensor_size < SENSOR_MAX ? newMonitor->sensor_size : SENSOR_MAX;
    for(int i = 0; i < temp; i++)
    {
        if(RT_EOK == ReadFileData(old_dev_file, &sensor, lenght, sizeof(sensorOld_t)))
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
            rt_memcpy(newMonitor->sensor[i].__stora, sensor.__stora, size(sen_storaOld_t) * 4);
        }

    }
    lenght += sizeof(sensorOld_t) * 20;

    temp = newMonitor->device_size < DEVICE_MAX ? newMonitor->device_size : DEVICE_MAX;
    for(int i = 0; i < temp; i++)
    {
        if(RT_EOK == ReadFileData(old_dev_file, &device, lenght, sizeof(deviceOld_t)))
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
        }

    }
    lenght += sizeof(deviceOld_t) * 16;

    temp = newMonitor->line_size < LINE_MAX ? newMonitor->line_size : LINE_MAX;
    for(int i = 0; i < 2; i++)
    {
        if(RT_EOK == ReadFileData(old_dev_file, &line, lenght, sizeof(lineOld_t)))
        {
            newMonitor->line[i].crc = line.crc;
            newMonitor->line[i].type = line.type;
            newMonitor->line[i].uuid = line.uuid;
            strncpy(newMonitor->line[i].name, line.name, MODULE_NAMESZ);
            newMonitor->line[i].addr = line.addr;
            newMonitor->line[i].ctrl_addr = line.ctrl_addr;
            newMonitor->line[i].port[0].ctrl.d_state = line.d_state;
            newMonitor->line[i].port[0].ctrl.d_value = line.d_value;
            rt_memcpy(newMonitor->line[i].port[0]._manual, line._manual, sizeof(type_manual_t));
            newMonitor->line[i].storage_size = 1;
            newMonitor->line[i].save_state = line.save_state;
            newMonitor->line[i].conn_state = line.conn_state;
        }
    }
    lenght += sizeof(lineOld_t) * 2;

    ReadFileData(old_dev_file, &newMonitor->crc, lenght, 2);

    //2.获取sys_set
    lenght = FileHeadSpace;
    if(RT_EOK == ReadFileData(old_sysset_file, &oldSet, lenght, sizeof(sys_setOld_t)))
    {
        rt_memset(&newSet, 0, sizeof(sys_set_t));

        newSet->crc = oldSet.crc;
        rt_memcpy((u8 *)&newSet->tempSet, (u8 *)&oldSet.tempSet, sizeof(proTempSetOld_t));
        rt_memcpy((u8 *)&newSet->co2Set, (u8 *)&oldSet.co2Set, sizeof(proCo2SetOld_t));
        rt_memcpy((u8 *)&newSet->humiSet, (u8 *)&oldSet.humiSet, sizeof(proHumiSetOld_t));
        rt_memcpy((u8 *)&newSet->line1Set, (u8 *)&oldSet.line1Set, sizeof(proLineOld_t));
        newSet->line1_4Set.brightMode = LINE_MODE_BY_POWER;
        newSet->line1_4Set.byAutoDimming = AUTO_DIMMING;
        newSet->line1_4Set.mode = 1;
        newSet->line1_4Set.tempStartDimming = 350;
        newSet->line1_4Set.tempOffDimming = 400;
        newSet->line1_4Set.sunriseSunSet = 10;
        rt_memcpy((u8 *)&newSet->line2Set, (u8 *)&oldSet.line2Set, sizeof(proLineOld_t));
        rt_memcpy((u8 *)&newSet->stageSet, (u8 *)&oldSet.stageSet, sizeof(stageOld_t));
        rt_memcpy(newSet->co2Cal, oldSet.co2Cal, 20);
        for(int port = 0; port < 20; port++)
        {
            newSet->ph[port].ph_a = oldSet.ph[port].ph_a;
            newSet->ph[port].ph_b = oldSet.ph[port].ph_b;
            newSet->ph[port].uuid = oldSet.ph[port].uuid;
        }

        for(int port = 0; port < 20; port++)
        {
            newSet->ec[port].ec_a = oldSet.ec[port].ec_a;
            newSet->ec[port].ec_b = oldSet.ec[port].ec_b;
            newSet->ec[port].uuid = oldSet.ec[port].uuid;
        }
        newSet->startCalFlg = oldSet.startCalFlg;
        newSet->hub_info = oldSet.hub_info;
        newSet->ver = HUB_VER_NO;
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
        }

        lenght += sizeof(recipeOld_t);
    }

    ReadFileData(old_recipe_file, newRecipeList->allot_add, lenght, 10);
    lenght += 10;

    ReadFileData(old_recipe_file, newRecipeList->saveFlag, lenght, 1);
}
