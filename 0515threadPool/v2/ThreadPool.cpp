/* #pragma once
#ifndef _THREADPOOL_H
#define _THREADPOOL_H */

#include <iostream>
#include <pthread.h>
#include <string.h>//c语言中的memory相关函数
#include <string>//c++中string类
#include<cstring>
#include <unistd.h>
#include "TaskQueue.h"

class ThreadPool
{
public:
    //创建线程池并初始化
    ThreadPool(int min,int max);
    //销毁线程池
    ~ThreadPool();

    //给线程池添加任务的函数
    void addTask(Task task);

    //获取线程池中工作的线程的个数
    int getBusyNum();

    //获取线程池中活着的线程的个数
    int getAliveNum();

private:
    //工作线程
    static void * worker(void * arg);//static是为了消除this指针
    //管理者线程
    static void * manager(void *arg);
    //单个线程退出
    void threadExit();
private:
    TaskQueue* taskQ;        //任务队列
    

    pthread_t managerID; //管理者线程ID
    pthread_t *threadIDs;//工作的线程ID

    int minNum;         //最小线程数
    int maxNum;         //最大线程数
    int busyNum; 
    int liveNum;
    int exitNum;
    pthread_mutex_t mutexPool;//锁整个的线程池
    
    pthread_cond_t notEmpty;  //任务队列是不是空了

    bool shutdown;       //是不是要销毁线程池，销毁为1，不销毁为0

    static const int NUMBER = 2;
};

/* #endif //_THREADPOOL_H */

ThreadPool::ThreadPool(int min, int max)
{
    do 
    {
        //实例化任务队列
        taskQ = new TaskQueue;
        if (taskQ == nullptr)
        {
            std::cout << "malloc taskQ fail..."<< std::endl;
            break;
        }

        threadIDs = new pthread_t[max];
        if (threadIDs == nullptr)
        {
            std::cout << "malloc threadIDs fail..."<< std::endl;
            break;
        }
        memset(threadIDs, 0, sizeof(pthread_t) * max);
        minNum = min;
        maxNum = max;
        busyNum = 0;
        liveNum = min;    // 和最小个数相等
        exitNum = 0;

        if (pthread_mutex_init(&mutexPool, NULL) != 0 || pthread_cond_init(&notEmpty, NULL) != 0 )
        {
            std::cout << "mutex or condition init fail..."<< std::endl;
            break;
        }

        shutdown = false;//线程池默认是不关闭的

        // 创建线程
        pthread_create(&managerID, NULL, manager, this);//c++中的this指针指向当前类被实例化的对象
        for (int i = 0; i < min; ++i)
        {
            pthread_create(&threadIDs[i], NULL, worker, this);
        }
        return;
    } while (0);

    // 释放资源
    if (threadIDs) delete[] threadIDs;//threadID是个数组，所以需要使用delete[]
    if (taskQ) delete taskQ;//new <----->delete
}

ThreadPool::~ThreadPool()
{
    // 关闭线程池
    shutdown = true;
    // 阻塞回收管理者线程
    pthread_join(managerID, NULL);
    // 唤醒阻塞的消费者线程
    for (int i = 0; i < liveNum; ++i)
    {
        pthread_cond_signal(&notEmpty);
    }
    // 释放堆内存
    if (taskQ)
    {
        delete taskQ;
    }
    if (threadIDs)
    {
        delete[] threadIDs;
    }

    pthread_mutex_destroy(&mutexPool);
    pthread_cond_destroy(&notEmpty);
}

void ThreadPool::addTask(Task task)
{
    /* 往任务队列中添加任务的时候其实是不需要互斥锁的 */

    //STL容器维护的queue没有上限
    /* while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        // 阻塞生产者线程
        pthread_cond_wait(&notFull, &mutexPool);
    } */
    if (shutdown)
    {
        
        return;
    }
    // 添加任务
    taskQ->addTask(task);

    pthread_cond_signal(&notEmpty);
}

int ThreadPool::getBusyNum()
{
    pthread_mutex_lock(&mutexPool);
    int busyNum = this->busyNum;
    pthread_mutex_unlock(&mutexPool);
    return busyNum;
}

int ThreadPool::getAliveNum()
{
    pthread_mutex_lock(&mutexPool);
    int aliveNum = this->liveNum;
    pthread_mutex_unlock(&mutexPool);
    return aliveNum;
}

void *ThreadPool::worker(void *arg)
{
    //ThreadPool* pool = (ThreadPool*)arg;
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while (true)
    {
        pthread_mutex_lock(&pool->mutexPool);
        // 当前任务队列是否为空
        while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
        {
            // 阻塞工作线程
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            // 判断是不是要销毁线程
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    pool->threadExit();
                }
            }
        }

        // 判断线程池是否被关闭了
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            pool->threadExit();
        }

        // 从任务队列中取出一个任务
        Task task = pool->taskQ->takeTask();
        
        // 解锁
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexPool);

        
        std::cout << "thread " << std::to_string(pthread_self()) << "start working..." << std::endl;
        
        task.function(task.arg);
        delete task.arg;
        task.arg = nullptr;

        printf("thread %ld end working...\n", pthread_self());
        //std::cout << "thread " << std::to_string(pthread_self()) << "end working..." << std::endl;
        pthread_mutex_lock(&pool->mutexPool);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexPool);
    }
    return NULL;
}

void *ThreadPool::manager(void *arg)
{
    //ThreadPool* pool = (ThreadPool*)arg;
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (!pool->shutdown)
    {
        // 每隔3s检测一次
        sleep(3);

        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->taskQ->taskNumber();
        int liveNum = pool->liveNum;
        // 取出忙的线程的数量
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexPool);


        // 添加线程
        // 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if (queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter = 0;
            for (int i = 0; i < pool->maxNum && counter < NUMBER
                && pool->liveNum < pool->maxNum; ++i)
            {
                if (pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }
        // 销毁线程
        // 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busyNum * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            // 让工作的线程自杀
            for (int i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}

void ThreadPool::threadExit()
{
    pthread_t tid = pthread_self();
    for (int i = 0; i < maxNum; ++i)
    {
        if (threadIDs[i] == tid)
        {
            threadIDs[i] = 0;
            //printf("threadExit() called, %ld exiting...\n", tid);
            std::cout << "threadExit() called," << std::to_string(tid) << "exiting..." << std::endl ;
            break;
        }
    }
    pthread_exit(NULL);
}
