/****************************************Copyright (c)****************************************************
**                             �� �� �� �� �� �� �� �� �� �� �� ˾
**                                http://www.huaning-iot.com
**                                http://hichard.taobao.com
**
**
**--------------File Info---------------------------------------------------------------------------------
** File Name:           mqtt_client.h
** Last modified Date:  2019-10-25
** Last Version:        v1.0
** Description:         mqtt�ͻ������ʵ��
**
**--------------------------------------------------------------------------------------------------------
** Created By:          Renhaibo�κ���
** Created date:        2019-10-25
** Version:             v1.0
** Descriptions:        The original version ��ʼ�汾
**
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
** Version:
** Description:
**
*********************************************************************************************************/
#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************
**  ͷ�ļ�����
*********************************************************************************************************/
#include <stdint.h>
#include <rtthread.h>
#include "MQTTPacket.h"
#include "gpio.h"

#ifdef MQTT_USING_TLS
#include <tls_client.h>
#endif

/*********************************************************************************************************
**  һЩ����
*********************************************************************************************************/
// ��������ĵ�������������
#ifdef MQTT_MAX_MESSAGE_HANDLERS
#define MAX_MESSAGE_HANDLERS    MQTT_MAX_MESSAGE_HANDLERS
#else 
#define MAX_MESSAGE_HANDLERS    4
#endif
  
#ifdef MQTT_USING_TLS
#define MQTT_TLS_READ_BUFFER    4096
#endif
  
// MQTT Packet ID���ֵ���壬���ֵ����MQTT��׼�淶�����������޸�
#define MAX_PACKET_ID           65535
// TLS���ܴ���ʱʹ�ã����ڽ��ճ�ʱ
#define MQTT_SOCKET_TIMEO       5000

#define MQTT_URI                "tcp://192.168.0.222:1883"//"tcp://192.168.0.29:61613"//
#define MQTT_USERNAME           "hydro"//"admin"//
#define MQTT_PASSWORD           "hydro"//"password"
//#define MQTT_SUBTOPIC           "SN/ctr"
//#define MQTT_PUBTOPIC           "SN/reply"
#define MQTT_WILLMSG            "Goodbye!"

/*********************************************************************************************************
**  MQTT��Ϣ��·����ֵ����
*********************************************************************************************************/
enum QoS { QOS0, QOS1, QOS2, SUBFAIL=0x80 };

/*********************************************************************************************************
**  ����ֵ����������б���Ϊ��ֵ
*********************************************************************************************************/
enum returnCode { PAHO_BUFFER_OVERFLOW = -2, PAHO_FAILURE = -1, PAHO_SUCCESS = 0};

/*********************************************************************************************************
**  MQTT�ͻ�������CMDֵ���壬��ʱֵ������Ϊ��λ
*********************************************************************************************************/
enum mqttControl
{
    MQTT_CTRL_SET_CONN_TIMEO = 0,      /* set mqtt connect timeout */  
    MQTT_CTRL_SET_MSG_TIMEO,           /* set mqtt msg timeout */  
    MQTT_CTRL_SET_RECONN_INTERVAL,     /* set reconnect interval   */  
    MQTT_CTRL_SET_KEEPALIVE_INTERVAL,  /* set keepalive interval   */  
    MQTT_CTRL_SET_KEEPALIVE_COUNT      /* set keepalive count      */
}; 

/*********************************************************************************************************
**  MQTT��Ϣ�ṹ����
*********************************************************************************************************/
typedef struct mqtt_message
{
  enum QoS qos;
  unsigned char retained;
  unsigned char dup;
  unsigned short id;
  void *payload;
  size_t payloadlen;
} mqtt_message;

typedef struct message_data
{
  mqtt_message* message;
  MQTTString* topic_name;
} message_data;

/*********************************************************************************************************
**  ����һ����Ϣ�ṹ�����ڵȴ�������Ӧ��Ļ�Ӧ
*********************************************************************************************************/
#pragma pack(1)
typedef struct mqtt_meaasge_ack {
  uint16_t packet_id;             // mqtt package id
  uint8_t msg_type;                // mqtt ��Ϣ����
} mqtt_message_ack;
#pragma pack()

/*********************************************************************************************************
**  MQTT�ͻ��˽ṹ����
*********************************************************************************************************/

struct mqtt_client
{
  // ���²�����Ҫ��ʼ�����������mqtt�ͻ���Ӧ��
  const char *uri;
  uint8_t is_quit;           // �Ƿ��˳��������ʼ��Ϊ0
  MQTTPacket_connectData condata;
  size_t buf_size, readbuf_size;
  
  // ���²��������ȳ�ʼ��������mqtt�ͻ���Ӧ�ã�Ҳ����������ͨ��paho_mqtt_control����
  unsigned int keepalive_interval;      // keepalive���������Ϊ��λ
  uint16_t keepalive_count;              // keepalive�����������ô�����Ӧ����ر�����
  int connect_timeout;                  // ���ӳ�ʱ������Ϊ��λ
  int reconnect_interval;               // �������Ӽ��������Ϊ��λ
  int msg_timeout;                      // ��Ϣͨ�ų�ʱ������Ϊ��λ�������������������Ϊ0
  
  //�����²�����Ӧ�ñ���������Ҫ��ʼ��
  unsigned int next_packetid;
  int sock;
  unsigned char *buf, *readbuf;
  int isconnected;
  uint16_t keepalive_counter;
  uint32_t tick_ping;
  
  // ���²�����һЩ�ص���������Ҫ��ʼ�����������mqtt�ͻ���Ӧ��
  void (*connect_callback)(mqtt_client *);
  void (*online_callback)(mqtt_client *);
  void (*offline_callback)(mqtt_client *);
  
  // ���²����Ƕ�������Ļص���������Ҫ��ʼ����������mqtt�ͻ���Ӧ��
  struct message_handlers
  {
    char *topicFilter;
    void (*callback)(mqtt_client *, message_data *);
    enum QoS qos;
  } message_handlers[MAX_MESSAGE_HANDLERS];      /* Message handlers are indexed by subscription topic */
  void (*default_message_handlers) (mqtt_client *, message_data *);
  
  //�����²�����Ӧ�ñ���������Ҫ��ʼ��
  rt_mutex_t mqtt_lock;                          /* mqtt lock */
  rt_mq_t    msg_queue;                         /* ���ڴ���Ӧ��� */
#ifdef MQTT_USING_TLS
    MbedTLSSession *tls_session;                /* mbedtls session struct */
#endif
};

/* subscribe topic receive data callback */
typedef void (*subscribe_cb)(mqtt_client *client, message_data *data);

/*********************************************************************************************************
** �ⲿ��������
*********************************************************************************************************/
extern int paho_mqtt_start(mqtt_client *client, rt_uint8_t  priority);
extern int paho_mqtt_stop(mqtt_client *client);
extern int paho_mqtt_subscribe(mqtt_client *client, enum QoS qos, const char *topic, subscribe_cb callback);
extern int paho_mqtt_unsubscribe(mqtt_client *client, const char *topic);
extern int paho_mqtt_publish(mqtt_client *client, enum QoS qos, const char *topic, void *payload, size_t length);
extern int paho_mqtt_is_connected(mqtt_client *client);
extern int paho_mqtt_control(mqtt_client *client, int cmd, void *arg);
mqtt_client *GetMqttClient(void);
int mqtt_start(void);
void SetRecvMqttFlg(u8);
int GetMqttStartFlg(void);
u8 GetRecvMqttFlg(void);
#ifdef __cplusplus
    }           // __cplusplus
#endif

#endif          // endif of __MQTTCLIENT_H__

/*********************************************************************************************************
** END FILE
*********************************************************************************************************/
