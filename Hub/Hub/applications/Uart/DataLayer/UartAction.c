/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-04-10     Administrator       the first version
 */
#include "UartEventType.h"
#include "UartDataLayer.h"
#include "DeviceUartClass.h"
#include "Module.h"
#include "Recipe.h"

u8 sys_warn[WARN_MAX];
#if(HUB_IRRIGSTION == HUB_SELECT)
#define     ADD_WATER       1
#define     NO_ADD_WATER    0
ph_cal_t ph_cal[SENSOR_MAX];
ec_cal_t ec_cal[SENSOR_MAX];
phcal_data_t phdataTemp[SENSOR_MAX];
eccal_data_t ecdataTemp[SENSOR_MAX];
#elif(HUB_ENVIRENMENT == HUB_SELECT)
#define LINE_OFF        0
#define LINE_UP         1
#define LINE_DOWN       2
#define LINE_STABLE     3//稳定
#define LINE_MIN_VALUE  30
#define LINE_DIMMING    40
#endif

static u8 GetDeviceStateByFunc(type_monitor_t *monitor, u8 func)
{
    int     i       = 0;
    int     port    = 0;

    //1 如果有开启则返回
    for(i = 0; i < monitor->device_size; i++)
    {
        for(port = 0; port < monitor->device[i].storage_size; port++)
        {
            if(func == monitor->device[i].port[port].func)
            {
                if(ON == monitor->device[i].port[port].ctrl.d_state)
                {
                    break;
                }
            }
        }
    }
    //2 检测是否有端口开启
    if(i == monitor->device_size)
    {
        return ON;
    }
    else
    {
        return OFF;
    }
}

void warnProgram(type_monitor_t *monitor, sys_set_t *set)
{

#if(HUB_SELECT == HUB_ENVIRENMENT)
    u8              co2State    = OFF;
    u8              tempState   = OFF;
    u8              humiState   = OFF;
    int             data        = 0;
    static u8       co2State_pre    = OFF;
    static u8       tempState_pre   = OFF;
    static u8       humiState_pre   = OFF;
    static time_t   co2WarnTime;
    static time_t   tempWarnTime;
    static time_t   humiWarnTime;
    static u8       humiStateLow_pre    = OFF;
    static u8       humiStateHigh_pre   = OFF;
    static u8       tempStateLow_pre    = OFF;
    static u8       tempStateHigh_pre   = OFF;
    static u8       co2StateLow_pre     = OFF;
    static u8       co2StateHigh_pre    = OFF;
    static u8       vpdStateLow_pre     = OFF;
    static u8       vpdStateHigh_pre    = OFF;
#elif(HUB_SELECT == HUB_IRRIGSTION)
    sensor_t        *sensor;
    u8              tankState[TANK_LIST_MAX]    = {OFF,OFF,OFF,OFF};
    u16             ec          = 0;
    u16             ph          = 0;
    u16             wt          = 0;
    u16             wl          = 0;
    u16             sw          = 0;    //基质湿度
    u16             sec         = 0;    //基质EC
    u16             st          = 0;    //基质温度
    static time_t   tankAutoValve[TANK_LIST_MAX];
    static u8       tankState_pre[TANK_LIST_MAX]    = {OFF,OFF,OFF,OFF};
#endif
    static int      smog_Pre    = 0;
    static int      leakage_Pre = 0;


#if(HUB_SELECT == HUB_ENVIRENMENT)
    //白天
    if(DAY_TIME == set->dayOrNight)
    {
        //1.温度
        data = getSensorDataByFunc(monitor, F_S_TEMP);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.dayTempEn)
            {
                if(data <= set->sysWarn.dayTempMin)
                {
                    set->warn[WARN_TEMP_LOW - 1] = ON;
                    set->warn_value[WARN_TEMP_LOW - 1] = data;
                    tempStateLow_pre = ON;
                }
                else if(data >= set->sysWarn.dayTempMax)
                {
                    set->warn[WARN_TEMP_HIGHT - 1] = ON;
                    set->warn_value[WARN_TEMP_HIGHT - 1] = data;
                    tempStateHigh_pre = ON;
                }
                else
                {
                    if(ON == tempStateLow_pre)
                    {
                        if(data > set->sysWarn.dayTempMin + 20)
                        {
                            set->warn[WARN_TEMP_LOW - 1] = OFF;
                            tempStateLow_pre = OFF;
                        }
                    }

                    if(ON == tempStateHigh_pre)
                    {
                        if(data + 20 < set->sysWarn.dayTempMax)
                        {
                            set->warn[WARN_TEMP_HIGHT - 1] = OFF;
                            tempStateHigh_pre = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_TEMP_LOW - 1] = OFF;
                set->warn[WARN_TEMP_HIGHT - 1] = OFF;
            }


            if(data > set->tempSet.dayCoolingTarget ||
               data < set->tempSet.dayHeatingTarget)
            {
                tempState = ON;
            }
            else
            {
                tempState = OFF;
            }

            if(tempState_pre != tempState)
            {
                tempState_pre = tempState;

                if(ON == tempState_pre)
                {
                    tempWarnTime = getTimeStamp();
                }
            }

            if(ON == tempState)
            {
                if(getTimeStamp() > tempWarnTime + set->sysWarn.tempTimeoutseconds)
                {
                    if(ON == set->sysWarn.tempTimeoutEn)
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_TEMP_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
            }
        }

        //2.湿度
        data = getSensorDataByFunc(monitor, F_S_HUMI);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.dayhumidEn)
            {
                if(data <= set->sysWarn.dayhumidMin)
                {
                    set->warn[WARN_HUMI_LOW - 1] = ON;
                    set->warn_value[WARN_HUMI_LOW - 1] = data;
                    humiStateLow_pre = ON;
                }
                else if(data >= set->sysWarn.dayhumidMax)
                {
                    set->warn[WARN_HUMI_HIGHT - 1] = ON;
                    set->warn_value[WARN_HUMI_HIGHT - 1] = data;
                    humiStateHigh_pre = ON;
                }
                else
                {
                    //如果出现报警，需要解除报警的条件需要做振荡过滤
                    if(ON == humiStateLow_pre)
                    {
                        if(data > set->sysWarn.dayhumidMin + 20)
                        {
                            set->warn[WARN_HUMI_LOW - 1] = OFF;
                            humiStateLow_pre = OFF;
                        }
                    }

                    if(ON == humiStateHigh_pre)
                    {
                        if(data + 20 < set->sysWarn.dayhumidMax)
                        {
                            set->warn[WARN_HUMI_HIGHT - 1] = OFF;
                            humiStateHigh_pre = ON;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_HUMI_LOW - 1] = OFF;
                set->warn[WARN_HUMI_HIGHT - 1] = OFF;
            }

            if(data > set->humiSet.dayDehumiTarget ||
               data < set->humiSet.dayHumiTarget)
            {
                humiState = ON;
            }
            else
            {
                humiState = OFF;
            }

            if(humiState_pre != humiState)
            {
                humiState_pre = humiState;

                if(ON == humiState_pre)
                {
                    humiWarnTime = getTimeStamp();
                }
            }

            if(ON == humiState)
            {
                if(getTimeStamp() > humiWarnTime + set->sysWarn.humidTimeoutseconds)
                {
                    if(WARN_HUMI_TIMEOUT <= WARN_MAX)
                    {
                        if(ON == set->sysWarn.humidTimeoutEn)
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = ON;
                            set->warn_value[WARN_HUMI_TIMEOUT - 1] = data;
                        }
                        else
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
            }
        }

        //3.CO2
        data = getSensorDataByFunc(monitor, F_S_CO2);
        if(VALUE_NULL!= data)
        {
            if(ON == set->sysWarn.dayCo2En)
            {
                if(data <= set->sysWarn.dayCo2Min)
                {
                    set->warn[WARN_CO2_LOW - 1] = ON;
                    set->warn_value[WARN_CO2_LOW - 1] = data;
                    co2StateLow_pre = ON;
                }
                else if(data >= set->sysWarn.dayCo2Max)
                {
                    set->warn[WARN_CO2_HIGHT - 1] = ON;
                    set->warn_value[WARN_CO2_HIGHT - 1] = data;
                    co2StateHigh_pre = ON;
                }
                else
                {
                    if(ON == co2StateLow_pre)
                    {
                        if(data > set->sysWarn.dayCo2Min + 50)
                        {
                            set->warn[WARN_CO2_LOW - 1] = OFF;
                            co2StateLow_pre = OFF;
                        }
                    }

                    if(ON == co2StateHigh_pre)
                    {
                        if(data + 50 < set->sysWarn.dayCo2Max)
                        {
                            set->warn[WARN_CO2_HIGHT - 1] = OFF;
                            co2StateHigh_pre = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_CO2_LOW - 1] = OFF;
                set->warn[WARN_CO2_HIGHT - 1] = OFF;
            }

            if(data < set->co2Set.dayCo2Target)
            {
                co2State = ON;
            }
            else
            {
                co2State = OFF;
            }

            if(co2State_pre != co2State)
            {
                co2State_pre = co2State;

                if(ON == co2State_pre)
                {
                    co2WarnTime = getTimeStamp();
                }
            }

            if(ON == co2State)
            {
                if(getTimeStamp() > co2WarnTime + set->sysWarn.co2Timeoutseconds)
                {
                    if(ON == set->sysWarn.co2TimeoutEn)
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_CO2_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
            }
        }

        //4.vpd
        if(ON == set->sysWarn.dayVpdEn)
        {
            if(0 != getVpd())
            {
                if(getVpd() <= set->sysWarn.dayVpdMin)
                {
                    set->warn[WARN_VPD_LOW - 1] = ON;
                    set->warn_value[WARN_VPD_LOW - 1] = getVpd();
                    vpdStateLow_pre = ON;
                }
                else if(getVpd() >= set->sysWarn.dayVpdMax)
                {
                    set->warn[WARN_VPD_HIGHT - 1] = ON;
                    set->warn_value[WARN_VPD_HIGHT - 1] = getVpd();
                    vpdStateHigh_pre = ON;
                }
                else
                {
                    if(ON == vpdStateLow_pre)
                    {
                        if(getVpd() > set->sysWarn.dayVpdMin + 10)
                        {
                            set->warn[WARN_VPD_LOW - 1] = OFF;
                            vpdStateLow_pre = OFF;
                        }
                    }

                    if(ON == vpdStateHigh_pre)
                    {
                        if(getVpd() + 10 < set->sysWarn.dayVpdMax)
                        {
                            set->warn[WARN_VPD_HIGHT - 1] = OFF;
                            vpdStateHigh_pre = OFF;
                        }
                    }
                }
            }
        }
        else
        {
            set->warn[WARN_VPD_LOW - 1] = OFF;
            set->warn[WARN_VPD_HIGHT - 1] = OFF;
        }

        //5.par
        data = getSensorDataByFunc(monitor, F_S_PAR);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.dayParEn)
            {
                if(data <= set->sysWarn.dayParMin)
                {
                    set->warn[WARN_PAR_LOW - 1] = ON;
                    set->warn_value[WARN_PAR_LOW - 1] = data;
                }
                else if(data >= set->sysWarn.dayParMax)
                {
                    set->warn[WARN_PAR_HIGHT - 1] = ON;
                    set->warn_value[WARN_PAR_HIGHT - 1] = data;
                }
                else
                {
                    set->warn[WARN_PAR_LOW - 1] = OFF;
                    set->warn[WARN_PAR_HIGHT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_PAR_LOW - 1] = OFF;
                set->warn[WARN_PAR_HIGHT - 1] = OFF;
            }
        }
    }
    else if(NIGHT_TIME == set->dayOrNight)
    {
        //1.temp
        data = getSensorDataByFunc(monitor, F_S_TEMP);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.nightTempEn)
            {
                if(data <= set->sysWarn.nightTempMin)
                {
                    set->warn[WARN_TEMP_LOW - 1] = ON;
                    set->warn_value[WARN_TEMP_LOW - 1] = data;
                    tempStateLow_pre = ON;
                }
                else if(data >= set->sysWarn.nightTempMax)
                {
                    set->warn[WARN_TEMP_HIGHT - 1] = ON;
                    set->warn_value[WARN_TEMP_HIGHT - 1] = data;
                    tempStateHigh_pre = ON;
                }
                else
                {
                    if(ON == tempStateLow_pre)
                    {
                        if(data > set->sysWarn.nightTempMin + 20)
                        {
                            set->warn[WARN_TEMP_LOW - 1] = OFF;
                            tempStateLow_pre = OFF;
                        }
                    }

                    if(ON == tempStateHigh_pre)
                    {
                        if(data + 20 < set->sysWarn.nightTempMax)
                        {
                            set->warn[WARN_TEMP_HIGHT - 1] = OFF;
                            tempStateHigh_pre = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_TEMP_LOW - 1] = OFF;
                set->warn[WARN_TEMP_HIGHT - 1] = OFF;
            }

            if(data > set->tempSet.nightCoolingTarget ||
               data < set->tempSet.nightHeatingTarget)
            {
                tempState = ON;
            }
            else
            {
                tempState = OFF;
            }

            if(tempState_pre != tempState)
            {
                tempState_pre = tempState;

                if(ON == tempState_pre)
                {
                    tempWarnTime = getTimeStamp();
                }
            }

            if(ON == tempState)
            {
                if(getTimeStamp() > tempWarnTime + set->sysWarn.tempTimeoutseconds)
                {
                    if(ON == set->sysWarn.tempTimeoutEn)
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_TEMP_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_TEMP_TIMEOUT - 1] = OFF;
            }
        }

        //2.湿度
        data = getSensorDataByFunc(monitor, F_S_HUMI);
        if(VALUE_NULL != data)
        {
            if(ON == set->sysWarn.nighthumidEn)
            {
                if(data <= set->sysWarn.nighthumidMin)
                {
                    set->warn[WARN_HUMI_LOW - 1] = ON;
                    set->warn_value[WARN_HUMI_LOW - 1] = data;
                    humiStateLow_pre = ON;
                }
                else if(data >= set->sysWarn.nighthumidMax)
                {
                    set->warn[WARN_HUMI_HIGHT - 1] = ON;
                    set->warn_value[WARN_HUMI_HIGHT - 1] = data;
                    humiStateHigh_pre = ON;
                }
                else
                {
                    if(ON == humiStateLow_pre)
                    {
                        if(data > set->sysWarn.nighthumidMin + 20)
                        {
                            set->warn[WARN_HUMI_LOW - 1] = OFF;
                            humiStateLow_pre = OFF;
                        }
                    }

                    if(ON == humiStateHigh_pre)
                    {
                        if(data + 20 < set->sysWarn.nighthumidMax)
                        {
                            set->warn[WARN_HUMI_HIGHT - 1] = OFF;
                            humiStateHigh_pre = OFF;
                        }
                    }
                }
            }

            if(data > set->humiSet.nightDehumiTarget ||
               data < set->humiSet.nightHumiTarget)
            {
                humiState = ON;
            }
            else
            {
                humiState = OFF;
            }

            if(humiState_pre != humiState)
            {
                humiState_pre = humiState;

                if(ON == humiState_pre)
                {
                    humiWarnTime = getTimeStamp();
                }
            }

            if(ON == humiState)
            {
                if(getTimeStamp() > humiWarnTime + set->sysWarn.humidTimeoutseconds)
                {
                    if(WARN_HUMI_TIMEOUT <= WARN_MAX)
                    {
                        if(ON == set->sysWarn.humidTimeoutEn)
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = ON;
                            set->warn_value[WARN_HUMI_TIMEOUT - 1] = data;
                        }
                        else
                        {
                            set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_HUMI_TIMEOUT - 1] = OFF;
            }
        }

        //3.CO2
        data = getSensorDataByFunc(monitor, F_S_CO2);
        if(VALUE_NULL!= data)
        {
            if(ON == set->sysWarn.nightCo2En)
            {
                if(data <= set->sysWarn.nightCo2Min)
                {
                    set->warn[WARN_CO2_LOW - 1] = ON;
                    set->warn_value[WARN_CO2_LOW - 1] = data;
                    co2StateLow_pre = ON;
                }
                else if(data >= set->sysWarn.nightCo2Max)
                {
                    set->warn[WARN_CO2_HIGHT - 1] = ON;
                    set->warn_value[WARN_CO2_HIGHT - 1] = data;
                    co2StateHigh_pre = ON;
                }
                else
                {
                    if(ON == co2StateLow_pre)
                    {
                        if(data > set->sysWarn.nightCo2Min + 50)
                        {
                            set->warn[WARN_CO2_LOW - 1] = OFF;
                            co2StateLow_pre = OFF;
                        }
                    }

                    if(ON == co2StateHigh_pre)
                    {
                        if(data + 50 < set->sysWarn.nightCo2Max)
                        {
                            set->warn[WARN_CO2_HIGHT - 1] = OFF;
                            co2StateHigh_pre = OFF;
                        }
                    }
                }
            }
            else
            {
                set->warn[WARN_CO2_LOW - 1] = OFF;
                set->warn[WARN_CO2_HIGHT - 1] = OFF;
            }

            if(data > set->co2Set.nightCo2Target)
            {
                co2State = ON;
            }
            else
            {
                co2State = OFF;
            }

            if(co2State_pre != co2State)
            {
                co2State_pre = co2State;

                if(ON == co2State_pre)
                {
                    co2WarnTime = getTimeStamp();
                }
            }

            if(ON == co2State)
            {
                if(getTimeStamp() > co2WarnTime + set->sysWarn.co2Timeoutseconds)
                {
                    if(ON == set->sysWarn.co2TimeoutEn)
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = ON;
                        set->warn_value[WARN_CO2_TIMEOUT - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
                    }
                }
            }
            else
            {
                set->warn[WARN_CO2_TIMEOUT - 1] = OFF;
            }
        }

        //4.vpd
        if(ON == set->sysWarn.nightVpdEn)
        {
            if(0 != getVpd())
            {
                if(getVpd() <= set->sysWarn.nightVpdMin)
                {
                    set->warn[WARN_VPD_LOW - 1] = ON;
                    set->warn_value[WARN_VPD_LOW - 1] = getVpd();
                    vpdStateLow_pre = ON;
                }
                else if(getVpd() >= set->sysWarn.nightVpdMax)
                {
                    set->warn[WARN_VPD_HIGHT - 1] = ON;
                    set->warn_value[WARN_VPD_HIGHT - 1] = getVpd();
                    vpdStateHigh_pre = ON;
                }
                else
                {
                    if(ON == vpdStateLow_pre)
                    {
                        if(getVpd() > set->sysWarn.nightVpdMin + 10)
                        {
                            set->warn[WARN_VPD_LOW - 1] = OFF;
                            vpdStateLow_pre = OFF;
                        }
                    }

                    if(ON == vpdStateHigh_pre)
                    {
                        if(getVpd() + 10 < set->sysWarn.nightVpdMax)
                        {
                            set->warn[WARN_VPD_HIGHT - 1] = OFF;
                            vpdStateHigh_pre = OFF;
                        }
                    }
                }
            }
        }
        else
        {
            set->warn[WARN_VPD_LOW - 1] = OFF;
            set->warn[WARN_VPD_HIGHT - 1] = OFF;
        }
    }

    //灯光警告 比如开着的时候 检测到灯光的值是黑的
    if(ON == set->sysWarn.lightEn)
    {
        if((LINE_MODE_BY_POWER == set->line1Set.brightMode) ||
           (LINE_MODE_AUTO_DIMMING == set->line1Set.brightMode) ||
           (LINE_MODE_BY_POWER == set->line2Set.brightMode) ||
           (LINE_MODE_AUTO_DIMMING == set->line2Set.brightMode))
        {
            if((ON == monitor->line[0].port[0].ctrl.d_state) ||
               (ON == monitor->line[0].port[1].ctrl.d_state) ||
               (ON == monitor->line[0].port[2].ctrl.d_state) ||
               (ON == monitor->line[0].port[3].ctrl.d_state) ||
               (ON == monitor->line[1].port[0].ctrl.d_state))//灯开关为开
            {
                data = getSensorDataByFunc(monitor, F_S_PAR);
                if(VALUE_NULL != data)
                {
                    if(data < 30)//检测到灯光没开
                    {
                        set->warn[WARN_LINE_STATE - 1] = ON;
                        set->warn_value[WARN_LINE_STATE - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_LINE_STATE - 1] = OFF;
                    }
                }
            }
            else if((OFF == monitor->line[0].port[0].ctrl.d_state) &&
                    (OFF == monitor->line[0].port[1].ctrl.d_state) &&
                    (OFF == monitor->line[0].port[2].ctrl.d_state) &&
                    (OFF == monitor->line[0].port[3].ctrl.d_state) &&
                    (OFF == monitor->line[1].port[0].ctrl.d_state))
            {
                data = getSensorDataByFunc(monitor, F_S_PAR);
                if(VALUE_NULL != data)
                {
                    if(data > 30)//检测到灯光没开
                    {
                        set->warn[WARN_LINE_STATE - 1] = ON;
                        set->warn_value[WARN_LINE_STATE - 1] = data;
                    }
                    else
                    {
                        set->warn[WARN_LINE_STATE - 1] = OFF;
                    }
                }
            }
        }
        else
        {
            data = getSensorDataByFunc(monitor, F_S_PAR);
            if(VALUE_NULL != data)
            {
                if(data > 30)//检测到灯光没开
                {
                    set->warn[WARN_LINE_STATE - 1] = ON;
                    set->warn_value[WARN_LINE_STATE - 1] = data;
                }
                else
                {
                    set->warn[WARN_LINE_STATE - 1] = OFF;
                }
            }
        }

        //灯光过温保护 过温关灯 告警
        data = getSensorDataByFunc(monitor, F_S_TEMP);
        if(VALUE_NULL != data)
        {
            if(data > set->line1Set.tempStartDimming ||
               data > set->line2Set.tempStartDimming)
            {
                set->warn[WARN_LINE_AUTO_T - 1] = ON;
                set->warn_value[WARN_LINE_AUTO_T - 1] = data;
            }
            else if(data > set->line1Set.tempOffDimming ||
                    data > set->line2Set.tempOffDimming)
            {
                set->warn[WARN_LINE_AUTO_OFF - 1] = ON;
                set->warn_value[WARN_LINE_AUTO_OFF - 1] = data;
            }
            else
            {
                set->warn[WARN_LINE_AUTO_T - 1] = OFF;
                set->warn[WARN_LINE_AUTO_OFF - 1] = OFF;
            }
        }
    }

    //O2低氧
    data = getSensorDataByFunc(monitor, F_S_O2);
    if(VALUE_NULL != data)
    {
        //氧气低于18%要报警
        if(ON == set->sysWarn.o2ProtectionEn)
        {
            if(data < 180)
            {
                set->warn[WARN_O2_LOW - 1] = ON;
                set->warn_value[WARN_O2_LOW - 1] = data;
            }
        }
        else
        {
            if(data > (180 + 20))
            {
                set->warn[WARN_O2_LOW - 1] = OFF;
                set->warn_value[WARN_O2_LOW - 1] = 0;
            }
        }
    }
#elif(HUB_SELECT == HUB_IRRIGSTION)

    //ph ec
    for(u8 tank = 0; tank < GetSysTank()->tank_size; tank++)
    {
//        for(u8 item1 = 0; item1 < 2;item1++)
        {
            for(u8 item2 = 0; item2 < TANK_SENSOR_MAX; item2++)
            {
                if(GetSysTank()->tank[tank].sensorId[0][item2] != 0)
                {
                    sensor = GetSensorByAddr(monitor, GetSysTank()->tank[tank].sensorId[0][item2]);

                    for(u8 sto = 0; sto < sensor->storage_size; sto++)
                    {
                        if(F_S_PH == sensor->__stora[sto].func)
                        {
                            ph = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_EC == sensor->__stora[sto].func)
                        {
                            ec = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_WT == sensor->__stora[sto].func)
                        {
                            wt = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_WL == sensor->__stora[sto].func)
                        {
                            wl = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_SW == sensor->__stora[sto].func)
                        {
                            sw = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_SEC == sensor->__stora[sto].func)
                        {
                            sec = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                        else if(F_S_ST == sensor->__stora[sto].func)
                        {
                            st = getSensorDataByAddr(monitor, sensor->addr, sto);
                        }
                    }
                }
            }
        }

        //
        for(u8 item1 = 0; item1 < TANK_WARN_ITEM_MAX; item1++)
        {
            if(F_S_PH == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.phEn)
                {
                    if(ph < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_PH_LOW - 1] = ON;
                        set->warn_value[WARN_PH_LOW - 1] = ph;
                    }
                    else if(ph > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_PH_HIGHT - 1] = ON;
                        set->warn_value[WARN_PH_HIGHT - 1] = ph;
                    }
                    else
                    {
                        set->warn[WARN_PH_LOW - 1] = OFF;
                        set->warn[WARN_PH_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_PH_LOW - 1] = OFF;
                    set->warn[WARN_PH_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_EC == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.ecEn)
                {
                    if(ec < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_EC_LOW - 1] = ON;
                        set->warn_value[WARN_EC_LOW - 1] = ec;
                    }
                    else if(ec > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_EC_HIGHT - 1] = ON;
                        set->warn_value[WARN_EC_HIGHT - 1] = ec;
                    }
                    else
                    {
                        set->warn[WARN_EC_LOW - 1] = OFF;
                        set->warn[WARN_EC_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_EC_LOW - 1] = OFF;
                    set->warn[WARN_EC_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_WT == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.wtEn)
                {
                    if(wt < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_WT_LOW - 1] = ON;
                        set->warn_value[WARN_WT_LOW - 1] = wt;
                    }
                    else if(wt > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_WT_HIGHT - 1] = ON;
                        set->warn_value[WARN_WT_HIGHT - 1] = wt;
                    }
                    else
                    {
                        set->warn[WARN_WT_LOW - 1] = OFF;
                        set->warn[WARN_WT_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_WT_LOW - 1] = OFF;
                    set->warn[WARN_WT_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_WL == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.wlEn)
                {
                    if(wl < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_WL_LOW - 1] = ON;
                        set->warn_value[WARN_WL_LOW - 1] = wl;
                    }
                    else if(wl > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_WL_HIGHT - 1] = ON;
                        set->warn_value[WARN_WL_HIGHT - 1] = wl;
                    }
                    else
                    {
                        set->warn[WARN_WL_LOW - 1] = OFF;
                        set->warn[WARN_WL_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_WL_LOW - 1] = OFF;
                    set->warn[WARN_WL_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_SW == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.mmEn)
                {
                    if(sw < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_SOIL_W_LOW - 1] = ON;
                        set->warn_value[WARN_SOIL_W_LOW - 1] = sw;
                    }
                    else if(sw > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_SOIL_W_HIGHT - 1] = ON;
                        set->warn_value[WARN_SOIL_W_HIGHT - 1] = sw;
                    }
                    else
                    {
                        set->warn[WARN_SOIL_W_LOW - 1] = OFF;
                        set->warn[WARN_SOIL_W_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_SOIL_W_LOW - 1] = OFF;
                    set->warn[WARN_SOIL_W_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_SEC == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.meEn)
                {
                    if(sec < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_SOIL_EC_LOW - 1] = ON;
                        set->warn_value[WARN_SOIL_EC_LOW - 1] = sec;
                    }
                    else if(sec > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_SOIL_EC_HIGHT - 1] = ON;
                        set->warn_value[WARN_SOIL_EC_HIGHT - 1] = sec;
                    }
                    else
                    {
                        set->warn[WARN_SOIL_EC_LOW - 1] = OFF;
                        set->warn[WARN_SOIL_EC_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_SOIL_EC_LOW - 1] = OFF;
                    set->warn[WARN_SOIL_EC_HIGHT - 1] = OFF;
                }
            }
            else if(F_S_ST == set->tankWarnSet[tank][item1].func)
            {
                if(ON == set->sysWarn.mtEn)
                {
                    if(st < set->tankWarnSet[tank][item1].min)
                    {
                        set->warn[WARN_SOIL_T_LOW - 1] = ON;
                        set->warn_value[WARN_SOIL_T_LOW - 1] = st;
                    }
                    else if(st > set->tankWarnSet[tank][item1].max)
                    {
                        set->warn[WARN_SOIL_T_HIGHT - 1] = ON;
                        set->warn_value[WARN_SOIL_T_HIGHT - 1] = st;
                    }
                    else
                    {
                        set->warn[WARN_SOIL_T_LOW - 1] = OFF;
                        set->warn[WARN_SOIL_T_HIGHT - 1] = OFF;
                    }
                }
                else
                {
                    set->warn[WARN_SOIL_T_LOW - 1] = OFF;
                    set->warn[WARN_SOIL_T_HIGHT - 1] = OFF;
                }
            }
        }

        //自动补水超时报警
        if(VALUE_NULL == wl)
        {
            tankState[tank] = OFF;
        }
        else
        {
            if(wl < GetSysTank()->tank[tank].autoFillHeight)
            {
                tankState[tank] = ON;
            }
            else
            {
                tankState[tank] = OFF;
            }
        }

        if(tankState_pre[tank] != tankState[tank])
        {
            tankState_pre[tank] = tankState[tank];
            if(tankState[tank] == ON)
            {
                tankAutoValve[tank] = getTimeStamp();
            }
        }

        if(ON == tankState[tank])
        {
            if(ON == set->sysWarn.autoFillTimeout)
            {
                if(getTimeStamp() > GetSysTank()->tank[tank].poolTimeout + tankAutoValve[tank])
                {
                    set->warn[WARN_AUTOFILL_TIMEOUT - 1] = ON;
                    set->warn_value[WARN_AUTOFILL_TIMEOUT - 1] = wl;
                    break;
                }
                else
                {
                    set->warn[WARN_AUTOFILL_TIMEOUT - 1] = OFF;
                }
            }
            else
            {
                set->warn[WARN_AUTOFILL_TIMEOUT - 1] = OFF;
            }
        }
        else
        {
            set->warn[WARN_AUTOFILL_TIMEOUT - 1] = OFF;
        }
    }
#endif

    //烟感
    if(ON == set->sysWarn.smokeEn)
    {
        int value = getSensorDataByFunc(monitor, F_S_SM);
        if(VALUE_NULL != value)
        {
            if(smog_Pre != value)
            {
                smog_Pre = value;

                if(0x01 == value)
                {
                    set->warn[WARN_SMOKE - 1] = ON;
                }
                else
                {
                    set->warn[WARN_SMOKE - 1] = OFF;
                }
            }
        }
    }
    else
    {
        set->warn[WARN_SMOKE - 1] = OFF;
    }

    //漏水
    if(ON == set->sysWarn.waterEn)
    {
        int value = getSensorDataByFunc(monitor, F_S_LK);
        if(VALUE_NULL != value)
        {
            if(leakage_Pre != value)
            {
                leakage_Pre = value;

                if(0x01 == value)
                {
                    set->warn[WARN_WATER - 1] = ON;
                }
                else
                {
                    set->warn[WARN_WATER - 1] = OFF;
                }
            }
        }
    }
    else
    {
        set->warn[WARN_WATER - 1] = OFF;
    }

    rt_memcpy(sys_warn, set->warn, WARN_MAX);
}

//执行设备手动控制功能
void menualHandProgram(type_monitor_t *monitor, type_uart_class *deviceUart, type_uart_class *lineUart)
{
    int         i           = 0;
    int         port        = 0;
    device_t    *device     = RT_NULL;
    line_t      *line       = RT_NULL;
    time_t      nowTime     = getTimeStamp();

    for(i = 0; i < monitor->device_size; i++)
    {
        device = &monitor->device[i];
        for(port = 0; port < device->storage_size; port++)
        {
            if(MANUAL_HAND_OFF == device->port[port].manual.manual)
            {
                 deviceUart->DeviceCtrlSingle(device, port, OFF);
            }
            else if(MANUAL_HAND_ON == device->port[port].manual.manual)
            {
                //如果是在开启的时间内就需要打开
                if((nowTime >= device->port[port].manual.manual_on_time_save) &&
                   (nowTime <= (device->port[port].manual.manual_on_time + device->port[port].manual.manual_on_time_save)))
                {
                    deviceUart->DeviceCtrlSingle(device, port, ON);
                }
                else
                {
                    device->port[port].manual.manual = MANUAL_NO_HAND;
                    deviceUart->DeviceCtrlSingle(device, port, OFF);
                }
            }
            else
            {
                //如果上一个时刻为手动开启 突然关闭手动要将状态还回去
                if((nowTime >= device->port[port].manual.manual_on_time_save) &&
                   (nowTime <= (device->port[port].manual.manual_on_time + device->port[port].manual.manual_on_time_save)))
                {
                    deviceUart->DeviceCtrlSingle(device, port, OFF);
                }
            }


        }
    }

#if(HUB_SELECT == HUB_ENVIRENMENT)
    for(i = 0; i < monitor->line_size; i++)
    {
        line = &monitor->line[i];
        if(MANUAL_HAND_OFF == line->port[0]._manual.manual)
        {
            if(LINE_TYPE == line->type)
            {
                lineUart->LineCtrl(line, 0, 0, 0);
            }
            else if(LINE_4_TYPE == line->type)
            {
                u16 ctrlValue[LINE_PORT_MAX] = {0,0,0,0};
                lineUart->Line4Ctrl(line, ctrlValue);
            }
        }
        else if(MANUAL_HAND_ON == line->port[0]._manual.manual)
        {
            if((nowTime >= line->port[0]._manual.manual_on_time_save) &&
               (nowTime <= (line->port[0]._manual.manual_on_time + line->port[0]._manual.manual_on_time_save)))
            {
                if(LINE_TYPE == line->type)
                {
                    lineUart->LineCtrl(line, 0, ON, 100);
                }
                else if(LINE_4_TYPE == line->type)
                {
                    u8 res = 0;
                    u16 ctrlValue[LINE_PORT_MAX] = {0,0,0,0};
                    for(int port = 0; port < LINE_PORT_MAX; port++)
                    {
                        GetRealLine4V(&GetSysSet()->dimmingCurve, port, 100, &res);
                        ctrlValue[port] = (100 << 8) | res;
                    }
                    lineUart->Line4Ctrl(line, ctrlValue);
                }
            }
            else
            {
                line->port[0]._manual.manual = MANUAL_NO_HAND;
                u16 ctrlValue[LINE_PORT_MAX] = {0,0,0,0};
                lineUart->Line4Ctrl(line, ctrlValue);
            }
        }
    }
#endif
}

#if(HUB_ENVIRENMENT == HUB_SELECT)
//获取当前的参数设置
void GetNowSysSet(proTempSet_t *tempSet, proCo2Set_t *co2Set, proHumiSet_t *humiSet,
        proLine_t *line1Set, proLine_4_t *line_4Set, proLine_t *line2Set, struct recipeInfor *info)
{
    u8              item = 0;
    u8              index = 0;
    time_t          starts;
    sys_set_t       *set = GetSysSet();
    sys_recipe_t    *recipe = GetSysRecipt();
    u8              usedCalFlg = OFF; // 如果为OFF 则使用系统设置 否则
    type_sys_time   time;
    char firstStartAt[15] = "";

    changeCharToDate(set->stageSet.starts, &time);
    starts = changeDataToTimestamp(time.year, time.month, time.day, time.hour, time.minute, time.second);

    //如果不使能日历 或者 不处于日历的
    if(OFF == set->stageSet.en)
    {
        usedCalFlg = OFF;
    }
    else if(ON == set->stageSet.en)
    {
        for(index = 0; index < STAGE_LIST_MAX; index++)
        {
            if((0 != set->stageSet._list[index].recipeId) && (0 != set->stageSet._list[index].duration_day))
            {

                if((getTimeStamp() >= starts) && (getTimeStamp() <= starts + set->stageSet._list[index].duration_day * 24 * 60 * 60))
                {
                    for(item = 0; item < recipe->recipe_size; item++)
                    {
                        if(recipe->recipe[item].id == set->stageSet._list[index].recipeId)
                        {
                            struct tm *myTime;

                            if(index)
                            {
                                myTime = getTimeStampByDate(&starts);
                            }
                            else
                            {
                                time_t startT = starts + set->stageSet._list[index - 1].duration_day * 24 * 60 * 60;
                                myTime = getTimeStampByDate(&startT);
                            }
                            sprintf(firstStartAt,"%02d%02d%02d%02d%02d%02d",
                                    myTime->tm_year + 1900, myTime->tm_mon + 1, myTime->tm_mday,
                                    myTime->tm_hour, myTime->tm_min, myTime->tm_sec);

                            usedCalFlg = ON;
                            break;
                        }
                    }
                }

                if(ON == usedCalFlg)
                {
                    break;
                }

                starts += set->stageSet._list[index].duration_day * 24 * 60 * 60;
            }
        }
    }

    if(OFF == usedCalFlg)
    {
        //使用系统设置
        if(RT_NULL != tempSet)
        {
            rt_memcpy((u8 *)tempSet, (u8 *)&set->tempSet, sizeof(proTempSet_t));
        }

        if(RT_NULL != co2Set)
        {
            rt_memcpy((u8 *)co2Set, (u8 *)&set->co2Set, sizeof(proCo2Set_t));
        }

        if(RT_NULL != humiSet)
        {
            rt_memcpy((u8 *)humiSet, (u8 *)&set->humiSet, sizeof(proHumiSet_t));
        }

        if(RT_NULL != line1Set)
        {
            rt_memcpy((u8 *)line1Set, (u8 *)&set->line1Set, sizeof(proLine_t));
        }

        if(RT_NULL != line2Set)
        {
            rt_memcpy((u8 *)line2Set, (u8 *)&set->line2Set, sizeof(proLine_t));
        }

        if(RT_NULL != line_4Set)
        {
            rt_memcpy((u8 *)line_4Set, (u8 *)&set->line1_4Set, sizeof(proLine_4_t));
        }

        if(RT_NULL != info)
        {
            strncpy(info->name, "--", RECIPE_NAMESZ - 1);
            info->name[RECIPE_NAMESZ - 1] = '\0';

            info->week = 0;//天化为星期
            info->day = 0;
        }
    }
    else if(ON == usedCalFlg)
    {
        //使用日历设置, 但是相关联的一些标志要使用系统的
        if(RT_NULL != tempSet)
        {
            rt_memcpy((u8 *)tempSet, (u8 *)&set->tempSet, sizeof(proTempSet_t));
            tempSet->dayCoolingTarget = recipe->recipe[item].dayCoolingTarget;
            tempSet->dayHeatingTarget = recipe->recipe[item].dayHeatingTarget;
            tempSet->nightCoolingTarget = recipe->recipe[item].nightCoolingTarget;
            tempSet->nightHeatingTarget = recipe->recipe[item].nightHeatingTarget;
        }

        if(RT_NULL != co2Set)
        {
            rt_memcpy((u8 *)co2Set, (u8 *)&set->co2Set, sizeof(proCo2Set_t));
            co2Set->dayCo2Target = recipe->recipe[item].dayCo2Target;
            co2Set->nightCo2Target = recipe->recipe[item].nightCo2Target;
        }

        if(RT_NULL != humiSet)
        {
            rt_memcpy((u8 *)humiSet, (u8 *)&set->humiSet, sizeof(proHumiSet_t));
            humiSet->dayHumiTarget = recipe->recipe[item].dayHumidifyTarget;
            humiSet->dayDehumiTarget = recipe->recipe[item].dayDehumidifyTarget;
            humiSet->nightHumiTarget = recipe->recipe[item].nightHumidifyTarget;
            humiSet->nightDehumiTarget = recipe->recipe[item].nightDehumidifyTarget;
        }

        if(RT_NULL != line1Set)
        {
            rt_memcpy((u8 *)line1Set, (u8 *)&set->line1Set, sizeof(proLine_t));
            line1Set->brightMode = recipe->recipe[item].line_list[0].brightMode;
            line1Set->byPower = recipe->recipe[item].line_list[0].byPower;
            line1Set->byAutoDimming = recipe->recipe[item].line_list[0].byAutoDimming;
            line1Set->mode = recipe->recipe[item].line_list[0].mode;
            line1Set->lightOn = recipe->recipe[item].line_list[0].lightOn;
            line1Set->lightOff = recipe->recipe[item].line_list[0].lightOff;
            changeCharToDate(firstStartAt, &time);
            line1Set->firstCycleTime = time.hour * 60 + time.minute;// 云服务器修改协议，后续逻辑修改较多，在此转化
            line1Set->firstRuncycleTime = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
            line1Set->duration = recipe->recipe[item].line_list[0].duration;
            line1Set->pauseTime = recipe->recipe[item].line_list[0].pauseTime;
        }

        if(RT_NULL != line2Set)
        {
            rt_memcpy((u8 *)line2Set, (u8 *)&set->line2Set, sizeof(proLine_t));
            line2Set->brightMode = recipe->recipe[item].line_list[1].brightMode;
            line2Set->byPower = recipe->recipe[item].line_list[1].byPower;
            line2Set->byAutoDimming = recipe->recipe[item].line_list[1].byAutoDimming;
            line2Set->mode = recipe->recipe[item].line_list[1].mode;
            line2Set->lightOn = recipe->recipe[item].line_list[1].lightOn;
            line2Set->lightOff = recipe->recipe[item].line_list[1].lightOff;
            changeCharToDate(firstStartAt, &time);
            line2Set->firstCycleTime = time.hour * 60 + time.minute;// 云服务器修改协议，后续逻辑修改较多，在此转化
            line2Set->firstRuncycleTime = systimeToTimestamp(time.year, time.month, time.day, time.hour, time.minute, 0);
            line2Set->duration = recipe->recipe[item].line_list[1].duration;
            line2Set->pauseTime = recipe->recipe[item].line_list[1].pauseTime;
        }

        if(RT_NULL != line_4Set)
        {
            rt_memcpy((u8 *)line_4Set, (u8 *)&recipe->recipe[item].line_4, sizeof(proLine_4_t));
            line_4Set->tempStartDimming = set->line1_4Set.tempStartDimming;
            line_4Set->tempOffDimming = set->line1_4Set.tempOffDimming;
            line_4Set->sunriseSunSet = set->line1_4Set.sunriseSunSet;
            strncpy(line_4Set->firstStartAt, firstStartAt, 15);
        }

        if(RT_NULL != info)
        {
            char year[5] = " ", mon[3] = " ", day[3] = " ";
            strncpy(info->name, recipe->recipe[item].name, RECIPE_NAMESZ - 1);
            info->name[RECIPE_NAMESZ - 1] = '\0';
            strncpy(year, set->stageSet.starts, 4);
            year[4] = '\0';
            strncpy(mon, &set->stageSet.starts[4], 2);
            mon[2] = '\0';
            strncpy(day, &set->stageSet.starts[6], 2);
            day[2] = '\0';
            time_t time1 = changeDataToTimestamp(atoi(year), atoi(mon), atoi(day), 0, 0, 0);
            if(getTimeStamp() > time1)
            {
                info->week = (getTimeStamp() - time1) / (24 * 60 * 60) / 7;//天化为星期
                info->day = (getTimeStamp() - time1) / (24 * 60 * 60) % 7;
            }
        }
    }
}

//专用于co2关联
u8 GetDeviceStateByCo2Lock(type_monitor_t *monitor, u8 type)
{
    device_t    *device     = RT_NULL;

    for(int i = 0; i < monitor->device_size; i++)
    {
        device = &monitor->device[i];
        if(type == device->type)
        {
            if(ON == device->port[0].ctrl.d_state)
            {
                return YES;
            }
        }
        else if(AC_4_TYPE == device->type)
        {
            for(int j = 0; j < device->storage_size; j++)
            {
                if(type == device->port[j].type)
                {
                    if(ON == device->port[j].ctrl.d_state)
                    {
                        return YES;
                    }
                }
            }
        }
    }

    return NO;
}

//mPeriod 周期 单位ms
void co2Program(type_monitor_t *monitor, type_uart_class uart, u16 mPeriod)
{
    int             co2Now      = 0;
    u16             co2Target   = 0;
    static u16      runTime     = 0;
    static u16      stopTime    = 0;
    u8              switchFlg   = 0;
    proCo2Set_t     co2Set;

    GetNowSysSet(RT_NULL, &co2Set, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    if(SENSOR_CTRL_AVE == GetSysSet()->sensorMainType)//平均模式
    {
        co2Now = getSensorDataByFunc(monitor, F_S_CO2);
    }
    else if(SENSOR_CTRL_MAIN == GetSysSet()->sensorMainType)
    {
        sensor_t *sensor = GetMainSensorByAddr(monitor, BHS_TYPE);
        if(sensor)
        {
            for(int i = 0; i < sensor->storage_size; i++)
            {
                if(F_S_CO2 == sensor->__stora[i].func)
                {
                    co2Now = sensor->__stora[i].value;
                }
            }
        }
        else
        {
            co2Now = VALUE_NULL;
        }
    }

    if(VALUE_NULL != co2Now)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            co2Target = co2Set.dayCo2Target;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            co2Target = co2Set.nightCo2Target;
        }

        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            if(ON == co2Set.isFuzzyLogic)
            {
                //检测当前
                //开10s 再关闭 10秒
                if((runTime < 10 * 1000) && (co2Now <= co2Target))
                {
                    runTime += mPeriod;
                    switchFlg = 1;
                }
                else
                {
                    switchFlg = 0;
                    if(stopTime < 10 * 1000)
                    {
                        stopTime += mPeriod;
                    }
                    else
                    {
                        runTime = 0;
                        stopTime = 0;
                    }
                }

                if(1 == switchFlg)
                {
                    if(!((ON == co2Set.dehumidifyLock && YES == GetDeviceStateByCo2Lock(monitor, DEHUMI_TYPE)) ||
                         (ON == co2Set.coolingLock && YES == GetDeviceStateByCo2Lock(monitor, COOL_TYPE))))
                    {
                        if((VALUE_NULL == getSensorDataByFunc(monitor, F_S_O2)) ||
                           (getSensorDataByFunc(monitor, F_S_O2) >= 180))
                        {
                            uart.DeviceCtrl(monitor, F_Co2_UP, ON);
                        }
                    }
                    else
                    {
                        uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                    }
                }
                else
                {
                    uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                }
            }
            else
            {
                if(co2Now <= co2Target)
                {
                    //如果和制冷联动 则制冷的时候不增加co2
                    //如果和除湿联动 则除湿的时候不增加co2
                    if(!((ON == co2Set.dehumidifyLock && YES == GetDeviceStateByCo2Lock(monitor, DEHUMI_TYPE)) ||
                         (ON == co2Set.coolingLock && YES == GetDeviceStateByCo2Lock(monitor, COOL_TYPE))))
                    {
                        if((VALUE_NULL == getSensorDataByFunc(monitor, F_S_O2)) ||
                           (getSensorDataByFunc(monitor, F_S_O2) >= 180))
                        {
                            uart.DeviceCtrl(monitor, F_Co2_UP, ON);
                        }
                    }
                    else
                    {
                        uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                    }
                }
                else if(co2Now >= co2Target + co2Set.co2Deadband)
                {
                    uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
                }
            }
            uart.DeviceCtrl(monitor, F_Co2_DOWN, OFF);
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            if(co2Now > co2Target)
            {
                uart.DeviceCtrl(monitor, F_Co2_DOWN, ON);
            }
            else if(co2Now + co2Set.co2Deadband <= co2Target)
            {
                uart.DeviceCtrl(monitor, F_Co2_DOWN, OFF);
            }
            uart.DeviceCtrl(monitor, F_Co2_UP, OFF);
        }
    }
}

void humiProgram(type_monitor_t *monitor, type_uart_class uart)
{
    int             humiNow             = 0;
    u16             humiTarget          = 0;
    u16             dehumiTarget        = 0;
    proHumiSet_t    humiSet;
    proTempSet_t    tempSet;

    GetNowSysSet(&tempSet, RT_NULL, &humiSet, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    if(SENSOR_CTRL_AVE == GetSysSet()->sensorMainType)//平均模式
    {
        humiNow = getSensorDataByFunc(monitor, F_S_HUMI);
    }
    else if(SENSOR_CTRL_MAIN == GetSysSet()->sensorMainType)
    {
        sensor_t *sensor = GetMainSensorByAddr(monitor, BHS_TYPE);
        if(sensor)
        {
            for(int i = 0; i < sensor->storage_size; i++)
            {
                if(F_S_HUMI == sensor->__stora[i].func)
                {
                    humiNow = sensor->__stora[i].value;
                }
            }
        }
        else
        {
            humiNow = VALUE_NULL;
        }
    }

    if(VALUE_NULL != humiNow)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            humiTarget = humiSet.dayHumiTarget;
            dehumiTarget = humiSet.dayDehumiTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            humiTarget = humiSet.nightHumiTarget;
            dehumiTarget = humiSet.nightDehumiTarget;
        }

        //达到湿度目标
        if(humiNow >= dehumiTarget)
        {
            uart.DeviceCtrl(monitor, F_DEHUMI, ON);
        }
        else if(humiNow <= dehumiTarget - humiSet.humidDeadband)
        {
            uart.DeviceCtrl(monitor, F_DEHUMI, OFF);
        }

        if(humiNow <= humiTarget)
        {
            uart.DeviceCtrl(monitor, F_HUMI, ON);
        }
        else if(humiNow >= humiTarget + humiSet.humidDeadband)
        {
            uart.DeviceCtrl(monitor, F_HUMI, OFF);
        }

        //当前有一个逻辑是降温和除湿联动选择
        if(ON == tempSet.coolingDehumidifyLock)
        {
            //如果除湿是开的话，AC_cool 不能关，因为可能AC_cool 上插着风扇
            if(ON == GetDeviceStateByFunc(monitor, F_DEHUMI))
            {
                uart.DeviceCtrl(monitor, F_COOL, ON);
            }
        }
    }

}

void tempProgram(type_monitor_t *monitor, type_uart_class uart)
{
    int             tempNow             = 0;
    u16             coolTarge           = 0;
    u16             HeatTarge           = 0;
    proTempSet_t    tempSet;

    GetNowSysSet(&tempSet, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL);

    if(SENSOR_CTRL_AVE == GetSysSet()->sensorMainType)//平均模式
    {
        tempNow = getSensorDataByFunc(monitor, F_S_TEMP);
    }
    else if(SENSOR_CTRL_MAIN == GetSysSet()->sensorMainType)
    {
        sensor_t *sensor = GetMainSensorByAddr(monitor, BHS_TYPE);
        if(sensor)
        {
            for(int i = 0; i < sensor->storage_size; i++)
            {
                if(F_S_TEMP == sensor->__stora[i].func)
                {
                    tempNow = sensor->__stora[i].value;
                }
            }
        }
        else
        {
            tempNow = VALUE_NULL;
        }
    }

    if(VALUE_NULL != tempNow)
    {
        if(DAY_TIME == GetSysSet()->dayOrNight)
        {
            coolTarge = tempSet.dayCoolingTarget;
            HeatTarge = tempSet.dayHeatingTarget;
        }
        else if(NIGHT_TIME == GetSysSet()->dayOrNight)
        {
            coolTarge = tempSet.nightCoolingTarget;
            HeatTarge = tempSet.nightHeatingTarget;
        }

        if(tempNow >= coolTarge)
        {
            //打开所以制冷功能设备
            uart.DeviceCtrl(monitor, F_COOL, ON);
        }
        else if(tempNow <= (coolTarge - tempSet.tempDeadband))
        {
            uart.DeviceCtrl(monitor, F_COOL, OFF);
        }

        if(tempNow <= HeatTarge)
        {
            uart.DeviceCtrl(monitor, F_HEAT, ON);
        }
        else if(tempNow >= HeatTarge + tempSet.tempDeadband)
        {
            uart.DeviceCtrl(monitor, F_HEAT, OFF);
        }
    }
}

void dimmingLineCtrl(u8 *stage, u16 ppfd)
{
    //stage 范围在10 - 115之间，一档为5 %
    int         par         = 0;
    static u8   STAGE_VALUE = 5;

    par = getSensorDataByFunc(GetMonitor(), F_S_PAR);
    if(VALUE_NULL != par)
    {
        if(par + 50 <= ppfd)
        {
            if(*stage <= 115 - STAGE_VALUE)
            {
                *stage = *stage + STAGE_VALUE;
            }
        }
        else if(par > ppfd + 50)
        {
            if(*stage > STAGE_VALUE)
            {
                *stage -= STAGE_VALUE;
            }
        }
    }
}

//返回一个周期需要的时间 单位s
static void GetLine_4CyclePeriodTime(proLine_4_t *set, time_t *time)
{
    *time = 0;
    for(int i = 0; i < LINE_4_CYCLE_MAX; i++)
    {
        *time += set->cycleList[i].duration;
    }
    *time += set->pauseTime;
}

/**
 *
 * @param startT    ：开始时间时间戳
 * @param continueT ：持续时间
 * @param sunrise   ：日升日落时间
 * @param nowV      ：当前时间戳
 * @param TargetV   ：目标值
 * @param retV 当前灯光需要设置的值
 * 时间单位 s
 */

static void GetLine4Value(time_t nowT, time_t startT, time_t continueT, u16 sunrise, u8 lowV, u8 TargetV, u8 *retV)
{
    u16 slowUpT = 0;//日升日落需要的时间
    float value = 0;

    if(continueT < sunrise)//日升日落时间比开始的持续时间还久
    {
        slowUpT = continueT;
    }
    else
    {
        slowUpT = sunrise;
    }

    if(nowT > startT)
    {
        //上升阶段
        if(nowT < startT + slowUpT)
        {
            if(TargetV > lowV)
            {
                value = (float)(TargetV - lowV) / slowUpT;
                *retV = value * (nowT - startT) + lowV;
                if(*retV > TargetV)
                {
                    *retV = TargetV;
                }
            }
            else
            {
                *retV = TargetV;
            }
        }
        //下降阶段
        //nowT - startT >= continueT - sunrise
        else if(nowT + sunrise >= startT + continueT)
        {
            if(TargetV > lowV)
            {
                value = (float)(TargetV - lowV) / slowUpT;
                *retV = value * (continueT - (nowT - startT)) + lowV;
            }
        }
        else
        {
            *retV = TargetV;
        }
    }
    else
    {
        *retV = lowV;
    }
}

//获取真正的地址
void GetRealLine4V(dimmingCurve_t* curve, u8 port, u8 value, u8 *real)
{
    int x1 = 0;
    int x2 = 0;
    int y1 = 0;
    int y2 = 0;
    float a = 0.0;
    float b = 0.0;

    switch(port)
    {
        case 0:
            x1 = curve->onOutput1;
            y1 = curve->onVoltage1;
            x2 = 100;
            y2 = curve->fullVoltage1;
            break;
        case 1:
            x1 = curve->onOutput2;
            y1 = curve->onVoltage2;
            x2 = 100;
            y2 = curve->fullVoltage2;
            break;
        case 2:
            x1 = curve->onOutput3;
            y1 = curve->onVoltage3;
            x2 = 100;
            y2 = curve->fullVoltage3;
            break;
        case 3:
            x1 = curve->onOutput4;
            y1 = curve->onVoltage4;
            x2 = 100;
            y2 = curve->fullVoltage4;
            break;
        default : break;
    }

    if(x2 > x1)
    {
        a = (float)(y2 - y1)/(x2 - x1);
        b = (float)(x2*y1 - x1*y2)/(x2 - x1);
    }

    int res = a * value + b;
    if(res > 0)
    {
        *real = res;
    }
    else {
        *real = 0;
    }
}

void line_4Program(line_t *line, type_uart_class lineUart)
{
    u8              state                       = 0;
    time_t          now_time                    = 0;    //化当前时间为hour + minute +second 格式
    proLine_4_t     line_4set;
    line_4_timer_t  *nowTimerSet                = RT_NULL;
    type_sys_time   time;
    u8              lineRecipeNo                = 0;
    time_t          startOnTime                 = 0;    //开始开启的时间戳
    time_t          onContineTime               = 0;    //持续开启的时间
    static u8       stage[LINE_PORT_MAX]        = {0,0,0,0};
    u16             ctrlValue[LINE_PORT_MAX]    = {0,0,0,0};
    sys_set_t       *sys_set                    = GetSysSet();

    //1.获取灯光设置
    GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, RT_NULL, &line_4set, RT_NULL, RT_NULL);

    //3.判断模式是recycle 还是 timer,是否需要开灯
    getRealTimeForMat(&time);
    now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;//精确到秒

    if(LINE_BY_TIMER == line_4set.mode)
    {
        //3.1 选中定时器设置
        for(int i = 0; i < LINE_4_TIMER_MAX; i++)
        {
            u16 onTime = line_4set.timerList[i].on;
            u16 offTime = line_4set.timerList[i].off;

            //判断是否是跨天
            if(offTime > onTime)
            {
                if(now_time >= onTime * 60 &&
                   now_time < offTime * 60)
                {
                    nowTimerSet = &line_4set.timerList[i];
                    break;
                }
            }
            else
            {
                if((now_time >= onTime * 60 && now_time <= 24 * 60 * 60) ||
                   (now_time < offTime * 60))
                {
                    nowTimerSet = &line_4set.timerList[i];
                    break;
                }
            }
        }

        //3.1.1
        if(nowTimerSet)
        {
           //开
           state = nowTimerSet->en;
           lineRecipeNo = nowTimerSet->no;
           //将开始的时间转化为timestamp
            time_t time1 = getTimeStamp();
            struct tm *timeTemp = getTimeStampByDate(&time1);

            startOnTime = systimeToTimestamp(timeTemp->tm_year + 1900, timeTemp->tm_mon + 1, timeTemp->tm_mday,
                nowTimerSet->on / 60, nowTimerSet->on % 60, 0);
            if(nowTimerSet->off > nowTimerSet->on)
            {
                onContineTime = (nowTimerSet->off - nowTimerSet->on) * 60;
            }
            else
            {
                onContineTime = (nowTimerSet->off + 24 * 60 - nowTimerSet->on) * 60;
            }
        }
        else
        {
           state = OFF;
        }
    }
    else if(LINE_BY_CYCLE == line_4set.mode)
    {
        //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
        type_sys_time getTime;
        changeCharToDate(line_4set.firstStartAt, &getTime);
        //time_t firstCycleTime = getTime.hour * 60 + getTime.minute;
        time_t isStartTime = systimeToTimestamp(getTime.year, getTime.month, getTime.day, getTime.hour, getTime.minute, 0);

        if(getTimeStamp() >= isStartTime)
        {
            time_t timeStage = getTimeStamp() - isStartTime;    //当前和开始时间相比走了多久
            time_t periodTime = 0;
            GetLine_4CyclePeriodTime(&line_4set, &periodTime);  //获取一个周期的时间

            //获取当前的配方号以及开关状态
            time_t timeAdd = 0;
            int i = 0;
            for(i = 0; i < LINE_4_CYCLE_MAX; i++)
            {
                if(((timeStage % periodTime) >= timeAdd) &&
                   ((timeStage % periodTime) < timeAdd + line_4set.cycleList[i].duration))
                {
                    state = ON;
                    lineRecipeNo = line_4set.cycleList[i].no;
                    time_t time1 = getTimeStamp() - (timeStage % periodTime) + timeAdd;
                    struct tm *timeTemp = getTimeStampByDate(&time1);
                    startOnTime = systimeToTimestamp(timeTemp->tm_year + 1900, timeTemp->tm_mon + 1, timeTemp->tm_mday,
                                    timeTemp->tm_hour, timeTemp->tm_min, 0);
                    onContineTime = line_4set.cycleList[i].duration;
                    break;
                }
                timeAdd += line_4set.cycleList[i].duration;
            }

            //如果是不开启的状态
            if(i == LINE_4_CYCLE_MAX)
            {
                state = OFF;
            }
        }
        else
        {
            state = OFF;
        }
    }

    //4.固定比例 / 恒光模式
    if(ON == state)
    {
        //lineRecipeNo 指的是配方名称 配方名称比下标多1
        lineRecipeNo -= 1;
        //如果是恒光模式
        if(LINE_MODE_AUTO_DIMMING == line_4set.brightMode)
        {
            dimmingLineCtrl(&stage[0], sys_set->line1_4Set.byAutoDimming);
            dimmingLineCtrl(&stage[1], sys_set->line1_4Set.byAutoDimming);
            dimmingLineCtrl(&stage[2], sys_set->line1_4Set.byAutoDimming);
            dimmingLineCtrl(&stage[3], sys_set->line1_4Set.byAutoDimming);
        }
        //如果是按照比例
        else if(LINE_MODE_BY_POWER == line_4set.brightMode)
        {
            //日升日落
            if(0 == line_4set.sunriseSunSet)
            {
                state = OFF;
            }
            else
            {
                GetLine4Value(getTimeStamp(), startOnTime, onContineTime, line_4set.sunriseSunSet * 60,
                              sys_set->dimmingCurve.onOutput1, sys_set->lineRecipeList[lineRecipeNo].output1, &stage[0]);
                GetLine4Value(getTimeStamp(), startOnTime, onContineTime, line_4set.sunriseSunSet * 60,
                              sys_set->dimmingCurve.onOutput2, sys_set->lineRecipeList[lineRecipeNo].output2, &stage[1]);
                GetLine4Value(getTimeStamp(), startOnTime, onContineTime, line_4set.sunriseSunSet * 60,
                              sys_set->dimmingCurve.onOutput3, sys_set->lineRecipeList[lineRecipeNo].output3, &stage[2]);
                GetLine4Value(getTimeStamp(), startOnTime, onContineTime, line_4set.sunriseSunSet * 60,
                              sys_set->dimmingCurve.onOutput4, sys_set->lineRecipeList[lineRecipeNo].output4, &stage[3]);
            }
        }

        //获取温度
        s16 temperature = 0;
        for(u8 index = 0; index < GetMonitor()->sensor_size; index++)
        {
            for(u8 item = 0; item < GetMonitor()->sensor[index].storage_size; item++)
            {
                if(F_S_TEMP == GetMonitor()->sensor[index].__stora[item].func)
                {
                    temperature = GetMonitor()->sensor[index].__stora[item].value;
                }
            }
        }

        //过温保护
        if(temperature >= line_4set.tempOffDimming)
        {
            LOG_D("------in dimin off");
            state = OFF;
        }
        else if(temperature >= line_4set.tempStartDimming)
        {
            LOG_D("------in dimin");
            for(int port = 0; port < LINE_PORT_MAX; port++)
            {
                stage[port] /= 2;
            }
        }
    }
    else
    {
        for(int port = 0; port < LINE_PORT_MAX; port++)
        {
            stage[port] = 0;
        }
    }

    if(OFF == state)
    {
        rt_memset(ctrlValue, 0, sizeof(ctrlValue));
    }
    else
    {
        for(int i = 0; i < LINE_PORT_MAX; i++)
        {
            u8 res = 0;
            GetRealLine4V(&GetSysSet()->dimmingCurve, i, stage[i], &res);
            ctrlValue[i] = (stage[i] << 8) | res;
        }
    }
    lineUart.Line4Ctrl(line, ctrlValue);
}

void lineProgram(type_monitor_t *monitor, u8 line_no, type_uart_class lineUart, u16 mPeroid)
{
    u8              state           = 0;
    u8              value           = 0;
    u8              sunriseFlg      = 0;
    u8              temp_stage      = 0;
    time_t          now_time        = 0;    //化当前时间为hour + minute +second 格式
    time_t          start_time      = 0;    //化开始循环时间为hour + minute +second 格式
    time_t          period_time     = 0;    //化一个循环时间为hour + minute +second 格式
    time_t          temp_time       = 0;
    line_t          *line           = RT_NULL;
    proLine_t       line_set;
    type_sys_time   time;
    u16             temperature     = 0;
    static u8       stage[LINE_MAX] = {LINE_MIN_VALUE,LINE_MIN_VALUE};
    static u16      cnt[LINE_MAX]   = {0, 0};

    //1.获取灯光设置
    if(0 == line_no)
    {
        line = &monitor->line[0];
        GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, &line_set, RT_NULL, RT_NULL, RT_NULL);
    }
    else if(1 == line_no)
    {
        line = &monitor->line[1];
        GetNowSysSet(RT_NULL, RT_NULL, RT_NULL, RT_NULL, RT_NULL, &line_set, RT_NULL);
    }
    else
    {
        LOG_E("lineProgram err1");
        return;
    }

    //2.判断灯光设置的合理性
    if(line_set.hidDelay > 180 || line_set.hidDelay < 3)
    {
        line_set.hidDelay = 3;
    }

    if(line_set.byPower > 115 || line_set.byPower < 10)
    {
        line_set.byPower = 10;
    }

    //3.判断模式是recycle 还是 timer,是否需要开灯
    getRealTimeForMat(&time);
    if(LINE_BY_TIMER == line_set.mode)
    {
        //3.1 如果是定时器模式 那就需要看看是否处于定时器范围内
        //3.1.1 处于正常的一天内
        if(line_set.lightOn < line_set.lightOff)
        {
            if(time.hour * 60 * 60 + time.minute * 60 + time.second > line_set.lightOn * 60)
            {
               //小于持续时间
               if(time.hour * 60 * 60 + time.minute * 60 + time.second <= line_set.lightOff * 60)
               {
                   //开
                   state = ON;

                   // 3.1.1 lightOff - lightOn <= sunriseSunSet  该过程只有上升过程
                   now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
                   start_time = line_set.lightOn;
                   if(line_set.lightOff <= line_set.lightOn + line_set.sunriseSunSet)
                   {
                       sunriseFlg = LINE_UP;
                   }
                   // 3.1.2 sunriseSunSet <= lightOff - lightOn &&  2*sunriseSunSet >= lightOff - lightOn  该过程有上升过程 下降过程不完整
                   else if((line_set.lightOff >= line_set.lightOn + line_set.sunriseSunSet) &&
                           (line_set.lightOff <= line_set.lightOn + 2 *line_set.sunriseSunSet))
                   {
                       if(now_time <= (line_set.sunriseSunSet + line_set.lightOn) * 60)
                       {
                           sunriseFlg = LINE_UP;
                       }
                       else
                       {
                           sunriseFlg = LINE_DOWN;
                       }
                   }
                   // 3.1.3 2*sunriseSunSet < lightOff - lightOn  该过程有上升过程 下降过程 恒定过程
                   else if(line_set.lightOff > line_set.lightOn + 2 *line_set.sunriseSunSet)
                   {
                       if(now_time <= (line_set.sunriseSunSet + line_set.lightOn) * 60)
                       {
                           sunriseFlg = LINE_UP;
                       }
                       //now_time - lightOn < lightOff - lightOn - sunriseSunSet 恒定
                       else if(now_time + line_set.sunriseSunSet * 60 < line_set.lightOff * 60)
                       {
                           sunriseFlg = LINE_STABLE;
                       }
                       else
                       {
                           sunriseFlg = LINE_DOWN;
                       }
                   }
               }
               else
               {
                   state = OFF;
               }
            }
            else
            {
                state = OFF;
            }
        }
        //跨天的话
        else if(line_set.lightOn > line_set.lightOff)
        {
            now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;

            if((now_time < line_set.lightOff * 60) || (now_time >= line_set.lightOn * 60))
            {
                state = ON;

                //1.如果on 和 off之间的时间不足sunset时，那么只有上升阶段
                if(line_set.lightOff + 24 * 60  <= line_set.sunriseSunSet + line_set.lightOn)
                {
                    sunriseFlg = LINE_UP;
                }
                //2.
                else if((line_set.lightOff + 24 * 60 > line_set.sunriseSunSet + line_set.lightOn) &&
                        (line_set.lightOff + 24 * 60 <= line_set.sunriseSunSet * 2 + line_set.lightOn))
                {
                    if((now_time > line_set.lightOn * 60))//都化为秒
                    {
                        if(now_time <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                    }
                    else if(now_time < line_set.lightOff * 60)
                    {
                        if(now_time + 24 * 60 * 60 <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                    }
                }
                //3.
                else if(line_set.lightOff + 24 * 60 > line_set.sunriseSunSet * 2 + line_set.lightOn)
                {
                    if((now_time > line_set.lightOn * 60))//都化为秒
                    {
                        if(now_time <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else if(line_set.lightOff * 60 + 24 * 60 * 60 <= line_set.sunriseSunSet * 60 + now_time)
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                        else
                        {
                            sunriseFlg = LINE_STABLE;
                        }
                    }
                    else if((now_time < line_set.lightOff * 60))//都化为秒
                    {
                        if(now_time + 24 * 60 * 60 <= line_set.sunriseSunSet * 60 + line_set.lightOn * 60)
                        {
                            sunriseFlg = LINE_UP;
                        }
                        else if(line_set.lightOff * 60 <= line_set.sunriseSunSet * 60 + now_time)
                        {
                            sunriseFlg = LINE_DOWN;
                        }
                        else
                        {
                            sunriseFlg = LINE_STABLE;
                        }
                    }
                }
            }
        }
    }
    else if(LINE_BY_CYCLE == line_set.mode)
    {

        //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
        now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
        start_time = line_set.firstCycleTime * 60;
        period_time = line_set.duration + line_set.pauseTime;
        if(getTimeStamp() > line_set.firstRuncycleTime)
        {
            state = ON;

            if(((getTimeStamp() - line_set.firstRuncycleTime) %
                (line_set.duration + line_set.pauseTime)) <=
                line_set.duration)
            {
                temp_time = (now_time - start_time) % period_time;
                // 3.2.1 duration <= sunriseSunSet  该过程只有上升过程
                if(line_set.duration <= line_set.sunriseSunSet * 60)
                {
                    sunriseFlg = LINE_UP;
                }
                // 3.2.2 sunriseSunSet <= duration &&  2*sunriseSunSet >= duration  该过程有上升过程 下降过程不完整
                else if((line_set.duration >= line_set.sunriseSunSet * 60) &&
                        (line_set.duration <= 2 * line_set.sunriseSunSet * 60))
                {
                    if(temp_time <= line_set.sunriseSunSet * 60)
                    {
                        sunriseFlg = LINE_UP;
                    }
                    else
                    {
                        sunriseFlg = LINE_DOWN;
                    }
                }
                // 3.2.3 2*sunriseSunSet < duration  该过程有上升过程 下降过程 恒定过程
                else if(line_set.duration > 2 *line_set.sunriseSunSet * 60)
                {
                    if(temp_time <= line_set.sunriseSunSet * 60)
                    {
                        sunriseFlg = LINE_UP;
                    }
                    //temp_time < duration - sunriseSunSet 恒定
                    else if(line_set.duration > temp_time + line_set.sunriseSunSet * 60)
                    {
                        sunriseFlg = LINE_STABLE;
                    }
                    else
                    {
                        sunriseFlg = LINE_DOWN;
                    }
                }
            }
            else
            {
                state = OFF;
            }
        }
        else
        {
            state = OFF;
        }
    }

    //4.固定比例 / 恒光模式
    if(ON == state)
    {
        //4.0 计算升档时间
        if(LINE_BY_TIMER == line_set.mode)
        {
            now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
            if(LINE_MODE_BY_POWER == line_set.brightMode)
            {
                if(LINE_UP == sunriseFlg)
                {
                    if((line_set.sunriseSunSet + line_set.lightOn) * 60 > now_time)
                    {
                        if(line_set.byPower > line->port[0].ctrl.d_value)
                        {
                            if(now_time > line_set.lightOn * 60)
                            {
                                temp_stage = (((line_set.sunriseSunSet + line_set.lightOn) * 60 - now_time) *1000 / mPeroid)/
                                             (line_set.byPower - line->port[0].ctrl.d_value);
                            }
                            else if(now_time < line_set.lightOff * 60)
                            {
                                temp_stage = (((line_set.sunriseSunSet + line_set.lightOn) * 60 - now_time - 24 * 60 * 60) *1000 / mPeroid)/
                                             (line_set.byPower - line->port[0].ctrl.d_value);
                            }
                        }
                    }
                }
                else if(LINE_DOWN == sunriseFlg)
                {
                    if(line_set.byPower > line->port[0].ctrl.d_value)
                    {
                        if(now_time > line_set.lightOn * 60)
                        {
                            if(line_set.lightOff * 60 + 24 * 60 * 60 < now_time + line_set.sunriseSunSet)
                            {
                                temp_stage = ((line_set.lightOff * 60 + 24 * 60 * 60 - now_time) *1000 / mPeroid)/(line_set.byPower - line->port[0].ctrl.d_value);
                            }
                        }
                        else if(now_time < line_set.lightOff * 60)
                        {
                            if(line_set.lightOff * 60 < now_time + line_set.sunriseSunSet)
                            {
                                temp_stage = ((line_set.lightOff * 60 - now_time) *1000 / mPeroid)/(line_set.byPower - line->port[0].ctrl.d_value);
                            }
                        }
                    }
                }
            }
        }
        else if(LINE_BY_CYCLE == line_set.mode)
        {
            if(LINE_MODE_BY_POWER == line_set.brightMode)
            {
                now_time = time.hour * 60 * 60 + time.minute * 60 + time.second;
                start_time = line_set.firstCycleTime * 60;
                if(now_time > start_time)
                {
                    period_time = line_set.duration + line_set.pauseTime;
                    temp_time = (now_time - start_time) % period_time;
                    if(LINE_UP == sunriseFlg)
                    {
                        //(日升日落 - 当前时间)/(目标值 - 当前值)
                        if(line_set.sunriseSunSet * 60 > temp_time)
                        {
                            if(line_set.byPower > line->port[0].ctrl.d_value)
                            {
                                temp_stage = ((line_set.sunriseSunSet * 60 - temp_time)*1000/mPeroid)/
                                        (line_set.byPower - line->port[0].ctrl.d_value);
                            }
                        }
                    }
                    else if(LINE_DOWN == sunriseFlg)
                    {
                        //(结束时间 - 当前时间)/(当前值 - 最小值)
                        if(line_set.duration <= temp_time + line_set.sunriseSunSet * 60)//结束时间 - 当前时间 <= 日升日落
                        {
                            if(line->port[0].ctrl.d_value > LINE_MIN_VALUE)
                            {
                                temp_stage = ((line_set.duration - temp_time) * 1000/mPeroid) / (line->port[0].ctrl.d_value - LINE_MIN_VALUE);
                            }
                        }
                    }
                }
            }
        }

        //4.1 恒光模式
        if(LINE_MODE_AUTO_DIMMING == line_set.brightMode)
        {
            dimmingLineCtrl(&stage[line_no], line_set.byAutoDimming);
            value = stage[line_no];
        }
        //4.2 固定比例
        else if(LINE_MODE_BY_POWER == line_set.brightMode)
        {
            if(0 == line_set.sunriseSunSet)
            {
                value = line_set.byPower;
            }
            else if((line_set.sunriseSunSet > 0) && (line_set.sunriseSunSet <= 30))//sunriseSunSet 单位分钟
            {
                if(LINE_UP == sunriseFlg)
                {
                    if(temp_stage < 1)
                    {
                        temp_stage = 1;
                    }

                    if(cnt[line_no] < temp_stage)
                    {
                        cnt[line_no]++;
                    }
                    else
                    {
                        cnt[line_no] = 0;
                        if(stage[line_no]  + 1 <= line_set.byPower)
                        {
                            stage[line_no] ++;
                        }
                    }
                }
                else if(LINE_STABLE == sunriseFlg)
                {
                    stage[line_no] = line_set.byPower;
                }
                else if(LINE_DOWN == sunriseFlg)
                {
                    if(temp_stage < 1)
                    {
                        temp_stage = 1;
                    }

                    if(cnt[line_no] < temp_stage)
                    {
                        cnt[line_no]++;
                    }
                    else
                    {
                        cnt[line_no] = 0;
                        if(stage[line_no] >= 1 + LINE_MIN_VALUE)
                        {
                            stage[line_no] -= 1;
                        }
                    }
                }

                value = stage[line_no];
            }
        }


        for(u8 index = 0; index < monitor->sensor_size; index++)
        {
            for(u8 item = 0; item < monitor->sensor[index].storage_size; item++)
            {
                if(F_S_TEMP == monitor->sensor[index].__stora[item].func)
                {
                    temperature = monitor->sensor[index].__stora[item].value;
                }
            }
        }

        //过温保护
        if(temperature >= line_set.tempOffDimming)
        {
            LOG_D("------in dimin off");
            state = OFF;
        }
        else if(temperature >= line_set.tempStartDimming)
        {
            LOG_D("------in dimin");
            stage[line_no] = LINE_DIMMING;
            value = stage[line_no];
        }
    }
    else
    {
        stage[line_no] = LINE_MIN_VALUE;
        value = stage[line_no];
    }

    if(value <= LINE_MIN_VALUE)
    {
        value = LINE_MIN_VALUE;
    }
    else if(value >= 115)
    {
        value = 115;
    }

    if(OFF == state)
    {
        value = 0;
    }

    lineUart.LineCtrl(line, 0, state, value);
    for(int i = 0; i < GetMonitor()->line_size; i++)
    {
        if((0 == line_no) && (1 == GetLineType(GetMonitor())))
        {
            if(LINE1_TYPE == GetMonitor()->line[i].type)
            {
                lineUart.LineCtrl(&GetMonitor()->line[i], 0, state, value);
            }
        }
        else if(1 == line_no)
        {
            if(LINE2_TYPE == GetMonitor()->line[i].type)
            {
                lineUart.LineCtrl(&GetMonitor()->line[i], 0, state, value);
            }
        }
    }
}

void Light12Program(type_monitor_t *monitor, type_uart_class deviceUart)
{
    u8                  index               = 0;
    u8                  port                = 0;
    u8                  state               = 0;
    device_t            *device             = RT_NULL;
    time_t              startOnTime         = 0;    //开始开启的时间戳
    int                 on_time             = 0;
    int                 off_time            = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        device = &monitor->device[index];

        //1.仅仅针对12灯光
        if(LIGHT_12_TYPE != device->type)
        {
            continue;
        }

        //2.
        for(port = 0; port < device->storage_size; port++)
        {
            state = 0;
            if(BY_RECYCLE == device->port[port].mode)
            {
                //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
                if(getTimeStamp() > device->port[port].cycle.start_at_timestamp)
                {
                    if(((getTimeStamp() - device->port[port].cycle.start_at_timestamp) %
                        (device->port[port].cycle.duration + device->port[port].cycle.pauseTime)) <=
                        device->port[port].cycle.duration)
                    {
                        state = ON;
                    }
                    else
                    {
                        state = OFF;
                    }
                }
                else
                {
                    state = OFF;
                }
            }
            else if(BY_SCHEDULE == device->port[port].mode)
            {
               //将开始的时间转化为timestamp
                time_t time1 = getTimeStamp();
                struct tm *timeTemp = getTimeStampByDate(&time1);

                startOnTime = timeTemp->tm_hour * 60 * 60 + timeTemp->tm_min * 60 + timeTemp->tm_sec;
                on_time = device->port[port].timer[0].on_at * 60;
                off_time = device->port[port].timer[0].duration * 60;
                //正常一天的设置 //duration在这里是offtime
                if(!(0 == off_time && 0 == on_time))
                {
                    if(off_time > on_time)
                    {
                        if(startOnTime >= on_time &&
                           startOnTime < off_time)
                        {
                            state = ON;
                        }
                        else
                        {
                            state = OFF;
                        }
                    }
                    //跨天 offtime < ontime
                    else
                    {
                        if(startOnTime >= on_time ||
                           startOnTime < off_time)
                        {
                            state = ON;
                        }
                        else
                        {
                            state = OFF;
                        }
                    }
                }
            }
            device->port[port].ctrl.d_state = state;
        }
        deviceUart.DeviceCtrlSingle(device, 0, device->port[0].ctrl.d_state );
    }
}

void timmerProgram(type_monitor_t *monitor, type_uart_class deviceUart)
{
    u8                  index       = 0;
    u8                  port        = 0;
    u8                  item        = 0;
    u8                  state       = 0;
    device_t            *device     = RT_NULL;
    type_sys_time       sys_time;

    getRealTimeForMat(&sys_time);

    for(index = 0; index < monitor->device_size; index++)
    {
        //如果是定时器的话
        device = &monitor->device[index];

        for(port = 0; port < device->storage_size; port++)
        {
            if(TIMER_TYPE == device->port[port].type)
            {
                if(BY_RECYCLE == device->port[port].mode)
                {
                    //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
                    if(getTimeStamp() > device->port[port].cycle.start_at_timestamp)
                    {
                        if(((getTimeStamp() - device->port[port].cycle.start_at_timestamp) %
                            (device->port[port].cycle.duration + device->port[port].cycle.pauseTime)) <=
                            device->port[port].cycle.duration)
                        {
                            state = ON;
                        }
                        else
                        {
                            state = OFF;
                        }
                    }
                    else
                    {
                        state = OFF;
                    }

                    device->port[port].ctrl.d_state = state;
                    deviceUart.DeviceCtrlSingle(device, port, state);
                }
                else if(BY_SCHEDULE == device->port[port].mode)//定时器模式
                {
                   for(item = 0; item < TIMER_GROUP; item++)//该功能待测试
                   {
                       //选择处于第几组定时器
                       if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second > device->port[port].timer[item].on_at * 60)
                       {
                           //小于持续时间
                           if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second <= (device->port[port].timer[item].on_at *60
                                   + device->port[port].timer[item].duration) )
                           {
                               state = device->port[port].timer[item].en;
                               break;
                           }
                       }
                       else
                       {
                           //1.判断如果存在跨天的话
                           if((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration) >
                               24 * 60 * 60)
                           {
                               //如果当前时间处于跨天的时间
                               if((sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second) <
                                       ((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration)- 24 * 60 * 60))
                               {
                                   state = device->port[port].timer[item].en;
                                   break;
                               }
                           }
                       }
                   }

                   if(item == TIMER_GROUP)
                   {
                       device->port[port].ctrl.d_state = 0;
                       deviceUart.DeviceCtrlSingle(device, port, 0);
                   }
                   else
                   {
                       //判断是否允许今日使能
                       time_t nowTime = getTimeStamp();
                       u8 today = getTimeStampByDate(&nowTime)->tm_wday;
                       int day = 0;
                       for(day = 0; day < 7; day++)
                       {
                           if(today == (day + 1))
                           {
                               if((device->port[port].weekDayEn >> day) & 0x01)
                               {
                                   break;
                               }
                           }
                       }

                       if(day == 7)
                       {
                           device->port[port].ctrl.d_state = 0;
                           deviceUart.DeviceCtrlSingle(device, port, 0);
                       }
                       else
                       {
                           device->port[port].ctrl.d_state = state;
                           deviceUart.DeviceCtrlSingle(device, port, state);
                       }
                   }
                }
            }
        }
    }

}

//默认在420ppm 环境中校准
void co2Calibrate1(type_monitor_t *monitor, int *data, u8 *do_cal_flg, u8 *saveFlg, PAGE_CB cb)
{
    u8              index                   = 0;
    u8              port                    = 0;
    sensor_t        *sensor                 = RT_NULL;
    static u8       cal_flag[SENSOR_MAX];      //标记是否校准完成
    static u8       cal_cnt[SENSOR_MAX];
    static u8       start                   = NO;
    static time_t   start_time              = 0;
    static int      STAND_CO2               = 420;
    static int      data1[SENSOR_MAX];


    //1.是否开始校准
    if(start != *do_cal_flg)
    {
        start = *do_cal_flg;

        if(YES == start)
        {
            start_time = getTimeStamp();
            rt_memset(data1, 0, SENSOR_MAX);
            rt_memset(cal_cnt, 0, SENSOR_MAX);
            rt_memset(cal_flag, CAL_NO, SENSOR_MAX);
        }
    }

    //LOG_E("-----------------time goes %d",getTimeStamp() - start_time);

    //2.60秒内完成采集与平均
    if(getTimeStamp() <= start_time + 60)
    {
        //遍历全部sensor 中的CO2
        for(index = 0; index < monitor->sensor_size; index++)
        {
            sensor = &GetMonitor()->sensor[index];

            for(port = 0; port < sensor->storage_size; port++)
            {
                if(F_S_CO2 == sensor->__stora[port].func)
                {
                    //3.如果10组是稳定的,那么就平均,否则重新采集
                    if(CAL_YES != cal_flag[index])
                    {
                        //LOG_D("value = %d",sensor->__stora[port].value);
                        if(cal_cnt[index] < 10)
                        {
                            //4.判断是否符合条件
                            if(abs(sensor->__stora[port].value - STAND_CO2) <= 300)
                            {
                                //LOG_W("co2Calibrate 1");
                                data1[index] += sensor->__stora[port].value;
                                cal_cnt[index]++;
                                cal_flag[index] = CAL_FAIL;
                            }
                            else
                            {
                                //LOG_W("co2Calibrate 2, data = %d",abs(sensor->__stora[port].value - STAND_CO2));
                                data1[index] = 0;
                                cal_cnt[index] = 0;
                                cal_flag[index] = CAL_FAIL;
                            }
                        }
                        else
                        {
                            //5.采集完毕
                            data1[index] /= cal_cnt[index];
                            data1[index] = STAND_CO2 - data1[index];
                            cal_flag[index] = CAL_YES;
                            data[index] = data1[index];
                        }
                    }
                }
            }
        }
    }
    else
    {
        *do_cal_flg = NO;
        start = NO;
        *saveFlg = YES;

        //6.判断是否是全部采集完成
        for(index = 0; index < monitor->sensor_size; index++)
        {
            if(CAL_FAIL == cal_flag[index])
            {
                cb(NO);
                return;
            }
        }

        cb(YES);

        /*for(index = 0; index < monitor->sensor_size; index++)
        {
            LOG_W("num %d, data = %d",index,data[index]);
        }*/
    }
}


#elif(HUB_IRRIGSTION == HUB_SELECT)

u8 pumpDoing(u8 addr, u8 port)
{
    u8                  state           = OFF;
    u8                  item            = 0;
    type_sys_time       sys_time;
    device_t            *device         = GetDeviceByAddr(GetMonitor(), addr);
    time_t              time            = getTimeStamp();

    getRealTimeForMat(&sys_time);

    if(RT_NULL != device)
    {
        if((PUMP_TYPE == device->port[port].type) ||
           (VALVE_TYPE == device->port[port].type))
        {
            //定时器模式
            if(BY_RECYCLE == device->port[port].mode)
            {
                //1.判断当前时间是否是满足进入循环周期的条件,即大于开始时间
                if(time > device->port[port].cycle.start_at_timestamp)
                {
                    if((time - device->port[port].cycle.start_at_timestamp) <=
                       (device->port[port].cycle.duration + device->port[port].cycle.pauseTime) * device->port[port].cycle.times)
                    {
                        if(((time - device->port[port].cycle.start_at_timestamp) %
                            (device->port[port].cycle.duration + device->port[port].cycle.pauseTime)) <=
                            device->port[port].cycle.duration)
                        {
                            state = ON;
                        }
                        else
                        {
                            state = OFF;
                        }
                    }
                }
                else if(time > device->port[port].cycle1.start_at_timestamp)
                {
                    if((time - device->port[port].cycle1.start_at_timestamp) <=
                       (device->port[port].cycle1.duration + device->port[port].cycle1.pauseTime) * device->port[port].cycle1.times)
                    {
                        if(((time - device->port[port].cycle1.start_at_timestamp) %
                            (device->port[port].cycle1.duration + device->port[port].cycle1.pauseTime)) <=
                            device->port[port].cycle1.duration)
                        {
                            state = ON;
                        }
                        else
                        {
                            state = OFF;
                        }
                    }
                }
                else
                {
                    state = OFF;
                }
            }
            else if(BY_SCHEDULE == device->port[port].mode)//定时器模式
            {
                for(item = 0; item < TIMER_GROUP; item++)//该功能待测试
                {
                    //选择处于第几组定时器
                    if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second > device->port[port].timer[item].on_at * 60)
                    {
                        //小于持续时间
                        if(sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second <= (device->port[port].timer[item].on_at *60
                                + device->port[port].timer[item].duration) )
                        {
                            state = device->port[port].timer[item].en;
                            break;
                        }
                    }
                    else
                    {
                        //1.判断如果存在跨天的话
                        if((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration) >
                            24 * 60 * 60)
                        {
                            //如果当前时间处于跨天的时间
                            if((sys_time.hour * 60 * 60 + sys_time.minute * 60 + sys_time.second) <
                                    ((device->port[port].timer[item].on_at *60 + device->port[port].timer[item].duration)- 24 * 60 * 60))
                            {
                                state = device->port[port].timer[item].en;
                                break;
                            }
                        }
                    }
                }

                if(item == TIMER_GROUP)
                {
                    state = OFF;
                }
                else
                {
                    time_t nowTime = getTimeStamp();
                    u8 today = getTimeStampByDate(&nowTime)->tm_wday;
                    int day = 0;
                    for(day = 0; day < 7; day++)
                    {
                       if(today == (day + 1))
                       {
                           if((device->port[port].weekDayEn >> day) & 0x01)
                           {
                               break;
                           }
                       }
                    }

                    if(7 == day)
                    {
                        state = OFF;
                    }
                }
            }
        }
    }

    if(OFF == state)
    {
        return OFF;
    }
    else
    {
        return ON;
    }
}

void pumpProgram(type_monitor_t *monitor, sys_tank_t *tank_list, type_uart_class deviceUart)//未完待续
{
    u8          addr                    = 0;
    u8          port                    = 0;
    u8          sensor_index            = 0;
    u8          valve_index             = 0;
    u8          valve_index1            = 0;
    u8          tank                    = 0;
    u16         ph                      = 0;
    u16         ec                      = 0;
    u16         wl                      = 0;
    u8          type                    = 0;
    u8          port1                   = 0;
    device_t    *device                 = RT_NULL;
    static u8   waterState[TANK_LIST_MAX] = {NO_ADD_WATER,NO_ADD_WATER,NO_ADD_WATER,NO_ADD_WATER};

    for(tank = 0; tank < tank_list->tank_size; tank++)
    {
        //1.如果子阀的类型被修改了之后需要删除
        for(u8 item = 0; item < VALVE_MAX; item++)
        {
            if(tank_list->tank[tank].valve[item] > 0xFF)
            {
                port1 = tank_list->tank[tank].valve[item];
                device = GetDeviceByAddr(monitor, tank_list->tank[tank].valve[item] >> 8);
                if(RT_NULL != device)
                {
                    type = device->port[port1].type;
                }
            }
            else
            {
                device = GetDeviceByAddr(monitor, tank_list->tank[tank].valve[item]);
                if(RT_NULL != device)
                {
                    type = device->port[0].type;
                }
            }

            if(VALVE_TYPE != type)
            {
                if(item == VALVE_MAX - 1)
                {
                    GetSysTank()->tank[tank].valve[item] = 0;
                }
                else
                {
                    for(;item < VALVE_MAX - 1; item++)
                    {
                        GetSysTank()->tank[tank].valve[item] =
                                GetSysTank()->tank[tank].valve[item + 1];
                    }
                }
            }
        }
        //2.如果灌溉水阀类型变化，删除
        if(tank_list->tank[tank].autoFillValveId > 0xFF)
        {
            port1 = tank_list->tank[tank].autoFillValveId;
            if(RT_NULL != GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId >> 8))
            {
                type = GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId >> 8)->port[port1].type;
            }
        }
        else
        {
            if(RT_NULL != GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId))
            {
                type = GetDeviceByAddr(monitor, tank_list->tank[tank].autoFillValveId)->port[0].type;
            }
        }

        if(VALVE_TYPE != type)
        {
            tank_list->tank[tank].autoFillValveId = 0;
        }

        for(sensor_index = 0; sensor_index < monitor->sensor_size; sensor_index++)
        {
            for(u8 item = 0; item < TANK_SENSOR_MAX; item++)
            {
                //只管桶内的ph ec wl
                if(tank_list->tank[tank].sensorId[0][item] == monitor->sensor[sensor_index].addr)
                {
                    for(u8 stor = 0; stor < monitor->sensor[sensor_index].storage_size; stor++)
                    {
                        if(F_S_PH == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            ph = getSensorDataByAddr(monitor, monitor->sensor[sensor_index].addr, stor);
                        }
                        else if(F_S_EC == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            ec = getSensorDataByAddr(monitor, monitor->sensor[sensor_index].addr, stor);
                        }
                        else if(F_S_WL == monitor->sensor[sensor_index].__stora[stor].func)
                        {
                            wl = getSensorDataByAddr(monitor, monitor->sensor[sensor_index].addr, stor);
                        }
                    }
                }
            }
        }

        //1.判断是否需要补水
        if(wl < tank_list->tank[tank].autoFillHeight)
        {
            waterState[tank] = ADD_WATER;
        }
        else
        {
            if(wl > tank_list->tank[tank].autoFillFulfilHeight)
            {
                //如果高过目标水位则关闭
                waterState[tank] = NO_ADD_WATER;
            }
        }

        if(tank_list->tank[tank].autoFillValveId > 0xFF)
        {
            addr = tank_list->tank[tank].autoFillValveId >> 8;
            port = tank_list->tank[tank].autoFillValveId;
        }
        else
        {
            addr = tank_list->tank[tank].autoFillValveId;
            port = 0;
        }

        if(ADD_WATER == waterState[tank])
        {
            deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, ON);

        }
        else if(NO_ADD_WATER == waterState[tank])
        {
            deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, OFF);
        }

        //2.阀门开的条件为: 定时器满足 ph ec 水位满足
        for(valve_index = 0; valve_index < VALVE_MAX; valve_index++)
        {
            u8 state = OFF;
            if(0 != tank_list->tank[tank].valve[valve_index])
            {
                if(tank_list->tank[tank].valve[valve_index] > 0xFF)
                {
                    addr = tank_list->tank[tank].valve[valve_index] >> 8;
                    port = tank_list->tank[tank].valve[valve_index];
                }
                else
                {
                    addr = tank_list->tank[tank].valve[valve_index];
                    port = 0;
                }

                state = pumpDoing(addr, port);

                if(((wl < tank_list->tank[tank].autoFillHeight) && (ON != tank_list->tank[tank].wlMonitorOnly)) ||
                   ((ph < tank_list->tank[tank].lowPhProtection) && (ON != tank_list->tank[tank].phMonitorOnly)) ||
                   ((ph > tank_list->tank[tank].highPhProtection) && (ON != tank_list->tank[tank].phMonitorOnly)) ||
                   ((ec > tank_list->tank[tank].highEcProtection) && (ON != tank_list->tank[tank].ecMonitorOnly)))
                {
                    state = OFF;
                }

                deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, state);
            }
        }

        //3.如果水泵没有关联的阀的话 以水泵的定时器为主
        for(valve_index = 0; valve_index < VALVE_MAX; valve_index++)
        {
            if(0 != tank_list->tank[tank].valve[valve_index])
            {
                break;
            }
        }

        if(valve_index == VALVE_MAX)
        {
            u8 state = OFF;

            if(tank_list->tank[tank].pumpId > 0xFF)
            {
                addr = tank_list->tank[tank].pumpId >> 8;
                port = tank_list->tank[tank].pumpId;
            }
            else
            {
                addr = tank_list->tank[tank].pumpId;
                port = 0;
            }

            state = pumpDoing(addr, port);

            if(((wl < tank_list->tank[tank].autoFillHeight) && (ON != tank_list->tank[tank].wlMonitorOnly)) ||
               ((ph < tank_list->tank[tank].lowPhProtection) && (ON != tank_list->tank[tank].phMonitorOnly)) ||
               ((ph > tank_list->tank[tank].highPhProtection) && (ON != tank_list->tank[tank].phMonitorOnly)) ||
               ((ec > tank_list->tank[tank].highEcProtection) && (ON != tank_list->tank[tank].ecMonitorOnly)))
            {
                state = OFF;
            }

            deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, state);
        }
        else
        {
            //如果有关联阀的
            //需要添加如果有关联的阀门开则开 所关联的阀门全关则关
            for(valve_index1 = 0; valve_index1 < VALVE_MAX; valve_index1++)
            {
                if(tank_list->tank[tank].valve[valve_index1] > 0xFF)
                {
                    addr = tank_list->tank[tank].valve[valve_index1] >> 8;
                    port = tank_list->tank[tank].valve[valve_index1];
                }
                else
                {
                    addr = tank_list->tank[tank].valve[valve_index1];
                    port = 0;
                }

                if(RT_NULL != GetDeviceByAddr(monitor, addr))
                {
                    if(ON == GetDeviceByAddr(monitor, addr)->port[port].ctrl.d_state)
                    {
                        break;
                    }
                }
            }

            if(tank_list->tank[tank].pumpId > 0xFF)
            {
                addr = tank_list->tank[tank].pumpId >> 8;
                port = tank_list->tank[tank].pumpId;
            }
            else
            {
                addr = tank_list->tank[tank].pumpId;
                port = 0;
            }

            if(VALVE_MAX == valve_index1)
            {
                //所关联的阀门状态为全关,则关闭水泵
                deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, OFF);
            }
            else
            {
                //所关联的阀门状态有开着的，打开水泵
                deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, ON);
            }
        }

        //5.阀门开的条件为: 定时器满足 ph ec 水位满足
        for(int index = 0; index < VALVE_MAX; index++)
        {
            u8 state = OFF;
            if(0 != tank_list->tank[tank].nopump_valve[index])
            {
                if(tank_list->tank[tank].nopump_valve[index] > 0xFF)
                {
                    addr = tank_list->tank[tank].nopump_valve[index] >> 8;
                    port = tank_list->tank[tank].nopump_valve[index];
                }
                else
                {
                    addr = tank_list->tank[tank].nopump_valve[index];
                    port = 0;
                }

                state = pumpDoing(addr, port);

                if(((wl < tank_list->tank[tank].autoFillHeight) && (ON != tank_list->tank[tank].wlMonitorOnly)) ||
                   ((ph < tank_list->tank[tank].lowPhProtection) && (ON != tank_list->tank[tank].phMonitorOnly)) ||
                   ((ph > tank_list->tank[tank].highPhProtection) && (ON != tank_list->tank[tank].phMonitorOnly)) ||
                   ((ec > tank_list->tank[tank].highEcProtection) && (ON != tank_list->tank[tank].ecMonitorOnly)))
                {
                    state = OFF;
                }

                deviceUart.DeviceCtrlSingle(GetDeviceByAddr(monitor, addr), port, state);
            }
        }
    }
}


void setPhCalWithUUID(u32 uuid)
{
    u8 index = 0;

    for(index = 0; index < SENSOR_MAX; index++)
    {
        if(ph_cal[index].uuid == uuid)
        {
            break;
        }
    }

    if(index == SENSOR_MAX)
    {
        //当前没有存储该uuid
        for(index = 0; index < SENSOR_MAX; index++)
        {
            if(0 == ph_cal[index].uuid)
            {
                ph_cal[index].uuid = uuid;
                break;
            }
        }
    }
}

void setEcCalWithUUID(u32 uuid)
{
    u8 index = 0;

    for(index = 0; index < SENSOR_MAX; index++)
    {
        if(ec_cal[index].uuid == uuid)
        {
            break;
        }
    }

    if(index == SENSOR_MAX)
    {
        //当前没有存储该uuid
        for(index = 0; index < SENSOR_MAX; index++)
        {
            if(0 == ec_cal[index].uuid)
            {
                ec_cal[index].uuid = uuid;
                break;
            }
        }
    }
}

ph_cal_t *getPhCalByuuid(u32 uuid)
{
    for(int i = 0; i < SENSOR_MAX; i++)
    {
        if(uuid == ph_cal[i].uuid)
        {
            return &ph_cal[i];
        }
    }

    return RT_NULL;
}

ec_cal_t *getEcCalByuuid(u32 uuid)
{
    for(int i = 0; i < SENSOR_MAX; i++)
    {
        if(uuid == ec_cal[i].uuid)
        {
            return &ec_cal[i];
        }
    }

    return RT_NULL;
}

u8 isBelongToTank(u16 id, sys_tank_t *list)
{
    int     i   = 0;
    int     j   = 0;

    for(i = 0; i < list->tank_size; i++)
    {
        if(id == list->tank[i].autoFillValveId)
        {
            return YES;
        }
        else if(id == list->tank[i].pumpId)
        {
            return YES;
        }
        else
        {
            for(j = 0; j < VALVE_MAX; j++)
            {
                if(id == list->tank[i].valve[j])
                {
                    return YES;
                }
                else if(id == list->tank[i].nopump_valve[j])
                {
                    return YES;
                }
            }
        }
    }

    return NO;
}

//关闭无用的设备
void closeUnUseDevice(type_monitor_t *monitor, type_uart_class *uart)
{
    int index = 0;

    for(index = 0; index < monitor->device_size; index++)
    {
        if(1 == monitor->device[index].storage_size)
        {
            if(NO == isBelongToTank(monitor->device[index].addr, GetSysTank()))
            {
                uart->DeviceCtrlSingle(&monitor->device[index], 0, OFF);
            }
        }
        else
        {
            for(int port = 0; port < monitor->device[index].storage_size; port++)
            {
                if(NO == isBelongToTank((monitor->device[index].addr << 8) | port, GetSysTank()))
                {
                    uart->DeviceCtrlSingle(&monitor->device[index], port, OFF);
                }
            }
        }
    }
}

#endif

