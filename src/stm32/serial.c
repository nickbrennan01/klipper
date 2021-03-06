// STM32 serial
//
// Copyright (C) 2019  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "autoconf.h" // CONFIG_SERIAL_BAUD
#include "board/serial_irq.h" // serial_rx_byte
#include "command.h" // DECL_CONSTANT_STR
#include "internal.h" // enable_pclock
#include "sched.h" // DECL_INIT

// Select the configured serial port
#if CONFIG_SERIAL_PORT == 1
DECL_CONSTANT_STR("RESERVE_PINS_serial", "PA10,PA9");
#define GPIO_Rx GPIO('A', 10)
#define GPIO_Tx GPIO('A', 9)
#define USARTx USART1
#define USARTx_IRQn USART1_IRQn
#define USARTx_IRQHandler USART1_IRQHandler
#elif CONFIG_SERIAL_PORT == 2
DECL_CONSTANT_STR("RESERVE_PINS_serial", "PA3,PA2");
#define GPIO_Rx GPIO('A', 3)
#define GPIO_Tx GPIO('A', 2)
#define USARTx USART2
#define USARTx_IRQn USART2_IRQn
#define USARTx_IRQHandler USART2_IRQHandler
#elif CONFIG_SERIAL_PORT == 103
DECL_CONSTANT_STR("RESERVE_PINS_serial", "PD9,PD8");
#define GPIO_Rx GPIO('D', 9)
#define GPIO_Tx GPIO('D', 8)
#define USARTx USART3
#define USARTx_IRQn USART3_IRQn
#define USARTx_IRQHandler USART3_IRQHandler
#else
DECL_CONSTANT_STR("RESERVE_PINS_serial", "PB11,PB10");
#define GPIO_Rx GPIO('B', 11)
#define GPIO_Tx GPIO('B', 10)
#define USARTx USART3
#define USARTx_IRQn USART3_IRQn
#define USARTx_IRQHandler USART3_IRQHandler
#endif

#define CR1_FLAGS (USART_CR1_UE | USART_CR1_RE | USART_CR1_TE   \
                   | USART_CR1_RXNEIE)

void
serial_init(void)
{
    enable_pclock((uint32_t)USARTx);

    uint32_t pclk = get_pclock_frequency((uint32_t)USARTx);
    uint32_t div = DIV_ROUND_CLOSEST(pclk, CONFIG_SERIAL_BAUD);
    USARTx->BRR = (((div / 16) << USART_BRR_DIV_Mantissa_Pos)
                   | ((div % 16) << USART_BRR_DIV_Fraction_Pos));
    USARTx->CR1 = CR1_FLAGS;
    NVIC_SetPriority(USARTx_IRQn, 0);
    NVIC_EnableIRQ(USARTx_IRQn);

    gpio_peripheral(GPIO_Rx, GPIO_FUNCTION(7), 1);
    gpio_peripheral(GPIO_Tx, GPIO_FUNCTION(7), 0);
}
DECL_INIT(serial_init);

void __visible
USARTx_IRQHandler(void)
{
    uint32_t sr = USARTx->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE))
        serial_rx_byte(USARTx->DR);
    if (sr & USART_SR_TXE && USARTx->CR1 & USART_CR1_TXEIE) {
        uint8_t data;
        int ret = serial_get_tx_byte(&data);
        if (ret)
            USARTx->CR1 = CR1_FLAGS;
        else
            USARTx->DR = data;
    }
}

void
serial_enable_tx_irq(void)
{
    USARTx->CR1 = CR1_FLAGS | USART_CR1_TXEIE;
}
