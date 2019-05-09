//https://blog.csdn.net/tanningzhong/article/details/71629699
#include "base/asyncinvoker.h"  
#include <iostream>  
#include <memory>  

class MyClass
{

public:
	MyClass();
	void OneTask(rtc::Thread *thread, int x);

	void AnotherAsyncTask(int x);

private:
	std::unique_ptr<rtc::AsyncInvoker> invoker;
};
MyClass::MyClass()
{
	//主线程
	invoker.reset(new rtc::AsyncInvoker());
	std::cout << "Main Thread ID:" << rtc::Thread::Current()->GetId() << std::endl;
}

void MyClass::OneTask(rtc::Thread *thread, int x)
{
	//函数AnotherAsyncTask是在子线程中执行的。
	invoker->AsyncInvoke<void>(RTC_FROM_HERE, thread, rtc::Bind(&MyClass::AnotherAsyncTask, (MyClass*)this, x));
}
void MyClass::AnotherAsyncTask(int x)
{
	std::cout << "Worker Thread ID:" << rtc::Thread::Current()->GetId() << std::endl;
	std::cout << "Input Value Is:" << x << std::endl;
}
/*
Main Thread ID:26320
Worker Thread ID:27636
Input Value Is:10

*/
int main2()
{
	//创建一个县城并启动，这个是子线程
	std::unique_ptr<rtc::Thread> myThread = rtc::Thread::Create();
	myThread->Start();
	MyClass *myClass = new MyClass;
	//让子线程执行
	myClass->OneTask(myThread.get(), 10);

	Sleep(10000);

	return 0;
}