/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-08     Administrator       the first version
 */
#ifndef APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_
#define APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_

#include "Gpio.h"
#include "mqtt_client.h"
#include "CloudProtocolBusiness.h"
#include "TcpProgram.h"

#pragma pack(4)//因为cjson 不能使用1字节对齐

typedef void(*PAGE_CB)(u8);

void initSysSet(void);
void initSysSetExtern(void);
void initCloudSet(void);
void initCloudProtocol(void);
char *GetSnName(char *,u8);
void analyzeCloudData(char *,u8 );
hub_t *GetHub(void);
rt_err_t SendDataToCloud(mqtt_client *, char *, u8 , u16 , u8 *, u16 *, u8, u8, u8);
time_t ReplyTimeStamp(void);
u16 getVpd(void);
void initSysTank(void);
time_t changeTmTotimet(struct tm *);
struct tm* getTimeStampByDate(time_t *);
struct sys_tank *GetSysTank(void);
void initOfflineFlag(void);
void initHubinfo(void);
struct sysSet *GetSysSet(void);
void autoBindPumpTotank(type_monitor_t *, struct sys_tank *);
rt_err_t ReplyDeviceListDataToCloud(mqtt_client *client, int *sock, u8 sendCloudFlg);
rt_err_t ReplyDataToCloud(mqtt_client *client, int *sock, u8 sendCloudFlg);
void co2Calibrate(type_monitor_t *monitor, int *data, u8 *do_cal_flg, u8 *saveFlg, PAGE_CB cb);
void sendwarnningInfo(void);
void sendOfflinewarnning(type_monitor_t *);
void resetSysSetPhCal(u32);
void resetSysSetEcCal(u32);
#if(HUB_SELECT  == HUB_IRRIGSTION)
aqua_set* GetAquaSetList(void);
aqua_info_t* GetAquaInfoList(void);
void initAquaSetAndInfo(void);
void addToAquaInfoList(aqua_info_t *info, u8 recipe_no);
void addNewAquaSetAndInfo(u32 uuid);
void addToAquaSetList(aqua_set *set);
void SetAquaWarn(aqua_state_t *aqua_state);
void AddAquaWarn(u8 addr);
aqua_state_t *GetAquaWarnByAddr(u8 id);
aqua_state_t *GetAquaWarn(void);
aqua_recipe* GetAquaRecipe(u32 uuid, u8 no);
aqua_set* GetAquaSetByUUID(u32 uuid);
aqua_info_t *GetAquaInfoByUUID(u32 uuid);
void initTankWarnState(void);
void InitAquaWarn(void);
#endif
#endif /* APPLICATIONS_CLOUDPROTOCOL_CLOUDPROTOCOL_H_ */
