![](https://www.state-machine.com/img/qcalc.png)

"qcalc" is a powerful, cross-platform calculator specifically designed for
embedded systems programmers. The calculator accepts whole expressions in
the **C-syntax** and displays results simultaneously in decimal, hexadecimal,
and binary without the need to explicitly convert the result to these bases.


General Requirements
====================
The "qcalc" package requires Python 3,  which is included in the
[QTools distribution](https://www.state-machine.com/qtools)
for Windows and is typically included with other operating systems, such as
Linux and MacOS.


Installation
============
The `qcalc.py` script can be used standalone, **without** any
installation (see Using "qcalc" below).

Alternatively, you can **install** `qcalc.py` with `pip` from PyPi by
executing the following command:


`pip install qcalc`


Or directly from the sources directory (e.g., `/qp/qtools/qcalc`):


`python setup.py install --install-dir=/qp/qtools/qcalc`


Using "qcalc"
==============
If you are using `qcalc` as a standalone Python script, you invoke
it from a console as follows:

`python /path-to-qcalc-script/qcalc.py [expression]`

Alternatively, if you've installed `qcalc` with `pip`, you invoke
it from a console as follows:

`qcalc [expression]`


Batch mode
----------
If you provide the optional [expression] argument, qcalc will evaluate
the expression, print the result and terminate.


Interactive mode
----------------
Otherwise, if no [expression] argument is provided, qcalc will start in
the interactive mode, where you can enter expressions via your keyboard.


Features
========
The most important feature of "qcalc" is that it accepts expressions
in the **C-syntax** -- with the same operands and precedence rules as
in the C or C++ source code. Among others, the expressions can contain
all bit-wise operators (`<<`, `>>`, `|`, `&`, `^`, `~`) as well as
mixed decimal, **hexadecimal** and **binary** constants.
"qcalc" is also a powerful floating-point scientific calculator and
supports all mathematical functions (`sin()`, `cos()`, `tan()`,
`exp()`, `ln()`, ...). Some examples of acceptable expressions are:


- `((0xBEEF << 16) | 1280) & ~0xFF` -- binary operators, mixed hex and decimal numbers
- `($1011 << 24) | (1280 >> 8) ^ 0xFFF0` -- mixed @ref qcalc_bin "binary", dec and hex numbers
- `(1234 % 55) + 4321//33` -- remainder, integer division (note the `//` integer division operator
- `pi/6` -- pi-constant
- `pow(sin(ans),2) + pow(cos(ans),2)` -- scientific floating-point calculations, **ans-variable**


> **NOTE** "qcalc" internally uses the Python command `eval` to evaluate the expressions.
Please refer to the documentation of the
[Python math expressions](https://en.wikibooks.org/wiki/Python_Programming/Basic_Math)
for more details of supported syntax and features.


Automatic Conversion to Hexadecimal and Binary
----------------------------------------------
If the result of expression evaluation is integer (as opposed to floating point),
"qcalc" automatically displays the result in hexadecimal and binary formats
(see "qcalc" screenshot above). For better readability the hex display shows
an apostrophe between the two 16-bit half-words (e.g., `0xDEAD'BEEF`).
Similarly, the binary output shows an apostrophe between the four 8-bit
bytes (e.g., `0b11011110'10101101'10111110'11101111`).


Hexadecimal and Binary Numbers
------------------------------
As the extension to the C-syntax, QCalc supports both **hexadecimal numbers**
and **binary numbers**. These numbers are represented as `0x...` and`0b...`,
respectively, and can be mixed into expressions. Here are a few examples
of such expressions:

<pre>
(0b0110011 << 14) & 0xDEADBEEF
(0b0010 | 0b10000) * 123
</pre>


History of Inputs
-----------------
As a console application "qcalc" "remembers" the history of the recently
entered expressions. You can recall and navigate the history of previously
entered expressions by pressing the "Up" / "Down" keys.


The ans Variable
----------------
"qcalc" stores the result of the last computation in the `ans` variable.
Here are some examples of expressions with the `ans` variable:

- `1/ans` -- find the inverse of the last computation@n
- `log(ans)/log(2)` -- find log-base-2 of the last computation@n


64-bit Range
------------
"qcalc" supports the 64-bit range and switches to 64-bit arithmetic automatically
when an **integer** result of a computation exceeds the 32-bit range.
Here are some examples of the 64-bit output:

<pre>
> 0xDEADBEEF << 27
= 501427843159293952 | 0x06F5'6DF7'7800'0000
= 0b00000110'11110101'01101101'11110111'01111000'00000000'00000000'00000000
> 0xDEADBEEF << 24
= 62678480394911744 | 0x00DE'ADBE'EF00'0000
= 0b00000000'11011110'10101101'10111110'11101111'00000000'00000000'00000000
> 0xDEADBEEF << 34
! out of range
>
</pre>


Error Handling
---------------
Expressions that you enter into "qcalc" might have all kinds of errors:
syntax errors,  computation errors (e.g., division by zero), etc.
In all these cases, "qcalc" responds with the `Error` message and the
explanation of the error:

<pre>
> (2*4) + )
Traceback (most recent call last):
  File "C:\qp\qtools\qcalc\qcalc.py", line 54, in _main
    result = eval(expr)
  File "<string>", line 1
    (2*4) + )
            ^
SyntaxError: unmatched ')'
>
</pre>


More Information
================
More information about "qcalc" is available online at:

- https://www.state-machine.com/qtools/qcalc.html

More information about the QTools collection is available
online at:

- https://www.state-machine.com/qtools/


