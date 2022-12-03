#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record.h"

int main() 
{
    FILE *fp;
    fp  = fopen ("fileName", "w");
    if (fp == NULL)
    {
        return -1;
    }
    struct Record a={"record", 0 , "name", "surname", "city"};  //id =0 for heap
    fwrite(&a, sizeof(Record), 1 ,fp);
    char* endln = "\n";
    fputs(endln, fp);
   // fputs(a.name, fp);
   // fputs(a.surname, fp);
   // fputs(a.city, fp);
    //*a->name= "mara";
   // printf("1\n");
    a = randomRecord();
    fwrite(&a, sizeof(Record), 1 ,fp);


    if(fwrite != 0)
        printf("contents to file written successfully !\n");
    
    return 0;
}
