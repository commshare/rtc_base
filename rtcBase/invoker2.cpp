#include "base/asyncinvoker.h"  
#include <iostream>  
#include <memory>  

class MyClass2
{

public:
	MyClass2();
	void OneTask(rtc::Thread *thread, int x);

	void AnotherAsyncTask(int x);

	void myFunction(int x);

private:
	std::unique_ptr<rtc::AsyncInvoker> invoker;
	rtc::Thread *mainThread;
};

MyClass2::MyClass2()
{
	invoker.reset(new rtc::AsyncInvoker());
	std::cout << "Main Thread ID:" << rtc::Thread::Current()->GetId() << std::endl;
	//当前的线程就认为是主线程，当前是在main方法创建的线程
	mainThread = rtc::Thread::Current();
}

void MyClass2::OneTask(rtc::Thread *thread, int x)
{
	/*
	函数AnotherAsyncTask中的invoker->AsyncInvoke<void>(RTC_FROM_HERE, mainThread, rtc::Bind(&MyClass::myFunction, (MyClass*)this, x));
	是想在mainThread中执行myFunction，但是myFunction并未执行，打印结果依然和上图相同。
	*/
	invoker->AsyncInvoke<void>(RTC_FROM_HERE, thread, rtc::Bind(&MyClass2::AnotherAsyncTask, (MyClass2*)this, x));
}
void MyClass2::AnotherAsyncTask(int x)
{
	std::cout << "Worker Thread ID:" << rtc::Thread::Current()->GetId() << std::endl;
	std::cout << "Input Value Is:" << x << std::endl;

	//让mainthread去执行myFunction
	invoker->AsyncInvoke<void>(RTC_FROM_HERE, mainThread, rtc::Bind(&MyClass2::myFunction, (MyClass2*)this, x));
}

void MyClass2::myFunction(int x)
{
	std::cout << "myFunction Thread ID:" << rtc::Thread::Current()->GetId() << std::endl;
	std::cout << "myFunction Value Is:" << x << std::endl;
}

int main3()
{
	std::unique_ptr<rtc::Thread> myThread = rtc::Thread::Create();
	myThread->Start();
	MyClass2 *myClass = new MyClass2;
	myClass->OneTask(myThread.get(), 10);

	/*
	函数AnotherAsyncTask中的invoker->AsyncInvoke<void>(RTC_FROM_HERE, mainThread, rtc::Bind(&MyClass::myFunction, (MyClass*)this, x));是想在mainThread中执行myFunction，但是myFunction并未执行，打印结果依然和上图相同。
这里需要将main.cpp稍作修改，添加代码实现线程的消息循环，新的main.cpp如下所示。

Main Thread ID:30188
MAIN() CUR Thread ID:30188
Worker Thread ID:14532
Input Value Is:10
myFunction Thread ID:30188
myFunction Value Is:10

	*/
	std::cout << "MAIN() CUR Thread ID:" << rtc::Thread::Current()->GetId() << std::endl;
	while (true)
	{		
		rtc::Thread::Current()->ProcessMessages(0);
		rtc::Thread::Current()->SleepMs(1);
	}

	return 0;
}