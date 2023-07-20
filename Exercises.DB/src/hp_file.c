#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define HP_ERROR -1
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
    
  CALL_BF(BF_CreateFile(fileName));

  int fd1; //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  CALL_BF(BF_OpenFile(fileName, &fd1));
   
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);
  
  //Allocate memory for the first block which is the heap file info
  HP_info *Description = malloc(sizeof(HP_info));
  Description->fileDesc = fd1;
  Description->id = 0; //Heap file
  Description->NumRecords = Max_Records;
  Description->IdLast =0; //No other record

  CALL_BF(BF_AllocateBlock(fd1 ,block));
  data = BF_Block_GetData(block);
  memcpy(data, Description, sizeof(HP_info));
  

  BF_Block_SetDirty(block);
  CALL_BF( BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  CALL_BF (BF_CloseFile(fd1));

  return 0;
}

HP_info* HP_OpenFile(char *fileName)
{
  int fd1; //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  int result = BF_OpenFile(fileName, &fd1);
  if(result < 0) //If BF_OpenFile does not successs, print error code. 
  {
    BF_PrintError(result);
    return NULL;
  }

  void* data;
  BF_Block *block;
  BF_Block_Init(&block); //Alocating a temp block

  BF_GetBlock(fd1,0, block);
  data = BF_Block_GetData(block);
  if(((HP_info*)data)->id!=0)
  {
  printf("Not a heap file");
  return NULL;
  }
  HP_info *hp = malloc(sizeof(HP_info));
  memcpy(hp,data, sizeof(HP_info));

  //We need to return the HT_info 
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
    
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
  int total_records; //How many records can be stored in each block
  int records;
  int num;
  void *data;
  int result;
  BF_Block *block;
  BF_Block_Init(&block);
  

  if(hp_info->fileDesc < 0)
  {
    return -1; //error
  }

  total_records = hp_info->NumRecords;
  if(hp_info->IdLast == 0)
  {
    //Case 1: Empty Heap
    result = BF_AllocateBlock(hp_info->fileDesc,block);   
    BF_GetBlock(hp_info->fileDesc,1,block);
    data = BF_Block_GetData(block); 
    memcpy((data), &record, sizeof(Record));

    //We need to add the information about the block
    HP_block_info info;
    info.counter =1;
    info.next =NULL;
    memcpy(data+(Max_Records*sizeof(Record)), &info, sizeof(HP_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    //We need to modify the information of the HP_info
    hp_info->IdLast = 1;
    BF_GetBlock(hp_info->fileDesc,0,block);
    data = BF_Block_GetData(block); 
    memcpy((data), hp_info, sizeof(HP_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    return 1; //We added at block 1
   
  }
  else //There is already a block
  {
    //We find the last block and we need to know how many records are in it
    BF_GetBlock(hp_info->fileDesc, hp_info->IdLast,block);
    data = BF_Block_GetData(block);
    HP_block_info *hp_block_info;
    hp_block_info=data + Max_Records*sizeof(Record);

    if (hp_block_info->counter == total_records) 
    {
      //Case 2: The block is already full
      //Create a new block
      void *data1;
      BF_Block *block1;                     //Initializing the "new" last block
      BF_Block_Init(&block1);
      result = BF_AllocateBlock(hp_info->fileDesc,block1);
      BF_GetBlockCounter(hp_info->fileDesc,&num); //where we have to store the new record
      num--;
      BF_GetBlock(hp_info->fileDesc,num,block1);
      data1 = BF_Block_GetData(block1); 

      //store the record
      memcpy((data1), &record, sizeof(Record));

      //create a new hp_block_info
      HP_block_info info;
      info.counter =1;
      info.next =NULL;
      
      //Store the hp_block_info at the end of data
      memcpy(data1+(Max_Records*sizeof(Record)), &info, sizeof(HP_block_info));
      BF_Block_SetDirty(block1); //For the "new "last block
      BF_UnpinBlock(block1);

      //link the pointers
      hp_block_info->next=data1;
      memcpy(data + (Max_Records*sizeof(Record)), hp_block_info, sizeof(HP_block_info));
  
      //Update the HP_info
      hp_info->IdLast = num;
      BF_GetBlock(hp_info->fileDesc,0,block);
      data = BF_Block_GetData(block); 
      memcpy((data), hp_info, sizeof(HP_info));
      BF_Block_SetDirty(block);
      BF_UnpinBlock(block);

      return num;
    }
    else //base case
    {
      //Case 3: We just add the new record
      //store the record
      memcpy(data + (hp_block_info->counter * sizeof(Record)), &record, sizeof(Record));
      hp_block_info->counter++;
      memcpy(data + (Max_Records*sizeof(Record)), hp_block_info, sizeof(HP_block_info));

      //We do not need to update the HP_info
      return hp_info->IdLast;
    }
  }
}

int HP_GetAllEntries(HP_info* hp_info, int value)
{
  int total_records;
  int count=1;
  int result;
  int temp;
  BF_Block *block;
  BF_Block_Init(&block);

  if(hp_info->fileDesc < 0)
  {
    return -1; //error
  }

  total_records = hp_info->NumRecords;
  if (total_records == 0) //Empty
  {
    printf("The file has no records.\n");
    return -1;
  }

  int blocks;
  BF_GetBlockCounter(hp_info->fileDesc,&blocks);
  blocks =blocks -1; //because of block 0 
  for(int i=1; i<blocks; i++)
  {
    //Read the first block
    void* data;
    BF_GetBlock(hp_info->fileDesc,i,block);
    data = BF_Block_GetData(block);
    HP_block_info * hp_block_info;
    hp_block_info=data + Max_Records*sizeof(Record);
    temp = hp_block_info->counter;

    //For each block check all its records
    for (int j=0; j<temp; j++)
    {
      Record * rec;
      rec = data + j*sizeof(Record);
      if (rec->id == value) //We found the record
      {
        printRecord(*rec);
        return i;
      } 
    }
  }
  return blocks;
}
