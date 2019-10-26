#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "mapreduce.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Map(char *file_name)
{
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);
    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1)
    {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL)
            MR_Emit(token, (char *)"1");
    }
    free(line);
    fclose(fp);
}

void Reduce(char *key, int partition_number)
{
    int count = 0;
    char *value, name[100];
    while ((value = MR_GetNext(key, partition_number)) != NULL)
        count++;
    sprintf(name, "result-%d.txt", partition_number);
    FILE *fp = fopen(name, "a");
    printf("%s: %d\n", key, count);
    fprintf(fp, "%s: %d\n", key, count);
    fclose(fp);
}

int main(int argc, char *argv[])
{
    MR_Run(argc - 1, &(argv[1]), Map, 10, Reduce, 10);
}
