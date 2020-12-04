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

struct ext2_super_block superblock;

__u32 blocksize;

void groupSummary();
void freeBlockEntries();
void freeiNodeEntries();
void iNodeSummary();
void directoryEntries();
void indirectBlockReferences();

int main(int argc, char** argc)
{
    if (argc != 2)
        fprintf(stderr, "Error: Incorrect usage. Expected: ./lab3a [image]\n");

    img = open(argv[1], O_RDONLY);
    if (img == -1)
    {
        fprintf(stderr, "Error: Unable to open the specified file");
        exit(1);
    }
    
    if (pread(img, &superblock; sizeof(struct ext2_super_block), SUPERBLOCK_OFFSET) != sizeof(struct ext2_super_block))
    {
        fprintf(stderr, "Error: Encountered an issue on pread() for superblock\n");
        exit(1);
    }
    if (superblock.s_magic != EXT2_SUPER_MAGIC)
    {
        fprintf(stderr, "Error: File system is not EXT2\n");
        exit(1);
    }

    blocksize = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;
    
    /* superblock summary */
    cout << "SUPERBLOCK," 
        << superblock.s_blocks_count << ','
        << superblock.s_inodes_count << ','
        << blocksize << ','
        << superblock.s_inode_size << ','
        << superblock.s_blocks_per_group << ','
        << superblock.s_inodes_per_group << ','
        << superblock.s_first_ino << endl;

    int i = 0;
    for (i = 0; i < numGroups; i++)
    {
        groupSummary(i, numGroups);
    }
    
    //groupSummary();
    //freeBlockEntries();
    //freeiNodeEntries();
    //iNodeSummary();
    //directoryEntries();
    //indirectBlockReferences();

    return 0;
}

void groupSummary(int index, __u32 max)
{
    struct ext2_group_desc group;
    __32 offset = (superblock.s_first_data_block + 1) * blocksize;
    pread(img, &group, sizeof(group), offset);
    
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

    return;
}
