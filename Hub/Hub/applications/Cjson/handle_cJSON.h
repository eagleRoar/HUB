#ifndef __HANDLE_CJSON_H__
#define __HANDLE_CJSON_H__
#include "cJSON.h"
#include "string.h"
#include "stdint.h"
#include "stdlib.h"

typedef uint8_t (*rt_cjson_handle_cb_t)(cJSON *json);

struct rt_cjson_handle_hook
{
    rt_cjson_handle_cb_t cb;
    struct rt_cjson_handle_hook *next;
};

typedef struct rt_cjson_handle
{
    char *name;
    struct rt_cjson_handle_hook *hook;
    struct rt_cjson_handle *list;
    struct rt_cjson_handle *next;
} * rt_cjson_handle_t;

//  处理类 接口
void rt_cjson_handle(cJSON *json);
void rt_cjson_handle_from_item(cJSON *json,char *item_name);

//  功能类 接口(添加，查找，删除)
rt_cjson_handle_t rt_cjson_handle_add(char *name);
rt_cjson_handle_t rt_cjson_handle_add_to_parent(rt_cjson_handle_t parent, char *name);
rt_cjson_handle_t rt_cjson_handle_find(rt_cjson_handle_t handle, char *name);
void rt_cjson_handle_remove(char *name);

//  功能类 接口(添加钩子函数)
void rt_cjson_handle_add_hook(rt_cjson_handle_t handle, rt_cjson_handle_cb_t cb);
rt_cjson_handle_t rt_cjson_handle_add_hook_to_parent(rt_cjson_handle_t parent, char *name, rt_cjson_handle_cb_t cb);

#endif
