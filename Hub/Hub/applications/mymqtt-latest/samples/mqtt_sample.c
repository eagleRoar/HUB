#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <rtthread.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME    "mqtt.sample"
#define DBG_LEVEL           DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "mqtt_client.h"
#include "Gpio.h"
#include "cJSON.h"

__attribute__((section(".ccmbss"))) type_mqtt_ip mqtt_use;

void InitMqttUrlUse(void)
{
    mqtt_use.mqtt_url_use = USE_AMAZON;
    strcpy(mqtt_use.use_ip, "0.0.0.0");
}

type_mqtt_ip *getMqttUrlUse(void)
{
    return &mqtt_use;
}

u8 GetMqttUse(void)
{
    return mqtt_use.mqtt_url_use;
}

void setMqttUse(u8 use)
{
    mqtt_use.mqtt_url_use = use;
}

void setMqttUseIp(char *ip)
{
    strcpy(mqtt_use.use_ip, ip);
}

char *getMqttUseIp(void)
{
    return mqtt_use.use_ip;
}

/**
 * MQTT URI farmat:
 * domain mode
 * tcp://iot.eclipse.org:1883
 *
 * ipv4 mode
 * tcp://192.168.10.1:1883
 * ssl://192.168.10.1:1884
 *
 * ipv6 mode
 * tcp://[fe80::20c:29ff:fe9a:a07e]:1883
 * ssl://[fe80::20c:29ff:fe9a:a07e]:1884
 */

/* define MQTT client context */
static mqtt_client client;
static int is_started = 0;

static u8 startMqttFlg = 0;
static u8 recvMqttFlg = 0;

int GetMqttStartFlg(void)
{
    return is_started;
}

u8 GetStartMqttFlg(void)
{
    return startMqttFlg;
}

u8 GetRecvMqttFlg(void)
{
    return recvMqttFlg;
}

void SetRecvMqttFlg(u8 flag)
{
    recvMqttFlg = flag;
}

mqtt_client *GetMqttClient(void)
{
    return &client;
}

static void mqtt_sub_callback(mqtt_client *c, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
//    LOG_D("mqtt sub callback: %.*s %.*s",
//               msg_data->topic_name->lenstring.len,
//               msg_data->topic_name->lenstring.data,
//               msg_data->message->payloadlen,
//               (char *)msg_data->message->payload);

    analyzeCloudData((char *)msg_data->message->payload, YES);
    SetRecvMqttFlg(ON);
}

static void mqtt_sub_default_callback(mqtt_client *c, message_data *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
//    LOG_D("mqtt sub default callback: %.*s %.*s",
//               msg_data->topic_name->lenstring.len,
//               msg_data->topic_name->lenstring.data,
//               msg_data->message->payloadlen,
//               (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_connect_callback!");
}

static void mqtt_online_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_online_callback!");
}

static void mqtt_offline_callback(mqtt_client *c)
{
    LOG_D("inter mqtt_offline_callback!");
}

char   url[50]  = " ";
int mqtt_start(void)
{
    /* init condata param by using MQTTPacket_connectData_initializer */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[20] = { 0 };
    static char name[20];

    if (is_started)
    {
        LOG_E("mqtt client is already connected.");
        return -1;
    }
    /* config MQTT context param */
    {
        client.isconnected = 0;

        //按照选择判断
        while(YES != GetFileSystemState());

        if(USE_ALIYUN == GetMqttUse())
        {
            strcpy(url, "tcp://mqtt.pro-leaf.cn:1883");
        }
        else if(USE_IP == GetMqttUse())
        {
            sprintf(url, "%s%s%s", "tcp://", getMqttUrlUse()->use_ip, ":1883");
        }
        else
        {
            //亚马逊
            strcpy(url, "tcp://mqtt.pro-leaf.com:1883");
        }
        client.uri = url;

        /* generate the random client ID */
//        rt_snprintf(cid, sizeof(cid), "rtthread%d", rt_tick_get());
        GetSnName(cid, 12);
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.username.cstring = MQTT_USERNAME;
        client.condata.password.cstring = MQTT_PASSWORD;
        client.condata.keepAliveInterval = 60;//30;
        client.condata.cleansession = 1;

        /* config MQTT will param. */
        client.condata.willFlag = 1;
        client.condata.will.qos = 1;
        client.condata.will.retained = 0;
        rt_memset(name, ' ', 20);
        GetSnName(name, 12);
        strcpy(name + 11, "/reply");
        client.condata.will.topicName.cstring = name;
        client.condata.will.message.cstring = MQTT_WILLMSG;

        /* malloc buffer. */
        client.buf_size = client.readbuf_size = /*1024*/1024 * 3;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf))
        {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }

        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        rt_memset(name, ' ', 20);
        GetSnName(name, 12);
        strcpy(name + 11, "/ctr");
        client.message_handlers[0].topicFilter = rt_strdup(name);
        client.message_handlers[0].callback = mqtt_sub_callback;
        client.message_handlers[0].qos = QOS1;

        startMqttFlg = 1;

        /* set default subscribe event callback */
        client.default_message_handlers = mqtt_sub_default_callback;
    }
    
    {
      int value;
      uint16_t u16Value;
      value = 5;
      paho_mqtt_control(&client, MQTT_CTRL_SET_CONN_TIMEO, &value);
      value = 5;
      paho_mqtt_control(&client, MQTT_CTRL_SET_MSG_TIMEO, &value);
      value = 5;
      paho_mqtt_control(&client, MQTT_CTRL_SET_RECONN_INTERVAL, &value);
      value = 30;
      paho_mqtt_control(&client, MQTT_CTRL_SET_KEEPALIVE_INTERVAL, &value);
      u16Value = 3;
      paho_mqtt_control(&client, MQTT_CTRL_SET_KEEPALIVE_COUNT, &u16Value);
    }

    /* run mqtt client */
    paho_mqtt_start(&client, 20);

    is_started = 1;

    return 0;
}


