ENTRY(_Start)
SECTIONS
{
    . = 0x40000000;
    startup : { startup.o(.text)}
    .data : {*(.data)}
    .bss : {*(.bss)}
    . = . + 0x500;
    sp_top = .;
}

