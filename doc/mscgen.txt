Building Mscgen
===============

Building is done with the GNU autotools and should be largely automatic.

Preparation
-----------

If you obtained the sources directly from the SVN repository, you will first
need to run autogen.sh to generate the configure script.

Note: This is not required for the downloadable source tarballs as they are
       already contain a configure script.


Win32 - Native
--------------

A native build has no runtime dependency on Cygwin and can be ran on any
Windows machine.  It does however use Cygwin to configure and build.

You can get Cygwin from http://www.cygwin.com.  The following packages
need to be installed via the Cygwin setup.exe:

 - gcc
 - gcc-mingw
 - make
 - flex
 - bison
 - binutils

A copy of the Win32 libgd library is also needed.  If you checked out the
mscgen sources from Google Code, you will already have this in the gdwin32
directory.  Otherwise you can obtain a copy from the GD site, or from Google
Code:

  http://www.libgd.org/releases/oldreleases/gd-2.0.34-win32.zip

  svn checkout http://mscgen.googlecode.com/svn/trunk/gdwin32 /tmp/gdwin32

You will have to unpack the zip file, and ensure the file bgd.dll is on your
path.

From the top level directory, run the configure script with options to
build natively and use the Win32 binary gdwin32 you downloaded (an unpacked):

  $ ./configure CFLAGS=-mno-cygwin \
                GDLIB_CFLAGS="-I/tmp/gdwin32/include" \
                GDLIB_LIBS="-L/tmp/gdwin32/lib -lbgd"
  $ make
  $ make check
  $ make install

This will install a native version of mscgen and the accompanying bgd.dll file
into the cygwin directories.  If you wish to install elsewhere, either use the
--prefix option to 'configure', or manually copy mscgen.exe and the DLL.

The DLL dependencies of the mscgen.exe can be checked using cygcheck:

$ cygcheck.exe mscgen.exe
Found: C:\cygwin\bin\mscgen.exe
C:\cygwin\bin\mscgen.exe
  C:\cygwin\bin\bgd.dll
    C:\WINDOWS\system32\KERNEL32.dll
      C:\WINDOWS\system32\ntdll.dll
    C:\WINDOWS\system32\MSVCRT.dll


Win32 - Cygwin
--------------

You will need Cygwin (from http://www.cygwin.com) and the following packages
installed via the Cygwin setup program:

 - gcc
 - make
 - flex
 - bison
 - binutils
 - libgd2
 - libgd-devel
 - pkg-config

From the top level directory, run the configure script and then make:

  $ ./configure
  $ make
  $ make check
  $ make install


Other Unix-like Platforms
-------------------------

You will need:

 - flex, bison, gcc and pkg-config installed
 - GD >= 2.0.22 libs installed (libgd-devel)
 - GD's dependencies, which are dependent on how it was configured, but
    will typically be a subset of the following:
      libpng libz libfreetype libm
 - GNU make

GD can be obtained from http://www.libgd.org/ and will need to be downloaded
and installed, or a package, such as an RPM maybe used instead.
For RedHat and SuSE, the package gd-devel should be installed, which will
normally cause any dependent libraries to also be installed.

From the top level directory, run the configure script and then make:

  $ ./configure
  $ make
  $ make check
  $ sudo make install


Syntax Highlighting
===================

A lang file for GtkSourceView based editors is included with mscgen.
The mscgen.lang file, normally installed with mscgen to /usr/share/doc/mscgen
but needs copying to one of two locations for GtkSourceView:

  /usr/share/gtksourceview-2.0/language-specs 
    - System wide installation

  ~/.local/share/gtksourceview-2.0/language-specs  
    - User specific installation

This then allows mscgen input files to be displayed with syntax highlighting
in applications such a gedit and Anjuta.  The language spec associates itself
with .msc files.


Bugs
====

An issue tracker is available at the following location:

  http://code.google.com/p/mscgen/issues/list

You may also email me directly at Michael.McTernan.2001@cs.bris.ac.uk.


Updates
=======

A release announcement mailing list for new releases is available at the
following location:

  http://groups.google.com/group/mscgen-announce

LICENCE
=======

Mscgen, Copyright (C) 2010 Michael C McTernan,
                      Michael.McTernan.2001@cs.bris.ac.uk
Mscgen comes with ABSOLUTELY NO WARRANTY.  Mscgen is free software, and you
are welcome to redistribute it under certain conditions; see the COPYING
file for details.

PNG rendering support is provided by libgd, www.libgd.org; see the COPYING.gd
file for full credits.

END OF FILE
