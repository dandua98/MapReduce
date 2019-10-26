#include "threadpool.h"
#include <iostream>

void *threadRunner(void *param)
{
  Thread_run((ThreadPool_t *)param);
  return NULL;
}

ThreadPool_t *ThreadPool_create(int num)
{
  ThreadPool_t *threadPool = new ThreadPool_t;

  ThreadPool_work_queue_t *taskQueue = new ThreadPool_work_queue_t;
  threadPool->taskQueue = taskQueue;

  for (size_t i = 0; i < num; i++)
  {
    pthread_t *thread = new pthread_t;
    pthread_create(thread, NULL, threadRunner, threadPool);
    threadPool->workers.push_back(thread);
  }

  threadPool->condition = running;

  pthread_mutex_init(&threadPool->taskQueue->taskMutex, NULL);
  pthread_cond_init(&threadPool->taskQueue->wcond, NULL);

  return threadPool;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg)
{
  ThreadPool_work_t *work = new ThreadPool_work_t;
  work->func = func;
  work->arg = arg;

  int error = pthread_mutex_lock(&tp->taskQueue->taskMutex);
  if (error < 0)
  {
    return false;
  }
  tp->taskQueue->tasks.push(work);

  pthread_cond_signal(&tp->taskQueue->wcond);
  pthread_mutex_unlock(&tp->taskQueue->taskMutex);

  return true;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp)
{
  pthread_mutex_lock(&tp->taskQueue->taskMutex);
  while (tp->condition != stopped && tp->taskQueue->tasks.empty())
  {
    pthread_cond_wait(&tp->taskQueue->wcond, &tp->taskQueue->taskMutex);
  }

  if (tp->condition == stopped)
  {
    pthread_mutex_unlock(&tp->taskQueue->taskMutex);
    pthread_exit(NULL);
    return NULL;
  }

  ThreadPool_work_t *work = tp->taskQueue->tasks.front();
  tp->taskQueue->tasks.pop();

  pthread_mutex_unlock(&tp->taskQueue->taskMutex);

  return work;
}

void *Thread_run(ThreadPool_t *tp)
{
  ThreadPool_work_t work = *ThreadPool_get_work(tp);
  work.func(work.arg);
  return NULL;
}

void ThreadPool_destroy(ThreadPool_t *tp)
{
  pthread_mutex_lock(&tp->taskQueue->taskMutex);
  tp->condition = stopped;
  pthread_cond_broadcast(&tp->taskQueue->wcond);
  pthread_mutex_unlock(&tp->taskQueue->taskMutex);

  for (size_t i = 0; i < tp->workers.size(); i++)
  {
    pthread_join(*tp->workers[i], NULL);
    pthread_cond_broadcast(&tp->taskQueue->wcond);
  }
  tp->workers.clear();

  pthread_mutex_destroy(&tp->taskQueue->taskMutex);
  pthread_cond_destroy(&tp->taskQueue->wcond);
}
