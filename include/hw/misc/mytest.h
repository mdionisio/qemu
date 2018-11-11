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

/**
 * MyTest is a device connected to the system bus to map a registry on memory
 * and to raise an interrupt.
 * 
 * a Device Connected to the system bus is already implemented in qemu in the
 * SysBusDevice
 * 
 * In Qemu object is implemented in C trought 2 struct:
 * a. states representation
 * b. methods representation
 * and a TypeInfo that describe the 2
 * 
 * it is nice to provide for each new object type 3 macro:
 * 1. to convert a generic object state in our object state (dynamic canst in c++)
 * 2. to convert a generic object interface in out object interface (dynamic canst in c++)
 * 3. to convert a our object state in out object interface
 * 
 */


typedef struct MyTest {
    /*< private >*/
    SysBusDevice parent_obj;        // hineritance means contains cantains parent interface
    QEMUTimer timer;                // timer used to simulate latency to generate an interrupt

    /*< public >*/
    // settings
    MemoryRegion iomem;             // io memory mapped region with callback for reading and writing
    qemu_irq irq;                   // irq on system bus to use

    // data
    uint32_t data1;                 // internal device data

} MyTest;

typedef struct MyTestClass
{
    /*< private >*/
    SysBusDeviceClass parent;       // hineritance means contains cantains parent interface

    /*< public >*/
    void (*method) (MyTest *obj);   // example of public virtual method
} MyTestClass;

void mytest_method(MyTest *obj);    // convinient public way to easy call public method on object