#include <algorithm>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mapreduce.h"
#include "threadpool.h"

std::vector<std::map<std::string, std::vector<std::string>>> partitions;
std::vector<pthread_mutex_t> partition_locks;

int num_partitions = 0;

Reducer reducer;

/*
* custom comparator to sort filenames by filesizes in descending order
*/
bool compare_file_size(char *file1, char *file2)
{
  struct stat stat1, stat2;
  stat(file1, &stat1);
  stat(file2, &stat2);
  return stat1.st_size > stat2.st_size;
}

void MR_Run(int num_files, char *filenames[],
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers)
{
  std::sort(filenames, filenames + num_files, compare_file_size);

  partitions.clear();
  partition_locks.clear();

  num_partitions = num_reducers;
  for (size_t i = 0; i < (unsigned int)num_partitions; i++)
  {
    partitions.push_back(std::map<std::string, std::vector<std::string>>());
    partition_locks.push_back(PTHREAD_MUTEX_INITIALIZER);
  }

  ThreadPool_t *mapPool = ThreadPool_create(num_mappers);
  for (size_t i = 0; i < (unsigned int)num_files; i++)
  {
    ThreadPool_add_work(mapPool, (thread_func_t)map, filenames[i]);
  }
  ThreadPool_destroy(mapPool);

  reducer = concate;
  ThreadPool_t *reducerPool = ThreadPool_create(num_partitions);
  for (size_t i = 0; i < (unsigned int)num_partitions; i++)
  {
    ThreadPool_add_work(reducerPool, (thread_func_t)MR_ProcessPartition, (void *)i);
  }
  ThreadPool_destroy(reducerPool);

  for (size_t i = 0; i < (unsigned int)num_partitions; i++)
  {
    pthread_mutex_destroy(&partition_locks[i]);
  }
}

void MR_Emit(char *key, char *value)
{
  unsigned long partition_index = MR_Partition(key, num_partitions);
  pthread_mutex_lock(&partition_locks[partition_index]);
  partitions[partition_index][std::string(key)]
      .push_back(std::string(value));
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
  for (std::pair<std::string, std::vector<std::string>> partition : partitions[partition_number])
  {
    reducer((char *)partition.first.c_str(), partition_number);
  }
}

char *MR_GetNext(char *key, int partition_number)
{
  std::string curr_key = key;

  if (!partitions[partition_number][curr_key].empty())
  {
    std::string value = partitions[partition_number][curr_key].back();
    partitions[partition_number][curr_key].pop_back();
    return (char *)value.c_str();
  }
  return NULL;
}
