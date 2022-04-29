#ifndef __PARSE_CJSON_H__
#define __PARSE_CJSON_H__
#include "cJSON.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

typedef  struct rt_cjson_parse{
    char *p;
    char *buf;
    uint16_t len;
    uint16_t count;                             //  接受多少个
    uint8_t last_flag;                          //  上一个标志
    // uint8_t comma_flag;                     //  ,
    uint8_t receive_state;                      //  接收状态 true:正在接收
    uint8_t Angle_brackets_left_flag;           //  {
    uint8_t Angle_brackets_right_flag;          //  }
    uint8_t middle_brackets_left_flag;          //  [
    uint8_t middle_brackets_right_flag;         //  ]
    uint8_t double_quotation_marks_left_flag;   //  "(左)
    uint8_t double_quotation_marks_right_flag;  //  "(右)
} rt_cjson_parse_t;

//  创建一个 parse json cache 处理缓冲区
rt_cjson_parse_t * rt_cjson_parse_create(uint16_t buf_len);
//  释放处理缓冲区
void rt_cjson_parse_free(rt_cjson_parse_t *cache);
//  往 处理缓冲区中添加一个字符
cJSON * rt_cjson_parse_ch(rt_cjson_parse_t *cache ,char ch);
//  用处理缓冲区解析字符串
cJSON * rt_cjson_parse_str(rt_cjson_parse_t *cache ,char *str);

#endif
