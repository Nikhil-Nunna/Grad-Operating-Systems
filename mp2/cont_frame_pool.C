/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

// Pull in static vars
ContFramePool* ContFramePool::Head = nullptr;
ContFramePool* ContFramePool::Tail = nullptr;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no)
{
    // prep for bit shifting
    unsigned int bitmap_index = _frame_no /4;
    unsigned char mask = 0x3 << ((_frame_no % 4) * 2);

    //Get Frame state in binary to switch case through
    unsigned char frame_state = (bitmap[bitmap_index] & mask) >> ((_frame_no % 4) * 2);
    if (frame_state == 0x0)
    {
        return FrameState::Free;
    }
    else if (frame_state == 0x1)
    {
        return FrameState::Used;
    }
    else if (frame_state == 0x2)
    {
        return FrameState::HoS;
    }
}

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state) 
{
    // prep for bit shifting
    unsigned int bitmap_index = _frame_no /4;
    unsigned char shift = ((_frame_no % 4) * 2);
    unsigned char mask = 0;

    // Switch through on requested state to set state
    switch(_state) {
        case FrameState::Free:
            mask = 0x3 << shift;
            bitmap[bitmap_index] &= ~mask;
            break;
        case FrameState::Used:
            mask = 0x1 << shift;
            bitmap[bitmap_index] |= mask;
            break;
        case FrameState::HoS:
            mask = 0x2 << shift;
            bitmap[bitmap_index] |= mask;
            break;

    }
    
}

/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{   
    // Add Frame pool to Doubly Linked List
    if (Head == nullptr)
    {
        Head = this;
    }
    if (Tail == nullptr)
    {
        Tail = this;
    }
    if (Tail != this)
    {
        Tail -> next = this;
        prev = Tail;
        Tail = this;
    }
    // Set Frame pool private vars
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    // allocate bitmap
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }

    // Everything ok. Proceed to mark all frame as free.
    for(int fno = base_frame_no; fno < base_frame_no + nframes; fno++) {
        set_state(fno, FrameState::Free);
    }

    // Mark the first frame as being used if it is being used
    set_state(_info_frame_no, FrameState::HoS);
    if (info_frame_no >= base_frame_no)
    {
        nFreeFrames--;
    }
    
    // Mark subsequent frames as being used for information frames
    unsigned long info_frames = needed_info_frames(_n_frames);
    for (unsigned long i = _info_frame_no + 1; i < _info_frame_no + info_frames; i++)
    {
        set_state(i, FrameState::Used);
        if (i >= base_frame_no)
        {
            nFreeFrames--;
        }
    }
    
    
    Console::puts("Frame Pool initialized\n");
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Implement First Fit
    // Check there are enough free frames
    if (nFreeFrames < _n_frames)
    {
        return 0;
    }
    // set vars to find start frame
    unsigned long counter = 0;
    unsigned long start_pos = base_frame_no;
    bool found = false;
    // Loop through frame pool to find a contigious block of _n_frames
    for (unsigned long i = base_frame_no; i < base_frame_no + nframes; i++)
    { 
        if(get_state(i) == FrameState::Free)
        {
            counter++;
            // if found break and go to allocation
            if (counter == _n_frames)
            {
                found = true;
                break;
            }
            
        }
        // if not found then reset counter and move start to next frame
        else
        {
            counter = 0;
            start_pos = i + 1;
        }
        
    }
    // allocate the frames as HOS and used
    if(found)
    {
        // Allocated Hos
        set_state(start_pos, FrameState::HoS);
        nFreeFrames--;
        for (unsigned long i = start_pos + 1; i < start_pos + _n_frames; i++)
        {
            // Alocated subsequent frames
            set_state(i, FrameState::Used);
            nFreeFrames--;
        }
        return start_pos;
    }

    return -1;    
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // Mark first frame as Hos
    set_state(_base_frame_no, FrameState::HoS);
    nFreeFrames--;
    for(int fno = _base_frame_no + 1; fno < _base_frame_no + _n_frames; fno++)
    {
        // Mark subsequent frames as used
        set_state(fno, FrameState::Used);
        nFreeFrames--;
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    if (Tail != nullptr)
    {
        ContFramePool * pool = Tail;
        bool not_found = true;
        // Find the Frame pool this frame belongs too
        while (not_found)
        {
            // Find largest Base Frame less than _first_Frame and ensure first frame is less than the last frame in pool
            if ((pool->base_frame_no <= _first_frame_no) && (pool->base_frame_no + pool->nframes > _first_frame_no))
            {
            
                not_found = false;
            }
            else
            {
                // iterate to the previous frame pool
                if (pool != Head)
                {
                    pool = pool->prev;
                }
                else
                {
                    return;
                }
                
            }
            
        }
        // Ensure frame is start of the contigous block
        if(pool->get_state(_first_frame_no) == FrameState::HoS)
        {
            // Set vars to release frames
            unsigned long curr_frame_no =_first_frame_no;
            // Release Hos
            pool->set_state(curr_frame_no, FrameState::Free);
            pool->nFreeFrames++;
            curr_frame_no++;
            bool alloc = true;
            while (alloc)
            {   
                // Release Frames till next block our empty frames
                if((pool->get_state(curr_frame_no) != FrameState::HoS) && (pool->get_state(curr_frame_no) != FrameState::Free))
                {
                    pool->set_state(curr_frame_no,FrameState::Free);
                    curr_frame_no++;
                    pool->nFreeFrames++;
                }
                // end loop
                else
                {
                    alloc = false;
                }
            }   
        }

    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // return frames by FRAME_SIZE divide by 4 plus one if remainder
    // Divide by 4 because 2 bits per frame to address it
    return (((_n_frames / FRAME_SIZE) / 4) + (_n_frames % FRAME_SIZE > 0 ? 1 : 0));
}

unsigned long ContFramePool::free_frames()
{
    // Return the number of Free Frames
    return this->nFreeFrames;
}
