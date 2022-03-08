/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-08-03    chenyong      the first version
 */

#include <rtthread.h>
#include <webclient.h>
#include <rtdbg.h>

#define GET_HEADER_BUFSZ               1024
#define GET_RESP_BUFSZ                 1024

#define GET_LOCAL_URI                  "http://www.rt-thread.com/service/rt-thread.txt"

/* send HTTP GET request by common request interface, it used to receive longer data */
static int webclient_get_comm(const char *uri)
{
    struct webclient_session* session = RT_NULL;
    unsigned char *buffer = RT_NULL;
    int index, ret = 0;
    int bytes_read, resp_status;
    int content_length = -1;

    buffer = (unsigned char *) web_malloc(GET_RESP_BUFSZ);
    if (buffer == RT_NULL)
    {
        LOG_E("no memory for receive buffer.");
        ret = -RT_ENOMEM;
        goto __exit;

    }

    /* create webclient session and set header response size */
    session = webclient_session_create(GET_HEADER_BUFSZ);
    if (session == RT_NULL)
    {
        ret = -RT_ENOMEM;
        goto __exit;
    }

    /* send GET request by default header */
    if ((resp_status = webclient_get(session, uri)) != 200)
    {
        LOG_E("webclient GET request failed, response(%d) error.", resp_status);
        ret = -RT_ERROR;
        goto __exit;
    }

    LOG_I("webclient get response data: ");

    content_length = webclient_content_length_get(session);
    if (content_length < 0)
    {
        LOG_E("webclient GET request type is chunked.");
        do
        {
            bytes_read = webclient_read(session, (void *)buffer, GET_RESP_BUFSZ);
            if (bytes_read <= 0)
            {
                break;
            }

            for (index = 0; index < bytes_read; index++)
            {
                LOG_I("%c", buffer[index]);
            }
        } while (1);

    }
    else
    {
        int content_pos = 0;

        do
        {
            bytes_read = webclient_read(session, (void *)buffer,
                    content_length - content_pos > GET_RESP_BUFSZ ?
                            GET_RESP_BUFSZ : content_length - content_pos);
            if (bytes_read <= 0)
            {
                break;
            }

            for (index = 0; index < bytes_read; index++)
            {
                LOG_I("%c", buffer[index]);
            }

            content_pos += bytes_read;
        } while (content_pos < content_length);

    }

__exit:
    if (session)
    {
        webclient_close(session);
    }

    if (buffer)
    {
        web_free(buffer);
    }

    return ret;
}

/* send HTTP GET request by simplify request interface, it used to received shorter data */
static int webclient_get_smpl(const char *uri)
{
    char *response = RT_NULL;
    size_t resp_len = 0;
    int index;

    if (webclient_request(uri, RT_NULL, RT_NULL, 0, (void **)&response, &resp_len) < 0)
    {
        LOG_E("webclient send get request failed.");
        return -RT_ERROR;
    }

    LOG_I("webclient send get request by simplify request interface.");
    LOG_I("webclient get response data: ");
    for (index = 0; index < rt_strlen(response); index++)
    {
        LOG_I("%c", response[index]);
    }

    if (response)
    {
        web_free(response);
    }

    return 0;
}


int webclient_get_test(int argc, char **argv)
{
    char *uri = RT_NULL;

    if (argc == 1)
    {
        uri = web_strdup(GET_LOCAL_URI);
        if(uri == RT_NULL)
        {
            LOG_E("no memory for create get request uri buffer.");
            return -RT_ENOMEM;
        }

        webclient_get_comm(uri);
    }
    else if (argc == 2)
    {
        if (rt_strcmp(argv[1], "-s") == 0)
        {
            uri = web_strdup(GET_LOCAL_URI);
            if(uri == RT_NULL)
            {
                LOG_E("no memory for create get request uri buffer.");
                return -RT_ENOMEM;
            }

            webclient_get_smpl(uri);
        }
        else
        {
            uri = web_strdup(argv[1]);
            if(uri == RT_NULL)
            {
                LOG_E("no memory for create get request uri buffer.");
                return -RT_ENOMEM;
            }
            webclient_get_comm(uri);
        }
    }
    else if(argc == 3 && rt_strcmp(argv[1], "-s") == 0)
    {
        uri = web_strdup(argv[2]);
        if(uri == RT_NULL)
        {
            LOG_E("no memory for create get request uri buffer.");
            return -RT_ENOMEM;
        }

        webclient_get_smpl(uri);
    }
    else
    {
        LOG_I("web_get_test [URI]     - webclient GET request test.");
        LOG_I("web_get_test -s [URI]  - webclient simplify GET request test.");
        return -RT_ERROR;
    }

    if (uri)
    {
        web_free(uri);
    }

    return RT_EOK;
}

#ifdef FINSH_USING_MSH
#include <finsh.h>
MSH_CMD_EXPORT_ALIAS(webclient_get_test, web_get_test, webclient get request test);
#endif /* FINSH_USING_MSH */
