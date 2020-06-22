// Fuck.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <thread>

#pragma comment(lib, "WS2_32.lib")

using namespace std;

void GetPipe();
int sock();

SOCKADDR_IN sSendAddr; // 接受地址和发送地址
SOCKET		s;
bool ret = false;
bool over = false;

// 主函数
int main()
{
	over = false;
	thread th(GetPipe);
	th.detach();

start:
	system("cls");
	int num = sock();
	cout << "正在初始化sock" << endl;
	if (num != 0)
	{
		Sleep(2000);
		goto start;
	}
	if (num == 0 && over == true)
		return 0;
}

WCHAR buf[4096] = { 0 };

// 管道创建
HANDLE hReadF = NULL;
HANDLE hWriteF = NULL;
void GetPipe()
{
	// 文件位置
	WCHAR ExeName[]	= LR"(bedrock_server.exe)";
	wcout << L"ExeName == " << ExeName << endl;

	// 安全性保证
	SECURITY_ATTRIBUTES saPipe;

	saPipe.nLength				= sizeof(SECURITY_ATTRIBUTES);	// 大小
	saPipe.lpSecurityDescriptor = NULL;							// 安全描述符
	saPipe.bInheritHandle		= TRUE;							// 是否可继承

	// 管道 尾部和头部
	HANDLE hReadPipe  = NULL;
	HANDLE hWritePipe = NULL;

	// 创建管道
	BOOL bSuccess = CreatePipe(&hReadPipe, &hWritePipe, &saPipe, 0);
	if (bSuccess == false)
	{
		wcout << L"CreatePipe Faild" << endl;
		system("pause");
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
		over = true;
		exit(-1);
	}

	SECURITY_ATTRIBUTES saPipeF;

	saPipeF.nLength = sizeof(SECURITY_ATTRIBUTES);	// 大小
	saPipeF.lpSecurityDescriptor = NULL;							// 安全描述符
	saPipeF.bInheritHandle = TRUE;							// 是否可继承

	bSuccess = CreatePipe(&hReadF, &hWriteF, &saPipeF, 0);
	if (bSuccess == false)
	{
		cout << "createpipF faild" << endl;
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
		CloseHandle(hWriteF);
		CloseHandle(hReadF);
		over = true;
		exit(-1);
	}

	wcout << L"CreatePipe Successflly" << endl;

	PROCESS_INFORMATION pi;					// 新进程线程信息
	STARTUPINFO			si;					// 新进程窗口特性

	memset(&si, 0, sizeof(si));				//********************

	HANDLE hTemp = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hTempF = GetStdHandle(STD_INPUT_HANDLE);

	// 重定向标准输出
	SetStdHandle(STD_OUTPUT_HANDLE, hWritePipe);
	SetStdHandle(STD_INPUT_HANDLE, hReadF);

	// 通过当前信息初始化si
	GetStartupInfo(&si);

	/*
	https://baike.baidu.com/item/CreateProcess/11050419?fr=aladdin // 百度百科
	CreateProcess
		lpApplicationName	 创建线程的位置
		lpCommandLine		 命令行命令
		lpProcessAttributes	 进程句柄是否可以被继承
		lpThreadAttributes	 线程句柄是否可以被继承 (通常为NULL)
		bInheritHandles		 创建的进程是否调用进程处继承句柄
		dwCreationFlags		 进程标志 进程优先级
		lpEnvironment		 新进程模块环境 NULL则继承调用环境
		lpCurrentDirectory	 新进程工作路径 NULl则继承调用环境
		lpStartupInfo		 决定新进程的主窗体如何显示
		lpProcessInformation 用来接收新进程的识别信息
	*/
	// 创建进程
	bSuccess = CreateProcess(NULL, ExeName, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	
	SetStdHandle(STD_OUTPUT_HANDLE, hTemp);
	SetStdHandle(STD_INPUT_HANDLE, hTempF);

	// 创建进程失败
	if (bSuccess == false)
	{
		wcout << L"CreateProcess Faild" << endl;
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(hReadPipe);
		CloseHandle(hWritePipe);
		CloseHandle(hWriteF);
		CloseHandle(hReadF);
		over = true;
		exit(-2);
	}

	DWORD dwRead	 = 0;
	DWORD dwAvail	 = 0;
	 
	wcout << L"CreateProcess Successflly" << endl;

	// 消除保存的主线程句柄(CloseHandle计数问题)
	CloseHandle(pi.hThread);
	bool num = false;
	do
	{
		// wchar_t = char * 2
		memset(buf, 0, 4096 * 2);

		// 判断管道是否有内容 非阻塞
		PeekNamedPipe(hReadPipe, NULL, NULL, &dwRead, &dwAvail, NULL);
		if (dwAvail > 0) // 如果字节大于0
		{
			// 阻塞
			if (ReadFile(hReadPipe, buf, 4096, &dwRead, NULL))
			{
				// 默认是单字节 直接输出buf不会识别char输出的是地址
				cout << (const  char*)buf << endl;
				if (ret == true)
				{
					::sendto(s, (const char*)buf, 4096*2,0, (SOCKADDR*)&sSendAddr, sizeof(sSendAddr));
				}
				// 如果输入stop
				if (strcmp((const char*)buf, "Quit correctly") == 0)
					break;
			}
		}
	} while (true);

	cout << "over" << endl;
	TerminateProcess(pi.hProcess, pi.dwProcessId);
	CloseHandle(pi.hProcess);
	CloseHandle(hReadPipe);
	CloseHandle(hWritePipe);
	CloseHandle(hWriteF);
	CloseHandle(hReadF);
	over = true;
	exit(0);
}
// 远程
int sock()
{
	WSADATA		wsd;
	USHORT uPort = 6234;
	CHAR szBuf[4096 * 2] = { 0 };
	int nButLen = 4096 * 2;
	int nResult = 0;
	int nSenderAddrSize = sizeof(sSendAddr);

	nResult = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (nResult != NO_ERROR)
	{
		cout << "初始化Socket失败" << endl;
		return -1;
	}

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == s)
	{
		cout << "初始化 socket 失败" << WSAGetLastError() << endl;
		return -2;
	}
	SOCKADDR_IN sRecvAddr;
	sRecvAddr.sin_family = AF_INET;
	sRecvAddr.sin_port = htons(uPort);
	sRecvAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	nResult = ::bind(s, (SOCKADDR*)&sRecvAddr, sizeof(sRecvAddr));
	if (-1 == nResult)
	{
		cout << "绑定 sRecvAddr 失败" << WSAGetLastError() << endl;
		return -3;
	}

	cout << "--------------------start--------------------" << endl;
	DWORD dw;
	ret = true;
	do
	{
		memset(szBuf, 0, 4096 * 2);
		nResult = recvfrom(s, szBuf, 4096 * 2, 0, (SOCKADDR*)&sSendAddr, &nSenderAddrSize);
		if (over == true)
		{
			break;
		}
		if (strlen(szBuf) > 0)
		{
			szBuf[strlen(szBuf)] = '\n';
			szBuf[strlen(szBuf) + 1] = '\0';
			szBuf[4096 * 2 -1] = '\0';
			WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szBuf, strlen(szBuf), &dw, NULL);
			WriteFile(hWriteF, szBuf, strlen(szBuf), &dw, NULL);
		}
	} while (true);

	nResult = closesocket(s);
	if (nResult == SOCKET_ERROR)
	{
		cout << "closesocket faild" << WSAGetLastError() << endl;
	}

	WSACleanup();
	return 0;
}