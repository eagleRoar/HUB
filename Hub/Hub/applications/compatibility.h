/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-13     Administrator       the first version
 */
#ifndef APPLICATIONS_COMPATIBILITY_H_
#define APPLICATIONS_COMPATIBILITY_H_

#include "Gpio.h"
#include "cJSON.h"


typedef struct JTest1{
    char    name[5];
    int     age;
}jtest1;

typedef struct JTest2{
    char    name[5];
    char    name1[2];
    int     age;
}jtest2;

typedef struct JTest3{
    char    name[5];
    char    name1[2];
    int     age;
    u8      num;
}jtest3;

#endif /* APPLICATIONS_COMPATIBILITY_H_ */
