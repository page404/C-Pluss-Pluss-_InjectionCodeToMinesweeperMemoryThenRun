#include "Inject.h"

//要注入到目标程序中的代码
DWORD InjectCode(tagInject* stInject)
{
  //定义新的函数指针类型
  typedef int (__stdcall *PFN_MSG)(HWND, LPCTSTR, LPCTSTR, UINT);
  typedef VOID (__stdcall *PFN_SLEEP)(DWORD);
  
  PFN_MSG pfnMSG = NULL;
  PFN_SLEEP pfnSleep = NULL;
  int nRet = 0;

  pfnMSG = (PFN_MSG)stInject->pfnMessageBox;
  pfnSleep = (PFN_SLEEP)stInject->pfnSleep;

  do 
  {
    //MessageBoxA
    nRet = pfnMSG(NULL,stInject->szMSGText,stInject->szMSGCaption,MB_OKCANCEL);
    if (IDCANCEL == nRet)
    {
      break;
    }
    
    //Sleep
    pfnSleep(stInject->dwMilliseconds);

  } while (TRUE);

  
  return 0x7F;
}

//远程线程无DLL注入  该函数为 Main 函数调用 
BOOL Inject()
{
  BOOL bRet = FALSE;

  HANDLE hProcess = NULL;
  HMODULE hUser32 = NULL;
  HMODULE hKernel32 = NULL;
  //User32动态链接库中的MessageBoxA函数地址
  LPVOID lpMessageBox = NULL;
  //Kernel32动态链接库中的Sleep函数地址
  LPVOID lpSleep = NULL;
  
  LPVOID lpFuncAddr = NULL;
  LPVOID lpParamAddr = NULL;

  BOOL bWrite = FALSE;
  SIZE_T dwNumberOfBytesWritten = 0;

  HANDLE hRemoteThread = NULL;
  
  //要注入到扫雷内存中的代码的长度
  UINT nInjectCodeSize = 0;

  __try
  {
    //取目标进程句柄
	HANDLE hProcess = NULL;
    HWND hWnd = NULL;
    DWORD dwPID = 0;
  
    //检索窗口
    hWnd = FindWindow(_T("扫雷"), NULL);
    if (NULL == hWnd)
    {
      printf(_T("FindWindow Err!\r\n"));
      __leave;
    }
    
    //取进程ID
    GetWindowThreadProcessId(hWnd, &dwPID);
    if (0 == dwPID)
    {
      printf(_T("GetWindowThreadProcessId Err!\r\n"));
      __leave;
    }
    
    //打开进程
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
    if (NULL == hProcess)
    {
      printf(_T("GetProcessHandle Err!\r\n"));
      __leave;
    }
    
    //---------------------------------------------------------------------

    //载入Kernel32动态链接库
    hKernel32 = LoadLibrary(_T("Kernel32"));
    if (NULL == hKernel32)
    {
      printf(_T("LoadLibrary Kernel32 Err!\r\n"));
      __leave;
    }
    //检索Kernel32动态链接库中的Sleep函数地址
    lpSleep = (LPVOID)GetProcAddress(hKernel32, _T("Sleep"));
    if (NULL == lpSleep)
    {
      printf(_T("GetProcAddress Sleep Err!\r\n"));
      __leave;
    }
    FreeLibrary(hKernel32);

    //载入User32动态链接库
    hUser32 = LoadLibrary(_T("User32"));
    if (NULL == hUser32)
    {
      printf(_T("LoadLibrary User32 Err!\r\n"));
      __leave;
    }
    //检索User32动态链接库中的MessageBoxA函数地址
    lpMessageBox = (LPVOID)GetProcAddress(hUser32, _T("MessageBoxA"));
    if (NULL == lpMessageBox)
    {
      printf(_T("GetProcAddress MessageBoxA Err!\r\n"));
      __leave;
    }
    FreeLibrary(hUser32);
    
    //---------------------------------------------------------------------
    
	//tagInject 为在 Inject.h 中定义的结构体
    tagInject stInject;
	//结构体清零
    RtlZeroMemory(&stInject, sizeof(tagInject));
    
    //结构体成员赋值
    //MessageBoxA
    stInject.pfnMessageBox = lpMessageBox;
    strcpy(stInject.szMSGText, _T("C++ Inject Success"));
    strcpy(stInject.szMSGCaption, _T("Page404"));
    //Sleep
    stInject.pfnSleep = lpSleep;
    stInject.dwMilliseconds = 1800;

    //---------------------------------------------------------------------
    
	//--计算可执行代码的长度
	UINT nInjectCodeLength = (int)InjectCode;
	UINT nInjectLength = (int)Inject;
	//得到 InjectCode 要注入代码段的长度
	nInjectCodeSize = abs((long)nInjectLength - (long)nInjectCodeLength);  
	
    //申请虚拟内存空间，为了写入远程线程执行函数
    lpFuncAddr = VirtualAllocEx(hProcess,NULL,nInjectCodeSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);

    if (NULL == lpFuncAddr)
    {
      printf(_T("VirtualAllocEx Func Err!\r\n"));
      __leave;
    }
    //写入远程线程执行函数到目标进程
    bWrite = WriteProcessMemory(hProcess,lpFuncAddr,&InjectCode,nInjectCodeSize,&dwNumberOfBytesWritten);
    if (!bWrite)
    {
      printf(_T("WriteProcessMemory InjectCode Err!\r\n"));
      __leave;
    }
    bWrite = FALSE;

    //申请虚拟内存空间，为了写入执行参数
    lpParamAddr = VirtualAllocEx(hProcess,NULL,sizeof(tagInject),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    
    if (NULL == lpParamAddr)
    {
      printf(_T("VirtualAllocEx Param Err!\r\n"));
      __leave;
    }
    //写入远程线程执行参数到目标进程
    bWrite = WriteProcessMemory(hProcess,lpParamAddr,&stInject,sizeof(tagInject),&dwNumberOfBytesWritten);
    if (!bWrite)
    {
      printf(_T("WriteProcessMemory tagInject Err!\r\n"));
      __leave;
    }
    bWrite = FALSE;

    //---------------------------------------------------------------------

    //创建远程线程，执行已写入的函数
    hRemoteThread = CreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE )lpFuncAddr,lpParamAddr,0,NULL);
	
    if (NULL == hRemoteThread)
    {
      printf(_T("CreateRemoteThread Err!\r\n"));
      __leave;
    }

    //---------------------------------------------------------------------
    
    //Inject执行成功
    bRet = TRUE;
    printf(_T("Inject OK!\r\n"));
  }
  __finally
  {
	  return bRet;
  }
}


