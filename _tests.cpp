/*
  ==============================================================================

    sample_database.cpp
    Created: 28 Jul 2014 6:37:32pm
    Author:  Jim Credland

  ==============================================================================
*/


/**
 Provides audio-thread access to samples stored or referred to in
 /store/settings/current/samples

 - Samples are indexed 1, 2, 3 ... so they can be referred to by sample
 mapping objects and other places.
 - Samples must be available instantly.
 - When loading a new sample any interval of silence should be as short as possible.
 - We should avoid putting large objects in the inter-thread call queue.
 - Samples must not load in the audio thread.


 - We should aim for a generic object transfer system to the audio thread.
 - Putting things into a ValueTree on the far side makes sense.  Though no listeners on that end.

 */

#define JCF_DEBUG_LEVEL 0

/***********************************************************/

class GrimExampleObject :
    public GarbageCollectedObject
{
public:
    GrimExampleObject()
    {
        if (JCF_DEBUG_LEVEL > 2) DBG ("GrimExampleObject created");

        objCount++;
        myId = counter++;
    }
    ~GrimExampleObject()
    {
        if (JCF_DEBUG_LEVEL > 2) DBG ("GrimExampleObject destroyed");

        objCount--;
    }
    typedef ReferenceCountedObjectPtr<GrimExampleObject> Ptr;
    static int objCount;
    static int counter;
    int myId;
private:
    float data[8192];
};

int GrimExampleObject::objCount = 0;
int GrimExampleObject::counter = 0;

class GrimReaperObjectTest : public UnitTest
{
public:

    GrimReaperObjectTest() :
        UnitTest ("GrimReaperObjectTest") {}

    class CriticalThread :
        public Thread
    {
    public:
        CriticalThread() :
            Thread ("testaudiothread"),
            inBoundQueue (4096 /* size */)
        {
            startThread();
        }
        /** Pretend we are in the audio callback. */
        void run()
        {
            while (!threadShouldExit())
            {
                inBoundQueue.synchronize();

                if (!myObjects.empty())
                {
                    GrimExampleObject::Ptr o = myObjects.top();

                    if (JCF_DEBUG_LEVEL > 2)
                        DBG ("(audioThread) dropping object " + String (o->myId));

                    myObjects.pop();
                }
            }
        }
        /** Method called from message thread. */
        void pushObject (GrimExampleObject::Ptr obj)
        {
            if (JCF_DEBUG_LEVEL > 2)
                DBG ("(mainthread)pushObject objid " + String (obj->myId));

            jassert (inBoundQueue.callf (std::bind (&CriticalThread::audioThreadAddObject, this, obj)));
        }
    private:
        void audioThreadAddObject (GrimExampleObject::Ptr obj)
        {
            if (JCF_DEBUG_LEVEL > 2)
                DBG ("(audioThread)addObject objid " + String (obj->myId));

            myObjects.push (obj);
        }
        LockFreeCallQueue inBoundQueue;
        std::stack<GrimExampleObject::Ptr> myObjects;
    };


    /** Send a bunch of objects to the critical thread
     and check they get deleted on the message thread. */
    void runTest()
    {
        beginTest ("Grim Reaper Critical Thread Reference Counting and Deleting");
        int createdCount = 0;
        {
            CriticalThread criticalThread;
            Random rand = getRandom();
            int t = 200;

            while (--t > 0)
            {
                usleep (rand.nextInt (4000));
                /* Ping the message manager otherwise the timer
                 will never happen :( */
                MessageManager::getInstance()->runDispatchLoopUntil (20);
                int countThisTime = rand.nextInt (20);

                if (JCF_DEBUG_LEVEL > 1) DBG ("Sending " + String (countThisTime) + " objects");

                for (int i = 0; i < countThisTime; ++i)
                {
                    {
                        /* Main deal. Create object and push to another thread. */
                        createdCount++;
                        GrimExampleObject::Ptr exampleObject = new GrimExampleObject();
                        criticalThread.pushObject (exampleObject);

                        if (rand.nextInt (10) > 8) usleep (rand.nextInt (1000)); /* Simulate occasional delays in message thread. */
                    }
                }
            }

            /* Shutdown by letting the timer run for a moment longer to clear any queued deletes. */
            usleep (2000);
            expect (criticalThread.stopThread (200));
        }
        MessageManager::getInstance()->runDispatchLoopUntil (1000);
        DBG ("Objects created " + String (createdCount));
        jassert (GrimExampleObject::objCount == 0);
        expect (GrimExampleObject::objCount == 0);
    }
};


/***********************************************************/


class CriticalThreadValueTreeTest :
    public UnitTest
{
public:

    class CriticalThread :
        public Thread
    {
    public:
        CriticalThread (ValueTree source) :
            Thread ("testaudiothread"),
            inboundQueue (4096 /* size */)
        {
            criticalTree = new CriticalThreadValueTree (source, inboundQueue);
            startThread();
        }

        ~CriticalThread()
        {
            DBG ("CriticalThread Object Destructor");
        }

        /** Pretend we are in the audio callback. */
        void run()
        {
            while (!threadShouldExit())
            {
                inboundQueue.synchronize();
                usleep (200); /* 0.2ms delay. */
            }
        }

        ValueTree& getTreeReference()
        {
            return criticalTree->readonly->getReference();
        }

    private:
        LockFreeCallQueue inboundQueue;
        ScopedPointer<CriticalThreadValueTree> criticalTree;
    };



    CriticalThreadValueTreeTest() :
        UnitTest ("CriticalThreadValueTreeTest")
    { }
    ~CriticalThreadValueTreeTest() {}

    /** Value tree test. */
    void runTest()
    {
        beginTest ("CriticalThreadValueTreeTest");
        {
            ValueTree sourceTree ("test-tree");
            sourceTree.setProperty ("example", "example", nullptr);
            CriticalThread criticalThread (sourceTree);
            Random rand = getRandom();
            int t = 100;
            sourceTree.getOrCreateChildWithName ("subtree", nullptr);
            sourceTree.getChildWithName ("subtree").setProperty ("extra", new GrimExampleObject(), nullptr);

            while (--t >= 0)
            {
                usleep (rand.nextInt (10000));
                /* Simulate dispatch loop for timer. */
                MessageManager::getInstance()->runDispatchLoopUntil (10);
                sourceTree.setProperty ("count", t, nullptr);
                /* Make a number of changes */
                continue; // DEBUG
                int max = rand.nextInt (4);

                for (int i = 0; i < max; ++i)
                {
                    GrimExampleObject* obj = new GrimExampleObject();
                    sourceTree.setProperty ("object", obj, nullptr);

                    if (rand.nextInt (10) > 8) usleep (rand.nextInt (2000)); /* Simulate occasional delays in message thread. */
                }
            }

            sleep (2);
            expect (criticalThread.stopThread (200));
            DBG (criticalThread.getTreeReference().toXmlString());
            DBG (sourceTree.toXmlString());
            expect (criticalThread.getTreeReference().isEquivalentTo (sourceTree));
        }
        MessageManager::getInstance()->runDispatchLoopUntil (1000);
        jassert (GrimExampleObject::objCount == 0);
        expect (GrimExampleObject::objCount == 0);
    }

};

/*************************************************/

juce_ImplementSingleton (GarbageCollector)

static GrimReaperObjectTest reaper_test;
static CriticalThreadValueTreeTest value_tree_test;


