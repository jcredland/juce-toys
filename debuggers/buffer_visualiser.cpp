/*
 ==============================================================================

 buffer_visualiser.cpp
 Created: 11 Nov 2014 6:27:09pm
 Author:  Jim Credland

 ==============================================================================
 */


#ifdef JUCE_DEBUG

namespace BufferVisualiser
{

juce_ImplementSingleton (Store)

/** draws the selected data source(s) on a graph. */
class Graph
    :
    public Component
{
public:
    Graph (Main& owner)
        :
        mouseOverGraph (false),
        mouseX (0),
        mouseY (0),
        owner (owner),
        src (nullptr)
    {

    }

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
    Main& owner;
    DataSnapshot::Ptr src;
    float entriesPerPixel;


};

/** displays information on the last selected data source. */
class Info
    :
    public Component
{
public:
    Info (Main& owner)
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

        DataSnapshot::Ptr s = owner.getCurrentSnapshot();

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
    Main& owner;
    TextEditor info;
};

/** displays a list of available data sources and stored objects. */
class List
    :
    public Component,
    public ListBoxModel
{
public:
    List (Main& owner)
        :
        owner (owner)
    {
        addAndMakeVisible (list);
        list.setModel (this);
        list.setRowHeight (13.0f);
    }

    int getNumRows()
    {
        return Store::getInstance()->size();
    }

    void paintListBoxItem (int rowNumber, Graphics& g,
                           int width, int height, bool rowIsSelected)
    {
        String s = Store::getInstance()->get (rowNumber)->getName();

        if (rowIsSelected)
            g.fillAll (Colours::red);

        g.setColour (Colours::black);
        g.drawText (s, 0, 0, width, height, Justification::left, false);
    }

    void listBoxItemClicked (int row, const MouseEvent& e) override
    {
        owner.bufferListUpdated();
    }

    void paint (Graphics& g)
    {
        g.fillAll (Colours::lightgrey);
    }

    void resized()
    {
        list.setBounds (getLocalBounds());
    }

    void update()
    {
        list.updateContent();

        repaint();
    }

    DataSnapshot::Ptr getCurrentSelection()
    {
        int row = list.getSelectedRow();
        Store* s = Store::getInstance();

        if (!s || row < 0 || row > s->size())
            return nullptr;

        return s->get (row);
    }
private:
    Main& owner;
    ListBox list;
};

Main::Main()
    :
    paused (false)
{
    list = new List (*this);
    graph = new Graph (*this);
    info = new Info (*this);

    pauseUpdates.setButtonText ("Pause");
    pauseUpdates.addListener (this);

    addAndMakeVisible (pauseUpdates);
    addAndMakeVisible (graph);
    addAndMakeVisible (info);
    addAndMakeVisible (list);


    startTimer (100);
    Store::getInstance()->addListener (this);
}

Main::~Main()
{
    Store::getInstance()->removeListener (this);
}

void Main::buttonClicked (Button* button)
{
    if (button == &pauseUpdates)
    {
        paused = ! paused;
        Store::getInstance()->setPause (paused);
        pauseUpdates.setButtonText (paused ? "Resume" : "Pause");
    }
}

void Main::resized()
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

void Main::paint (juce::Graphics& g)
{
    g.fillAll (Colours::darkgrey);
}

void Main::bufferListUpdated()
{
    triggerAsyncUpdate();
}

void Main::handleAsyncUpdate()
{
    graph->update();
    info->update();
    list->update();
}

DataSnapshot::Ptr Main::getCurrentSnapshot()
{
    return list->getCurrentSelection();
}

};

#endif
