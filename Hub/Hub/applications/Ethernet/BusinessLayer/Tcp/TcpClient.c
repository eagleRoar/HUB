/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-25     Administrator       the first version
 */
#include "TcpClient.h"
#include "GlobalConfig.h"
#include "EthernetBusiness.h"
#include "Informationmonitor.h"

#define RX_CB_HANDLE(_buff, _len)  \
    do                             \
    {                              \
        if (thiz->rx)              \
            thiz->rx(_buff, _len); \
    } while (0)

struct rt_tcpclient
{
    int sock_fd;
    int pipe_read_fd;
    int pipe_write_fd;
    char pipe_name[8];
    rx_cb_t rx;
};


#define BUFF_SIZE (1024)
#define MAX_VAL(A, B) ((A) > (B) ? (A) : (B))
#define STRCMP(a, R, b) (strcmp((a), (b)) R 0)
#define     WRONG_NUM       -1

static rt_tcpclient_t *tcpclient_create(void);

static rt_err_t socket_init(rt_tcpclient_t *thiz, const char *hostname, rt_uint32_t port);
static rt_err_t socket_deinit(rt_tcpclient_t *thiz);
static rt_err_t pipe_init(rt_tcpclient_t *thiz);
static rt_err_t pipe_deinit(rt_tcpclient_t *thiz);
static void select_handle(rt_tcpclient_t *thiz, char *pipe_buff, char *sock_buff);
static rt_err_t tcpclient_thread_init(rt_tcpclient_t *thiz);
static void tcpclient_thread_entry(void *param);

extern struct ethDeviceStruct *eth;

char pipe_buff[BUFF_SIZE], sock_buff[BUFF_SIZE];
/**
 * 获取tcpClient 的实例化
 * @return
 */
static rt_tcpclient_t *tcpclient_create(void)
{
    rt_tcpclient_t *thiz = RT_NULL;

    thiz = rt_malloc(sizeof(rt_tcpclient_t));
    if (thiz == RT_NULL)
    {
        LOG_E("tcpclient_create fail");
        return RT_NULL;
    }

    thiz->sock_fd = WRONG_NUM;
    thiz->pipe_read_fd = WRONG_NUM;
    thiz->pipe_write_fd = WRONG_NUM;
    rt_memset(thiz->pipe_name, 0, sizeof(thiz->pipe_name));
    thiz->rx = RT_NULL;

    return thiz;
}

/**
 * socket 初始化
 * @param thiz
 * @param url
 * @param port
 * @return
 */
static rt_err_t socket_init(rt_tcpclient_t *thiz, const char *url, rt_uint32_t port)
{
    struct sockaddr_in dst_addr;
    struct hostent *hostname;
    rt_int32_t res = 0;

    if (thiz == RT_NULL)
    {
        LOG_E("socket_init 1");
        return RT_ERROR;
    }

    thiz->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (WRONG_NUM == thiz->sock_fd)
    {
        LOG_E("socket_init 2");
        return RT_ERROR;
    }

    hostname = gethostbyname(url);

    dst_addr.sin_family = AF_INET;
    dst_addr.sin_port = htons(port);
    dst_addr.sin_addr = *((struct in_addr *)hostname->h_addr);

    rt_memset(&(dst_addr.sin_zero), 0, sizeof(dst_addr.sin_zero));

    res = connect(thiz->sock_fd, (struct sockaddr *)&dst_addr, sizeof(struct sockaddr));
    if (WRONG_NUM == res )
    {
        LOG_E("socket_init 3");
        return RT_ERROR;
    }

    return RT_EOK;
}

rt_err_t socket_reset(rt_tcpclient_t *thiz, const char *url, rt_uint32_t port)
{
    if(WRONG_NUM != thiz->sock_fd)
    {
        socket_deinit(thiz);
    }

    return socket_init(thiz, url, port);
}

/**
 * 删除socket
 * @param thiz
 * @return
 */
static rt_err_t socket_deinit(rt_tcpclient_t *thiz)
{
    if (thiz == RT_NULL)
    {
        return RT_ERROR;
    }

    if(WRONG_NUM != thiz->sock_fd)
    {
        closesocket(thiz->sock_fd);
        //RT_ASSERT(res == 0);
        thiz->sock_fd = WRONG_NUM;
    }

    return RT_EOK;
}


/**
 * pipe管道初始化
 * @param thiz
 * @return
 */
static rt_err_t pipe_init(rt_tcpclient_t *thiz)
{
    char dev_name[32];
    static int pipeno = 0;
    rt_pipe_t *pipe = RT_NULL;

    if (thiz == RT_NULL)
    {
        LOG_D("pipe_init 1");
        return RT_ERROR;
    }

    snprintf(thiz->pipe_name, sizeof(thiz->pipe_name), "pipe%d", pipeno++);

    pipe = rt_pipe_create(thiz->pipe_name, PIPE_BUFSZ);
    if (pipe == RT_NULL)
    {
        LOG_D("pipe_init 2");
        return RT_ERROR;
    }

    snprintf(dev_name, sizeof(dev_name), "/dev/%s", thiz->pipe_name);
    thiz->pipe_read_fd = open(dev_name, O_RDONLY, 0);
    if (thiz->pipe_read_fd < 0)
    {
        LOG_E("pipe_init 3");
        goto fail_read;
    }

    thiz->pipe_write_fd = open(dev_name, O_WRONLY, 0);
    if (thiz->pipe_write_fd < 0)
    {
        LOG_E("pipe_init 4");
        goto fail_write;
    }

    LOG_D("pipe init succeed");
    return RT_EOK;

fail_write:
    close(thiz->pipe_read_fd);
fail_read:
    return RT_ERROR;
}

/**
 * 删除pipe 管道
 * @param thiz
 * @return
 */
static rt_err_t pipe_deinit(rt_tcpclient_t *thiz)
{
    if (thiz == RT_NULL)
    {
        return RT_ERROR;
    }

    if(WRONG_NUM != thiz->pipe_read_fd)
    {
        close(thiz->pipe_read_fd);
        //RT_ASSERT(res == 0);
        thiz->pipe_read_fd = WRONG_NUM;
    }

    if(WRONG_NUM != thiz->pipe_write_fd)
    {
        close(thiz->pipe_write_fd);
        //RT_ASSERT(res == 0);
        thiz->pipe_write_fd = WRONG_NUM;
    }

    rt_pipe_delete(thiz->pipe_name);

    return RT_EOK;
}

static rt_err_t tcpclient_thread_init(rt_tcpclient_t *thiz)
{
    rt_thread_t tcpclient_tid = RT_NULL;

    tcpclient_tid = rt_thread_create(TCP_CLIENT_TASK, tcpclient_thread_entry, thiz, /*2048*/1024*2, 12, 10);
    if (tcpclient_tid == RT_NULL)
    {
        LOG_E("tcpclient_thread_init fail");
        return RT_ERROR;
    }

    rt_thread_startup(tcpclient_tid);

    return RT_EOK;
}

static void select_handle(rt_tcpclient_t *thiz, char *pipe_buff, char *sock_buff)
{
    fd_set fds;
    rt_int32_t max_fd = 0, res = 0;

    max_fd = MAX_VAL(thiz->sock_fd, thiz->pipe_read_fd) + 1;
    FD_ZERO(&fds);

    while (1)
    {
        eth->tcp.SetConnectStatus(ON);
        eth->tcp.SetConnectTry(OFF);

        FD_SET(thiz->sock_fd, &fds);
        FD_SET(thiz->pipe_read_fd, &fds);

        res = select(max_fd, &fds, RT_NULL, RT_NULL, RT_NULL);

        /* socket is ready */
        if (FD_ISSET(thiz->sock_fd, &fds))
        {
            if(WRONG_NUM == thiz->sock_fd)
            {
                /* 可能由于socket正在更换 */
                continue;
            }
            res = recv(thiz->sock_fd, sock_buff, BUFF_SIZE, 0);

            if(res <= 0)
            {
                goto exit;
            }

            /* have received data, clear the end */
            sock_buff[res] = '\0';
            RX_CB_HANDLE(sock_buff, res);
        }

        /* pipe is read */
        if (FD_ISSET(thiz->pipe_read_fd, &fds))
        {
            /* read pipe */
            res = read(thiz->pipe_read_fd, pipe_buff, BUFF_SIZE);

            if(res <= 0)
            {
                goto exit;
            }

            /* write socket */
            send(thiz->sock_fd, pipe_buff, res, 0);
        }

        rt_thread_mdelay(20);
    }
exit:
    LOG_D("destory select");
    tcpclient_destory(thiz);
}

static void tcpclient_thread_entry(void *param)
{
    rt_tcpclient_t *temp = param;

    rt_memset(sock_buff, 0, BUFF_SIZE);
    rt_memset(pipe_buff, 0, BUFF_SIZE);

    select_handle(temp, pipe_buff, sock_buff);
}

rt_tcpclient_t *rt_tcpclient_start(const char *hostname, rt_uint32_t port)
{
    rt_tcpclient_t *thiz = RT_NULL;

    thiz = tcpclient_create();
    if (thiz == RT_NULL)
    {
        LOG_E("rt_tcpclient_start 1");
        return RT_NULL;
    }

    if(RT_EOK != socket_init(thiz, hostname, port))
    {
        LOG_E("rt_tcpclient_start 2");
        goto quit;
    }

    if(RT_EOK != pipe_init(thiz))
    {
        LOG_E("rt_tcpclient_start 3");
        goto quit;
    }

    if (RT_EOK != tcpclient_thread_init(thiz))
    {
        LOG_E("rt_tcpclient_start 4");
        goto quit;
    }

    return thiz;

quit:
    tcpclient_destory(thiz);
    return RT_NULL;
}

/**
 * tcpClient 实例删除
 * @param thiz
 * @return
 */
rt_err_t tcpclient_destory(rt_tcpclient_t *thiz)
{
//    rt_thread_t tcpThread = RT_NULL;
    LOG_I("tcpclient_destory");
    if (thiz == RT_NULL)
    {
        return RT_ERROR;
    }

    if (WRONG_NUM != thiz->sock_fd)
    {
        socket_deinit(thiz);
    }

    if ((WRONG_NUM != thiz->pipe_read_fd) ||
        (WRONG_NUM != thiz->pipe_write_fd))
    {
        pipe_deinit(thiz);
    }

//    tcpThread = rt_thread_find(TCP_CLIENT_TASK);//该操作引发bug
//    if(RT_NULL != tcpThread)
//    {
//        rt_thread_delete(tcpThread);
//    }

    if(RT_NULL != thiz)
    {

        rt_free(thiz);
        thiz = RT_NULL;
    }
    eth->tcp.SetConnectStatus(OFF);
    return RT_EOK;
}

rt_err_t rt_tcpclient_send(rt_tcpclient_t *thiz, const void *buff, rt_size_t len)
{
    rt_size_t bytes = 0;

    if (thiz == RT_NULL)
    {
        LOG_E("tcpclient send param is NULL");
        return RT_ERROR;
    }

    if (buff == RT_NULL)
    {
        LOG_E("tcpclient send buff is NULL");
        return RT_ERROR;
    }

    bytes = write(thiz->pipe_write_fd, buff, len);
    return bytes;
}

rt_err_t rt_tcpclient_attach_rx_cb(rt_tcpclient_t *thiz, rx_cb_t cb)
{
    if (thiz == RT_NULL)
    {
        LOG_D("callback attach param is NULL");
        return RT_ERROR;
    }

    thiz->rx = cb;
    LOG_D("callback attach attach succeed");
    return 0;
}
