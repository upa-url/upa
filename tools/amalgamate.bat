@echo off

REM the directory path of this file
set p=%~dp0

REM CD to repository root folder
%~d0
cd %p%..

REM Amalgamate
python tools/amalgamate/amalgamate.py -c tools/amalgamate/config-cpp.json -s . -p tools/amalgamate/config-cpp.prologue --no-duplicates
python tools/amalgamate/amalgamate.py -c tools/amalgamate/config-h.json -s .

REM Copy url_for_*.h files
copy /y include\upa\url_for_*.h single_include\upa
