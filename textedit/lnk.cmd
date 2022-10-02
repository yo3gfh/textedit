@echo off

if exist %1.exe del %1.exe

if not exist rsrc.obj goto nores

\masm32\bin\Link /SUBSYSTEM:WINDOWS /OPT:NOREF %1.obj rsrc.obj
goto TheEnd

:nores
\masm32\bin\Link /SUBSYSTEM:WINDOWS /OPT:NOREF %1.obj

:TheEnd
if errorlevel 0 dir %1.* 

