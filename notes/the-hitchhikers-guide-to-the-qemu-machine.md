# The Hitchhiker's Guide to the QEMU Machine

## Introduction
First you my want to walk through those tutorials which I found helpful:
 - [The Quick Guide to QEMU Setup](http://opensourceforu.com/2011/05/quick-quide-to-qemu-setup/)
 - [Using QEMU for Embedded Systems Development, Part 1](https://opensourceforu.com/2011/06/qemu-for-embedded-systems-development-part-1/)
 - [Using QEMU for Embedded Systems Development, Part 2](https://opensourceforu.com/2011/07/qemu-for-embedded-systems-development-part-2/)
 - I summarized and updated it in [first bare metal program](first-bare-metal-program.md)

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
    device spec [here][1] though it is not necessary ro analyze it for now.
4. Create a bus to said device
5. Point from the system memory (`mem`) at the specified address (`base`)
    to the mmio region of the created device (`sysbus_mmio_get_region(s, 0)`).

Many other things happens there but this is enough to show validate for us that
there is an UART device connected at specified the address (`0x09000000`).

[1]: http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183f/DDI0183.pdf "pl011 spec sheet"


#### VIRT_MEM
Well, it's very nice that we know how print things to the screen... But it's not
enough, since we are writing on a bare metal machine we need to know what the
address spouse be in order for the program to run, the comment above `VIRT_MEM`
seems to be promising that this address is the RAM in which we can run our code.
Lets not trust it for now and lets try and follow the link:

1. `memory_region_add_subregion(sysmem, vms->memmap[VIRT_MEM].base, machine->ram);`
    (line: 1507)

Analyses:
1. This is quite simple, we create Pointer from the system memory (`sysmem`) to
    the RAM memory region (`machine->ram`) created previously be the QEMU system
    at the specified address (`vms->memmap[VIRT_MEM].base`).

After creating a the program in [my bare metal tutorial](first-bare-metal-program.md)

We can execute it and we will print `Hello world`.