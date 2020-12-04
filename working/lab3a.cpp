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

    __u32 blocksPerGroup = (superblock.s_blocks_count >= superblock.s_blocks_per_group) 
        ? superblock.s_blocks_per_group : (superblock.s_blocks_count % superblock.s_blocks_per_group);
    __u32 iNodesPerGroup = (superblock.s_inodes_count >= superblock.s_inodes_per_group) 
        ? superblock.s_inodes_per_group : (superblock.s_inodes_count % superblock.s_inodes_per_group);

    return 0;
}
