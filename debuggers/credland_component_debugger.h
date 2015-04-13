
#ifndef BigKickTrigger_credland_component_debugger_h
#define BigKickTrigger_credland_component_debugger_h

/* You may need to uncomment the following line. */
//#include "../JuceLibraryCode/JuceHeader.h"

/** 
 Include one of these and attach it to a component you want to use as the root (usually your main window)
 and a second window will appear showing a tree view of all your components.
 
 When you click on a component a box will be drawn on your software showing the bounds of the component. 
 
 Components marked as not-visible will be shown in grey. 
 
 Components with zero sizes will be highlighted in red. 
 
 Components partially or completely outside their parents bounds will be marked in yellow.
 
 The whole tree is rebuilt everytime you click refresh - though I might modify it to use a WeakReference
 and update in real-time at some point.
 
 @note
 This code isn't pretty, and the UI isn't pretty - it's for debugging and definitely not
 production use!
 */

class ComponentDebugger
:
public DocumentWindow
{
public:
    class Debugger;

    class ComponentTreeViewItem
    :
    public TreeViewItem
    {
    public:
        ComponentTreeViewItem(Debugger * owner, Component * c);

        bool mightContainSubItems() override
        {
            return numChildren > 0;
        }
        void paintItem(Graphics & g, int w, int h)
        {
            if (isVisible)
                g.setColour(Colours::black);
            else
                g.setColour(Colours::grey);

            if (zeroSizeFlag)
                g.fillAll(Colours::red.withAlpha(0.5f));
            else if (outsideBoundsFlag)
                g.fillAll(Colours::yellow.withAlpha(0.5f));

            g.drawText(type + ": " + name  + " bounds: " + bounds, 0, 0, w, h, Justification::left, true);
        }

        void itemSelectionChanged(bool nowSelected) override;
    private:
        String name;
        String type;
        bool isVisible;
        String bounds;
        int numChildren;
        Debugger * owner;
        Rectangle<int> location;
        bool zeroSizeFlag;
        bool outsideBoundsFlag;
    };

    class Debugger
    :
    public Component,
    public Button::Listener
    {
    public:
        Debugger(Component * rootComponent)
        :
        root(rootComponent),
        refreshButton("Refresh")
        {
            addAndMakeVisible(refreshButton);
            addAndMakeVisible(tree);
            refreshButton.addListener(this);
            refresh();
            tree.setLookAndFeel(&lookAndFeel);
            refreshButton.setLookAndFeel(&lookAndFeel);
        }

        void buttonClicked(Button *) override
        {
            refresh();
        }

        void resized() override
        {
            refreshButton.setBounds(0, 0, 130, 20);
            tree.setBounds(0, 20, getWidth(), getHeight());
        }

        void paint(Graphics & g) override
        {
            g.fillAll(Colours::lightgrey);
        }

        void refresh()
        {
            tree.setRootItem(new ComponentTreeViewItem(this, root));
        }

        Rectangle<int> getLocationOf(Component * c)
        {
            return root->getLocalArea(c, c->getLocalBounds());
        }

        class Highlighter : public Component
        {
        public:
            void paint(Graphics & g)  { g.setColour(Colours::red); g.drawRect(getLocalBounds()); }
        };

        void setHighlight(Rectangle<int> bounds)
        {
            highlight = new Highlighter();
            highlight->setBounds(bounds);
            root->addAndMakeVisible(highlight);
        }

    private:
        ScopedPointer<Highlighter> highlight;
        LookAndFeel_V3 lookAndFeel;
        Component * root;
        TextButton refreshButton;
        TreeView tree;

        JUCE_DECLARE_NON_COPYABLE(Debugger)
    };

    ComponentDebugger(Component * root)
    :
    DocumentWindow("Component Debugger", Colours::black, 7, true),
    debugger(root)
    {
        debugger.setSize(400, 400);
        setContentNonOwned(&debugger, true);
        setResizable (true, false);
        setUsingNativeTitleBar (true);
        setVisible (true);
    }

private:
    Debugger debugger;
};




#endif
