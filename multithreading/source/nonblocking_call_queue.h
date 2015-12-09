/*
  ==============================================================================

    audiothread_fifo.h
    Created: 20 Jan 2014 11:34:38am
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef AUDIOTHREAD_FIFO_H_INCLUDED
#define AUDIOTHREAD_FIFO_H_INCLUDED


/**
 * @brief Allows a function call to be executed on a different thread, in a fast
 * lock-safe and thread-safe manner. 
 *
 * A function call queue which enables functions to be called asychronously on
 * another thread.  This is a one-reader, one-writer FIFO.  You should be using
 * callf() from a single thread, and calling synchronize on a different single
 * thread.  If you need two threads to communicate in both directions in a lock
 * free fashion then use two LockFreeCallQueue objects.  
 *
 * It has the following special features: 
 *   - No locking 
 *   - Avoids using the system allocator except during the constructor.  
 *
 *   Watch out for: 
 *   - Objects you pass as function arguments, which are passed by value, doing
 *   memory allocation, which may result in a lock.
 *
 * ## Unit Tests
 * A unit-test is provided in the _tests directory which be useful reference.
 */

class LockFreeCallQueue
{
public:
    LockFreeCallQueue (int RingBufferSize)
        :
        fifo (RingBufferSize),
        acceptingJobs (true)
    {
        // TODO: data should be aligned to a cache line boundary.
        // Allocate double size buffer to easily support variable length messages,
        // by hanging them over the end of the buffer.
        fifodata = new char[ RingBufferSize * 2 ];
    }

    ~LockFreeCallQueue()
    {
        delete [] fifodata;
    }

    /** @brief return true if the queue is empty. */
    bool isEmpty()
    {
        return fifo.getTotalSize() == fifo.getFreeSpace();
    }

    /** @brief Return the amount of free space in the queue. 
     *
     * If the queue does not have enough free space your callf() call will be 
     * dropped which may be a problem.  You may want to check the result of 
     * this function call during debugging. And bear in mind there are no 
     * guarantees of it not filling up during operation - so if something is
     * critical and your application will fail if it doesn't get through you 
     * may need to think about some signalling back. 
     */
    int getFreeSpace()
    {
        return fifo.getFreeSpace();
    }

    /**
     * @brief Calls a function, via the queue, on a different thread. 
     *
     * Add a function to the queue.  Use std::bind to call the function like
     * this:

    @code
    class MyClass {
    public:
        void queueUpdateJob() {
            queue.callf(std::bind(&MyClass::doSomething, this, i, j);
        }
    private:
        void update(int i, int j) {
            // stuff happens.
        }
        LockFreeCallQueue queue;
    };
    @endcode


    */
    template <class Functor>
    bool callf (Functor const& f)
    {
        if (! acceptingJobs) 
            return false;

        /* allocSize cannot be bigger than 2Gb ok!. */
        // jassert (sizeof (WorkItem<Functor>) < std::numeric_limits<int>::max());

        const int allocSize = static_cast<int>(roundUpToCacheLineBoundary (sizeof (WorkItem<Functor>)));
        int idx1, idx2, sz1, sz2;
        fifo.prepareToWrite (allocSize, idx1, sz1, idx2, sz2);

        if (sz1 + sz2 < allocSize) 
            return false;

        /* double size buffer means we can ignore idx2 and sz2.  Weird syntax for new is a 'placement
         * new'.  Familar only if you write custom allocators regularly. */
        new (fifodata + idx1) WorkItem<Functor> (f);
        fifo.finishedWrite (allocSize);

        return true;
    }

    /** @brief Execute all the calls in the queue. 
     *
     * Call this function in the target thread.  When this function is called
     * all the calls added with callf will be executed. 
     *
     * @returns true if there was anything in the queue, false if the queue was
     * empty. 
     * */
    bool synchronize()
    {
        bool didSomething = false;

        while (fifo.getNumReady() > 0)
        {
            int idx1, idx2, sz1, sz2;
            fifo.prepareToRead (1, idx1, sz1, idx2, sz2);
            didSomething = true;
            Work* w = reinterpret_cast<Work*> (fifodata + idx1);
            /* notice only one function pointer invocation here, not two virtual function calls. */
            const int sizeofWorkItem = (*w->execAndDestructFn) (w);
            const int allocSize = roundUpToCacheLineBoundary (sizeofWorkItem);
            fifo.finishedRead (allocSize);
        }

        return didSomething;
    }

    /** Disables the this LockFreeCallQueue. You may need to use this during shutdown to avoid
     * threads continuing to put work, and objects, into a queue that no longer has anyone executing 
     * it.  It's probably not a big deal in an application, but in a plugin you could end up with 
     * memory leaks or or stuff if resources are being held by objects in the queue. 
     * @todo: put in a proper example of when this might be important.
     */
    void stop()
    {
        acceptingJobs = false;
    }
private:
    /* Avoid a vtable and two separate virtual function dispatches (operator() and destructor) by putting
     everything into a single function and implementing our own virtual function call. */

    /** 
     Type for pointer to a function that executes work, calls the destructor for the and
     returns size of concrete instance.
     */
    typedef int (*WorkExecAndDestructFunctionPtr) (void* workItemStorage);

    class Work
    {
        // NOTE: no vtable.
    public:
        Work (WorkExecAndDestructFunctionPtr f) :
            execAndDestructFn (f)
        { }
        WorkExecAndDestructFunctionPtr execAndDestructFn;
    };

    /** WorkItem template - extends Work for each type of Functor */
    template <class Functor>
    struct WorkItem : public Work
    {
        explicit WorkItem (Functor const& fun) :
            Work (&WorkItem::myExecAndDestruct),
            myCall (fun)
        {}

    private:
        static int myExecAndDestruct (void* workItemStorage)
        {
            /* cast to *concrete* work item pointer of this template type */
            WorkItem* that = reinterpret_cast<WorkItem*> (workItemStorage);
            that->myCall();
            that->~WorkItem(); // invoke concrete dtor (destructs functor)
            return static_cast<int>(sizeof (WorkItem));
        }

        Functor myCall;
    };

    /* For clarity (maybe) what's happening here is:
     1  There are instances of the templated class WorkItem for each type of Functor, and hence
        multiple versions of myExecAndDestruct.
     2  When the WorkItem is created the Work::execAndDestructFn pointer is set to
        point to the appropriate instance of myExecAndDestruct - the pointer is passed
        to the Work superclass constructor by the WorkItem<Functor> class.

     To call we:
     -  Get the data from the fifo.
     -  Cast it to be a Work.
     -  Call the appropriate instance of myExecAndDestruct
        via the execAndDestructFn pointer.
    */

    AbstractFifo fifo;
    char* fifodata;

    /* Used to avoid false sharing and to give correct alignment to embedded structs. */
    int roundUpToCacheLineBoundary (int x)
    {
        return x; // TODO
    }

    bool acceptingJobs;
};





#endif  // AUDIOTHREAD_FIFO_H_INCLUDED
