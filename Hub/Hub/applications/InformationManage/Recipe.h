/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-06     Administrator       the first version
 */
#ifndef APPLICATIONS_INFORMATIONMANAGE_RECIPE_H_
#define APPLICATIONS_INFORMATIONMANAGE_RECIPE_H_

#include "Gpio.h"
#include "InformationMonitor.h"

void initSysRecipe(void);
sys_recipe_t *GetSysRecipt(void);
void AddRecipe(recipe_t *, sys_recipe_t *);
rt_err_t GetRecipeByid(u8, sys_recipe_t *, recipe_t *);
u8 AllotRecipeId(char *, sys_recipe_t *);
rt_err_t deleteRecipe(u8 , sys_recipe_t *, sys_set_t *);
#endif /* APPLICATIONS_INFORMATIONMANAGE_RECIPE_H_ */
