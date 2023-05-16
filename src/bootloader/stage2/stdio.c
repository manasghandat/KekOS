#include "stdio.h"
#include "x86.h"

/*
* Prints a  single character to the screen
* Parameters:
*   @c : character to print
*/
void putc(char c)
{
    x86_Video_WriteCharTeletype(c, 0);
}

/*
* Prints string to the screen
* Parameters:
*   @str : pointer to the buffer to print
*/
void puts(const char* str)
{
    while (*str)
    {
        putc(*str);
        *str++;
    }
}