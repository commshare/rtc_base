// rtcBase.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//https://blog.csdn.net/u010643777/article/details/88430144

/*
ws2_32.lib
如果没有链接，会报如下错：

error LNK2019: 无法解析的外部符号 __imp__closesocket@4，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__recv@16，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__accept@12，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__listen@8，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__ntohs@4，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__getsockname@12，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__bind@12，该符号在函数 _wmain 中被引用
error LNK2019: 无法解析的外部符号 __imp__socket@12，该符号在函数 _wmain 中被引用



error LNK2019: 无法解析的外部符号 _in6addr_any

winmm.lib
error LNK2019: 无法解析的外部符号 __imp__timeGetTime@0

Rpcrt4.lib
error LNK2019: 无法解析的外部符号 __imp__UuidEqual@12

Ntdsapi.lib
error LNK2019: 无法解析的外部符号 __imp__DsMakeSpnA@28

dmoguids.lib
error LNK2001: 无法解析的外部符号 _IID_IMediaBuffer
wmcodecdspuuid.lib

error LNK2001: 无法解析的外部符号 _CLSID_CWMAudioAEC
Strmiids.lib

无法解析的外部符号 _MEDIATYPE_Audio


*/
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include "pch.h"
#include <iostream>
//invoke_test.cc
#include "base/thread.h"
#include "base/bind.h"
#include "base/location.h"
#include <stdio.h>
#include "base/task_queue.h"
class Callback {
public:
	Callback() {}
	~Callback() {}
	int Print() {
		rtc::Thread *thread = rtc::Thread::Current();
		printf("callback %p\n", thread);
		return 1000;
	}
};
void Work()
{
	printf("thisis work!!!!\n");
}
//  // Ex: bool result = thread.Invoke<bool>(RTC_FROM_HERE,
  // &MyFunctionReturningBool);
int main() {

	rtc::TaskQueue tq("zbtq");

	tq.PostTask([]() { Work(); });


	rtc::Thread worker;
	rtc::Thread *thread = rtc::Thread::Current();
	printf("main %p\n", thread);
	worker.Start();
	Callback test;
	//在其它线程保证某方法被此线程调用，相当于消息的异步通知，即 POST
	//invoke是怎么阻塞在哪里，直到回调函数被执行的。
	int ret = worker.Invoke<int>(RTC_FROM_HERE, rtc::Bind(&Callback::Print, &test));

	printf("worker %p and ret %d \n", &worker,ret);

	worker.Stop();


	return 0;
}

//int main()
//{
//    std::cout << "Hello World!\n"; 
//}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
