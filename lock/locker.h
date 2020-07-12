#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

//typedef atomic_t sem_t;

//信号量、共享内存，以及消息队列等System主要关注进程间通信；
//条件变量、互斥锁，主要关注线程间通信。

class sem
{
public:
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)        //int sem_init (sem_t *sem, int pshared, unsigned int value); return 0 always;信号量
        {
            throw std::exception();             //抛出异常
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);        //int sem_destroy (sem_t *sem); We're done with the semaphore, destroy it.
    }
    bool wait()
    {
        return sem_wait(&m_sem) == 0;   //Wait for semaphore (blocking).
    }
    bool post()
    {
        return sem_post(&m_sem) == 0;   //Post a semaphore.
    }

private:
    sem_t m_sem;
};
class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)//线程锁：互斥变量
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};
class cond
{
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)  //初始化一个条件变量
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);      //销毁一个条件变量
    }
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);      //令一个消费者等待在条件变量上
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;   //生产者通知等待在条件变量上的消费者
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;    //生产者向消费者广播消息
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;              //条件变量
};
#endif
