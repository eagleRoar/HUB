/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-31     Administrator       the first version
 */
#ifndef APPLICATIONS_OLED1309_OLEDBUSINESS_OLEDBUSINESS_H_
#define APPLICATIONS_OLED1309_OLEDBUSINESS_OLEDBUSINESS_H_

#include "Gpio.h"

void HomePage(u8g2_t *, type_page_t);
void SensorStatePage(u8g2_t *, type_page_t);
void DeviceStatePage(u8g2_t *, type_page_t);

#endif /* APPLICATIONS_OLED1309_OLEDBUSINESS_OLEDBUSINESS_H_ */
