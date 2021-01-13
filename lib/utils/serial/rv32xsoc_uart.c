#include <sbi_utils/serial/rv32xsoc_uart.h>

int rv32xsoc_uart_init(void) {
	return 0;
}
int rv32xsoc_uart_putchar(int ch) {

	while(RV32XSOC_UART_TX_GET_STAT_FULL())
		asm volatile("nop");

	*RV32XSOC_UART_TX_BUF = ch;
	RV32XSOC_UART_TX_SET_EN(1);

	return (unsigned int) ch;
}

int rv32xsoc_uart_getchar(void) {
	int ch = -1;

	RV32XSOC_UART_RX_SET_EN(1);

	/* Blocking */
	while(RV32XSOC_UART_RX_GET_STAT_EMPTY()) {
		asm volatile("nop");
	}
	ch = *RV32XSOC_UART_RX_BUF;
	return ch;
}
void __attribute__((weak)) rv32xsoc_uart_rx_interrupt_handler(void) {
}
void __attribute__((weak)) rv32xsoc_uart_tx_interrupt_handler(void) {
}


