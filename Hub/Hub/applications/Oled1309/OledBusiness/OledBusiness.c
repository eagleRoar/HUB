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

char    data[80];
extern void GetUpdataFileFromWeb(void);
#if (NEW_OLED == 0)
static void ShowCursor(u8g2_t *u8g2, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, u8 show)
{
    if(YES == show)
    {
        u8g2_DrawTriangle(u8g2, x0, y0, x1, y1, x2, y2);
    }
}


void HomePage(u8g2_t *uiShow, type_page_t page)
{
//    time_t      now;
    char        time[16]    = "";
    char        temp[2];
    char        temp3;
    char        temp1[4];
    type_sys_time   sys_time;
    u8          line        = LINE_HIGHT;
    u8          column      = 0;

    u8g2_ClearBuffer(uiShow);

    //1.显示时间
//    now = time(RT_NULL);
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
//            temp3 = ' ';
//            strncat(time, &temp3, 1);
        }
        else
        {
            strcpy(temp, "AM");
            strncat(time, temp, 2);
//            temp3 = ' ';
//            strncat(time, &temp3, 1);
        }

        itoa(sys_time.year, temp1, 10);
        strncat(time, temp1, 4);
//        LOG_D("time = %s",time);
        strcpy(data, time);

        column = 12;
        u8g2_DrawStr(uiShow, line, column, data);
    }
    else
    {
        strcpy(data, getRealTime());
        column = 12;
        u8g2_DrawStr(uiShow, line, column, &data[10]);
    }

    //2.显示sensor state
    column += COLUMN_HIGHT;
    u8g2_DrawStr(uiShow, line, column, SENSOR_STATE_NAME);

    //2.显示sensor state
    column += COLUMN_HIGHT;
    u8g2_DrawStr(uiShow, line, column, DEVICE_STATE_NAME);

    ShowCursor(uiShow, 0, COLUMN_HIGHT*page.cusor, 0, COLUMN_HIGHT * (page.cusor + 1),
               LINE_HIGHT, COLUMN_HIGHT*page.cusor + (COLUMN_HIGHT + 1)/2, page.cusor_show);

    u8g2_SendBuffer(uiShow);
}

void SensorStatePage(u8g2_t *uiShow, type_page_t page)
{
    u8              line        = LINE_HIGHT;
    u8              column      = COLUMN_HIGHT;
//    u8              index       = 0;
    u8              storage     = 0;
    sensor_t        *module     = RT_NULL;

    u8g2_ClearBuffer(uiShow);

    if(GetMonitor()->sensor_size > 0)
    {
        module = GetSensorByType(GetMonitor(), BHS_TYPE);
        if(RT_NULL != module)
        {
            for(storage = 0; storage < module->storage_size; storage++)
            {
               itoa(module->__stora[storage].value, data ,10);
//               LOG_D("SensorStatePage--------------name %s",module->__stora[storage].name);
               u8g2_DrawStr(uiShow, line, column, module->__stora[storage].name);
               u8g2_DrawStr(uiShow, line + LINE_HIGHT*10, column, data);

               column += COLUMN_HIGHT;
            }
        }
    }
    else
    {
        rt_memcpy(data, "no sensor connect", sizeof("no sensor connect"));
        u8g2_DrawStr(uiShow, line, column, data);
    }

    ShowCursor(uiShow, 0, COLUMN_HIGHT*page.cusor, 0, COLUMN_HIGHT * (page.cusor + 1),
               LINE_HIGHT, COLUMN_HIGHT*page.cusor + (COLUMN_HIGHT + 1)/2, page.cusor_show);
    u8g2_SendBuffer(uiShow);

}

void DeviceStatePage(u8g2_t *uiShow, type_page_t page)
{
    u8              line                = LINE_HIGHT;
    u8              column              = COLUMN_HIGHT;
    static u8       index               = 0;
    u8              list[DEVICE_TIME4_MAX];
    u8              list_index          = 0;
    u8              device_sum          = 0;

    u8g2_ClearBuffer(uiShow);
    rt_memset(data, 0, 80);

    for(index = 0; index < GetMonitor()->device_size; index++)
    {
        list[device_sum] = index;
        device_sum++;
    }

    if(device_sum > 4)
    {
        device_sum = 4;
    }

    for(list_index = 0; list_index < device_sum; list_index++)
    {
        rt_memcpy(data, GetMonitor()->device[list[list_index]].name, 8);
        u8g2_DrawStr(uiShow, line, column + list_index * COLUMN_HIGHT, data);

        if(CON_FAIL == GetMonitor()->device[list[list_index]].conn_state)
        {
            u8g2_DrawStr(uiShow, line + 10 * LINE_HIGHT, column + list_index * COLUMN_HIGHT, "fail");
        }
        else
        {
            u8g2_DrawStr(uiShow, line + 10 * LINE_HIGHT, column + list_index * COLUMN_HIGHT, "ok");
        }
    }

    ShowCursor(uiShow, 0, COLUMN_HIGHT*page.cusor, 0, COLUMN_HIGHT * (page.cusor + 1),
               LINE_HIGHT, COLUMN_HIGHT*page.cusor + (COLUMN_HIGHT + 1)/2, page.cusor_show);
    u8g2_SendBuffer(uiShow);
}
#else
void HomePage_new(type_page_t page, u8 canShow)
{
//    time_t      now;
    char        time[16]    = "";
    char        temp[2];
    char        temp3;
    char        temp1[4];
    char        show[4][16] = {"Sensor State", "Device State", "QR Code", "Update App"};
    type_sys_time   sys_time;
    u8          line        = LINE_HIGHT;
    u8          column      = 0;

//    clear_screen();

    //1.显示时间
//    now = time(RT_NULL);
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
        ST7567_GotoXY(line, column);
        ST7567_Puts(data, &Font_6x12, 1);
    }
    else
    {
        strcpy(data, getRealTime());
        column = 0;
        ST7567_GotoXY(line, column);
        ST7567_Puts(data, &Font_5x7, 1);
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
    u8              line        = LINE_HIGHT;
    u8              column      = 0;
    char            name[5]     = " ";
    float           num;
    sys_set_t       *set        = GetSysSet();
    static u8       showInde    = 0;
    static u8       showStorage = 0;

    //1.清除界面
    clear_screen();

    //2.轮流显示当前的sensor

    if(monitor->sensor_size > 0)
    {
        //2.1 先显示固定位置的名称
        ST7567_GotoXY(LINE_HIGHT, 0);
        rt_memcpy(name, monitor->sensor[showInde].__stora[showStorage].name, 4);
        name[4] = '\0';
        ST7567_Puts(name, &Font_12x24, 1);

        line = LINE_HIGHT + 4 * 12 + 8;
        column = 24 - 16;
        ST7567_GotoXY(line, column);
        if(F_S_HUMI == monitor->sensor[showInde].__stora[showStorage].func ||
           F_S_TEMP == monitor->sensor[showInde].__stora[showStorage].func)
        {
            num = monitor->sensor[showInde].__stora[showStorage].value;
            num /= 10;
            sprintf(name, "%.1f", num);
        }
        else
        {
            itoa(monitor->sensor[showInde].__stora[showStorage].value, name, 10);
        }
        name[4] = '\0';
        ST7567_Puts(name, &Font_8x16, 1);

        //2.2 显示单位
        if(F_S_CO2 == monitor->sensor[showInde].__stora[showStorage].func)
        {
            //显示单位
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("ppm", &Font_8x16, 1);

            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("ppm", &Font_8x16, 1);

            //显示目标值
            line = LINE_HIGHT;
            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("co2 tgt", &Font_6x12, 1);

            line = LINE_HIGHT + 4 * 12 + 8;
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
            ST7567_Puts(name, &Font_8x16, 1);
        }
        else if(F_S_HUMI == monitor->sensor[showInde].__stora[showStorage].func)
        {
            //显示单位
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("%", &Font_8x16, 1);

            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("%", &Font_8x16, 1);

            column += 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("%", &Font_8x16, 1);

            //显示目标值
            line = LINE_HIGHT;
            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("humi tgt", &Font_6x12, 1);

            line = LINE_HIGHT + 4 * 12 + 8;
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
            ST7567_Puts(name, &Font_8x16, 1);

            line = LINE_HIGHT;
            column = 48;
            ST7567_GotoXY(line, column);
            ST7567_Puts("dehu tgt", &Font_6x12, 1);

            line = LINE_HIGHT + 4 * 12 + 8;
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
            ST7567_Puts(name, &Font_8x16, 1);
        }
        else if(F_S_TEMP == monitor->sensor[showInde].__stora[showStorage].func)
        {
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_DrawCircle(line, column, 1, 1);
            ST7567_GotoXY(line + 3, column);
            ST7567_Puts("C", &Font_8x16, 1);

            column = 32;
            ST7567_DrawCircle(line, column, 1, 1);
            ST7567_GotoXY(line + 3, column);
            ST7567_Puts("C", &Font_8x16, 1);

            column += 16;
            ST7567_DrawCircle(line, column, 1, 1);
            ST7567_GotoXY(line + 3, column);
            ST7567_Puts("C", &Font_8x16, 1);

            //显示目标值
            line = LINE_HIGHT;
            column = 32;
            ST7567_GotoXY(line, column);
            ST7567_Puts("heat tgt", &Font_6x12, 1);

            line = LINE_HIGHT + 4 * 12 + 8;
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
            ST7567_Puts(name, &Font_8x16, 1);

            line = LINE_HIGHT;
            column = 48;
            ST7567_GotoXY(line, column);
            ST7567_Puts("cool tgt", &Font_6x12, 1);

            line = LINE_HIGHT + 4 * 12 + 8;
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
            ST7567_Puts(name, &Font_8x16, 1);
        }
        else if(F_S_PAR == monitor->sensor[showInde].__stora[showStorage].func)
        {
            line = LINE_HIGHT + 4 * 12 + 5 * 8;
            column = 24 - 16;
            ST7567_GotoXY(line, column);
            ST7567_Puts("ppfd", &Font_8x16, 1);
        }
    }

    if(showStorage + 1 < monitor->sensor[showInde].storage_size)
    {
        showStorage++;
    }
    else
    {
        showStorage = 0;
        if(showInde + 1 < monitor->sensor_size) // showInde < monitor->sensor_size - 1
        {
            showInde++;
        }
        else
        {
            showInde = 0;
            showStorage = 0;
        }
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
        ST7567_Puts("NO DEVICE", &Font_12x24, 0);
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
    //            LOG_I("name %s",monitor->device[index + start].name);//Justin debug
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
    //4.刷新界面
    ST7567_UpdateScreen();
}

void qrcode(void)
{
#define DEFAULT_QR_VERSION 6
//#define DEFAULT_QR_STRING "HELLO WORLD"

    QRCode qrc;
    uint8_t x, y, *qrcodeBytes = (uint8_t *)rt_calloc(1, qrcode_getBufferSize(DEFAULT_QR_VERSION));
    int8_t result;
//    char *qrstr = DEFAULT_QR_STRING;
    char qrstr[13] = " ";

    rt_memset(qrstr, ' ', 13);
    GetSnName(qrstr, 12);

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
//                        rt_kprintf("ÛÛ");
                        ST7567_DrawLine(x, y, x, y, 1);
                    }
                    else
                    {
//                        rt_kprintf("  ");
                    }
                }
//                rt_kprintf("\n");
            }
        }
        else
        {
            rt_kprintf("QR CODE(%s) General FAILED(%d)\n", qrstr, result);
        }

        LOG_D("qrc.size = %d",qrc.size);

        rt_free(qrcodeBytes);
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

    //1.清除界面
//    clear_screen();

    //2.提示是否要下载
    ST7567_GotoXY(LINE_HIGHT, 0);
    ST7567_Puts("Update New APP?", &Font_8x16, 1);

    //3.确定是否升级
    line = 40;
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
            GetUpdataFileFromWeb();
        }
        page->select = OFF;
    }

    //5.刷新界面
    ST7567_UpdateScreen();
}

#endif
