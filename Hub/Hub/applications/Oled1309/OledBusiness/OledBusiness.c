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

char    data[80];

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
    u8          line        = LINE_HIGHT;
    u8          column      = 0;

    u8g2_ClearBuffer(uiShow);

    //1.显示时间
//    now = time(RT_NULL);
    strcpy(data, /*ctime(&now)*/getRealTime());
    column = 12;
    u8g2_DrawStr(uiShow, line, column, &data[10]);

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
    u8              index       = 0;
    u8              storage     = 0;
    type_module_t   module;

    u8g2_ClearBuffer(uiShow);

    for(index = 0; index < GetMonitor()->module_size; index++)
    {
        module = GetMonitor()->module[index];
        if(SENSOR_TYPE == module.s_or_d)
        {
            break;
        }
    }

    if(index != GetMonitor()->module_size)
    {
        if(module.storage_size > 4)
        {
            LOG_E("sensor storage size is too long to show");
        }
        else
        {
            for(storage = 0; storage < module.storage_size; storage++)
            {
                itoa(module.storage_in[storage]._d_s.s_value, data ,10);
                u8g2_DrawStr(uiShow, line, column, module.storage_in[storage]._d_s.name);
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
    u8              list[MODULE_MAX];
    u8              list_index          = 0;
    u8              device_sum          = 0;
    type_module_t   module;

    u8g2_ClearBuffer(uiShow);
    rt_memset(data, 0, 80);

    for(index = 0; index < GetMonitor()->module_size; index++)
    {
        module = GetMonitor()->module[index];
        if(DEVICE_TYPE == module.s_or_d)
        {
            list[device_sum] = index;
            device_sum++;
        }
    }

    if(device_sum > 4)//Justin debug 仅仅测试 最多显示4行
    {
        device_sum = 4;
    }

    for(list_index = 0; list_index < device_sum; list_index++)
    {
        rt_memcpy(data, GetMonitor()->module[list[list_index]].name, 8);
        u8g2_DrawStr(uiShow, line, column + list_index * COLUMN_HIGHT, data);

        if(CON_FAIL == GetMonitor()->module[list[list_index]].conn_state)
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
