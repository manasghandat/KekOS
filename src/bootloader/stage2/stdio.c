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

void puts_f(const char far* str)
{
    while (*str)
    {
        putc(*str);
        *str++;
    }
}

int* printf_nummber(int *argp, int length, bool sign, int radix);

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
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = 10;
    bool sign = false; 
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
                length = PRINTF_LENGTH_SHORT;
                state = PRINTF_STATE_LENGTH_SHORT;
                break;
                
                case 'l':
                length = PRINTF_LENGTH_LONG;
                state = PRINTF_STATE_LENGTH_LONG;
                break;

                default:
                goto PRINTF_STATE_SPEC_;
            }
            break;

        case PRINTF_STATE_LENGTH_LONG:
            if(*fmt == 'l')
            {
                state = PRINTF_STATE_SPEC;
                length = PRINTF_LENGTH_LONG_LONG;
            }
            else
            {
                goto PRINTF_STATE_SPEC_;
            }
            break;

        case PRINTF_STATE_LENGTH_SHORT:
            if(*fmt == 'h')
            {
                state = PRINTF_STATE_SPEC;
                length = PRINTF_LENGTH_SHORT_SHORT;
            }
            else
            {
                goto PRINTF_STATE_SPEC_;
            }
            break;

        case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
            switch (*fmt)
            {
            case 'c':
                putc((char *)*argp);
                argp++;
                break;

            case 's':
                if (length == PRINTF_LENGTH_LONG_LONG || length == PRINTF_LENGTH_LONG)
                {
                    puts_f(*(const char far**)argp);
                    argp+=2;
                }
                else
                {
                    puts(*(const char*)argp);
                    argp++;
                }
                break;
            case '%':
                putc('%');
                break;

            case 'd':
            case 'l':
                radix = 10;
                sign = true;
                argp = printf_number(argp,length,sign,radix);
                break;

            case 'u':
                radix = 10;
                sign = false;
                argp = printf_number(argp,length,sign,radix);
                break;

            case 'X':
            case 'x':
            case 'p':
                radix = 16;
                sign = false;
                argp = printf_number(argp,length,sign,radix);
                break;

            case 'o':
                radix = 8;
                sign = false;
                argp = printf_number(argp,length,sign,radix);
                break;

            default:
                break;
            }

            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            radix = 10;
            sign = false;
            break;

        }
        fmt++;
    }
    
}

const char g_HexChars[] = "0123456789abcdef";

int* printf_nummber(int *argp, int length, bool sign, int radix)
{
    char buf[0x20];
    unsigned long long number;
    int number_signed = 1;
    int pos = 0;

    switch (length)
    {
        case PRINTF_LENGTH_SHORT_SHORT:
        case PRINTF_LENGTH_SHORT:
        case PRINTF_LENGTH_DEFAULT:
            if(sign)
            {
                int n = *argp;
                if(n < 0)
                {
                    n = -n;
                    number_signed = -1;
                }
                number = (unsigned long long)n;
            }
            else
            {
                number = *(unsigned int*)argp;
            }
            argp++;
            break;
        
        case PRINTF_LENGTH_LONG:
            if(sign)
            {
                long int n = *(long int*)argp;
                if(n < 0)
                {
                    n = -n;
                    number_signed = -1;
                }
                number = (unsigned long long)n;
            }
            else
            {
                number = *(unsigned long int*)argp;
            }
            argp += 2;
            break;
            
        case PRINTF_LENGTH_LONG_LONG:
            if(sign)
            {
                long long int n = *(long long int*)argp;
                if(n < 0)
                {
                    n = -n;
                    number_signed = -1;
                }
                number = (unsigned long long)n;
            }
            else
            {
                number = *(unsigned long long int*)argp;
            }
            argp += 4;
            break;
        
    }

    do
    {
        
    } while (number > 0);
    


}