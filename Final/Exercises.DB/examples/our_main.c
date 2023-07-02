#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 30 // you can change it if you want
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main()
{
    BF_Init(LRU);
    //srand(12569874);
    printf("Creating a primate index.\n");
    HT_CreateFile(FILE_NAME,ID,10);
    printf("Creating a secondary index with key the name of the record.\n");
    SHT_CreateSecondaryIndex(INDEX_NAME,NAME,10,FILE_NAME);
    HT_info* info = HT_OpenFile(FILE_NAME);
    SHT_info* index_info = SHT_OpenSecondaryIndex(INDEX_NAME);
    printf("HT fileDesc: %d  SHT fileDesc: %d\n",info->fileDesc ,index_info->fileDesc);

    Record record=randomRecord();
    char searchName[15];
    strcpy(searchName, record.name);
    int result;

    printf("Insert Entries\n");
    for (int id = 0; id < RECORDS_NUM; ++id) 
    {
        record = randomRecord();
        printf("Inserting at the primate index.\n");
        int block_id = HT_InsertEntry(info, record);
        printf("Inserting at the secondary index.\n");
        SHT_SecondaryInsertEntry(index_info, record, block_id);
        printf("\n");
    }

    printf("RUN PrintAllEntries for primate index.\n");
    int id =rand() % RECORDS_NUM;
    printf("Searching for: %d\n",id);
    result = HT_GetAllEntries(info, &id);
    if (result == -1)
    {
        printf("Error.\n");
    }
    else
    {
        printf("Read %d blocks.\n", result);
    }

    printf("\nRUN PrintAllEntries for secondary index.\n");
    printf("Searching for: %s\n",searchName);
    SHT_SecondaryGetAllEntries(info,index_info,searchName);

    printf("\nHash Statistics for primate index:\n");
    HashStatistics(FILE_NAME);

    printf("\nHash Statistics for secondary index:\n");
    SHT_HashStatistics(INDEX_NAME);
}