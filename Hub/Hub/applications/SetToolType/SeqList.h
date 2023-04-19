/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-05     Administrator       the first version
 */
#ifndef APPLICATIONS_SETTOOLTYPE_SEQLIST_H_
#define APPLICATIONS_SETTOOLTYPE_SEQLIST_H_

#include "Gpio.h"

#pragma pack(1)
//该数据结构存储数据以及长度
typedef struct dataSegment{
    u8 *data;
    u8 len;
}type_dataSegment;

typedef struct KeyVale{
    long key;                       //键值对 作为存储结构唯一键值
    type_dataSegment dataSegment;
}KV;

typedef struct SeqList{
    KV  keyData;
    struct SeqList *next;
}Node;

u8 CheckKeyExist(Node *, KV);
void CreatTail(Node *, KV, u8);
void DeleteNode(Node *, KV);
void PrintNode(Node *);
u8 KeyExist(Node *, KV);
u8 DataCorrect(Node *, KV);
#endif /* APPLICATIONS_SETTOOLTYPE_SEQLIST_H_ */
