#include <iostream>
#include <string>
#include "thread_pool.h"

using namespace std;

void func0(){
    cout << "func0()" << endl;
}

void func1(int a){
    cout << "func1() a = " << a << endl;
}

void func2(int a, string s){
    cout << "func2() a = " << a << ", s = " << s << endl;
}

void test1(){
    ThreadPool tpool;
    tpool.init(1);
    tpool.start();
    tpool.exec(1000, func0);
    tpool.exec(func1, 10);
    tpool.exec(func2, 20, "king");
    tpool.waitForAllDone();
    tpool.stop();
}

int func1_future(int a){
    cout << "func1() a = " << a << endl;
    return a;
}

string func2_future(int a, string s){
    cout << "func2() a = " << a << ", s = " << s << endl;
    return s;
}

void test2(){
    ThreadPool tpool;
    tpool.init(1);
    tpool.start();

    std::future<decltype(func1_future(0))> ret1 = tpool.exec(func1_future, 10);

    std::future<decltype(func2_future(0,""))> ret2 = tpool.exec(func2_future, 20, "james");

    std::cout << "ret1 : " << ret1.get() << std::endl;
    std::cout << "ret2 : " << ret2.get() << std::endl;

    tpool.waitForAllDone();
    tpool.stop();
}

class Test
{
public:
    int test(int i){
        cout << _name << ", i = " << i << endl;
        return i;
    }
    void setName(string name){
        _name = name;
    }
    string _name;
};

void test3() // 测试类对象函数的绑定
{
    ThreadPool threadpool;
    threadpool.init(2);
    threadpool.start(); // 启动线程池
    Test t1;
    Test t2;
    t1.setName("Test1");
    t2.setName("Test2");
    auto f1 = threadpool.exec(std::bind(&Test::test, &t1, std::placeholders::_1), 10);
    auto f2 = threadpool.exec(std::bind(&Test::test, &t2, std::placeholders::_1), 20);
    threadpool.waitForAllDone();
    cout << "t1 " << f1.get() << endl;
    cout << "t2 " << f2.get() << endl;
	threadpool.stop();
}

int main()
{
    test1(); // 简单测试线程池
    test2(); // 测试任务函数返回值
    test3(); // 测试类对象函数的绑定
    cout << "main finish!" << endl;

    return 0;
}