:: Batch file for building the Python package, checking it,
:: and uploading it to PyPi.
::
:: usage:
:: make
::
@setlocal

:: set the project
@set PROJ=qutest

:: adjust the Python location for your system
@set PYTHON=C:\tools\Python39-32\python.exe

:: cleanup any previous builds...
@rmdir /S /Q build
@rmdir /S /Q dist
@rmdir /S /Q %PROJ%.egg-info

:: execute the build...
%PYTHON% setup.py sdist bdist_wheel

:: check the build...
twine check dist/*

:: upload to PyPi
twine upload dist/*

:: cleanup after the build...
@rmdir /S /Q build
@rmdir /S /Q dist
@rmdir /S /Q %PROJ%.egg-info

@endlocal
