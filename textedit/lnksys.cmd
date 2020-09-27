@echo off

if exist "%1.sys" del "%1.sys"

if not exist rsrc.obj goto nores

\masm32\bin\Link /driver /base:0x10000 /align:32 /out:"%1.sys" /subsystem:native /ignore:4078 "%1.obj" rsrc.obj
dir "%1.*"
goto TheEnd

:nores
\masm32\bin\Link /driver /base:0x10000 /align:32 /out:"%1.sys" /subsystem:native /ignore:4078 "%1.obj"
dir "%1.*"

:TheEnd
