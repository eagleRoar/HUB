/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-06     Administrator       the first version
 */
#include "Recipe.h"

sys_recipe_t sys_recipe;

void initSysRecipe(void)
{
    rt_memset(GetSysRecipt(), 0, sizeof(sys_recipe_t));
    GetSysRecipt()->crc = usModbusRTU_CRC((u8 *)GetSysRecipt() + 2, sizeof(sys_recipe_t) - 2);
}

//分配配方id
u8 AllotRecipeId(char *name, sys_recipe_t *sys_rec)
{
    u8          index       = 0;
    u8          ret         = 0xFF;

    for(index = 1; index < REC_ALLOT_ADDR; index++)
    {
        if(sys_rec->allot_add[index] != index)
        {
            sys_rec->allot_add[index] = index;

            ret = index;
            break;
        }
    }

    return ret;
}

rt_err_t GetRecipeByid(u8 id, sys_recipe_t *sys_rec, recipe_t *rec)
{
    u8          index       = 0;
    rt_err_t    ret         = RT_ERROR;

    for(index = 0; index < RECIPE_LIST_MAX; index++)
    {
        if(id == sys_rec->recipe[index].id)
        {
            rt_memcpy((u8 *)rec, (u8 *)&sys_rec->recipe[index], sizeof(recipe_t));
            ret = RT_EOK;
        }
    }

    return ret;
}

//增加配方
void AddRecipe(recipe_t *rec, sys_recipe_t *sys_rec)
{
    u8      index       = 0;

    LOG_I("name  = %s",rec->name);
    for(index = 0; index < sys_rec->recipe_size; index++)
    {
        if(sys_rec->recipe[index].id == rec->id)
        {
            rt_memcpy(&sys_rec->recipe[index], rec, sizeof(recipe_t));
            LOG_E("name = %s",sys_rec->recipe[index].name);//Justin debug 仅仅测试
            break;
        }
    }

    if(sys_rec->recipe_size == index)
    {
        if(sys_rec->recipe_size >= RECIPE_LIST_MAX)
        {
            LOG_E("recipe size = %d, can not add new",sys_rec->recipe_size);
        }
        else
        {
            rt_memcpy(&sys_rec->recipe[index], rec, sizeof(recipe_t));
            LOG_E("name = %s",sys_rec->recipe[index].name);//Justin debug 仅仅测试
            sys_rec->recipe_size++;
        }
    }
}

rt_err_t deleteRecipe(u8 id, sys_recipe_t *list, sys_set_t *sys_set)
{
    rt_err_t ret = RT_ERROR;

    //1.删除配方列表当前的配方项
    for(u8 index = 0; index < list->recipe_size; index++)
    {
        if(id == list->recipe[index].id)
        {
            //删除分配的地址
            for(u8 addr = 0; addr < REC_ALLOT_ADDR; addr++)
            {
                if(list->allot_add[addr] == list->recipe[index].id)
                {
                    list->allot_add[addr] = 0;
                }
            }

            //如果是在最后面的删除 则不同往前推
            if(index == list->recipe_size - 1)
            {
                rt_memset((u8 *)&list->recipe[index], 0, sizeof(recipe_t));
            }
            else
            {
                if(list->recipe_size > 0)
                {
                    for(u8 item = index; item < list->recipe_size - 1; item++)
                    {
                        rt_memcpy((u8 *)&list->recipe[index], (u8 *)&list->recipe[index + 1], sizeof(recipe_t));
                        rt_memset((u8 *)&list->recipe[index + 1], 0, sizeof(recipe_t));
                    }
                }
            }

            if(list->recipe_size > 0)
            {
                list->recipe_size--;
            }
        }
    }

    //2.同步删除日历关联的配方
    for(u8 cal = 0; cal < STAGE_LIST_MAX; cal++)
    {
        if(sys_set->stageSet._list[cal].recipeId == id)
        {
            sys_set->stageSet._list[cal].recipeId = 0;
        }
    }

    return ret;
}

sys_recipe_t *GetSysRecipt(void)
{
    return &sys_recipe;
}
