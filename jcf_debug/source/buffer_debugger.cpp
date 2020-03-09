/*
 ==============================================================================

 buffer_visualiser.cpp
 Created: 11 Nov 2014 6:27:09pm
 Author:  Jim Credland

 ==============================================================================
 */
class Graph;
class List;
class Info;
class BufferDebuggerMain;

/** Provides the public interface for adding and removing buffers
 to the store.  As we can't guarantee that the buffer viewer will
 be available this is a singleton.
 */
class BufferDebuggerStore
    :
    public DeletedAtShutdown
{
public:
    class DataSnapshot
        :
        public ReferenceCountedObject
    {
    public:
        typedef ReferenceCountedObjectPtr<DataSnapshot> Ptr;

        /** DataSnapshot holds a single copy of the buffer. */
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
        {}

        int getSize() const { return size; }

        void setMinMax (float min, float max)
        {
            scale = 1.0f / (max - min);
            shift = -min;
        }

        String getName() const { return name; }

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



    BufferDebuggerStore()
        :
        paused (false)
    {}

    juce_DeclareSingleton (BufferDebuggerStore, false)

    ~BufferDebuggerStore()
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
                 const float* data, int size, float min, float max)
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

    void addListener (Listener* l) { listeners.add (l); }

    void removeListener (Listener* l) { listeners.remove (l); }

private:
    bool paused;

    std::map<String, int> nameToLocation;

    ReferenceCountedArray<DataSnapshot> buffers;

    ListenerList<Listener> listeners;

    CriticalSection dataBufferLock;
};

juce_ImplementSingleton (BufferDebuggerStore)


/** Main component of our document window.  Displays the buffers. */
class BufferDebuggerMain
    :
    public Component,
    public Timer,
    public BufferDebuggerStore::Listener,
    public AsyncUpdater,
    public Button::Listener
{
public:
    /** draws the selected data source(s) on a graph. */
    class Graph
        :
        public Component
    {
    public:
        Graph (BufferDebuggerMain& owner)
            :
            mouseOverGraph (false),
            mouseX (0),
            mouseY (0),
            owner (owner),
            src (nullptr)
        {}

        ~Graph() {}

        void update()
        {
            src = owner.getCurrentSnapshot();
            updateSizing();
        }

        void resized()
        {
            updateSizing();
        }

        void updateSizing()
        {
            if (! src) return;

            entriesPerPixel = float (src->getSize()) / float (getWidth());

            repaint();
        }

        void paint (Graphics& g)
        {
            g.fillAll (Colours::lightgrey);

            if (! src)
                return;

            const int w = getWidth();
            const float h = float (getHeight());
            g.setColour (Colours::blue);

            for (int x = 0; x < (w - 1); ++x)
            {
                float a, b;
                float idx = getIndexForX (x);
                float idx2 = getIndexForX (x + 1);
                getMinMaxForPosition (idx, idx2, a, b);

                g.drawLine (float (x), a * h, x + 1.0f, b * h);

                if (mouseOverGraph && x == mouseX)
                {
                    g.setColour (Colours::red);
                    g.drawLine (float (x) + 0.5f, 0.0, float (x) + 0.5f, getHeight());
                    g.setColour (Colours::blue);
                }
            }

            if (mouseOverGraph)
            {
                g.setColour (Colours::red);
                g.setFont (11.0f);
                const String comment = "index = " + String (getIndexForX (mouseX))
                                       + "   value = " + String (src->getRaw (getIndexForX (mouseX)));

                g.drawText (comment, 4, 4, getWidth() - 8, 20, Justification::left, false);
            }
        }

        void mouseMove (const MouseEvent& e)
        {
            mouseX = e.x;
            mouseY = e.y;
            repaint();
        }

        void mouseEnter (const MouseEvent& e)
        {
            mouseOverGraph = true;
        }

        void mouseExit (const MouseEvent& e)
        {
            mouseOverGraph = false;
        }



        float getIndexForX (int x)
        {
            return entriesPerPixel * x;
        }

        void getMinMaxForPosition (float stloc, float endloc,
                                   float& min, float& max)
        {
            int st = stloc;
            int ed = endloc;
            /* This looks like a C++11 ism. */
            max = std::numeric_limits<float>::lowest();
            min = (std::numeric_limits<float>::max) ();

            for (int i = st; i <= ed; i++)
            {
                const float a = src->getNormalized (i);

                if (a < min)
                    min = a;

                if (a > max)
                    max = a;
            }
        }

    private:
        bool mouseOverGraph;
        int mouseX, mouseY;
        BufferDebuggerMain& owner;
        BufferDebuggerStore::DataSnapshot::Ptr src;
        float entriesPerPixel;
    };

    /** displays information on the last selected data source. */
    class Info
        :
        public Component
    {
    public:
        Info (BufferDebuggerMain& owner)
            :
            owner (owner)
        {
            addAndMakeVisible (info);
            info.setReadOnly (true);
            info.setMultiLine (true);
            info.setFont (Font ("Courier", 12.0f, 0));
        }

        void resized()
        {
            info.setBounds (getLocalBounds());
        }

        void paint (Graphics& g)
        {
            g.fillAll (Colours::lightgrey);
        }

        void update()
        {
            BufferDebuggerStore::DataSnapshot::Ptr s = owner.getCurrentSnapshot();

            if (! s)
                return;

            String t;

            t += "Name: " + s->getName();
            t += "\n";
            t += "Size: " + String (s->getSize());
            t += "\n";

            float max = std::numeric_limits<float>::lowest();
            float min = std::numeric_limits<float>::max();

            for (int i = 0; i < s->getSize(); ++i)
            {
                float a = s->getRaw (i);
                min = jmin (min, a);
                max = jmax (max, a);
            }

            t += "Min : " + String (min);
            t += "\n";
            t += "Max : " + String (max);
            t += "\n";

            info.setText (t);
            repaint();
        }

    private:
        BufferDebuggerMain& owner;
        TextEditor info;
    };

    /** displays a list of available data sources and stored objects. */
    class List
        :
        public Component,
        public ListBoxModel
    {
    public:
        List (BufferDebuggerMain& owner)
            :
            owner (owner)
        {
            addAndMakeVisible (list);
            list.setModel (this);
            list.setRowHeight (13.0f);
        }

        int getNumRows() override
        {
            return BufferDebuggerStore::getInstance()->size();
        }

        void paintListBoxItem (int rowNumber, Graphics& g,
                               int width, int height, bool rowIsSelected) override
        {
            String s = BufferDebuggerStore::getInstance()->get (rowNumber)->getName();

            if (rowIsSelected)
                g.fillAll (Colours::red);

            g.setColour (Colours::black);
            g.drawText (s, 0, 0, width, height, Justification::left, false);
        }

        void listBoxItemClicked (int row, const MouseEvent& e) override
        {
            owner.bufferListUpdated();
        }

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::lightgrey);
        }

        void resized() override
        {
            list.setBounds (getLocalBounds());
        }

        void update()
        {
            list.updateContent();

            repaint();
        }

        BufferDebuggerStore::DataSnapshot::Ptr getCurrentSelection()
        {
            int row = list.getSelectedRow();
            BufferDebuggerStore* s = BufferDebuggerStore::getInstance();

            if (!s || row < 0 || row > s->size())
                return nullptr;

            return s->get (row);
        }
    private:
        BufferDebuggerMain& owner;
        ListBox list;
    };


    BufferDebuggerMain();

    ~BufferDebuggerMain();

    void timerCallback() override
    {
        repaint();
    }

    void resized() override;

    void bufferListUpdated() override;

    void handleAsyncUpdate() override;

    void paint (Graphics& g) override;

    void buttonClicked (Button* button) override;

    /** Get the currently selected source. */
    BufferDebuggerStore::DataSnapshot::Ptr getCurrentSnapshot()
    {
        return list->getCurrentSelection();
    }
private:
    bool paused;
    std::unique_ptr<Graph> graph;
    std::unique_ptr<List> list;
    std::unique_ptr<Info> info;
    TextButton pauseUpdates;
};


BufferDebugger::BufferDebugger()
    :
    DocumentWindow ("Debug Buffer Viewer",
        Colours::lightgrey,
        DocumentWindow::allButtons),
    mainComponent (new BufferDebuggerMain)
{
    mainComponent->setSize (500, 250);
    setContentNonOwned (mainComponent.get(), true);
    setResizable (true, false);
    setUsingNativeTitleBar (true);

    setVisible (true);
}

BufferDebugger::~BufferDebugger()
{}


void BufferDebugger::capture (const String& name, const float* dataToCapture, int size, float min, float max)
{
    BufferDebuggerStore::getInstance()->record (name, dataToCapture, size, min, max);
}



BufferDebuggerMain::BufferDebuggerMain()
    :
    paused (false)
{
    list = std::make_unique<List>(*this);
    graph = std::make_unique<Graph>(*this);
    info = std::make_unique<Info>(*this);

    pauseUpdates.setButtonText ("Pause");
    pauseUpdates.addListener (this);

    addAndMakeVisible (pauseUpdates);
    addAndMakeVisible (graph.get());
    addAndMakeVisible (info.get());
    addAndMakeVisible (list.get());


    startTimer (100);
    BufferDebuggerStore::getInstance()->addListener (this);
}

BufferDebuggerMain::~BufferDebuggerMain()
{
    BufferDebuggerStore::getInstance()->removeListener (this);
}

void BufferDebuggerMain::buttonClicked (Button* button)
{
    if (button == &pauseUpdates)
    {
        paused = ! paused;
        BufferDebuggerStore::getInstance()->setPause (paused);
        pauseUpdates.setButtonText (paused ? "Resume" : "Pause");
    }
}

void BufferDebuggerMain::resized()
{
    int h2 = getHeight() / 2;
    int w2 = getWidth() / 2;
    const int pad = 4;

    Rectangle<int> a (0, 0, getWidth(), h2);
    graph->setBounds (a.reduced (pad));

    Rectangle<int> b (0, h2, w2, h2);
    list->setBounds (b.reduced (pad));

    Rectangle<int> c (w2, h2, w2, h2);
    info->setBounds (c.reduced (pad).withTrimmedBottom (20));

    pauseUpdates.setBounds (c.withTop (c.getBottom() - 20));
}

void BufferDebuggerMain::paint (juce::Graphics& g)
{
    g.fillAll (Colours::darkgrey);
}

void BufferDebuggerMain::bufferListUpdated()
{
    triggerAsyncUpdate();
}

void BufferDebuggerMain::handleAsyncUpdate()
{
    graph->update();
    info->update();
    list->update();
}


