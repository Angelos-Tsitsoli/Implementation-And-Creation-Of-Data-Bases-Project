#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

int HP_CreateFile(char *fileName)
{
  
  int result = BF_CreateFile(fileName);
  if ( result < 0) //If BF_CreateFile does not successs, print error code.
  {
    BF_PrintError("CreateFile error.\n");
    return -1;
  } 

  int fd1; //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  result = BF_OpenFile(fileName, &fd1);
  if(result < 0) //If BF_OpenFile does not successs, print error code. 
  {
    BF_PrintError("OpenFile error.\n");
    return -1;
  }
  
  void* data;

  //Allocate memory for the first block whitch is the heap file info
  HP_info Description;
  Description.fileDesc = fd1;
  Description.id = 0; //Heap file
  //Description.NumRecords = 
  Description.IdLast =0; //No other record

  result = BF_AllocateBlock(fd1 ,&Description);
  if(result < 0)
  {
    BF_PrintError("Error in allocating block.\n");
    return -1;
  }

  result = BF_CloseFile(fd1);
  if(result < 0)
  {
    BF_PrintError("CloseFile error.\n");
    return -1;
  }
  return 0;
}

HP_info* HP_OpenFile(char *fileName)
{
  int fd1; //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  int result = BF_OpenFile(fileName, &fd1);
  if(result < 0) //If BF_OpenFile does not successs, print error code. 
  {
    BF_PrintError("OpenFile error.\n");
    return NULL;
  }
  HP_info *info = BF_Block_GetData(fd1);
  if (info == NULL)
  {
    BF_PrintError("Get Block Data error.\n");
    return NULL;
  }
  if (info->id == 1) //It is a hash table
  {
    BF_PrintError("The file is not heap organazed.\n");
    return NULL;
  }

  //We need to return the HT_info 
  info ->fileDesc = fd1;
  
  HP_info *hp = malloc(sizeof(HP_info));
  hp->fileDesc = info ->fileDesc;
  hp->id = 0;
  hp->NumRecords = info->NumRecords;
  hp->IdLast = info->IdLast;

  return hp;
}

int HP_CloseFile( HP_info* hp_info )
{
  int result = BF_CloseFile(hp_info ->fileDesc);
  if (result < 0)
  {
    return -1;
  }
  //Free the extra memory.
  free(hp_info);
  return 0;
}

int HP_InsertEntry(HP_info* hp_info, Record record)
{
  //When we insert in a heap file, we insert always at the end
  //int last = hp_info->IdLast;
  //int fileDesc= hp_info->fileDesc;
  //int fd1; //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  //int result = BF_OpenFile(fileName, &fd1);
  //if(result < 0) //If BF_OpenFile does not successs, print error code. 
  //{
  //  BF_PrintError("OpenFile error.\n");
  //  return -1;
  //}

  //BF_getBlockCounter(file_desc) - 1 pointer se last record
  
  
  
  //πρεπει να ενημερωσουμε τις καταλλήλες δομες
  //hp_infp & HP_block_info

  return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value)
{
  return 0;
}
