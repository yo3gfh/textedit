#include <windows.h>

int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow )
{
    MessageBox ( NULL, TEXT("      This is just a useless program :-)\n"
                            "       to determine all other projects\n"
                            "in the workspace to build at the same time."), 
                       TEXT("A.P.O.L.O.G.Y"), MB_OK|MB_ICONINFORMATION );
    return 0;
}
