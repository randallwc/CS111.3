#include <iostream>     // cout
using namespace std;

#include <unistd.h>     // pread
#include <cerrno>       // error
#include <cstring>      // string
#include <ctime>        // time_t
#include <fcntl.h>      // open
#include "ext2_fs.h"    // header file

//GLOBAL STRUCTS
struct ext2_super_block superblock;

//GLOBAL VARS
int img = -1;
__u32 blocksize = 0;

//HELPER FUNCTION DEFINITIONS
/* read in the group and get a Summary */
// void groupSummary(int index, __u32 max);//TODO
void groupSummary(int index);
/* free block bitmap for each group */
void freeBlockBitmap(int index, int offset, __u32 numBlocks);
/* free iNode Bitmap */
void freeiNodeBitmap(int index, int offset, int table, __u32 numBytes);
/* summary of iNode */
void iNodeSummary(int table, int numiNode);
/* read direct block references */
void directoryEntries(int numParentiNode, int offset);
/* read indirect block references */
void indirectBlockReferences(int numiNode, int blockNumber, int offset, int depth, char type);
/* make a time string buffer */
void formatTime(__u32 time, char* timeStr);

int main(int argc, char** argv)
{
    // check that there is only one argument
    if (argc != 2)
        fprintf(stderr, "Error: Incorrect usage. Expected: ./lab3a [image]\n");

    // open img in read only
    img = open(argv[1], O_RDONLY);
    if (img == -1)
    {
        fprintf(stderr, "Error: Unable to open the specified file");
        exit(1);
    }
    
    // read in the superblock
    if (pread(img, &superblock, sizeof(struct ext2_super_block), 1024) != sizeof(struct ext2_super_block))
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for superblock\n");
        exit(1);
    }

    // check if the file system is EXT2
    if (superblock.s_magic != EXT2_SUPER_MAGIC)
    {
        fprintf(stderr, "Error: File system is not EXT2\n");
        exit(1);
    }

    // get blocksize from the superblock -- superblock is constant 1024
    blocksize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    
    // superblock summary
    cout << "SUPERBLOCK," 
         << superblock.s_blocks_count << ','     /* total number of blocks (decimal) */
         << superblock.s_inodes_count << ','     /* total number of i-nodes (decimal) */
         << blocksize << ','                     /* block size (in bytes, decimal) */
         << superblock.s_inode_size << ','       /* i-node size (in bytes, decimal) */
         << superblock.s_blocks_per_group << ',' /* blocks per group (decimal) */
         << superblock.s_inodes_per_group << ',' /* i-nodes per group (decimal) */
         << superblock.s_first_ino << endl;      /* first non-reserved i-node (decimal) */

    /* FOR LAB3b
    // get number of groups for groupSummary
    __u32 numGroups = superblock.s_blocks_count / superblock.s_blocks_per_group;
    numGroups += !! (((int) superblock.s_blocks_count) % ((int) superblock.s_blocks_per_group));

    for (__u32 i = 0; i < numGroups; i++)
        groupSummary(i);
    */

    // just for lab3a
    groupSummary(0);

    return 0;
}

void groupSummary(int index)
{
    // set up the blocks group descriptor struct
    struct ext2_group_desc group;

    // get the offset
    __u32 offset = (superblock.s_first_data_block + 1) * blocksize;

    __u32 numBlocks, numiNodes;

    // read into the group struct
    if (pread(img, &group, sizeof(struct ext2_group_desc), offset) != sizeof(struct ext2_group_desc))
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for group\n");
        exit(1);
    }
    
    /* check dec of numBlocks and numiNodes */
    // get the number of blocks
    if(superblock.s_blocks_count >= superblock.s_blocks_per_group){
        numBlocks = superblock.s_blocks_count;
    }
    else{
        numBlocks = superblock.s_blocks_count % superblock.s_blocks_per_group;
    }

    // get the number of inodes 
    if(superblock.s_inodes_count >= superblock.s_inodes_per_group){
        numiNodes = superblock.s_inodes_count;
    }
    else{
        numiNodes = superblock.s_inodes_count % superblock.s_inodes_per_group;
    }
    
    cout << "GROUP,"
         << "0,"                                  /* group number (decimal, starting from zero) -- only one group for 3a */
         << numBlocks << ','                      /* total number of blocks in this group (decimal) */
         << numiNodes << ','                      /* total number of i-nodes in this group (decimal) */
         << superblock.s_free_blocks_count << ',' /* number of free blocks (decimal) */
         << superblock.s_free_inodes_count << ',' /* number of free i-nodes (decimal) */
         << group.bg_block_bitmap << ','          /* block number of free block bitmap for this group (decimal) */
         << group.bg_inode_bitmap << ','          /* block number of free i-node bitmap for this group (decimal) */
         << group.bg_inode_table << endl;         /* block number of first block of i-nodes in this group (decimal) */

    freeBlockBitmap(index, group.bg_block_bitmap, numBlocks);

    freeiNodeBitmap(index, group.bg_inode_bitmap, group.bg_inode_table, numiNodes / 8);

    return;
}

void freeBlockBitmap(int index, int offset, __u32 numBlocks)
{
    offset = 1024 + blocksize * (offset - 1);
    char* blockBitmap = (char *)malloc(blocksize); // TODO -- use new

    // TODO -- error check
    if (pread(img, blockBitmap, blocksize, offset) != blocksize)
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for block bitmap\n");
        exit(1);
    }
    
    int blockNum = superblock.s_first_data_block + index * superblock.s_blocks_per_group;
    int mask = 1;
    char bit;
    for (__u32 i = 0; i < numBlocks; i++)
    {
        bit = blockBitmap[i];
        for (__u8 j = 0; j < 8; j++)
        {
            if (!(bit & mask))
                cout << "BFREE," << blockNum << endl; // number of the free block (decimal)
            
            // shift bit and increment blockNum
            bit = bit >> 1;
            blockNum++;
        }
    }

    free(blockBitmap); // TODO -- use delete
    return;
}

void freeiNodeBitmap(int index, int offset, int table, __u32 numBytes)
{
    // get offset
    offset = 1024 + blocksize * (offset - 1);

    // set iNode bitmap
    unsigned char* iNodeBitmap = (unsigned char*)malloc(blocksize);//TODO -- use new

    // TODO -- error check
    // read into the iNode bitmap
    if (pread(img, iNodeBitmap, blocksize, offset) != blocksize)
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for inode bitmap\n");
        exit(1);
    }

    //  get number of inodes
    __u32 iNodeNum = superblock.s_inodes_per_group * index;
    iNodeNum += 1;

    // set up bitmask
    unsigned char bit, mask;
    mask = 1;

    // loop until the number of bytes
    for (__u32 i = 0; i < numBytes; i++)
    {
        bit = iNodeBitmap[i];

        // loop through the bitmask
        for (__u8 j = 0; j < 8; j++)
        {
            if (!(bit & mask))
                printf("IFREE,%d\n", iNodeNum); // number of the free I-node (decimal)
            else
                iNodeSummary(table, iNodeNum);

            // shift bit and increment iNodeNum
            bit = bit >> 1;
            iNodeNum++;
        }
    }

    free(iNodeBitmap);//TODO -- use delete
}

void iNodeSummary(int table, int numiNode)
{
    // get offset
    __u32 offset = 1024 + blocksize * (table - 1) + 128 * (numiNode - 1);

    struct ext2_inode iNode;

    // read into iNode struct
    if (pread(img, &iNode, sizeof(struct ext2_inode), offset) != sizeof(struct ext2_inode))
    {
        fprintf(stderr, "Error: Encountered an issue on call to pread() for inode\n");
        exit(1);
    }

    // if mode is 0 then no summary
    if(iNode.i_mode == 0){
        return;
    }

    // if no links then no summary
    if(iNode.i_links_count == 0){
        return;
    }

    char type = '?';
    unsigned int mode = (__uint16_t)(iNode.i_mode >> 12);
    
    if (mode == 0xa)
        type = 's'; // symbolic link
    
    else if (mode == 0x8)
        type = 'f'; // file
    
    else if (mode == 0x4)
        type = 'd'; // directory

    // format the times
    char ctime[18], mtime[18], atime[20];
    formatTime(iNode.i_ctime, ctime); // time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
    formatTime(iNode.i_mtime, mtime); // modification time (mm/dd/yy hh:mm:ss, GMT)
    formatTime(iNode.i_atime, atime); // time of last access (mm/dd/yy hh:mm:ss, GMT)

    // 2 * blocks count / (2 << log2(Block size))
    int numBlocks = 2 * iNode.i_blocks / (2 << superblock.s_log_block_size);

    printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d",
        numiNode,             /* inode number (decimal) */
        type,                 /* file type ('f' for file, 'd' for directory, 's' for symbolic link, '?' for anything else) */
        iNode.i_mode & 0xFFF, /* mode (low order 12-bits, octal ... suggested format "%o") */
        iNode.i_uid,          /* owner (decimal) */
        iNode.i_gid,          /* group (decimal) */
        iNode.i_links_count,  /* link count (decimal) */
        ctime,                /* time of last I-node change (mm/dd/yy hh:mm:ss, GMT) */
        mtime,                /* modification time (mm/dd/yy hh:mm:ss, GMT) */
        atime,                /* time of last access (mm/dd/yy hh:mm:ss, GMT) */
        iNode.i_size,         /* file size (decimal) */
        numBlocks);           /* number of (512 byte) blocks of disk space (decimal) taken up by this file */

    // if the file is a symbolic link
    // print symbolic links
    // file will contain zero data blocks if the file size is greater than 60
    if (type != 's' || iNode.i_size > 60)
        for (int i = 0; i < 15; i++)
            printf(",%d", iNode.i_block[i]);
    printf("\n");

    // if the file is a directory go into directory entries
    if (type == 'd')
        for (int i = 0; i < EXT2_NDIR_BLOCKS; i++)
            if (iNode.i_block[i] != 0) // recursive
                directoryEntries(numiNode, iNode.i_block[i]); // input parent inode and offset

    int block_offset = 12;
    int depth = 1;
    if (iNode.i_block[EXT2_IND_BLOCK] != 0)
        indirectBlockReferences(numiNode, iNode.i_block[EXT2_IND_BLOCK], block_offset, depth, type);

    block_offset += 256;
    depth++;// 2
    if (iNode.i_block[EXT2_DIND_BLOCK] != 0)
        indirectBlockReferences(numiNode, iNode.i_block[EXT2_DIND_BLOCK], block_offset, depth, type);

    block_offset += 65536;
    depth++;// 3
    if (iNode.i_block[EXT2_TIND_BLOCK] != 0)
        indirectBlockReferences(numiNode, iNode.i_block[EXT2_TIND_BLOCK], block_offset, depth, type);

    return;
}

void directoryEntries(int numParentiNode, int offset)
{
    // get offsets
    offset = 1024 + blocksize * (offset - 1);
    __u32 byte_offset = 0;

    // make dirEntry struct
    struct ext2_dir_entry dirEntry;

    while (byte_offset < blocksize)
    {
        // set name entry to 0
        memset(dirEntry.name, 0, 256);

        //TODO -- error check pread
        /* read from img into the dirEntry struct */
        if (pread(img, &dirEntry, sizeof(dirEntry), offset + byte_offset) != sizeof(dirEntry))
        {
            fprintf(stderr, "Error: Encountered an issue on pread() for directory entries\n");
            exit(1);
        }

        // if the inode number is not 0 then print
        if (dirEntry.inode != 0)
        {
            memset(&dirEntry.name[dirEntry.name_len], 0, 256 - dirEntry.name_len);
            cout << "DIRENT,"
                 << numParentiNode << ','                     /* parent inode number (decimal) ... the I-node number of the directory that contains this entry */
                 << byte_offset << ','                        /* logical byte offset (decimal) of this entry within the directory */
                 << dirEntry.inode << ','                     /* inode number of the referenced file (decimal) */
                 << dirEntry.rec_len << ','                   /* entry length (decimal) */
                 << (unsigned short) dirEntry.name_len << ',' /* name length (decimal) */
                 << '\'' << dirEntry.name << '\'' << endl;    /* name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will be no single-quotes or commas in any of the file names. */
        }
        byte_offset += dirEntry.rec_len;
    }
}

void indirectBlockReferences(int numiNode, int blockNumber, int offset, int depth, char type)
{
    // get length
    __u32 length = blocksize / sizeof(__u32);

    // create space for the indent array
    __u32 * blockNumber_arr = (__u32 *)malloc(blocksize);// TODO -- use new

    // TODO -- error check pread
    // read into the array
    if (pread(img, blockNumber_arr, blocksize, blockNumber * blocksize) != blocksize)
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for indirect block references");
        exit(1);
    }

    for(__u32 i = 0; i < length; i++){
        // when 
        if(blockNumber_arr[i]){
            cout << "INDIRECT,"
                 << numiNode << ','             /* I-node number of the owning file (decimal) */
                 << depth << ','                /*(decimal) level of indirection for the block being scanned ... 
                                                        1 for single indirect, 
                                                        2 for double indirect, 
                                                        3 for triple */
                 << offset << ','               /* logical block offset (decimal) represented by the referenced block. 
                                                        If the referenced block is a data block, this is the logical block offset 
                                                        of that block within the file. 
                                                        If the referenced block is a single- or double-indirect block, 
                                                        this is the same as the logical offset of the first data block to which it refers. */
                 << blockNumber << ','          /* block number of the (1, 2, 3)
                                                        indirect block being scanned (decimal) . . . 
                                                        not the highest level block (in the recursive scan), 
                                                        but the lower level block that contains the block reference reported by this entry. */
                 << blockNumber_arr[i] << endl; /* block number of the referenced block (decimal) */

            // check if it is a directory with a depth of 1
            if(depth == 1 && type == 'd'){
                directoryEntries(blockNumber, blockNumber_arr[i]);
            }

            // if the depth is greater than 1 then do an indirect block reference
            if(depth > 1){
                indirectBlockReferences(numiNode, blockNumber_arr[i], offset, depth - 1, type);
            }
        }

        // 1 for single indirect
        if(depth == 1){
            offset++;
        }
        // 2 for double indirect
        else if(depth == 2){
            offset += 256;
        }
        // 3 for triple
        else if(depth == 3){
            offset += 65536;
        }
        else{
            fprintf(stderr, "invalid depth\n");
            exit(1);
        }
    }

    // free the array
    free(blockNumber_arr); // TODO -- use delete

    return;
}

void formatTime(__u32 time, char* timeStr)
{
    time_t tmp = time;
    tm* tm = gmtime(&tmp);
    if (tm == NULL)
    {
        fprintf(stderr, "Error: Encountered an issue on gmtime\n");
        exit(1);
    }

    // set up timeStr with correct values
    if ((sprintf(timeStr, "%02d/%02d/%02d %02d:%02d:%02d", 
                    tm->tm_mon + 1,
                    tm->tm_mday,
                    tm->tm_year % 100,
                    tm->tm_hour,
                    tm->tm_min,
                    tm->tm_sec)) < 0)
    {
        fprintf(stderr, "Error: Unable to move time string into buffer with sprintf\n");
        exit(1);
    }
}
