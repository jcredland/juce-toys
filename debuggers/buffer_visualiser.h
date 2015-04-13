/*
 ==============================================================================

 buffer_visualiser.h
 Created: 11 Nov 2014 6:27:09pm
 Author:  Jim Credland

 ==============================================================================
 */

#ifndef BUFFER_VISUALISER_H_INCLUDED
#define BUFFER_VISUALISER_H_INCLUDED



/**
 @file

 This class lets you look at pictures of small buffers (of floats) during
 execution.

 Buffer visualiser allows objects to register buffers which might be of
 interest to the developer.  The buffers are presumed to be smallish so
 we do not provide zoom and scroll facilities.

 The developer can select the named buffers from a list.

 A trigger can be called to cause a particular buffer to be saved for
 debugging.

 The visualiser makes no attempt to be thread safe and just dips into
 data owned by other objects and threads.

 Using?

 1. Insert:

 JCF_DEBUG_BUFFER(name, data, size, min, max)

 e.g.

 JCF_DEBUG_BUFFER("DelayBuffer", delayBuffer, 1024, -1.0f, 1.0f);

 Somewhere in your code. Where delayBuffer is something like float
 delayBuffer[1024] and contains the data you want to inspect.

 2. And instantate a BufferVisualiser::Visualiser somewhere,
 perhaps in your MainComponent private member variables:

 BufferVisualiser::Visualiser visualiser;

 @note
 This code isn't pretty, and the UI isn't pretty - it's for debugging and
 definitely not production use!

 In the future I might modify it to support buffers of doubles and
 other things.
 */

#ifndef JUCE_DEBUG
#define JCF_DEBUG_BUFFER(name, data, size, min, max)
#else
#define JCF_DEBUG_BUFFER(name, data, size, min, max) BufferVisualiser::Store::getInstance()->record(name, data, size, min, max)



namespace BufferVisualiser
{

/** Stores data copied from a buffer, possibly on a different thread. */
class DataSnapshot
    :
    public ReferenceCountedObject
{
public:
    typedef ReferenceCountedObjectPtr<DataSnapshot> Ptr;

    DataSnapshot (const String& name, const float* dataToCopy,
                  int size, float min, float max)
        :

        data (size* sizeof (float), false),
        size (size),
        name (name)
    {
        data.copyFrom (dataToCopy, 0, sizeof (float) * size);
        setMinMax (min, max);
    }
    ~DataSnapshot()
    {

    }

    int getSize() const
    {
        return size;
    }
    void setMinMax (float min, float max)
    {
        scale = 1.0f / (max - min);
        shift = -min;
    }
    String getName() const
    {
        return name;
    }
    float getRaw (int index) const
    {
        jassert (index < size);
        const float* t = (float*) data.getData();
        return t[index];
    }
    float getNormalized (int index) const
    {
        jassert (index < size);
        const float* t = (float*) data.getData();
        return 1.0f - (scale * (shift + t[index]));
    }
private:
    MemoryBlock data;
    int size;
    String name;
    float scale;
    float shift;
};

/** Provides the public interface for adding and removing buffers
 to the store.  As we can't guarantee that the buffer viewer will
 be available this is a singleton.
 */
class Store
    :
    public DeletedAtShutdown
{
public:
    Store()
        :
        paused (false)
    {}

    juce_DeclareSingleton (Store, false)

    ~Store()
    {
        clearSingletonInstance();
    }

    void setPause (bool p)
    {
        ScopedLock lock (dataBufferLock);
        paused = p;
    }

    /** Takes a copy of the buffer and stores it.  Replaces any buffer with the same name. */
    void record (const String& name,
                 float* data, int size, float min, float max)
    {
        {
            ScopedLock lock (dataBufferLock);

            if (paused)
                return;

            DataSnapshot* ds = new DataSnapshot (name, data, size, min, max);

            if (nameToLocation.find (name) == nameToLocation.end())
            {
                nameToLocation[name] = buffers.size();
                buffers.add (ds);
            }
            else
            {
                const int loc = nameToLocation[name];
                buffers.set (loc, ds);
            }
        }

        listeners.call (&Listener::bufferListUpdated);
    }

    /** Appends the buffer to an osciliscope view */
    void oscilloscope (const String& name,
                       float* data, int size, float min, float max)
    {
        ScopedLock lock (dataBufferLock);
        jassertfalse;
        /* Insert code for copying data onto the end of buffer. */
    }

    int size()
    {
        return buffers.size();
    }

    DataSnapshot::Ptr get (int index)
    {
        ScopedLock lock (dataBufferLock);

        return buffers[index];
    }

    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void bufferListUpdated() = 0;
    };

    void addListener (Listener* l)
    {
        listeners.add (l);
    }

    void removeListener (Listener* l)
    {
        listeners.remove (l);
    }

private:
    bool paused;
    std::map<String, int> nameToLocation;

    ReferenceCountedArray<DataSnapshot> buffers;

    ListenerList<Listener> listeners;

    CriticalSection dataBufferLock;
};


class Graph;
class List;
class Info;

/** Displays the buffers. */
class Main
    :
    public Component,
    public Timer,
    public Store::Listener,
    public AsyncUpdater,
    public Button::Listener
{
public:
    Main();
    ~Main();
    void timerCallback()
    {
        repaint();
    }
    void resized() override;

    void bufferListUpdated() override;

    void handleAsyncUpdate() override;

    void paint (Graphics& g) override;

    void buttonClicked (Button* button) override;

    /** Get the currently selected source. */
    DataSnapshot::Ptr getCurrentSnapshot();
private:
    bool paused;
    ScopedPointer<Graph> graph;
    ScopedPointer<List> list;
    ScopedPointer<Info> info;
    TextButton pauseUpdates;
};


class BufferList;

/** Add one of these to your program to see the debug buffers. */
class Visualiser
    :
    public DocumentWindow
{
public:
    Visualiser()
        :
        DocumentWindow ("Debug Buffer Viewer",
                        Colours::lightgrey,
                        DocumentWindow::allButtons)
    {
        mainComponent.setSize (500, 250);
        setContentNonOwned (&mainComponent, true);
        setResizable (true, false);
        setUsingNativeTitleBar (true);

        setVisible (true);
    }
    ~Visualiser() {}
private:
    Main mainComponent;

};
};

#endif // JUCE_DEBUG

#endif  // BUFFER_VISUALISER_H_INCLUDED
