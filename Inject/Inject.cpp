#include "Inject.h"

//Ҫע�뵽Ŀ������еĴ���
DWORD InjectCode(tagInject* stInject)
{
  //�����µĺ���ָ������
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

//Զ���߳���DLLע��  �ú���Ϊ Main �������� 
BOOL Inject()
{
  BOOL bRet = FALSE;

  HANDLE hProcess = NULL;
  HMODULE hUser32 = NULL;
  HMODULE hKernel32 = NULL;
  //User32��̬���ӿ��е�MessageBoxA������ַ
  LPVOID lpMessageBox = NULL;
  //Kernel32��̬���ӿ��е�Sleep������ַ
  LPVOID lpSleep = NULL;
  
  LPVOID lpFuncAddr = NULL;
  LPVOID lpParamAddr = NULL;

  BOOL bWrite = FALSE;
  SIZE_T dwNumberOfBytesWritten = 0;

  HANDLE hRemoteThread = NULL;
  
  //Ҫע�뵽ɨ���ڴ��еĴ���ĳ���
  UINT nInjectCodeSize = 0;

  __try
  {
    //ȡĿ����̾��
	HANDLE hProcess = NULL;
    HWND hWnd = NULL;
    DWORD dwPID = 0;
  
    //��������
    hWnd = FindWindow(_T("ɨ��"), NULL);
    if (NULL == hWnd)
    {
      printf(_T("FindWindow Err!\r\n"));
      __leave;
    }
    
    //ȡ����ID
    GetWindowThreadProcessId(hWnd, &dwPID);
    if (0 == dwPID)
    {
      printf(_T("GetWindowThreadProcessId Err!\r\n"));
      __leave;
    }
    
    //�򿪽���
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
    if (NULL == hProcess)
    {
      printf(_T("GetProcessHandle Err!\r\n"));
      __leave;
    }
    
    //---------------------------------------------------------------------

    //����Kernel32��̬���ӿ�
    hKernel32 = LoadLibrary(_T("Kernel32"));
    if (NULL == hKernel32)
    {
      printf(_T("LoadLibrary Kernel32 Err!\r\n"));
      __leave;
    }
    //����Kernel32��̬���ӿ��е�Sleep������ַ
    lpSleep = (LPVOID)GetProcAddress(hKernel32, _T("Sleep"));
    if (NULL == lpSleep)
    {
      printf(_T("GetProcAddress Sleep Err!\r\n"));
      __leave;
    }
    FreeLibrary(hKernel32);

    //����User32��̬���ӿ�
    hUser32 = LoadLibrary(_T("User32"));
    if (NULL == hUser32)
    {
      printf(_T("LoadLibrary User32 Err!\r\n"));
      __leave;
    }
    //����User32��̬���ӿ��е�MessageBoxA������ַ
    lpMessageBox = (LPVOID)GetProcAddress(hUser32, _T("MessageBoxA"));
    if (NULL == lpMessageBox)
    {
      printf(_T("GetProcAddress MessageBoxA Err!\r\n"));
      __leave;
    }
    FreeLibrary(hUser32);
    
    //---------------------------------------------------------------------
    
	//tagInject Ϊ�� Inject.h �ж���Ľṹ��
    tagInject stInject;
	//�ṹ������
    RtlZeroMemory(&stInject, sizeof(tagInject));
    
    //�ṹ���Ա��ֵ
    //MessageBoxA
    stInject.pfnMessageBox = lpMessageBox;
    strcpy(stInject.szMSGText, _T("C++ Inject Success"));
    strcpy(stInject.szMSGCaption, _T("Page404"));
    //Sleep
    stInject.pfnSleep = lpSleep;
    stInject.dwMilliseconds = 1800;

    //---------------------------------------------------------------------
    
	//--�����ִ�д���ĳ���
	UINT nInjectCodeLength = (int)InjectCode;
	UINT nInjectLength = (int)Inject;
	//�õ� InjectCode Ҫע�����εĳ���
	nInjectCodeSize = abs((long)nInjectLength - (long)nInjectCodeLength);  
	
    //���������ڴ�ռ䣬Ϊ��д��Զ���߳�ִ�к���
    lpFuncAddr = VirtualAllocEx(hProcess,NULL,nInjectCodeSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE);

    if (NULL == lpFuncAddr)
    {
      printf(_T("VirtualAllocEx Func Err!\r\n"));
      __leave;
    }
    //д��Զ���߳�ִ�к�����Ŀ�����
    bWrite = WriteProcessMemory(hProcess,lpFuncAddr,&InjectCode,nInjectCodeSize,&dwNumberOfBytesWritten);
    if (!bWrite)
    {
      printf(_T("WriteProcessMemory InjectCode Err!\r\n"));
      __leave;
    }
    bWrite = FALSE;

    //���������ڴ�ռ䣬Ϊ��д��ִ�в���
    lpParamAddr = VirtualAllocEx(hProcess,NULL,sizeof(tagInject),MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    
    if (NULL == lpParamAddr)
    {
      printf(_T("VirtualAllocEx Param Err!\r\n"));
      __leave;
    }
    //д��Զ���߳�ִ�в�����Ŀ�����
    bWrite = WriteProcessMemory(hProcess,lpParamAddr,&stInject,sizeof(tagInject),&dwNumberOfBytesWritten);
    if (!bWrite)
    {
      printf(_T("WriteProcessMemory tagInject Err!\r\n"));
      __leave;
    }
    bWrite = FALSE;

    //---------------------------------------------------------------------

    //����Զ���̣߳�ִ����д��ĺ���
    hRemoteThread = CreateRemoteThread(hProcess,NULL,0,(LPTHREAD_START_ROUTINE )lpFuncAddr,lpParamAddr,0,NULL);
	
    if (NULL == hRemoteThread)
    {
      printf(_T("CreateRemoteThread Err!\r\n"));
      __leave;
    }

    //---------------------------------------------------------------------
    
    //Injectִ�гɹ�
    bRet = TRUE;
    printf(_T("Inject OK!\r\n"));
  }
  __finally
  {
	  return bRet;
  }
}


