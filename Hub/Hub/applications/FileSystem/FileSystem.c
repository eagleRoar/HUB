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
rt_mutex_t dynamic_mutex = RT_NULL;
static u8 fileSystemState = NO;
extern u8 saveModuleFlag;
char            new_dev_file[]          = "/main/informations/module.bin";
char            new_sysset_file[]       = "/main/informations/sys_set.bin";
char            new_recipe_file[]       = "/main/informations/recipe.bin";
char            new_tank_file[]         = "/main/informations/tank.bin";
char            new_struct_ver_file[]   = "/main/informations/ver.bin";
char            new_mqtt_url_file[]     = "/main/informations/mqtt_url.bin";

char            backup_dev_file[]       = "/backup/informations/module.bin";
char            backup_sysset_file[]    = "/backup/informations/sys_set.bin";
char            backup_recipe_file[]    = "/backup/informations/recipe.bin";
char            backup_tank_file[]      = "/backup/informations/tank.bin";
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

static void GetSysTankFromFile(sys_tank_t *list, char *fileName)
{
    static u8       FileHeadSpace       = 5;
    u16             length              = 0;

    length = FileHeadSpace;
    if(RT_EOK == ReadFileData(fileName, (u8 *)list, length, sizeof(sys_tank_t)))
    {
        LOG_I("Get tankList data OK");
    }
    else
    {
        LOG_E("Get tankList data Fail");
    }
}

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
    GetMonitorFromFile(GetMonitor(), backup_dev_file);
    SaveMonitorToFile(GetMonitor(), new_dev_file);
    GetSysSetFromFile(GetSysSet(), backup_sysset_file);
    SaveSysSetToFile(GetSysSet(), new_sysset_file);
#if(HUB_ENVIRENMENT == HUB_SELECT)
    GetRecipeListFromFile(GetSysRecipt(), backup_recipe_file);
    SaveRecipeListToFile(GetSysRecipt(), new_recipe_file);
#elif(HUB_IRRIGSTION == HUB_SELECT)
    GetSysTankFromFile(GetSysTank(), backup_tank_file);
    SaveSysTankToFile(GetSysTank(), new_tank_file);
#endif
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

    sensorSize = GetMonitor()->sensor_size;
    deviceSize = GetMonitor()->device_size;
    lineSize = GetMonitor()->line_size;
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
               (YES == saveModuleFlag))
            {
                SaveMonitorToFile(GetMonitor(), new_dev_file);

                sensorSize = GetMonitor()->sensor_size;
                deviceSize = GetMonitor()->device_size;
                lineSize = GetMonitor()->line_size;
                saveModuleFlag = NO;
            }

            if(YES == saveMqttUrlFile)
            {
                SaveMqttUrl(getMqttUrlUse(), new_mqtt_url_file);

                //重启
                rt_hw_cpu_reset();
                saveMqttUrlFile = NO;
            }
        }

        //10s 任务
        if(ON == Timer30sTouch)
        {

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

        //2.2判断是否是由于旧版本升级来的 此时存在旧结构体数据,迁移旧数据
        //数据迁移由于之前版本太多不能做兼容
//        if(RT_EOK == CheckDirectory(old_info))
//        {
//            OldDataMigration();
//            SaveMonitorToFile(GetMonitor(), new_dev_file);
//            SaveSysSetToFile(GetSysSet(), new_sysset_file);
//#if(HUB_ENVIRENMENT == HUB_SELECT)
//            SaveRecipeListToFile(GetSysRecipt(), new_recipe_file);
//#elif(HUB_IRRIGSTION == HUB_SELECT)
//            SaveSysTankToFile(GetSysTank(), new_tank_file);
//#endif
//
//            rt_kprintf("OldDataMigration get old data to new module\r\n");
//        }
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
#if(HUB_ENVIRENMENT == HUB_SELECT)
        GetRecipeListFromFile(GetSysRecipt(), new_recipe_file);
#elif(HUB_IRRIGSTION == HUB_SELECT)
        GetSysTankFromFile(GetSysTank(), new_tank_file);
#endif
        GetMqttUrlFile(getMqttUrlUse(), new_mqtt_url_file);
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
        dynamic_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
        rt_thread_startup(&file_sys_thread);
    }
}
