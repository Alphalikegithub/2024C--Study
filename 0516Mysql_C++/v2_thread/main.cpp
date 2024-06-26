#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include "MysqlConnect.h"
#include "ConnectionPool.h"

using std::cout;
using std::cin;
using std::endl;
//1.单线程: 使用/不使用连接池
//2.多线程: 使用/不使用连接池

void op1(int begin,int end){
    for (int i = begin; i < end; ++i)
    {
        MysqlConnect conn;
        //conn.connect("root","9c57029624e3768a","testdb","172.23.27.54");
        conn.connect("root","9c57029624e3768a","testdb","127.0.0.1");
        //string sql = "insert into person values(9,28,'woman','xiaosb')";
        char sql[1024] = { 0 };
        //sprintf(sql,"insert into person values(%d,29,'man','jitang')",i);
        sprintf(sql, "insert into person values(%d, 25, 'man', 'tom')", i);
        conn.update(sql);
    }
}

void op2(ConnectionPool* pool,int begin,int end){
    for (int i = begin; i < end; ++i)
    {
        std::shared_ptr<MysqlConnect> conn = pool->getConnection();
        char sql[1024] = { 0 };
        sprintf(sql, "insert into person values(%d, 25, 'man', 'tom')", i);
        conn->update(sql);
    }
}

void test1(){
    //非连接池，单线程，用时:121593067692纳秒，121593毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op1(0,5000);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto length = end - begin;
    cout << "非连接池，单线程，用时:" << length.count() << "纳秒，" << length.count()/1000000 << "毫秒" << endl;
}
void test2(){
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op2(pool,0,5000);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto length = end - begin;
    cout << "连接池, 单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

}

void test3(){
    //非连接池，多线程，用时:81477572578纳秒，81477毫秒
    MysqlConnect conn;
    conn.connect("root","9c57029624e3768a","testdb","127.0.0.1");
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op1,0,1000);
    std::thread t2(op1,1000,2000);
    std::thread t3(op1,2000,3000);
    std::thread t4(op1,3000,4000);
    std::thread t5(op1,4000,5000);
    t1.join();//阻塞主线程
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto length = end - begin;
    cout << "非连接池，多线程，用时:" << length.count() << "纳秒，" << length.count() / 1000000 << "毫秒" << endl;

}
void test4(){
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op2,pool,0,1000);
    std::thread t2(op2,pool,1000,2000);
    std::thread t3(op2,pool,2000,3000);
    std::thread t4(op2,pool,3000,4000);
    std::thread t5(op2,pool,4000,5000);
    t1.join();//阻塞主线程
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    auto length = end - begin;
    cout << "连接池，多线程，用时:" << length.count() << "纳秒，" << length.count() / 1000000 << "毫秒" << endl;
}

int query(){
    MysqlConnect conn;
    conn.connect("root","9c57029624e3768a","testdb","127.0.0.1");
    //string sql = "insert into person values(5,24,'man','david')";
    //string sql = "insert into person values(6,25,'woman','lisa')";
    //string sql = "insert into person values(7,26,'man','baoqiang')";
    //string sql = "insert into person values(8,27,'man','xiaoniao')";
    string sql = "insert into person values(10,29,'woman','xiaosb')";
    bool flag = conn.update(sql);
    cout << "flag value:" << flag << endl;

    sql = "select * from person";
    conn.query(sql);
    while(conn.next()){
        cout << conn.value(0) << ","
             << conn.value(1) << ","
             << conn.value(2) << ","
             << conn.value(3) << endl;

    }
    return 0;
}

int main(int argc, char** argv){ // 注意参数类型应该是 char** 而不是 void**  
    // int result = query(); // 调用query函数  
    // return result; // 返回query函数的返回值  
    test4();
    return 0;
}