#include <iostream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mapreduce.h"
#include "threadpool.h"

std::vector<std::multimap<std::string, std::string>> partitions;
std::vector<pthread_mutex_t> partition_locks;

std::vector<std::multimap<std::string, std::string>::iterator> reducer_iterators;

int num_partitions = 0;

Reducer reducer;

/*
* custom sorter to sort filenames by filesizes in descending order
*/
bool compareFileSize(char *file1, char *file2)
{
  struct stat file1StatBuf, file2StatBuf;
  stat(file1, &file1StatBuf);
  stat(file2, &file2StatBuf);
  return file1StatBuf.st_size > file2StatBuf.st_size;
}

void MR_Run(int num_files, char *filenames[],
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers)
{
  for (size_t i = 0; i < num_files; i++)
  {
    if (access(filenames[i], F_OK) != 0)
    {
      std::cout << filenames[i] << " not found" << std::endl;
      exit(1);
    }
  }

  std::sort(filenames, filenames + num_files, compareFileSize);

  partitions.clear();
  partition_locks.clear();

  num_partitions = num_reducers;
  for (size_t i = 0; i < num_partitions; i++)
  {
    partitions.push_back(std::multimap<std::string, std::string>());
    partition_locks.push_back(PTHREAD_MUTEX_INITIALIZER);
    pthread_mutex_init(&partition_locks[i], 0);
  }

  ThreadPool_t *mapPool = ThreadPool_create(num_mappers);
  for (size_t i = 0; i < num_files; i++)
  {
    ThreadPool_add_work(mapPool, (thread_func_t)map, filenames[i]);
  }
  ThreadPool_destroy(mapPool);

  reducer = concate;
  for (size_t i = 0; i < num_reducers; i++)
  {
    reducer_iterators.push_back(partitions[i].begin());
    pthread_t *thread = new pthread_t;
    pthread_create(thread, NULL, (void *_Nullable (*)(void *))MR_ProcessPartition, (void *)i);
  }
}

void MR_Emit(char *key, char *value)
{
  unsigned long partition_index = MR_Partition(key, num_partitions);

  pthread_mutex_lock(&partition_locks[partition_index]);
  partitions[partition_index]
      .insert(std::make_pair(std::string(key), std::string(value)));
  pthread_mutex_unlock(&partition_locks[partition_index]);
}

unsigned long MR_Partition(char *key, int num_partitions)
{
  unsigned long hash = 5381;
  int c;

  while ((c = *key++) != '\0')
    hash = hash * 33 + c;

  return hash % num_partitions;
}

void MR_ProcessPartition(int partition_number)
{
  std::multimap<std::string, std::string> partitionMap = partitions[partition_number];
  std::multimap<std::string, std::string>::iterator it, end;

  for (it = partitionMap.begin(), end = partitionMap.end();
       it != end; it = partitionMap.upper_bound(it->first))
  {
    reducer((char *)it->first.c_str(), partition_number);
  }
}

char *MR_GetNext(char *key, int partition_number)
{
  std::multimap<std::string, std::string>::iterator it = reducer_iterators[partition_number];

  if (it == partitions[partition_number].end())
  {
    return NULL;
  }

  if (it->first == std::string(key))
  {
    char *val = (char *)it->second.c_str();
    reducer_iterators[partition_number]++;
    return val;
  }
  else
  {
    return NULL;
  }
}
