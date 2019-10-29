#include "threadpool.h"

ThreadPool_t *ThreadPool_create(int num)
{
  ThreadPool_t *threadPool = new ThreadPool_t;
  threadPool->workQueue = new ThreadPool_work_queue_t;

  for (size_t i = 0; i < (unsigned int)num; i++)
  {
    pthread_t *thread = new pthread_t;
    pthread_create(thread, NULL, (void *(*)(void *))Thread_run, threadPool);
    threadPool->workers.push_back(thread);
  }

  return threadPool;
}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg)
{
  pthread_mutex_lock(&tp->workQueue->wq_mutex);
  tp->workQueue->tasks.push(new ThreadPool_work_t{func, arg});
  pthread_mutex_unlock(&tp->workQueue->wq_mutex);
  pthread_cond_signal(&tp->workQueue->wq_cond);

  return true;
}

ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp)
{
  pthread_mutex_lock(&tp->workQueue->wq_mutex);
  while (!tp->stopped && tp->workQueue->tasks.empty())
  {
    pthread_cond_wait(&tp->workQueue->wq_cond, &tp->workQueue->wq_mutex);
  }

  if (tp->stopped)
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
}

void ThreadPool_destroy(ThreadPool_t *tp)
{
  while (true)
  {
    pthread_mutex_lock(&tp->workQueue->wq_mutex);
    int size = tp->workQueue->tasks.size();
    pthread_mutex_unlock(&tp->workQueue->wq_mutex);
    if (size == 0)
      break;
  }

  pthread_mutex_lock(&tp->workQueue->wq_mutex);
  tp->stopped = true;
  pthread_mutex_unlock(&tp->workQueue->wq_mutex);
  pthread_cond_broadcast(&tp->workQueue->wq_cond);

  for (size_t i = 0; i < (unsigned int)tp->workers.size(); i++)
  {
    pthread_join(*tp->workers[i], NULL);
    delete tp->workers[i];
  }

  pthread_mutex_destroy(&tp->workQueue->wq_mutex);
  pthread_cond_destroy(&tp->workQueue->wq_cond);

  delete tp->workQueue;
  delete tp;
}
