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

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPEC 4

#define PRINTF_LENGTH_DEFAULT 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4

void printf(const char* fmt,...)
{
    int* argp = (int *)&fmt;
    argp++;
    int state = PRINTF_STATE_NORMAL;
    int length;
    while (*fmt)
    {
        switch (state)
        {
        case PRINTF_STATE_NORMAL:
            switch (*fmt)
            {
            case '%':
                state = PRINTF_STATE_LENGTH;
                break;
            default:
                putc(*fmt);
                break;
            }
            break;
        
        case PRINTF_STATE_LENGTH:
            switch (*fmt)
            {
                case 'h':

                break;

                case 'l':

                break;
            }

            break;
        }
        fmt++;
    }
    
}