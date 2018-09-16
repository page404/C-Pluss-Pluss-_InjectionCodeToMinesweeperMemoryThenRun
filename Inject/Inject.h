#if !defined(INJECT_H__4171DE09_86EA_4445_A718_8B2764037094__INCLUDED_)
#define INJECT_H__4171DE09_86EA_4445_A718_8B2764037094__INCLUDED_

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//-------------------------------------------------------------------------

#define InjectTarClassName _T("É¨À×")

//-------------------------------------------------------------------------

typedef struct  
{
  //MessageBoxA
  TCHAR szMSGText[100];
  TCHAR szMSGCaption[16];
  LPVOID pfnMessageBox;

  //Sleep
  DWORD dwMilliseconds;
  LPVOID pfnSleep;

  //

} tagInject;

//-------------------------------------------------------------------------

BOOL Inject();
HANDLE GetProcessHandle(TCHAR* pszInjectTarClassName);
DWORD ThreadProc(tagInject* stInject);

#endif //!defined(INJECT_H__4171DE09_86EA_4445_A718_8B2764037094__INCLUDED_)
