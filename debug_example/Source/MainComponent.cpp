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

#include "MainComponent.h"

/**
 * SampleComponent displays a coloured box so we can demonstrate both the
 * ComponentDebugger and the ValueTreeEditor.
 *
 * It has a button attached just so we can see subcomponents appear in the
 * ComponentDebugger.  It doesn't do anything.
 */
class SampleComponent
:
public Component,
        public ValueTree::Listener
{
public:
    SampleComponent(ValueTree & tree_) : tree(tree_), button("Button")
    {
        static float hue = 0.0f;
        bgColour = Colours::red.withHue(hue).withSaturation(0.4f).withBrightness(0.4f);
        hue += 0.1f;

        tree.addListener(this);

        addAndMakeVisible(button);

        refresh();
    }

    /**
     * Update our info from the ValueTree and repaint.
     */
    void refresh()
    {
        setComponentID("sample");
        setName(tree["name"].toString());

        repaint();
    }

    void resized() override
    {
        button.setBounds(getLocalBounds().reduced(6).withWidth(80));
    }

    void paint(Graphics & g) override
    {
        g.setColour(bgColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

        g.setColour(bgColour.contrasting(0.6f));
        g.drawText(getName(), getLocalBounds(), Justification::centred);
    }
    
private:
    void valueTreePropertyChanged(ValueTree &, const Identifier &) override { refresh(); }
    void valueTreeChildRemoved(ValueTree &, ValueTree &, int) override {}
    void valueTreeChildOrderChanged(ValueTree &, int , int) override {}
    void valueTreeRedirected(ValueTree &) override {}
    void valueTreeChildAdded(ValueTree &, ValueTree &) override {}
    void valueTreeParentChanged(ValueTree &) override {}

    ValueTree tree;
    Colour bgColour;
    TextButton button;
};

class OffScreenComponent : public Component
{
public:
    OffScreenComponent(const String & name) : Component(name) {}
    void paint(Graphics & g) override { g.setColour(Colours::darkgrey); g.drawRect(getLocalBounds()); }
};

MainContentComponent::MainContentComponent()
:
menuBar(this),
sampleTree("root")
{
    addAndMakeVisible(menuBar);

    for (int i = 0; i < 6; ++i)
        addSampleComponent();

    zeroSizedMisconfiguredComponent = new Component("Misconfigured Component");
    addAndMakeVisible(zeroSizedMisconfiguredComponent);

    offScreenMisconfiguredComponent = new OffScreenComponent("Offscreen Component");
    addAndMakeVisible(offScreenMisconfiguredComponent);

    getLookAndFeel().setDefaultLookAndFeel(&lookAndFeel);

    setSize (600, 400);
}

MainContentComponent::~MainContentComponent()
{
}

void MainContentComponent::paint (Graphics& g)
{
    g.fillAll (Colour::greyLevel(0.1f));
    g.setFont (Font (16.0f));
    g.setColour (Colours::grey);
    g.drawText ("Credland Technical Limited - Debugger Demo", getLocalBounds(), Justification::bottomLeft, true);
}

void MainContentComponent::resized()
{
    menuBar.setBounds(getLocalBounds().withHeight(20));

    auto bounds = getLocalBounds().reduced(20, 0).withTrimmedTop(20);

    auto h = 30;
    auto yPad = 10;
    auto numComponents = components.size();

    // Center the components:
    auto y = (bounds.getHeight() - (h * numComponents + yPad * (numComponents - 1))) / 2;

    for (auto c: components)
    {
        c->setBounds(bounds.getX(), y, bounds.getWidth(), h);
        y += h + yPad;
    }

    if (offScreenMisconfiguredComponent)
        offScreenMisconfiguredComponent->setBounds(getWidth()-50, getHeight() - 40, 100, 30);
}

void MainContentComponent::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
    switch(menuItemID)
    {
        case menuDebugger_ValueTree:
            valueTreeDebugger = new jcf::ValueTreeDebugger(sampleTree);
            break;

        case menuDebugger_Component:
            componentDebugger = new jcf::ComponentDebugger(this);
            break;

        case menuDebugger_Buffer:
            AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::InfoIcon,
                    "Not implemented",
                    "A buffer debugger demo will be added to the next version of the demo");
            break;

        case menuDebugger_Quit:
            JUCEApplication::getInstance()->systemRequestedQuit();
            break;

        default:
            return;
    }
}

PopupMenu MainContentComponent::getMenuForIndex(int /* topLevelMenuIndex */, const String &menuName)
{
    PopupMenu menu;
    menu.addItem(menuDebugger_Buffer,    "Buffer Debugger");
    menu.addItem(menuDebugger_Component, "Component Debugger");
    menu.addItem(menuDebugger_ValueTree, "ValueTree Debugger");
    menu.addSeparator();
    menu.addItem(menuDebugger_Quit,      "Quit");
    return menu;
}

void MainContentComponent::addSampleComponent()
{
    ValueTree node("node");
    node.setProperty("name", "Edit this text in the ValueTreeDebugger", nullptr);
    sampleTree.addChild(node, -1, nullptr);

    {
        auto comp = new SampleComponent(node);
        addAndMakeVisible(comp);
        components.add(comp);
    }

    resized();
}

StringArray MainContentComponent::getMenuBarNames()
{
    return StringArray("Debuggers");
}
