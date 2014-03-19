//
//  GlobalMemory.h
//  streamTRAX
//
//  Created by Ben Hillery on 11/22/11.
//  This is a port of FreeRTOS MALLOC to TRAX. The main idea is to store free blocks of memory in a linked list memory pool.
//  No attempt is made to merge adjacent free blocks. While this will quickly become inefficient for applications that
//  require random sizes of memory, it should be okay for most things.
//

#ifndef streamTRAX_GlobalMemory_h
#define streamTRAX_GlobalMemory_h

//#include "Semaphore.h"


//#include <new>

#define END_MEMORY      14
#define SEM_BUFFER_SIZE 8 //size of the semaphore buffer. max size = max number of atomicincs available
#define GLOBAL_SEMAPHORE 2  // used by the memory pool

#define MEM_SIZE loadi(0,15)

//place the heap somewhere after the semaphore buffer
#define HEAP_LOCATION   (loadi(0,END_MEMORY)+SEM_BUFFER_SIZE + 100) 

//relative addresses of BlockLink Variables
#define NEXT_BLOCK          0
#define BLOCK_SIZE          1
#define BLOCKLINK_SIZE      2 // the number of words stored in a blocklink

//relative addresses of TraxHeap Variables
#define FREE_WORDS_REMAINING    0
#define HEAP_IS_INITIALIZED     1
#define HEAP_START_BLOCK        2 //blocklink that points to the first free block
#define HEAP_END_BLOCK          4 //blocklink that marks the end of the list
#define HEAP_ARRAY              6 //the start of free memory
#define TOTAL_HEAP_SIZE         (MEM_SIZE - HEAP_LOCATION - HEAP_ARRAY) // the total size of the available heap 
#define HEAP_MIN_BLOCK_SIZE     0 //smallest allowable heap block size. default 0

//macros to make accessing heap globals easier
#define heapStart   BlockLink(HEAP_LOCATION+HEAP_START_BLOCK)
#define heapEnd     BlockLink(HEAP_LOCATION+HEAP_END_BLOCK)
#define readFreeWordsRemaining  loadi(HEAP_LOCATION,FREE_WORDS_REMAINING)
#define readIfHeapIsInitialized loadi(HEAP_LOCATION,HEAP_IS_INITIALIZED)

#define GLOBAL_SIZE_FACTOR  (4)       //how many bytes per global word 
//#define globalSize(Y)   (sizeof(Y)/4) //convert sizeof() from bytes to words
#define HEAP_INIT_CODE (112432)

typedef unsigned long int raw_global_ptr;//make it more clear when we are dealing with addresses
typedef int global_offset;

void traxFree(raw_global_ptr);
raw_global_ptr traxMalloc( int );
//template <class X> class global_address;

/*
template <class X> class global_ptr
{//this class behaves as a local pointer to a globally-stored object
protected:
    //raw_global_ptr itsPtr; 
    //global_offset  offset;
    X pObj;
public:
    global_ptr( const raw_global_ptr ptr)   : pObj(ptr)                            {}
    global_ptr( void* ptr = 0 )             : pObj((raw_global_ptr)ptr)            {}
    global_ptr( const global_ptr<X> & other_ptr )      : pObj(other_ptr.pObj)      {}
	global_ptr( const global_address<X> & other_addr) : pObj(other_addr.get())     {}
    ~global_ptr()                                                                  {}
    
    //X* operator->()   const throw()                 {return &pObj;}//call object member functions. untested.
    X operator*()    const throw()                  {return pObj;}
    X operator[](const int index)                   {return X(pObj.addr+(X::globalSize()*index));}
    const raw_global_ptr get()    const             {return pObj.addr;}//return the raw address
    operator global_ptr<X>*()                       {return reinterpret_cast<global_ptr<X>*>(pObj.addr);}
    void operator delete(void *ptr)                 {traxFree((raw_global_ptr)ptr);}  //ptr should be itsPtr, by virtue of operator X*()
};
*/

/*
template< class X > class global_address
{//this class provides a local interface to a globally-stored pointer to a globally-stored object
private:  
protected:
    raw_global_ptr  addr;
    global_offset   offset;//offset does not propagate to copies
public:
    //explicit global_address()                            {}//zero-argument constructor must be empty
    explicit global_address( void* ptr = 0) : addr((raw_global_ptr)ptr),offset(0){}
    explicit global_address( raw_global_ptr new_addr, global_offset new_offset = 0) : addr(new_addr),offset(new_offset) {}
    global_address( const global_address< X > & ptr ) { storei(loadi(ptr.addr,ptr.offset),addr,offset);}//copy constructor
    ~global_address()                                    {}; //handle deletion of linked members, if any

    //required functions for working with global_ptr
    global_ptr<global_address> operator &()         { return global_ptr<global_address>(get());} //address operator returns a global_ptr
    static unsigned int globalSize()                { return 1;} //how many words of global memory does this class use?
    friend class global_ptr<global_address>;
    void print()
    {
#if TRAX==0
        printf("addr= %d, offset= %d, value=%d ", (int)addr, (int)offset, (int)loadi(addr,offset));
#endif
    }

    //member functions that dictate actual class behavior
    X operator*() const throw()                     {return X(loadi(addr,offset));}
    X operator[](const int index)                   {return X(loadi(addr,offset)+(X::globalSize()*index));}
    const raw_global_ptr get() const                {return loadi(addr,offset);}//return the raw address
    operator global_ptr<X>*()                       {return reinterpret_cast<global_ptr<X>*>(loadi(addr,offset));}
    //operator void*()                                {return (void*)loadi(addr,offset);}
    //const global_address& operator =(const void* value)  { storei((raw_global_ptr)value,addr,offset); return *this;} //assignment operator saves data
    const global_address& operator =(const raw_global_ptr value){ storei(value,addr,offset); return *this;} //assignment operator saves data
    const global_address& operator =(const global_address<X> & ptr) { storei(loadi(ptr.addr,ptr.offset),addr,offset); return *this;} //assignment operator saves data
    const global_address& operator =(const global_ptr<X> & ptr) { storei(ptr.get(),addr,offset); return *this;} //assignment operator saves data
    void operator delete(void *ptr)                 {traxFree((raw_global_ptr)ptr);}  //ptr should be itsPtr, by virtue of operator X*()
};
*/

class BlockLink
{
protected:
    raw_global_ptr addr; // global address of the block
    
public:
    BlockLink()
    {
        //do nothing
    }
    BlockLink(raw_global_ptr newAddr)
    {
        addr = newAddr;
    }
    
    BlockLink nextBlock()
    {
        return BlockLink(loadi(addr,NEXT_BLOCK));
    }
    
    int blockSize()
    {
        return loadi(addr,BLOCK_SIZE);
    }
    
    raw_global_ptr address()
    {
        return addr;
    }
    
    //save the address of the next block. warning: not thread safe!
    void setNextBlock(BlockLink nextBlock )
    {
        storei(nextBlock.addr,addr,NEXT_BLOCK);
    }
    
    //save a new block size. warning: not thread safe!
    void setBlockSize( int newSize)
    {
        storei(newSize,addr,BLOCK_SIZE);
    }
    
#if TRAX==0
    void print()
    {
        printf("Block address: %u, next: %u, size: %u\n", (unsigned int)addr, (unsigned int)nextBlock().address(), (unsigned int)blockSize());
    }
#else
    void print()
    {
        trax_printi(800);
        trax_printi(addr);
        trax_printi(blockSize());
    }
#endif
};

#if TRAX==0
void printHeap()
{
    BlockLink currentBlock = heapStart;
    if (readIfHeapIsInitialized) {
        printf("**Printing Heap**\n Heap Initialized\n Free Memory: %d\n Location: %d\n",readFreeWordsRemaining,HEAP_LOCATION);
        printf("Block List:\n");
        while(1)
        {
            currentBlock.print();
            if (currentBlock.nextBlock().address() == 0) {
                break;
            }
            currentBlock = currentBlock.nextBlock();
        }
    }else
    {
        printf("**Printing Heap**\nHeap Uninitialized\n");
    }
}
#else
void printHeap()
{
    BlockLink currentBlock = heapStart;
    if (readIfHeapIsInitialized) {
        trax_printi(778);
        while(1)
        {
            currentBlock.print();
            if (currentBlock.nextBlock().address() == 0) {
                break;
            }
            currentBlock = currentBlock.nextBlock();
        }
    }else
    {
        trax_printi(776);
    }
}
#endif

//stuffs a block into the list of free blocks. warning: not thread safe!
inline void heapInsertBlockIntoFreeList( BlockLink heapBlockToInsert)
{
    BlockLink heapIterator;
    int heapBlockSize;
    
    heapBlockSize = heapBlockToInsert.blockSize();
    
    //iterate until we find a bigger block than the one we have
    for (heapIterator = heapStart; heapIterator.nextBlock().blockSize() < heapBlockSize; heapIterator = heapIterator.nextBlock() ) 
    {
        //empty loop body. all the work is done in the loop definition
    }
    
    heapBlockToInsert.setNextBlock(heapIterator.nextBlock());
    heapIterator.setNextBlock(heapBlockToInsert);
}

//initializes the heap. warning: not thread safe!
inline void heapInit()
{
    BlockLink firstFreeBlock = BlockLink(HEAP_LOCATION + HEAP_ARRAY);
    
    //heapStart holds the pointer to the first item in the list of free blocks
    heapStart.setNextBlock(firstFreeBlock);
    heapStart.setBlockSize(0);
    
    //heapEnd marks the end of the list of free blocks
    heapEnd.setBlockSize(TOTAL_HEAP_SIZE);
    heapEnd.setNextBlock(BlockLink(0));
    
    //we start with a single free block that has all the free memory
    firstFreeBlock.setBlockSize(TOTAL_HEAP_SIZE);
    firstFreeBlock.setNextBlock(heapEnd);
    
    //store the number of free words
    storei(TOTAL_HEAP_SIZE,HEAP_LOCATION,FREE_WORDS_REMAINING);
}


//this is the main malloc routine. returns the global address of the allocated chunk.
raw_global_ptr traxMalloc( int wantedSize )
{
    BlockLink currentBlock, previousBlock, newBlock;
    raw_global_ptr blockAddress = 0;
    int semHandle = 0;
    
    

    //traxSemPost(GLOBAL_SEMAPHORE);
    trax_semacq(GLOBAL_SEMAPHORE);
    {
        //init the heap if not done already
        if( readIfHeapIsInitialized != HEAP_INIT_CODE)
        {
            heapInit();
            storei(HEAP_INIT_CODE,HEAP_LOCATION,HEAP_IS_INITIALIZED);
        }
        
        //increase the size of the wanted block to account for the link header
        if (wantedSize > 0) 
        {
            wantedSize += BLOCKLINK_SIZE;
        }
        
        if ((wantedSize > 0)&&(wantedSize < readFreeWordsRemaining)) 
        {
            previousBlock = heapStart;
            currentBlock = previousBlock.nextBlock();
            
            //free blocks are stored in size order. traverse the list from the smallest (start) 
            //until we find one big enough
            while ((currentBlock.blockSize() < wantedSize) && (currentBlock.nextBlock().address() != 0) ) {
                previousBlock = currentBlock;
                currentBlock = currentBlock.nextBlock();
            }
            //if we got to the end, then no big enough block was found
            if(currentBlock.nextBlock().address() != 0)
            {
                //get the address of the data space
                blockAddress = currentBlock.address() + BLOCKLINK_SIZE;
                
                //delete the current block from the linked list
                previousBlock.setNextBlock(currentBlock.nextBlock());
                
                //if there is extra memory left over, we can add that back to the list
                if ((currentBlock.blockSize() - wantedSize) > HEAP_MIN_BLOCK_SIZE) 
                {
                    //make a new block with the remaining space
                    newBlock = BlockLink(currentBlock.address() + wantedSize);
                    
                    //calculate the size of the new block
                    newBlock.setBlockSize(currentBlock.blockSize() - wantedSize);
                    currentBlock.setBlockSize(wantedSize);
                    
                    heapInsertBlockIntoFreeList( newBlock );
                }
                //update the new free word count
                storei((readFreeWordsRemaining - wantedSize),HEAP_LOCATION,FREE_WORDS_REMAINING);
            }
        }
    }
    trax_semrel(GLOBAL_SEMAPHORE);
    //traxSemFree(GLOBAL_SEMAPHORE);
    
    return blockAddress;
}

//gets the amount of free memory
int getFreeHeapSize()
{
    return loadi(HEAP_LOCATION,FREE_WORDS_REMAINING);
}

//frees allocated memory
void traxFree( raw_global_ptr freeBlock )
{
    raw_global_ptr blockAddress = freeBlock;
    BlockLink newLink;

    if(freeBlock)
    {
        //calculate the address of the blocklink structure
        blockAddress -= BLOCKLINK_SIZE;
#if TRAX==0       
        if(BlockLink(blockAddress).blockSize()<1)
        {
            printf("free called on block: heapsize: %d ",getFreeHeapSize());
            BlockLink(blockAddress).print();
        }
#endif
        
        //add the freed block to the free block list
        //traxSemPost(GLOBAL_SEMAPHORE);
	trax_semacq(GLOBAL_SEMAPHORE);
        {
            heapInsertBlockIntoFreeList(BlockLink(blockAddress));
            //update the new free word count
            storei(((int)readFreeWordsRemaining + (int)BlockLink(blockAddress).blockSize()),HEAP_LOCATION,FREE_WORDS_REMAINING);
        }
	trax_semrel(GLOBAL_SEMAPHORE);
        //traxSemFree(GLOBAL_SEMAPHORE);
    }
}

#endif
