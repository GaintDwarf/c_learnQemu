# The Hitchhiker's Guide to the QEMU Machine

## Introduction
First you my want to walk through those tutorials which I found helpful:
 - [The Quick Guide to QEMU Setup](http://opensourceforu.com/2011/05/quick-quide-to-qemu-setup/)
 - [Using QEMU for Embedded Systems Development, Part 1](https://opensourceforu.com/2011/06/qemu-for-embedded-systems-development-part-1/)
 - [Using QEMU for Embedded Systems Development, Part 2](https://opensourceforu.com/2011/07/qemu-for-embedded-systems-development-part-2/)
 - I summarized and updated it in [first bare metal program](first-bare-metal-program.md)

For vscode qemu development it's recommanded to use this `c_cpp_properties.json` file
```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include/**",
                "/usr/lib/x86_64-linux-gnu/glib-2.0/include/**",
                "/usr/include/glib-2.0/**",
                "/usr/include/pixman-1/**"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "clang-x64"
        }
    ],
    "version": 4
}
```

## Game plan
Work in stages, what do I mean? write a bare metal program which can run on a
defined qemu machine (this guide walks through the `virt` qemu machine). 
After we proved that a specific program, which is design to test as a few things
as we can, we can start building a machine which we be able to run it.

## The Magnificent UART device
UART is a simple device for asynchronous serial communication. we can use this
device to print thing out of the qemu (maybe even receiving later?) lets look
at the `hw/arm/virt.c` source code and discover what is his address and in which
addresses the code will be executable.

### The virt Source Code
If we will just search the word UART we will find those lines 
(`hw/arm/virt.c:129-160`):
```c
static const MemMapEntry base_memmap[] = {
    /* Space up to 0x8000000 is reserved for a boot ROM */
    [VIRT_FLASH] =              {          0, 0x08000000 },
    [VIRT_CPUPERIPHS] =         { 0x08000000, 0x00020000 },
    /* GIC distributor and CPU interfaces sit inside the CPU peripheral space */
    [VIRT_GIC_DIST] =           { 0x08000000, 0x00010000 },
    [VIRT_GIC_CPU] =            { 0x08010000, 0x00010000 },
    [VIRT_GIC_V2M] =            { 0x08020000, 0x00001000 },
    [VIRT_GIC_HYP] =            { 0x08030000, 0x00010000 },
    [VIRT_GIC_VCPU] =           { 0x08040000, 0x00010000 },
    /* The space in between here is reserved for GICv3 CPU/vCPU/HYP */
    [VIRT_GIC_ITS] =            { 0x08080000, 0x00020000 },
    /* This redistributor space allows up to 2*64kB*123 CPUs */
    [VIRT_GIC_REDIST] =         { 0x080A0000, 0x00F60000 },
    [VIRT_UART] =               { 0x09000000, 0x00001000 },
    [VIRT_RTC] =                { 0x09010000, 0x00001000 },
    [VIRT_FW_CFG] =             { 0x09020000, 0x00000018 },
    [VIRT_GPIO] =               { 0x09030000, 0x00001000 },
    [VIRT_SECURE_UART] =        { 0x09040000, 0x00001000 },
    [VIRT_SMMU] =               { 0x09050000, 0x00020000 },
    [VIRT_PCDIMM_ACPI] =        { 0x09070000, MEMORY_HOTPLUG_IO_LEN },
    [VIRT_ACPI_GED] =           { 0x09080000, ACPI_GED_EVT_SEL_LEN },
    [VIRT_MMIO] =               { 0x0a000000, 0x00000200 },
    /* ...repeating for a total of NUM_VIRTIO_TRANSPORTS, each of that size */
    [VIRT_PLATFORM_BUS] =       { 0x0c000000, 0x02000000 },
    [VIRT_SECURE_MEM] =         { 0x0e000000, 0x01000000 },
    [VIRT_PCIE_MMIO] =          { 0x10000000, 0x2eff0000 },
    [VIRT_PCIE_PIO] =           { 0x3eff0000, 0x00010000 },
    [VIRT_PCIE_ECAM] =          { 0x3f000000, 0x01000000 },
    /* Actual RAM size depends on initial RAM and device memory settings */
    [VIRT_MEM] =                { GiB, LEGACY_RAMLIMIT_BYTES },
};
```
This array/table can tell us many things for now let's focus on `VIRT_UART` and
`VIRT_MEM`, first and foremost we can infer the thous are the address for the
uart device and the runnable run section.


#### VIRT_UART
If we will follow the links we will find the following:
1. `create_uart(vms, VIRT_UART, sysmem, serial_hd(0));` (line: 1836)
2. `hwaddr base = vms->memmap[uart].base; hwaddr size = vms->memmap[uart].size;`
    (lines: 727-728)
3. `DeviceState *dev = qdev_create(NULL, TYPE_PL011);` (line 732)
4. `SysBusDevice *s = SYS_BUS_DEVICE(dev);` (line: 733)
5. `memory_region_add_subregion(mem, base, sysbus_mmio_get_region(s, 0));`
    (lines: 737-738)

Lets analyse those lines:
1. Calls an inner method called `create_uart` and passes the index `VIRT_UART`
    too.
2. Sets local parameters which takes the base and the offset from sent index.
3. Create a device typed `pl011` which is a simple UART device you can find the
    device spec [here][pl011] though it is not necessary ro analyze it for now.
4. Create a bus to said device
5. Point from the system memory (`mem`) at the specified address (`base`)
    to the mmio region of the created device (`sysbus_mmio_get_region(s, 0)`).

Many other things happens there but this is enough to show validate for us that
there is an UART device connected at specified the address (`0x09000000`).

[pl011]: http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183f/DDI0183.pdf "pl011 spec sheet"


#### VIRT_MEM
Well, it's very nice that we know how print things to the screen... But it's not
enough, since we are writing on a bare metal machine we need to know what the
address spouse be in order for the program to run, the comment above `VIRT_MEM`
seems to be promising that this address is the RAM in which we can run our code.
Lets not trust it for now and lets try and follow the link:

1. `memory_region_add_subregion(sysmem, vms->memmap[VIRT_MEM].base, machine->ram);`
    (line: 1507)
2. `vms->bootinfo.loader_start = vms->memmap[VIRT_MEM].base;` (line: 1871)

Analyses:
1. This is quite simple, we create Pointer from the system memory (`sysmem`) to
    the RAM memory region (`machine->ram`) created previously be the QEMU system
    at the specified address (`vms->memmap[VIRT_MEM].base`).
2. We can see that the base address of the ram is also set to be the boot loader
    address. they load the kernel at line 1875 
        `arm_load_kernel(ARM_CPU(first_cpu), machine, &vms->bootinfo);`

After creating a the program in [my bare metal tutorial](first-bare-metal-program.md)

We can execute it and we will print `Hello world`.

## 'THOG' Implementation
> 'THOG' - stands for 'The Heart of Gold'.

After writing some bare metal program it's time to write our own new QEMU 
machine, let's have a little deeper look at the `virt.c` source code, and see
how can we implement a new machine (we'll call it THOG) that will be able to
run our program.

#### 'THOG' Machine Requirements
- UART device at `0x09000000`.
- Boot able code which starts at `0x400000000`

### First Steps
Every machine needs a `class_init` method, `machine_info` of `TypeInfo`
parameter, `machine_init` which register the machine type and calls `type_init`
to add the new type (`DEFINE_VIRT_MACHINE_LATEST` is a macro which adds child 
instances to the `virt` machine, using the basic `virt` as a parent and creates
a child instance, with the class_init, info, machine_init and type_init).

> Before diving into what is the propose of each requirement it's recommended
> to read the documentation in [include/qom/object.h][objecth] which describes 
> the way QEMU implements Object Oriented Programing in C, the title of the doc
> is "Base Object Type System".

[objecth]: https://github.com/qemu/qemu/blob/master/include/qom/object.h "object.h"

#### class_init
The class initializing method, used to set the machine class (its parent) 
default values.

#### machine_info
The information of the newly created type, usually will store a `TYPE_MACHINE`
as `.parent`, the class_init in `class_size`

#### machine_init
Simply register the new type.

#### type_init
Creates a constructor for the new class, calls the sent machine init

#### More Options 
##### instance_init
This function is called to initialize an object. The parent class will have
already been initialized so the type is only responsible for initializing its
own members.

##### DEFINE_MACHINE
A marco which helps shorten all most of the init requirement need only machine
init method and a machine name and it creates the other things automatically 
with some default values.
this is the implementation of this macro.
```c
#define DEFINE_MACHINE(namestr, machine_initfn) \
    static void machine_initfn##_class_init(ObjectClass *oc, void *data) \
    { \
        MachineClass *mc = MACHINE_CLASS(oc); \
        machine_initfn(mc); \
    } \
    static const TypeInfo machine_initfn##_typeinfo = { \
        .name       = MACHINE_TYPE_NAME(namestr), \
        .parent     = TYPE_MACHINE, \
        .class_init = machine_initfn##_class_init, \
    }; \
    static void machine_initfn##_register_types(void) \
    { \
        type_register_static(&machine_initfn##_typeinfo); \
    } \
    type_init(machine_initfn##_register_types)
```
--------------------------------------------------------------------------------


After understanding what are the requirements of a machine we can write short
implementation which will look something like:
```c
#include "qemu/osdep.h"
#include "hw/boards.h"

static void thog_init(MachineState *machine)
{
    // thog init method
}

static void thog_machine_init(MachineClass *mc)
{
    mc->desc = "THOG Machine";
    mc->init = thog_init;
}

DEFINE_MACHINE("thog", thog_machine_init)
```

We can compile only this file, by changing the `arm-softmmu.mak` file, and 
configure by `./configure --target-list=arm-softmmu` add the object to 
`Makefile.objs` and compile.

We can see the new machine by:
```bash
$ ./arm-softmmu/qemu-system-arm -machine help
Supported machines are:
none                 empty machine
thog                 THOG Machine
```

Of course this machine cannot do anything yet.

### Our UART Device

This is virt's uart device, we'll analyze it add see what can we in our machine:
```c
static void create_uart(const VirtMachineState *vms, int uart,
                        MemoryRegion *mem, Chardev *chr)
{
    char *nodename;
    hwaddr base = vms->memmap[uart].base;
    hwaddr size = vms->memmap[uart].size;
    int irq = vms->irqmap[uart];
    const char compat[] = "arm,pl011\0arm,primecell";
    const char clocknames[] = "uartclk\0apb_pclk";
    DeviceState *dev = qdev_create(NULL, TYPE_PL011);
    SysBusDevice *s = SYS_BUS_DEVICE(dev);

    qdev_prop_set_chr(dev, "chardev", chr);
    qdev_init_nofail(dev);
    memory_region_add_subregion(mem, base,
                                sysbus_mmio_get_region(s, 0));
    sysbus_connect_irq(s, 0, qdev_get_gpio_in(vms->gic, irq));

    nodename = g_strdup_printf("/pl011@%" PRIx64, base);
    qemu_fdt_add_subnode(vms->fdt, nodename);
    /* Note that we can't use setprop_string because of the embedded NUL */
    qemu_fdt_setprop(vms->fdt, nodename, "compatible",
                         compat, sizeof(compat));
    qemu_fdt_setprop_sized_cells(vms->fdt, nodename, "reg",
                                     2, base, 2, size);
    qemu_fdt_setprop_cells(vms->fdt, nodename, "interrupts",
                               GIC_FDT_IRQ_TYPE_SPI, irq,
                               GIC_FDT_IRQ_FLAGS_LEVEL_HI);
    qemu_fdt_setprop_cells(vms->fdt, nodename, "clocks",
                               vms->clock_phandle, vms->clock_phandle);
    qemu_fdt_setprop(vms->fdt, nodename, "clock-names",
                         clocknames, sizeof(clocknames));

    if (uart == VIRT_UART) {
        qemu_fdt_setprop_string(vms->fdt, "/chosen", "stdout-path", nodename);
    } else {
        /* Mark as not usable by the normal world */
        qemu_fdt_setprop_string(vms->fdt, nodename, "status", "disabled");
        qemu_fdt_setprop_string(vms->fdt, nodename, "secure-status", "okay");

        qemu_fdt_add_subnode(vms->fdt, "/secure-chosen");
        qemu_fdt_setprop_string(vms->fdt, "/secure-chosen", "stdout-path",
                                nodename);
    }

    g_free(nodename);
}
```

First lets have a look on the function's input:
1. `const VirtMachineState *vms` - a sturct which extends the `MachineState`, is
        used to describe the running machine. this is not a nessassery parameter
        though it exists helps the developer to store all the importent
        structures in a way which any functions which gets it also gets the fdt,
        boot info, memory map and many other paramter which might come in handy
        to the reciving functions. In the `create_uart` function it is used to
        get the memory map, the irq map (interrupt mapping), the fdt (flattened 
        device tree) and the gic device (ARM generic interrupt controller).

2. `int uart` - the index of the uart device on the memory map.

3. `MemoryRegion *mem` - the system's memory (originally fetched using the
        `get_system_memory` function), used when we will need to add the new 
        memory regions of the uart device.

4. `Chardev *chr` -  the charcter device object. obtained using 
        `serial_hd(index)`, used as the character serial device to connect with.

Secondly, lets analyze the device creation and placement in the system's memory.
We can see that we create a new qemu device of pl011 with is a simple UART 
device, we also create a sysbus device which will be used to connect the memory 
regions to.
The sent chardev is set as the chardev property of our new qdev and the UART
device is initialized.
The MMIO of the newly created bus is added as a subregion of the system's 
memory, when the base is the base address described in the memory map.
The sysbus is connected to a interrupt request a the address desribed in the
memory map using he machine's GIC.

Lastly, the fdt should be created for our new device, so when a program will 
start running (ex: the linux kernel) it will know about the device and will be
able to use the correct drivers. 

