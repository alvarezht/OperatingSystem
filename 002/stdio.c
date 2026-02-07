#include <stdarg.h>

#define UART0_BASE 0x101f1000

#define UART_DR      0x00  // Data Register
#define UART_FR      0x18  // Flag Register
#define UART_FR_TXFF 0x20  // Transmit FIFO Full
#define UART_FR_RXFE 0x10  // Receive FIFO Empty

volatile unsigned int * const UART0 = (unsigned int *)UART0_BASE;

// Function to send a single character via UART
void uart_putc(char c) {
    // Wait until there is space in the FIFO
    while (UART0[UART_FR / 4] & UART_FR_TXFF);
    UART0[UART_DR / 4] = c;
}

// Function to receive a single character via UART
char uart_getc() {
    // Wait until data is available
    while (UART0[UART_FR / 4] & UART_FR_RXFE);
    return (char)(UART0[UART_DR / 4] & 0xFF);
}

// Function to send a string via UART
void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Function to receive a line of input via UART
void uart_gets_input(char *buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) { // Leave space for null terminator
        c = uart_getc();
        if (c == '\n' || c == '\r') {
            uart_putc('\n'); // Echo newline
            break;
        }
        uart_putc(c); // Echo character
        buffer[i++] = c;
    }
    buffer[i] = '\0'; // Null terminate the string
}

// Simple function to convert string to integer
int uart_atoi(const char *s) {
    int num = 0;
    int sign = 1;
    int i = 0;

    // Handle optional sign
    if (s[i] == '-') {
        sign = -1;
        i++;
    }

    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num * 10 + (s[i] - '0');
    }

    return sign * num;
}

// Function to convert integer to string
void uart_itoa(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0 && i < 14) { // Reserve space for sign and null terminator
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the string
    int start = 0, end = i - 1;
    char temp;
    while (start < end) {
        temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

float uart_atof(const char *s){
    float num = 0.0;
    int sign = 1;
    
    int i = 0;
    double divisor = 1.0;

    if(s[i] == '-') {
        sign = -1;
        i++;
    }

    for(; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num * 10 + (s[i] - '0');
    }

    if(s[i] == '.') {
        i++;
        for(; s[i] >= '0' && s[i] <= '9'; i++) {
            num = num * 10 + (s[i] - '0');
            divisor *= 10.0;
        }
    }

    return sign * (num / divisor);
}

void uart_ftoa(float num, char *buffer, int *precision) {
    int i = 0;
    int is_negative = 0;
    
    if (num < 0) {
        is_negative = 1;
        buffer[i++] = '-';
        num = -num;
    }
    
    int int_part = (int)num;
    float float_part = num - (float)int_part;
    
    char int_str[15];
    uart_itoa(int_part, int_str);
    
    int j = 0;
    while (int_str[j] != '\0') {
        buffer[i++] = int_str[j++];
    }
    
    if (*precision > 0) {
        buffer[i++] = '.';
        
        for (int k = 0; k < *precision; k++) {
            float_part = float_part * 10;
            int digit = (int)float_part;
            buffer[i++] = '0' + digit;
            float_part -= digit;
        }
    }
    
    buffer[i] = '\0';
}

void PRINT(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[32];
    int i = 0;
    
    while (format[i] != '\0') {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'd') {
                int val = va_arg(args, int);
                uart_itoa(val, buffer);
                uart_puts(buffer);
            } else if (format[i] == 'f') {
                float val = va_arg(args, double);
                int precision = 6;
                uart_ftoa(val, buffer, &precision);
                uart_puts(buffer);
            } else if (format[i] == '.' && (format[i+1] >= '0' || format[i+1] <= '9') && format[i+2] == 'f') {
                float val = va_arg(args, double);
                int precision = format[i+1] - '0';
                uart_ftoa(val, buffer, &precision);
                uart_puts(buffer);
                i++;
                i++;
            } else {
                uart_putc('%');
                uart_putc(format[i]);
            }
        } else {
            uart_putc(format[i]);
        }
        i++;
    }
    
    va_end(args);
}

void READ(const char *format, void *ptr) {
    char buffer[32];
    uart_gets_input(buffer, 32);
    
    int i = 0;
    while (format[i] != '\0') {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'd') {
                int *p = (int *)ptr;
                *p = uart_atoi(buffer);
                return;
            } else if (format[i] == 'f') {
                float *p = (float *)ptr;
                *p = uart_atof(buffer);
                return;
            }
        }
        i++;
    }
}
