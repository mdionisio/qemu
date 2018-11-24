#include <assert.h>

#include "qemu/osdep.h"
#include "sysemu/sysemu.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "hw/qdev-properties.h"
#include "qapi/visitor.h"

#include "hw/misc/mytest.h"

#define DEBUG_MYTEST 1

#ifdef DEBUG_MYTEST

#define DPRINTF(fmt, args...) \
    do { \
        fprintf(stderr, "[%s:%d]%s: " fmt "\n", __FILE__, __LINE__, \
                                            __func__, ##args);  \
    } while (0)

#else

#define DPRINTF(fmt, args...)

#endif


/*< public method >*/

void mytest_method(MyTest *obj)
{
    DPRINTF("");
    MyTestClass *klass = MYTEST_GET_CLASS(obj);
    if (klass->method != NULL) {
        klass->method(obj);
    }
}

void mytest_reset(MyTest *obj) {
    DPRINTF("");
    DeviceClass *klass = DEVICE_GET_CLASS(obj);
    if (klass->reset != NULL) {
        klass->reset(&obj->parent_obj.parent_obj);
    }
}

/*< private method >*/

static uint64_t mytest_iomem_read(void *opaque, hwaddr offset, unsigned size)
{
    MyTest *s = (MyTest *)opaque;

    if (offset % 4 != 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "[%s]%s: only aligned accessed allowed offset 0x%"
                      HWADDR_PRIx, TYPE_MYTEST, __func__, offset);
        return 0;
    }

    switch (offset) {
        case 0:
            DPRINTF("read(offset=0x%" HWADDR_PRIx ", size=%u)", offset, size);

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
                DPRINTF("reset IRQ");
                timer_del(&(s->timer));
                qemu_set_irq(s->irq, 0);
            } else if (s->data1 == 1) {
                s->data1 = 0xFFFFFFFF;
                DPRINTF("raise postponed");
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


static void mytest_get(Object *obj, Visitor *v, const char *name,
                                   void *opaque, Error **errp)
{
    MyTest *s = MYTEST(obj);
    int64_t value = s->data1;
    DPRINTF("");

    visit_type_int(v, name, &value, errp);
}

static void mytest_set(Object *obj, Visitor *v, const char *name,
                                   void *opaque, Error **errp)
{
    MyTest *s = MYTEST(obj);
    Error *local_err = NULL;
    int64_t temp;
    DPRINTF("");

    visit_type_int(v, name, &temp, &local_err);

    s->data1 = temp;
}


static void mytest_init(Object *obj)
{
    DPRINTF("");
    // DeviceState *d = DEVICE(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    MyTest *s = MYTEST(obj);
    //s->data1 = 0;                     // not initialized here becase there is the property initialized defined

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

    // example of property
    object_property_add(obj, "data", "int",
                        mytest_get,
                        mytest_set, NULL, NULL, NULL);
}

static void mytest_finalize(Object *obj) 
{
    DPRINTF("");
    MyTest *s = MYTEST(obj);
    timer_deinit(&(s->timer));
}

static void mytest_timer_expired(void *dev)
{
    MyTest *s = MYTEST(dev);

    DPRINTF("");

    DPRINTF("raise IRQ");
    qemu_set_irq(s->irq, 1);

    timer_del(&(s->timer));
}

static DeviceRealize old_callback_realize = NULL;
static void mytest_realize(DeviceState *dev, Error **errp)
{
    DPRINTF("");

    if (old_callback_realize != NULL) {
        DPRINTF("call base realize");
        old_callback_realize(dev, errp);
    }
}

// example of property that can be initialized
static Property properties[] = {
    DEFINE_PROP_UINT32("data1", MyTest, data1, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void mytest_static_reset(DeviceState *dev) {
    DPRINTF("");
}

static void mytest_static_init(ObjectClass *klass, void *data)
{
    DeviceClass       *dc = DEVICE_CLASS(klass);
    //SysBusDeviceClass *sdc = SYS_BUS_DEVICE_CLASS(klass);
    MyTestClass       *mdc = MYTEST_CLASS(klass);

    DPRINTF("");

    mdc->method = NULL;  // initialize our virtual method

    // overwrite virtual method
    old_callback_realize = dc->realize; dc->realize = mytest_realize;
    dc->reset = mytest_static_reset;

    dc->desc = "MyTest Device";
    dc->props = properties;

    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
}

static const TypeInfo mytest_info = {
    .name          = TYPE_MYTEST,           // The name of the type
    .parent        = TYPE_SYS_BUS_DEVICE,   // The name of the parent type
    .instance_size = sizeof(MyTest),        // The size of the object
    .instance_init = mytest_init,           // This function is called to initialize an object
    .instance_post_init = NULL,             // This function is called after all @instance_init functions were called
    .abstract      = FALSE,                 // to specify it it can be directly initialized or not
    .class_size    = sizeof(MyTestClass),   // The size of the class object (is 0 means size of the parent)
    .class_init    = mytest_static_init,    // Class Initialized. Called after parent class initialized
    .class_base_init = NULL,                // Call after before .class_init after all base class initialization but on all base class
    .instance_finalize = mytest_finalize,   // This function is called during object destruction. And before parent object destruction
    .class_finalize = NULL,                 // This function is called during class destruction
    .class_data     = NULL,                 // Data to pass to the @class_init, @class_base_init and @class_finalize functions.
    .interfaces     = NULL,                 // NULL terminater list of interface (Multihineritance of interface)
};

static void mytest_register_type(void)
{
    DPRINTF("");
    type_register_static(&mytest_info);
}
type_init(mytest_register_type)