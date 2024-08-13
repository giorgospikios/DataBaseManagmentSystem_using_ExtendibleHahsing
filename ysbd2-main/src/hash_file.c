#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "bf.h"
#include "hash_file.h"

#define MAX_OPEN_FILES 20
#define MAX_BUCKETS (BF_BLOCK_SIZE/sizeof(int))

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

struct {
    int open_files;

    struct {
        char fileName[PATH_MAX];
        int file_desc;
        int hash_table[MAX_BUCKETS]; // maps: bucket => block
        int local_depths[MAX_BUCKETS]; // maps: bucket => block
        int num_of_max_records; // max number of records within a block
        int global_depth;
        bool available; // available or not
    } hashTableInfo[MAX_OPEN_FILES];
} openFileIndex;

static void printRecord(Record record) {
    printf("(%d)\n", record.id);
}

// static void printRecord(Record record) {
//     printf("(%d,%s,%s,%s)\n", record.id, record.name, record.surname, record.city);

// }

static int hash(int x, unsigned int depth) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;

    int y = (int) x;
    if (y < 0) {
        y = -y;
    }

    y = y >> (sizeof (int)*8 - (depth) - 1);

    return y;
}

static int power2(int exp) {
    return 1 << exp;
}

HT_ErrorCode HT_Init() {
    openFileIndex.open_files = 0;

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        openFileIndex.hashTableInfo[i].file_desc = 0;
        openFileIndex.hashTableInfo[i].num_of_max_records = 0;
        openFileIndex.hashTableInfo[i].available = true;

        for (int j = 0; j < MAX_BUCKETS; j++) {
            openFileIndex.hashTableInfo[i].hash_table[j] = 0;
            openFileIndex.hashTableInfo[i].local_depths[j] = 0;
        }
    }

    return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *fileName, int depth) {
    int file_desc;
    int status;
    uint blocks = power2(depth);
    BF_ErrorCode code;
    BF_Block *block = NULL;

    BF_Block_Init(&block);

    //
    // Allocate data file: 2^D blocks
    //
    switch ((code = BF_CreateFile(fileName))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    switch ((code = BF_OpenFile(fileName, &file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    for (uint i = 0; i < blocks; i++) {
        switch ((code = BF_AllocateBlock(file_desc, block))) {
            case BF_OK:
                BF_UnpinBlock(block);
                status = HT_OK;
                break;
            default:
                BF_PrintError(code);
                return HT_ERROR;
        }
    }

    switch ((code = BF_CloseFile(file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // Allocate index file
    // 
    char indexfile[PATH_MAX] = {};
    strcpy(indexfile, fileName);
    strcat(indexfile, ".index");


    switch ((code = BF_CreateFile(indexfile))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    switch ((code = BF_OpenFile(indexfile, &file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // Allocate 1 block for metadata: recods and global depth
    //    
    switch ((code = BF_AllocateBlock(file_desc, block))) {
        case BF_OK:
        {
            char* blocks_data = BF_Block_GetData(block);
            sprintf(blocks_data, "%ld,%d", (BF_BLOCK_SIZE / (sizeof (Record))), depth); // max records per block and global depth
            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);
            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // Allocate 1 block for hash table (fill with zeroes extend 2^D blocks) (bucket to block mapping)
    // 
    switch ((code = BF_AllocateBlock(file_desc, block))) {
        case BF_OK:
        {
            int* blocks_data = (int*) BF_Block_GetData(block);

            for (int i = 0; i < MAX_BUCKETS; i++) {
                if (i < blocks) {
                    blocks_data[i] = i;
                }
            }

            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);
            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // Allocate 1 block for hash table (fill with zeroes) (local depths)
    // 
    switch ((code = BF_AllocateBlock(file_desc, block))) {
        case BF_OK:
        {
            int* blocks_data = (int*) BF_Block_GetData(block);

            for (int i = 0; i < MAX_BUCKETS; i++) {
                if (i < blocks) {
                    blocks_data[i] = depth;
                }
            }

            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);
            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    switch ((code = BF_CloseFile(file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    BF_Block_Destroy(&block);

    printf("[INFO]: Database created: file: '%s', index: '%s', capacity: %ld, global depth :%d, initial blocks: %d, max buckets: %ld \n", fileName, indexfile, (BF_BLOCK_SIZE / (sizeof (Record))), depth, blocks, MAX_BUCKETS);

    return status;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc) {
    BF_ErrorCode code;
    BF_Block *block = NULL;
    BF_Block_Init(&block);
    int status;

    for (*indexDesc = 0; *indexDesc < MAX_OPEN_FILES; (*indexDesc)++) {
        if (openFileIndex.hashTableInfo[*indexDesc].available == true) {
            break;
        }
    }

    if (*indexDesc >= MAX_OPEN_FILES) {
        perror("[CRITICAL ERROR]: Maximum number of open files reached \n");
        return HT_ERROR;
    } else {
        openFileIndex.hashTableInfo[*indexDesc].available = false;
        strcpy(openFileIndex.hashTableInfo[*indexDesc].fileName, fileName);
    }

    char indexfile[PATH_MAX] = {};
    strcpy(indexfile, fileName);
    strcat(indexfile, ".index");

    printf("[INFO]: Opening new hash file at slot %d, file: %s, index: %s \n", *indexDesc, fileName, indexfile);

    //
    // Open data file
    //
    switch ((code = BF_OpenFile(fileName, &openFileIndex.hashTableInfo[*indexDesc].file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // Open index file, read global depth and hashtable and close (index file is not kept open)
    //
    int temp_file_desc;

    switch ((code = BF_OpenFile(indexfile, &temp_file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    // Read block with metadata
    switch ((code = BF_GetBlock(temp_file_desc, 0, block))) {
        case BF_OK:
        {
            char* blocks_data = BF_Block_GetData(block);
            char * t1 = strtok(blocks_data, ",");
            char * t2 = strtok(NULL, "");

            openFileIndex.hashTableInfo[*indexDesc].num_of_max_records = atoi(t1);
            openFileIndex.hashTableInfo[*indexDesc].global_depth = atoi(t2);

            BF_UnpinBlock(block);
            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    // Read block with hash table
    switch ((code = BF_GetBlock(temp_file_desc, 1, block))) {
        case BF_OK:
        {
            int* blocks_data = (int*) BF_Block_GetData(block);

            for (int i = 0; i < MAX_BUCKETS; i++) {
                openFileIndex.hashTableInfo[*indexDesc].hash_table[i] = blocks_data[i];
            }
            BF_UnpinBlock(block);

            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    // Read block with local depths table
    switch ((code = BF_GetBlock(temp_file_desc, 2, block))) {
        case BF_OK:
        {
            int* blocks_data = (int*) BF_Block_GetData(block);

            for (int i = 0; i < MAX_BUCKETS; i++) {
                openFileIndex.hashTableInfo[*indexDesc].local_depths[i] = blocks_data[i];
            }
            BF_UnpinBlock(block);

            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    switch ((code = BF_CloseFile(temp_file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

//    int num_blocks;
//    BF_GetBlockCounter(openFileIndex.hashTableInfo[*indexDesc].file_desc, &num_blocks);
//
//    printf("[INFO]: Database opened: file: '%s', index: '%s', capacity: %ld, global depth: %d, initial blocks: %d, max buckets: %ld \n", fileName, indexfile, (BF_BLOCK_SIZE / (sizeof (Record))), openFileIndex.hashTableInfo[*indexDesc].global_depth, num_blocks, MAX_BUCKETS);
//
//
//
//    int blocks = power2(openFileIndex.hashTableInfo[*indexDesc].global_depth);
//
//    printf("Global depth: %d (%d blocks):\n", openFileIndex.hashTableInfo[*indexDesc].global_depth, blocks);
//
//    for (int i = 0; i < blocks; i++) {
//        printf(" - Bucket %3d => block %3d, depth: %3d \n", i, openFileIndex.hashTableInfo[*indexDesc].hash_table[i], openFileIndex.hashTableInfo[*indexDesc].local_depths[i]);
//    }

    BF_Block_Destroy(&block);
    
    return status;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {
    int status;
    BF_ErrorCode code;
    BF_Block *block = NULL;
    BF_Block_Init(&block);


    //
    // Close data file
    //
    switch ((code = BF_CloseFile(openFileIndex.hashTableInfo[indexDesc].file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // Close index file (flush changes to disk)
    //
    int temp_file_desc;
    char indexfile[PATH_MAX] = {};
    strcpy(indexfile, openFileIndex.hashTableInfo[indexDesc].fileName);
    strcat(indexfile, ".index");


    switch ((code = BF_OpenFile(indexfile, &temp_file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    // Read block with metadata
    switch ((code = BF_GetBlock(temp_file_desc, 0, block))) {
        case BF_OK:
        {
            char* blocks_data = BF_Block_GetData(block);
            sprintf(blocks_data, "%d,%d", openFileIndex.hashTableInfo[indexDesc].num_of_max_records, openFileIndex.hashTableInfo[indexDesc].global_depth);

            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);
            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    // Read block with hash table
    switch ((code = BF_GetBlock(temp_file_desc, 1, block))) {
        case BF_OK:
        {
            int* blocks_data = (int*) BF_Block_GetData(block);

            for (int i = 0; i < MAX_BUCKETS; i++) {
                blocks_data[i] = openFileIndex.hashTableInfo[indexDesc].hash_table[i];
            }
            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);

            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    // Read block with local depths table
    switch ((code = BF_GetBlock(temp_file_desc, 2, block))) {
        case BF_OK:
        {
            int* blocks_data = (int*) BF_Block_GetData(block);

            for (int i = 0; i < MAX_BUCKETS; i++) {
                blocks_data[i] = openFileIndex.hashTableInfo[indexDesc].local_depths[i];
            }
            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);

            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    switch ((code = BF_CloseFile(temp_file_desc))) {
        case BF_OK:
            status = HT_OK;
            break;
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }


    //
    // Clean up
    //
    openFileIndex.hashTableInfo[indexDesc].available = true;

    BF_Block_Destroy(&block);

    return status;
}

static void extend_hashtable(int indexDesc) {
    openFileIndex.hashTableInfo[indexDesc].global_depth++;

    int buckets = power2(openFileIndex.hashTableInfo[indexDesc].global_depth);
    int temp[MAX_BUCKETS] = {};

    memcpy(temp, openFileIndex.hashTableInfo[indexDesc].hash_table, sizeof (temp));

    for (int i = 0; i < buckets; i++) {
        openFileIndex.hashTableInfo[indexDesc].hash_table[i] = temp[i / 2];
    }

    memcpy(temp, openFileIndex.hashTableInfo[indexDesc].local_depths, sizeof (temp));

    for (int i = 0; i < buckets; i++) {
        openFileIndex.hashTableInfo[indexDesc].local_depths[i] = temp[i / 2];
    }
}

// HT_ErrorCode HT_PrintHashTable(int indexDesc, Record record)
// {
//     BF_ErrorCode code;
//     BF_Block *block = NULL;
//     BF_Block_Init(&block);
//     int status = HT_OK;

//     int file_desc = openFileIndex.hashTableInfo[indexDesc].file_desc;
//     int bucket = hash((unsigned int) record.id, openFileIndex.hashTableInfo[indexDesc].global_depth) % MAX_BUCKETS;

//     int block_num = openFileIndex.hashTableInfo[indexDesc].hash_table[bucket];
//     int num_of_max_records = openFileIndex.hashTableInfo[indexDesc].num_of_max_records;

//     // 
//     // Try to insert if the block has space
//     // 
//     switch ((code = BF_GetBlock(file_desc, block_num, block))) {
//         case BF_OK:
//         {
//             RecordBlock* blocks_data = (RecordBlock*) BF_Block_GetData(block);

//             if (blocks_data->num_records < num_of_max_records) {
//                 blocks_data->records[blocks_data->num_records++] = record;

//                 BF_Block_SetDirty(block);

//                 BF_UnpinBlock(block);

//                 BF_Block_Destroy(&block);

//                 return HT_OK;
//             } else {
//                 break;
//             }
//         }
//         default:
//             BF_PrintError(code);
//             return HT_ERROR;
//     }

//     //
//     // If the block is full, we need to check local depth and extend hash table if needed:
//     //
//     openFileIndex.hashTableInfo[indexDesc].local_depths[bucket]++;
    
//     if (openFileIndex.hashTableInfo[indexDesc].local_depths[bucket] > openFileIndex.hashTableInfo[indexDesc].global_depth) {
        
//         printf(" hastable doubled \n");
//         extend_hashtable(indexDesc);
//     }

//     return status;
// }

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
    BF_ErrorCode code;
    BF_Block *block = NULL;
    BF_Block_Init(&block);

    int status;
    int num_blocks;
    int file_desc = openFileIndex.hashTableInfo[indexDesc].file_desc;

    BF_GetBlockCounter(openFileIndex.hashTableInfo[indexDesc].file_desc, &num_blocks);

    if (id == NULL) {
        for (int i = 0; i < num_blocks; i++) {
            switch ((code = BF_GetBlock(file_desc, i, block))) {
                case BF_OK:
                {
                    RecordBlock* blocks_data = (RecordBlock*) BF_Block_GetData(block);

                    for (int j = 0; j < blocks_data->num_records; j++) {
                        printf("Table scan: => ");
                        printRecord(blocks_data->records[j]);
                    }

                    BF_UnpinBlock(block);

                    status = HT_OK;
                    break;
                }
                default:
                    BF_PrintError(code);
                    return HT_ERROR;
            }

        }
    } else {
        int bucket = hash((unsigned int) *id, openFileIndex.hashTableInfo[indexDesc].global_depth) % MAX_BUCKETS;

        int block_num = openFileIndex.hashTableInfo[indexDesc].hash_table[bucket];

        switch ((code = BF_GetBlock(file_desc, block_num, block))) {
            case BF_OK:
            {
                RecordBlock* blocks_data = (RecordBlock*) BF_Block_GetData(block);

                for (int j = 0; j < blocks_data->num_records; j++) {
                    if (blocks_data->records[j].id == *id) {
                        printf("Match for id: %d : => ", *id);
                        printRecord(blocks_data->records[j]);
                    }
                }

                BF_UnpinBlock(block);

                status = HT_OK;
                break;
            }
            default:
                BF_PrintError(code);
                return HT_ERROR;
        }

    }

    BF_Block_Destroy(&block);

    return status;
}

HT_ErrorCode HT_Statistics(const char *fileName) {
    HT_ErrorCode hcode;
    int indexDesc;
    HT_ErrorCode status;
    BF_ErrorCode code;
    BF_Block *block = NULL;
    BF_Block_Init(&block);

    switch (hcode = HT_OpenIndex(fileName, &indexDesc)) {
        case HT_OK:
            status = HT_OK;
            break;
        default:
            return HT_ERROR;
    }

    int file_desc = openFileIndex.hashTableInfo[indexDesc].file_desc;
    int total_buckets = power2(openFileIndex.hashTableInfo[indexDesc].global_depth);

    int rmin = INT_MAX;
    int rmax = INT_MIN;
    float sum = 0;
    int total_blocks = 0;
    BF_GetBlockCounter(file_desc, &total_blocks);

    for (int bucket = 0; bucket < total_buckets; bucket++) {
        int block_num = openFileIndex.hashTableInfo[indexDesc].hash_table[bucket];

        switch ((code = BF_GetBlock(file_desc, block_num, block))) {
            case BF_OK:
            {
                RecordBlock* blocks_data = (RecordBlock*) BF_Block_GetData(block);

                if (blocks_data->num_records < rmin) {
                    rmin = blocks_data->num_records;
                }

                if (blocks_data->num_records > rmax) {
                    rmax = blocks_data->num_records;
                }

                sum = sum + blocks_data->num_records;


                BF_UnpinBlock(block);

                status = HT_OK;
                break;
            }
            default:
                BF_PrintError(code);
                return HT_ERROR;
        }
    }

    switch ((hcode = HT_CloseFile(indexDesc))) {
        case HT_OK:
            status = HT_OK;
            break;
        default:
            return HT_ERROR;
    }

    printf("Statistics: \n");
    printf(" + Data blocks: %d \n", total_blocks);
    printf(" + Min records: %d \n", rmin);
    printf(" + Max records: %d \n", rmax);
    printf(" + Avg records: %.2f \n", sum / (total_buckets));

    BF_Block_Destroy(&block);

    return status;
}

// static void extend_hashtable(int indexDesc) {
//     openFileIndex.hashTableInfo[indexDesc].global_depth++;

//     int buckets = power2(openFileIndex.hashTableInfo[indexDesc].global_depth);
//     int temp[MAX_BUCKETS] = {};

//     memcpy(temp, openFileIndex.hashTableInfo[indexDesc].hash_table, sizeof (temp));

//     for (int i = 0; i < buckets; i++) {
//         openFileIndex.hashTableInfo[indexDesc].hash_table[i] = temp[i / 2];
//     }

//     memcpy(temp, openFileIndex.hashTableInfo[indexDesc].local_depths, sizeof (temp));

//     for (int i = 0; i < buckets; i++) {
//         openFileIndex.hashTableInfo[indexDesc].local_depths[i] = temp[i / 2];
//     }
// }

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
    BF_ErrorCode code;
    BF_Block *block = NULL;
    BF_Block_Init(&block);
    int status = HT_OK;

    int file_desc = openFileIndex.hashTableInfo[indexDesc].file_desc;
    int bucket = hash((unsigned int) record.id, openFileIndex.hashTableInfo[indexDesc].global_depth) % MAX_BUCKETS;

    int block_num = openFileIndex.hashTableInfo[indexDesc].hash_table[bucket];
    int num_of_max_records = openFileIndex.hashTableInfo[indexDesc].num_of_max_records;

    // 
    // Try to insert if the block has space
    // 
    switch ((code = BF_GetBlock(file_desc, block_num, block))) {
        case BF_OK:
        {
            RecordBlock* blocks_data = (RecordBlock*) BF_Block_GetData(block);

            if (blocks_data->num_records < num_of_max_records) {
                blocks_data->records[blocks_data->num_records++] = record;

                BF_Block_SetDirty(block);

                BF_UnpinBlock(block);

                BF_Block_Destroy(&block);

                return HT_OK;
            } else {
                break;
            }
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }

    //
    // If the block is full, we need to check local depth and extend hash table if needed:
    //
    openFileIndex.hashTableInfo[indexDesc].local_depths[bucket]++;
    
    if (openFileIndex.hashTableInfo[indexDesc].local_depths[bucket] > openFileIndex.hashTableInfo[indexDesc].global_depth) {
        printf("[INFO]: Hashtable extension ... at id: %d \n", record.id);

        extend_hashtable(indexDesc);
    }

    //
    // At this point, in all subsequent cases, we need to allocate at least a new block to the physical (data) file
    //
    // We need to update half links that point to the initial block to the newly created block
    //
    BF_Block * empty_block = NULL;

    BF_Block_Init(&empty_block);

    switch ((code = BF_AllocateBlock(file_desc, empty_block))) {
        case BF_OK:
        {
            int total_blocks;
            BF_GetBlockCounter(file_desc, &total_blocks);

            int empty_block_num = total_blocks - 1;

            int local = openFileIndex.hashTableInfo[indexDesc].local_depths[bucket] - 1;
            int global = openFileIndex.hashTableInfo[indexDesc].global_depth;
            int sibling_group_size = power2(global - local); // total number of all siblings
            int siblings = sibling_group_size / 2; // siblings that need to point to the new block

//            printf("[DEBUG]: %d-%d \n", sibling_group_size, siblings);

            for (int i = 0; i < siblings; i++) {
                openFileIndex.hashTableInfo[indexDesc].hash_table[2 * bucket + siblings / 2 + i + 1] = empty_block_num;
            }

            status = HT_OK;
            break;
        }
        default:
            BF_PrintError(code);
            return HT_ERROR;
    }
    
    
    //
    // Now we need to reallocate the records in the previous and the new block. 
    // The previous block number has been doubled after the doubling of the hash table
    //
    RecordBlock* blocks_data_1 = (RecordBlock*) BF_Block_GetData(block);
    RecordBlock* blocks_data_2 = (RecordBlock*) BF_Block_GetData(empty_block);
    
    RecordBlock cache;
    
    memcpy(&cache, blocks_data_1, sizeof(RecordBlock));
    
    blocks_data_1->num_records = 0;
    blocks_data_2->num_records = 0;
    
    for (int i=0;i<cache.num_records;i++) {
        Record r = cache.records[i];
        
        int bucket = hash((unsigned int) r.id, openFileIndex.hashTableInfo[indexDesc].global_depth) % MAX_BUCKETS;
        
        if (bucket % 2 == 0) {
            blocks_data_1->records[blocks_data_1->num_records++] = r;
        } else {
            blocks_data_2->records[blocks_data_2->num_records++] = r;
        }
    }
    
    //
    // Clean up for new block
    //
    
    BF_Block_SetDirty(empty_block);
    BF_UnpinBlock(empty_block);
    BF_Block_Destroy(&empty_block);

    //
    // Clean up for old block
    //
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);

    HT_InsertEntry(indexDesc,record);

    return status;
}
