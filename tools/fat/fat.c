#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef uint8_t bool;
#define true 1
#define false 0

/*
* Bootsector struct
* It contains info about the boot image
* Used for providing various argument values about disk
*/
typedef struct 
{
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;          // serial number, value doesn't matter
    uint8_t VolumeLabel[11];    // 11 bytes, padded with spaces
    uint8_t SystemId[8];

    // ... we don't care about code ...

} __attribute__((packed)) BootSector;


typedef struct 
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) DirectoryEntry;

// BootSector global variable
BootSector g_BootSector;
uint8_t * g_Fat = NULL;
// Contains root directory and is an array of directory entry
DirectoryEntry* g_RootDirectory = NULL;
uint32_t g_RootDirectoryEnd;

/*
* This function reads reads boot sector from disk and stores it in global variable
* Parameters:
*   disk: disk image in which you want to store bootsector information
* Return value: boolean
*   returns if the read was sucessful or not
*/
bool readBootSector(FILE *disk)
{
    return fread(&g_BootSector,sizeof(g_BootSector),1,disk)>0;
}


/*
* This function reads the sectors from the disk into the buffer
* Parameters:
*   disk      : disk image from which data is read
*   lda       : sector number 
*   count     : number of sectors to read
*   bufferOut : pointer on where to store the data
* Return value: boolean
*   returns if the read was sucessful or not
*/
bool readSectors(FILE *disk, uint32_t lba, uint32_t count, void* bufferOut)
{
    bool ok = true;
    ok = ok && (fseek(disk,lba * g_BootSector.BytesPerSector, SEEK_SET)==0);
    ok = ok && (fread(bufferOut,g_BootSector.BytesPerSector,count,disk)==count);
}


/*
* This function reads file allocation table into memory.
* This is stored in the global variable `g_Fat`
* Parameters:
*   disk      : disk image from which data is read
* Return value: boolean
*   returns if the read was sucessful or not
*/
bool readFat(FILE *disk)
{
    g_Fat = (uint8_t *)malloc(g_BootSector.SectorsPerFat * g_BootSector.BytesPerSector);
    return readSectors(disk,g_BootSector.ReservedSectors,g_BootSector.SectorsPerFat,g_Fat);
}

/*
* This function allocates memory for root directory and reads its content
* Parameters:
*   disk : disk image from which data is read
* Return value: boolean
*   returns if the read was sucessful or not
*/
bool readRootDirectory(FILE *disk)
{
    //Calculation of the begning of sector 
    uint32_t lba = g_BootSector.ReservedSectors + g_BootSector.SectorsPerFat * g_BootSector.FatCount;
    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    uint32_t sectors = (size / g_BootSector.BytesPerSector);
    if (size % g_BootSector.BytesPerSector >0)
    {
        sectors ++;
    }
    g_RootDirectoryEnd = lba + sectors;
    g_RootDirectory = (DirectoryEntry *) malloc(sectors * g_BootSector.BytesPerSector);
    return readSectors(disk,lba,sectors,g_RootDirectory);
}

/*
* This function finds the file in file in the root directory
* Parameters:
*   name : name of the directory
* Return Value: DirectoryEntry*
*   returns a pointer to corresponding directory entry
*   if the pointer is not found return NULL
*/
DirectoryEntry* findFile(const char* name)
{
    for (uint32_t i = 0; i < g_BootSector.DirEntryCount; i++)
    {
        if (memcmp(name, g_RootDirectory[i].Name, 11) == 0)
        {
            return &g_RootDirectory[i];
        }
        
    }
    
}

bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer)
{
    bool ok = true;
    uint16_t currentCluster = fileEntry->FirstClusterLow;
    do
    {
        uint32_t lba = g_RootDirectoryEnd + (currentCluster - 2) * g_BootSector.SectorsPerCluster;
        ok = ok && readSectors(disk,lba,g_BootSector.SectorsPerCluster,outputBuffer);
        outputBuffer += g_BootSector.SectorsPerCluster * g_BootSector.BytesPerSector;

        uint32_t fatIndex = currentCluster * 2/3;
        if (currentCluster % 2 == 0)
        {
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
        }
        else
        {
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
        }
    } while (ok & currentCluster < 0x0FF8);
    
    return ok;
}

int main(int argc, char **argv)
{
    /*
    * Loop to check if the program is called with correct arguments
    * The correct way yo call the program is: 
    * $ ./(Program_name) disk_image file_name
    */
    if (argc < 3)
    {
        printf("Syntax %s <disk image> <file name>\n",argv[0]);
        return -1;
    }

    FILE * disk = fopen(argv[1],"rb");
    // Throw error message
    if(!disk)
    {
        fprintf(stderr,"Cannot open disk image %s!",argv[1]);
        return -1;
    }

    // Throw error message
    if(!readBootSector(disk))
    {
        fprintf(stderr,"Cannot open boot sector!");
        return -2;
    }

    // Throw error message
    if(!readFat(disk))
    {
        fprintf(stderr,"Cannot read FAT file!");
        free(g_Fat);
        return -3;
    }

    // Throw error message
    if(!readRootDirectory(disk))
    {
        fprintf(stderr,"Cannot read FAT file!");
        free(g_Fat);
        free(g_RootDirectory);
        return -4;
    }

    DirectoryEntry* fileEntry = findFile(argv[2]);
    if(!fileEntry)
    {
        fprintf(stderr,"Could not find file %s!",argv[1]);
        free(g_Fat);
        free(g_RootDirectory);
        return -5;
    }

    uint8_t* buffer = (uint8_t*) malloc(fileEntry->Size + g_BootSector.BytesPerSector);
    if(!readFile(fileEntry,disk,buffer))
    {
        fprintf(stderr,"Could not read file %s!",argv[1]);
        free(g_Fat);
        free(g_RootDirectory);
        free(buffer);
        return -6;
    }

    for (size_t i = 0; i < fileEntry->Size; i++)
    {
        if (isprint(buffer[i]))
        {
            fputc(buffer[i],stdout);
        }
        else
        {
            printf("<%02x>",buffer[i]);
        }
    }
    printf("\n");

    free(buffer);
    free(g_Fat);
    free(g_RootDirectory);
    return 0;
}
