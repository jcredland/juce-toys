/*
 * (c) Credland Technical Limited.
 * MIT License
 *
 * Credland Technical Limited provide a range of consultancy and contract
 * services including JUCE software development, support and consultancy.
 *
 * Contact via http://www.credland.net/
 */

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class MainContentComponent
:
public Component,
public MenuBarModel
{
public:
    virtual StringArray getMenuBarNames() override;

    MainContentComponent();

    ~MainContentComponent();

    void paint (Graphics&) override;

    void resized() override;
    
    enum
    {
        menuDebugger,
        menuDebugger_ValueTree,
        menuDebugger_Component,
        menuDebugger_Buffer,
        menuDebugger_Quit
    };
    
    PopupMenu getMenuForIndex(int topLevelMenuIndex, const String &menuName) override;
    void menuItemSelected (int menuItemID, int topLevelMenuIndex) override;

    /**
     * Adds a sub-component that'll be used to show off the ComponentDebugger
     * and ValueTreeDebugger
     */
    void addSampleComponent();

private:
    OwnedArray<Component> components;

    MenuBarComponent menuBar;

    ScopedPointer<ComponentBuilder> builder;

    /**
     * The misconfigured component is here so you can see how
     * they show up in red in the component debugger.
     */
    ScopedPointer<Component> zeroSizedMisconfiguredComponent;
    ScopedPointer<Component> offScreenMisconfiguredComponent;

    ScopedPointer<jcf::ValueTreeDebugger> valueTreeDebugger;
    ScopedPointer<jcf::ComponentDebugger> componentDebugger;

    ValueTree sampleTree;

    LookAndFeel_V3 lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};


#endif  // MAINCOMPONENT_H_INCLUDED
