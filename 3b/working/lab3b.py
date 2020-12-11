# NAME: Dylan Gunn,William Randall
# EMAIL: dylankgunn@gmail.com,wrandall1000@gmail.com
# ID: 805112866,805167986

import csv
import sys
import math

# GLOBAL LISTS
bfree = []
ifree = []

# GLOBAL ERROR FLAG
error_flag = False


# TODO -- do dictionary entries for each of these?
# CLASS DEFINITIONS
class Superblock:
    def __init__(self, row):
        self.num_blocks = int(row[1])
        self.num_inodes = int(row[2])
        self.block_size = int(row[3])
        self.inode_size = int(row[4])
        self.blocks_per_group = int(row[5])
        self.inode_per_group = int(row[6])
        self.first_inode = int(row[7])


class Group:
    def __init__(self, row):
        self.num = int(row[1])
        self.inode_per_group = int(row[3])
        self.inode_table = int(row[8])


class Inode:
    def __init__(self, row):
        self.num = int(row[1])
        self.file_type = row[2]
        self.mode = int(row[3])
        self.owner = int(row[4])
        self.group = int(row[5])
        self.link_count = int(row[6])
        self.ctime = row[7]
        self.mtime = row[8]
        self.atime = row[9]
        self.file_size = int(row[10])
        self.num_blocks = int(row[11])
        self.directories = []
        self.directories.extend([int(val) for val in row[12:24]])
        self.indirect_refs = []
        self.indirect_refs.extend([int(val) for val in row[24:27]])


class Dirent:
    def __init__(self, row):
        self.parent_inode = int(row[1])
        self.log_offset = int(row[2])
        self.inode = int(row[3])
        self.entry_len = int(row[4])
        self.name_len = int(row[5])
        self.name = row[6]


class Indirect:
    def __init__(self, row):
        self.inode_num = int(row[1])
        self.level = int(row[2])
        self.block_offset = int(row[3])
        self.indir_block_num = int(row[4])
        self.ref_block_num = int(row[5])


# HELPER FUNCTION DEFINITIONS
def check_blocks(inodes, indirects, initial_block, num_blocks):
    blocks_allocated_d = {}
    duplicate_blocks = set()
    offsets = {1: 12, 2: 268, 3: 65804}
    depths = {0: '', 1: ' INDIRECT', 2: ' DOUBLE INDIRECT', 3: ' TRIPLE INDIRECT'}

    def analyze_direct_block(offset_t, block_t, inode_t, num_blocks_t) -> int:
        global error_flag
        num = inode_t.num
        level = 0
        off = offset_t
        element = [num, off, level]
        # if the block is out of range
        if block_t >= num_blocks_t or block_t < 0:
            print(f'INVALID BLOCK {block_t} IN INODE {num} AT OFFSET {off}')
            error_flag = True
        # if block is from 0 to the first block that block is reserved
        elif 0 < block_t < initial_block:
            print(f"RESERVED BLOCK {block_t} IN INODE {num} AT OFFSET {off}")
            error_flag = True
        elif block_t == 0:
            pass  # TODO -- what to if block == 0
        # if block is not 0 add it to the allocated dictionary
        else:
            if block_t not in blocks_allocated_d:
                blocks_allocated_d[block_t] = [element]  # list of lists
            else:
                duplicate_blocks.add(block_t)
                blocks_allocated_d[block_t].append(element)
        return off + 1

    def analyze_indirect_block(block_t, inode_t, num_blocks_t, depth_t) -> int:
        global error_flag
        num = inode_t.num
        off = offsets[depth_t]
        depth_string = depths[depth_t]
        level = 0
        element = [num, off, level]
        # if the block is out of range
        if block_t >= num_blocks_t or block_t < 0:
            print(f"INVALID{depth_string} BLOCK {block_t} IN INODE {num} AT OFFSET {off}")
            error_flag = True
        # if block is from 0 to the first block that block is reserved
        elif 0 < block_t < initial_block:
            print(f"RESERVED{depth_string} BLOCK {block_t} IN INODE {num} AT OFFSET {off}")
            error_flag = True
        elif block_t == 0:
            pass  # TODO -- what to if block == 0
        else:
            if block_t not in blocks_allocated_d:
                blocks_allocated_d[block_t] = [element]  # list of lists
            else:
                duplicate_blocks.add(block_t)
                blocks_allocated_d[block_t].append(element)
        return depth + 1

    def analyze_indirect_referenced_block(indirect_t, num_blocks_t):
        global error_flag
        block_t = indirect_t.ref_block_num
        depth_string = depths[indirect_t.level]
        num = indirect_t.inode_num
        off = indirect_t.block_offset
        level = indirect_t.level
        element = [num, off, level]
        # if the block is out of range
        if block_t >= num_blocks_t or block_t < 0:
            # print(indirect_t.level) TODO -- why is this here
            print(f"INVALID{depth_string} BLOCK {block} IN INODE {num} AT OFFSET {off}")
            error_flag = True
        # if block is from 0 to the first block that block is reserved
        elif 0 < block_t < initial_block:
            # print(indirect_t.level) TODO -- why is this here
            print(f"RESERVED{depth_string} BLOCK {block} IN INODE {num} AT OFFSET {off}")
            error_flag = True
        elif block_t == 0:
            pass  # TODO -- what to if block == 0
        else:
            if block_t not in blocks_allocated_d:
                blocks_allocated_d[block_t] = [element]  # list of lists
            else:
                duplicate_blocks.add(block_t)
                blocks_allocated_d[block_t].append(element)

    def print_duplicate_blocks():
        global error_flag
        # find all duplicate blocks in the duplicate blocks set
        for block_t in duplicate_blocks:
            for block_list in blocks_allocated_d[block_t]:
                assert isinstance(block_list, list)
                num, off, level = block_list
                depth_string = depths[level]
                print(f"DUPLICATE{depths.get(block_list[2])} BLOCK {block_t} IN INODE {num} AT OFFSET {off}")
                error_flag = True

    def analyze_blocks():
        global error_flag
        for block_t in range(initial_block, num_blocks):
            if block_t in blocks_allocated_d:
                if block_t in bfree:
                    print(f"ALLOCATED BLOCK {block_t} ON FREELIST")
                    error_flag = True
            # block not in blocks allocated dictionary
            else:
                if block_t not in bfree:
                    print(f"UNREFERENCED BLOCK {block_t}")
                    error_flag = True

    # look at each inode
    for inode in inodes:
        # skip soft links and links of size less than or equal to 60
        if inode.file_type == 's' and inode.file_size <= 60:
            continue
        # look at each direct block
        offset = 0
        for block in inode.directories:
            offset = analyze_direct_block(offset, block, inode, num_blocks)
        # look at each indirect block
        depth = 1
        for block in inode.indirect_refs:
            depth = analyze_indirect_block(block, inode, num_blocks, depth)

    # look at each indirect referenced block
    for indirect in indirects:
        analyze_indirect_referenced_block(indirect, num_blocks)

    print_duplicate_blocks()
    analyze_blocks()


def check_inodes(inodes, dirents, first, cap):
    allocated = []  # TODO -- name?
    link_count = {}
    parent_dir = {2: 2}

    def check_freelist(inodes_t, alloc):
        global error_flag
        for inode in inodes_t:
            num = inode.num
            if num != 0:
                alloc.append(num)
                if num in ifree:
                    print(f"ALLOCATED INODE {num} ON FREELIST")
                    error_flag = True
            if inode.file_type == '0' and num not in ifree:
                print(f"UNALLOCATED INODE {num} NOT ON FREELIST")
                error_flag = True
        for num in range(first, cap):  # TODO -- cap + 1?
            if num in alloc and num in ifree:
                print(f"ALLOCATED INODE {num} ON FREELIST")
                error_flag = True
            if num not in alloc and num not in ifree:
                print(f"UNALLOCATED INODE {num} NOT ON FREELIST")
                error_flag = True

    def check_directories(dirents_t, alloc):
        global error_flag
        for entry in dirents_t:
            if entry.name != "'.'" and entry.name != "'..'":
                parent_dir[entry.inode] = entry.parent_inode
            if entry.inode > cap or entry.inode <= 0:
                print(f"DIRECTORY INODE {entry.parent_inode} NAME {entry.name} INVALID INODE {entry.inode}")
                error_flag = True
            elif entry.inode not in alloc:
                print(f"DIRECTORY INODE {entry.parent_inode} NAME {entry.name} UNALLOCATED INODE {entry.inode}")
                error_flag = True
            else:
                inode = entry.inode
                if inode not in link_count:
                    link_count[inode] = 1
                else:
                    link_count[inode] += 1

    def check_links(inodes_t, counts):
        global error_flag
        for inode in inodes_t:
            num = inode.num
            if num not in counts:
                counts[num] = 0
            if counts[num] != inode.link_count:
                print(f"INODE {num} HAS {counts[num]} LINKS BUT LINKCOUNT IS {inode.link_count}")
                error_flag = True

    def check_curr_parent_dir(dirents_t):
        global error_flag
        for entry in dirents_t:
            if entry.name == "'.'" and entry.inode != entry.parent_inode:
                print(f"DIRECTORY INODE {entry.parent_inode} NAME '.' LINK TO INODE {entry.inode} SHOULD BE {entry.parent_inode}")
                error_flag = True
            if entry.name == "'..'" and parent_dir[entry.parent_inode] != entry.inode:
                print(f"DIRECTORY INODE {entry.parent_inode} NAME '..' LINK TO INODE {entry.inode} SHOULD BE {entry.parent_inode}")
                error_flag = True

    check_freelist(inodes, allocated)
    check_directories(dirents, allocated)
    check_links(inodes, link_count)
    check_curr_parent_dir(dirents)

argv = str()
def main():
    global argv
    # Get arg
    argv = sys.argv

    # Check if more than 1 arg
    if len(argv) != 2:
        sys.stderr.write("Error: Program expects exactly one argument.")
        exit(1)

    # Load CSV
    try:
        csv_file = open(argv[1], 'r')
    except OSError:
        sys.stderr.write("Error: Unable to open specified file.")
        exit(1)

    # Create variables
    superblock = None
    group = None
    inodes = []
    dirents = []
    indirects = []

    # Set up CSV
    reader = csv.reader(csv_file, delimiter=',')
    for row in reader:
        element = row[0]
        if element == "SUPERBLOCK":
            superblock = Superblock(row)  # only one superblock
        elif element == "GROUP":
            group = Group(row)  # only one group
        elif element == "BFREE":
            bfree.append(int(row[1]))
        elif element == "IFREE":
            ifree.append(int(row[1]))
        elif element == "INODE":
            inodes.append(Inode(row))
        elif element == "DIRENT":
            dirents.append(Dirent(row))
        elif element == "INDIRECT":
            indirects.append(Indirect(row))
        else:
            sys.stderr.write("Error: Inconcistency in CSV")
            exit(1)

    # TODO -- if not superblock or not group, error?

    initial_block = int(group.inode_table)
    initial_block += int(math.ceil(superblock.inode_size * group.inode_per_group / superblock.block_size))
    num_blocks = superblock.num_blocks

    # TODO -- change function names
    check_blocks(inodes, indirects, initial_block, num_blocks)
    check_inodes(inodes, dirents, superblock.first_inode, superblock.num_inodes)


if __name__ == "__main__":
    main()
    if error_flag:
        exit(2)
    else:
        exit(0)
