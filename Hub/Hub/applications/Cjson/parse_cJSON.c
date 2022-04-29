#include "parse_cJSON.h"

#define DBG_TAG "parse.cjson"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

rt_cjson_parse_t *rt_cjson_parse_create(uint16_t buf_len)
{
    rt_cjson_parse_t *rt_cjson_parse_obj = (rt_cjson_parse_t *)malloc(sizeof(rt_cjson_parse_t));
    memset(rt_cjson_parse_obj, 0, sizeof(rt_cjson_parse_t));
    rt_cjson_parse_obj->buf = (char *)malloc(sizeof(char) * buf_len);
    rt_cjson_parse_obj->len = buf_len;
    rt_cjson_parse_obj->p = rt_cjson_parse_obj->buf;
    LOG_W("malloc memory size:%d", sizeof(rt_cjson_parse_t) + sizeof(char) * buf_len);
    return rt_cjson_parse_obj;
}
void rt_cjson_parse_free(rt_cjson_parse_t *rt_cjson_parse_obj)
{
    LOG_W("free memory size:%d", sizeof(rt_cjson_parse_t) + sizeof(char) * rt_cjson_parse_obj->len);
    free(rt_cjson_parse_obj->buf);
    free(rt_cjson_parse_obj);
}

void rt_cjson_parse_start(rt_cjson_parse_t *rt_cjson_parse_obj)
{
    rt_cjson_parse_obj->receive_state = 1; //  置为正在接收数据
    char *buf = rt_cjson_parse_obj->buf;
    uint16_t len = rt_cjson_parse_obj->len;
    memset(buf, 0, sizeof(char) * rt_cjson_parse_obj->len);
    memset(rt_cjson_parse_obj, 0, sizeof(rt_cjson_parse_t));
    rt_cjson_parse_obj->p = buf;
    rt_cjson_parse_obj->buf = buf;
    rt_cjson_parse_obj->len = len;
}

cJSON *rt_cjson_parse_end(rt_cjson_parse_t *rt_cjson_parse_obj, uint8_t error)
{
    cJSON *json = 0;
    if (!error)
    {
        rt_cjson_parse_obj->receive_state = 0;
        if (rt_cjson_parse_obj->count > 2)
            json = cJSON_Parse(rt_cjson_parse_obj->buf);
    }
    rt_cjson_parse_start(rt_cjson_parse_obj);
    return json;
}

#define RECEIVE_IS_NOT_JSON(rt_cjson_parse_obj, if_on)                    \
    do                                                                \
    {                                                                 \
        if (!(if_on))                                                 \
        {                                                             \
            *rt_cjson_parse_obj->p = '\0';                                \
            LOG_E("This is not JSON data:\n%s", rt_cjson_parse_obj->buf); \
            return rt_cjson_parse_end(rt_cjson_parse_obj, 1);                 \
        }                                                             \
    } while (0)

cJSON *rt_cjson_parse_ch(rt_cjson_parse_t *rt_cjson_parse_obj, char ch)
{
    //  可视字符区
    if (32 <= ch && ch <= 126)
    {
        //  json数据开始标志
        if (ch == '{')
        {
            //  开始接收数据
            if (rt_cjson_parse_obj->Angle_brackets_left_flag == 0)
                rt_cjson_parse_start(rt_cjson_parse_obj);
            rt_cjson_parse_obj->Angle_brackets_left_flag++;
        }
        else if (ch == '[')
        {
            //  开始接收数据
            if (rt_cjson_parse_obj->middle_brackets_left_flag == 0 && !rt_cjson_parse_obj->Angle_brackets_left_flag)
                rt_cjson_parse_start(rt_cjson_parse_obj);
            rt_cjson_parse_obj->middle_brackets_left_flag++;
        }

        //  两个开始标志
        if (rt_cjson_parse_obj->Angle_brackets_left_flag || rt_cjson_parse_obj->middle_brackets_left_flag)
        {
            //  缓存字符
            *rt_cjson_parse_obj->p++ = ch;
            //  若已接收到一个{ 就意味着开始记录数据了
            rt_cjson_parse_obj->count++;

            /*  --------------------------   ↓ 错误规则 ↓   --------------------------    */
            //  不在双引号中
            if (rt_cjson_parse_obj->double_quotation_marks_left_flag == rt_cjson_parse_obj->double_quotation_marks_right_flag)
            {
                switch (rt_cjson_parse_obj->last_flag)
                {
                case '{':
                {
                    RECEIVE_IS_NOT_JSON(rt_cjson_parse_obj, ch == '"');
                    break;
                }
                case '[':
                case ',':
                case ':':
                {
                    RECEIVE_IS_NOT_JSON(rt_cjson_parse_obj, ch == '"' || ch == '{' || ch == '[' || ('0' <= ch && ch <= '9'));
                    break;
                }
                case '}':
                case ']':
                {
                    RECEIVE_IS_NOT_JSON(rt_cjson_parse_obj, ch == ',' || ch == '}' || ch == ']');
                    break;
                }
                case '"':
                {
                    RECEIVE_IS_NOT_JSON(rt_cjson_parse_obj, ch == ':' || ch == ',' || ch == '}' || ch == ']');
                    break;
                }
                }
            }
            // //  处于 [ .... 中括号内容中
            // if (rt_cjson_parse_obj->middle_brackets_left_flag > rt_cjson_parse_obj->middle_brackets_right_flag)
            // {

            // }
            /*  --------------------------   ↑ 错误规则 ↑   --------------------------    */

            //  缓冲区溢出
            if (rt_cjson_parse_obj->count >= rt_cjson_parse_obj->len - 1)
            {
                *rt_cjson_parse_obj->p = '\0';
                LOG_E("Overflow parsing JSON:\n%s", rt_cjson_parse_obj->buf);
                return rt_cjson_parse_end(rt_cjson_parse_obj, 1);
            }

            //  记录标志
            switch (ch)
            {
            case '{':
            {
                break;
            }
            case '[':
            {
                break;
            }
            case '}':
            {
                rt_cjson_parse_obj->Angle_brackets_right_flag++;
                break;
            }
            case ']':
            {
                rt_cjson_parse_obj->middle_brackets_right_flag++;
                break;
            }
            case '"':
            {
                if (rt_cjson_parse_obj->double_quotation_marks_left_flag == rt_cjson_parse_obj->double_quotation_marks_right_flag)
                    rt_cjson_parse_obj->double_quotation_marks_left_flag++;
                else
                    rt_cjson_parse_obj->double_quotation_marks_right_flag++;
                break;
            }
            case ',':
            {
                break;
            }
            case ':':
            {
                break;
            }
            }
            rt_cjson_parse_obj->last_flag = ch;

            //  总标志判断，全部相等就结束
            if (
                (rt_cjson_parse_obj->Angle_brackets_right_flag == rt_cjson_parse_obj->Angle_brackets_left_flag) &&
                (rt_cjson_parse_obj->middle_brackets_right_flag == rt_cjson_parse_obj->middle_brackets_left_flag) &&
                (rt_cjson_parse_obj->double_quotation_marks_left_flag == rt_cjson_parse_obj->double_quotation_marks_right_flag))
            {
                return rt_cjson_parse_end(rt_cjson_parse_obj, 0);
            }
        }
    }
    return (cJSON *)1;
}

cJSON *rt_cjson_parse_str(rt_cjson_parse_t *rt_cjson_parse_obj, char *str)
{
    uint16_t count = strlen(str);
    if (count)
    {
        cJSON *json;
        do
        {
            json = rt_cjson_parse_ch(rt_cjson_parse_obj, *str++);
            if (json == 0)
                return (cJSON *)0;
            else if (json > (cJSON *)1)
                return json;
        } while (count--);
    }
    return (cJSON *)0;
}
