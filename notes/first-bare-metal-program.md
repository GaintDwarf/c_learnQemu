# First Bare-Metal Program
> source : https://opensourceforu.com/2011/07/qemu-for-embedded-systems-development-part-2/

## The Application
We need some small c program, find what is the address of UART0.

###### test.c
--------------------------------------------------------------------------------
```c
volatile unsigned char * const UART0_PTR = (unsigned char *)0x(The adderess of the uart device);
void print(const char *str)
{
    while(*str)
    {
        UART0_PTR = *(str++);
    }
}

int main()
{
    print("hello world");
}
```

And now we can print to the screen.

## The Startup Code
We need some instructions to tell the processor where to start. (this is arm assembly code).

###### startup.s
---------------------------------------------------------------------------------------------
```assembly
.global _Start
_Start:
    LDR sp, = sp_top          # Set the stack address to the top
    BL  (your main function)  # Jump to the main function
    B   .                     # infinite loop to prevent crush
```

## The Linker
In order to make sure that the program will sit in the currect addresses/section we need to write a short
linker script for the the compiler.

###### linker.ld
--------------------------------------------------------------------------------------------
```LD
ENTRY(_Start)                           # Set the entry point to the program
SECTIONS                                # Specify the sections
{
    . = 0x4000000;                      # The start address id 0x4000000 (where the execution 
                                        #                               start when using -kernel 
                                        #                               in the qemu)
    startup : { startup.o(.text)}       # Mark that this is where the startup will sit (the name of the 
                                        #                                        compiled assembly file)
    .data : {*(.data)}                  # Set the location of the data segement
    .bss : {*(.bss)}                    # Set the location of the bss (staticlly allocated variables)
    . = . + 0x500;                      # Leave some space to the stack (0x500 will be the max length 
                                        #                                of the stack)
    sp_top = .;                         # Save the top of the stack (used in the startup.s)
}
```

## Compiling and running on qemu

> in this example compileing to a normal arm cpu which is matching for `virt` qemu machine (ARM).

- assemble the startup.s file ```arm-none-eabi-as  startup.s -o startup.o ```
- assemble the test.c file    ```arm-none-eabi-gcc -c test.c -o init.o ```
- link files into an elf file ```arm-none-eabi-ld  -T linker.ld init.o startup.o -o output.elf ```
- run on qemu                 ```qemu-system-arm -M virt -nographic -kernel output.elf```


