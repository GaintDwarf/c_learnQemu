
// Where the UART0 Serial sits
volatile unsigned char * const UART0_PTR = (unsigned char *)0x09000000;

// copy a string to the uart device
void display(const char *string)
{
    while((*UART0_PTR = *(string++)) != '\0');
}
 
int my_init(){
        display("Hello World\n");
}

