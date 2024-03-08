# Creation of the following Structures : Heap, Hash Table, Secondary Hash Table

## Implementation
The implementation of the structures is explained on this pdf.<br>
[ReadMe.pdf](https://github.com/sdi2000200/YSBD-Project/files/12103922/ReadMe.pdf)

## Purpose
The purpose of this paper is to understand the internal workings of Database Systems
Database Systems in terms of block level management and also in terms of block level management.
record level. Also, through the work it will be understood whether it can improve
the performance of a Database Management System (DBMS) by the existence of indexes on the
records.

## Functions

### Heap:
* int HP_CreateFile(char *fileName, /*όνομα αρχείου*/);
*  HP_info* HP_OpenFile( char *fileName /* όνομα αρχείου */ )
*  int HP_CloseFile( HP_info* header_info )
*  int HP_InsertEntry(HP_info* header_info, /* επικεφαλίδα του αρχείου* Record record /* δομή που προσδιορίζει την εγγραφή */ )
*  int HP_GetAllEntries(HP_info* header_info, /* επικεφαλίδα του αρχείου*/int id /* η τιμή id της εγγραφής στην οποία πραγματοποιείται η αναζήτηση*/)

### Hash Table :
* int HT_CreateFile(char *fileName, /* όνομα αρχείου */ int buckets /* αριθμός κάδων κατακερματισμού*/)
* HT_info* HT_OpenFile( char *fileName /* όνομα αρχείου */ )
* int HT_CloseFile(HT_info* header_info )
* int HT_InsertEntry(HT_info* header_info, /* επικεφαλίδα του αρχείου*/ Record record /* δομή που προσδιορίζει την εγγραφή */ )
* int HT_GetAllEntries(HT_info* header_info, /*επικεφαλίδα του αρχείου*/ int id /*τιμή του πεδίου-κλειδιού προς αναζήτηση*/)



## Contributors of the project :
* [Vicky-Christofilopoulou]( https://github.com/Vicky-Christofilopoulou )
* [Angelos-Tsitsoli]( https://github.com/Angelos-Tsitsoli )
