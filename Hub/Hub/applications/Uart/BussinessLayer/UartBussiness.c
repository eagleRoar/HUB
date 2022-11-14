/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-04-12     Administrator       the first version
 */
#include "Uart.h"
#include "UartBussiness.h"
#include "UartDataLayer.h"
#include "Module.h"
#include "InformationMonitor.h"


type_connect_t      senConnectState[SENSOR_MAX];
type_connect_t      devConnectState[DEVICE_MAX];
type_connect_t      lineConnectState[LINE_MAX];
u8                  ask_device          = 0;
u8                  ask_sensor          = 0;
u8                  ask_line            = 0;
u8                  special[DEVICE_MAX]           ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//特色操作
u8                  deviceEvent         = 0;

extern u8 saveModuleFlag;

void initConnectState(void)
{
    rt_memset(senConnectState, 0, sizeof(type_connect_t) * SENSOR_MAX);
    rt_memset(devConnectState, 0, sizeof(type_connect_t) * DEVICE_MAX);
    rt_memset(lineConnectState, 0, sizeof(type_connect_t) * LINE_MAX);
}

u8 askSensorStorage(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                 = NO;
    u8              buffer[8];
    u16             crc16Result         = 0x0000;

    if(ask_sensor >= monitor->sensor_size)
    {
       //一个循环结束
        ask_sensor = 0;
        ret = YES;
    }

    if(0 < monitor->sensor_size)
    {
        buffer[0] = monitor->sensor[ask_sensor].addr;
        buffer[1] = READ_MUTI;
        buffer[2] = (monitor->sensor[ask_sensor].ctrl_addr >> 8) & 0x00FF;
        buffer[3] = monitor->sensor[ask_sensor].ctrl_addr & 0x00FF;
        buffer[4] = (monitor->sensor[ask_sensor].storage_size >> 8) & 0x00FF;
        buffer[5] = monitor->sensor[ask_sensor].storage_size & 0x00FF;
        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

//        LOG_D("ask sensor name %s",monitor->sensor[ask_sensor].name);

        rt_device_write(serial, 0, buffer, 8);

        senConnectState[ask_sensor].send_count ++;

        senConnectState[ask_sensor].send_count ++;
//        if(senConnectState[ask_sensor].send_count >= CONNRCT_MISS_MAX)
        {
//            if(ask_sensor < monitor->sensor_size)
            {
                ask_sensor ++;
            }
        }
        senConnectState[ask_sensor].send_state = ON;
    }

    return ret;
}

u8 askLineHeart(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                         = NO;
    u8              buffer[8];
    u16             crc16Result                 = 0x0000;
    proLine_t       line_set;
    static u8       manual_state[LINE_MAX]      = {0,0};
    static u8       value_pre[LINE_MAX]         = {0,0};
    static u8       state_pre[LINE_MAX]         = {0,0};
    static time_t   protectTime[LINE_MAX]       = {0,0};

    if(ask_line >= monitor->line_size)
    {
       //一个循环结束
        ask_line = 0;
    }

    if(0 < monitor->line_size)
    {
        buffer[0] = monitor->line[ask_line].addr;
        buffer[1] = WRITE_SINGLE;
        buffer[2] = (monitor->line[ask_line].ctrl_addr >> 8) & 0x00FF;
        buffer[3] = monitor->line[ask_line].ctrl_addr & 0x00FF;

        if(manual_state[ask_line] != monitor->line[ask_line]._manual.manual)
        {
            manual_state[ask_line] = monitor->line[ask_line]._manual.manual;

            if(MANUAL_HAND_ON == manual_state[ask_line])
            {
                monitor->line[ask_line]._manual.manual_on_time_save = getTimeStamp();
            }
        }

        if(MANUAL_HAND_ON == monitor->line[ask_line]._manual.manual)
        {
            monitor->line[ask_line].d_state = ON;
            value_pre[ask_line] = monitor->line[ask_line].d_value;
            monitor->line[ask_line].d_value = 100;

            if(getTimeStamp() >=
                    (monitor->line[ask_line]._manual.manual_on_time_save +
                     monitor->line[ask_line]._manual.manual_on_time))
            {
                monitor->line[ask_line]._manual.manual = MANUAL_NO_HAND;
                monitor->line[ask_line].d_state = OFF;
                monitor->line[ask_line].d_value = value_pre[ask_line];
                saveModuleFlag = YES;
            }
        }
        else if(MANUAL_HAND_OFF == monitor->line[ask_line]._manual.manual)
        {
            monitor->line[ask_line].d_state = OFF;
        }
        else if(MANUAL_NO_HAND == monitor->line[ask_line]._manual.manual)
        {
            //如果是HID 类型的灯, 保护时间不足的话就不能开
            if(0 == ask_line)
            {
                line_set = GetSysSet()->line1Set;
            }
            else if(1 == ask_line)
            {
                line_set = GetSysSet()->line2Set;
            }

            if(LINE_HID == line_set.lightsType)
            {
                if(ON == monitor->line[ask_line].d_state)
                {
                    if(getTimeStamp() <= protectTime[ask_line] + line_set.hidDelay * 60)
                    {
                        monitor->line[ask_line].d_state = OFF;
                        LOG_W("name %s is in hot start delay",monitor->line[ask_line].name);
                    }
                }
            }
        }

        if(state_pre[ask_line] != monitor->line[ask_line].d_state)
        {
            state_pre[ask_line] = monitor->line[ask_line].d_state;

            if(OFF == monitor->line[ask_line].d_state)
            {
                protectTime[ask_line] = getTimeStamp();
            }
        }

        buffer[4] = monitor->line[ask_line].d_state;
        buffer[5] = monitor->line[ask_line].d_value;

        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

        rt_device_write(serial, 0, buffer, 8);
//        LOG_W("ask no %d line,data %x %x %x %x %x %x %x %x",ask_line,
//                buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
        lineConnectState[ask_line].send_count ++;
//        if(lineConnectState[ask_line].send_count >= CONNRCT_MISS_MAX)//Justin debug
        {
            ask_line ++;
        }
        lineConnectState[ask_line].send_state = ON;
    }

    ret = YES;
    return ret;
}

/**
 *event : 事件 如 控制端口 询问端口 更改端口
 */
u8 askDeviceHeart_new(type_monitor_t *monitor, rt_device_t serial, u8 event)
{
    u8              ret                                         = NO;
    u8              port                                        = 0;
    u16             value                                       = 0;
    u8              buffer[8];
    u16             crc16Result                                 = 0x0000;
    u16             temp                                        = 0x0000;
    device_t        *device                                     = RT_NULL;
    struct control  control[DEVICE_MAX][DEVICE_PORT_MAX];
    static u8       manual_state[DEVICE_MAX][DEVICE_PORT_MAX]   ={0};
    static u8       state_pre[DEVICE_MAX][DEVICE_PORT_MAX]      ={0};
    static time_t   protectTime[DEVICE_MAX][DEVICE_PORT_MAX]        ={0};

    if(ask_device >= monitor->device_size)
    {
       //一个循环结束
        ask_device = 0;
        ret = YES;
    }

    device = &monitor->device[ask_device];

    for(port = 0; port < device->storage_size; port++)
    {
        //1.判断是否是手动开关,如果是手动关的话需要记录时间,达到设置的时间后就关闭
        if(manual_state[ask_device][port] != device->port[port].manual.manual)
        {
            manual_state[ask_device][port] = device->port[port].manual.manual;

            //如果是手动开启的话 记录开启时的时间
            if(MANUAL_HAND_ON == manual_state[ask_device][port])
            {
                device->port[port].manual.manual_on_time_save = getTimeStamp();
            }
        }

        //2.判断是否是手动开关
        if(MANUAL_HAND_ON == device->port[port].manual.manual)
        {
            //HVAC特殊处理
            if(HVAC_6_TYPE == device->port[port].type)
            {
                if(HVAC_COOL == device->_hvac.manualOnMode)
                {
                    value = GetValueAboutHACV(device, ON, OFF);
                }
                else
                {
                    value = GetValueAboutHACV(device, OFF, ON);
                }

                control[ask_device][0].d_state = value >> 8;
                control[ask_device][0].d_value = value;
            }
            else if(IR_AIR_TYPE == device->port[port].type)
            {

                control[ask_device][0].d_state = 0xe0;
                control[ask_device][0].d_value = 0x00;
            }
            else
            {

                control[ask_device][port].d_state = ON;
            }

            if(getTimeStamp() >=
                    (device->port[port].manual.manual_on_time_save +
                            device->port[port].manual.manual_on_time))
            {
                device->port[port].manual.manual = MANUAL_NO_HAND;
                if(HVAC_6_TYPE == device->port[port].type)
                {
                    value = GetValueAboutHACV(device, OFF, OFF);

                    control[ask_device][0].d_state = value >> 8;
                    control[ask_device][0].d_value = value;
                }
                else if(IR_AIR_TYPE == device->port[port].type)
                {

                    control[ask_device][0].d_state = 0x60;
                    control[ask_device][0].d_value = 0x00;
                }
                else
                {

                    control[ask_device][port].d_state = OFF;
                }
                //保存到SD卡
                saveModuleFlag = YES;
            }
        }
        else if(MANUAL_HAND_OFF == device->port[port].manual.manual)
        {
            if(HVAC_6_TYPE == device->port[port].type)
            {
                value = GetValueAboutHACV(device, OFF, OFF);

                control[ask_device][0].d_state = 0x60;
                control[ask_device][0].d_value = 0x00;
            }
            else
            {

                control[ask_device][port].d_state = OFF;
            }
        }
        else if(MANUAL_NO_HAND == device->port[port].manual.manual)
        {
            //2.1.特殊处理hvac 的风扇是否是常开
            if(HVAC_6_TYPE == device->type)
            {
                if(ON == device->_hvac.fanNormallyOpen)
                {
                    device->port[0].ctrl.d_value |= 0x04;
                }
            }
            //2.2 如果制冷制热除湿设备开了设备保护，那么在保护时间内不使能
            else if((F_COOL == device->port[port].func) ||
                    (F_HEAT == device->port[port].func) ||
                    (F_DEHUMI == device->port[port].func))
            {
                if(ON == device->port[port].ctrl.d_state)
                {
                    if(ON == device->port[port].hotStartDelay)
                    {
                        if(getTimeStamp() <= protectTime[ask_device][port] + 5 * 60)//保护时间为5分钟
                        {
                            device->port[port].ctrl.d_state = OFF;//压缩机保护
                            LOG_W("name %s is in hot start delay",monitor->device[ask_device].name);
                        }
                    }
                }
            }
        }

        //3.制冷制热除湿设备有热保护
        if(state_pre[ask_device][port] != device->port[port].ctrl.d_state)
        {
            state_pre[ask_device][port] = device->port[port].ctrl.d_state;

            if((F_COOL == device->port[port].func) ||
               (F_HEAT == device->port[port].func) ||
               (F_DEHUMI == device->port[port].func))
            {
                if(OFF == state_pre[ask_device][port])
                {
                    protectTime[ask_device][port] = getTimeStamp();
                }
            }
        }

        //4.如果是在检修状态则不输出
        if(ON == GetSysSet()->sysPara.maintain)
        {
            device->port[port].ctrl.d_state = OFF;
            control[ask_device][port].d_state = OFF;
            //LOG_D("in maintain, all device off");
        }

    }

    if(monitor->device_size > 0)
    {
        //5.整合数据发送给相应的设备
        buffer[0] = device->addr;
        if(1 == monitor->device[ask_device].storage_size)
        {
            if(MANUAL_NO_HAND == device->port[0].manual.manual)
            {
                buffer[1] = WRITE_SINGLE;
                buffer[2] = (device->ctrl_addr >> 8) & 0x00FF;
                buffer[3] = device->ctrl_addr & 0x00FF;
                buffer[4] = device->port[0].ctrl.d_state;
                buffer[5] = device->port[0].ctrl.d_value;
            }
            else
            {
                //手动模式
                buffer[1] = WRITE_SINGLE;
                buffer[2] = (device->ctrl_addr >> 8) & 0x00FF;
                buffer[3] = device->ctrl_addr & 0x00FF;
                buffer[4] = control[ask_device][0].d_state;
                buffer[5] = control[ask_device][0].d_value;
            }
        }
        else
        {
            //5.1特殊处理,如果是AC_4则要先问类型
            if(((AC_4_TYPE == device->type) || (IO_4_TYPE == device->type)) &&
                    (YES != special[ask_device]))
            {
                buffer[1] = READ_MUTI;
                buffer[2] = 0x04;
                buffer[3] = 0x40;
                buffer[4] = device->storage_size >> 8;
                buffer[5] = device->storage_size;
                LOG_I("ask ac_4");//Justin debug 仅仅测试
            }
            else
            {
                buffer[1] = WRITE_SINGLE;
                buffer[2] = device->ctrl_addr >> 8;
                buffer[3] = device->ctrl_addr;
                for(u8 item = 0; item < device->storage_size; item++)
                {
                    if(MANUAL_NO_HAND == device->port[item].manual.manual)
                    {
                        if(ON == device->port[item].ctrl.d_state)
                        {
                            temp |= 1 << item;
                        }
                    }
                    else
                    {
                        if(ON == control[ask_device][item].d_state)
                        {
                            temp |= 1 << item;
                        }
                    }
                }
                buffer[4] = temp >> 8;
                buffer[5] = temp;
            }
        }
        crc16Result = usModbusRTU_CRC(buffer, 6);
        buffer[6] = crc16Result;                             //CRC16低位
        buffer[7] = (crc16Result>>8);                        //CRC16高位

        rt_device_write(serial, 0, buffer, 8);
        //LOG_I("ask name %s",monitor->device[ask_device].name);

        devConnectState[ask_device].send_count ++;
//        if(devConnectState[ask_device].send_count >= CONNRCT_MISS_MAX)
        {
            ask_device++;
        }
        devConnectState[ask_device].send_state = ON;
    }

    return ret;
}

void replyStrorageType(type_monitor_t *monitor, u8 addr, u8 *data, u8 dataLen)
{
    for(int i = 0; i < monitor->device_size; i++)
    {
        if(addr == monitor->device[i].addr)
        {
            if((AC_4_TYPE == monitor->device[i].type) ||
               (IO_4_TYPE == monitor->device[i].type))
            {

                LOG_W("reply ac_4 port type");//Justin debug 仅仅测试
                special[i] = YES;      //标志已经收到端口数据
                if(dataLen/2 > TIMER_GROUP)
                {
                    dataLen = TIMER_GROUP * 2;
                }

                for(u8 storage = 0; storage < dataLen/2; storage++)
                {
                    monitor->device[i].port[storage].type =
                            (data[2 * storage] << 8) | data[2 * storage + 1];
                    monitor->device[i].port[storage].func =
                            GetFuncByType(monitor->device[i].port[storage].type);
                }
                LOG_I("recv ac_4 port %x %x %x %x",monitor->device[i].port[0].type,
                        monitor->device[i].port[1].type,monitor->device[i].port[2].type,
                        monitor->device[i].port[3].type);
            }
        }
    }
}

void UpdateModuleConnect(type_monitor_t *monitor, u8 addr)//Justin debug 已经修改逻辑需要注意
{
    sensor_t *sensor = RT_NULL;
    device_t *device = RT_NULL;
    line_t *line = RT_NULL;

    sensor = GetSensorByAddr(monitor, addr);
    device = GetDeviceByAddr(monitor, addr);
    line = GetLineByAddr(monitor, addr);

    if(RT_NULL != device)
    {
        if(DEVICE_TYPE == getSOrD(device->type))
        {
            for(u8 index = 0; index < monitor->device_size; index++)
            {
                if(addr == monitor->device[index].addr)
                {
                    devConnectState[index].send_state = OFF;
                    monitor->device[index].conn_state = CON_SUCCESS;
                    devConnectState[index].send_count = 0;
                }
            }
        }
    }
    else if(RT_NULL != sensor)
    {
        if(SENSOR_TYPE == getSOrD(sensor->type))
        {
            for(u8 index = 0; index < monitor->sensor_size; index++)
            {
                if(addr == monitor->sensor[index].addr)
                {
                    senConnectState[index].send_state = OFF;
                    monitor->sensor[index].conn_state = CON_SUCCESS;
                    senConnectState[index].send_count = 0;
                }
            }
        }
    }
    else if(RT_NULL != line)
    {
        if(LINE1OR2_TYPE == getSOrD(line->type))
        {
            for(u8 index = 0; index < monitor->line_size; index++)
            {
                if(addr == monitor->line[index].addr)
                {
                    lineConnectState[index].send_state = OFF;
                    monitor->line[index].conn_state = CON_SUCCESS;
                    lineConnectState[index].send_count = 0;
                }
            }
        }
    }
}

void MonitorModuleConnect(type_monitor_t *monitor)
{
    u8                      index                          = 0;

    if(SENSOR_MAX >= monitor->sensor_size)
    {
        for(index = 0; index < monitor->sensor_size; index++)
        {
            //超过十次没有接收到认为失联
            if(senConnectState[index].send_count >= CONNRCT_MISS_MAX)
            {
                monitor->sensor[index].conn_state = CON_FAIL;
                senConnectState[index].send_count = 0;
            }
        }
    }
    else
    {
        LOG_E("MonitorModuleConnect err1");
    }

    if(DEVICE_MAX >= monitor->device_size)
    {
        for(index = 0; index < monitor->device_size; index++)
        {
            //超过十次没有接收到认为失联
            if(devConnectState[index].send_count >= CONNRCT_MISS_MAX)
            {
                monitor->device[index].conn_state = CON_FAIL;
                devConnectState[index].send_count = 0;
            }
        }
    }
    else
    {
        LOG_D("MonitorModuleConnect err2");
    }

    if(LINE_MAX >= monitor->line_size)
    {
        for(index = 0; index < monitor->line_size; index++)
        {
            if(lineConnectState[index].send_count >= CONNRCT_MISS_MAX)
            {
                monitor->line[index].conn_state = CON_FAIL;
                lineConnectState[index].send_count = 0;
            }
        }
    }
    else
    {
        LOG_D("MonitorModuleConnect err3");
    }
}

void AnalyzeData(rt_device_t serial, type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    /* 获取命令 */

    switch (data[0])
    {
        case REGISTER_CODE:
            /* device类注册 */
            AnlyzeDeviceRegister(monitor, serial ,data, dataLen, 0);
            /* 后续需要修改成如果需要修改地址的再发送从新配置地址命令 */
            break;
        default:
            /* 接受地址码 */
            AnlyzeModuleInfo(monitor, data, dataLen);
            break;
    }
}

void AnlyzeModuleInfo(type_monitor_t *monitor, u8 *data, u8 dataLen)
{
    if(YES == FindModuleByAddr(monitor, data[0]))
    {
        AnlyzeStorage(monitor, data[0], data[1],&data[3], data[2]);
        if(READ_MUTI == data[1])
        {
            replyStrorageType(monitor, data[0], &data[3], data[2]);
        }
        UpdateModuleConnect(monitor, data[0]);

//        if(RT_NULL != GetDeviceByAddr(monitor, data[0]))//Justin debug
//        {
//            LOG_W("recv device name %s",GetDeviceByAddr(monitor, data[0])->name);
//            for(u8 index = 0; index < dataLen; index++)
//            {
//                rt_kprintf("%x ",data[index]);
//            }
//            rt_kprintf("\r\n");
//        }
    }
}

void findDeviceLocation(type_monitor_t *monitor, cloudcmd_t *cmd,rt_device_t serial)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u8 addr = 0x00;

    if(0 != cmd->get_id.value)
    {
        //无论是device还是指定的port口，都是device闪烁
        if(cmd->get_id.value > 0xFF)
        {
            addr = (cmd->get_id.value >> 8) & 0x00FF;
        }
        else
        {
            addr = cmd->get_id.value;
        }

        for(i = 0; i < monitor->device_size; i++)
        {
            if(addr == monitor->device[i].addr)
            {
                buffer[0] = monitor->device[i].addr;
                buffer[1] = READ_MUTI;
                buffer[2] = 0x00;
                buffer[3] = 0x00;
                buffer[4] = 0x00;
                buffer[5] = 0x01;

                crc16Result = usModbusRTU_CRC(buffer, 6);
                buffer[6] = crc16Result;                         //CRC16低位
                buffer[7] = (crc16Result>>8);                    //CRC16高位

                rt_device_write(serial, 0, buffer, 8);

                cmd->get_id.value = 0;
            }
        }
    }
}

void findLineLocation(type_monitor_t *monitor, cloudcmd_t *cmd,rt_device_t serial)
{
    u16 i = 0;
    u8 buffer[15];
    u16 crc16Result = 0x0000;
    u8 addr = 0x00;

    if(0 != cmd->get_id.value)
    {
        //无论是device还是指定的port口，都是device闪烁
        if(cmd->get_id.value > 0xFF)
        {
            addr = (cmd->get_id.value >> 8) & 0x00FF;
        }
        else
        {
            addr = cmd->get_id.value;
        }

        for(i = 0; i < monitor->line_size; i++)
        {
            if(addr == monitor->line[i].addr)
            {
                buffer[0] = monitor->line[i].addr;
                buffer[1] = READ_MUTI;
                buffer[2] = 0x00;
                buffer[3] = 0x00;
                buffer[4] = 0x00;
                buffer[5] = 0x01;

                crc16Result = usModbusRTU_CRC(buffer, 6);
                buffer[6] = crc16Result;                         //CRC16低位
                buffer[7] = (crc16Result>>8);                    //CRC16高位

                rt_device_write(serial, 0, buffer, 8);
                cmd->get_id.value = 0;
            }
        }

    }
}

void setDeviceEvent(u8 event)
{
    deviceEvent = event;
}

u8 getDeviceEvent(void)
{
    return deviceEvent;
}
