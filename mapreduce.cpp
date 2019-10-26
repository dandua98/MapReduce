#include <iostream>
#include <map>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "mapreduce.h"
#include "threadpool.h"

std::vector<std::multimap<std::string, std::string>> partitions;
std::vector<pthread_mutex_t> partition_locks;

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

  // temp
  // for (size_t i = 0; i < num_partitions; i++)
  // {
  //   for (std::multimap<std::string, std::string>::const_iterator iter = partitions[num_partitions].begin();
  //        iter != partitions[num_partitions].end(); ++iter)
  //   {
  //     std::cout << iter->first << '\t' << iter->second << '\n';
  //   }
  // }

  reducer = concate;
  for (size_t i = 0; i < num_reducers; i++)
  {
    MR_ProcessPartition(i);
  }
}

void MR_Emit(char *key, char *value)
{
  unsigned long partition_index = MR_Partition(key, num_partitions);
  partitions[partition_index].insert(std::make_pair(std::string(key), std::string(value)));
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
}

char *MR_GetNext(char *key, int partition_number)
{
  return NULL;
}
