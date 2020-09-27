@echo off

if not exist rsrc.rc goto over1
\masm32\bin\rc /v rsrc.rc
\masm32\bin\cvtres /machine:ix86 rsrc.res
:over1

if exist "%1.obj" del "%1.obj"
if exist "%1.sys" del "%1.sys"

\masm32\bin\ml /c /coff "%1.asm"
if errorlevel 1 goto errasm

if not exist rsrc.obj goto nores

\masm32\bin\Link /driver /base:0x10000 /align:32 /out:"%1.sys" /subsystem:native /ignore:4078 "%1.obj" rsrc.obj
if errorlevel 1 goto errlink

dir "%1.*"
goto TheEnd

:nores
\masm32\bin\Link /driver /base:0x10000 /align:32 /out:"%1.sys" /subsystem:native /ignore:4078 "%1.obj"
if errorlevel 1 goto errlink
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
