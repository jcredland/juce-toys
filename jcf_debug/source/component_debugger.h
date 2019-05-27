
#ifndef BigKickTrigger_credland_component_debugger_h
#define BigKickTrigger_credland_component_debugger_h

/*
 * (c) Credland Technical Limited.
 * MIT License
 *
 * JCF_DEBUG - Debugging helpers for JUCE.  Demo application.
 *
 * Don't forget to install the VisualStudio or Xcode debug scripts as
 * well.  These ensure that your IDEs debugger displays the contents
 * of ValueTrees, Strings and Arrays in a useful way!
 *
 *
 * Credland Technical Limited provide a range of consultancy and contract
 * services including:
 * - JUCE software development and support
 * - information security consultancy
 *
 * Contact via http://www.credland.net/
 */


/**
 Include one of these and attach it to a component you want to use
 as the root (usually your main window) and a second window will
 appear showing a tree view of all your components.

 e.g.

 std::unique_ptr<jcf::ComponentDebugger> debugger;

 Then something like:

 MainComponent::MainComponent()
 {
    debugger = std::make_unique<jcf::ComponentDebugger>(this);
 }

 When you click on a component in the debugger's list a box will
 be drawn on your software showing the bounds of the component.

 Components marked as not-visible will be shown in grey.

 Components with zero sizes will be highlighted in red.

 Components partially or completely outside their parents bounds
 will be marked in yellow.
 */

class ComponentBoundsEditor;

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
        ComponentTreeViewItem (Debugger* owner, Component* c);

        /* Standard JUCE overrides. */
        bool mightContainSubItems() override;
        void paintItem (Graphics& g, int w, int h) override;
        void itemSelectionChanged (bool nowSelected) override;

    private:
        String name;
        String type;
        bool isVisible;

        int numChildren;

        Debugger* owner;

        bool zeroSizeFlag;
        bool outsideBoundsFlag;

        WeakReference<Component> component;
    };

    /**
     * The main debugger component.  Attached to the ComponentDebugger
     * document window.
     */
    class Debugger
        :
        public Component,
        public Button::Listener
    {
    public:
        Debugger (Component* rootComponent);

        ~Debugger();

        void buttonClicked (Button*) override;
        void refresh();

        /**
         * Returns the location of the component in the
         * root component's coordinate space.
         */
        Rectangle<int> getLocationOf (Component* c);

        /**
         * Highlighter is a component which draws over your existing
         * application to show where a selected component is
         * positioned.
         *
         * @todo - allow the component to be dragged using this.
         */
        class Highlighter
            :
            public Component
        {
        public:
            Highlighter()
            {
                setInterceptsMouseClicks (false, false);
            }
            void paint (Graphics& g)
            {
                g.setColour (Colours::red);
                g.drawRect (getLocalBounds(), 2.0f);
            }
        };

        void setComponentToEdit (Component*);

        void setHighlight (Component* component)
        {
            highlight = std::make_unique<Highlighter>();
            highlight->setBounds (getLocationOf (component));
            root->addAndMakeVisible (highlight.get());
        }

    private:
        void resized() override;
        void paint (Graphics& g) override { g.fillAll (Colours::lightgrey); }
        void copyBoundsToClipboard();

        std::unique_ptr<Highlighter> highlight;
        std::unique_ptr<ComponentBoundsEditor> boundsEditor;

        /* We use a default look and feel just in case your
         * application has some wacky and unreadable alternative
         * in use. */
        LookAndFeel_V3 lookAndFeel;

        Component* root;

        TextButton refreshButton;
        TextButton copyBoundsButton;
        TextButton hideHighlightButton;
        TreeView tree;

        JUCE_DECLARE_NON_COPYABLE (Debugger)
    };

    /**
     Show a component debugger, using root as the starting point.
     */
    ComponentDebugger (Component* root)
        :
        DocumentWindow ("Component Debugger", Colours::black, 7, true),
        debugger (root)
    {
        debugger.setSize (400, 400);
        setContentNonOwned (&debugger, true);
        setResizable (true, false);
        setUsingNativeTitleBar (true);
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        /**
         * It's a little bit unclear what we should do here,
         * as the user may be managing the lifetime of the
         * component and we want to keep the interface very
         * simple So we will just hide  ourselves.
         *
         * Typically - this will be fine.
         */
        setVisible(false);
    }

private:
    Debugger debugger;
};


#endif
