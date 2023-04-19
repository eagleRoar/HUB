/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-13     Administrator       the first version
 */

#include "compatibility.h"
#include "SDCardBusiness.h"
#include "CloudProtocolBusiness.h"

extern u8 CheckDirectory(char*);

//存入版本2
//void saveToSDTest()
//{
//    jtest2 test;
//    char    *str        = RT_NULL;
//    cJSON *json = cJSON_CreateObject();
//
//    strncpy(test.name,"hell",5);
//    strncpy(test.name1,"o",2);
//    test.age = 20;
//
//    CheckDirectory(CJSON_DIR);
//
//    if(json)
//    {
//        cJSON_AddStringToObject(json, "name", test.name);
//        cJSON_AddStringToObject(json, "name1", test.name1);
//        cJSON_AddNumberToObject(json, "age", test.age);
//
//        str = cJSON_PrintUnformatted(json);
//        cJSON_Delete(json);
//
//        LOG_I("saveToSDTest memory = %d",rt_strlen(str));
//
//        //1.save to sd
//        if(RT_EOK == WriteFileData(CJSON_FILE, (u8 *)str, 0, rt_strlen(str)))
//        {
//            LOG_W("----------------------------- save test ok");
//        }
//    }
//
//}

//struct device{
//    u16             crc;
//    u32             uuid;
//    char            name[MODULE_NAMESZ];
//    u8              addr;                                   //hub管控的地址
//    u8              type;                                   //类型
//    u16             ctrl_addr;                              //实际上终端需要控制的地址
//    u8              main_type;                              //主类型 如co2 温度 湿度 line timer
//    u8              conn_state;                             //连接状态
//    u8              reg_state;                              //注册状态
//    u8              save_state;                             //是否已经存储
//    u8              storage_size;                           //寄存器数量
//    u8              color;                                  //颜色
//    //12个端口需要 state time cycle
//    struct port{
//        char    name[STORAGE_NAMESZ];
//        u16     addr;
//        u8      type;                                       //类型
//        u8      hotStartDelay;                              //对于制冷 制热 除湿设备需要有该保护
//        u8      mode;                                       // 模式 1-By Schedule 2-By Recycle
//        u8      func;
//        //timer
//        type_timmer_t timer[TIMER_GROUP];
//        type_cycle_t cycle;
//        type_manual_t manual;
//        type_ctrl_t ctrl;
//    }port[DEVICE_PORT_MAX];
//    //特殊处理
//    struct hvac
//    {
//        u8      manualOnMode;        //1-cooling 2-heating //手动开关的时候 该选项才有意义
//        u8      fanNormallyOpen;     //风扇常开 1-常开 0-自动
//        u8      hvacMode;            //1-conventional 模式 2-HEAT PUM 模式 O 模式 3-HEAT PUM 模式 B 模式
//    }_hvac;
//};

void saveToSDTest()
{
    u8 index = 0;
    char    *str        = RT_NULL;
    cJSON *json = cJSON_CreateObject();
    cJSON *device = RT_NULL;
    cJSON *item = RT_NULL;
    type_monitor_t *monitor = GetMonitor();

    CheckDirectory(CJSON_DIR);

    if(json)
    {
        cJSON_AddNumberToObject(json, "device_size", monitor->device_size);

        device = cJSON_CreateArray();
        if(device)
        {
            for(index = 0; index < monitor->device_size; index++)
            {
                item = cJSON_CreateObject();
                if(item)
                {
                    cJSON_AddNumberToObject(item, "crc", monitor->device[index].crc);
                    cJSON_AddNumberToObject(item, "uuid", monitor->device[index].uuid);
                    cJSON_AddNumberToObject(item, "addr", monitor->device[index].addr);
                    cJSON_AddNumberToObject(item, "type", monitor->device[index].type);
                    cJSON_AddNumberToObject(item, "ctrl_addr", monitor->device[index].ctrl_addr);
                    cJSON_AddNumberToObject(item, "main_type", monitor->device[index].main_type);
                    cJSON_AddNumberToObject(item, "conn_state", monitor->device[index].conn_state);
                    cJSON_AddNumberToObject(item, "reg_state", monitor->device[index].reg_state);
                    cJSON_AddNumberToObject(item, "save_state", monitor->device[index].save_state);
                    cJSON_AddNumberToObject(item, "storage_size", monitor->device[index].storage_size);
                    cJSON_AddNumberToObject(item, "color", monitor->device[index].color);
                    cJSON_AddNumberToObject(item, "reg_state", monitor->device[index].reg_state);
                    cJSON_AddStringToObject(item, "name", monitor->device[index].name);

                    cJSON_AddItemToArray(device, item);
                }
            }

            cJSON_AddItemToObject(json, "device", device);
        }

        str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);

        LOG_I("saveToSDTest memory = %d",rt_strlen(str));

        //1.save to sd
        if(RT_EOK == WriteFileData(CJSON_FILE, (u8 *)str, 0, rt_strlen(str)))
        {
            LOG_D("%s",str);
            cJSON_free(str);
        }
    }

}

//向前兼容
void readFromSDtest_Forward()
{
    u8 data[100];
    jtest1 test;
    cJSON *json = RT_NULL;

    if(RT_EOK == ReadFileData(CJSON_FILE, data, 0, 100))
    {
        json = cJSON_Parse(data);

        if(json)
        {
            GetValueByC16(json, "name", test.name, 5);
            GetValueByInt(json, "age", &test.age);

            cJSON_Delete(json);
        }
    }

    LOG_D("-----------name = %s",test.name);
    LOG_D("-----------age = %d",test.age);
}

//向后兼容
void readFromSDtest_Backward()
{
    u8 data[100];
    jtest3 test;
    cJSON *json = RT_NULL;

    rt_memset(&test, 0, sizeof(jtest3));

    if(RT_EOK == ReadFileData(CJSON_FILE, data, 0, 100))
    {
        json = cJSON_Parse(data);

        if(json)
        {
            GetValueByC16(json, "name", test.name, 5);
            GetValueByC16(json, "name1", test.name1, 2);
            GetValueByInt(json, "age", &test.age);
            GetValueByU8(json, "num", &test.num);

            cJSON_Delete(json);
        }
    }

    LOG_D("---------------name = %s",test.name);
    LOG_D("---------------name1 = %s",test.name1);
    LOG_D("---------------age = %d",test.age);
    LOG_D("---------------num = %d",test.num);
}
