#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define HP_ERROR -1
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HP_ERROR;        \
    }                         \
  }

int MinTable(int A[], int lenght)
{
  int min = 1000000;
  for (int i=0; i<lenght; i++)
  {
    if (A[i]< min)
    {
      min = A[i];
    }
  }
  return min;
}

int MaxTable(int A[], int lenght)
{
  int max = -1;
  for (int i=0; i<lenght; i++)
  {
    if (A[i]> max)
    {
      max = A[i];
    }
  }
  return max;
}

int Sum(int A[], int lenght)
{
  int sum =0;
  for (int i=0; i<lenght; i++)
  {
    sum = sum + A[i];
  }
  return sum;
}

int HashFunction(int id, int buckets)
{
  return (id % buckets);
}

int HT_CreateFile(char *fileName, Record_Attribute keyAttribute, int buckets)
{
  (BF_CreateFile(fileName));
  int fd1;                      //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  (BF_OpenFile(fileName, &fd1));

  for (int i = 0; i < 2; i++) //Block 0: Hashing Info, Block 1:  Hash Table
  {
    void *data;
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_OR_DIE(BF_AllocateBlock(fd1, block));
    data = BF_Block_GetData(block);

    if (i == 0) //Block 0
    {
      //Allocate memory for the first block which is the hash file info
      HT_info *Description = malloc(sizeof(HT_info));
      Description->id = 1;                        //Hash table
      Description->numBuckets = buckets;          // το πλήθος των “κάδων” του αρχείου κατακερματισμού
      Description->keyAttribute = keyAttribute;   //key hash table
      Description->NumRecords = Max_Records;
      memcpy(data, Description, sizeof(HT_info));
    }
    else //Block 1
    {
      for (int z = 0; z < buckets; z++)
      {
        void *data1;
        BF_Block *block1;
        BF_Block_Init(&block1);
        CALL_OR_DIE(BF_AllocateBlock(fd1, block1)); //One block for each bucket
        data1 = BF_Block_GetData(block1);

        //We need to create the HT_block_info
        HT_block_info ht_block_info;
        BF_GetBlockCounter(fd1, &ht_block_info.blockid);
        ht_block_info.blockid = ht_block_info.blockid - 1;
        ht_block_info.next = NULL;
        ht_block_info.NumRecords = 0;

        //Store it at the end of each block
        memcpy(data1 + (Max_Records * sizeof(Record)), &ht_block_info, (sizeof(HT_block_info)));
        BF_Block_SetDirty(block1);
        CALL_OR_DIE(BF_UnpinBlock(block1));
      }
    }
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
  }
  CALL_OR_DIE(BF_CloseFile(fd1));
  return 0;
}

HT_info *HT_OpenFile(char *fileName)
{
  int fd1;
  int result = BF_OpenFile(fileName, &fd1);
  if (result < 0) //If BF_OpenFile does not successs, print error code.
  {
    BF_PrintError(result);
    return NULL;
  }

  void *data;
  BF_Block *block;
  BF_Block_Init(&block); //Alocating a temp block

  BF_GetBlock(fd1, 0, block);
  data = BF_Block_GetData(block);
  ((HT_info *)data)->fileDesc=fd1;
  if (((HT_info *)data)->id != 1)
  {
    printf("Not a hash file.\n");
    return NULL;
  }
  HT_info *ht = malloc(sizeof(HT_info));
  memcpy(ht, data, sizeof(HT_info));

  //We need to return the HT_info
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return ht;
}

int HT_CloseFile(HT_info *HT_info)
{
  int result = BF_CloseFile(HT_info->fileDesc);
  if (result < 0)
  {
    return -1;
  }
  //Free the extra memory.
  free(HT_info);
  return 0;
}

int insert_to_bucket(HT_info *ht_info, Record record, int bucket)
{
  //We need to insert to the correct bucket.
  //If the block is full, we check the next block of this bucket, untill we can insert the record.
  //After the insertion, if the block is full, we create a new count_overflow bucket.

  void *data;
  Record rec;
  int fd1 = ht_info->fileDesc;

  BF_Block *block;
  BF_Block_Init(&block);
  CALL_OR_DIE(BF_GetBlock(fd1, bucket + 1, block));
  data = BF_Block_GetData(block); //First block of the bucket

  int num_rec; //For the records which are stored in the block
  void *next_block;

  HT_block_info *ht;
  ht = data + Max_Records * sizeof(Record);
  num_rec = ht->NumRecords;

  if (num_rec < Max_Records)  //Case 1: We do not have an count_overflow bucket yet.
  {
    //We insert the record at this block
    printf("HT_InsertEntry: Record %d hashes to bucket %d. It is inserted into its first block.\n", record.id, bucket);
    memcpy(data + num_rec * sizeof(Record), &record, sizeof(Record));
    ht->NumRecords = ht->NumRecords + 1;
    memcpy(data + (Max_Records * sizeof(Record)), ht, sizeof(HT_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    num_rec = ht->NumRecords;

    printf(" The block now contains : Number: %d Names: ", num_rec);
    for (int i = 0; i < num_rec; i++)
    {
      memcpy(&rec, data + i * sizeof(Record), sizeof(Record));
      printf("%s,", rec.name);
    }
    printf("\n");

    //Now we check if the block is full, in order to create an count_overflow bucket
    if (num_rec == Max_Records)
    {
      void *data1;
      BF_Block *block1; //Initializing the "new" last block
      BF_Block_Init(&block1);
      int result = BF_AllocateBlock(fd1, block1);
      data1 = BF_Block_GetData(block1);

      int c;
      BF_GetBlockCounter(fd1, &c);
      c--;
      printf("Linking bucket %d to block %d.\n", bucket, c);

      //create a new hp_block_info
      HT_block_info ht1;
      ht1.blockid = c;
      ht1.next = NULL;
      ht1.NumRecords = 0;

      //Store the hp_block_info at the end of data
      memcpy(data1 + (Max_Records * sizeof(Record)), &ht1, sizeof(HT_block_info));
      BF_Block_SetDirty(block1); 
      BF_UnpinBlock(block1);

      //link the pointers
      ht->next = data1;
      memcpy(data + (Max_Records * sizeof(Record)), ht, sizeof(HT_block_info));
      BF_Block_SetDirty(block); 
      BF_UnpinBlock(block);
    }
    return ht->blockid;
  }
  else //Block of bucket is full
  {
    //We need to find one that is not full
    void *data1;
    BF_Block *block1;   //Temp block for the search, will store the bucket where we are going to add
    BF_Block_Init(&block1);

    HT_block_info *ht1;   //The ht_block_info for the new block
    HT_block_info *next;   //pointer for the search
    next_block = ht->next;

    while (num_rec >= Max_Records)
    {
      //We need to find the ht_block_info of the block
      next = next_block + Max_Records * sizeof(Record);
      BF_GetBlock(fd1, next->blockid, block1);
      data1 = BF_Block_GetData(block1);
      num_rec = next->NumRecords;
      next_block = next->next;  //Link the pointers
    }

    //data1 now points to the block we want to add the record, which is block1.

    printf("HT_InsertEntry: Record %d hashes to bucket %d .Writing into the available connected block: %d.\n", record.id, bucket, next->blockid);
    memcpy(data1 + num_rec * sizeof(Record), &record, sizeof(Record));
    next->NumRecords = next->NumRecords + 1;
    memcpy(data1 + (Max_Records * sizeof(Record)), next, sizeof(HT_block_info));
    BF_Block_SetDirty(block1);
    BF_UnpinBlock(block1);

    ht1 = data1 + Max_Records * sizeof(Record); //We store the HT_block_info of the block in which we just added the record
    num_rec = next->NumRecords;

    printf(" The block now contains : %d", num_rec);
    for (int i = 0; i < num_rec; ++i)
    {
      memcpy(&rec, data1 + i * sizeof(Record), sizeof(Record));
      printf(",%s", rec.name);
    }
    printf("\n");

    //Now we check if the block is full, in order to create a new one
    if (num_rec == Max_Records)
    {
      void *data2;
      BF_Block *block2;     //Initializing the "new" last block
      BF_Block_Init(&block2);
      int result = BF_AllocateBlock(fd1, block2);
      data2 = BF_Block_GetData(block2);

      int c;
      BF_GetBlockCounter(fd1, &c);
      c--;
      printf("Linking bucket %d to block %d.\n", bucket, c);

      //create a new hp_block_info
      HT_block_info ht2;
      ht2.blockid = c;
      ht2.next = NULL;
      ht2.NumRecords = 0;

      //Store the hp_block_info at the end of data
      memcpy(data2 + (Max_Records * sizeof(Record)), &ht2, sizeof(HT_block_info));

      //We link the pointers of the "old" last block to the "new last block"
      ht1->next = data2;
      memcpy(data1 + (Max_Records * sizeof(Record)), ht1, sizeof(HT_block_info));
      BF_Block_SetDirty(block2); //For the "new "last block
      BF_UnpinBlock(block2);

      BF_Block_SetDirty(block1); //For the "old"last block
      BF_UnpinBlock(block1);
    }
    return ht1->blockid;
  }
}

int HT_InsertEntry(HT_info *ht_info, Record record)
{
  //We need to find which is the key of the hash table
  int hasharg = 0;
  if (ht_info->keyAttribute == ID)
  {
    hasharg = record.id;
  }
  else if (ht_info->keyAttribute == NAME)
  {
    char clone[15];
    strcpy(clone, record.name);
    for (int i = 0; i < strlen(record.name); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (ht_info->keyAttribute == SURNAME)
  {
    char clone[20];
    strcpy(clone, record.surname);
    for (int i = 0; i < strlen(record.surname); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (ht_info->keyAttribute == CITY)
  {
    char clone[20];
    strcpy(clone, record.city);
    for (int i = 0; i < strlen(record.city); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }

  int bucket = HashFunction(hasharg, ht_info->numBuckets); //We found the correct bucket
  return insert_to_bucket(ht_info, record, bucket);
}

int HT_GetAllEntries(HT_info *ht_info, void *value)
{
  int block_num;
  int i;
  int hasharg = 0;
  int found = 0;
  int read_blocks = 0;
  int entries;
  HT_block_info *next;
  void *data;
  void *next_block;
  Record *rec;
  rec = malloc(sizeof(Record));
  char a[15];

  if (ht_info->keyAttribute == ID)
  {
    sprintf(a, "%d", *(int *)value);
    hasharg = atoi(a);
  }
  else if (ht_info->keyAttribute == NAME)
  {
    char clone[15];
    strcpy(clone, (char *)value);
    for (int i = 0; i < strlen((char *)value); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (ht_info->keyAttribute == SURNAME)
  {
    char clone[20];
    strcpy(clone, (char *)value);
    for (int i = 0; i < strlen((char *)value); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (ht_info->keyAttribute == CITY)
  {
    char clone[20];
    strcpy(clone, (char *)value);
    for (int i = 0; i < strlen((char *)value); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }

  block_num = HashFunction(hasharg, ht_info->numBuckets) + 1;

  BF_Block *block;
  BF_Block_Init(&block);
  while (block_num >= 0) //If the bucket points to at least one more block, we need to check those blocks aswell
  {
    (BF_GetBlock(ht_info->fileDesc, block_num, block));
    data = BF_Block_GetData(block); //First block of the bucket

    read_blocks++;
    HT_block_info *ht;
    ht = data + Max_Records * sizeof(Record);
    entries = ht->NumRecords;
    next_block = ht->next; //Link the pointers

    //for each block, cheack all the records that it has
    for (i = 0; i < entries; i++) 
    {
      memcpy(rec, data + i * sizeof(Record), sizeof(Record));

      //We compare value to the correct attribute
      if (ht_info->keyAttribute == ID)
      {
        if (rec->id == *(int *)value)
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s %s %s,   \n", rec->id, rec->name, rec->surname, rec->city);
          found = 1;
        }
      }
      else if (ht_info->keyAttribute == NAME)
      {
        if (!strcmp(rec->name, value))
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s %s %s,   \n", rec->id, rec->name, rec->surname, rec->city);
          found = 1;
        }
      }
      else if (ht_info->keyAttribute == SURNAME)
      {
        if (!strcmp(rec->surname, value))
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s %s %s,   \n", rec->id, rec->name, rec->surname, rec->city);
          found = 1;
        }
      }
      else if (ht_info->keyAttribute == CITY)
      {
        if (!strcmp(rec->city, value))
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s %s %s,   \n", rec->id, rec->name, rec->surname, rec->city);
          found = 1;
        }
      }
    }

    //Go to the next block
    if (next->next == NULL) //If the next is NULL, this is the last count_overflow block, so we stop
    {
      break;
    }
    next = next_block + Max_Records * sizeof(Record);
    BF_GetBlock(ht_info->fileDesc, next->blockid, block);
    data = BF_Block_GetData(block);
    block_num = next->blockid;
  }

  printf("%d Blocks read in total\n", read_blocks);
  if (found == 0)
  {
    printf("No records with the given value found.\n");
    return -1;
  }
  return read_blocks;
}

int HashStatistics( char* fileName)
{
  //How many blocks are in this file
  HT_info *info;
  int fd1;
  int result = BF_OpenFile(fileName, &fd1);
  if (result < 0) //If BF_OpenFile does not successs, print error code.
  {
    BF_PrintError(result);
    return -1;
  }

  void *data;
  BF_Block *block;
  BF_Block_Init(&block); //Alocating a temp block

  BF_GetBlock(fd1, 0, block);
  data = BF_Block_GetData(block);
  info = data;
  //info = data + (Max_Records * sizeof(Record));
  int last;
  BF_GetBlockCounter(fd1, &last);
  last--;
  printf("The file contains %d blocks.\n", last);

  //Minimun, Maximum and Average records per bucket
  int buckets = info->numBuckets;
  int count[buckets];   //Store how many records each bucket has
  int block_num;
  void* next_block;
  void *data1;
  int read_blocks[buckets];
  int count_overflow =0;   //To count buckets which have overflow block
  int OverBucket[buckets];  //To count overflow blocks per bucket

  BF_Block *block1;
  BF_Block_Init(&block1);
  HT_block_info *ht;
  HT_block_info *next;
  next_block = ht->next; //Link the pointers
  
  for (int i=0; i<buckets; i++) //Check every bucket
  {
    //First block of the bucket
    (BF_GetBlock(fd1, i+1, block1));
    data1 = BF_Block_GetData(block1);
    ht = data1 + Max_Records * sizeof(Record);
    count[i] = ht->NumRecords;
    block_num = ht->blockid;
    read_blocks[i] =1;

    //We do the first run of the loop, outside
    next_block = ht->next;
    next = next_block + Max_Records * sizeof(Record);
    block_num = next->blockid;
    next_block = next->next;
    read_blocks[i]++;
    if (next_block+ Max_Records * sizeof(Record)!=NULL) //If the next is NULL,does not have an count_overflow block, so we stop
    {
      block_num = -1;
    }
    count_overflow++;
    OverBucket[i]=1;
    while(block_num >= 0)
    {
      count[i] = count[i] + next->NumRecords;
      if (next->next == NULL) //If the next is NULL, this is the last count_overflow block, so we stop
      {
        break;
      }
      (BF_GetBlock(fd1, next->blockid, block1));
      data1 = BF_Block_GetData(block1); //First block of the bucket
      next = next_block + Max_Records * sizeof(Record);
      block_num = next->blockid;;
      next_block = next->next; //Link the pointers
      read_blocks[i]++;
      count_overflow++;
      OverBucket[i]++;
    }
  }

  printf("Minimun records per block: %d.\n", MinTable(count, buckets));
  printf("Maximum records per block: %d.\n", MaxTable(count, buckets));
  printf("Average records per block: %d.\n", Sum(count, buckets)/buckets);
  printf("Average number of blocks per bucket: %d.\n", Sum(read_blocks, buckets)/buckets);
  printf("Number of buckets which have an overflow block: %d.\n", count_overflow);
  for (int i=0; i<buckets; i++) //For every bucket
  {
    printf("Bucket %d has %d overflow blocks.\n", i, OverBucket[i]);
  }
  return 0;
}
