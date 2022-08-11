/*****************************************************************************
* getopt.c - competent and free getopt library.
*
* Copyright (c) 2002-2003, Mark K. Kim
* Copyright (c) 2012-2017, Kim Grasman <kim.grasman@gmail.com>
*
* Modified by Quantum Leaps 2018 <https://www.state-machine.com>
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of Kim Grasman nor the
*     names of contributors may be used to endorse or promote products
*     derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL KIM GRASMAN BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/
#ifndef INCLUDED_GETOPT_PORT_H
#define INCLUDED_GETOPT_PORT_H

#if defined(__cplusplus)
extern "C" {
#endif

extern char const *optarg; /* argument of current option */
extern int optind;         /* index of first non-option in argv */
extern int optopt;         /* single option character, as parsed */
extern int opterr;         /* flag to enable built-in diagnostics */

int getopt(int argc, char *argv[], char const *optstr);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDED_GETOPT_PORT_H */
