/*
 * SPDX-License-Identifier:    Apache-2.0
 *
 * Copyright 2018 Canaan Inc.
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <sbi/riscv_asm.h>

/* clang-format off */

#define K210_HART_COUNT		2
#define K210_HART_STACK_SIZE	4096

/* Register base address */

/* Under Coreplex */
#define CLINT_BASE_ADDR		(0x02000000U)
#define PLIC_BASE_ADDR		(0x0C000000U)
#define PLIC_NUM_CORES		(K210_HART_COUNT)

/* Under TileLink */
#define GPIOHS_BASE_ADDR	(0x38001000U)

/* Under AXI 64 bit */
#define RAM_BASE_ADDR		(0x80000000U)
#define RAM_SIZE		(6 * 1024 * 1024U)

#define IO_BASE_ADDR		(0x40000000U)
#define IO_SIZE			(6 * 1024 * 1024U)

#define AI_RAM_BASE_ADDR	(0x80600000U)
#define AI_RAM_SIZE		(2 * 1024 * 1024U)

#define AI_IO_BASE_ADDR		(0x40600000U)
#define AI_IO_SIZE		(2 * 1024 * 1024U)

#define AI_BASE_ADDR		(0x40800000U)
#define AI_SIZE			(12 * 1024 * 1024U)

#define FFT_BASE_ADDR		(0x42000000U)
#define FFT_SIZE		(4 * 1024 * 1024U)

#define ROM_BASE_ADDR		(0x88000000U)
#define ROM_SIZE		(128 * 1024U)

/* Under AHB 32 bit */
#define DMAC_BASE_ADDR		(0x50000000U)

/* Under APB1 32 bit */
#define GPIO_BASE_ADDR		(0x50200000U)
#define UART1_BASE_ADDR		(0x50210000U)
#define UART2_BASE_ADDR		(0x50220000U)
#define UART3_BASE_ADDR		(0x50230000U)
#define SPI_SLAVE_BASE_ADDR	(0x50240000U)
#define I2S0_BASE_ADDR		(0x50250000U)
#define I2S1_BASE_ADDR		(0x50260000U)
#define I2S2_BASE_ADDR		(0x50270000U)
#define I2C0_BASE_ADDR		(0x50280000U)
#define I2C1_BASE_ADDR		(0x50290000U)
#define I2C2_BASE_ADDR		(0x502A0000U)
#define FPIOA_BASE_ADDR		(0x502B0000U)
#define SHA256_BASE_ADDR	(0x502C0000U)
#define TIMER0_BASE_ADDR	(0x502D0000U)
#define TIMER1_BASE_ADDR	(0x502E0000U)
#define TIMER2_BASE_ADDR	(0x502F0000U)

/* Under APB2 32 bit */
#define WDT0_BASE_ADDR		(0x50400000U)
#define WDT1_BASE_ADDR		(0x50410000U)
#define OTP_BASE_ADDR		(0x50420000U)
#define DVP_BASE_ADDR		(0x50430000U)
#define SYSCTL_BASE_ADDR	(0x50440000U)
#define AES_BASE_ADDR		(0x50450000U)
#define RTC_BASE_ADDR		(0x50460000U)

/* Under APB3 32 bit */
#define SPI0_BASE_ADDR		(0x52000000U)
#define SPI1_BASE_ADDR		(0x53000000U)
#define SPI3_BASE_ADDR		(0x54000000U)

#define read_cycle()		csr_read(CSR_MCYCLE)

/*
 * PLIC External Interrupt Numbers
 */
enum plic_irq {
	IRQN_NO_INTERRUPT        = 0, /*!< The non-existent interrupt */
	IRQN_SPI0_INTERRUPT      = 1, /*!< SPI0 interrupt */
	IRQN_SPI1_INTERRUPT      = 2, /*!< SPI1 interrupt */
	IRQN_SPI_SLAVE_INTERRUPT = 3, /*!< SPI_SLAVE interrupt */
	IRQN_SPI3_INTERRUPT      = 4, /*!< SPI3 interrupt */
	IRQN_I2S0_INTERRUPT      = 5, /*!< I2S0 interrupt */
	IRQN_I2S1_INTERRUPT      = 6, /*!< I2S1 interrupt */
	IRQN_I2S2_INTERRUPT      = 7, /*!< I2S2 interrupt */
	IRQN_I2C0_INTERRUPT      = 8, /*!< I2C0 interrupt */
	IRQN_I2C1_INTERRUPT      = 9, /*!< I2C1 interrupt */
	IRQN_I2C2_INTERRUPT      = 10, /*!< I2C2 interrupt */
	IRQN_UART1_INTERRUPT     = 11, /*!< UART1 interrupt */
	IRQN_UART2_INTERRUPT     = 12, /*!< UART2 interrupt */
	IRQN_UART3_INTERRUPT     = 13, /*!< UART3 interrupt */
	IRQN_TIMER0A_INTERRUPT   = 14, /*!< TIMER0 channel 0 or 1 interrupt */
	IRQN_TIMER0B_INTERRUPT   = 15, /*!< TIMER0 channel 2 or 3 interrupt */
	IRQN_TIMER1A_INTERRUPT   = 16, /*!< TIMER1 channel 0 or 1 interrupt */
	IRQN_TIMER1B_INTERRUPT   = 17, /*!< TIMER1 channel 2 or 3 interrupt */
	IRQN_TIMER2A_INTERRUPT   = 18, /*!< TIMER2 channel 0 or 1 interrupt */
	IRQN_TIMER2B_INTERRUPT   = 19, /*!< TIMER2 channel 2 or 3 interrupt */
	IRQN_RTC_INTERRUPT       = 20, /*!< RTC tick and alarm interrupt */
	IRQN_WDT0_INTERRUPT      = 21, /*!< Watching dog timer0 interrupt */
	IRQN_WDT1_INTERRUPT      = 22, /*!< Watching dog timer1 interrupt */
	IRQN_APB_GPIO_INTERRUPT  = 23, /*!< APB GPIO interrupt */
	IRQN_DVP_INTERRUPT       = 24, /*!< Digital video port interrupt */
	IRQN_AI_INTERRUPT        = 25, /*!< AI accelerator interrupt */
	IRQN_FFT_INTERRUPT       = 26, /*!< FFT accelerator interrupt */
	IRQN_DMA0_INTERRUPT      = 27, /*!< DMA channel0 interrupt */
	IRQN_DMA1_INTERRUPT      = 28, /*!< DMA channel1 interrupt */
	IRQN_DMA2_INTERRUPT      = 29, /*!< DMA channel2 interrupt */
	IRQN_DMA3_INTERRUPT      = 30, /*!< DMA channel3 interrupt */
	IRQN_DMA4_INTERRUPT      = 31, /*!< DMA channel4 interrupt */
	IRQN_DMA5_INTERRUPT      = 32, /*!< DMA channel5 interrupt */
	IRQN_UARTHS_INTERRUPT    = 33, /*!< Hi-speed UART0 interrupt */
	IRQN_GPIOHS0_INTERRUPT   = 34, /*!< Hi-speed GPIO0 interrupt */
	IRQN_GPIOHS1_INTERRUPT   = 35, /*!< Hi-speed GPIO1 interrupt */
	IRQN_GPIOHS2_INTERRUPT   = 36, /*!< Hi-speed GPIO2 interrupt */
	IRQN_GPIOHS3_INTERRUPT   = 37, /*!< Hi-speed GPIO3 interrupt */
	IRQN_GPIOHS4_INTERRUPT   = 38, /*!< Hi-speed GPIO4 interrupt */
	IRQN_GPIOHS5_INTERRUPT   = 39, /*!< Hi-speed GPIO5 interrupt */
	IRQN_GPIOHS6_INTERRUPT   = 40, /*!< Hi-speed GPIO6 interrupt */
	IRQN_GPIOHS7_INTERRUPT   = 41, /*!< Hi-speed GPIO7 interrupt */
	IRQN_GPIOHS8_INTERRUPT   = 42, /*!< Hi-speed GPIO8 interrupt */
	IRQN_GPIOHS9_INTERRUPT   = 43, /*!< Hi-speed GPIO9 interrupt */
	IRQN_GPIOHS10_INTERRUPT  = 44, /*!< Hi-speed GPIO10 interrupt */
	IRQN_GPIOHS11_INTERRUPT  = 45, /*!< Hi-speed GPIO11 interrupt */
	IRQN_GPIOHS12_INTERRUPT  = 46, /*!< Hi-speed GPIO12 interrupt */
	IRQN_GPIOHS13_INTERRUPT  = 47, /*!< Hi-speed GPIO13 interrupt */
	IRQN_GPIOHS14_INTERRUPT  = 48, /*!< Hi-speed GPIO14 interrupt */
	IRQN_GPIOHS15_INTERRUPT  = 49, /*!< Hi-speed GPIO15 interrupt */
	IRQN_GPIOHS16_INTERRUPT  = 50, /*!< Hi-speed GPIO16 interrupt */
	IRQN_GPIOHS17_INTERRUPT  = 51, /*!< Hi-speed GPIO17 interrupt */
	IRQN_GPIOHS18_INTERRUPT  = 52, /*!< Hi-speed GPIO18 interrupt */
	IRQN_GPIOHS19_INTERRUPT  = 53, /*!< Hi-speed GPIO19 interrupt */
	IRQN_GPIOHS20_INTERRUPT  = 54, /*!< Hi-speed GPIO20 interrupt */
	IRQN_GPIOHS21_INTERRUPT  = 55, /*!< Hi-speed GPIO21 interrupt */
	IRQN_GPIOHS22_INTERRUPT  = 56, /*!< Hi-speed GPIO22 interrupt */
	IRQN_GPIOHS23_INTERRUPT  = 57, /*!< Hi-speed GPIO23 interrupt */
	IRQN_GPIOHS24_INTERRUPT  = 58, /*!< Hi-speed GPIO24 interrupt */
	IRQN_GPIOHS25_INTERRUPT  = 59, /*!< Hi-speed GPIO25 interrupt */
	IRQN_GPIOHS26_INTERRUPT  = 60, /*!< Hi-speed GPIO26 interrupt */
	IRQN_GPIOHS27_INTERRUPT  = 61, /*!< Hi-speed GPIO27 interrupt */
	IRQN_GPIOHS28_INTERRUPT  = 62, /*!< Hi-speed GPIO28 interrupt */
	IRQN_GPIOHS29_INTERRUPT  = 63, /*!< Hi-speed GPIO29 interrupt */
	IRQN_GPIOHS30_INTERRUPT  = 64, /*!< Hi-speed GPIO30 interrupt */
	IRQN_GPIOHS31_INTERRUPT  = 65, /*!< Hi-speed GPIO31 interrupt */
	IRQN_MAX
};

/* IRQ number settings */
#define PLIC_NUM_SOURCES    (IRQN_MAX - 1)
#define PLIC_NUM_PRIORITIES (7)

/* clang-format on */

#endif /* _PLATFORM_H_ */
