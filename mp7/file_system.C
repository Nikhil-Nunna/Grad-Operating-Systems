/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    // allocate the inode list and free block list
    inodes = new Inode[MAX_INODES];
    free_blocks = new unsigned char[512];

    // mark blocks 0 and 1 as used
    free_blocks[0] = '1';
    free_blocks[1] = '1';
    // mark blocks as empty
    for (int i = 2; i < 512; i++)
    {
        free_blocks[i]='0';
    }
    
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    
    // Write the file system data to disk before destroying object
    disk->write(0,(unsigned char *)inodes);
    disk->write(1,free_blocks);
    // delete objects after writing them to disk
    delete[] free_blocks;
    delete[] inodes;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");

    /* Here you read the inode list and the free list into memory */

    //save the disk your mounting on
    disk = _disk;

    //inode block
    _disk->read(0,(unsigned char *)inodes);
    //free list block
    _disk->read(1,free_blocks);
    return true;

}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */
    // Create Inode and Free blocks list for formatting.
    Inode * inodes_list;
    inodes_list = new Inode[MAX_INODES];
    unsigned char *  free_blocks_list;
    free_blocks_list = new unsigned char[512];
    // mark blocks 0 and 1 as used
    free_blocks_list[0] = '1';
    free_blocks_list[1] = '1';
    // mark other blocks as empty
    for (int i = 2; i < 512; i++)
    {
        free_blocks_list[i]='0';
    }
    // write inode and free block lists to disk 
    _disk->write(0,(unsigned char *)inodes_list);
    _disk->write(1,free_blocks_list);
    // blow away these temp lists used for format
    delete[] inodes_list;
    delete[] free_blocks_list;
    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    // loop thru inode list till you find one with the same file_id
    for(int i = 0; i<MAX_INODES; i++){
        // check if node is valid
        if (inodes[i].val)
        {
            // return the inode if it has the same id
            if (_file_id == inodes[i].id)
            {
                return &inodes[i];
            }
        }
        
    }
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */

    // loop thru inode list to ensure there isn't one with the same ID
    for(int i = 0; i<MAX_INODES; i++){
        // check if node is valid
        if(inodes[i].val)
        {
            // return false if it has the same id
            if (_file_id == inodes[i].id)
            {
                return false;
            }
        }

    }

    
    // save position in pos when inode pos is found
    int pos = 0;
    // loop thru inode list to find empty inode
    for (int j = 0; j < MAX_INODES; j++)
    {   
        // find unused inode to use
        if (!inodes[j].val)
        {   
            //set id
            inodes[j].id = _file_id;
            // save position in list
            pos=j;
            // set inode as valid
            inodes[j].val=true;
            break;
        }
    }

    // loop thru free blocks find block that is empty
    for (int f = 0; f < 512; f++)
    {   
        // check if block is empty
        if (free_blocks[f] == '0')
        {
            // set block to used
            free_blocks[f] = '1';
            // set the block number in inode
            inodes[pos].block_num = f;
            return true;
        }
    }
    return false;
        
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */

    // loop thru inode list till you find one with the same file_id
    for(int i = 0; i<MAX_INODES; i++){
        // check if node is valid
        if (inodes[i].val)
        {
            // delete the inode and its blocks if it has the same id
            if (_file_id == inodes[i].id)
            {   
                // set block to free
                free_blocks[inodes[i].block_num] = '0';
                // set inode to invalid
                inodes[i].val=false;
                return true;
            }
        }
        
    }
    return false;
}

SimpleDisk * FileSystem::get_disk(){
    // return disk
    return disk;
}