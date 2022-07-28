/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-26     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_BUSINESSLAYER_SDCARDBUSINESS_H_
#define APPLICATIONS_SDCARD_BUSINESSLAYER_SDCARDBUDDNESS_H_

#include "Gpio.h"
#include "SDCardData.h"
#include "Recipe.h"

#define         DOWNLOAD_DIR            "/download"                                                 //APP升级包文件夹
#define         MODULE_DIR              "/moduleInfo"                                               //模块相关信息文件夹
#define         SETTING_DIR             "/master"
#define         DOWNLOAD_FILE           "download/downloadFile.bin"                                 //APP升级包文件
#define         MODULE_FILE             "moduleInfo/module.bin"                                     //模块信息文件
#define         SYSSET_FILE             "moduleInfo/sys_set6.bin"                                    //模块信息文件
#define         RECIPE_FILE             "moduleInfo/recipe.bin"                                     //模块信息文件


#define     SD_HEAD_CORE        0xa5a55a5a                             //该core 主要是为了验证该文件是否有写过
#define     SD_HEAD_SIZE        4
#define     SD_PAGE_SIZE        1
#define     SD_INFOR_SIZE       SD_HEAD_SIZE + SD_PAGE_SIZE            //存储的有几段struct 数量位占据1位

void InitSDCard(void);
rt_err_t SaveModule(type_monitor_t *);
rt_err_t TakeMonitorFromSD(type_monitor_t *);
rt_err_t TackSysSetFromSD(sys_set_t *);
rt_err_t SaveSysSet(sys_set_t *);
rt_err_t TackRecipeFromSD(sys_recipe_t *);
#endif /* APPLICATIONS_SDCARD_BUSINESSLAYER_SDCARDBUSINESS_H_ */
