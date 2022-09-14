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
type_connect_t      devConnectState[DEVICE_TIME4_MAX];
type_connect_t      timeConnectState[TIME12_MAX];
type_connect_t      lineConnectState[LINE_MAX];
u8                  ask_device          = 0;
u8                  ask_time12          = 0;
u8                  ask_sensor          = 0;
u8                  ask_line            = 0;
u8                  special[DEVICE_TIME4_MAX]           ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};//特色操作

extern u8 saveModuleFlag;

void initConnectState(void)
{
    rt_memset(senConnectState, 0, sizeof(type_connect_t) * SENSOR_MAX);
    rt_memset(devConnectState, 0, sizeof(type_connect_t) * DEVICE_TIME4_MAX);
    rt_memset(timeConnectState, 0, sizeof(type_connect_t) * TIME12_MAX);
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

        rt_device_write(serial, 0, buffer, 8);

        senConnectState[ask_sensor].send_count ++;

        senConnectState[ask_sensor].send_count ++;
        if(senConnectState[ask_sensor].send_count >= CONNRCT_MISS_MAX)
        {
            if(ask_sensor < monitor->sensor_size)
            {
                ask_sensor ++;
            }
        }
        senConnectState[ask_sensor].send_state = ON;
    }

    ret = YES;
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
        lineConnectState[ask_line].send_count ++;
        if(lineConnectState[ask_line].send_count >= CONNRCT_MISS_MAX)
        {
            ask_line ++;
        }
        lineConnectState[ask_line].send_state = ON;
    }

    ret = YES;
    return ret;
}

u8 askDeviceHeart(type_monitor_t *monitor, rt_device_t serial)
{
    u8              ret                                 = NO;
    u8              buffer[8];
    u16             crc16Result                         = 0x0000;
    u16             temp                                = 0x0000;
    static u8       askTime12Flag                       = 0;
    static u8       manual_state[DEVICE_TIME4_MAX]      ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static u8       time12_manual_state[TIME12_MAX][TIMER12_PORT_MAX]      ={0};
    static u8       state_pre[DEVICE_TIME4_MAX]         ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static time_t   protectTime[DEVICE_TIME4_MAX][TIMER_GROUP]     ={0};

    if(0 == askTime12Flag)
    {
        if(ask_device >= monitor->device_size)
        {
           //一个循环结束
            askTime12Flag = 1;
        }
    }
    else if(1 == askTime12Flag)
    {
        if(ask_time12 >= monitor->timer12_size)
        {
           //一个循环结束
            askTime12Flag = 0;
            ask_time12 = 0;
            ask_device = 0;
        }
    }

    //询问device
    if(0 == askTime12Flag)
    {
        if(0 < monitor->device_size)
        {
            buffer[0] = monitor->device[ask_device].addr;

            if(manual_state[ask_device] != monitor->device[ask_device]._manual[0].manual)
            {
                manual_state[ask_device] = monitor->device[ask_device]._manual[0].manual;

                //如果是手动开启的话 记录开启时的时间
                if(MANUAL_HAND_ON == manual_state[ask_device])
                {
                    monitor->device[ask_device]._manual[0].manual_on_time_save = getTimeStamp();
                }
            }

            if((TIMER_TYPE == monitor->device[ask_device].type) || (PUMP_TYPE == monitor->device[ask_device].type))
            {
                if(MANUAL_HAND_ON == monitor->device[ask_device]._manual[0].manual)
                {
                    monitor->device[ask_device]._storage[0]._time4_ctl.d_state = ON;

                    if(getTimeStamp() >=
                            (monitor->device[ask_device]._manual[0].manual_on_time_save +
                             monitor->device[ask_device]._manual[0].manual_on_time))
                    {
                        monitor->device[ask_device]._manual[0].manual = MANUAL_NO_HAND;
                        monitor->device[ask_device]._storage[0]._time4_ctl.d_state = OFF;
                        saveModuleFlag = YES;
                    }
                }
                else if(MANUAL_HAND_OFF == monitor->device[ask_device]._manual[0].manual)
                {
                    monitor->device[ask_device]._storage[0]._time4_ctl.d_state = OFF;
                }

                //如果是在检修状态则不输出
                if(ON == GetSysSet()->sysPara.maintain)
                {
                    monitor->device[ask_device]._storage[0]._time4_ctl.d_state = OFF;
                    LOG_D("in maintain, all device off");
                }
                buffer[1] = WRITE_SINGLE;
                buffer[2] = (monitor->device[ask_device].ctrl_addr >> 8) & 0x00FF;
                buffer[3] = monitor->device[ask_device].ctrl_addr & 0x00FF;
                buffer[4] = monitor->device[ask_device]._storage[0]._time4_ctl.d_state;
                buffer[5] = monitor->device[ask_device]._storage[0]._time4_ctl.d_value;
            }
            else
            {
                //如果是AC_4 或者是AC_12 的话就需要先询问是什么类型
                if((AC_4_TYPE == monitor->device[ask_device].type)&&
                    (YES != special[ask_device]))
                {
                    //如果还没有询问各个端口的类型就需要先询问
                    buffer[1] = READ_MUTI;
                    buffer[2] = (0x0440 >> 8) & 0x00FF;
                    buffer[3] = 0x0440 & 0x00FF;
                    buffer[4] = (monitor->device[ask_device].storage_size >> 8) & 0x00FF;
                    buffer[5] = monitor->device[ask_device].storage_size & 0x00FF;

                }
                else
                {
                    //如果是手动开启的话需要对比开启时间 时间到达后需要返回非手动状态
                    if(MANUAL_HAND_ON == monitor->device[ask_device]._manual[0].manual)
                    {
                        if(HVAC_6_TYPE == monitor->device[ask_device].type)
                        {
                            if(HVAC_CONVENTIONAL == monitor->device[ask_device]._hvac.hvacMode)
                            {
                                if(HVAC_COOL == monitor->device[ask_device]._hvac.manualOnMode)
                                {
                                    monitor->device[ask_device]._storage[0]._port.d_value = 0x08;
                                }
                                else if(HVAC_HEAT == monitor->device[ask_device]._hvac.manualOnMode)
                                {
                                    monitor->device[ask_device]._storage[0]._port.d_value = 0x10;
                                }

                            }
                            else if(HVAC_PUM_O == monitor->device[ask_device]._hvac.hvacMode)
                            {
                                if(HVAC_COOL == monitor->device[ask_device]._hvac.manualOnMode)
                                {
                                    monitor->device[ask_device]._storage[0]._port.d_value = 0x08;
                                }
                                else if(HVAC_HEAT == monitor->device[ask_device]._hvac.manualOnMode)
                                {
                                    monitor->device[ask_device]._storage[0]._port.d_value = 0x18;
                                }
                            }
                            else if(HVAC_PUM_B == monitor->device[ask_device]._hvac.hvacMode)
                            {
                                if(HVAC_COOL == monitor->device[ask_device]._hvac.manualOnMode)
                                {
                                    monitor->device[ask_device]._storage[0]._port.d_value = 0x18;
                                }
                                else if(HVAC_HEAT == monitor->device[ask_device]._hvac.manualOnMode)
                                {
                                    monitor->device[ask_device]._storage[0]._port.d_value = 0x08;
                                }
                            }
                        }
                        else//除了hvac_6
                        {
                            for(u8 item = 0; item < monitor->device[ask_device].storage_size; item++)
                            {
                                if((TIMER_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (VALVE_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (PUMP_TYPE == monitor->device[ask_device].device_timer_type[item]))
                                {
                                    monitor->device[ask_device]._storage[item]._time4_ctl.d_state = ON;
                                }
                                else
                                {
                                    monitor->device[ask_device]._storage[item]._port.d_state = ON;
                                }
                            }
                        }

                        //如果达到手动开启的时间之后将状态置为非手动
                        for(u8 item = 0; item < monitor->device[ask_device].storage_size; item++)
                        {
                            if(getTimeStamp() >=
                                    (monitor->device[ask_device]._manual[item].manual_on_time_save +
                                     monitor->device[ask_device]._manual[item].manual_on_time))
                            {
                                monitor->device[ask_device]._manual[item].manual = MANUAL_NO_HAND;

                                if(HVAC_6_TYPE == monitor->device[ask_device].type)
                                {
                                    if(HVAC_CONVENTIONAL == monitor->device[ask_device]._hvac.hvacMode)
                                    {
                                        if(HVAC_COOL == monitor->device[ask_device]._hvac.manualOnMode)
                                        {
                                            monitor->device[ask_device]._storage[0]._port.d_value &= 0xF7;
                                        }
                                        else if(HVAC_HEAT == monitor->device[ask_device]._hvac.manualOnMode)
                                        {
                                            monitor->device[ask_device]._storage[0]._port.d_value &= 0xEF;
                                        }

                                    }
                                    else if(HVAC_PUM_O == monitor->device[ask_device]._hvac.hvacMode)
                                    {
                                        if(HVAC_COOL == monitor->device[ask_device]._hvac.manualOnMode)
                                        {
                                            monitor->device[ask_device]._storage[0]._port.d_value &= 0xF7;
                                        }
                                        else if(HVAC_HEAT == monitor->device[ask_device]._hvac.manualOnMode)
                                        {
                                            monitor->device[ask_device]._storage[0]._port.d_value &= 0xE7;
                                        }
                                    }
                                    else if(HVAC_PUM_B == monitor->device[ask_device]._hvac.hvacMode)
                                    {
                                        if(HVAC_COOL == monitor->device[ask_device]._hvac.manualOnMode)
                                        {
                                            monitor->device[ask_device]._storage[0]._port.d_value &= 0xE7;
                                        }
                                        else if(HVAC_HEAT == monitor->device[ask_device]._hvac.manualOnMode)
                                        {
                                            monitor->device[ask_device]._storage[0]._port.d_value &= 0xF7;
                                        }
                                    }
                                }
                                else
                                {
                                    if((TIMER_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                       (VALVE_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                       (PUMP_TYPE == monitor->device[ask_device].device_timer_type[item]))
                                    {
                                        monitor->device[ask_device]._storage[item]._time4_ctl.d_state = OFF;
                                    }
                                    else
                                    {
                                        monitor->device[ask_device]._storage[item]._port.d_state = OFF;
                                    }
                                }

                                saveModuleFlag = YES;
                            }
                        }
                    }
                    else if(MANUAL_HAND_OFF == monitor->device[ask_device]._manual[0].manual)
                    {
                        for(u8 item = 0; item < monitor->device[ask_device].storage_size; item++)
                        {
                            if((TIMER_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                               (VALVE_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                               (PUMP_TYPE == monitor->device[ask_device].device_timer_type[item]))
                            {
                                monitor->device[ask_device]._storage[item]._time4_ctl.d_state = OFF;
                            }
                            else
                            {
                                monitor->device[ask_device]._storage[item]._port.d_state = OFF;
                            }
                        }
                    }
                    else if(MANUAL_NO_HAND == monitor->device[ask_device]._manual[0].manual)
                    {
                        //制冷 制热 除湿
                        if((COOL_TYPE == monitor->device[ask_device].type) || (HEAT_TYPE == monitor->device[ask_device].type) ||
                           (DEHUMI_TYPE == monitor->device[ask_device].type))
                        {
                            if(ON == monitor->device[ask_device]._storage[0]._port.d_state)
                            {
                                if(ON == monitor->device[ask_device].hotStartDelay)
                                {
                                    if(getTimeStamp() <= protectTime[ask_device][0] + 5 * 60)//保护时间为5分钟
                                    {
                                        monitor->device[ask_device]._storage[0]._port.d_state = OFF;//压缩机保护
                                        LOG_W("name %s is in hot start delay",monitor->device[ask_device].name);
                                    }
                                }
                            }
                        }
                        else if(TIMER_TYPE == monitor->device[ask_device].type)
                        {
                            for(u8 item = 0; item < monitor->device[ask_device].storage_size; item++)
                            {
                                if((COOL_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (HEAT_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (DEHUMI_TYPE == monitor->device[ask_device].device_timer_type[item]))
                                {
                                    if(getTimeStamp() <= protectTime[ask_device][item] + 5 * 60)//保护时间为5分钟
                                    {
                                        monitor->device[ask_device]._storage[item]._time4_ctl.d_state = OFF;//压缩机保护
                                        LOG_W("name %s is in hot start delay",monitor->device[ask_device].name);
                                    }
                                }
                            }
                        }
                    }

                    if(state_pre[ask_device] != monitor->device[ask_device]._storage[0]._port.d_state)
                    {
                        state_pre[ask_device] = monitor->device[ask_device]._storage[0]._port.d_state;

                        //制冷 制热 除湿
                        if((COOL_TYPE == monitor->device[ask_device].type) || (HEAT_TYPE == monitor->device[ask_device].type) ||
                           (DEHUMI_TYPE == monitor->device[ask_device].type))
                        {
                            if(OFF == state_pre[ask_device])
                            {
                                protectTime[ask_device][0] = getTimeStamp();
                            }
                        }
                        else if(TIMER_TYPE == monitor->device[ask_device].type)
                        {
                            for(u8 item = 0; item < monitor->device[ask_device].storage_size; item++)
                            {
                                if((COOL_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (HEAT_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (DEHUMI_TYPE == monitor->device[ask_device].device_timer_type[item]))
                                {
                                    protectTime[ask_device][item] = getTimeStamp();
                                }
                            }
                        }
                    }

                    //设置HVAC 风扇常开
                    if(HVAC_6_TYPE == monitor->device[ask_device].type)
                    {
                        if(ON == monitor->device[ask_device]._hvac.fanNormallyOpen)
                        {
                            monitor->device[ask_device]._storage[0]._port.d_value |= 0x04;
                        }
                    }

                    //维修中关闭输出
                    if(ON == GetSysSet()->sysPara.maintain)
                    {
                        monitor->device[ask_device]._storage[0]._port.d_state = OFF;
                        LOG_D("in maintain, all device off");
                    }

                    if(1 == monitor->device[ask_device].storage_size)
                    {
                        buffer[1] = WRITE_SINGLE;
                        buffer[2] = (monitor->device[ask_device].ctrl_addr >> 8) & 0x00FF;
                        buffer[3] = monitor->device[ask_device].ctrl_addr & 0x00FF;
                        buffer[4] = monitor->device[ask_device]._storage[0]._port.d_state;
                        buffer[5] = monitor->device[ask_device]._storage[0]._port.d_value;
                    }
                    else
                    {
                        buffer[1] = WRITE_SINGLE;
                        buffer[2] = (monitor->device[ask_device].ctrl_addr >> 8) & 0x00FF;
                        buffer[3] = monitor->device[ask_device].ctrl_addr & 0x00FF;
                        for(u8 item = 0; item < monitor->device[ask_device].storage_size; item++)
                        {
                            if(AC_4_TYPE == monitor->device[ask_device].type)
                            {
                                if((VALVE_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (PUMP_TYPE == monitor->device[ask_device].device_timer_type[item]) ||
                                   (TIMER_TYPE == monitor->device[ask_device].device_timer_type[item]))
                                {
                                    if(ON == monitor->device[ask_device]._storage[item]._time4_ctl.d_state)
                                    {
                                        temp |= 1 << item;
                                    }
                                }
                                else
                                {
                                    if(ON == monitor->device[ask_device]._storage[item]._port.d_state)
                                    {
                                        temp |= 1 << item;
                                    }
                                }
                            }
                            else
                            {
                                if(ON == monitor->device[ask_device]._storage[item]._port.d_state)
                                {
                                    temp |= 1 << item;
                                }
                            }
                        }

                        buffer[4] = temp >> 8;
                        buffer[5] = temp;
                    }
                }
            }

            crc16Result = usModbusRTU_CRC(buffer, 6);
            buffer[6] = crc16Result;                             //CRC16低位
            buffer[7] = (crc16Result>>8);                        //CRC16高位

            rt_device_write(serial, 0, buffer, 8);
            devConnectState[ask_device].send_count ++;
            if(devConnectState[ask_device].send_count >= CONNRCT_MISS_MAX)
            {
                ask_device ++;
            }
            devConnectState[ask_device].send_state = ON;
        }
    }
    //询问io_12
    else
    {
        if(0 < monitor->timer12_size)
        {
            buffer[0] = monitor->time12[ask_time12].addr;

            for(u8 port = 0; port < monitor->time12[ask_time12].storage_size; port++)
            {
                //记录手动开启的时间
                if(time12_manual_state[ask_time12][port] != monitor->time12[ask_time12]._manual[port].manual)
                {
                    time12_manual_state[ask_time12][port] = monitor->time12[ask_time12]._manual[port].manual;

                    //如果是手动开启的话 记录开启时的时间
                    if(MANUAL_HAND_ON == time12_manual_state[ask_time12][port])
                    {
                        monitor->time12[ask_time12]._manual[port].manual_on_time_save = getTimeStamp();
                    }
                }

                //如果是手动开启
                if(MANUAL_HAND_ON == monitor->time12[ask_time12]._manual[port].manual)
                {
                    monitor->time12[ask_time12]._time12_ctl[port].d_state = ON;

                    if(getTimeStamp() >=
                            (monitor->time12[ask_time12]._manual[port].manual_on_time_save +
                             monitor->time12[ask_time12]._manual[port].manual_on_time))
                    {
                        monitor->time12[ask_time12]._manual[port].manual = MANUAL_NO_HAND;
                        monitor->time12[ask_time12]._time12_ctl[port].d_state = OFF;
                        saveModuleFlag = YES;
                    }
                }
                else if(MANUAL_HAND_OFF == monitor->time12[ask_time12]._manual[port].manual)
                {
                    monitor->time12[ask_time12]._time12_ctl[port].d_state = OFF;
                }

                //如果是在检修状态则不输出
                if(ON == GetSysSet()->sysPara.maintain)
                {
                    monitor->time12[ask_time12]._time12_ctl[port].d_state = OFF;
                }
            }

            buffer[1] = WRITE_SINGLE;
            buffer[2] = (monitor->time12[ask_time12].ctrl_addr >> 8) & 0x00FF;
            buffer[3] = monitor->time12[ask_time12].ctrl_addr & 0x00FF;
            for(u8 item = 0; item < monitor->time12[ask_time12].storage_size; item++)
            {
                if(ON == monitor->time12[ask_time12]._time12_ctl[item].d_state)
                {
                    temp |= 1 << item;
                }
            }
            buffer[4] = temp >> 8;
            buffer[5] = temp;

            crc16Result = usModbusRTU_CRC(buffer, 6);
            buffer[6] = crc16Result;                             //CRC16低位
            buffer[7] = (crc16Result>>8);                        //CRC16高位

            rt_device_write(serial, 0, buffer, 8);
            timeConnectState[ask_time12].send_count ++;
            if(timeConnectState[ask_time12].send_count >= CONNRCT_MISS_MAX)
            {
                ask_time12 ++;
            }
            timeConnectState[ask_time12].send_state = ON;
        }
    }

    ret = YES;
    return ret;
}

void replyStrorageType(type_monitor_t *monitor, u8 addr, u8 *data, u8 dataLen)
{
    if(addr == monitor->device[ask_device].addr)
    {
        if(AC_4_TYPE == monitor->device[ask_device].type)
        {
            special[ask_device] = YES;      //标志已经收到端口数据
            if(dataLen/2 > TIMER_GROUP)
            {
                dataLen = TIMER_GROUP * 2;
            }

            for(u8 storage = 0; storage < dataLen/2; storage++)
            {
                monitor->device[ask_device].device_timer_type[storage] =
                        (data[2 * storage] << 8) | data[2 * storage + 1];
            }
        }
    }
}

void UpdateModuleConnect(type_monitor_t *monitor, u8 addr)
{
    if(addr == monitor->device[ask_device].addr)
    {
        devConnectState[ask_device].send_state = OFF;
        monitor->device[ask_device].conn_state = CON_SUCCESS;
        devConnectState[ask_device].send_count = 0;

        if(ask_device == monitor->device_size)
        {
           //一个循环结束
           ask_device = 0;
        }
        else
        {
            if(ask_device < monitor->device_size)
            {
               ask_device++;
            }
        }
    }
    else if(addr == monitor->sensor[ask_sensor].addr)
    {
        senConnectState[ask_sensor].send_state = OFF;
        monitor->sensor[ask_sensor].conn_state = CON_SUCCESS;
        senConnectState[ask_sensor].send_count = 0;

        if(ask_sensor == monitor->sensor_size)
        {
           //一个循环结束
           ask_sensor = 0;
        }
        else
        {
            if(ask_sensor < monitor->sensor_size)
            {
               ask_sensor++;
            }
        }
    }
    else if(addr == monitor->line[ask_line].addr)
    {
        lineConnectState[ask_line].send_state = OFF;
        monitor->line[ask_line].conn_state = CON_SUCCESS;
        lineConnectState[ask_line].send_count = 0;

        if(ask_line == monitor->line_size)
        {
            ask_line = 0;
        }
        else
        {
            if(ask_line < monitor->line_size)
            {
                ask_line++;
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

    if(DEVICE_TIME4_MAX >= monitor->device_size)
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
            AnlyzeDeviceRegister(monitor, serial ,data, dataLen);
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

        for(i = 0; i < monitor->timer12_size; i++)
        {
            if(addr == monitor->time12[i].addr)
            {
                buffer[0] = monitor->time12[i].addr;
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
