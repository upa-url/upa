@echo off
REM ðito bat failo kelias:
set p=%~dp0

del %p%\*.sdf
del %p%\*.VC.db
rmdir /S /Q %p%\ipch
del %p%\Debug\*.log
del %p%\Release\*.log
for /D %%s in (%p%\Debug\*.tlog) do rmdir /S /Q %%s
for /D %%s in (%p%\Release\*.tlog) do rmdir /S /Q %%s
