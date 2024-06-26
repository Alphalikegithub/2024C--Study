#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* //声明实现文件中定义的结构体
typedef struct ThreadPool ThreadPool;//为struct ThreadPool结构体定义一个别名ThreadPool
 */
//任务结构体
typedef struct  Task
{
    void (*function)(void *arg);//void *可以兼容各种各样的数据类型
    void *arg;
}Task;
//线程池结构体
typedef struct ThreadPool
{
    Task* taskQ;        //任务队列
    int queueCapacity;  //容量
    int queueSize;      //当前任务个数
    int queueFront;     //队头->取数据
    int queueRear;      //队尾->放数据

    pthread_t managerID; //管理者线程ID
    pthread_t *threadIDs;//工作的线程ID

    int minNum;         //最小线程数
    int maxNum;         //最大线程数
    int busyNum; 
    int liveNum;
    int exitNum;
    pthread_mutex_t mutexPool;//锁整个的线程池
    pthread_mutex_t mutexBusy;//锁busyNum变量
    pthread_cond_t notFull;   //任务队列是不是满了
    pthread_cond_t notEmpty;  //任务队列是不是空了

    int shutdown;       //是不是要销毁线程池，销毁为1，不销毁为0
}ThreadPool;

//创建线程池并初始化
ThreadPool * threadPoolCreate(int min,int max,int queueCapacity);
//销毁线程池
int threadPoolDestory(ThreadPool *pool);

//给线程池添加任务的函数
void threadPoolAdd(ThreadPool *pool,void(*func)(void *),void *arg);

//获取线程池中工作的线程的个数
int threadPoolBusyNum(ThreadPool *pool);

//获取线程池中活着的线程的个数
int threadPoolAliveNum(ThreadPool *pool);

//工作线程
void * worker(void * arg);
//管理者线程
void * manager(void *arg);

void threadExit(ThreadPool *pool);



#endif //_THREADPOOL_H