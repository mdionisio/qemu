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
 * 
 * MyTest ----> SysBus ----> Device ----> Object   (class diagram)
 * 
 * (http://people.redhat.com/~thuth/blog/qemu/2018/09/10/instance-init-realize.html)
 * There are three different initialization functions involved here, 
 * the class_init, the instance_init and the realize function.
 * 
 * 
 * 
 * 
 * Object  (see include/qom/object.h)
 *     - System for dynamically registering types
 *     - Support for single-inheritance of types
 *     - Multiple inheritance of stateless interfaces
 * 
 * Device   (see include/hw/qdev-core.h)
 *     -  2 step initialization (costructor + realize)
 * 
 * SysBusDevice (see include/hw/sysbus.h)
 *     -  device able to manage memory region and irq attached to the main system bus
 * 
 * 
 * Qemu Object Model: (https://wiki.qemu.org/Features/QOM)
 * 
 * - Everything in QOM is a device.
 * - Devices can have two types of relationships with other devices: device composition or device backlinks. 
 *   Device composition involves one device directly creating another device. 
 *   It will own life cycle management for the created device and will generally propagate events to that device (although there are exceptions).
 * 
 * 
 * - The two main "concepts" in QDev are devices and busses. A
 *   device is represented by a DeviceState and a bus is represented by a BusState. 
 *   Devices can have properties but busses cannot. 
 *   A device has no direct relationship with other devices. The only relationship is indirect through busses.
 * 
 * - A device may have zero or more busses associated with it via a has-a relationship. 
 *   Each child bus may have multiple devices associated with it via a reference. 
 *   All devices have a single parent bus and all busses have a single parent device. 
 *   These relationships form a strict tree where every alternating level is a bus level followed by a device level. 
 *   The root of the tree is the main system bus often referred to as SysBus.
 * 
 * 
 * ex:
 *    explore device property with:
 * ./arm-softmmu/qemu-system-arm -M sabrelite -cpu cortex-a9 -nographic -device mytest,help
 * ./arm-softmmu/qemu-system-arm -M sabrelite -cpu cortex-a9 -nographic -global mytest.data1=2
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
void mytest_reset(MyTest *obj);    // convinient public way to easy call public reset on object