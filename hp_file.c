#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

int HP_CreateFile(char *fileName)
{
    return 0;
}

HP_info* HP_OpenFile(char *fileName){
    return NULL ;
}


int HP_CloseFile( HP_info* hp_info ){
    return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
   return 0;
}

