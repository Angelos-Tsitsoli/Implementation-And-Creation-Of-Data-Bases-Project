#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
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

int SHT_CreateSecondaryIndex(char *sfileName,Record_Attribute keyAttribute, int buckets, char* fileName)
{
  //Same steps like in HT_CreateFile
  (BF_CreateFile(sfileName));
  int sfd1;                      //για την αναγνώριση ενός αρχείου το οποίο δημιουργείται από την Bf
  (BF_OpenFile(sfileName, &sfd1));

  for (int i = 0; i < 2; i++) //Block 0: Hashing Info, Block 1:  Hash Table
  {
    void *data;
    BF_Block *block;
    BF_Block_Init(&block);
    CALL_OR_DIE(BF_AllocateBlock(sfd1, block));
    data = BF_Block_GetData(block);

    if (i == 0) //Block 0
    {
      //Allocate memory for the first block which is the hash file info
      HT_info *Description = malloc(sizeof(HT_info));
      Description->id = 1;                        //Hash table
      Description->numBuckets = buckets;          // το πλήθος των “κάδων” του αρχείου κατακερματισμού
      Description->keyAttribute = keyAttribute;   //key hash table
      Description->NumRecords = Max_Pairs;
      memcpy(data, Description, sizeof(SHT_info));
    }
    else //Block 1
    {
      for (int z = 0; z < buckets; z++)
      {
        void *data1;
        BF_Block *block1;
        BF_Block_Init(&block1);
        CALL_OR_DIE(BF_AllocateBlock(sfd1, block1)); //One block for each bucket
        data1 = BF_Block_GetData(block1);

        //We need to create the HT_block_info
        HT_block_info sht_block_info;
        BF_GetBlockCounter(sfd1, &sht_block_info.blockid);
        sht_block_info.blockid = sht_block_info.blockid - 1;
        sht_block_info.next = NULL;
        sht_block_info.NumRecords = 0;

        //Store it at the end of each block
        memcpy(data1 + (Max_Pairs * sizeof(SHT_pair)), &sht_block_info, (sizeof(SHT_block_info)));
        BF_Block_SetDirty(block1);
        CALL_OR_DIE(BF_UnpinBlock(block1));
      }
    }
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
  }
  CALL_OR_DIE(BF_CloseFile(sfd1));
  return 0;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName)
{
  int fd1;
  int result = BF_OpenFile(indexName, &fd1);
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
  ((SHT_info *)data)->fileDesc=fd1;
  if (((SHT_info *)data)->id != 1)
  {
    printf("Not a hash file.\n");
    return NULL;
  }
  SHT_info *sht = malloc(sizeof(SHT_info));
  memcpy(sht, data, sizeof(SHT_info));

  //We need to return the HT_info
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return sht;
}

int SHT_CloseSecondaryIndex( SHT_info* SHT_info )
{
  int result = BF_CloseFile(SHT_info->fileDesc);
  if (result < 0)
  {
    return -1;
  }
  //Free the extra memory.
  free(SHT_info);
  return 0;
}

int insert_to_bucket2(SHT_info *sht_info, SHT_pair pair, int bucket)
{
  void *data;
  SHT_pair p;
  int sfd1 = sht_info->fileDesc;

  BF_Block *block;
  BF_Block_Init(&block);
  (BF_GetBlock(sfd1, bucket + 1, block));
  data = BF_Block_GetData(block); //First block of the bucket

  int num_pair; //For the pairs which are stored in the block
  void *next_block;

  SHT_block_info *sht;
  sht = data + Max_Pairs * sizeof(SHT_pair);
  num_pair = sht->NumPairs;

  if (num_pair < Max_Pairs)  //Case 1: We do not have an count_overflow bucket yet.
  {
    //We insert the record at this block
    printf("SHT_InsertEntry: Name %s hashes to bucket %d. It is inserted into its first block.\n", pair.name, bucket);
    memcpy(data + num_pair * sizeof(SHT_pair), &pair, sizeof(SHT_pair));
    sht->NumPairs = sht->NumPairs + 1;
    memcpy(data + (Max_Pairs * sizeof(SHT_pair)), sht, sizeof(SHT_block_info));
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    num_pair = sht->NumPairs;

    printf(" The block now contains : Number: %d Names: ", num_pair);
    for (int i = 0; i < num_pair; i++)
    {
      memcpy(&p, data + i * sizeof(SHT_pair), sizeof(SHT_pair));
      printf("%s,", p.name);
    }
    printf("\n");

    //Now we check if the block is full, in order to create an count_overflow bucket
    if (num_pair == Max_Pairs)
    {
      void *data1;
      BF_Block *block1; //Initializing the "new" last block
      BF_Block_Init(&block1);
      int result = BF_AllocateBlock(sfd1, block1);
      data1 = BF_Block_GetData(block1);

      int c;
      BF_GetBlockCounter(sfd1, &c);
      c--;
      printf("Linking bucket %d to block %d.\n", bucket, c);

      //create a new hp_block_info
      SHT_block_info sht1;
      sht1.blockid = c;
      sht1.next = NULL;
      sht1.NumPairs = 0;

      //Store the hp_block_info at the end of data
      memcpy(data1 + (Max_Pairs * sizeof(SHT_pair)), &sht1, sizeof(SHT_block_info));
      BF_Block_SetDirty(block1); 
      BF_UnpinBlock(block1);

      //link the pointers
      sht->next = data1;
      memcpy(data + (Max_Pairs * sizeof(SHT_pair)), sht, sizeof(SHT_block_info));
      BF_Block_SetDirty(block); 
      BF_UnpinBlock(block);
    }
    return sht->blockid;
  }
  else //Block of bucket is full
  {
    //We need to find one that is not full
    void *data1;
    BF_Block *block1;   //Temp block for the search, will store the bucket where we are going to add
    BF_Block_Init(&block1);

    SHT_block_info *sht1;   //The ht_block_info for the new block
    SHT_block_info *next;   //pointer for the search
    next_block = sht->next;

    while (num_pair >= Max_Pairs)
    {
      //We need to find the ht_block_info of the block
      next = next_block + Max_Pairs * sizeof(SHT_pair);
      BF_GetBlock(sfd1, next->blockid, block1);
      data1 = BF_Block_GetData(block1);
      num_pair = next->NumPairs;
      next_block = next->next;  //Link the pointers
    }

    //data1 now points to the block we want to add the record, which is block1.

    printf("SHT_InsertEntry: Name %s hashes to bucket %d .Writing into the available connected block: %d.\n", pair.name, bucket, next->blockid);
    memcpy(data1 + num_pair * sizeof(SHT_pair), &pair, sizeof(SHT_pair));
    next->NumPairs = next->NumPairs + 1;
    memcpy(data1 + (Max_Pairs * sizeof(SHT_pair)), next, sizeof(SHT_block_info));
    BF_Block_SetDirty(block1);
    BF_UnpinBlock(block1);

    sht1 = data1 + Max_Pairs * sizeof(SHT_pair); //We store the HT_block_info of the block in which we just added the record
    num_pair = next->NumPairs;

    printf(" The block now contains : %d", num_pair);
    for (int i = 0; i < num_pair; ++i)
    {
      memcpy(&p, data1 + i * sizeof(SHT_pair), sizeof(SHT_pair));
      printf(",%s", p.name);
    }
    printf("\n");

    //Now we check if the block is full, in order to create a new one
    if (num_pair == Max_Pairs)
    {
      void *data2;
      BF_Block *block2;     //Initializing the "new" last block
      BF_Block_Init(&block2);
      int result = BF_AllocateBlock(sfd1, block2);
      data2 = BF_Block_GetData(block2);

      int c;
      BF_GetBlockCounter(sfd1, &c);
      c--;
      printf("Linking bucket %d to block %d.\n", bucket, c);

      //create a new hp_block_info
      SHT_block_info sht2;
      sht2.blockid = c;
      sht2.next = NULL;
      sht2.NumPairs = 0;

      //Store the hp_block_info at the end of data
      memcpy(data2 + (Max_Pairs * sizeof(SHT_pair)), &sht2, sizeof(SHT_block_info));

      //We link the pointers of the "old" last block to the "new last block"
      sht1->next = data2;
      memcpy(data1 + (Max_Pairs * sizeof(SHT_pair)), sht1, sizeof(SHT_block_info));
      BF_Block_SetDirty(block2); //For the "new "last block
      BF_UnpinBlock(block2);

      BF_Block_SetDirty(block1); //For the "old"last block
      BF_UnpinBlock(block1);
    }
    return sht1->blockid;
  }
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id)
{
  //We need to find which is the key of the hash table
  int hasharg = 0;
  SHT_pair pair;

  if (sht_info->keyAttribute == NAME)
  {
    char clone[15];
    strcpy(clone, record.name);
    strcpy(pair.name , record.name);
    for (int i = 0; i < strlen(record.name); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (sht_info->keyAttribute == SURNAME)
  {
    char clone[20];
    strcpy(clone, record.surname);
    strcpy(pair.name , record.surname);
    for (int i = 0; i < strlen(record.surname); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (sht_info->keyAttribute == CITY)
  {
    char clone[20];
    strcpy(clone, record.city);
    strcpy(pair.name , record.city);
    for (int i = 0; i < strlen(record.city); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }

  int bucket = HashFunction(hasharg, sht_info->numBuckets); //We found the correct bucket
  pair.in_block = block_id;
  return insert_to_bucket2(sht_info, pair, bucket);
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name)
{
  int block_num;
  int i;
  int hasharg = 0;
  int found = 0;
  int read_blocks = 0;
  int entries;
  SHT_block_info *next;
  void *data;
  void *next_block;
  SHT_pair *p;
  int block_is_in;
  p = malloc(sizeof(SHT_pair));
  char a[15];
  
  if (sht_info->keyAttribute == NAME)
  {
    char clone[15];
    strcpy(clone, name);
    for (int i = 0; i < strlen(name); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (sht_info->keyAttribute == SURNAME)
  {
    char clone[20];
    strcpy(clone, (char *)name);
    for (int i = 0; i < strlen((char *)name); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }
  else if (sht_info->keyAttribute == CITY)
  {
    char clone[20];
    strcpy(clone, (char *)name);
    for (int i = 0; i < strlen((char *)name); i++)
    {
      hasharg = hasharg + (int)clone[i];
    }
  }

  block_num = HashFunction(hasharg, sht_info->numBuckets) + 1;

  BF_Block *block;
  BF_Block_Init(&block);
  while (block_num >= 0) //If the bucket points to at least one more block, we need to check those blocks aswell
  {
    (BF_GetBlock(sht_info->fileDesc, block_num, block));
    data = BF_Block_GetData(block); //First block of the bucket

    read_blocks++;
    SHT_block_info *sht;
    sht = data + Max_Pairs * sizeof(SHT_pair);
    entries = sht->NumPairs;
    next_block = sht->next; //Link the pointers

    //for each block, cheack all the records that it has
    for (i = 0; i < entries; i++) 
    {
      memcpy(p, data + i * sizeof(SHT_pair), sizeof(SHT_pair));
      
      /*if (!strcmp(p->name, name))
      {
        if (found == 0)
        {
          printf("Record found: ");
        }
        printf("%d %s ,   \n", p->in_block, p->name);
        found = 1;
      }*/
      
      //We compare value to the correct attribute
      if (sht_info->keyAttribute == NAME)
      {
        if (!strcmp(p->name, name))
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s ,   \n", p->in_block, p->name);
          block_is_in=p->in_block;
          found = 1;
        }
      }
      else if (sht_info->keyAttribute == SURNAME)
      {
        if (!strcmp(p->name, name))
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s ,   \n", p->in_block, p->name);
          block_is_in=p->in_block;
          found = 1;
        }
      }
      else if (sht_info->keyAttribute == CITY)
      {
        if (!strcmp(p->name, name))
        {
          if (found == 0)
          {
            printf("Record found: ");
          }
          printf("%d %s ,   \n", p->in_block, p->name);
          block_is_in=p->in_block;
          found = 1;
        }
      }
    }

    //Go to the next block
    if (next_block+ Max_Pairs * sizeof(SHT_pair)!=NULL) //If the next is NULL, this is the last count_overflow block, so we stop
    {
      break;
    }
    next = next_block + Max_Pairs * sizeof(SHT_pair);
    BF_GetBlock(ht_info->fileDesc, next->blockid, block);
    data = BF_Block_GetData(block);
    block_num = next->blockid;
  }

  //We need to find the record in the primary hash
  BF_GetBlock(ht_info->fileDesc, block_is_in +1 , block);
  void *hash_data = BF_Block_GetData(block);
  HT_block_info* ht1;
  ht1 = hash_data + Max_Records* sizeof(Record);
  int num_rec= ht1->NumRecords;
  for (i=0; i<num_rec ;i++)
  {
    if (strcmp(((Record*)hash_data)->name,p->name)==0){
    printf("(%s, %s, %s) \n",((Record*)hash_data)->name,((Record*)hash_data)->surname,((Record*)hash_data)->city );
    break;
   }
   hash_data=hash_data + i*sizeof(Record);
  }
  
  printf("%d Blocks read in total\n", read_blocks);
  if (found == 0)
  {
    printf("No records with the given value found.\n");
    return -1;
  }
  return read_blocks;

}

int SHT_HashStatistics( char* fileName)
{
  //How many blocks are in this file
  SHT_info *info;
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
  SHT_block_info *ht;
  SHT_block_info *next;
  
  for (int i=0; i<buckets; i++) //Check every bucket
  {
    //First block of the bucket
    (BF_GetBlock(fd1, i+1, block1));
    data1 = BF_Block_GetData(block1);
    ht = data1 + Max_Pairs * sizeof(SHT_pair);
    count[i] = ht->NumPairs;
    block_num = ht->blockid;
    read_blocks[i] =1;

    //We do the first run of the loop, outside
    next_block = ht->next;
    next = next_block + Max_Pairs * sizeof(SHT_pair);
    block_num = next->blockid;
    next_block = next->next;
    read_blocks[i]++;
    if (next_block+ Max_Pairs * sizeof(SHT_pair)!=NULL) //If the next is NULL,does not have an count_overflow block, so we stop
    {
      block_num = -1;
    }
    count_overflow++;
    OverBucket[i]=1;
    while(block_num >= 0)
    {
      count[i] = count[i] + next->NumPairs;
      if (next->next == NULL) //If the next is NULL, this is the last count_overflow block, so we stop
      {
        break;
      }
      (BF_GetBlock(fd1, next->blockid, block1));
      data1 = BF_Block_GetData(block1); //First block of the bucket
      next = next_block + Max_Pairs * sizeof(SHT_pair);
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

