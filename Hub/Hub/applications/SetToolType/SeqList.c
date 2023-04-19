/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-01-05     Administrator       the first version
 */
/**
 * 链表数据结构
 */

#include "SeqList.h"

static void FreeNodeMemory(Node* node)
{
    //1.删除节点里面的动态数据段
    if(node->keyData.dataSegment.data)
    {
        rt_free(node->keyData.dataSegment.data);
        node->keyData.dataSegment.data = RT_NULL;
    }

    //2.删除节点
    if(node)
    {
        rt_free(node);
        node = RT_NULL;
    }
}

//检查是否存在该key
u8 KeyExist(Node *head, KV keyData)
{
    Node *tail = RT_NULL;

    tail = head;
    while(tail)
    {
        if(tail->keyData.key == keyData.key)
        {
            return YES;
        }

        tail = tail->next;
    }

    return NO;
}

u8 DataCorrect(Node *head, KV keyData)
{
    Node *tail = RT_NULL;

    tail = head;
    while(tail)
    {
        if(tail->keyData.key == keyData.key)
        {
            //数据是否一样
            if(0 == rt_memcmp(keyData.dataSegment.data, tail->keyData.dataSegment.data, keyData.dataSegment.len))
            {
                return YES;
            }
        }

        tail = tail->next;
    }

    return NO;
}

//尾插法
/**
 *
 * @param head          : 头节点
 * @param keyData       : 键值对
 * @param keySensitive  : 存储数据列表的时候是否对key 敏感
 */
void CreatTail(Node *head, KV keyData, u8 keyUnique)
{

    Node *tail      = RT_NULL;      //移动指针
    Node *newNode   = RT_NULL;      //新节点

    //1.申请节点空间
    newNode = rt_malloc(sizeof(Node));

    if(RT_NULL != newNode)
    {
        newNode->keyData.dataSegment.len = keyData.dataSegment.len;
        //2.申请节点中数据空间
        newNode->keyData.dataSegment.data = rt_malloc(newNode->keyData.dataSegment.len);
        newNode->next = RT_NULL;

        if(RT_NULL != newNode->keyData.dataSegment.data)
        {
            //3.复制数据
            newNode->keyData.key = keyData.key;
            rt_memcpy(newNode->keyData.dataSegment.data, keyData.dataSegment.data, newNode->keyData.dataSegment.len);

            tail = head;
            while(tail->next)
            {
                //4.检查该key 是否存在，存在的话就结束
                if((tail->next->keyData.key == keyData.key) &&
                   ((YES == keyUnique) || ((NO == keyUnique) &&
                           (0 == rt_memcmp(newNode->keyData.dataSegment.data, tail->next->keyData.dataSegment.data, tail->next->keyData.dataSegment.len)))))
                {
                    //覆盖内容
                    rt_memcpy(tail->next->keyData.dataSegment.data, newNode->keyData.dataSegment.data, tail->next->keyData.dataSegment.len);

                    //5.释放内存
                    FreeNodeMemory(newNode);
                    newNode->keyData.dataSegment.data = RT_NULL;
                    newNode = RT_NULL;

                    return;
                }

                tail = tail->next;
            }
            tail->next = newNode;
        }
    }
    else
    {
        LOG_E("cerate new node fail");
    }
}

void DeleteNode(Node *head, KV keyData)
{
    Node *pre   = RT_NULL;      //前一节点
    Node *cur   = head;      //当前节点

    //2.如果不是头节点
    while(cur->next)
    {
        pre = cur;
        cur = cur->next;//当前节点一直往下查询

        if(keyData.key == cur->keyData.key)
        {
            //前一节点的next指向前一节点的next节点的next
            pre->next = cur->next;
            //删除无用节点
            //LOG_W("delete node = %x",cur->keyData);

            FreeNodeMemory(cur);

            //PrintNode(head);
            return;
        }
    }
}

void PrintNode(Node *head)
{
    Node *p;

    LOG_I("-----------------------------PrintNode");

    p = head;
    while (p) {
        rt_kprintf("key = %x,",p->keyData.key);
        for(u8 i = 0; i < p->keyData.dataSegment.len; i++)
        {
            rt_kprintf(" %x",p->keyData.dataSegment.data[i]);
        }
        rt_kprintf("\r\n");
        p = p->next;
    }

    LOG_W("---------------------------------------PrintNode end");
}

