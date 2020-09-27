@echo off

if exist %1.dll del %1.dll

\masm32\bin\Link /SUBSYSTEM:WINDOWS /DLL /DEF:%1.def %1.obj

dir %1.*
