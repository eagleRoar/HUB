/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-31     Administrator       the first version
 */

#include <u8g2_port.h>
#include "Oled1309.h"
#include "OledBusiness.h"
#include "Uart.h"
#include "UartBussiness.h"
#include "Module.h"

#include "ascii_fonts.h"
#include "qrcode.h"
#include "ST7567.h"
#include "SDCard.h"

char    data[80];

extern u8      next_flag;
extern void GetUpdataFileFromWeb(u8 *ret);
extern  __attribute__((section(".ccmbss"))) struct sdCardState      sdCard;
extern type_page_t     pageSelect;

void HomePage(type_page_t *page, type_monitor_t *monitor)
{
    char                time[22]    = "";
#if (HUB_SELECT == HUB_ENVIRENMENT)
    char                name[5]     = "";
    int                 data;
#elif (HUB_SELECT == HUB_IRRIGSTION)
    u8                  sensor_i    = 0;
    u8                  addr        = 0;
    u8                  port        = 0;
    static u8           tank_index  = 0;
    tank_t              *tank       = RT_NULL;
    sensor_t            *sensor     = RT_NULL;
    int                 data[3]     = {VALUE_NULL,VALUE_NULL,VALUE_NULL};
#endif
    char                value[5]    = "";
    char                day_night[6] = "";
    type_sys_time       time_for;


    //1.显示时间
    getRealTimeForMat(&time_for);
    if(TIME_STYLE_12H == GetSysSet()->sysPara.timeFormat)
    {
        if((time_for.hour > 12) || (12 == time_for.hour && time_for.second > 0))
        {
            time_for.hour -= 12;
        }
    }

    if(DAY_TIME == GetSysSet()->dayOrNight)
    {
        rt_memcpy(day_night, "Day", 3);
        day_night[3] = '\0';
    }
    else if(NIGHT_TIME == GetSysSet()->dayOrNight)
    {
        rt_memcpy(day_night, "Night", 5);
        day_night[5] = '\0';
    }

    if(NIGHT_TIME == GetSysSet()->dayOrNight)
    {
        sprintf(time," %02d:%02d:%02d %02d/%02d %5s ",time_for.hour,time_for.minute,time_for.second,
            time_for.month,time_for.day,day_night);
    }
    else if(DAY_TIME == GetSysSet()->dayOrNight)
    {
        sprintf(time," %02d:%02d:%02d %02d/%02d %3s   ",time_for.hour,time_for.minute,time_for.second,
            time_for.month,time_for.day,day_night);
    }

    time[21] = '\0';
    ST7567_GotoXY(0, 0);
    ST7567_Puts(time, &Font_6x12, 0);

#if (HUB_SELECT == HUB_ENVIRENMENT)
    //2.显示TEMP HUMI CO2
    rt_memcpy(name, "Temp", 4);
    name[4] = '\0';
    ST7567_GotoXY(8, 16);
    ST7567_Puts(name, &Font_6x12, 1);

    data = getSensorDataByFunc(monitor, F_S_TEMP);
    if((VALUE_NULL == data) || (YES != sdCard.readInfo))
    {
        ST7567_GotoXY(0, 32);
        ST7567_Puts("    ", &Font_8x16, 1);
        rt_memcpy(value, "--", 2);
        ST7567_GotoXY(8, 32);
        ST7567_Puts(value, &Font_8x16, 1);
    }
    else
    {
        rt_memset(value, ' ', 4);
        sprintf(value, "%d",data);
        value[3] = '\0';
        ST7567_GotoXY(0, 32);
        ST7567_Puts(value, &Font_11x18, 1);
        ST7567_DrawFilledCircle(21, 46, 1, 1);
    }
    //显示单位
    ST7567_DrawCircle(8*2, 54, 1, 1);
    ST7567_GotoXY(8*2+2, 52);
    ST7567_Puts("C", &Font_6x12, 1);

    ST7567_DrawLine(8 + 8*4 + 2, 22, 8 + 8*4 + 2, 56, 1);

    rt_memcpy(name, "Humi", 4);
    name[4] = '\0';
    ST7567_GotoXY(8 + 8*5, 16);
    ST7567_Puts(name, &Font_6x12, 1);

    data = getSensorDataByFunc(monitor, F_S_HUMI);
    if((VALUE_NULL == data) || (YES != sdCard.readInfo))
    {
        ST7567_GotoXY(8 + 8*5, 32);
        ST7567_Puts("    ", &Font_8x16, 1);
        rt_memcpy(value, "--", 2);
        ST7567_GotoXY(8 + 8*5, 32);
        ST7567_Puts(value, &Font_8x16, 1);
    }
    else
    {
        sprintf(value, "%d",data);
        value[3] = '\0';
        ST7567_GotoXY(8 + 8*5, 32);
        ST7567_Puts(value, &Font_11x18, 1);
        ST7567_DrawFilledCircle(8 + 8*5 + 2*11 - 1, 46, 1, 1);
    }

    //显示单位
    ST7567_GotoXY(8 + 8*6, 52);
    ST7567_Puts("%", &Font_6x12, 1);

    ST7567_DrawLine(8 + 8*9 + 2, 22, 8 + 8*9 + 2, 56, 1);

    rt_memcpy(name, "CO2", 3);
    name[3] = '\0';
    ST7567_GotoXY(8 + 8*11, 16);
    ST7567_Puts(name, &Font_6x12, 1);

    data = getSensorDataByFunc(monitor, F_S_CO2);
    if((VALUE_NULL == data) || (YES != sdCard.readInfo))
    {
        ST7567_GotoXY(8 + 8*9 + 4, 32);
        ST7567_Puts("      ", &Font_8x16, 1);
        rt_memcpy(value, "--", 2);
        ST7567_GotoXY(8 + 8*11, 32);
        ST7567_Puts(value, &Font_8x16, 1);
    }
    else
    {
        sprintf(value, "%4d",data);
        value[4] = '\0';
        ST7567_GotoXY(8 + 8*9 + 4, 32);
        ST7567_Puts(value, &Font_11x18, 1);
    }

    //显示单位
    ST7567_GotoXY(8 + 8*11, 52);
    ST7567_Puts("ppm", &Font_6x12, 1);
#elif (HUB_SELECT == HUB_IRRIGSTION)
    //显示桶内的sensor
//    if(0 == GetSysTank()->tank_size)
//    {
//        ST7567_GotoXY(30, 16);
//        ST7567_Puts("BLH-I", &Font_12x24, 1);
//    }
//    else
    {
        //1.显示PH EC WT
        if(YES == next_flag)
        {
            if(tank_index + 1 < GetSysTank()->tank_size)
            {
                tank_index++;
            }
            else
            {
                tank_index = 0;
            }

            //清除屏幕缓存
            ST7567_GotoXY(4, 28);
            ST7567_Puts("    ", &Font_8x16, 1);
            ST7567_GotoXY(47, 28);
            ST7567_Puts("    ", &Font_8x16, 1);
            ST7567_GotoXY(90, 28);
            ST7567_Puts("    ", &Font_8x16, 1);
            next_flag = NO;
        }

        {
            tank = &GetSysTank()->tank[tank_index];
            for(sensor_i = 0; sensor_i < TANK_SENSOR_MAX; sensor_i++)
            {
                if(tank->sensorId[0][sensor_i] > 0xFF)
                {
                    addr = tank->sensorId[0][sensor_i] >> 8;
                    port = tank->sensorId[0][sensor_i];
                }
                else
                {
                    addr = tank->sensorId[0][sensor_i];
                    port = 0;
                }

                sensor = GetSensorByAddr(GetMonitor(), addr);

                ST7567_GotoXY(15, 16);
                ST7567_Puts("PH", &Font_6x12, 1);
                ST7567_GotoXY(58, 16);
                ST7567_Puts("EC", &Font_6x12, 1);
                ST7567_GotoXY(100, 16);
                ST7567_Puts("WT", &Font_6x12, 1);

                if(RT_NULL != sensor)
                {
                    if(CON_FAIL != sensor->conn_state)
                    {
                        if (PHEC_TYPE == sensor->type)
                        {
                            //data[0] = sensor->__stora[1].value;
                            data[0] = getSensorDataByAddr(GetMonitor(), addr, 1);
                            //data[1] = sensor->__stora[0].value;
                            data[1] = getSensorDataByAddr(GetMonitor(), addr, 0);
                            //data[2] = sensor->__stora[2].value;
                            data[2] = getSensorDataByAddr(GetMonitor(), addr, 2);

                        }
                    }
                    else
                    {
                        //清除屏幕缓存
                        ST7567_GotoXY(4, 28);
                        ST7567_Puts("    ", &Font_8x16, 1);
                        ST7567_GotoXY(47, 28);
                        ST7567_Puts("    ", &Font_8x16, 1);
                        ST7567_GotoXY(90, 28);
                        ST7567_Puts("    ", &Font_8x16, 1);

                        ST7567_GotoXY(12, 28);
                        ST7567_Puts("--", &Font_8x16, 1);
                        ST7567_GotoXY(55, 28);
                        ST7567_Puts("--", &Font_8x16, 1);
                        ST7567_GotoXY(98, 28);
                        ST7567_Puts("--", &Font_8x16, 1);
                    }
                }
            }

            if(VALUE_NULL != data[0])
            {
                ST7567_GotoXY(4, 28);
                rt_memcpy(value, "   ", 3);
                if(data[0]/10 > 10)
                {
                    sprintf(value, "%3d", data[0]/10);
                }
                else
                {
                    sprintf(value, " %02d", data[0]/10);
                }
                value[3] = '\0';
                ST7567_Puts(value, &Font_11x18, 1);
                ST7567_DrawFilledCircle(25, 42, 1, 1);
            }
            else
            {
                ST7567_GotoXY(12, 28);
                ST7567_Puts("--", &Font_8x16, 1);
            }

            if(VALUE_NULL != data[1])
            {
                ST7567_GotoXY(47, 28);
                rt_memcpy(value, "   ", 3);
                if(data[1]/10 > 10)
                {
                    sprintf(value, "%3d", data[1]/10);
                }
                else
                {
                    sprintf(value, " %02d", data[1]/10);
                }
                value[3] = '\0';
                ST7567_Puts(value, &Font_11x18, 1);
                ST7567_DrawFilledCircle(68, 42, 1, 1);
            }
            else
            {
                ST7567_GotoXY(55, 28);
                ST7567_Puts("--", &Font_8x16, 1);
            }

            //显示单位
            ST7567_GotoXY(51, 46);
            ST7567_Puts("ms/cm", &Font_6x12, 1);

            if(VALUE_NULL != data[2])
            {
                ST7567_GotoXY(90, 28);
                rt_memcpy(value, "   ", 3);
                if(data[2] > 10)
                {
                    sprintf(value, "%3d", data[2]);
                }
                else
                {
                    sprintf(value, " %02d", data[2]);
                }
                value[3] = '\0';
                ST7567_Puts(value, &Font_11x18, 1);
                ST7567_DrawFilledCircle(111, 42, 1, 1);
            }
            else
            {
                ST7567_GotoXY(98, 28);
                ST7567_Puts("--", &Font_8x16, 1);
            }

            //显示单位
            ST7567_DrawCircle(100, 48, 1, 1);
            ST7567_GotoXY(102, 46);
            ST7567_Puts("C", &Font_6x12, 1);

            ST7567_DrawLine(43, 22, 43, 56, 1);
            ST7567_DrawLine(86, 22, 86, 56, 1);
        }

        ST7567_DrawFilledTriangle(62, 60, 66, 60, 64, 62, 1);
    }
#endif
    //刷新界面
    ST7567_UpdateScreen();
}

void SettingPage(type_page_t page, u8 canShow)
{
    char                time[22]    = "";
    char                temp[2];
    char                temp3;
    char                temp1[4];
    type_sys_time       time_for;
#if(HUB_SELECT == HUB_ENVIRENMENT)
    char                show[6][16] = {"Sensor List", "Device List", "Light List", "QR Code", "Update Firmware", "CO2 Calibration"};
#elif (HUB_SELECT == HUB_IRRIGSTION)
    char                show[4][16] = {"Device List", "QR Code", "Update App", "PHEC Calibrate"};
#endif
    type_sys_time       sys_time;
    u8                  line        = LINE_HIGHT;
    u8                  column      = 0;

    //1.显示时间
    if(TIME_STYLE_12H == GetSysSet()->sysPara.timeFormat)
    {
        getRealTimeForMat(&sys_time);
        if(sys_time.hour > 12)
        {
            itoa(sys_time.hour - 12, temp, 10);

            if(sys_time.hour < 20)
            {
                temp[1] = temp[0];
                temp[0] = '0';
            }
        }

        strncat(time, temp, 2);
        temp3 = ':';
        strncat(time, &temp3, 1);
        itoa(sys_time.minute, temp, 10);
        if(sys_time.minute < 10)
        {
            temp[1] = temp[0];
            temp[0] = '0';
        }
        strncat(time, temp, 2);
        temp3 = ':';
        strncat(time, &temp3, 1);
        itoa(sys_time.second, temp, 10);
        if(sys_time.second < 10)
        {
            temp[1] = temp[0];
            temp[0] = '0';
        }
        strncat(time, temp, 2);


        if((sys_time.hour >= 12))
        {
            strcpy(temp, "PM");
            strncat(time, temp, 2);
        }
        else
        {
            strcpy(temp, "AM");
            strncat(time, temp, 2);
        }

        itoa(sys_time.year, temp1, 10);
        strncat(time, temp1, 4);
        strcpy(data, time);

        column = 0;
        ST7567_GotoXY(0, column);
        ST7567_Puts(data, &Font_6x12, 0);
    }
    else
    {
        getRealTimeForMat(&time_for);
        sprintf(time," %02d:%02d:%02d %02d/%02d      ",time_for.hour,time_for.minute,time_for.second,
                time_for.month,time_for.day);
        time[21] = '\0';
        column = 0;
        ST7567_GotoXY(0, column);
        ST7567_Puts(time, &Font_6x12, 0);
    }

    //循环列表选择
    column = 16;
    if(page.cusor <= canShow)
    {
        for(u8 index = 0; index < canShow; index++)
        {
            ST7567_GotoXY(line, column + index * 16);
            //显示选中
            if(page.cusor == index + 1)
            {
                ST7567_Puts(show[index], &Font_8x16, 0);
            }
            else
            {
                ST7567_Puts(show[index], &Font_8x16, 1);
            }
        }
    }
    else
    {
        for(u8 index = 0; index < canShow; index++)
        {
            ST7567_GotoXY(line, column + index * 16);
            //显示选中
            if(page.cusor == index + 1 + page.cusor - canShow)
            {
                ST7567_Puts(show[index + page.cusor - canShow], &Font_8x16, 0);
            }
            else
            {
                ST7567_Puts(show[index + page.cusor - canShow], &Font_8x16, 1);
            }
        }
    }

    ST7567_UpdateScreen();
}

void SensorStatePage_new(type_monitor_t *monitor)
{
#define     SHOWNUM     5
    u8              line        = LINE_HIGHT;
    u8              column      = 0;
    char            name[5]     = " ";
    char            name1[16]   = " ";
    float           num;
    int             data;
    sys_set_t       *set        = GetSysSet();
    static u8       showInde    = 0;
    static u8       showList[SHOWNUM] = {F_S_CO2,F_S_TEMP,F_S_HUMI,F_S_LIGHT,F_S_PAR};

    //1.清除界面
    clear_screen();

    //2.轮流显示当前的sensor
    if(monitor->sensor_size > 0)
    {
        //2.1 先显示固定位置的名称
        if(F_S_LIGHT != showList[showInde])
        {
            ST7567_GotoXY(LINE_HIGHT, 0);
        }

        if(F_S_CO2 == showList[showInde])
        {
            strcpy(name, "CO2");
        }
        else if(F_S_TEMP == showList[showInde])
        {
            strcpy(name, "Temp");
        }
        else if(F_S_HUMI == showList[showInde])
        {
            strcpy(name, "Humi");
        }
        else if(F_S_PAR == showList[showInde])
        {
            strcpy(name, "PAR");
        }
        name[4] = '\0';

        if(F_S_LIGHT != showList[showInde])
        {
            ST7567_Puts(name, &Font_12x24, 1);
        }

        line = LINE_HIGHT + 4 * 12 + 8;
        column = 24 - 16;
        ST7567_GotoXY(line, column);
        if(F_S_HUMI == showList[showInde] ||
           F_S_TEMP == showList[showInde])
        {

            data = getSensorDataByFunc(monitor, showList[showInde]);
            if(VALUE_NULL == data)
            {
                strcpy(name, "--");
            }
            else
            {
                num = data;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
        }
        else
        {
            data = getSensorDataByFunc(monitor, showList[showInde]);
            if(VALUE_NULL == data)
            {
                strcpy(name, "--");
            }
            else
            {
                sprintf(name, "%d", data);
            }
        }
        name[4] = '\0';

        if(F_S_LIGHT != showList[showInde])
        {
            ST7567_Puts(name, &Font_8x16, 1);
        }
        else
        {
            ST7567_GotoXY(LINE_HIGHT, 0);
            ST7567_Puts("Photocell", &Font_8x16, 1);
            ST7567_GotoXY(LINE_HIGHT, 16);
            if(VALUE_NULL == data)
            {
                ST7567_Puts("Intensity --", &Font_8x16, 1);
            }
            else
            {
                sprintf(name1, "%s %d","Intensity",data);
                ST7567_Puts(name1, &Font_8x16, 1);
            }
        }

        //2.2 显示单位
        if(F_S_CO2 == showList[showInde])
        {
            //显示单位
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("ppm", &Font_8x16, 1);

            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("ppm", &Font_6x12, 1);

            //显示目标值
            line = LINE_HIGHT;
            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("CO2   tgt:", &Font_6x12, 1);

            line = LINE_HIGHT + /*4 * 12 + 8*/6 * 10;
            ST7567_GotoXY(line, column);
            if(DAY_TIME == set->dayOrNight)
            {
                num = set->co2Set.dayCo2Target;
                sprintf(name, "%.1f", num);
            }
            else
            {
                num = set->co2Set.nightCo2Target;
                sprintf(name, "%.1f", num);
            }
            name[4] = '\0';
            ST7567_Puts(name, &Font_6x12, 1);
        }
        else if(F_S_HUMI == showList[showInde])
        {
            //显示单位
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("%", &Font_8x16, 1);

            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("%", &Font_6x12, 1);

            column += 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("%", &Font_6x12, 1);

            //显示目标值
            line = LINE_HIGHT;
            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("Humi  tgt:", &Font_6x12, 1);

            line = LINE_HIGHT + 6 * 10;
            ST7567_GotoXY(line, column);
            if(DAY_TIME == set->dayOrNight)
            {
                num = set->humiSet.dayHumiTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            else
            {
                num = set->humiSet.nightHumiTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            name[4] = '\0';
            ST7567_Puts(name, &Font_6x12, 1);

            line = LINE_HIGHT;
            column = 48;
            ST7567_GotoXY(line, column);
            ST7567_Puts("Dehum tgt:", &Font_6x12, 1);

            line = LINE_HIGHT + 6 * 10;
            ST7567_GotoXY(line, column);
            if(DAY_TIME == set->dayOrNight)
            {
                num = set->humiSet.dayDehumiTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            else
            {
                num = set->humiSet.nightDehumiTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            name[4] = '\0';
            ST7567_Puts(name, &Font_6x12, 1);
        }
        else if(F_S_TEMP == showList[showInde])
        {
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_DrawCircle(line, column, 1, 1);
            ST7567_GotoXY(line + 3, column);
            ST7567_Puts("C", &Font_8x16, 1);

            column = 32;
            ST7567_DrawCircle(line, column, 1, 1);
            ST7567_GotoXY(line + 3, column);
            ST7567_Puts("C", &Font_6x12, 1);

            column += 16;
            ST7567_DrawCircle(line, column, 1, 1);
            ST7567_GotoXY(line + 3, column);
            ST7567_Puts("C", &Font_6x12, 1);

            //显示目标值
            line = LINE_HIGHT;
            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("Heat  tgt:", &Font_6x12, 1);

            line = LINE_HIGHT + 6 * 10;
            ST7567_GotoXY(line, column);
            if(DAY_TIME == set->dayOrNight)
            {
                num = set->tempSet.dayHeatingTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            else
            {
                num = set->tempSet.nightHeatingTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            name[4] = '\0';
            ST7567_Puts(name, &Font_6x12, 1);

            line = LINE_HIGHT;
            column = 48;
            ST7567_GotoXY(line, column);
            ST7567_Puts("Cool  tgt:", &Font_6x12, 1);

            line = LINE_HIGHT + 6 * 10;
            ST7567_GotoXY(line, column);
            if(DAY_TIME == set->dayOrNight)
            {
                num = set->tempSet.dayCoolingTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            else
            {
                num = set->tempSet.nightCoolingTarget;
                num /= 10;
                sprintf(name, "%.1f", num);
            }
            name[4] = '\0';
            ST7567_Puts(name, &Font_6x12, 1);
        }
        else if(F_S_PAR == showList[showInde])
        {
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("ppfd", &Font_8x16, 1);
        }
    }

    if(showInde < SHOWNUM - 1)
    {
        showInde++;
    }
    else
    {
        showInde = 0;
    }

    ST7567_UpdateScreen();
}

void DeviceStatePage_new(type_monitor_t *monitor)
{
#define     CAN_SHOW        4

    u8              line        = LINE_HIGHT;
    u8              column      = 0;
    char            name[9]     = " ";
    u8              num         = 0;
    u8              showNum     = monitor->device_size;
    static u8       showInde    = CAN_SHOW - 1;//4表示一个界面显示4行
    static u8       start       = 0;

    //1.清除界面
    clear_screen();

    //2.显示状态
    if(YES != sdCard.readInfo)
    {
        ST7567_GotoXY(LINE_HIGHT, 0);
        ST7567_Puts("SD is not init", &Font_8x16, 0);
    }
    else
    {
        if(showNum < CAN_SHOW)
        {
            num = showNum;
        }
        else
        {
            num = CAN_SHOW;
        }

        if(0 == num)
        {
            ST7567_GotoXY(LINE_HIGHT, 0);
            ST7567_Puts("None", &Font_12x24, 0);
        }
        else
        {
            for(u8 index = 0; index < num; index++)
            {
                line = LINE_HIGHT;
                column = index * 16;
                if(CON_FAIL == monitor->device[index + start].conn_state)
                {
                    ST7567_GotoXY(line, column);
                    rt_memcpy(name, monitor->device[index + start].name, 8);
                    name[8] = '\0';
                    ST7567_Puts(name, &Font_8x16, 1);

                    line = 8 * 8 + 8;
                    ST7567_GotoXY(line, column);
                    ST7567_Puts("offline", &Font_8x16, 0);
                }
                else
                {
                    ST7567_GotoXY(line, column);
                    rt_memcpy(name, monitor->device[index + start].name, 8);
                    name[8] = '\0';
                    ST7567_Puts(name, &Font_8x16, 1);

                    line = 8 * 8 + 8;
                    ST7567_GotoXY(line, column);
                    ST7567_Puts("online", &Font_8x16, 1);
                }
            }
        }

        //3.计算循环播放
        if(showNum > CAN_SHOW)
        {
            if(showInde + 1 < showNum)
            {
                showInde++;
                start = showInde + 1 - CAN_SHOW;
            }
            else
            {
                showInde = CAN_SHOW - 1;
                start = 0;
            }
        }
        else
        {
            start = 0;
            showInde = CAN_SHOW - 1;
        }
    }

    //4.刷新界面
    ST7567_UpdateScreen();
}

void LineStatePage_new(type_monitor_t *monitor)
{
#define     CAN_SHOW        4

    u8              line        = LINE_HIGHT;
    u8              column      = 0;
    char            name[9]     = " ";
    u8              num         = 0;
    u8              showNum     = monitor->line_size;
    static u8       showInde    = CAN_SHOW - 1;//4表示一个界面显示4行
    static u8       start       = 0;

    //1.清除界面
    clear_screen();

    if(YES != sdCard.readInfo)
    {
        ST7567_GotoXY(LINE_HIGHT, 0);
        ST7567_Puts("SD is not init", &Font_8x16, 0);
    }
    else
    {
        //2.显示状态
        if(showNum < CAN_SHOW)
        {
            num = showNum;
        }
        else
        {
            num = CAN_SHOW;
        }

        if(0 == num)
        {
            ST7567_GotoXY(LINE_HIGHT, 0);
            ST7567_Puts("None", &Font_12x24, 0);
        }
        else
        {
            for(u8 index = 0; index < num; index++)
            {
                line = LINE_HIGHT;
                column = index * 16;
                if(CON_FAIL == monitor->line[index + start].conn_state)
                {
                    ST7567_GotoXY(line, column);
                    rt_memcpy(name, monitor->line[index + start].name, 8);
                    name[8] = '\0';
                    ST7567_Puts(name, &Font_8x16, 1);

                    line = 8 * 8 + 8;
                    ST7567_GotoXY(line, column);
                    ST7567_Puts("offline", &Font_8x16, 0);
                }
                else
                {
                    ST7567_GotoXY(line, column);
                    rt_memcpy(name, monitor->line[index + start].name, 8);
                    name[8] = '\0';
                    ST7567_Puts(name, &Font_8x16, 1);

                    line = 8 * 8 + 8;
                    ST7567_GotoXY(line, column);
                    ST7567_Puts("online", &Font_8x16, 1);
                }
            }
        }

        //3.计算循环播放
        if(showNum > CAN_SHOW)
        {
            if(showInde + 1 < showNum)
            {
                showInde++;
                start = showInde + 1 - CAN_SHOW;
            }
            else
            {
                showInde = CAN_SHOW - 1;
                start = 0;
            }
        }
        else
        {
            start = 0;
            showInde = CAN_SHOW - 1;
        }
    }
    //4.刷新界面
    ST7567_UpdateScreen();
}

void qrcode(void)
{
#define DEFAULT_QR_VERSION 6

    QRCode qrc;
    uint8_t x, y, *qrcodeBytes = (uint8_t *)rt_calloc(1, qrcode_getBufferSize(DEFAULT_QR_VERSION));
    int8_t result;
    char qrstr[13] = " ";

    rt_memset(qrstr, ' ', 13);
    GetSnName(qrstr, 12);
    LOG_D("sn = %s",qrstr);

    if (qrcodeBytes)
    {
        result = qrcode_initText(&qrc, qrcodeBytes, DEFAULT_QR_VERSION, ECC_LOW, qrstr);

        if (result >= 0)
        {
            rt_kprintf("\n");
            for (y = 0; y < qrc.size; y++)
            {
                for (x = 0; x < qrc.size; x++)
                {
                    if (qrcode_getModule(&qrc, x, y))
                    {
                        ST7567_DrawLine(x, y, x, y, 1);
                    }
                    else
                    {
                    }
                }
            }
        }
        else
        {
            rt_kprintf("QR CODE(%s) General FAILED(%d)\n", qrstr, result);
        }

        LOG_D("qrc.size = %d",qrc.size);

        rt_free(qrcodeBytes);

        ST7567_GotoXY(0, 48);
        ST7567_Puts(qrstr, &Font_8x16, 1);
        ST7567_UpdateScreen();
    }
    else
    {
        rt_kprintf("Warning: no memory!\n");
    }
}

void UpdateAppProgram(type_page_t *page, u32 *info)
{
    u8              line        = LINE_HIGHT;
    u8              column      = 0;
    u8              downloadRes = RT_ERROR;

    //2.提示是否要下载
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts("Update New APP?", &Font_8x16, 1);

    //3.确定是否升级
    line = 28;
    column = 32;
    ST7567_GotoXY(line, column);
    if(1 == page->cusor)
    {
        ST7567_Puts("NO", &Font_12x24, 0);
    }
    else
    {
        ST7567_Puts("NO", &Font_12x24, 1);
    }

    line = 64;
    column = 32;
    ST7567_GotoXY(line, column);
    if(2 == page->cusor)
    {
        ST7567_Puts("YES", &Font_12x24, 0);
    }
    else
    {
        ST7567_Puts("YES", &Font_12x24, 1);
    }

    //4.判断按键确定是否
    if(ON == page->select)
    {
        if(1 == page->cusor)
        {
            //返回上一页
            *info >>= 8;
        }
        else if(2 == page->cusor)
        {
            clear_screen();
            //4.1 显示正在下载
            ST7567_GotoXY(LINE_HIGHT, 0);
            ST7567_Puts("Download now", &Font_8x16, 1);
            ST7567_UpdateScreen();

            GetUpdataFileFromWeb(&downloadRes);

            if(DOWNLOAD_FAIL == downloadRes)
            {
                LOG_I("download fail");
                ST7567_GotoXY(LINE_HIGHT, 0);
                ST7567_Puts("Download fail", &Font_8x16, 1);
                ST7567_UpdateScreen();
            }
            else if(DOWNLOAD_OK == downloadRes)
            {
                LOG_I("download success");
                ST7567_Fill(0);
                ST7567_GotoXY(LINE_HIGHT, 0);
                ST7567_Puts("Download OK", &Font_8x16, 1);
                ST7567_UpdateScreen();
            }
            else if(DOWNLOAD_NONEED == downloadRes)
            {
                LOG_I("Version is Last");
                ST7567_Fill(0);
                ST7567_GotoXY(LINE_HIGHT, 0);
                ST7567_Puts("Version is Last", &Font_8x16, 1);
                ST7567_UpdateScreen();
            }
        }
        page->select = OFF;
    }

    //5.刷新界面
    ST7567_UpdateScreen();
}

void co2CalibratePage(type_page_t *page, u32 *info)
{
    if(NO == GetSysSet()->startCalFlg)
    {
        //1.
        ST7567_GotoXY(LINE_HIGHT, 0);
        ST7567_Puts("Start CO2 Cali-", &Font_8x16, 1);
        ST7567_GotoXY(LINE_HIGHT, 16);
        ST7567_Puts("bration", &Font_8x16, 1);
        //2.
        ST7567_GotoXY(28, 16 * 2);
        ST7567_Puts("No", &Font_12x24, (1 == page->cusor) ? 0 : 1);
        ST7567_GotoXY(64, 16 * 2);
        ST7567_Puts("Yes", &Font_12x24, (2 == page->cusor) ? 0 : 1);

        if(ON == page->select)
        {
            //如果是否的话就返回
            if(1 == page->cusor)
            {
                *info >>= 8;
            }
            else
            {
                GetSysSet()->startCalFlg = YES;
                ST7567_GotoXY(LINE_HIGHT, 0);
                ST7567_Puts("Calibrate now..", &Font_8x16, 1);
                ST7567_GotoXY(LINE_HIGHT, 16);
                ST7567_Puts("                ", &Font_8x16, 1);
                ST7567_GotoXY(LINE_HIGHT, 16*2);
                ST7567_Puts("                ", &Font_8x16, 1);
                ST7567_GotoXY(LINE_HIGHT, 16*3);
                ST7567_Puts("                ", &Font_8x16, 1);
            }

            page->select = OFF;
        }
    }
    else
    {
        ST7567_GotoXY(LINE_HIGHT, 0);
        ST7567_Puts("Calibrate now..", &Font_8x16, 1);
        ST7567_GotoXY(LINE_HIGHT, 16);
        ST7567_Puts("                ", &Font_8x16, 1);
        ST7567_GotoXY(LINE_HIGHT, 16*2);
        ST7567_Puts("                ", &Font_8x16, 1);
        ST7567_GotoXY(LINE_HIGHT, 16*3);
        ST7567_Puts("                ", &Font_8x16, 1);
    }

    //3
    ST7567_UpdateScreen();
}

void PhEcCalibratePage(type_page_t *page)
{

    //1.
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts("PH Calibrate", &Font_8x16, 1 == page->cusor ? 0 : 1);
    ST7567_GotoXY(LINE_HIGHT, 16);
    ST7567_Puts("EC Calibrate", &Font_8x16, 2 == page->cusor ? 0 : 1);

    //2
    ST7567_UpdateScreen();
}

void PhCalibratePage(type_page_t *page, ph_cal_t *ph)
{
    char data[7] = "";
    char data1[7] = "";
    char show[16] = "";

    //1.
    ST7567_GotoXY(LINE_HIGHT, 0);
    if(CAL_INCAL == ph->cal_7_flag)
    {
        strncpy(data, "Cal..." ,7);
    }
    else
    {
        strncpy(data, "OK" ,7);
    }

    if(CAL_INCAL == ph->cal_4_flag)
    {
        strncpy(data1, "Cal..." ,7);
    }
    else
    {
        strncpy(data1, "OK" ,7);
    }

    sprintf(show,"%6s %6s","PH7.0 ",data);
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts(show, &Font_6x12, 1);
    sprintf(show,"%6s %6s","PH4.0 ",data1);
    ST7567_GotoXY(LINE_HIGHT, 32);
    ST7567_Puts(show, &Font_6x12, 1);

    //2.显示按钮
    ST7567_GotoXY(LINE_HIGHT*11, 0);
    ST7567_Puts("Start", &Font_8x16, 1 == page->cusor ? 0 : 1);
    ST7567_GotoXY(LINE_HIGHT*11, 32);
    ST7567_Puts("Start", &Font_8x16, 2 == page->cusor ? 0 : 1);
    ST7567_GotoXY(LINE_HIGHT*11, 48);
    ST7567_Puts("Reset", &Font_8x16, 3 == page->cusor ? 0 : 1);
    //显示按钮框
    ST7567_DrawRectangle(LINE_HIGHT*11, 0, LINE_HIGHT*5, 16, 1);
    ST7567_DrawRectangle(LINE_HIGHT*11, 32, LINE_HIGHT*5, 16, 1);
    ST7567_DrawRectangle(LINE_HIGHT*11, 48, LINE_HIGHT*5, 16, 1);


    for(int i = 0; i < GetMonitor()->sensor_size; i++)
    {
        if(PHEC_TYPE == GetMonitor()->sensor[i].type)
        {
            if(CON_FAIL != GetMonitor()->sensor[i].conn_state)
            {
                for(int port = 0; port < GetMonitor()->sensor[i].storage_size; port++)
                {
                    if(F_S_PH == GetMonitor()->sensor[i].__stora[port].func)
                    {
                        if(1 == page->cusor)
                        {
                            sprintf(show, "%.1f", (float)GetMonitor()->sensor[i].__stora[port].value / 100);
                            ST7567_GotoXY(LINE_HIGHT, 16);
                            ST7567_Puts(show, &Font_6x12, 1);
                            ST7567_GotoXY(LINE_HIGHT, 48);
                            ST7567_Puts("-- ", &Font_6x12, 1);
                        }
                        else if(2 == page->cusor)
                        {
                            ST7567_GotoXY(LINE_HIGHT, 16);
                            ST7567_Puts("-- ", &Font_6x12, 1);
                            sprintf(show, "%.1f", (float)GetMonitor()->sensor[i].__stora[port].value / 100);
                            ST7567_GotoXY(LINE_HIGHT, 48);
                            ST7567_Puts(show, &Font_6x12, 1);
                        }
                        else if(3 == page->cusor)
                        {
                            ST7567_GotoXY(LINE_HIGHT, 16);
                            ST7567_Puts("-- ", &Font_6x12, 1);
                            ST7567_GotoXY(LINE_HIGHT, 48);
                            ST7567_Puts("-- ", &Font_6x12, 1);
                        }
                    }
                }
            }
        }
    }

    //3.按键点击事件处理
    if(YES == page->select)
    {
        if((1 == page->cusor) && (CAL_INCAL != ph->cal_4_flag))
        {
            ph->cal_7_flag = CAL_INCAL;
            ph->time = getTimeStamp();
        }
        else if((2 == page->cusor) && (CAL_INCAL != ph->cal_7_flag))
        {
            ph->cal_4_flag = CAL_INCAL;
            ph->time = getTimeStamp();
        }
        else if(3 == page->cusor)
        {
            ph->cal_7_flag = CAL_NO;
            ph->cal_4_flag = CAL_NO;
            resetSysSetPhCal();
            GetSysSet()->saveFlag = YES;
        }
        page->select = NO;
    }

    //4.
    ST7567_UpdateScreen();
}

void ecCalibratePage(type_page_t *page, ec_cal_t *ec)
{
    char data[7] = "";
    char data1[7] = "";
    char show[16] = "";

    //1.
    ST7567_GotoXY(LINE_HIGHT, 0);
    if(CAL_INCAL == ec->cal_0_flag)
    {
        strncpy(data, "Cal..." ,7);
    }
    else
    {
        strncpy(data, "OK" ,7);
    }

    if(CAL_INCAL == ec->cal_141_flag)
    {
        strncpy(data1, "Cal..." ,7);
    }
    else
    {
        strncpy(data1, "OK" ,7);
    }

    sprintf(show,"%6s %6s","EC0.0 ",data);
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts(show, &Font_6x12, 1);
    sprintf(show,"%6s %6s","EC1.41",data1);
    ST7567_GotoXY(LINE_HIGHT, 32);
    ST7567_Puts(show, &Font_6x12, 1);

    //2.显示按钮
    ST7567_GotoXY(LINE_HIGHT*11, 0);
    ST7567_Puts("Start", &Font_8x16, 1 == page->cusor ? 0 : 1);
    ST7567_GotoXY(LINE_HIGHT*11, 32);
    ST7567_Puts("Start", &Font_8x16, 2 == page->cusor ? 0 : 1);
    ST7567_GotoXY(LINE_HIGHT*11, 48);
    ST7567_Puts("Reset", &Font_8x16, 3 == page->cusor ? 0 : 1);

    //显示按钮框
    ST7567_DrawRectangle(LINE_HIGHT*11, 0, LINE_HIGHT*5, 16, 1);
    ST7567_DrawRectangle(LINE_HIGHT*11, 32, LINE_HIGHT*5, 16, 1);
    ST7567_DrawRectangle(LINE_HIGHT*11, 48, LINE_HIGHT*5, 16, 1);

    for(int i = 0; i < GetMonitor()->sensor_size; i++)
    {
        if(PHEC_TYPE == GetMonitor()->sensor[i].type)
        {
            if(CON_FAIL != GetMonitor()->sensor[i].conn_state)
            {
                for(int port = 0; port < GetMonitor()->sensor[i].storage_size; port++)
                {
                    if(F_S_EC == GetMonitor()->sensor[i].__stora[port].func)
                    {
                        if(1 == page->cusor)
                        {
                            sprintf(show, "%.1f ms/cm", (float)GetMonitor()->sensor[i].__stora[port].value / 100);
                            ST7567_GotoXY(LINE_HIGHT, 16);
                            ST7567_Puts(show, &Font_6x12, 1);
                            ST7567_GotoXY(LINE_HIGHT, 48);
                            ST7567_Puts("--  ms/cm", &Font_6x12, 1);
                        }
                        else if(2 == page->cusor)
                        {
                            ST7567_GotoXY(LINE_HIGHT, 16);
                            ST7567_Puts("--  ms/cm", &Font_6x12, 1);
                            sprintf(show, "%.1f ms/cm", (float)GetMonitor()->sensor[i].__stora[port].value / 100);
                            ST7567_GotoXY(LINE_HIGHT, 48);
                            ST7567_Puts(show, &Font_6x12, 1);
                        }
                        else if(3 == page->cusor)
                        {
                            ST7567_GotoXY(LINE_HIGHT, 16);
                            ST7567_Puts("--  ms/cm", &Font_6x12, 1);
                            ST7567_GotoXY(LINE_HIGHT, 48);
                            ST7567_Puts("--  ms/cm", &Font_6x12, 1);
                        }
                    }
                }
            }
        }
    }

    //3.按键点击事件处理
    if(YES == page->select)
    {
        if((1 == page->cusor) && (CAL_INCAL != ec->cal_141_flag))
        {
            ec->cal_0_flag = CAL_INCAL;
            ec->time = getTimeStamp();
        }
        else if((2 == page->cusor) && (CAL_INCAL != ec->cal_0_flag))
        {
            ec->cal_141_flag = CAL_INCAL;
            ec->time = getTimeStamp();
        }
        else if(3 == page->cusor)
        {
            ec->cal_0_flag = CAL_NO;
            ec->cal_141_flag = CAL_NO;
            resetSysSetEcCal();
            GetSysSet()->saveFlag = YES;
        }
        page->select = NO;
    }

    //4.
    ST7567_UpdateScreen();
}

void co2CalibraterResPage(u8 flag)
{
   if(NO == flag)
   {
       //失败
       ST7567_GotoXY(LINE_HIGHT, 0);
       ST7567_Puts("Calibrate fail ", &Font_8x16, 1);
       ST7567_GotoXY(LINE_HIGHT, 16);
       ST7567_Puts("                ", &Font_8x16, 1);
       ST7567_GotoXY(LINE_HIGHT, 16*2);
       ST7567_Puts("                ", &Font_8x16, 1);
       ST7567_GotoXY(LINE_HIGHT, 16*3);
       ST7567_Puts("                ", &Font_8x16, 1);
   }
   else if(YES == flag)
   {
       //成功
       ST7567_GotoXY(LINE_HIGHT, 0);
       ST7567_Puts("Calibrate OK  ", &Font_8x16, 1);
       ST7567_GotoXY(LINE_HIGHT, 16);
       ST7567_Puts("                ", &Font_8x16, 1);
       ST7567_GotoXY(LINE_HIGHT, 16*2);
       ST7567_Puts("                ", &Font_8x16, 1);
       ST7567_GotoXY(LINE_HIGHT, 16*3);
       ST7567_Puts("                ", &Font_8x16, 1);
   }

   ST7567_UpdateScreen();
   pageSelect.cusor = 1;//避免客户直接点击YES 又再次校准
}

void factoryPage(type_page_t page, u8 canShow)
{
    char                show[5][16] = {"Sensor State", "Device State", "Line State", "SD Card", "Test"};
    u8                  column      = 0;
    u8                  line        = LINE_HIGHT;

    ST7567_GotoXY(line, column);
    ST7567_Puts("FACTORY MODE", &Font_8x16, 0);

    column = 16;
    if(page.cusor <= canShow)
    {
        for(u8 index = 0; index < canShow; index++)
        {
            ST7567_GotoXY(line, column + index * 16);
            //显示选中
            if(page.cusor == index + 1)
            {
                ST7567_Puts(show[index], &Font_8x16, 0);
            }
            else
            {
                ST7567_Puts(show[index], &Font_8x16, 1);
            }
        }
    }
    else
    {
        for(u8 index = 0; index < canShow; index++)
        {
            ST7567_GotoXY(line, column + index * 16);
            //显示选中
            if(page.cusor == index + 1 + page.cusor - canShow)
            {
                ST7567_Puts(show[index + page.cusor - canShow], &Font_8x16, 0);
            }
            else
            {
                ST7567_Puts(show[index + page.cusor - canShow], &Font_8x16, 1);
            }
        }
    }

    ST7567_UpdateScreen();
}

//工厂模式
void SensorStatePage_fac(type_monitor_t *monitor, u8 canShow)
{
#define                 LIST_NUM        9
    int                 data            = 0;
    char                temp[17]        = "";
    char                show[LIST_NUM][6] = {"Temp", "Humi", "Co2", "Light", "Par", "Ec", "Ph","wt", "wl"};
    u8                  show_fun[LIST_NUM] = {F_S_TEMP,F_S_HUMI,F_S_CO2,F_S_LIGHT,F_S_PAR,F_S_EC,F_S_PH,F_S_WT,F_S_WL};
    u8                  column      = 0;
    u8                  line        = LINE_HIGHT;
    static u8           showInde    = 0;
    static u8           connect[SENSOR_MAX];
    static u8           first_flag      = YES;

    if(YES == first_flag)
    {
        rt_memcpy(connect, CON_NULL, SENSOR_MAX);
        first_flag = NO;
    }

    //1.特殊处理,如果是有断线的就清除
    for(u8 index = 0; index < SENSOR_MAX; index++)
    {
        if(connect[index] != monitor->sensor[index].conn_state)
        {
            //如果是上次状态是在线这次掉线，则删除
            if(/*(CON_SUCCESS == connect[index]) && */(CON_FAIL == monitor->sensor[index].conn_state))
            {
                deleteModule(monitor, monitor->sensor[index].addr);
            }

            connect[index] = monitor->sensor[index].conn_state;
        }
    }

    //2.显示值
    for(u8 index = showInde; index < showInde + canShow; index++)
    {
        column = (index - showInde) * 16;
        ST7567_GotoXY(line, column);
        data = getSensorDataByFunc(monitor, show_fun[index]);
        rt_memset(temp, ' ', 16);
        temp[16] = '\0';
        if(VALUE_NULL == data)
        {
            sprintf(temp,"%5s      -- ",show[index]);
            ST7567_Puts(temp, &Font_8x16, 0);
        }
        else
        {
            sprintf(temp,"%5s     %4d",show[index],data);
            ST7567_Puts(temp, &Font_8x16, 1);
        }
    }

    //3.刷新界面
    ST7567_UpdateScreen();

    //4.计算循环,显示到底部了再次从前面开始
    if(showInde + canShow < LIST_NUM)
    {
        showInde++;
    }
    else
    {
        showInde = 0;
    }
}

void lineStatePage_fac(type_page_t *page, type_monitor_t *monitor, u8 canShow)
{

    int                 data            = 0;
    char                name[7]         = "";
    char                temp[17]        = "";
    u8                  list_num        = 0;
    u8                  column          = 0;
    u8                  line            = LINE_HIGHT;
    static u8           showInde        = 0;
    static u8           connect[LINE_MAX];
    static u8           first_flag      = YES;

    if(YES == first_flag)
    {
        rt_memcpy(connect, CON_NULL, LINE_MAX);
        first_flag = NO;
    }

    //1.特殊处理,如果是有断线的就清除
    for(u8 index = 0; index < LINE_MAX; index++)
    {
        if(connect[index] != monitor->line[index].conn_state)
        {
            //如果是上次状态是在线这次掉线，则删除
            if(/*(CON_SUCCESS == connect[index]) && */(CON_FAIL == monitor->line[index].conn_state))
            {
                deleteModule(monitor, monitor->line[index].addr);
            }

            connect[index] = monitor->line[index].conn_state;
        }
    }

    list_num = monitor->line_size;

    if(canShow > list_num)
    {
        canShow = list_num;
    }

    //1.
    for(u8 index = showInde; index < showInde + canShow; index++)
    {
        column = (index - showInde) * 16;
        ST7567_GotoXY(line, column);
        strncpy(name,GetMonitor()->line[index].name,6);
        name[6] = '\0';
        data = GetMonitor()->line[index].conn_state;
        rt_memset(temp, ' ', 16);
        temp[16] = '\0';
//        if(CON_FAIL == data)
//        {
//            sprintf(temp,"%6s  offline", name);
//            temp[16] = '\0';
//            ST7567_Puts(temp, &Font_8x16, 0);
//        }
//        else
        if(CON_SUCCESS == data)
        {
            if(OFF == GetMonitor()->line[index].d_state)
            {
                sprintf(temp,"%6s   OFF", name);
            }
            else
            {
                sprintf(temp,"%6s   %3d%%", name, GetMonitor()->line[index].d_value);
            }
            temp[16] = '\0';
            ST7567_Puts(temp, &Font_8x16, 1);
        }
    }

    //2.是否全开/全关
    ST7567_GotoXY(48, 48);
    ST7567_Puts("ON", &Font_8x16, 1 == page->cusor ? 0 : 1);
    ST7567_GotoXY(64, 48);
    ST7567_Puts("OFF", &Font_8x16, 2 == page->cusor ? 0 : 1);

    if(ON == page->select)
    {
        if(1 == page->cusor)
        {
            lineStage_Fa(monitor);
        }
        else if(2 == page->cusor)
        {
            lineStageClose_Fa(monitor);
        }
        page->select = OFF;
    }

    //3.刷新界面
    ST7567_UpdateScreen();

    //4.计算循环,显示到底部了再次从前面开始
    if(list_num > canShow)
    {
        if(showInde + canShow < list_num)
        {
            showInde++;
        }
        else
        {
            showInde = 0;
        }
    }
}

void deviceStatePage_fac(type_page_t *page, type_monitor_t *monitor, u8 canShow)
{
    int                 data            = 0;
    char                name[7]         = "";
    char                temp[17]        = "";
    u8                  list_num        = 0;
    u8                  column          = 0;
    u8                  line            = LINE_HIGHT;
    static u8           showInde        = 0;
    static u8           connect[DEVICE_MAX];
    static u8           first_flag      = YES;

    if(YES == first_flag)
    {
        rt_memcpy(connect, CON_NULL, DEVICE_MAX);
        first_flag = NO;
    }

    //1.特殊处理,如果是有断线的就清除
    for(u8 index = 0; index < DEVICE_MAX; index++)
    {
        if(connect[index] != monitor->device[index].conn_state)
        {
            //如果是上次状态是在线这次掉线，则删除
            if(/*(CON_SUCCESS == connect[index]) && */(CON_FAIL == monitor->device[index].conn_state))
            {
                deleteModule(monitor, monitor->device[index].addr);
            }

            connect[index] = monitor->device[index].conn_state;
        }
    }

    list_num = monitor->device_size;

    if(canShow > list_num)
    {
        canShow = list_num;
    }

    //2.
    for(u8 index = showInde; index < showInde + canShow; index++)
    {
        column = (index - showInde) * 16;
        ST7567_GotoXY(line, column);
        strncpy(name,GetMonitor()->device[index].name,6);
        name[6] = '\0';
        data = GetMonitor()->device[index].conn_state;
        rt_memset(temp, ' ', 16);
        temp[16] = '\0';
//        if(CON_FAIL == data)
//        {
//            sprintf(temp,"%6s  offline", name);
//            temp[16] = '\0';
//            ST7567_Puts(temp, &Font_8x16, 0);
//        }
//        else
        if(CON_SUCCESS == data)
        {
            if(MANUAL_HAND_ON == GetMonitor()->device[index].port[0].manual.manual)
            {
                sprintf(temp,"%6s  on     ", name);
            }
            else
            {
                sprintf(temp,"%6s  off    ", name);
            }
            temp[16] = '\0';
            ST7567_Puts(temp, &Font_8x16, 1);
        }
    }

    //3.是否全开/全关
    ST7567_GotoXY(48, 48);
    ST7567_Puts("ON", &Font_8x16, 1 == page->cusor ? 0 : 1);
    ST7567_GotoXY(64, 48);
    ST7567_Puts("OFF", &Font_8x16, 2 == page->cusor ? 0 : 1);

    if(ON == page->select)
    {
        if(1 == page->cusor)
        {
            openDevices_Fa(monitor);
        }
        else if(2 == page->cusor)
        {
            closeDevices_Fa(monitor);
        }
        page->select = OFF;
    }

    //4.刷新界面
    ST7567_UpdateScreen();

    //5.计算循环,显示到底部了再次从前面开始
    if(list_num > canShow)
    {
        if(showInde + canShow < list_num)
        {
            showInde++;
        }
        else
        {
            showInde = 0;
        }
    }
}

void SDState_Fac(void)
{
    char                temp[17]        = "";

    //1.根据sd卡的情况判断
    ST7567_GotoXY(LINE_HIGHT, 0);
    if((YES != sdCard.init) || (YES != sdCard.mount))
    {
        strcpy(temp,"SD Card   ERROR");
        temp[16] = '\0';
        ST7567_Puts(temp, &Font_8x16, 0);
    }
    else
    {
        strcpy(temp,"SD Card      OK");
        temp[16] = '\0';
        ST7567_Puts(temp, &Font_8x16, 1);
    }

    //2.刷新界面
    ST7567_UpdateScreen();
}

void testFacPage(type_page_t *page,type_monitor_t *monitor, u8 canShow)
{
    u8                  line            = LINE_HIGHT;
    u8                  column          = 0;
    char                show[5][17]     = {"Open Devices", "Close Devices", "Line stage", "Open Dry", "Close Dry"};
    FAC_FUNC            show_fun[5]     = {openDevices_Fa,closeDevices_Fa,lineStage_Fa,openDryFac,closeDryFac};

    //1.循环列表选择
    column = 0;
    if(page->cusor <= canShow)
    {
        for(u8 index = 0; index < canShow; index++)
        {
            ST7567_GotoXY(line, column + index * 16);
            //显示选中
            if(page->cusor == index + 1)
            {
                ST7567_Puts(show[index], &Font_8x16, 0);
            }
            else
            {
                ST7567_Puts(show[index], &Font_8x16, 1);
            }
        }
    }
    else
    {
        for(u8 index = 0; index < canShow; index++)
        {
            ST7567_GotoXY(line, column + index * 16);
            //显示选中
            if(page->cusor == index + 1 + page->cusor - canShow)
            {
                ST7567_Puts(show[index + page->cusor - canShow], &Font_8x16, 0);
            }
            else
            {
                ST7567_Puts(show[index + page->cusor - canShow], &Font_8x16, 1);
            }
        }
    }

    //2.选择功能
    if(ON == page->select)
    {
        show_fun[page->cusor - 1](monitor);
        page->select = OFF;
    }

    //2.刷新界面
    ST7567_UpdateScreen();
}

void openDevices_Fa(type_monitor_t *monitor)
{
    u8          index   = 0;
    u8          port    = 0;
    device_t    *device = RT_NULL;

    for(index = 0; index < monitor->device_size; index++)
    {
        device = &monitor->device[index];

        for(port = 0; port < device->storage_size; port++)
        {
            device->port[port].manual.manual = MANUAL_HAND_ON;
            device->port[port].manual.manual_on_time = 65535;
        }
    }
}

void closeDevices_Fa(type_monitor_t *monitor)
{
    u8          index   = 0;
    u8          port    = 0;
    device_t    *device = RT_NULL;

    for(index = 0; index < monitor->device_size; index++)
    {
        device = &monitor->device[index];

        for(port = 0; port < device->storage_size; port++)
        {
            device->port[port].manual.manual = MANUAL_HAND_OFF;
        }
    }
}

void lineStage_Fa(type_monitor_t *monitor)
{
    u8              index               = 0;
    line_t          *line               = RT_NULL;
    static u8       value[LINE_MAX]     = {0,0};
    static u8       stage[3] = {0, 49, 100};

    for(index = 0; index < LINE_MAX; index++)
    {
        line = &monitor->line[index];

        if(value[index] < 2)
        {
            value[index] += 1;
        }
        else
        {
            value[index] = 0;
        }

        line->d_state = ON;
        line->d_value = stage[value[index]];
    }
}

void lineStageClose_Fa(type_monitor_t *monitor)
{
    u8              index               = 0;
    line_t          *line               = RT_NULL;

    for(index = 0; index < LINE_MAX; index++)
    {
        line = &monitor->line[index];

        line->d_state = OFF;
        line->d_value = 0;
    }
}

void openDryFac(type_monitor_t *monitor)
{
    rt_pin_write(ALARM_OUT, 1);
}

void closeDryFac(type_monitor_t *monitor)
{
    rt_pin_write(ALARM_OUT, 0);
}

void PhCalCallBackPage(u8 flag)
{
    char data[7] = "";
    char data1[7] = "";
    char show[16] = "";

    //1.清除缓存数据
    //clear_screen();

    //2.显示
    if(1 == flag)
    {
        strncpy(data, "Cal...", 7);
        strncpy(data1, "OK", 7);
    }
    else if(2 == flag)
    {
        strncpy(data, "OK", 7);
        strncpy(data1, "Cal...", 7);
    }
    else if(3 == flag)
    {
        strncpy(data, "OK", 7);
        strncpy(data1, "OK", 7);
    }

    sprintf(show,"%s   %6s","PH 7.0",data);
    show[15] = '\0';
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts(show, &Font_8x16, 1);

    sprintf(show,"%s   %6s","PH 4.0",data1);
    show[15] = '\0';
    ST7567_GotoXY(LINE_HIGHT, 32);
    ST7567_Puts(show, &Font_8x16, 1);

    //2.刷新界面
    ST7567_UpdateScreen();
}

void EcCalCallBackPage(u8 flag)
{
    char data[7] = "";
    char data1[7] = "";
    char show[16] = "";

    //1.清除缓存数据
    //clear_screen();

    //2.显示
    if(1 == flag)
    {
        strncpy(data, "Cal...", 7);
        strncpy(data1, "OK", 7);
    }
    else if(2 == flag)
    {
        strncpy(data, "OK", 7);
        strncpy(data1, "Cal...", 7);
    }
    else if(3 == flag)
    {
        strncpy(data, "OK", 7);
        strncpy(data1, "OK", 7);
    }

    sprintf(show,"%s   %6s","EC 0.0",data);
    show[15] = '\0';
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts(show, &Font_8x16, 1);

    sprintf(show,"%s  %6s","EC 1.41",data1);
    show[15] = '\0';
    ST7567_GotoXY(LINE_HIGHT, 32);
    ST7567_Puts(show, &Font_8x16, 1);

    //2.刷新界面
    ST7567_UpdateScreen();
}

void testPage(void)
{
    char show[16];

    sprintf(show,"btn enter %d",rt_pin_read(BUTTON_ENTER));

    ST7567_GotoXY(LINE_HIGHT, 32);
    ST7567_Puts(show, &Font_8x16, 1);

    ST7567_UpdateScreen();
}
