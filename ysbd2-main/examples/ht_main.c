#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "hash_file.h"

#define GLOBAL_DEPT 2 // you can change it if you want
#define FILE_NAME "data.db"
#define INDEX_FILE_NAME "data.db.index"

#define RECORDS_NUM 10000 // if multiple of 8 all blocks will be full assuming equal distribution



#define CALL_OR_DIE(call)     \
  {                           \
    HT_ErrorCode code = call; \
    if (code != HT_OK) {      \
      printf("Error\n");      \
      exit(code);             \
    }                         \
  }

int main() {
    const char* names[] = {
        "Yannis",
        "Christofos",
        "Sofia",
        "Marianna",
        "Vagelis",
        "Maria",
        "Iosif",
        "Dionisis",
        "Konstantina",
        "Theofilos",
        "Giorgos",
        "Dimitris"
    };

    const char* surnames[] = {
        "Ioannidis",
        "Svingos",
        "Karvounari",
        "Rezkalla",
        "Nikolopoulos",
        "Berreta",
        "Koronis",
        "Gaitanis",
        "Oikonomou",
        "Mailis",
        "Michas",
        "Halatsis"
    };

    const char* cities[] = {
        "Athens",
        "San Francisco",
        "Los Angeles",
        "Amsterdam",
        "London",
        "New York",
        "Tokyo",
        "Hong Kong",
        "Munich",
        "Miami"
    };

    remove(FILE_NAME);
    remove(INDEX_FILE_NAME);
    
    BF_Init(LRU);

    CALL_OR_DIE(HT_Init());
    
    printf("Physical limits:\n");
    printf(" - Bucket size         : %d \n", BF_BLOCK_SIZE);
    printf(" - Max buckets         : %ld \n", (BF_BLOCK_SIZE/sizeof(int)));
    printf(" - Max records/block   : %ld \n", (BF_BLOCK_SIZE/sizeof(Record)));
    printf(" - Max records         : %ld \n", (BF_BLOCK_SIZE/sizeof(int))*(BF_BLOCK_SIZE/sizeof(Record)));
    printf(" - Size of RecordBlock : %ld \n", (sizeof(RecordBlock)));

    int indexDesc;
    CALL_OR_DIE(HT_CreateIndex(FILE_NAME, GLOBAL_DEPT));
    CALL_OR_DIE(HT_OpenIndex(FILE_NAME, &indexDesc));
    
    srand(12569874);
    int r;
    
    printf("Insert Entries\n");
    
    for (int id = 0; id < RECORDS_NUM; ++id) {
        Record record = {};
        
        // create a record
        record.id = id;
        r = rand() % 12;
        memcpy(record.name, names[r], strlen(names[r]) + 1);
        r = rand() % 12;
        memcpy(record.surname, surnames[r], strlen(surnames[r]) + 1);
        r = rand() % 10;
        memcpy(record.city, cities[r], strlen(cities[r]) + 1);

        CALL_OR_DIE(HT_InsertEntry(indexDesc, record));

        // CALL_OR_DIE(HT_PrintHashTable(indexDesc, record));
        
        CALL_OR_DIE(HT_PrintAllEntries(indexDesc, &id));
    }

    printf("RUN PrintAllEntries\n");
    
    CALL_OR_DIE(HT_PrintAllEntries(indexDesc, NULL));


    CALL_OR_DIE(HT_CloseFile(indexDesc));
    
    CALL_OR_DIE(HT_Statistics(FILE_NAME));
    
    BF_Close();
    
    return 0;
}
