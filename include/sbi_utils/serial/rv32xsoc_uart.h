#ifndef RV32XSOC_UART_H
#define RV32XSOC_UART_TX_BUF ((volatile unsigned int *) 0x40000000)
#define RV32XSOC_UART_TX_STAT ((volatile unsigned int *) 0x40000004)

#define RV32XSOC_UART_RX_BUF ((volatile unsigned int *) 0x40000010)
#define RV32XSOC_UART_RX_STAT ((volatile unsigned int *) 0x40000014)

#define RV32XSOC_UART_TX_GET_STAT_FULL() (((*(RV32XSOC_UART_TX_STAT)) & 0x00000008) >> 3)
#define RV32XSOC_UART_TX_GET_STAT_EMPTY() (((*(RV32XSOC_UART_TX_STAT)) & 0x00000004) >> 2)
#define RV32XSOC_UART_TX_GET_STAT_BUSY() (((*(RV32XSOC_UART_TX_STAT)) & 0x00000002) >> 1)
#define RV32XSOC_UART_TX_GET_STAT_EN() (((*(RV32XSOC_UART_TX_STAT)) & 0x00000001))
#define RV32XSOC_UART_TX_SET_EN(en) (*(RV32XSOC_UART_TX_STAT) = *(RV32XSOC_UART_TX_STAT) | ((en) & 0x1));

#define RV32XSOC_UART_RX_GET_STAT_FULL() (((*(RV32XSOC_UART_RX_STAT)) & 0x00000008) >> 3)
#define RV32XSOC_UART_RX_GET_STAT_EMPTY() (((*(RV32XSOC_UART_RX_STAT)) & 0x00000004) >> 2)
#define RV32XSOC_UART_RX_GET_STAT_BUSY() (((*(RV32XSOC_UART_RX_STAT)) & 0x00000002) >> 1)
#define RV32XSOC_UART_RX_GET_STAT_EN() (((*(RV32XSOC_UART_RX_STAT)) & 0x00000001))
#define RV32XSOC_UART_RX_SET_EN(en) (*(RV32XSOC_UART_RX_STAT) = *(RV32XSOC_UART_RX_STAT) | ((en) & 0x1));

typedef union {
	struct {
		unsigned int en : 1;	
		unsigned int busy : 1;	
		unsigned int empty : 1;
		unsigned int full : 1;
		unsigned int unused: 28;
	} stat;
	unsigned int val;
} rv32xsoc_uart_tx_stat_t;

typedef rv32xsoc_uart_tx_stat_t rv32xsoc_uart_rx_stat_t;

int rv32xsoc_uart_init(void);
int rv32xsoc_uart_putchar(int ch);
int rv32xsoc_uart_getchar(void);

extern __attribute__((weak)) void rv32xsoc_uart_rx_interrupt_handler(void);
extern __attribute__((weak)) void rv32xsoc_uart_tx_interrupt_handler(void);
#endif
