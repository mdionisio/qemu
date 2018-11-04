#include <stdint.h>
#include "qemu/typedefs.h"
#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_MYTEST "mytest"
#define MYTEST(obj) \
    OBJECT_CHECK(MyTest, (obj), TYPE_MYTEST)

#define MYTEST_CLASS(klass) \
     OBJECT_CLASS_CHECK(MyTestClass, (klass), TYPE_MYTEST)

#define MYTEST_GET_CLASS(obj) \
    OBJECT_GET_CLASS(MyTestClass, (obj), TYPE_MYTEST)

typedef struct MyTest {
    /*< private >*/
    SysBusDevice parent_obj;
    QEMUTimer *timer;

    /*< public >*/
    // settings
    MemoryRegion iomem;
    qemu_irq irq;

    // data
    uint32_t data1;

} MyTest;

typedef struct MyTestClass
{
    /*< private >*/
    SysBusDeviceClass parent;

    /*< public >*/
    void (*method) (MyTest *obj);
} MyTestClass;

void mytest_method(MyTest *obj);