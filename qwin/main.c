/*****************************************************************************
* Product: QWIN GUI demo
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
#include <string.h>  /* for memset(), memmove() */
#include <math.h>
#include "bsp.h"

/* Local-scope objects -----------------------------------------------------*/
static uint8_t l_frameBuf[BSP_SCREEN_WIDTH * BSP_SCREEN_HEIGHT/8U];

/*..........................................................................*/
static int fun(int n) { /* function to show on the OLED display */
    double x = 3.1415926536/10.0 * n;
    double y = 0.5*(sin(x)*cos(0.1*x) + 1.0); /* range 0.0 .. 1.0 */
    return (int)(y*(BSP_SCREEN_HEIGHT - 1U) + 0.5);
}

/*..........................................................................*/
int main() {
    uint32_t n = 0U;

    BSP_init(); /* initialize the Board Support Package */

    /* clear the screen */
    memset(l_frameBuf, 0U, (BSP_SCREEN_WIDTH * BSP_SCREEN_HEIGHT/8U));

    for (;;) { /* for-ever */
        /* generate the new pixel column in the display based on the count */
        uint32_t pixCol = (1U << fun(n));

        /* advance the frame buffer by 1 pixel to the left */
        memmove(l_frameBuf, l_frameBuf + 1U,
                (BSP_SCREEN_WIDTH * BSP_SCREEN_HEIGHT/8U) - 1U);

        /* add the new column to the buffer at the right */
        l_frameBuf[BSP_SCREEN_WIDTH - 1U] = (uint8_t)pixCol;
        l_frameBuf[BSP_SCREEN_WIDTH + BSP_SCREEN_WIDTH - 1U] =
                (uint8_t)(pixCol >> 8);

        if (!BSP_isPaused()) {          /* not paused? */
            BSP_drawBitmap(l_frameBuf); /* draw the bitmap on the display */
            BSP_drawCount(n);
            ++n; /* advance the count */
        }

        BSP_sleep(1); /* sleep for 1 clock tick */
    }
    //return 0;
}

