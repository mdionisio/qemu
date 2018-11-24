/*
 * SABRELITE Board System emulation.
 *
 * Copyright (c) 2015 Jean-Christophe Dubois <jcd@tribudubois.net>
 *
 * This code is licensed under the GPL, version 2 or later.
 * See the file `COPYING' in the top level directory.
 *
 * It (partially) emulates a sabrelite board, with a Freescale
 * i.MX6 SoC
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "hw/arm/fsl-imx6.h"
#include "hw/boards.h"
#include "sysemu/sysemu.h"
#include "qemu/error-report.h"
#include "sysemu/qtest.h"

#include "hw/misc/mytest.h"

#define DEBUG_SABRELITE 1

#ifdef DEBUG_SABRELITE

#define DPRINTF(fmt, args...) \
    do { \
        fprintf(stderr, "[%s:%d]%s: " fmt "\n", __FILE__, __LINE__, \
                                            __func__, ##args);  \
    } while (0)

#else

#define DPRINTF(fmt, args...)

#endif

typedef struct IMX6Sabrelite {
    FslIMX6State soc;
    MemoryRegion ram;
    MyTest mytest_board_hw;
} IMX6Sabrelite;

static struct arm_boot_info sabrelite_binfo = {
    /* DDR memory start */
    .loader_start = FSL_IMX6_MMDC_ADDR,
    /* No board ID, we boot from DT tree */
    .board_id = -1,
};

/* No need to do any particular setup for secondary boot */
static void sabrelite_write_secondary(ARMCPU *cpu,
                                      const struct arm_boot_info *info)
{
}

/* Secondary cores are reset through SRC device */
static void sabrelite_reset_secondary(ARMCPU *cpu,
                                      const struct arm_boot_info *info)
{
}

static void sabrelite_init(MachineState *machine)
{
    IMX6Sabrelite *s = g_new0(IMX6Sabrelite, 1);
    Error *err = NULL;

    /* Check the amount of memory is compatible with the SOC */
    if (machine->ram_size > FSL_IMX6_MMDC_SIZE) {
        error_report("RAM size " RAM_ADDR_FMT " above max supported (%08x)",
                     machine->ram_size, FSL_IMX6_MMDC_SIZE);
        exit(1);
    }

    object_initialize(&s->soc, sizeof(s->soc), TYPE_FSL_IMX6);
    object_property_add_child(OBJECT(machine), "soc", OBJECT(&s->soc),
                              &error_abort);

    object_property_set_bool(OBJECT(&s->soc), true, "realized", &err);
    if (err != NULL) {
        error_report("%s", error_get_pretty(err));
        exit(1);
    }

    memory_region_allocate_system_memory(&s->ram, NULL, "sabrelite.ram",
                                         machine->ram_size);
    memory_region_add_subregion(get_system_memory(), FSL_IMX6_MMDC_ADDR,
                                &s->ram);

    {
        /*
         * TODO: Ideally we would expose the chip select and spi bus on the
         * SoC object using alias properties; then we would not need to
         * directly access the underlying spi device object.
         */
        /* Add the sst25vf016b NOR FLASH memory to first SPI */
        Object *spi_dev;

        spi_dev = object_resolve_path_component(OBJECT(&s->soc), "spi1");
        if (spi_dev) {
            SSIBus *spi_bus;

            spi_bus = (SSIBus *)qdev_get_child_bus(DEVICE(spi_dev), "spi");
            if (spi_bus) {
                DeviceState *flash_dev;
                qemu_irq cs_line;
                DriveInfo *dinfo = drive_get_next(IF_MTD);

                flash_dev = ssi_create_slave_no_init(spi_bus, "sst25vf016b");
                if (dinfo) {
                    qdev_prop_set_drive(flash_dev, "drive",
                                        blk_by_legacy_dinfo(dinfo),
                                        &error_fatal);
                }
                qdev_init_nofail(flash_dev);

                cs_line = qdev_get_gpio_in_named(flash_dev, SSI_GPIO_CS, 0);
                sysbus_connect_irq(SYS_BUS_DEVICE(spi_dev), 1, cs_line);
            }
        }
    }

    /*
     * MYTEST
     */
    object_initialize(&s->mytest_board_hw, sizeof(s->mytest_board_hw), TYPE_MYTEST);
    object_property_add_child(OBJECT(machine), "mytest", OBJECT(&s->mytest_board_hw),
                              &error_abort);
    #define MYTEST_ADDR (FSL_IMX6_MMDC_ADDR+0x40000000)
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->mytest_board_hw), 0, MYTEST_ADDR);

    sysbus_connect_irq(SYS_BUS_DEVICE(&s->mytest_board_hw), 0,
                        qdev_get_gpio_in(DEVICE(&s->soc.a9mpcore), 42));

    object_property_set_bool(OBJECT(&s->mytest_board_hw), true, "realized", &err);
    if (err != NULL) {
        error_report("%s", error_get_pretty(err));
        exit(1);
    }

    sabrelite_binfo.ram_size = machine->ram_size;
    sabrelite_binfo.kernel_filename = machine->kernel_filename;
    sabrelite_binfo.kernel_cmdline = machine->kernel_cmdline;
    sabrelite_binfo.initrd_filename = machine->initrd_filename;
    sabrelite_binfo.nb_cpus = smp_cpus;
    sabrelite_binfo.secure_boot = true;
    sabrelite_binfo.write_secondary_boot = sabrelite_write_secondary;
    sabrelite_binfo.secondary_cpu_reset_hook = sabrelite_reset_secondary;

    if (!qtest_enabled()) {
        arm_load_kernel(&s->soc.cpu[0], &sabrelite_binfo);
    }
}

typedef void (*MachineReset)(void);
static MachineReset old_callback_reset = NULL;

static void sabrelite_machine_reset(void) {
    DPRINTF("");
    bool ambiguous = FALSE;

    Object *tmp = object_resolve_path("/machine/mytest", &ambiguous);
    DPRINTF("object_resolve_path(\"/machine/mytest\", &ambiguous)=%p,%d", tmp, (int)ambiguous);
    if (NULL != tmp) {
        mytest_reset(MYTEST(tmp));
    }

    if (old_callback_reset != NULL) {
        DPRINTF("call parent reset");
        old_callback_reset();
    } else {
        qemu_devices_reset();
    }
}

static void sabrelite_machine_init(MachineClass *mc)
{
    mc->desc = "Freescale i.MX6 Quad SABRE Lite Board (Cortex A9)";
    mc->init = sabrelite_init;
    mc->max_cpus = FSL_IMX6_NUM_CPUS;
    mc->ignore_memory_transaction_failures = true;
    old_callback_reset = mc->reset;
    mc->reset = sabrelite_machine_reset;
}

DEFINE_MACHINE("sabrelite", sabrelite_machine_init)
