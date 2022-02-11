/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-11     Administrator       the first version
 */
#include <rtthread.h>

#ifdef PKG_USING_FAL
#include <fal.h>
extern int fal_init(void);
INIT_COMPONENT_EXPORT(fal_init);
#endif
