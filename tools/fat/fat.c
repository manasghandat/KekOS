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
* Used f
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


// BootSector global variable
BootSector g_BootSector;

/*
* This function reads reads boot sector from disk and stores it in global variable
* Parameters:
*   disk: disk image in which you want to store bootsector information
* Return value: boolean
*   returns if the read was sucessful or not
*/
bool readBootSector(FILE *disk){
    return fread(&g_BootSector,sizeof(g_BootSector),1,disk)>0;
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
        fprintf(stderr,"Cannot open boot sector!",argv[1]);
        return -1;
    }

    return 0;
}
