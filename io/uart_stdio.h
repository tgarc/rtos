#ifndef UART_STDIO_H
#define UART_STDIO_H

void UARTIOEnable(void);
void UARTIOOutputRedirect(void (*cb)(char c));

#endif
