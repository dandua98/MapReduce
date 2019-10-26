#include "threadpool.h"
#include <iostream>
void *threadRunner(void *param)
{
  Thread_run((ThreadPool_t *)param);
  return NULL;
}

ThreadPool_t *ThreadPool_create(int num)
{
  ThreadPool_t threadPool;

  for (size_t i = 0; i < num; i++)
  {
    pthread_t thread;
    pthread_create(&thread, NULL, threadRunner, &threadPool);
    threadPool.workers.push_back(&thread);
  }

  pthread_mutex_init(&threadPool.taskQueue.taskMutex, 0);
  pthread_cond_init(&threadPool.taskQueue.wcond, 0);

  ThreadPool_t *tp = &threadPool;
  return tp;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg)
{
  ThreadPool_work_t work = {.func = func, .arg = arg};

  int error = pthread_mutex_lock(&tp->taskQueue.taskMutex);
  if (error < 0)
  {
    return false;
  }
  tp->taskQueue.tasks.push(work);

  pthread_cond_signal(&tp->taskQueue.wcond);
  pthread_mutex_lock(&tp->taskQueue.taskMutex);

  return true;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp)
{
  std::queue<ThreadPool_work_t> *tasks = &tp->taskQueue.tasks;

  pthread_mutex_lock(&tp->taskQueue.taskMutex);
  while (tasks->empty())
  {
    pthread_cond_wait(&tp->taskQueue.wcond, &tp->taskQueue.taskMutex);
  }

  ThreadPool_work_t *work = &tasks->front();
  tasks->pop();

  pthread_mutex_unlock(&tp->taskQueue.taskMutex);

  return work;
}

void *Thread_run(ThreadPool_t *tp)
{
  ThreadPool_work_t *work = ThreadPool_get_work(tp);
  work->func(work->arg);
  return NULL;
}

void ThreadPool_destroy(ThreadPool_t *tp)
{
  for (size_t i = 0; i < tp->workers.size(); i++)
  {
    ThreadPool_add_work(tp, NULL, NULL);
  }

  for (size_t i = 0; i < tp->workers.size(); i++)
  {
    pthread_join(*tp->workers[i], NULL);
    delete *tp->workers[i];
  }
  tp->workers.clear();

  pthread_mutex_destroy(&tp->taskQueue.taskMutex);
  pthread_cond_destroy(&tp->taskQueue.wcond);
}
