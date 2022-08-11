/*****************************************************************************
* Product: Front Panel example, BSP for EK-LM3S811 board
* Last Update: 2016-05-03
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the following MIT License (MIT).
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
* Contact information:
* https://state-machine.com
* mailto:info@state-machine.com
*****************************************************************************/
#include <stdint.h>
//#include <intrinsics.h>

#include "bsp.h"            /* BSP interface */
#include "LM3S811.h"        /* the MCU-specific header (TI) */
#include "display96x16x1.h" /* the OLED display header (TI) */

#define PUSH_BUTTON             (1U << 4)
#define USER_LED                (1U << 5)

/* Local-scope objects -----------------------------------------------------*/
static int volatile l_tickCtr;
static int l_paused;


/* prototypes of ISRs defined in the BSP....................................*/
void SysTick_Handler(void);

/*..........................................................................*/
void SysTick_Handler(void) {
    static uint32_t btn_debounced  = PUSH_BUTTON;
    static uint8_t  debounce_state = 0U;
    uint32_t btn;

    ++l_tickCtr;  /* increment the tick counter */

    btn = GPIOC->DATA_Bits[PUSH_BUTTON]; /* read the push button */
    switch (debounce_state) {
        case 0:
            if (btn != btn_debounced) {
                debounce_state = 1U; /* transition to the next state */
            }
            break;
        case 1:
            if (btn != btn_debounced) {
                debounce_state = 2U; /* transition to the next state */
            }
            else {
                debounce_state = 0U; /* transition back to state 0 */
            }
            break;
        case 2:
            if (btn != btn_debounced) {
                debounce_state = 3U; /* transition to the next state */
            }
            else {
                debounce_state = 0U; /* transition back to state 0 */
            }
            break;
        case 3:
            if (btn != btn_debounced) {
                btn_debounced = btn; /* save the debounced button value */

                if (btn == 0U) {     /* is the button depressed? */
                    BSP_setPaused(1U);
                }
                else {
                    BSP_setPaused(0U);
                }
            }
            debounce_state = 0U;     /* transition back to state 0 */
            break;
    }
}

/*..........................................................................*/
void BSP_init(void) {
    /* NOTE: SystemInit() already called from startup_LM3S811.c
    *  but SystemCoreClock needs to be updated
    */
    SystemCoreClockUpdate();

    /* enable clock to the peripherals used by the application */
    SYSCTL->RCGC1 |= (1 << 16) | (1 << 17); /* enable clock to TIMER0 & 1 */
    SYSCTL->RCGC2 |= (1 <<  0) | (1 <<  2); /* enable clock to GPIOA & C  */
    __NOP();                                /* wait after enabling clocks */
    __NOP();
    __NOP();

    /* configure the LED and push button */
    GPIOC->DIR |= USER_LED;         /* set direction: output */
    GPIOC->DEN |= USER_LED;         /* digital enable */
    GPIOC->DATA_Bits[USER_LED] = 0; /* turn the User LED off */

    GPIOC->DIR &= ~PUSH_BUTTON;     /*  set direction: input */
    GPIOC->DEN |= PUSH_BUTTON;      /* digital enable */

    Display96x16x1Init(1);          /* initialize the OLED display */

    /* set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate */
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);

    /* set priorities of all interrupts in the system... */
    NVIC_SetPriority(SysTick_IRQn,   0);

    TIMER1->CTL |= ((1 << 0) | (1 << 16)); /* enable TIMER1 */
}
/*..........................................................................*/
void BSP_terminate(int result) {
}
/*..........................................................................*/
void BSP_sleep(uint32_t msec) {
    int tickCtr = l_tickCtr;
    while (tickCtr == l_tickCtr) { /* wait for the tickCtr change */
    }
}
/*..........................................................................*/
void BSP_setPaused(int paused) {
    l_paused = paused;
    if (l_paused) {
        GPIOC->DATA_Bits[USER_LED] = USER_LED; /* turn the User LED on  */
        Display96x16x1Clear();
        BSP_drawNString(35U, 0U, "PAUSED");
    }
    else {
        GPIOC->DATA_Bits[USER_LED] = 0U; /* turn the User LED off */
    }
}
/*..........................................................................*/
int BSP_isPaused(void) {
    return l_paused;
}

/*..........................................................................*/
void BSP_drawBitmap(uint8_t const *bitmap) {
    Display96x16x1ImageDraw(bitmap, 0, 0,
                            BSP_SCREEN_WIDTH, (BSP_SCREEN_HEIGHT >> 3));
}
/*..........................................................................*/
void BSP_drawCount(uint32_t n) {
}
/*..........................................................................*/
void BSP_drawNString(uint8_t x, uint8_t y, char const *str) {
    Display96x16x1StringDraw(str, x, y);
}

/*..........................................................................*/
void Q_onAssert(char const *module, int loc); /* prototype */
void Q_onAssert(char const *module, int loc) {
    /* NOTE: add here your application-specific error handling... */
    (void)module;
    (void)loc;

    NVIC_SystemReset(); /* reset the MCU */
}

