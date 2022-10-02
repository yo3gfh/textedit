@echo off

echo     [1] BUILDING WINDOWS DRIVER (SYS)

if not exist rsrc.rc goto over1

echo     [1] compiling resources...
\masm32\bin\rc /v rsrc.rc
\masm32\bin\cvtres /machine:ix86 rsrc.res
echo     [1] done...
:over1

if exist "%1.obj" del "%1.obj"
if exist "%1.sys" del "%1.sys"

echo     [2] assembling %1.asm...
\masm32\bin\ml /c /coff "%1.asm"
if errorlevel 1 goto errasm
echo     [2] done...

if not exist rsrc.obj goto nores

echo     [3] linking %1.sys...
\masm32\bin\Link /driver /base:0x10000 /align:32 /out:"%1.sys" /subsystem:native /ignore:4078 "%1.obj" rsrc.obj
if errorlevel 1 goto errlink
echo     [3] done...

dir "%1.*"
goto TheEnd

:nores
echo     [3] linking %1.sys...
\masm32\bin\Link /driver /base:0x10000 /align:32 /out:"%1.sys" /subsystem:native /ignore:4078 "%1.obj"
if errorlevel 1 goto errlink
echo     [3] done...

dir "%1.*"
goto TheEnd

:errlink
echo _
echo Link error
goto TheEnd

:errasm
echo _
echo Assembly Error
goto TheEnd

:TheEnd
