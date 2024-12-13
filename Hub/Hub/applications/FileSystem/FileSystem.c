///*
// * Copyright (c) 2006-2021, RT-Thread Development Team
// *
// * SPDX-License-Identifier: Apache-2.0
// *
// * Change Logs:
// * Date           Author       Notes
// * 2023-02-09     Administrator       the first version
// */
//
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
//rt_mutex_t dynamic_mutex = RT_NULL;
static u8 fileSystemState = NO;
extern u8 saveModuleFlag;
u8 saveAquaInfoFlag = NO;

extern sys_set_extern *GetSysSetExtern(void);

char            new_dev_file[]          = "/main/informations/module.bin";
char            new_sysset_file[]       = "/main/informations/sys_set.bin";
char            new_recipe_file[]       = "/main/informations/recipe.bin";
char            new_tank_file[]         = "/main/informations/tank.bin";
char            new_tank_backup1_file[]         = "/main/informations/tank_backup1.bin";
char            new_tank_backup2_file[]         = "/main/informations/tank_backup2.bin";
char            new_struct_ver_file[]   = "/main/informations/ver.bin";
char            new_mqtt_url_file[]     = "/main/informations/mqtt_url.bin";
char            new_aqua_info_file[]    = "/main/informations/aqua_info.bin";
char            new_aqua_set_file[]     = "/main/informations/aqua_set.bin";
char            new_sysset_ex_file[]     = "/main/informations/sys_set_ex.bin";
char            new_special_version_file[]  = "/main/informations/special_version.txt";

char            backup_dev_file[]       = "/backup/informations/module.bin";
char            backup_sysset_file[]    = "/backup/informations/sys_set.bin";
char            backup_recipe_file[]    = "/backup/informations/recipe.bin";
char            backup_tank_file[]      = "/backup/informations/tank.bin";
char            backup_tank_backup1_file[]      = "/backup/informations/tank_backup1.bin";
char            backup_tank_backup2_file[]      = "/backup/informations/tank_backup2.bin";
char            backup_ulog[]           = "/backup/console.txt";
char            backup_special_version_file[]  = "/backup/special_version.txt";
char            error_log_file[]  = "/backup/error_log.txt";
u8              saveMqttUrlFile         = NO;

sys_ver_t   sys_ver;

void setMqttUrlFileFlag(u8 flag)
{
    saveMqttUrlFile = flag;
}

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
        if (fd >= 0)
        {
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

//static void GetStructVer(sys_ver_t *ver, char *fileName)
//{
//    if(RT_EOK == ReadFileData(fileName, (u8 *)ver, 0, sizeof(sys_ver_t)))
//    {
//        LOG_I("Get struct ver data OK");
//    }
//    else
//    {
//        LOG_E("Get struct ver data Fail");
//    }
//}

static void SaveStructVer(char *fileName)
{
    strcpy(sys_ver.hub_ver, FIRMWAREVISION);
    sys_ver.monitor_ver = MONITOR_VER;
    sys_ver.sys_set_ver = SYS_SET_VER;
    sys_ver.recipe_ver = SYS_RECIPE_VER;
    sys_ver.tank_ver = SYS_TANK_VER;

    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)&sys_ver, 0, sizeof(sys_ver_t)))
    {
        LOG_I("Save struct ver data OK");
    }
    else
    {
        LOG_E("Save struct ver data Fail");
    }
}

void SaveConsole(char *data, size_t len)//保存日志文件
{
    if(RT_EOK == WriteFileData(backup_ulog, (u8 *)data, 0, len))
    {
        LOG_I("Save ulog data OK");
    }
    else
    {
        LOG_E("Save ulog data Fail");
    }
}

static void GetMonitorFromFile(type_monitor_t *monitor, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)monitor, length, sizeof(type_monitor_t)))
    {
        rt_kprintf("-----------Get monitor data OK\r\n");
    }
    else
    {
        rt_kprintf("-----------Get monitor data Fail\r\n");
    }

}

static void SaveMonitorToFile(type_monitor_t *monitor, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)monitor, length, sizeof(type_monitor_t)))
    {
        LOG_I("-----------------save monitor data OK");
    }
    else
    {
        LOG_I("-----------------save monitor data Fail");
    }
}

static void SaveMqttUrl(type_mqtt_ip *mqtt, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)mqtt, length, sizeof(type_mqtt_ip)))
    {
        LOG_I("-----------------save MqttUrl data OK");
    }
    else
    {
        LOG_I("-----------------save MqttUrl data Fail");
    }
}

static void GetMqttUrlFile(type_mqtt_ip *mqtt, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    if(GetFileLength(fileName) > 0)
    {
        if(RT_EOK == ReadFileData(fileName, (u8 *)mqtt, length, sizeof(type_mqtt_ip)))
        {
            rt_kprintf("-----------------get MqttUrl data OK\r\n");
        }
        else
        {
            rt_kprintf("-----------------get MqttUrl data Fail\r\n");
        }
    }
    else
    {
        rt_kprintf("-----------------mqtt file no exist\r\n");
    }
}

static void GetSysSetFromFile(sys_set_t *set, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;
    char name[HUB_NAMESZ];

    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)set, length, sizeof(sys_set_t)))
    {
        rt_kprintf("-----------------Get sysSet data OK\r\n");
        if(0 == strcmp(set->hub_info.name, ""))
        {
            strcpy(set->hub_info.name, GetSnName(name, 12));
        }
    }
    else
    {
        rt_kprintf("-----------------Get sysSet data Fail\r\n");
    }
}

static void GetSpecialVersionFromFile(void)
{
    cJSON   *json   = RT_NULL;
    int     length  = 0;
    char    *data   = RT_NULL;
    //1.首先从flash查找是否存在
    if(0 == GetFileLength(new_special_version_file) &&
       0 == GetFileLength(backup_special_version_file)) {
        SetSpecialVersion(0);

    } else {
        //2.读取SD中的数据
        length = GetFileLength(backup_special_version_file);
        if(length) {
            data = rt_malloc(length);

            if(data) {
                if(RT_EOK == ReadFileData(backup_special_version_file, data, 0, length)) {
                    json = cJSON_Parse(data);

                    if(json) {
                        u8 version = 0;
                        GetValueByU8(json, "specialVersion", &version);
                        SetSpecialVersion(version);

                        //3.将Sd卡的数据写进片上flash中
                        WriteFileData(new_special_version_file, data, 0, length);


                    } else {
                        SetSpecialVersion(0);
                    }
                } else {
                    SetSpecialVersion(0);
                }

                rt_free(data);
            }
        } else {
            //4.如果sd卡不存在 则读取片上flash
            length = GetFileLength(new_special_version_file);

            if(length) {
                data = rt_malloc(length);
                if(data)
                {
                    if(RT_EOK == ReadFileData(new_special_version_file, data, 0, length)) {
                        json = cJSON_Parse(data);

                        if(json) {
                            u8 version = 0;
                            GetValueByU8(json, "specialVersion", &version);
                            SetSpecialVersion(version);
                            //3.将片上flash的数据写进Sd卡中
                            WriteFileData(backup_special_version_file, data, 0, length);


                        } else {
                            SetSpecialVersion(0);
                        }
                    } else {
                        SetSpecialVersion(0);
                    }

                    rt_free(data);
                }
            }
        }
    }

//    LOG_E("------------------------特殊版本 = %d",GetSpecialVersion());
}

static void SaveSysSetToFile(sys_set_t *set, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)set, length, sizeof(sys_set_t)))
    {
        rt_kprintf("save sysSet data OK\r\n");
    }
    else
    {
        rt_kprintf("save sysSet data Fail\r\n");
    }
}

static void GetSysSetExFromFile(sys_set_extern *set, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;
    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)set, length, sizeof(sys_set_extern)))
    {
        rt_kprintf("-----------------Get sysSetEx data OK\r\n");
    }
    else
    {
        rt_kprintf("-----------------Get sysSetEx data Fail\r\n");
    }
}

static void SaveSysSetExToFile(sys_set_extern *set, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)set, length, sizeof(sys_set_extern)))
    {
        rt_kprintf("save sysSetEx data OK\r\n");
    }
    else
    {
        rt_kprintf("save sysSetEx data Fail\r\n");
    }
}

#if(HUB_ENVIRENMENT == HUB_SELECT)
static void GetRecipeListFromFile(sys_recipe_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)list, length, sizeof(sys_recipe_t)))
    {
        LOG_I("Get recipeList data OK");
    }
    else
    {
        LOG_E("Get recipeList data Fail");
    }
}

static void SaveRecipeListToFile(sys_recipe_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)list, length, sizeof(sys_recipe_t)))
    {
        LOG_I("Get recipeList data OK");
    }
    else
    {
        LOG_E("Get recipeList data Fail");
    }
}

#elif(HUB_IRRIGSTION == HUB_SELECT)

void printTankInfo(tank_t *tank)
{

    LOG_I("printTankInfo--------------------------------");
    LOG_D("tankNo                       = %d",tank->tankNo);
    LOG_D("name                         = %s",tank->name);
    LOG_D("autoFillValveId              = %d",tank->autoFillValveId);
    LOG_D("autoFillHeight               = %d",tank->autoFillHeight);
    LOG_D("autoFillFulfilHeight         = %d",tank->autoFillFulfilHeight);
    LOG_D("highEcProtection             = %d",tank->highEcProtection);
    LOG_D("lowPhProtection              = %d",tank->lowPhProtection);
    LOG_D("highPhProtection             = %d",tank->highPhProtection);
    LOG_D("phMonitorOnly                = %d",tank->phMonitorOnly);
    LOG_D("ecMonitorOnly                = %d",tank->ecMonitorOnly);
    LOG_D("wlMonitorOnly                = %d",tank->wlMonitorOnly);
    LOG_D("mmMonitorOnly                = %d",tank->mmMonitorOnly);
    LOG_D("highMmProtection             = %d",tank->highMmProtection);
    LOG_D("pumpId                       = %x",tank->pumpId);
    LOG_D("color                        = %d",tank->color);
    LOG_D("poolTimeout                  = %d",tank->poolTimeout);
    LOG_D("aquaId                       = %d",tank->aquaId);
    LOG_D("mixId                        = %d",tank->mixId);
    rt_kprintf("valve : ");
    for(int i = 0; i < VALVE_MAX; i++)
    {
        if(0 != tank->valve[i])
        {
            rt_kprintf("%x ",tank->valve[i]);
        }
    }
    rt_kprintf("\r\n");
    rt_kprintf("nopump_valve : ");
    for(int i = 0; i < VALVE_MAX; i++)
    {
        if(0 != tank->nopump_valve[i])
        {
            rt_kprintf("%x ",tank->nopump_valve[i]);
        }
    }
    rt_kprintf("\r\n");
    for(int i = 0; i < TANK_SINGLE_GROUD; i++)
    {
        for(int j = 0; j < TANK_SENSOR_MAX; j++)
        {
            if(0 != tank->sensorId[i][j])
            {
                rt_kprintf("%x ",tank->sensorId[i][j]);
            }
        }
    }
    rt_kprintf("\r\n");
}
//Justin 未完待续
static void saveErrorLog(char *data)
{
    int length = GetFileLength(error_log_file);

    //限制长度 < 10K
    if(length > 10 * 1024)
    {
        return;
    }

    //把错误信息写入文件
    WriteFileData(error_log_file, data, length, strlen(data));
}
/**
 * *获取tank list的数据
 * @param list
 * @param fileName
 */
static void GetSysTankFromFile(sys_tank_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    //1.由于tank 设置出现数据被串改的情况，需要做多备份去测试
    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)list, length, sizeof(sys_tank_t)))
    {
        LOG_I("Get tankList data OK");
    }
    else
    {
        LOG_E("Get tankList data Fail");
    }

    list->tank_size = TANK_LIST_MAX;
    for(int i = 0; i < TANK_LIST_MAX; i++)
    {
        list->tank[i].tankNo = i + 1;
        printTankInfo(&list->tank[i]);
    }
}

//static void GetSysTankFromFile(sys_tank_t *list, char *fileName, char *file_backup1, char *file_backup2)
//{
//    static u8       FileHeadSpace       = 5;
//    u16             length              = 0;
//    sys_tank_t      tankList;
//    u16             crc                 = 0;
//    u16             crc1                = 0;
//    u16             crc2                = 0;
//    char            errData[50]         = "";
//    u8              stateFlag           = 0;
//    u8              stateFlag1          = 0;
//    u8              stateFlag2          = 0;
//    type_sys_time       time_for;
//
//
//    //1.显示时间
//    getRealTimeForMat(&time_for);
//
//    length = FileHeadSpace;
//    if(RT_EOK != ReadFileData(fileName, (u8 *)&tankList, length, sizeof(sys_tank_t)))
//    {
//        LOG_E("Get tankList data Fail");
//    }
//    else
//    {
//        crc = usModbusRTU_CRC((u8 *)&tankList + 2, sizeof(sys_tank_t) - 2);
//        //如果是crc 错误会写入错误
//        if(crc != tankList.crc)
//        {
//            sprintf(errData, "%s: %s%d%d%d%d%d%d\r\n", ERR_NO_1, "get tanklist error",
//                    time_for.year, time_for.month, time_for.day, time_for.hour, time_for.minute, time_for.second);
//            saveErrorLog(errData);
//            stateFlag = 1;
//        }
//    }
//
//    if(RT_EOK != ReadFileData(file_backup1, (u8 *)&tankList, length, sizeof(sys_tank_t)))
//    {
//        LOG_E("Get tankList backup1 data Fail");
//    }
//    else
//    {
//        crc1 = usModbusRTU_CRC((u8 *)&tankList + 2, sizeof(sys_tank_t) - 2);
//        if(crc1 != tankList.crc)
//        {
//            sprintf(errData, "%s: %s%d%d%d%d%d%d\r\n", ERR_NO_2, "get tanklist error",
//                    time_for.year, time_for.month, time_for.day, time_for.hour, time_for.minute, time_for.second);
//            saveErrorLog(errData);
//            stateFlag1 = 1;
//        }
//    }
//
//    if(RT_EOK != ReadFileData(file_backup2, (u8 *)&tankList, length, sizeof(sys_tank_t)))
//    {
//        LOG_E("Get tankList backup2 data Fail");
//    }
//    else
//    {
//        crc2 = usModbusRTU_CRC((u8 *)&tankList + 2, sizeof(sys_tank_t) - 2);
//        if(crc2 != tankList.crc)
//        {
//            sprintf(errData, "%s: %s%d%d%d%d%d%d\r\n", ERR_NO_3, "get tanklist error",
//                    time_for.year, time_for.month, time_for.day, time_for.hour, time_for.minute, time_for.second);
//            saveErrorLog(errData);
//            stateFlag2 = 1;
//        }
//    }
//
//    if((0 == stateFlag && 0 == stateFlag1) && (crc == crc1))
//    {
//        ReadFileData(fileName, (u8 *)&tankList, length, sizeof(sys_tank_t));
//        memcpy(list, (u8 *)&tankList, sizeof(sys_tank_t));
//    }
//    else if((0 == stateFlag && 0 == stateFlag2) && (crc == crc2))
//    {
//        ReadFileData(fileName, (u8 *)&tankList, length, sizeof(sys_tank_t));
//        memcpy(list, (u8 *)&tankList, sizeof(sys_tank_t));
//    }
//    else if((0 == stateFlag1 && 0 == stateFlag2) && (crc1 == crc2))
//    {
//        ReadFileData(file_backup1, (u8 *)&tankList, length, sizeof(sys_tank_t));
//        memcpy(list, (u8 *)&tankList, sizeof(sys_tank_t));
//    }
//    else
//    {
//        memset((u8 *)list, 0, sizeof(sys_tank_t));
//        sprintf(errData, "%s: %s%d%d%d%d%d%d\r\n", ERR_NO_4, "set tanklist data 0",
//                time_for.year, time_for.month, time_for.day, time_for.hour, time_for.minute, time_for.second);
//        saveErrorLog(errData);
//    }
//
//    list->tank_size = TANK_LIST_MAX;
//    for(int i = 0; i < TANK_LIST_MAX; i++)
//    {
//        list->tank[i].tankNo = i + 1;
//        printTankInfo(&list->tank[i]);
//    }
//}

static void SaveSysTankToFile(sys_tank_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)list, length, sizeof(sys_tank_t)))
    {
        LOG_I("save tankList data OK");
    }
    else
    {
        LOG_I("save tankList data Fail");
    }
}


static void GetSysAquaInfoFromFile(aqua_info_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)list, length, sizeof(aqua_info_t) * TANK_LIST_MAX))
    {
        LOG_I("Get AquaInfo data OK");

//        for(int i = 0; i < 4; i++)
//        {
//            rt_kprintf("--------------------------------------------i = %d, uuid = %x\r\n",i, list[i].uuid);
//        }
    }
    else
    {
        LOG_E("Get AquaInfo data Fail");
    }
}

static void SaveSysAquaInfoToFile(aqua_info_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)list, length, sizeof(aqua_info_t) * TANK_LIST_MAX))
    {
        LOG_I("save AquaInfo data OK");
    }
    else
    {
        LOG_I("save AquaInfo data Fail");
    }
}

static void GetSysAquaSetFromFile(aqua_set *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)list, length, sizeof(aqua_info_t) * TANK_LIST_MAX))
    {
        LOG_I("Get AquaSet data OK");
    }
    else
    {
        LOG_E("Get AquaSet data Fail");
    }
}

static void SaveSysAquaSetToFile(aqua_set *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    RemoveFileDirectory(fileName);
    if(RT_EOK == WriteFileData(fileName, (u8 *)list, length, sizeof(aqua_info_t) * TANK_LIST_MAX))
    {
        LOG_I("save AquaSet data OK");
    }
    else
    {
        LOG_I("save AquaSet data Fail");
    }
}

#endif

//数据导出
void DataExport(void)
{
    char DirName[] = "/backup/informations";
    if(RT_ERROR == CheckDirectory(DirName))
    {
        CreateDirectory(DirName);
    }

    SaveMonitorToFile(GetMonitor(), backup_dev_file);
    SaveSysSetToFile(GetSysSet(), backup_sysset_file);
#if(HUB_ENVIRENMENT == HUB_SELECT)
    SaveRecipeListToFile(GetSysRecipt(), backup_recipe_file);
#elif(HUB_IRRIGSTION == HUB_SELECT)
    SaveSysTankToFile(GetSysTank(), backup_tank_file);
#endif
}

//数据导入
void DataImport(void)
{
    //Justin debug 暂时屏蔽数据导入功能 避免误操作导致数据错乱
//    GetMonitorFromFile(GetMonitor(), backup_dev_file);
//    SaveMonitorToFile(GetMonitor(), new_dev_file);
//    GetSysSetFromFile(GetSysSet(), backup_sysset_file);
//    SaveSysSetToFile(GetSysSet(), new_sysset_file);
//#if(HUB_ENVIRENMENT == HUB_SELECT)
//    GetRecipeListFromFile(GetSysRecipt(), backup_recipe_file);
//    SaveRecipeListToFile(GetSysRecipt(), new_recipe_file);
//#elif(HUB_IRRIGSTION == HUB_SELECT)
//    GetSysTankFromFile(GetSysTank(), backup_tank_file);
//    SaveSysTankToFile(GetSysTank(), new_tank_file);
//#endif
}

void RestoreFactorySettings(void)
{
    dfs_mkfs("elm", FLASH_MEMORY_NAME);
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
#if(HUB_SELECT == HUB_IRRIGSTION)
    static      u8              aquaSize = 0;
#endif

    sensorSize = GetMonitor()->sensor_size;
    deviceSize = GetMonitor()->device_size;
    lineSize = GetMonitor()->line_size;
#if(HUB_SELECT == HUB_IRRIGSTION)
    aquaSize = GetMonitor()->aqua_size;
#endif
    sys_set_extern *sys_set_ex = GetSysSetExtern();

    while(1)
    {
        time1S = TimerTask(&time1S, 1000/FILE_SYS_PERIOD, &Timer1sTouch);
        time30S = TimerTask(&time30S, 30000/FILE_SYS_PERIOD, &Timer30sTouch);

        //1s 任务
        if(ON == Timer1sTouch)
        {
            if((sensorSize != GetMonitor()->sensor_size) ||
               (deviceSize != GetMonitor()->device_size) ||
               (lineSize != GetMonitor()->line_size) ||
#if(HUB_SELECT == HUB_IRRIGSTION)
               (aquaSize != GetMonitor()->aqua_size) ||
#endif
               (YES == saveModuleFlag))
            {
                SaveMonitorToFile(GetMonitor(), new_dev_file);

                sensorSize = GetMonitor()->sensor_size;
                deviceSize = GetMonitor()->device_size;
                lineSize = GetMonitor()->line_size;
#if(HUB_SELECT == HUB_IRRIGSTION)
                aquaSize = GetMonitor()->aqua_size;
#endif
                saveModuleFlag = NO;
            }

            if(YES == saveMqttUrlFile)
            {
                SaveMqttUrl(getMqttUrlUse(), new_mqtt_url_file);

                //重启
                rt_hw_cpu_reset();
                saveMqttUrlFile = NO;
            }
#if(HUB_SELECT == HUB_IRRIGSTION)
            if(YES == saveAquaInfoFlag)
            {
                SaveSysAquaInfoToFile(GetAquaInfoList(), new_aqua_info_file);
                SaveSysAquaSetToFile(GetAquaSetList(), new_aqua_set_file);
                saveAquaInfoFlag = NO;
            }
#endif

        }

        //10s 任务
        if(ON == Timer30sTouch)
        {
            if(YES == sys_set_ex->saveFlag)
            {
                SaveSysSetExToFile(GetSysSetExtern(), new_sysset_ex_file);
                sys_set_ex->saveFlag = NO;
            }

            if(YES == GetSysSet()->saveFlag)
            {
                SaveSysSetToFile(GetSysSet(), new_sysset_file);

                GetSysSet()->crc = usModbusRTU_CRC((u8 *)GetSysSet()+2, sizeof(sys_set_t) - 2);
                GetSysSet()->saveFlag = NO;
            }
#if(HUB_ENVIRENMENT == HUB_SELECT)
            if(YES == GetSysRecipt()->saveFlag)
            {
                SaveRecipeListToFile(GetSysRecipt(), new_recipe_file);

                GetSysRecipt()->crc = usModbusRTU_CRC((u8 *)GetSysRecipt()+2, sizeof(sys_recipe_t) - 2);
                GetSysRecipt()->saveFlag = NO;
            }
#elif(HUB_IRRIGSTION == HUB_SELECT)
            if(YES == GetSysTank()->saveFlag)
            {
                SaveSysTankToFile(GetSysTank(), new_tank_file);
                LOG_I("---------------------SaveSysTankToFile");

                GetSysTank()->crc = usModbusRTU_CRC((u8 *)GetSysTank() + 2, sizeof(sys_tank_t) - 2);
                GetSysTank()->saveFlag = NO;
            }
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
    char        main_information[]  = "/main/informations";
//    char        old_info[]          = "/backup/moduleInfo";
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

    //将sd挂载
    if (0 != dfs_mount(SDCARD_MEMORY_NAME, "/backup", "elm", 0, 0))
    {
        //flash 格式化
        if(0 == dfs_mkfs("elm", SDCARD_MEMORY_NAME))
        {
            dfs_mount(SDCARD_MEMORY_NAME, "/backup", "elm", 0, 0);
        }
    }
    else
    {
    }

    //2.检查是否存在主存储区文件夹
    if(RT_ERROR == CheckDirectory(main_information))
    {
        //2.1新建主存储区文件夹
        if(RT_EOK == CreateDirectory(main_information))
        {
            LOG_I("create main informations file OK");
        }
        else
        {
            LOG_E("create main informations file Fail");
        }
    }
    else
    {
        //3.读取主存储设备数据
        GetMonitorFromFile(GetMonitor(), new_dev_file);
        if(0 == GetFileLength(new_sysset_file))
        {
            initCloudProtocol();
        }
        else
        {
            GetSysSetFromFile(GetSysSet(), new_sysset_file);
        }

        if(GetFileLength(new_sysset_ex_file))
        {
            GetSysSetExFromFile(GetSysSetExtern(), new_sysset_ex_file);
        }
#if(HUB_ENVIRENMENT == HUB_SELECT)
        GetRecipeListFromFile(GetSysRecipt(), new_recipe_file);
#elif(HUB_IRRIGSTION == HUB_SELECT)
//        GetSysTankFromFile(GetSysTank(), new_tank_file, new_tank_backup1_file, new_tank_backup2_file);
        GetSysTankFromFile(GetSysTank(), new_tank_file);
        GetSysAquaInfoFromFile(GetAquaInfoList(), new_aqua_info_file);
        GetSysAquaSetFromFile(GetAquaSetList(), new_aqua_set_file);
#endif
        GetMqttUrlFile(getMqttUrlUse(), new_mqtt_url_file);
        GetSpecialVersionFromFile();
    }

    //4.标记文件系统准备完成
    SetFileSystemState(YES);

    SaveStructVer(new_struct_ver_file);

    //5.文件系统线程
    if(RT_EOK != rt_thread_init(&file_sys_thread,
            FILE_SYS_TASK, FileSystemEntry, RT_NULL, file_sys_task, sizeof(file_sys_task), FILE_SYS_PRIORITY, 10))
    {
        LOG_E("uart thread fail");
    }
    else
    {
//        dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
        rt_thread_startup(&file_sys_thread);
    }
}
