#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ext2_fs.h"
using namespace std;

//GLOBAL STRUCTS
struct ext2_super_block superblock;

//GLOBAL VARS
int img = -1;
__u32 blocksize;

void groupSummary(int index, __u32 max); /* read in the group and get a Summary */
void freeBlockBitmap(int index, int off, __u32 numBlocks); /* free Block Bitmap */
void freeiNodeBitmap(int index, int off, int table, __u32 numBytes); /* free iNode Bitmap */
void iNodeSummary(int table, int numiNode); /* summary of iNode */
void directoryEntries(int numiNode, int off); /* read direct block references */
void indirectBlockReferences(int numiNode, int numberOfBlocks, int off, int depth, char type); /* read indirect block references */

void formatTime(__u32 time, char* timeStr); /* make a time string buffer */

int main(int argc, char** argc)
{
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

    blocksize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    
    /* superblock summary */
    cout << "SUPERBLOCK," 
        << superblock.s_blocks_count << ','     /* total number of blocks (decimal) */
        << superblock.s_inodes_count << ','     /* total number of i-nodes (decimal) */
        << blocksize << ','                     /* block size (in bytes, decimal) */
        << superblock.s_inode_size << ','       /* i-node size (in bytes, decimal) */
        << superblock.s_blocks_per_group << ',' /* blocks per group (decimal) */
        << superblock.s_inodes_per_group << ',' /* i-nodes per group (decimal) */
        << superblock.s_first_ino << endl;      /* first non-reserved i-node (decimal) */

    //get number of groups for groupSUmmary
    __u32 numGroups = superblock.s_blocks_count / superblock.s_blocks_per_group;
    numGroups += !! ((superblock.s_blocks_count) % (superblock.s_blocks_per_group));

    int i = 0;
    for (i = 0; i < numGroups; i++)
    {
        groupSummary(i, numGroups);
    }
    
    //directoryEntries();
    //indirectBlockReferences();

    return 0;
}

void groupSummary(int index, __u32 max)
{
    struct ext2_group_desc group;
    __u32 offset = (superblock.s_first_data_block + 1) * blocksize;
    if (pread(img, &group, sizeof(struct ext2_group_desc), offset) != sizeof(struct ext2_group_desc))
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for group\n");
        exit(1);
    }
    
    __u32 numBlocks = (superblock.s_blocks_count >= superblock.s_blocks_per_group) ?
        superblock.s_blocks_count : (superblock.s_blocks_count % superblock.s_blocks_per_group);
    __u32 numiNodes = (superblock.s_inodes_count >= superblock.s_inodes_per_group) ?
        superblock.s_inodes_count : (superblock.s_inodes_count % superblock.s_inodes_per_group);
    
    cout << "GROUP,"
        << "0," // Only one group for 3a
        << numBlocks << ','
        << numiNodes << ','
        << superblock.s_free_blocks_count << ','
        << superblock.s_free_inodes_count << ','
        << group.bg_block_bitmap << ','
        << group.bg_inode_bitmap << ','
        << group.bg_inode_table << endl;

    freeBlockBitmap(index, group.bg_block_bitmap, numBlocks);

    freeiNodeBitmap(index, group.bg_inode_bitmap, group.bg_inode_table, numiNodes / 8);

    return;
}

void freeBlockBitmap(int index, int off, __u32 numBlocks)
{
    off = 1024 + blocksize * (off - 1);
    char* blockBitmap = malloc(blocksize);

    if (pread(img, blockBitmap, blocksize, off) != numBlocks) //check sizing here
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for bitmap\n");
        exit(1);
    }
    
    int blockNum = superblock.s_first_data_block + index * superblock.s_blocks_per_group;
    int mask = 1;
    int bit;
    int i, j;
    for (i = 0; i < numBlocks; i++)
    {
        bit = blockBitmap[i];
        for (j = 0; j < 8; j++)
        {
            if (!(bit & mask))
                cout << "BFREE," << blockNum << endl;
            bit >>= 1;
            blockNum++;
        }
    }

    free(blockBitmap);
    return;
}

void freeiNodeBitmap(int index, int off, int table, __u32 numBytes)
{
    off = 1024 + blocksize * (off - 1);
    char* iNodeBitmap = malloc(blocksize);

    pread(img, iNodeBitmap, blocksize, off);

    int iNodeNum = superblock.s_inodes_per_group * index + 1;
    int bit;
    int mask = 1;
    int i, j;
    for (i = 0; i < numBytes; i++)
    {
        bit = iNodeBitmap[i];
        for (j = 0; j < 8; j++)
        {
            if (!(bit & mask))
                cout << "IFREE," << iNodeNum << endl;
            else
               iNodeSummary(table, iNodeNum); 
        }
    }
}

void iNodeSummary(int table, int numiNode)
{
    long off = 1024 + blocksize * (table - 1) + 128*(numiNode - 1);

    struct ext2_inode iNode;
    if (pread(img, &iNode, sizeof(struct ext2_inode), off) != sizeof(struct ext2_inode))
    {
        fprintf(stderr, "Error: Encountered an issue on call to pread() for inode\n");
        exit(1);
    }

    char type = '?';
    unsigned int mode = (__uint16_t)(iNode.i_mode >> 12);
    if (mode == 0xa)
        type = 's';
    else if (mode == 0x8)
        type = 'f';
    else if (mode == 0x4)
        type = 'd';

    char ctime[18], mtime[18], atime[20];
    formatTime(iNode.i_ctime, ctime);
    formatTime(iNode.i_mtime, mtime);
    formatTime(iNode.i_atime, atime);

    int numBlocks = 2 * iNode.i_blocks / (2 << superblock.s_log_block_size);

    cout << "INODE,"
        << numiNode << ','
        << type << ','
        << iNode.i_mode & 0xFFF << ',' //Check this
        << iNode.i_uid << ','
        << iNode.i_gid << ','
        << iNode.i_links_count << ','
        << ctime << ','
        << mtime << ','
        << atime << ','
        << iNode.i_size << ','
        << numBlocks;
    int i;
    if (type != 's' || iNode.i_size > 60)
        for (i = 0; i < 15, i++)
            cout << iNode.i_block[i] << ',';
    cout << endl;

    if (type == 'd')
        for (i = 0; i < 12; i++)
            if (iNode.i_block[i] != 0)
                directoryEntries(numiNode, iNode.i_block[i]);

    int numberOfBlocks = 12;
    int depth = 1;
    if (iNode.i_block[12] != 0)
        indirectBlockReferences(numiNode, iNode.i_block[12], numberOfBlocks, depth, type);

    numberOfBlocks += 256;
    depth++;
    if (iNode.i_block[13] != 0)
        indirectBlockReferences(numiNode, iNode.i_block[13], numberOfBlocks, depth, type);

    numberOfBlocks += 65536;
    depth++;
    if (iNode.i_block[14] != 0)
        indirectBlockReferences(numiNode, iNode.i_block[14], numberOfBlocks, depth, type);

    return;
}

void directoryEntries(int numiNode, int off)
{
    off = 1024 + blocksize * (off - 1);
    int boff = 0;
    struct ext2_dir_entry dirEntry;

    while (boff < blocksize)
    {
        memset(dirEntry.name, 0, 256);
        if (pread(img, &dirEntry, sizeof(struct ext2_dir_entry), off + boff) != sizeof(struct ext2_dir_entry))
        {
            fprintf(stderr, "Error: Encountered an issue on call to pread() for directory entry\n");
            exit(1);
        }
        if (entry.inode != 0)
        {
            memset(&entry.name[entry.name_len], 0, 256 - entry.name_len);
            cout << "DIRENT,"
                << numiNode << ','
                << boff << ','
                << dirEntry.inode << ','
                << dirEntry.rec_len << ','
                << dirEntry.name_len << ','
                << dirEntry.name << endl;
        }
        boff += dirEntry.rec_len;
    }
}


void indirectBlockReferences(int numiNode, int numberOfBlocks, int off, int depth, char type)
{
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
