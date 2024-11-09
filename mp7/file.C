/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    // set file system
    fs = _fs;
    // set inode
    inode = fs->LookupFile(_id);
    // set disk
    disk = fs->get_disk();
    // read disk to buffer
    disk->read(inode->block_num,block_cache);
    curr_poss = 0;

}

File::~File() {
    Console::puts("Closing file.\n");
    // writing back to disk
    disk->write(inode->block_num,block_cache);
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    // set counter for characters
    int count=0;
    for(int i = curr_poss; i<_n; i++)
    {
        // check end of file if it is then break;
        if (EoF())
        {
            break;
        }
        // read from buffer
        _buf[count] = block_cache[i];
        count++;
        curr_poss++;
        
    }
    return count;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    // set counter for characters
    int count=0;
    for(int i = curr_poss; i<_n; i++)
    {
        // check there is no space in the file then break;
        if (i == 512)
        {
            break;
        }
        // write a buffer
        block_cache[i] = _buf[count];
        count++;
        curr_poss++;
        
    }

    // update current poss if it is more than file length
    if (curr_poss > inode->file_length)
    {
        inode->file_length = curr_poss;
    }
    return count;
}

void File::Reset() {
    Console::puts("resetting file\n");
    // reset current poss to 0
    curr_poss = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    // check if current poss is equal to file length
    if (curr_poss == inode->file_length)
    {
        return true;
    }
    return false;
    
}
