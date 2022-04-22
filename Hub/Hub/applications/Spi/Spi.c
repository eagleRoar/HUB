/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-03-05     Administrator       the first version
 */

#include "Spi.h"
#include "spi_flash_sfud.h"
#include "fal_def.h"
#include "fal.h"

#define W25Q_SPI_DEVICE_NAME     "spi10"

/**
 * 总线上挂载设备 spi10
 * @return
 */
static int bsp_spi_attach_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    rt_err_t ret = rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_4);// spi10 表示挂载在 spi3 总线上的 0 号设备,PC0是片选，这一步就可以将从设备挂在到总线中。
    if(ret <0)
    {
        LOG_E("flash attach spi1 failed");
        return -RT_ERROR;
    }

    rt_sfud_flash_probe(FAL_USING_NOR_FLASH_DEV_NAME, "spi10");//Justin debug 注册nor flash

    return RT_EOK;
}
INIT_DEVICE_EXPORT(bsp_spi_attach_init);

void rtcTest(void)
{

    rt_err_t ret = RT_EOK;
    //time_t now;

    /* 设置日期 */
    ret = set_date(2022, 3, 5);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC date failed\n");
    }

    /* 设置时间 */
    ret = set_time(15, 15, 50);
    if (ret != RT_EOK)
    {
        LOG_D("set RTC time failed\n");
    }

    /* 延时3秒 */
    rt_thread_mdelay(3000);

//    /* 获取时间 */
//    now = time(RT_NULL);
//    LOG_D("%s\n", ctime(&now));
}

struct rt_spi_device *spi_dev_w25q;
void spiTest(void)
{
//    struct rt_spi_device *spi_dev_w25q;
    rt_uint8_t w25x_read_id = 0x90;
    rt_uint8_t id[5] = {0};

    /* 查找 spi 设备获取设备句柄 */
    spi_dev_w25q = (struct rt_spi_device *)rt_device_find(W25Q_SPI_DEVICE_NAME);
    if (!spi_dev_w25q)
    {
        LOG_E("spi sample run failed! can't find %s device!", W25Q_SPI_DEVICE_NAME);
    }
    else
    {

        /* config spi */
        {
            /*配置SPI通讯相关参数  */
            struct rt_spi_configuration cfg;
            cfg.data_width = 8;                                     /* 数据宽度 */
            cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;  /* 主从模式、时钟极性和时钟相位、数据传输顺序是MSB位在前还是LSB位在前 */
            cfg.max_hz = 20 * 1000 *1000;                           /* 20M */
            rt_spi_configure(spi_dev_w25q, &cfg);
        }

        /* 方式1：使用 rt_spi_send_then_recv()发送命令读取ID */
        rt_spi_send_then_recv(spi_dev_w25q, &w25x_read_id, 1, id, 5);
        LOG_D("use rt_spi_send_then_recv() read w25q ID is:%x%x", id[3], id[4]);

        /* 方式2：使用 rt_spi_transfer_message()发送命令读取ID */
//        struct rt_spi_message msg1, msg2;
//
//        msg1.send_buf   = &w25x_read_id;
//        msg1.recv_buf   = RT_NULL;
//        msg1.length     = 1;
//        msg1.cs_take    = 1;
//        msg1.cs_release = 0;
//        msg1.next       = &msg2;
//
//        msg2.send_buf   = RT_NULL;
//        msg2.recv_buf   = id;
//        msg2.length     = 5;
//        msg2.cs_take    = 0;
//        msg2.cs_release = 1;
//        msg2.next       = RT_NULL;
//
//        rt_spi_transfer_message(spi_dev_w25q, &msg1);
//        LOG_D("use rt_spi_transfer_message() read w25q ID is:%x%x%x%x%x", id[0], id[1], id[2], id[3], id[4]);

    }
}

extern int fal_init_check(void);

static void fal(uint8_t argc, char **argv) {

#define __is_print(ch)                ((unsigned int)((ch) - ' ') < 127u - ' ')
#define HEXDUMP_WIDTH                 16
#define CMD_PROBE_INDEX               0
#define CMD_READ_INDEX                1
#define CMD_WRITE_INDEX               2
#define CMD_ERASE_INDEX               3
#define CMD_BENCH_INDEX               4

    int result;
    static const struct fal_flash_dev *flash_dev = NULL;
    static const struct fal_partition *part_dev = NULL;
    size_t i = 0, j = 0;

    const char* help_info[] =
    {
            [CMD_PROBE_INDEX]     = "fal probe [dev_name|part_name]   - probe flash device or partition by given name",
            [CMD_READ_INDEX]      = "fal read addr size               - read 'size' bytes starting at 'addr'",
            [CMD_WRITE_INDEX]     = "fal write addr data1 ... dataN   - write some bytes 'data' starting at 'addr'",
            [CMD_ERASE_INDEX]     = "fal erase addr size              - erase 'size' bytes starting at 'addr'",
            [CMD_BENCH_INDEX]     = "fal bench <blk_size>             - benchmark test with per block size",
    };

    if (fal_init_check() != 1)
    {
        rt_kprintf("\n[Warning] FAL is not initialized or failed to initialize!\n\n");
        return;
    }

    if (argc < 2)
    {
        rt_kprintf("Usage:\n");
        for (i = 0; i < sizeof(help_info) / sizeof(char*); i++)
        {
            rt_kprintf("%s\n", help_info[i]);
        }
        rt_kprintf("\n");
    }
    else
    {
        const char *operator = argv[1];
        uint32_t addr, size;

        if (!strcmp(operator, "probe"))
        {
            if (argc >= 3)
            {
                char *dev_name = argv[2];
                if ((flash_dev = fal_flash_device_find(dev_name)) != NULL)
                {
                    part_dev = NULL;
                }
                else if ((part_dev = fal_partition_find(dev_name)) != NULL)
                {
                    flash_dev = NULL;
                }
                else
                {
                    rt_kprintf("Device %s NOT found. Probe failed.\n", dev_name);
                    flash_dev = NULL;
                    part_dev = NULL;
                }
            }

            if (flash_dev)
            {
                rt_kprintf("Probed a flash device | %s | addr: %ld | len: %d |.\n", flash_dev->name,
                        flash_dev->addr, flash_dev->len);
            }
            else if (part_dev)
            {
                rt_kprintf("Probed a flash partition | %s | flash_dev: %s | offset: %ld | len: %d |.\n",
                        part_dev->name, part_dev->flash_name, part_dev->offset, part_dev->len);
            }
            else
            {
                rt_kprintf("No flash device or partition was probed.\n");
                rt_kprintf("Usage: %s.\n", help_info[CMD_PROBE_INDEX]);
                fal_show_part_table();
            }
        }
        else
        {
            if (!flash_dev && !part_dev)
            {
                rt_kprintf("No flash device or partition was probed. Please run 'fal probe'.\n");
                return;
            }
            if (!rt_strcmp(operator, "read"))
            {
                if (argc < 4)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_READ_INDEX]);
                    return;
                }
                else
                {
                    addr = strtol(argv[2], NULL, 0);
                    size = strtol(argv[3], NULL, 0);
                    uint8_t *data = rt_malloc(size);
                    if (data)
                    {
                        if (flash_dev)
                        {
                            result = flash_dev->ops.read(addr, data, size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_read(part_dev, addr, data, size);
                        }
                        if (result >= 0)
                        {
                            rt_kprintf("Read data success. Start from 0x%08X, size is %ld. The data is:\n", addr,
                                    size);
                            rt_kprintf("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
                            for (i = 0; i < size; i += HEXDUMP_WIDTH)
                            {
                                rt_kprintf("[%08X] ", addr + i);
                                /* dump hex */
                                for (j = 0; j < HEXDUMP_WIDTH; j++)
                                {
                                    if (i + j < size)
                                    {
                                        rt_kprintf("%02X ", data[i + j]);
                                    }
                                    else
                                    {
                                        rt_kprintf("   ");
                                    }
                                }
                                /* dump char for hex */
                                for (j = 0; j < HEXDUMP_WIDTH; j++)
                                {
                                    if (i + j < size)
                                    {
                                        rt_kprintf("%c", __is_print(data[i + j]) ? data[i + j] : '.');
                                    }
                                }
                                rt_kprintf("\n");
                            }
                            rt_kprintf("\n");
                        }
                        rt_free(data);
                    }
                    else
                    {
                        rt_kprintf("Low memory!\n");
                    }
                }
            }
            else if (!strcmp(operator, "write"))
            {
                if (argc < 4)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_WRITE_INDEX]);
                    return;
                }
                else
                {
                    addr = strtol(argv[2], NULL, 0);
                    size = argc - 3;
                    uint8_t *data = rt_malloc(size);
                    if (data)
                    {
                        for (i = 0; i < size; i++)
                        {
                            data[i] = strtol(argv[3 + i], NULL, 0);
                        }
                        if (flash_dev)
                        {
                            result = flash_dev->ops.write(addr, data, size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_write(part_dev, addr, data, size);
                        }
                        if (result >= 0)
                        {
                            rt_kprintf("Write data success. Start from 0x%08X, size is %ld.\n", addr, size);
                            rt_kprintf("Write data: ");
                            for (i = 0; i < size; i++)
                            {
                                rt_kprintf("%d ", data[i]);
                            }
                            rt_kprintf(".\n");
                        }
                        rt_free(data);
                    }
                    else
                    {
                        rt_kprintf("Low memory!\n");
                    }
                }
            }
            else if (!rt_strcmp(operator, "erase"))
            {
                if (argc < 4)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_ERASE_INDEX]);
                    return;
                }
                else
                {
                    addr = strtol(argv[2], NULL, 0);
                    size = strtol(argv[3], NULL, 0);
                    if (flash_dev)
                    {
                        result = flash_dev->ops.erase(addr, size);
                    }
                    else if (part_dev)
                    {
                        result = fal_partition_erase(part_dev, addr, size);
                    }
                    if (result >= 0)
                    {
                        rt_kprintf("Erase data success. Start from 0x%08X, size is %ld.\n", addr, size);
                    }
                }
            }
            else if (!strcmp(operator, "bench"))
            {
                if (argc < 3)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_BENCH_INDEX]);
                    return;
                }
                else if ((argc > 3 && strcmp(argv[3], "yes")) || argc < 4)
                {
                    rt_kprintf("DANGER: It will erase full chip or partition! Please run 'fal bench %d yes'.\n", strtol(argv[2], NULL, 0));
                    return;
                }
                /* full chip benchmark test */
                uint32_t start_time, time_cast;
                size_t write_size = strtol(argv[2], NULL, 0), read_size = strtol(argv[2], NULL, 0), cur_op_size;
                uint8_t *write_data = (uint8_t *)rt_malloc(write_size), *read_data = (uint8_t *)rt_malloc(read_size);

                if (write_data && read_data)
                {
                    for (i = 0; i < write_size; i ++) {
                        write_data[i] = i & 0xFF;
                    }
                    if (flash_dev)
                    {
                        size = flash_dev->len;
                    }
                    else if (part_dev)
                    {
                        size = part_dev->len;
                    }
                    /* benchmark testing */
                    rt_kprintf("Erasing %ld bytes data, waiting...\n", size);
                    start_time = rt_tick_get();
                    if (flash_dev)
                    {
                        result = flash_dev->ops.erase(0, size);
                    }
                    else if (part_dev)
                    {
                        result = fal_partition_erase(part_dev, 0, size);
                    }
                    if (result >= 0)
                    {
                        time_cast = rt_tick_get() - start_time;
                        rt_kprintf("Erase benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
                    }
                    else
                    {
                        rt_kprintf("Erase benchmark has an error. Error code: %d.\n", result);
                    }
                    /* write test */
                    rt_kprintf("Writing %ld bytes data, waiting...\n", size);
                    start_time = rt_tick_get();
                    for (i = 0; i < size; i += write_size)
                    {
                        if (i + write_size <= size)
                        {
                            cur_op_size = write_size;
                        }
                        else
                        {
                            cur_op_size = size - i;
                        }
                        if (flash_dev)
                        {
                            result = flash_dev->ops.write(i, write_data, cur_op_size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_write(part_dev, i, write_data, cur_op_size);
                        }
                        if (result < 0)
                        {
                            break;
                        }
                    }
                    if (result >= 0)
                    {
                        time_cast = rt_tick_get() - start_time;
                        rt_kprintf("Write benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
                    }
                    else
                    {
                        rt_kprintf("Write benchmark has an error. Error code: %d.\n", result);
                    }
                    /* read test */
                    rt_kprintf("Reading %ld bytes data, waiting...\n", size);
                    start_time = rt_tick_get();
                    for (i = 0; i < size; i += read_size)
                    {
                        if (i + read_size <= size)
                        {
                            cur_op_size = read_size;
                        }
                        else
                        {
                            cur_op_size = size - i;
                        }
                        if (flash_dev)
                        {
                            result = flash_dev->ops.read(i, read_data, cur_op_size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_read(part_dev, i, read_data, cur_op_size);
                        }
                        /* data check */
                        for (int index = 0; index < cur_op_size; index ++)
                        {
                            if (write_data[index] != read_data[index])
                            {
                                rt_kprintf("%d %d %02x %02x.\n", i, index, write_data[index], read_data[index]);
                            }
                        }

                        if (memcmp(write_data, read_data, cur_op_size))
                        {
                            result = -RT_ERROR;
                            rt_kprintf("Data check ERROR! Please check you flash by other command.\n");
                        }
                        /* has an error */
                        if (result < 0)
                        {
                            break;
                        }
                    }
                    if (result >= 0)
                    {
                        time_cast = rt_tick_get() - start_time;
                        rt_kprintf("Read benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
                    }
                    else
                    {
                        rt_kprintf("Read benchmark has an error. Error code: %d.\n", result);
                    }
                }
                else
                {
                    rt_kprintf("Low memory!\n");
                }
                rt_free(write_data);
                rt_free(read_data);
            }
            else
            {
                rt_kprintf("Usage:\n");
                for (i = 0; i < sizeof(help_info) / sizeof(char*); i++)
                {
                    rt_kprintf("%s\n", help_info[i]);
                }
                rt_kprintf("\n");
                return;
            }
            if (result < 0) {
                rt_kprintf("This operate has an error. Error code: %d.\n", result);
            }
        }
    }
}

void getDevice(void)
{
    #define HEXDUMP_WIDTH                 16

    static const struct fal_flash_dev *flash_dev = NULL;
    static const struct fal_partition *part_dev = NULL;
    char *dev_name = FAL_USING_NOR_FLASH_DEV_NAME;
    u8 /*data[10],*/revData[10],result;

    if ((flash_dev = fal_flash_device_find(dev_name)) != NULL)
    {
        part_dev = NULL;
//        LOG_D("find flash device");
    }
    else if ((part_dev = fal_partition_find(dev_name)) != NULL)
    {
        flash_dev = NULL;
//        LOG_D("find flash partition");
    }

//    if (flash_dev)
//    {
//        LOG_D("Probed a flash device | %s | addr: %ld | len: %d |.", flash_dev->name,
//                flash_dev->addr, flash_dev->len);
//    }
//    else if (part_dev)
//    {
//        LOG_D("Probed a flash partition | %s | flash_dev: %s | offset: %ld | len: %d |.",
//                part_dev->name, part_dev->flash_name, part_dev->offset, part_dev->len);
//    }

//    for (int i = 0; i < 10; i++)
//    {
//        data[i] = i;
//    }
//    if (flash_dev)
//    {
//        result = flash_dev->ops.write(0, data, 10);
//    }
//    else if (part_dev)
//    {
//        result = fal_partition_write(part_dev, 0, data, 10);
//    }
//    if (result >= 0)
//    {
//        LOG_D("Write data success. Start from 0x%08X, size is %ld.", 0, 10);
//        LOG_D("Write data: ");
//        for (int i = 0; i < 10; i++)
//        {
//            LOG_D("%d ", data[i]);
//        }
//
//    }

    if (flash_dev)
    {
       result = flash_dev->ops.read(0, revData, 10);
    }
    else if (part_dev)
    {
       result = fal_partition_read(part_dev, 0, revData, 10);
    }
    if (result >= 0)
    {
//        LOG_D("Read data success. Start from 0x%08X, size is %ld. The data is:", 0,
//               10);
//
//        for(int i = 0; i< 10; i++)
//        {
//            LOG_D("%d ",revData[i]);
//        }

       //LOG_D("Offset (h) 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");
//       for (int i = 0; i < 10; i += 16)
//       {
//           rt_kprintf("[%08X] ", 0 + i);
//           /* dump hex */
//           for (int j = 0; j < 16; j++)
//           {
//               if (i + j < 10)
//               {
//                   rt_kprintf("%02X ", revData[i + j]);
//               }
//               else
//               {
//                   rt_kprintf("   ");
//               }
//           }
//           /* dump char for hex */
//           for (int j = 0; j < 16; j++)
//           {
//               if (i + j < 10)
//               {
//                   rt_kprintf("%c", __is_print(revData[i + j]) ? revData[i + j] : '.');
//               }
//           }
//           rt_kprintf("\n");
//       }
//       rt_kprintf("\n");
    }
    else
    {
        LOG_E("read error");
    }
}

/**
 * @brief  : spi flash线程入口
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.03.05
 */
void SpiTaskEntry(void* parameter)
{
    static u8 timeCnt = 0;
    //time_t now;
    //rt_uint8_t w25x_read_id = 0x90;
    //rt_uint8_t id[5] = {0};
//    rtcTest();
    getDevice();
    while(1)
    {
//        spiTest();
//        rt_spi_send_then_recv(spi_dev_w25q, &w25x_read_id, 1, id, 5);
//
//        LOG_D("use rt_spi_send_then_recv() read w25q ID is:%x%x%x%x%x", id[0], id[1], id[2], id[3], id[4]);

//        if(timeCnt < 10)
//        {
//            timeCnt++;
//        }
//        else
//        {
//            timeCnt = 0;
//
//            /* 获取时间 */
//            now = time(RT_NULL);
//            LOG_D("%s\n", ctime(&now));
//        }

        timeCnt++;

//        if(0 == timeCnt%2)
//        {
//            rt_pin_write(SPI1_CS_PIN, PIN_LOW);
//            rt_pin_write(SPI1_MOSI_PIN, PIN_LOW);
//            rt_pin_write(SPI1_MISO_PIN, PIN_LOW);
//            rt_pin_write(SPI1_SCK_PIN, PIN_LOW);
//        }
//        else
//        {
//            rt_pin_write(SPI1_CS_PIN, PIN_HIGH);
//            rt_pin_write(SPI1_MOSI_PIN, PIN_HIGH);
//            rt_pin_write(SPI1_MISO_PIN, PIN_HIGH);
//            rt_pin_write(SPI1_SCK_PIN, PIN_HIGH);
//        }

        rt_thread_mdelay(1000);
    }
}

/**
 * @brief  : spi flash线程
 * @para   : NULL
 * @author : Qiuyijie
 * @date   : 2022.03.05
 */
void SpiTaskInit(void)
{
    rt_err_t threadStart = RT_NULL;

    /* 创建led 线程 */
    rt_thread_t thread = rt_thread_create("spi task", SpiTaskEntry, RT_NULL, 1024, SPI_PRIORITY, 10);

    /* 如果线程创建成功则开始启动线程，否则提示线程创建失败 */
    if (RT_NULL != thread) {
        threadStart = rt_thread_startup(thread);
        if (RT_EOK != threadStart) {
            LOG_E("spi task start failed");
        }
    } else {
        LOG_E("spi task create failed");
    }
}
