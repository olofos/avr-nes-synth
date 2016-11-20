#include <stdlib.h>
#include <inttypes.h>

#include "config.h"
#include "fat32.h"
#include "sd.h"

#include "log.h"

#define CLUSTER_CACHE_SIZE 8
#define BYTES_PER_SECTOR   512

#define END_CLUSTER_MASK 0x0FFFFFF8

struct fat32_t {
    uint32_t fat_start;
    uint32_t data_start;
    uint32_t root_dir_cluster;
    uint8_t  sectors_per_cluster;

    uint8_t  sector_in_cluster;
    uint32_t current_cluster;

    uint32_t cluster_cache[CLUSTER_CACHE_SIZE];
};

struct fat32_t fat32_data;

struct fat32_file_t
{
    uint32_t bytes_left;
    uint32_t cluster;
    uint32_t size;

    uint8_t cluster_num;
};

struct fat32_file_t fat32_file;


static uint32_t fat32_read_partition_table()
{
    sd_begin_sector(0);
    sd_skip_bytes(0x1BE); // Skip to first partition entry

    uint8_t i;
    uint32_t partition_start;
        
    for(i = 0; i < 4; i++)
    {
        sd_skip_bytes(4); // Status + CHS address of first absolute sector 
        uint8_t partition_type = sd_read_uint8();
        sd_skip_bytes(3); // CHS address of last absolute sector 
        partition_start = sd_read_uint32();
        uint32_t length_sectors = sd_read_uint32();

        log_puts_P(PSTR("Partition "));
        log_put_uint8(i);
        log_puts_P(PSTR(":\n Type:   0x"));
        log_put_uint8_hex(partition_type);
        log_puts_P(PSTR("\n Start:  0x"));
        log_put_uint32_hex(partition_start);
        log_puts_P(PSTR("\n Length: 0x"));
        log_put_uint32_hex(length_sectors);
        log_puts_P(PSTR("\n\n"));

        if(partition_type == 0x0B || partition_type == 0x1B) // FAT32 with CHS addressing
        {
            break;
        }
    }

    if(i < 3)
    {
        sd_skip_bytes(16 * (3-i));
    }

    if(i > 3)
    {
        log_puts_P(PSTR("No FAT32 partition found\n"));
        return 0;
    } else {
        log_puts_P(PSTR("FAT32 filesystem found from partition "));
        log_put_uint8(i);
        log_puts_P(PSTR("\n"));
    }

    uint16_t ending = sd_read_uint16();

    sd_end_sector();

    if(ending != 0xAA55)
    {
        log_puts_P(PSTR("Expected 0xAA55 found 0x"));
        log_put_uint16_hex(ending);
        log_puts_P(PSTR("\n"));
        return 0;
    }

    return partition_start;
}

// http://www.easeus.com/resource/fat32-disk-structure.htm
// https://www.pjrc.com/tech/8051/ide/fat32.html
static void fat32_read_boot_sector(uint32_t partition_start)
{
    sd_begin_sector(partition_start);

    sd_skip_bytes(11); // Jump instruction + OEM name
    uint16_t sector_size = sd_read_uint16();
    fat32_data.sectors_per_cluster = sd_read_uint8();
    uint16_t reserved_sectors = sd_read_uint16();
    uint8_t number_of_fats = sd_read_uint8();
    sd_skip_bytes(19);
    
    uint32_t fat_size = sd_read_uint32(); // sectors per FAT
    sd_skip_bytes(4);
    fat32_data.root_dir_cluster = sd_read_uint32();

    sd_skip_bytes(0x1CE);

    uint16_t boot_sector_signature = sd_read_uint16();

    sd_end_sector();

    fat32_data.fat_start = partition_start + reserved_sectors;
    fat32_data.data_start = partition_start + reserved_sectors + number_of_fats * fat_size;

    if(sector_size != BYTES_PER_SECTOR)
    {
        log_puts_P(PSTR("Warning: expected sector size 512 got "));
        log_put_uint16(sector_size);
        log_puts_P(PSTR("\n"));
    }

    log_puts_P(PSTR("sector_size: "));
    log_put_uint16(sector_size);
    
    log_puts_P(PSTR("\nsectors_per_cluster: 0x"));
    log_put_uint8_hex(fat32_data.sectors_per_cluster);
    
    log_puts_P(PSTR("\nreserved_sectors: 0x"));
    log_put_uint16_hex(reserved_sectors);
    
    log_puts_P(PSTR("\nnumber_of_fats: 0x"));
    log_put_uint8_hex(number_of_fats);
    
    log_puts_P(PSTR("\nfat_size: 0x"));
    log_put_uint32_hex(fat_size);
    
    log_puts_P(PSTR("\nroot_dir_cluster: 0x"));
    log_put_uint32_hex(fat32_data.root_dir_cluster);
    
    log_puts_P(PSTR("\nboot_sector_signature: 0x"));
    log_put_uint16_hex(boot_sector_signature);

    log_puts_P(PSTR("\n"));

    log_puts_P(PSTR("\nFAT1 sector: 0x"));
    log_put_uint32_hex(fat32_data.fat_start);
    
    log_puts_P(PSTR("\nData block starts at: 0x"));
    log_put_uint32_hex(fat32_data.data_start);

    log_puts_P(PSTR("\n\n"));

}

static inline uint32_t fat32_get_sector(const uint32_t cluster, uint16_t sector)
{
    return fat32_data.data_start + (cluster - 2) * fat32_data.sectors_per_cluster + sector;
}


static void fat32_open_cluster(uint32_t cluster)
{
    sd_begin_sector(fat32_get_sector(cluster, 0));
    fat32_data.sector_in_cluster = 0;
    fat32_data.current_cluster = cluster;
}

void fat32_open_root_dir()
{
    fat32_open_cluster(fat32_data.root_dir_cluster);
    fat32_file.bytes_left = 0x0FFFFFFF;
}


void fat32_init()
{
    uint32_t partition_start = fat32_read_partition_table();
    log_puts("\n");
    fat32_read_boot_sector(partition_start);
}

static uint32_t fat32_get_next_cluster_from_fat(const uint32_t cluster)
{
    uint32_t address = fat32_data.fat_start * BYTES_PER_SECTOR + cluster * 4;
    sd_begin_sector(address / BYTES_PER_SECTOR);
    sd_skip_bytes((address) & 0x1ff);
    uint32_t next_cluster = sd_read_uint32();
    sd_end_sector();
    
//    log_putc('\n');
//    log_puts_P(PSTR("Current cluster: 0x"));
//    log_put_uint32_hex(cluster);
//    log_puts_P(PSTR("\nFinding next cluster at: 0x"));
//    log_put_uint32_hex(address);
//    log_puts_P(PSTR(" (sector "));
//    log_put_uint32_hex(address / BYTES_PER_SECTOR);
//    log_puts_P(PSTR(", skip 0x"));
//    log_put_uint16_hex(address & 0x1ff);
//    log_puts_P(PSTR(")\n"));
//    log_puts_P(PSTR("Next cluster: 0x"));
//    log_put_uint32_hex(next_cluster);
//    log_putc('\n');

    return next_cluster;
}

static uint32_t fat32_next_cluster()
{
    if(fat32_file.cluster_num < CLUSTER_CACHE_SIZE )
    {
        return fat32_data.cluster_cache[fat32_file.cluster_num++];
    } else {
        return fat32_get_next_cluster_from_fat(fat32_data.current_cluster);
    }
}

uint16_t fat32_read(void *sd_buf, uint16_t len)
{
    uint8_t *buf = (uint8_t*) sd_buf;
    uint16_t read = 0;
    while((read < len) && fat32_file.bytes_left)
    {
        if(sd_sector_done())
        {
            sd_end_sector();

            fat32_data.sector_in_cluster++;
            
            if(fat32_data.sector_in_cluster == fat32_data.sectors_per_cluster)
            {
                uint32_t next_cluster = fat32_next_cluster();
                
                if((next_cluster & END_CLUSTER_MASK) == END_CLUSTER_MASK)
                {
                    fat32_data.sector_in_cluster--;
                    break;
                }
                
                fat32_data.sector_in_cluster = 0;
                fat32_data.current_cluster = next_cluster;
            }

            sd_begin_sector(fat32_get_sector(fat32_data.current_cluster, fat32_data.sector_in_cluster));
        }

        if(sd_buf)
        {
            buf[read] = sd_read_uint8();
        } else {
            sd_read_uint8();
        }
        fat32_file.bytes_left--;
        read++;
    }
    return read;
}


static void fat32_create_cluster_cache(uint32_t cluster)
{
    fat32_data.cluster_cache[0] = fat32_get_next_cluster_from_fat(cluster);

    log_puts("Creating cluster cache:\n");

    log_puts(" 0x");
    log_put_uint32_hex(cluster);
    log_puts("\n");

    log_puts(" 0x");
    log_put_uint32_hex(fat32_data.cluster_cache[0]);
    log_puts("\n");
        

    for(uint8_t i = 1; i < CLUSTER_CACHE_SIZE; i++)
    {
        fat32_data.cluster_cache[i] = fat32_get_next_cluster_from_fat(fat32_data.cluster_cache[i-1]);

        log_puts(" 0x");
        log_put_uint32_hex(fat32_data.cluster_cache[i]);
        log_puts("\n");

        if((fat32_data.cluster_cache[i] & END_CLUSTER_MASK) == END_CLUSTER_MASK)
        {
            break;
        }
    }
    log_puts("Done.\n");
}

static uint8_t fat32_get_file_data(const char *filename, struct fat32_file_t *file_data)
{
    char dir_filename[11];
    uint8_t attrib;
    uint16_t cluster_hi;
    uint16_t cluster_lo;
    uint32_t file_size;

    uint16_t i = 0;
    uint8_t match;

    do {
        fat32_read(dir_filename, 11);
        fat32_read(&attrib, 1);
        fat32_read(NULL, 8);
        fat32_read(&cluster_hi, 2);
        fat32_read(NULL, 4);
        fat32_read(&cluster_lo, 2);
        fat32_read(&file_size, 4);

        match = 1;

        for(uint8_t j = 0; j < 11; j++)
        {
            if(dir_filename[j] != filename[j])
            {
                match = 0;
                break;
            }
        }

        if(match)
        {
            log_puts_P(PSTR("Filename "));
            log_puts(filename);
            log_puts_P(PSTR(" matches file "));
            log_put_uint16(i);
            log_puts_P(PSTR("\n"));
            break;
        }

        i++;
    } while(dir_filename[0]);

    sd_end_sector();

    if(!match)
    {
        return 0;
    }

    file_data->size = file_size;
    file_data->cluster = ((uint32_t) cluster_hi << 16) | cluster_lo;

    return 1;
}


// Assumes that we are in the beginning of a directory cluster
uint8_t fat32_open_file(const char *filename)
{
    if(!fat32_get_file_data(filename, &fat32_file))
    {
        return 0;
    }

    log_puts_P(PSTR("File cluster: 0x"));
    log_put_uint32_hex(fat32_file.cluster);
    log_putc('\n');

    fat32_create_cluster_cache(fat32_file.cluster);
    
    fat32_open_cluster(fat32_file.cluster);
    
    fat32_file.bytes_left = fat32_file.size;
    fat32_file.cluster_num = 0;
    
    return 1;
}

void fat32_close_file()
{
    fat32_file.bytes_left = 0;
    sd_end_sector();
}



void fat32_seek(uint16_t len)
{
    uint16_t seek_clusters = len / fat32_data.sectors_per_cluster / BYTES_PER_SECTOR;
    uint16_t seek_sectors = (len - seek_clusters * fat32_data.sectors_per_cluster) / BYTES_PER_SECTOR;
    uint16_t seek_bytes = len % BYTES_PER_SECTOR;

    log_puts(  "Seek clusters: ");
    log_put_uint16(seek_clusters);

    log_puts("\nSeek sectors:  ");
    log_put_uint16(seek_sectors);

    log_puts("\nSeek bytes:    ");
    log_put_uint16(seek_bytes);

    log_puts("\n");

    sd_end_sector();

    fat32_data.current_cluster = fat32_file.cluster;
    fat32_file.cluster_num = 0;

    while(seek_clusters--)
    {
        fat32_data.current_cluster = fat32_next_cluster();
    }

//    fat32_data.current_cluster = cluster;

    fat32_data.sector_in_cluster = seek_sectors;

    sd_begin_sector(fat32_get_sector(fat32_data.current_cluster, fat32_data.sector_in_cluster));
    sd_skip_bytes(seek_bytes);

    fat32_file.bytes_left = fat32_file.size - len;
}


void fat32_list_dir()
{
    char filename[12];
    uint8_t attrib;
    uint16_t cluster_hi;
    uint16_t cluster_lo;
    uint32_t filesize;

    uint16_t i = 0;

    do {
        fat32_read(filename, 11);
        filename[11] = 0;
        fat32_read(&attrib, 1);
        fat32_read(NULL, 8);
        fat32_read(&cluster_hi, 2);
        fat32_read(NULL, 4);
        fat32_read(&cluster_lo, 2);
        fat32_read(&filesize, 4);

        uint32_t file_cluster = ((uint32_t)cluster_hi << 16) | cluster_lo;
        if((attrib != 0x0F) && (filename[0] != 0))
        {
            log_puts_P(PSTR("File "));
            log_put_uint16(i);
            log_puts_P(PSTR(": "));
            log_puts(filename);
            log_puts_P(PSTR(" 0x"));
            log_put_uint32_hex(file_cluster);
            log_putc('\n');
        }

        if(sd_sector_done())
        {
            log_puts("Next sector\n");
            sd_end_sector();
            fat32_data.sector_in_cluster++;
            sd_begin_sector(fat32_get_sector(fat32_data.current_cluster, fat32_data.sector_in_cluster));
        }

        i++;
        
    } while(filename[0]);
}
