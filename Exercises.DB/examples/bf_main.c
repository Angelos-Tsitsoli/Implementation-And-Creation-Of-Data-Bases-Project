#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

int main() {
  int fd1;
  BF_Block *block;
  BF_Block_Init(&block);



  /* Πρώτο Μέρος: χρήση της βιβλιοθήκης για block η οποία δημιουργεί ένα
  αρχείο με 10 block, μεταφέρει το καθένα από τα blocks στην ενδιάμεση
  μνήμη και γράφει δύο εγγραφές στην αρχή του κάθε block. Για να
  καταγραφούν οι αλλαγές  στον δίσκο, χρειάζεται να γίνει dirty το block
  και unpin για να μην γεμίσει η ενδιάμεση μνήμη. Μπορούμε να ανοίξουμε
  το αρχείο block_example.db για να δούμε τα περιεχόμενά του.
  */
  CALL_OR_DIE(BF_Init(LRU));
  CALL_OR_DIE(BF_CreateFile("block_example.db"))
  CALL_OR_DIE(BF_OpenFile("block_example.db", &fd1));

  void* data;
  for (int i = 0; i < 10; ++i) {
    CALL_OR_DIE(BF_AllocateBlock(fd1, block));  // Δημιουργία καινούριου block
    data = BF_Block_GetData(block);             // Τα περιεχόμενα του block στην ενδιάμεση μνήμη
    Record* rec = data;                         // Ο δείκτης rec δείχνει στην αρχή της περιοχής μνήμης data
    rec[0] = randomRecord();
    rec[1] = randomRecord();
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  CALL_OR_DIE(BF_CloseFile(fd1));               //Κλείσιμο αρχείου και αποδέσμευση μνήμης
  CALL_OR_DIE(BF_Close());




  /* Δεύτερο Μέρος: χρήση της βιβλιοθήκης για block η οποία διαβάζει
  από κάθε block τα δύο πρώτα records.*/

  CALL_OR_DIE(BF_Init(LRU));
  CALL_OR_DIE(BF_OpenFile("block_example.db", &fd1));
  int blocks_num;
  CALL_OR_DIE(BF_GetBlockCounter(fd1, &blocks_num));

  for (int i = 0; i < blocks_num; ++i) {
    printf("Contents of Block %d\n\t",i);
    CALL_OR_DIE(BF_GetBlock(fd1, i, block));
    data = BF_Block_GetData(block);
    Record* rec= data;
    printRecord(rec[0]);
    printf("\t");
    printRecord(rec[1]);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(fd1));
  CALL_OR_DIE(BF_Close());

}

