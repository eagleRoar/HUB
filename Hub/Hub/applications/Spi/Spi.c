/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-05     Administrator       the first version
 */

#include "Spi.h"
#include "spi_flash_sfud.h"

#define W25Q_SPI_DEVICE_NAME     "spi10"
#define FAL_USING_NOR_FLASH_DEV_NAME "norflash0"


/**
 * 总线上挂载设备 spi10
 * @return
 */
int bsp_spi_attach_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    rt_err_t ret = rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_4);// spi10 表示挂载在 spi3 总线上的 0 号设备,PC0是片选，这一步就可以将从设备挂在到总线中。
    if(ret <0)
    {
        LOG_E("flash attach spi1 failed");
        return -RT_ERROR;
    }

    rt_sfud_flash_probe(FLASH_MEMORY_NAME, "spi10");// 注册nor flash

    return RT_EOK;
}

