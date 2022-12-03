#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }


int HT_CreateFile(char *fileName,  int buckets){
    return 0;
}

HT_info* HT_OpenFile(char *fileName){
    return NULL;
}


int HT_CloseFile( HT_info* HT_info ){
    return 0;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
    return 0;
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
    return 0;
}




