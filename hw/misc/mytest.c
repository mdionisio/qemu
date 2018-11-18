#include <assert.h>

#include "qemu/osdep.h"
#include "sysemu/sysemu.h"
#include "qemu/log.h"
#include "qemu/timer.h"

#include "hw/misc/mytest.h"

#define DEBUG_MYTEST 1

#ifdef DEBUG_MYTEST

#define DPRINTF(fmt, args...) \
    do { \
        fprintf(stderr, "[%s:%d]%s: " fmt , __FILE__, __LINE__, \
                                            __func__, ##args);  \
    } while (0)

#else

#define DPRINTF(fmt, args...)

#endif


/*< public method >*/

void mytest_method(MyTest *obj)
{
    MyTestClass *klass = MYTEST_GET_CLASS(obj);
    if (klass->method != NULL) {
        klass->method(obj);
    }
}

/*< private method >*/

static uint64_t mytest_iomem_read(void *opaque, hwaddr offset, unsigned size)
{
    MyTest *s = (MyTest *)opaque;

    if (offset % 4 != 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "[%s]%s: only aligned accessed allowed offset 0x%"
                      HWADDR_PRIx "\n", TYPE_MYTEST, __func__, offset);
        return 0;
    }

    switch (offset) {
        case 0:
            DPRINTF("read(offset=0x%" HWADDR_PRIx ", size=%u)\n", offset, size);

            return s->data1;
        default:
            qemu_log_mask(LOG_GUEST_ERROR, "[%s]%s: too much offset 0x%"
                      HWADDR_PRIx "\n", TYPE_MYTEST, __func__, offset);
            return 0;
    }

    assert(0);
}

static void mytest_iomem_write(void *opaque, hwaddr offset,
                             uint64_t value, unsigned size)
{
    MyTest *s = (MyTest *)opaque;

    if (offset % 4 != 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "[%s]%s: only aligned accessed allowed offset 0x%"
                      HWADDR_PRIx "\n", TYPE_MYTEST, __func__, offset);
        return ;
    }

    switch (offset) {
        case 0:
            
            s->data1 = (uint32_t)value;
            
            if (s->data1 == 0) {
                DPRINTF("reset IRQ\n");
                timer_del(&(s->timer));
                qemu_set_irq(s->irq, 0);
            } else if (s->data1 == 1) {
                s->data1 = 0xFFFFFFFF;
                DPRINTF("raise postponed\n");
                timer_mod(&(s->timer), qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + NANOSECONDS_PER_SECOND/2);
            } else {
                s->data1 ++;
            }
            
            DPRINTF("write(offset=0x%" HWADDR_PRIx ", value = 0x%08" PRIx32 ")\n", offset, s->data1);
            return;
        default:
            qemu_log_mask(LOG_GUEST_ERROR, "[%s]%s: too much offset 0x%"
                      HWADDR_PRIx "\n", TYPE_MYTEST, __func__, offset);
            return;
    }

    assert(0);
}

static const struct MemoryRegionOps mytest_iomem_ops = {
    .read = mytest_iomem_read,
    .write = mytest_iomem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void mytest_timer_expired(void *dev);

static void mytest_init(Object *obj)
{
    DPRINTF("mytest_init\n");
    // DeviceState *d = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    MyTest *s = MYTEST(obj);
    s->data1 = 0;

    memory_region_init_io(&s->iomem,                   // memory region to initialize
                          obj,                         // owner object
                          &mytest_iomem_ops, 
                          s,                           // object passed to the read/write callback
                          TYPE_MYTEST,                 // debug string
                          sizeof(uint32_t)             // memory region size
                          );
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    timer_init_ns(&(s->timer), QEMU_CLOCK_VIRTUAL, mytest_timer_expired, obj);
}

static void mytest_finalize(Object *obj) 
{
    DPRINTF("mytest_finalize\n");
    MyTest *s = MYTEST(obj);
    timer_deinit(&(s->timer));
}

static DeviceReset old_callback_reset = NULL;
static void mytest_reset_at_boot(DeviceState *dev)
{
    DPRINTF("reset on boot\n");
    MyTest *s = MYTEST(dev);
    s->data1 = 0;

    if (old_callback_reset != NULL) {
        old_callback_reset(dev);
    }

    timer_del(&(s->timer));
}

static void mytest_timer_expired(void *dev)
{
    MyTest *s = MYTEST(dev);

    DPRINTF("mytest timer expired\n");

    DPRINTF("raise IRQ\n");
    qemu_set_irq(s->irq, 1);

    timer_del(&(s->timer));
}

static DeviceRealize old_callback_realize = NULL;
static void mytest_realize(DeviceState *dev, Error **errp)
{
    DPRINTF("initialization\n");
    MyTest *s = MYTEST(dev);
    s->data1 = 0;

    if (old_callback_realize != NULL) {
        old_callback_realize(dev, errp);
    }
}

static void mytest_static_init(ObjectClass *klass, void *data)
{
    DeviceClass       *dc = DEVICE_CLASS(klass);
    //SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);
    MyTestClass       *mdc = MYTEST_CLASS(klass);
    mdc->method = NULL;  // initialize our virtual method

    // overwrite virtual method
    old_callback_reset =  dc->reset; dc->reset = mytest_reset_at_boot;
    old_callback_realize = dc->realize; dc->realize = mytest_realize;

    dc->desc = "MyTest Device";

    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo mytest_info = {
    .name          = TYPE_MYTEST,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(MyTest),
    .instance_init = mytest_init,
    .class_size    = sizeof(MyTestClass),
    .class_init    = mytest_static_init,
    .instance_finalize = mytest_finalize,
};

static void mytest_register_type(void)
{
    type_register_static(&mytest_info);
}
type_init(mytest_register_type)