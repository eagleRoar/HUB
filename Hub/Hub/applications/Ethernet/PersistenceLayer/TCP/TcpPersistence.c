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

void RegisterHub(rt_tcpclient_t *handle)
{
    u32                 id              = 0x00000000;
    type_hubreg_t       hubReg;
    type_package_t      pack;

    ReadUniqueId(&id);
    pack.package_top.checkId = CHECKID;                        //从机识别码
    pack.package_top.answer = ANSWER_ASK;                      //应答,主动发送
    pack.package_top.function = FUNCTION;                      //功能码
    pack.package_top.id = id;                                  //发送者id
    pack.package_top.serialNum = 0x00000000;                   //序列号

    hubReg.type = ENV_TYPE;                                             //环境控制
    hubReg.version = 0x01;                                              //版本号 BCD8421编码 需要注意
    hubReg.config_id = 0x00;                                            //配置ID
    hubReg.heart = 0x10;                                                //心跳间隔
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
    type_module_t       module;                         //只读
    type_sen_reg_t      sensorReg;
    type_dev_reg_t      deviceReg;


    ReadUniqueId(&id);

    if(0 >= GetMonitor()->monitorDeviceTable.deviceManageLength)
    {
        return;
    }
    //以下顺序不能变
    pack.package_top.checkId = CHECKID;                                                         //从机识别码
    pack.package_top.answer = ANSWER_ASK;                                                       //应答,主动发送
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
                sensorReg.sensor_id = module.uuid;                                              //传感器ID
                sensorReg.type_id = module.type;                                                //类型ID
                rt_memcpy(sensorReg.name, module.module_name, MODULE_NAMESZ);                   //名称
                itoa(sensorReg.type_id, sensorReg.product_type, 16);//16指的是16进制                         //产品型号,自己管控,只有同型号的产品才能分到同一组
                sensorReg.second = 0x0001;                                                      //发送频率时间，单位s
                sensorReg.group = 0x0001;                                                       //传感器分组
                sensorReg.large_scale = module.large_scale;                                     //用途大类  ; 1 : 大气环境控制  2 ：光照环境控制指的是日光灯照明设备
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
            }
            else if(DEVICE_TYPE == module.s_or_d)
            {
                deviceReg.device_id = module.uuid;
                deviceReg.type_id = module.type;
                rt_memcpy(deviceReg.name, module.module_name, MODULE_NAMESZ);
                itoa(deviceReg.device_id, deviceReg.product_type, 16);//16指的是16进制
                deviceReg.second = 0x0001;
                deviceReg.group = 0x0001;
                deviceReg.large_scale = module.large_scale;
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
                pack.package_top.crc = CRC16((u16*)&pack+3, sizeof(struct packTop)/2 - 3 + temp/2, 0);
                pack.package_top.length = sizeof(struct packTop) + temp;
            }

            rt_tcpclient_send(handle, &pack, pack.package_top.length);
            GetMonitor()->monitorDeviceTable.deviceTable[index].registerAnswer = SEND_OK;
        }
    }

}

