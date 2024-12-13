/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-05-24     Administrator       the first version
 */
#include <rtthread.h>
#include "qrcode.h"
#include "Oled1309.h"
#include "OledBusiness.h"
#include "ST7567.h"

void qrcode(void)
{
#define DEFAULT_QR_VERSION 6
#define DEFAULT_QR_STRING "HELLO WORLD"

    QRCode qrc;
    uint8_t x, y, *qrcodeBytes = (uint8_t *)rt_calloc(1, qrcode_getBufferSize(DEFAULT_QR_VERSION));
    int8_t result;

    char qrstr[13] = " ";

    GetSnName(qrstr, 12);
    if (qrcodeBytes)
    {

        result = qrcode_initText(&qrc, qrcodeBytes, DEFAULT_QR_VERSION, ECC_LOW, qrstr);

        if (result >= 0)
        {
            //rt_kprintf("\n");
            for (y = 0; y < qrc.size; y++)
            {
                for (x = 0; x < qrc.size; x++)
                {
                    if (qrcode_getModule(&qrc, x, y))
                    {
                        //rt_kprintf("ÛÛ");
                        ST7567_DrawLine(x, y, x, y, 1);
                    }
                    else
                    {
                        //rt_kprintf("  ");
                    }
                }
                //rt_kprintf("\n");
            }
        }
        else
        {
            rt_kprintf("QR CODE(%s) General FAILED(%d)\n", qrstr, result);
        }
        //rt_kprintf("qrc.size = %d\r\n",qrc.size);

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
