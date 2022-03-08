/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-12     Administrator       the first version
 */
#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_
#include <rtconfig.h>
#include <board.h>
#define RT_APP_PART_ADDR            (0x08000000 + 128*1024)//app partition begin address
#define NOR_FLASH_DEV_NAME          FAL_USING_NOR_FLASH_DEV_NAME//"norflash0"

#define STM32_FLASH_START_ADRESS_16K        ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define FLASH_SIZE_GRANULARITY_16K          (4 * 16 * 1024)

#define STM32_FLASH_START_ADRESS_64K        ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define FLASH_SIZE_GRANULARITY_64K          (1 * 64 * 1024)

#define STM32_FLASH_START_ADRESS_128K       ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define FLASH_SIZE_GRANULARITY_128K         (7 * 128 * 1024)

#define STM32_FLASH_START_ADRESS_512K       ((uint32_t)0x08000000) /* Base @ of Sector 5, 128 Kbytes */
#define FLASH_SIZE_GRANULARITY_512K         (512 * 1024)

/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev stm32_onchip_flash_512k;
//extern struct fal_flash_dev nor_flash0;
/* flash device table */

#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &stm32_onchip_flash_512k,                                             \
    /*&nor_flash0,*/                                                 \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
//app区的大小需要修改,以下的onchip_flash_64k名字要和定义的对应， Justin debug
#define FAL_PART_TABLE                                                                      \
{                                                                                           \
    /*{FAL_PART_MAGIC_WORD,   "bl",           "onchip_flash",     0,          32*1024,  0},*/  \
    {FAL_PART_MAGIC_WORD,   "app",            "onchip_flash_512k",     128*1024,   128*1024,  0},  \
    /*{FAL_PART_MAGIC_WORD,   "factory",      "onchip_flash",     160*1024,  92*1024,  0},*/  \
    {FAL_PART_MAGIC_WORD,   "download",       "onchip_flash_512k",     256*1024,  256*1024,  0},  \
    /*{FAL_PART_MAGIC_WORD,   "param",        "onchip_flash_512k",     320*1024,  192*1024,  0},*/  \
    /*{FAL_PART_MAGIC_WORD,   "filesys",      NOR_FLASH_DEV_NAME, 0,          8*1024*1024,  0},*/ \
}
#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */
