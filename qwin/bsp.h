/*****************************************************************************
* Product: BSP for QWIN GUI demo
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
#ifndef bsp_h
#define bsp_h

#define BSP_TICKS_PER_SEC    33U
#define BSP_SCREEN_WIDTH     96U
#define BSP_SCREEN_HEIGHT    16U

void BSP_init(void);
void BSP_sleep(uint32_t ticks);
void BSP_drawBitmap(uint8_t const *bitmap);
void BSP_drawCount(uint32_t n);
void BSP_drawNString(uint8_t x,  /* x in pixels */
                     uint8_t y,  /* y position in chars */
                     char const *str);

void BSP_setPaused(int paused);
int  BSP_isPaused(void);

void BSP_terminate(int result);

/* special adaptations for QWIN GUI applications */
#ifdef QWIN_GUI
    /* replace main() with main_gui() as the entry point to a GUI app. */
    #define main() main_gui()
    int main_gui(); /* prototype of the GUI application entry point */
#endif

#endif /* bsp_h */
