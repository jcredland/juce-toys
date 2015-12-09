
class CallQueueTest
:
public Thread,
public UnitTest
{
public:
    CallQueueTest()
    :
    Thread("Test Thread"),
    UnitTest("Call Queue Tests"),
    queue(1022)
    {
        threadCount = 0;
        threadErrorCount = 0;
        mainCount = 0;
        
        startThread();
        sleep(100);
    }
    
    template <int paddingSize>
    class UpdateObject
    {
    public:
        uint64 nextNumber;
        char padding[paddingSize];
    };
    
    void testNormalOperation()
    {
        beginTest("Normal Operation");
        startThread();
        
        Logger::outputDebugString("Starting...");
        
        Random rand;
        rand.setSeedRandomly();
        
        uint64 queueFullCounter = 0;
       
        
        while (++mainCount < 100000)
        {
            bool success;
            
            do
            {
                if (rand.nextFloat() > 0.5f)
                {
                    success = queue.callf(std::bind(&CallQueueTest::threadSetNumberNum, this, mainCount));
                }
                else
                {
                    UpdateObject<7> object; // Use various size objects here?
                    object.nextNumber = mainCount;
                    auto function = [this, object]()
                    {
                        threadSetNumberObj(object);
                    };
                    
                    success = queue.callf(function);
                }
                
                if (success)
                    break;
                
                queueFullCounter++;
                usleep(10);
            }
            while (! success);
            
            insertRandomDelay(rand);
            usleep(5);
            
            if (mainCount % 10000 == 0)
                Logger::outputDebugString(String(mainCount));
        }
        
        Logger::outputDebugString("Finished:  Queue Was Full " + String(queueFullCounter) + " times");
        
        stopThread(50);
        bool threadStopped = waitForThreadToExit(500);
        expect(threadStopped);
        
        expect(threadCount == (mainCount - 1));
    }
    
    /** Check that the queue full flag actually works. */
    void testQueueFull()
    {
        beginTest("Test Queue Full");
        stopThread(50);
        bool threadStopped = waitForThreadToExit(500);
        expect(threadStopped);
        
        bool success = true;
        
        for (int i = 0; i < 500; ++i)
        {
            UpdateObject<128> object;
            
            auto function = [this, object]()
            {
                threadSetNumberObj(object);
            };
            
            success = queue.callf(function);
        }
        
        expect(success == false);
    }
    
    void runTest() override
    {
        testNormalOperation();
        testQueueFull();
    }
    
    void run()
    {
        Random rand;
        rand.setSeedRandomly();
        
        while (1)
        {
            queue.synchronize();
            insertRandomDelay(rand);
            
            if (threadShouldExit())
            {
                Logger::outputDebugString("Thread finishing ... " + String(threadCount));
                expect(threadErrorCount == 0);  /* Errors are a fail. */
                return;
            }
            
            yield();
        }
    }
    
    template<class T>
    void threadSetNumberObj(T object)
    {
        threadSetNumberNum(object.nextNumber);
    }
    
    void threadSetNumberNum(uint64 nextNumber)
    {
        threadCount++;
        
        if (threadCount != nextNumber)
        {
            threadErrorCount++;
            threadCount = nextNumber;
        }
        
        if (nextNumber % 50000 == 0)
            Logger::outputDebugString("Thread Count Now " + String(threadCount) + " with " + String(threadErrorCount) + " errors.");
    }
    
    /* Random delays are used to ensure the queue is both starved
     and full at various times during the test. */
    void insertRandomDelay(Random & randomEngine)
    {
        if (randomEngine.nextFloat() > 0.99f && useDelay)
            sleep(randomEngine.nextFloat() * 250.0f);
    }
    
    uint64 threadErrorCount;
    uint64 threadCount;
    uint64 mainCount;
    
    bool useDelay;
    LockFreeCallQueue queue;
};
