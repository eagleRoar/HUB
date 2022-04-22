/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-25     Administrator       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <rtdbg.h>

#ifndef APPLICATIONS_ETHERNET_BUSINESSLAYER_TCPCLIENT_H_
#define APPLICATIONS_ETHERNET_BUSINESSLAYER_TCPCLIENT_H_

typedef void (*rx_cb_t)(void *buff, rt_size_t len);
typedef struct rt_tcpclient rt_tcpclient_t;

struct tcpClientStruct *GetTcpClientStruct(void);
void InitTcpclientStruct(void);
rt_tcpclient_t *rt_tcpclient_start(const char *hostname, rt_uint32_t port);
rt_err_t tcpclient_destory(rt_tcpclient_t *thiz);
rt_err_t rt_tcpclient_attach_rx_cb(rt_tcpclient_t *thiz, rx_cb_t cb);
rt_err_t rt_tcpclient_send(rt_tcpclient_t *thiz, const void *buff, rt_size_t len);
rt_err_t socket_reset(rt_tcpclient_t *thiz, const char *url, rt_uint32_t port);


#endif /* APPLICATIONS_ETHERNET_BUSINESSLAYER_TCPCLIENT_H_ */
