/*
 * Copyright (c) 2006-2018 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-11-27   ZhouXiaomin     first version
 */

/*include*******************************************************/
#include <rtthread.h>
/**************************************************************/
#include "uuzOpt.h"
#include "uuzUart.h"
/**************************************************************/
#include "uuzBBL.h"
#include "uuzConfigBBL.h"
#include "uuzConfigMBR.h"
#include "uuzMBR.h"
#include "uuzEventDEVID.h"
/**************************************************************/
#include "uuzDevCFG.h"
#include "uuzEventDEV.h"
/**************************************************************/
#include "uuzEventUART.h"
#include "uuzConfigHMI.h"
#include "uuzEventHMI.h"
#include "uuzLine1.h"
#include "uuzEXLDA1Event.h"
/**************************************************************/
#define DBG_ENABLE
#define DBG_SECTION_NAME "L1|R"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>
/**************************************************************/
#if defined(BSP_USING_UART)
#if defined(BSP_USING_UART3)
/**
 * @brief line1_uart_input
 * @param dev
 * @param size
 * @return
 */
rt_err_t line1_uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    //读取缓存数据
    rt_sem_release(&xUartE.uart[uuzUART_3].rx_sem);
    return RT_EOK;
}

#if 0
/**
 * @brief line1_thread_entry
 * @param parameter
 */
void line1_thread_entry(void* parameter)
{
    typedef_Uart* uart = (typedef_Uart*)parameter;
    u8 ucProtocol = uuzPROTOCOL_NULL;

    while (1) {
        if (uart->readylen) {
            if (uart->len >= 5) {
                /* Read Head Code is 'BBL' */
                ucProtocol = uuz_ucProtocolHeadByIsAssert(uart->rxbuff[0], uuzUART_LINE1);

                if (ucProtocol != uuzPROTOCOL_NULL) {
                    //达到检测长度，进行CRC16对比（LSB）
                    u16 usUhCRC = usModbusRTU_CRC(uart->rxbuff, (uart->len - 2));//计算当前的CRC16
                    u16 usRdCRC = usU8ToU16((uart->rxbuff + (uart->len - 2)), uuzLSB);//读取协议中的CRC16
#if 1
                    rt_kprintf("LDA-1-1:");
                    for (u8 ucIndex = 0; ucIndex < uart->len; ucIndex++) {
                        //一行最大长度24
                        if ((ucIndex != 0) && ((ucIndex % 24) == 0)) {
                            rt_kprintf("\r\n");
                        }
                        rt_kprintf("%02X ", uart->rxbuff[ucIndex]);
                    }
                    rt_kprintf("\r\n");
#endif
                    //如果CRC校验成功
                    if (usUhCRC == usRdCRC) {
                        uuz_vLDA1ReceiveFromUSART(uart->rxbuff, uart->txbuff, uuzUART_LINE1);
                    }
                }
                //清空接收区
                uart->len = 0;
            }
            //清空接收标记
            uart->readylen = 0;
        }
        rt_thread_mdelay(10);
    }
}
#endif

/**
 * @brief uuz_vLDA1ReceiveFromUSART
 * @param ucRxCode
 * @param ucTxCode
 * @param ucUart
 */
void uuz_vLDA1ReceiveFromUSART(u8* ucRxCode, u8* ucTxCode, u8 ucUart)
{
    // Init Temp Data
    u8 ucRegSta = 0;
    u8 ucBroadcastID[4] =
                { 0x00, 0x00, 0x00, 0x00 };

    if (ucRxCode) {
        //读取设备的modbus和对应设备号
        u8 ucHead = ucRxCode[0];
        if (ucHead == uuzBBL_HEAD) {
            u8 ucOrder = ucRxCode[1];
            u8 ucID[4];
            rt_memcpy(ucID, (ucRxCode + 2), 4);
            u8 ucRegID[4];
            rt_memcpy(ucRegID, (ucRxCode + 9), 4);
            u8 ucModbusID = ucRxCode[7];
            u8 ucSubType = ucRxCode[8];
            u8 ucDevModbusID = uuzDEV_TEST;
            //0:没有动作
            //1:更新设备的Modbus ID
            u8 ucRegToDo = 0;

            // BBL PROTOCOL
            switch (ucOrder) {
                case (uuzBBL_SEND_ID_REG | uuzBBL_ACK):
                    //接收其他主机发来的请求指令
                    if ((ucID[0] == 0xFF) && (ucID[1] == 0xFF) && (ucID[2] == 0xFF) && (ucID[3] == 0xFF)) {
                        //判断目标主机ID是否和本机一致
                        if ((rt_memcmp(xDeviceConfig.xSystemInfo.ucID, ucRegID, 4) != 0)) {
                            //在传感器线路上
                            //提示总线有多台主机，并向其他主机发送错误提示
                            uuz_vIDTipErr(ucUart, ucRegID);
                            // 有多台主机，需要在屏幕上显示数据
                            uuz_vDevPopTipToHmi(uuzHMI_HOST_REPEAT);
                        }
                    }
                    break;
                case (uuzBBL_SEND_ID_REG | uuzBBL_REPLY):
                    //判断目标主机ID是否和本机一致
                    if ((rt_memcmp(ucID, xDeviceConfig.xSystemInfo.ucID, 4) == 0)) {
                        //提示总线有多台主机，并向屏幕发送错误提示的回复,不需要再次回复
                        uuz_vDevPopTipToHmi(uuzHMI_HOST_REPEAT);
                    }
                    break;
                case (uuzBBL_BROADCAST_ID | uuzBBL_ACK):
                    //接收注册协议
                    LOG_I("Recvice by ACK");
                    // 是slave设备主动发来的数据
                    if (rt_memcmp(ucID, ucBroadcastID, 4) == 0) {
                        ucRegToDo = 1;
                    }
                    break;
                case (uuzBBL_BROADCAST_ID | uuzBBL_REPLY):
                    LOG_I("Recvice by REPLY");
                    if (!(rt_memcmp(xDeviceConfig.xSystemInfo.ucID, ucID, 4))) {
                        ucRegToDo = 1;
                    }
                    break;
                default:
                    break;
            }  //switch (ucOrder)

            //有需要更新的数据
            if (ucRegToDo == 1) {
                ucRegSta = RT_FALSE;

                //判断设备类型是否在范围内
                if (ucSubType == uuzDEV_SL_LDA1) {
                    //判断是否有重复的ID数据，返回0xFFU表示没有找到相关数据
                    //如果没有找到注册设备信息
                    ucRegSta = dev_list_cpu_is_repeat(ucRegID);
                    if (ucRegSta == 0xFFU) {
                        //是未在该设备注册的有效设备
                        ucRegSta = uuz_ucDevice_WaitIDIsRepeat(ucRegID);
                        if (ucRegSta == 0xFFU) {
                            //根据原始数据判断ID
                            ucDevModbusID = uuz_ucDevice_GetID(ucModbusID, ucUart);
                            if (ucDevModbusID != 0xFFU) {
                                ucRegSta = uuz_ucDevice_WaitIDAdd(ucRegID, ucDevModbusID,
                                        ucSubType, ucUart);
                            }
                        } else {
                            //有待注册数据，不用重新提示
                            uuz_vDevice_WaitIDReset(&xDevState.xWaitRegID[ucRegSta]);
                        }
                    } else {
                        //加载配置的设备数据ID
                        ucDevModbusID = xDevIDs.xDev[ucRegSta].ucModbusID;
                        //如果发来的Modbus-ID数据和目标数据不一致
                        if (ucDevModbusID != ucModbusID) {
                            //重置Modbus-ID的数据
                            uuz_vIDReply(xDevIDs.xDev[ucRegSta].ucUsart, xDevIDs.xDev[ucRegSta].ucID,
                                    xDevIDs.xDev[ucRegSta].ucModbusID);
                        }

                        dev_single_reset(&xDevIDs.xDev[ucRegSta]);                    //重置连接状态
                        xDevL.update(uuzDEV_SL_LDA1);                    //更新连接数据
                    }
                } else {
                    //提示未知设备注册失败，不需要再次回复
                    uuz_vIDRegErr(ucUart, ucRegID);
                    //屏幕发送设备串口插入错误的回复
                    uuz_vDevPopTipToHmi(uuzHMI_LINE_ERR);
                }
            }
        } else {
            // MODBUS-RTU RPOTOCOL
            u8 ucTxHead = ucTxCode[0];
            u16 usTxRegAddr = usU8ToU16(ucTxCode + 2, uuzMSB);

            //发送端和接收端相同
            if (ucHead == ucTxHead) {
                //读取发送参数
                Device_Typedef_t *xDev = NULL;
                u8 ucOpt = ucRxCode[1];
                u8 ucIndex = ucIDtoVal(xDev, ucHead, uuzDEV_SL_LDA1, ucUart);

                //没有错误数据
                if (ucIndex < uuzDEV_LT1_MAX) {
                    ExLDA1Value_Typedef_t* xValue;
                    Device_Typedef_t* xDevT;
                    switch (ucOpt) {
                        case uuzMBR_READ_HOLDING_REGISTER:
                            //获取到相关数据
                            if (usTxRegAddr == uuzADDR_RW_LT_STA_DATA) {
                                //更新LDA1的状态
                                if (ucUart == uuzUART_LINE1) {
                                    xValue = &xDevValue.xValue_LT1[ucIndex];
                                    xDevT = &xDevState.xLT1[ucIndex];
                                } else {
                                    xValue = &xDevValue.xValue_LT2[ucIndex];
                                    xDevT = &xDevState.xLT2[ucIndex];
                                }

                                //获取实时数据
                                uuz_vExLDA1DataGet((ucRxCode + 3), xValue);
                                //更新连接状态
                                dev_single_reset(xDevT);
                                xDevL.update(uuzDEV_SL_LDA1);
                            }
                            break;
                        case uuzMBR_WRITE_REGISTER:
                            if (usTxRegAddr == uuzADDR_RW_LT_STA_DATA) {
                                //获取实时数据
                                if (ucUart == uuzUART_LINE1) {
                                    xValue = &xDevValue.xValue_LT1[ucIndex];
                                    xDevT = &xDevState.xLT1[ucIndex];
                                } else {
                                    xValue = &xDevValue.xValue_LT2[ucIndex];
                                    xDevT = &xDevState.xLT2[ucIndex];
                                }

                                uuz_vExLDA1DataGet((ucRxCode + 4), xValue);
                                //更新连接状态
                                dev_single_reset(xDevT);
                                xDevL.update(uuzDEV_SL_LDA1);
                            }
                            break;
                        default:
                            break;
                    }
                }
            } else {
                LOG_E(
                        "The data header[0x%02X] to be received is inconsistent with the sender[0x%02X]",
                        ucHead,
                        ucTxHead);
            }
        }
    }
}
#endif /* BSP_USING_UART3 */
#endif /* BSP_USING_UART */
