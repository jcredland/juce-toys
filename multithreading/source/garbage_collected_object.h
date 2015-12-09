
#ifndef GARBAGECOLLECTOR_H_INCLUDED
#define GARBAGECOLLECTOR_H_INCLUDED

/**
 * @mainpage Credland Multithreading
 *
 * This JUCE module contains tools for handling lock-free concurrency and data
 * issues in plugins.  In particular: 
 * - A lock free call queue
 * - Simple garbage collection 
 * - A unidirectional lock-free ValueTree synchroniser
 *   
 * @note Many thanks to Ross Bencina for the help with the call queue.
 */

/**
 Hold and destroy ReferenceCountedObjects on the message thread.  You shouldn't
 have to interact with this object directly. Instead use GarbageCollectedObject.

 It's a singleton running in the background which every 150ms quickly checks
 to see if it can delete any objects.
 */
class GarbageCollector :
    Timer
{
public:
    juce_DeclareSingleton (GarbageCollector, false)
    GarbageCollector (int timerInterval = 150)
    {
        startTimer (timerInterval);
    }
    ~GarbageCollector()
    {
        clearSingletonInstance();
        /* If objects in our stack own other objects we might
         need to do a lot of clearing up. */
        int count = 100;

        while (data.size() > 0 && count--) timerCallback();

        jassert (data.size() == 0);
    }
    /** Add an object to be garbage collected. */

    bool isInList (ReferenceCountedObject* o)
    {
        return (std::find (data.begin(), data.end(), o) != data.end());
    }
private:
    static void add (ReferenceCountedObject* o)
    {
        getInstance()->addObject (o);
    }
    void addObject (ReferenceCountedObject* o)
    {
        jassert (! isInList (o)); /* duplicate. */
        o->incReferenceCount();
        data.push_back (o);
    }

    void timerCallback()
    {
        data.remove_if ([] (ReferenceCountedObject * o)
        {
            if (o->getReferenceCount() == 1)
            {
                o->decReferenceCount(); /* Will delete it too! */
                return true;
            }

            return false;
        });
    }
    std::list<ReferenceCountedObject*> data;
    friend class GarbageCollectedObject;
};

/**
 * @brief Allows objects to be created and destroyed on a non-critical thread
 * but passed over to a critical thread for use.  Ideal for some audio
 * applications.
 *
 * ## Problem
 * Because the system memory allocator probably uses a lock, it's at least
 * theoretically possible to get a long delay when you create or delete an
 * object.  This could cause problems with real-time application, dropped data
 * or glitches.
 *
 * See Ross's page:
 *
 * http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing
 *
 * ## Solution
 * Creating objects on the message thread avoids a possible memory allocation
 * lock.  They then need to be passed safely to the critical thread.  After
 * they are finished being used they should be deleted on the non-critical
 * thread.
 *
 * The GarbageCollector handles this last part.
 *
 * ## How 
 * You 
 * 1. Create GarbageCollectedObjects on the message thread.  
 *
 * 2. Then put them in a suitable container (juce::var,
 * juce::ReferenceCountedObjectPtr) to ensure the reference counting is managed
 * for you.  
 *
 * 3. And pass them in a suitable thread-safe manner, e.g. using
 * LockFreeCallQueue or AbstractFifo to the audio thread.
 *
 * 4. GarbageCollectedObjects are deleted by the GarbageCollector singleton
 * automatically when no longer required.
 *
 * ## Notes
 
 The GarbageCollector will keep a reference to the object as well. And, when it
 detects that the total reference count has decreased to one, it'll delete the
 object safely on the message thread.
 
 NOTE: It's important to (a) only create the objects on the message thread, and
 to (b) put them straight into a Reference Counting container.  Otherwise the
 object may be deleted on the next timer call to GarbageCollector. (Why? The
 Timer which triggers the garbage collector is on the message thread. If you
 created the object on a different thread the Timer might fire whilst you 
 are holding the object with a reference count of 0, and then it'll be deleted.
 Creating it on the message thread and putting it straight into a container
 means the ref-count is guaranteed to be incremented, preventing this embarrassing
 and fatal situation.).
 
 IMPORTANT: If you are using this to share an object with another thread, DO NOT 
 CHANGE the object after you've shared it.  Think of it as immutable.  Best to 
 code it as: all methods to be const or thread-safe and lock-free.
 
 It would be nice to get rid of constraint (a).

*/
class GarbageCollectedObject :
    public ReferenceCountedObject
{
public:
    GarbageCollectedObject()
    {
        GarbageCollector::add (this);
        /* If you create the object on something other than the message thread
         * there's a possible race condition.

         TODO: Fix this ... */
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
    }
    ~GarbageCollectedObject()
    {
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
    }
};




#endif  // SAMPLE_DATABASE_H_INCLUDED
