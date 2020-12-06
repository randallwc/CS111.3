#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include "ext2_fs.h"

#define EXT2_SUPER_MAGIC 0xEF53
#define SUPERBLOCK_OFFSET 1024

char * USAGE = "Usage: \" ./lab3a [image]\"\n";

struct ext2_super_block superblock;

int blsize = 0;
int img = -1;

// find full offset based on regular offset
unsigned long foff(unsigned int roff);

// read group
void rdgroup(int ind, int tgroups);

// read block bitmap
void rdbbit(unsigned int ngroup, unsigned int roff, unsigned int tblocks);

// read inode bitmap
void rdibit(unsigned int ngroup, unsigned int roff, unsigned int tblocks, unsigned int tinodes);

// read inode
void rdinode(unsigned int rofftab, unsigned int ninode);

// read indir
void rdindir(unsigned int ninode, unsigned int nblock, unsigned int off, unsigned int level, char ftype);

// read directory entry
void rdentry(unsigned int ninode, unsigned int roff);

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "%s", USAGE);
        exit(1);
    }
    
    img = open(argv[1], O_RDONLY);
    if (img < 0)
    {
        fprintf(stderr, "%s", USAGE);
        exit(1);
    }
    
    if (pread(img, &superblock, sizeof(struct ext2_super_block), SUPERBLOCK_OFFSET) <= 0)
    {
        fprintf(stderr, "pread failed\n");
        exit(2);
    }
    
    
    if (superblock.s_magic != EXT2_SUPER_MAGIC)
    {
        fprintf(stderr, "File system is not ext2\n");
        exit(1);
    }
    
    blsize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",
        superblock.s_blocks_count,  // total # blocks
        superblock.s_inodes_count,  // total # inodes
        blsize,         // block size
        superblock.s_inode_size,    // inode size
        superblock.s_blocks_per_group,  // blocks per group
        superblock.s_inodes_per_group,  // inodes per group
        superblock.s_first_ino);    // first non reserved inode
    
    int tgroups = superblock.s_blocks_count / superblock.s_blocks_per_group + !!((int)(superblock.s_blocks_count) % (int)(superblock.s_blocks_per_group));
    
    int i;
    for (i = 0; i < tgroups; i++)
    {
        rdgroup(i, tgroups);
    }
    
    return 0;
}

unsigned long foff(unsigned int roff)
{
    return SUPERBLOCK_OFFSET + blsize * (roff - 1);
}

void rdgroup(int ind, int total)
{
    unsigned long offset = (blsize == 1024) ? (2048 + 32 * ind) : (blsize + 32 * ind);
    
    struct ext2_group_desc group;
    
    pread(img, &group, sizeof(group), offset);
    
    unsigned int tblocks = (ind == total - 1) ? (superblock.s_blocks_count - (total - 1) * superblock.s_blocks_per_group) : superblock.s_blocks_per_group;
    unsigned int tinodes = (ind == total - 1) ? (superblock.s_inodes_count - (total - 1) * superblock.s_inodes_per_group) : superblock.s_inodes_per_group;
    
    printf("GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",
        ind,                // group num
        tblocks,            // # blocks in group
        tinodes,            // # inodes in group
        group.bg_free_blocks_count, // # free blocks
        group.bg_free_inodes_count, // # free inodes
        group.bg_block_bitmap,      // block bitmap offset
        group.bg_inode_bitmap,      // inode bitmap offset
        group.bg_inode_table);      // first inode block offset
    
    rdbbit(ind, group.bg_block_bitmap, tblocks);
    
    rdibit(ind, group.bg_inode_bitmap, group.bg_inode_table, tinodes);
    
    return;
}

void rdbbit(unsigned int ngroup, unsigned int roff, unsigned int tblocks)
{
    unsigned long off = foff(roff);
    
    unsigned char * bitmap = malloc(blsize);
    
    pread(img, bitmap, blsize, off);
    
    unsigned int nblock = superblock.s_first_data_block + ngroup * superblock.s_blocks_per_group;
    
    unsigned int i, j;
    unsigned char bit;
    for (i = 0; i < tblocks; i++)
    {
        bit = bitmap[i];
        for (j = 0; j < 8; j++)
        {
            if (!(int)(bit & 1))
            {
                printf("BFREE,%d\n", nblock);
            }
            bit >>= 1;
            nblock++;
        }
    }
    
    free(bitmap);
    
    return;
}

void rdibit(unsigned int ngroup, unsigned int roffbit, unsigned int rofftab, unsigned int tinodes)
{
    unsigned int tbytes = tinodes / 8;
    
    unsigned long offbit = foff(roffbit);
    
    unsigned char * bitmap = malloc(blsize);
    
    pread(img, bitmap, blsize, offbit);
    
    unsigned int ninode = ngroup * superblock.s_inodes_per_group + 1;
    
    unsigned int i, j;
    unsigned char bit;
    for (i = 0; i < tbytes; i++)
    {
        bit = bitmap[i];
        for (j = 0; j < 8; j++)
        {
            if (!(int)(bit & 1))
            {
                printf("IFREE,%d\n", ninode);
            }
            else
            {
                rdinode(rofftab, ninode);
            }
            bit >>= 1;
            ninode++;
        }
    }
    
    return;
}

void rdinode(unsigned int rofftab, unsigned int ninode)
{
    unsigned long off = foff(rofftab) + 128 * (ninode - 1);
    
    struct ext2_inode inode;
    
    pread(img, &inode, sizeof(inode), off);
    
    if (inode.i_mode == 0 || inode.i_links_count == 0)
    {
        return;
    }
    
    char ftype = '?';
    switch ((__uint16_t)(inode.i_mode >> 12))
    {
        case 0xa:
            ftype = 's';
            break;
        case 0x8:
            ftype = 'f';
            break;
        case 0x4:
            ftype = 'd';
            break;
    }
    
    char ictime[18], imtime[18], iatime[18];
    ictime[17] = '\0';
    imtime[17] = '\0';
    iatime[17] = '\0';
    time_t ctime = inode.i_ctime;
    time_t mtime = inode.i_mtime;
    time_t atime = inode.i_atime;
    strftime(ictime, 18, "%x %X", gmtime( &ctime ));
    strftime(imtime, 18, "%x %X", gmtime( &mtime ));
    strftime(iatime, 18, "%x %X", gmtime( &atime ));
    
    unsigned int tblocks = 2 * inode.i_blocks / (2 << superblock.s_log_block_size);
    
    printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
        ninode,     // inode number
        ftype,          // file type
        inode.i_mode & 0xFFF,   // last 12 bits of mode
        inode.i_uid,        // user
        inode.i_gid,        // group
        inode.i_links_count,    // link count
        ictime,     // create time
        imtime,     // mod time
        iatime,     // access time
        inode.i_size,       // file size
        tblocks);       // total # blocks
    unsigned int i;
    if (ftype != 's' || inode.i_size > 60)
    {
        for (i = 0; i < 15; i++)
        {
            printf(",%d", inode.i_block[i]);
        }
    }
    printf("\n");
    
    if (ftype == 'd')
    {
        for (i = 0; i < 12; i++)
        {
            if (inode.i_block[i] != 0)
            {
                rdentry(ninode, inode.i_block[i]);
            }
        }
    }
    
    if (inode.i_block[12] != 0)
    {
        rdindir(ninode, inode.i_block[12], 12, 1, ftype);
    }
    
    if (inode.i_block[13] != 0)
    {
        rdindir(ninode, inode.i_block[13], 256 + 12, 2, ftype);
    }
    
    if (inode.i_block[14] != 0)
    {
        rdindir(ninode, inode.i_block[14], 65536 + 256 + 12, 3, ftype);
    }
    
    return;
}

void rdindir(unsigned int ninode, unsigned int nblock, unsigned int off, unsigned int level, char ftype)
{
    
    __uint32_t indlen = blsize / sizeof(__uint32_t);
    
    __uint32_t * indarr = malloc(blsize);
    
    pread(img, indarr, blsize, blsize * nblock);
    
    unsigned int i;
    for (i = 0; i < indlen; i++)
    {
        if (indarr[i] != 0)
        {
            printf("INDIRECT,%d,%d,%d,%d,%d\n",
                ninode,
                level,
                off,
                nblock,
                indarr[i]);
            
            if ((level == 1) & (ftype == 'd'))
            {
                rdentry(ninode, indarr[i]);
            }
            
            if (level > 1)
            {
                rdindir(ninode, indarr[i], off, level - 1, ftype);
            }
        }
        
        switch (level)
        {
            case 1:
                off++;
                break;
            case 2:
                off += 256;
                break;
            case 3:
                off += 65536;
                break;
        }
    }
    
    free(indarr);
}

void rdentry(unsigned int pinode, unsigned int roff)
{
    unsigned long off = foff(roff);
    
    int boff = 0;
    
    struct ext2_dir_entry entry;
    
    while (boff < blsize)
    {
        memset(entry.name, 0, 256);
        pread(img, &entry, sizeof(entry), off + boff);
        if (entry.inode != 0)
        {
            memset(&entry.name[entry.name_len], 0, 256 - entry.name_len);
            printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
                pinode,
                boff,
                entry.inode,
                entry.rec_len,
                entry.name_len,
                entry.name);
        }
        boff += entry.rec_len;
    }   
}
