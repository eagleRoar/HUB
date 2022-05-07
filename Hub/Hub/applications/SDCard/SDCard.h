/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-01-24     Administrator       the first version
 */
#ifndef APPLICATIONS_SDCARD_SDCARD_H_
#define APPLICATIONS_SDCARD_SDCARD_H_

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>
#include <rtdbg.h>
#include <stdlib.h>

#include "GlobalConfig.h"
#include "typedef.h"
#include "string.h"
#include "Gpio.h"
#include "fal_cfg.h"
#include "InformationMonitor.h"
#include "SdcardBusiness.h"
#include "SdcardDataLayer.h"

#define         SD_DEVICE_NAME          "sd0"

#define         DOWNLOAD_DIR            "/download"                                                 //APP升级包文件夹
#define         DOWNLOAD_FILE           "download/downloadFile.bin"                                 //APP升级包文件
#define         MODULE_DIR              "/moduleInfo"                                               //模块相关信息文件夹
#define         MODULE_FILE             "moduleInfo/module.bin"                                     //模块信息文件
#define         TEST_DIR                "/test"
#define         TEST_FILE               "/test/test.txt"
#define         SETTING_DIR             "/master"
#define         ACTION_FILE             "master/action.bin"
#define         CONDITION_FILE          "master/condition.bin"
#define         EXCUTE_FILE             "master/excute.bin"
#define         DOTASK_FILE             "master/dotask.bin"

struct operateStruct{
    struct actionOpe{
        type_action_t               (*GetAction)(u8);
        u8                          (*ReadActionSum)(void);
        void                        (*AddActionToSD)(type_action_t);
    }action_op;

    struct conditionOpe{
        u8                          (*ReadConditionSum)(void);
        void                        (*AddConditionToSD)(type_condition_t);
    }condition_op;

    struct excuteOpe{
        u8                          (*ReadExcuteSum)(void);
        void                        (*AddExcuteToSD)(type_excute_t);
    }excute_op;

    struct dotaskOpe{
        u8                          (*ReadDotaskSum)(void);
        void                        (*AddDotaskToSD)(type_dotask_t);
    }dotask_op;
};

struct sdCardState{
    u8 init;                                                                                        //是否初始化成功
    u8 mount;                                                                                       //是否已经挂载
    u8 readInfo;                                                                                    //是否已经读取了信息
    type_sdoperate_t sd_operate;                                                                    //sd业务层操作的映射
};

int SDCardTaskInit(void);
void sd_dfs_event_entry(void* parameter);

#endif /* APPLICATIONS_SDCARD_SDCARD_H_ */
