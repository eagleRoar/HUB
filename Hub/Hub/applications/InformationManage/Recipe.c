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
}

//分配配方id
u8 DistributeRecipeId(char *name, sys_recipe_t *sys_rec)
{
    u8          index       = 0;
    u8          rec_index   = 0;
    u8          ret         = 0xFF;

    for(index = 1; index < 255; index++)
    {
        for(rec_index = 0; rec_index < RECIPE_LIST_MAX; rec_index++)
        {
            if(index == sys_rec->recipe[rec_index].id)
            {
                break;
            }
        }

        if(index != 255)
        {
            ret = index;
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
            rec = &sys_rec->recipe[index];
            ret = RT_EOK;
        }
    }

    return ret;
}

//增加配方
void AddRecipe(recipe_t *rec, sys_recipe_t *sys_rec)
{
    u8      index       = 0;

    for(index = 0; index < RECIPE_LIST_MAX; index++)
    {
        if(sys_rec->recipe[index].id == rec->id)
        {
            rt_memcpy(&sys_rec->recipe[index], rec, sizeof(recipe_t));
        }
    }
}

sys_recipe_t *GetSysRecipt(void)
{
    return &sys_recipe;
}