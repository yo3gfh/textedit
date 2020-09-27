@echo off

if not exist \masm32\bin\dumppe.exe goto message
\masm32\bin\dumppe -disasm %1

goto TheEnd

:message
echo.
echo    To use this menu option, you must first unzip
echo    the file called DUMPPE.ZIP in the BIN directory.
echo.
echo    You can then dis-assemble the executable files
echo    that you have assembled.
echo.
pause

:TheEnd