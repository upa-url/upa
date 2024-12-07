@echo off

REM the directory path of this file
set p=%~dp0

REM Unicode version
set UVER=15.1.0

mkdir %p%\data

for %%f in (DerivedCoreProperties.txt) do (
  curl -fsS -o %p%\data\%%f https://www.unicode.org/Public/%UVER%/ucd/%%f
)
