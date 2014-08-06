/*
  ==============================================================================

    sample_database.h
    Created: 28 Jul 2014 6:37:32pm
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef SAMPLE_DATABASE_H_INCLUDED
#define SAMPLE_DATABASE_H_INCLUDED

#include <list>

/**
 Hold and destroy ReferenceCountedObjects on the message thread.
 You shouldn't have to interact with this object directly most
 of the time. Instead @see GarbageCollectedObject.
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

 GarbageCollectedObjects are deleted by the GarbageCollector singleton
 automatically when no longer required.
 
 Use them when you need a ReferenceCountedObject on the audio thread.
 
 Because the system memory allocator probably uses a lock, it's possible to get
 a long delay when you create or delete an object.
 
 So instead you can create GarbageCollectedObjects on a non-critical thread.
 
 The put them in a suitable container (@see var, @see Value, @see
 ReferenceCountedObjectPtr) to ensure the reference counting is managed for you.
 
 And pass them in a suitable thread-safe manner (@see LockFreeManualQueue, @see
 AbstractFifo) to the audio thread.
 
 The GarbageCollector will keep a reference to the object as well. And, when it
 detects that the total reference count has decreased to one, it'll delete the
 object safely on the message thread.
 
 NOTE: It's important to (a) only create the objects on the message thread, and
 to (b) put them straight into a Reference Counting container.
 
 IMPORTANT: If you are using this to share an object with another thread, DO NOT 
 CHANGE the object after you've shared it.  Think of it as immutable.  Best to 
 code it as: all methods to be const or thread-safe and lock-free.
 
 Otherwise the object may be deleted on the next timer call to GarbageCollector.
 
 It would be nice to get rid of constraint (a).

 See
 http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing


*/
class GarbageCollectedObject :
    public ReferenceCountedObject
{
public:
    GarbageCollectedObject()
    {
        GarbageCollector::add (this);
        /* If you create the object on something other than the
         message thread there's a possible race condition.

         TODO: Fix this ... */
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
    }
    ~GarbageCollectedObject()
    {
        jassert (MessageManager::getInstance()->isThisTheMessageThread());
    }
};




#endif  // SAMPLE_DATABASE_H_INCLUDED
