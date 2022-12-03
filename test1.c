#include <stdio.h>

// defining a structure
struct employee {
   int id;
   char name[20];
};

int main()
{
   // initializing a structure
   struct employee e1 = {65, "Employee 1"};

   // declaring a file pointer
   FILE *filePointer;

   // opening the file in write mode
   filePointer = fopen("test.txt", "w");

   // checking if the file opening was unsuccessful
   if(filePointer == NULL) {
      // if yes, printing error message
      printf("Some error occured");
   }
   else {
      // if the file was successfully opened, then writing the above structure into the file
      fwrite(&e1, sizeof(e1), 1, filePointer);
   }

   // closing the file
   fclose(filePointer);

   return 0;
}