#include "threadpool.h"

ThreadPool_t *ThreadPool_create(int num)
{
  ThreadPool_work_queue_t *workQueue = new ThreadPool_work_queue_t;
  workQueue->wq_mutex = PTHREAD_MUTEX_INITIALIZER;
  workQueue->wq_cond = PTHREAD_COND_INITIALIZER;

  ThreadPool_t *threadPool = new ThreadPool_t;
  threadPool->workQueue = workQueue;

  for (size_t i = 0; i < (unsigned int)num; i++)
  {
    pthread_t *thread = new pthread_t;
    pthread_create(thread, NULL, (void *(*)(void *))Thread_run, threadPool);
    threadPool->workers.push_back(thread);
  }

  threadPool->condition = running;

  return threadPool;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg)
{
  ThreadPool_work_t *work = new ThreadPool_work_t;
  work->func = func;
  work->arg = arg;

  pthread_mutex_lock(&tp->workQueue->wq_mutex);
  tp->workQueue->tasks.push(work);
  pthread_cond_signal(&tp->workQueue->wq_cond);
  pthread_mutex_unlock(&tp->workQueue->wq_mutex);

  return true;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp)
{
  pthread_mutex_lock(&tp->workQueue->wq_mutex);
  while (tp->condition != stopped && tp->workQueue->tasks.empty())
  {
    pthread_cond_wait(&tp->workQueue->wq_cond, &tp->workQueue->wq_mutex);
  }

  if (tp->condition == stopped)
  {
    pthread_mutex_unlock(&tp->workQueue->wq_mutex);
    pthread_exit(NULL);
  }

  ThreadPool_work_t *work = tp->workQueue->tasks.front();
  tp->workQueue->tasks.pop();

  pthread_mutex_unlock(&tp->workQueue->wq_mutex);

  return work;
}

void *Thread_run(ThreadPool_t *tp)
{
  while (true)
  {
    ThreadPool_work_t *work = ThreadPool_get_work(tp);
    work->func(work->arg);
    delete work;
  }
  return NULL;
}

void ThreadPool_destroy(ThreadPool_t *tp)
{
  while (true)
  {
    pthread_mutex_lock(&tp->workQueue->wq_mutex);
    int size = tp->workQueue->tasks.size();
    pthread_mutex_unlock(&tp->workQueue->wq_mutex);
    if (size == 0)
    {
      break;
    }
  }

  pthread_mutex_lock(&tp->workQueue->wq_mutex);
  tp->condition = stopped;
  pthread_cond_broadcast(&tp->workQueue->wq_cond);
  pthread_mutex_unlock(&tp->workQueue->wq_mutex);

  for (size_t i = 0; i < (unsigned int)tp->workers.size(); i++)
  {
    pthread_join(*tp->workers[i], NULL);
    pthread_cond_broadcast(&tp->workQueue->wq_cond);
    delete tp->workers[i];
  }
  tp->workers.clear();

  pthread_mutex_destroy(&tp->workQueue->wq_mutex);
  pthread_cond_destroy(&tp->workQueue->wq_cond);

  delete tp->workQueue;
  delete tp;
}
