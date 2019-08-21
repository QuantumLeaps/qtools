cd ..
qclean

:: prepare to compress... ----------------------------------------------------
cd setup

:: cleanup
rmdir /S /Q qtools

mkdir qtools
mkdir qtools\source

xcopy /K ..\qspy\posix\rel\qspy      qtools\bin\
xcopy /K ..\qclean\posix\rel\qclean  qtools\bin\
xcopy /K ..\qfsgen\posix\rel\qfsgen  qtools\bin\
::xcopy /K ..\qudp\posix\rel\qudp    qtools\bin\
::xcopy /K ..\qudp\posix\rel\qudps   qtools\bin\

xcopy /K ..\doc\qclean.txt qtools\doc\
xcopy /K ..\doc\qfsgen.txt qtools\doc\
xcopy /K ..\doc\mscgen.txt qtools\doc\

xcopy /K /E ..\lint        qtools\lint\
xcopy /K /E ..\qspy        qtools\qspy\
xcopy /K /E C:\tools\Unity qtools\unity\

xcopy /K /E ..\licenses    qtools\licenses\

copy ..\version*           qtools\
copy ..\README.md          qtools\

c:\tools\7-Zip\7z a -tzip qtools\source\qclean.zip ..\qclean\
c:\tools\7-Zip\7z a -tzip qtools\source\mscgen.zip ..\mscgen\
c:\tools\7-Zip\7z a -tzip qtools\source\qfsgen.zip ..\qfsgen\
c:\tools\7-Zip\7z a -tzip qtools\source\qudp.zip   ..\qudp\

:: compress the final archive ------------------------------------------------
del qtools-posix.zip
c:\tools\7-Zip\7z a -tzip qtools-posix.zip qtools\




