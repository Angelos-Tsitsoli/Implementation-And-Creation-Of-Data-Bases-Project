#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 1000 // you can change it if you want
#define FILE_NAME "data.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
  BF_Init(LRU);
  
  int test = HP_CreateFile(FILE_NAME);
  HP_info* info = HP_OpenFile(FILE_NAME);
  printf("id: %d, numRecords: %d, idlast: %d\n", info->id, info->NumRecords, info->IdLast);
  Record record;
  int result;
  srand(12569874);
  printf("Insert Entries\n");
  for (int id = 0; id < RECORDS_NUM; ++id) {
    record = randomRecord();
    HP_InsertEntry(info, record);
  }

  printf("RUN PrintAllEntries\n");
  int id =rand() % RECORDS_NUM;
  printf("Searching for: %d\n",id);
  result = HP_GetAllEntries(info, id);
  if (result == -1)
  {
    printf("Error.\n");
  }
  else
  {
    printf("Read %d blocks.\n", result);
  }

  HP_CloseFile(info);
  BF_Close();
}
