#pragma once

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

#include <juce_gui_basics/juce_gui_basics.h>

class ValueTreeDebuggerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ValueTreeDebuggerLookAndFeel();
    juce::Font getTextButtonFont(juce::TextButton&, int /*buttonHeight*/) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawComboBox(
        juce::Graphics& g,
        int width,
        int height,
        bool isButtonDown,
        int buttonX,
        int buttonY,
        int buttonW,
        int buttonH,
        juce::ComboBox&
    ) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    void drawComboBoxTextWhenNothingSelected(juce::Graphics&, juce::ComboBox&, juce::Label&) override;
    void drawTextEditorOutline(juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTreeviewPlusMinusBox(juce::Graphics& g, const juce::Rectangle<float>& area, juce::Colour backgroundColour, bool isItemOpen, bool isMouseOver) override;

    
private:
    int comboBoxArrowAreaWidth{ 20 };
};

class TextButtonSmallLookAndFeel : public ValueTreeDebuggerLookAndFeel
{
public:
    juce::Font getTextButtonFont(juce::TextButton&, int /*buttonHeight*/);
};

class TextButtonLargeLookAndFeel : public ValueTreeDebuggerLookAndFeel
{
public:
    juce::Font getTextButtonFont(juce::TextButton&, int /*buttonHeight*/);
};

/* Tracks and braodcasts the selected property */
class ValueTreePropertySelection : public juce::ChangeBroadcaster
{
public:
    void select(juce::ValueTree selectedTree, juce::Identifier selectedPropertyName);
    void deselect();
    bool matchesAndIsSelected(juce::ValueTree selectedTree, juce::Identifier selectedPropertyName) const;

    bool selected{ false };
    juce::ValueTree tree;
    juce::Identifier propertyName;
};

class MiniToolbar :
    public juce::Component,
    public juce::TextEditor::Listener
{
public:
    MiniToolbar();
    ~MiniToolbar() override;

    void resized() override;

    void textEditorTextChanged(juce::TextEditor& textEditor) override;

    juce::TextButton butAddNode;
    juce::TextEditor entryToAdd;
    juce::TextButton butAddProp;
    juce::ComboBox comboPropType;
    juce::TextEditor entryNewValue;
    juce::TextButton butDelProp;
    juce::TextButton butDelNode;
    juce::TextButton butUndo;
    juce::TextButton butRedo;

private:
    void addButtonToToolbar(juce::Component& but);

    juce::FlexBox fb;
    TextButtonLargeLookAndFeel largeTextLnf;
};

/* Displays a var according to its type */
class DynamicValueView :
    public juce::Component,
    public juce::ValueTree::Listener,
    public juce::Label::Listener
{
public:
    DynamicValueView(const juce::ValueTree parentOfValue, const juce::Identifier nameOfProperty, juce::UndoManager* undoManager);
    ~DynamicValueView() override;
    void resized() override;

    void valueTreePropertyChanged(juce::ValueTree& changedTree, const juce::Identifier& prop) override;

    void labelTextChanged(juce::Label* labelThatHasChanged) override;

    juce::Label lbl;
    juce::TextButton butPlus{ "+" };
    juce::TextButton butMinus{ "-" };
    juce::ToggleButton butToggle;

private:
    void setVisibility();

    void setCallbacks();

    void resizedInt();
    void resizedBool();
    void resizedDefault();

    juce::var value();
    void setValue(const juce::var newValue);

    juce::ValueTree tree;
    juce::Identifier propertyName;
    juce::UndoManager* um;
    juce::SharedResourcePointer<TextButtonSmallLookAndFeel> textButtonLnf;
};

/* Displays a property name, type and value according to its type */
class ValueTreePropertyView :
    public juce::Component,
    public juce::ValueTree::Listener,
    public juce::ChangeListener
{
public:
    ValueTreePropertyView(const juce::ValueTree parentOfProperty, const juce::Identifier nameOfProperty, juce::UndoManager* undoManager, ValueTreePropertySelection& treeviewPropertySelection);
    ~ValueTreePropertyView() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void valueTreePropertyChanged(juce::ValueTree& changedTree, const juce::Identifier& prop) override;

    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    juce::Label propNameLbl;
    juce::Label propTypeLbl;
    DynamicValueView valView;

    int propNameLabelWidth{ 150 };
    int propTypeLabelWidth{ 80 };

    bool selected{ false };

    juce::ValueTree tree;
    juce::Identifier propertyName;
    ValueTreePropertySelection& propertySelection;
};

class Item;

/* The component displayed as a tree view item */
class ValueTreeView : public juce::Component
{
public:
    ValueTreeView(juce::String componentName, juce::ValueTree tree, Item& parentItem, juce::UndoManager* undoManager, ValueTreePropertySelection& treeviewPropertySelection);
    ~ValueTreeView() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent& evt) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    void createPropertyComponents();

    /* Get the property view referenced in the mouse event or nullptr */
    ValueTreePropertyView* propertyMoused(const juce::MouseEvent& evt);

    juce::Label lblType{};
    juce::OwnedArray<ValueTreePropertyView> props{};

    int treeTypeLabelWidth{ 150 };

private:
    Item& parent;
    juce::UndoManager* um;
    ValueTreePropertySelection& propertySelection;
    juce::Rectangle<int> propsArea;
    juce::SharedResourcePointer<ValueTreeDebuggerLookAndFeel> lnf{};
};

/* Tree View Item */
class Item :
    public juce::TreeViewItem,
    public juce::ValueTree::Listener,
    public juce::MouseListener
{
public:
    Item(juce::ValueTree treeToUse, juce::UndoManager* undoManager, ValueTreePropertySelection& treeviewPropertySelection);
    ~Item() override;

    // TreeViewItem
    bool mightContainSubItems() override;
    juce::String getUniqueName() const override;
    void itemOpennessChanged(bool isNowOpen) override;
    int getItemHeight() const override;
    std::unique_ptr<juce::Component> createItemComponent() override;
    bool customComponentUsesTreeViewMouseHandler() const override;
    juce::var getDragSourceDescription() override;
    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails&, int insertIndex) override;

    // Value Tree Listener
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& /*childWhichHasBeenAdded*/) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& /*childWhichHasBeenRemoved*/, int) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int, int) override;
    void valueTreeParentChanged(juce::ValueTree& /*treeWhoseParentHasChanged*/) override;
    void valueTreeRedirected(juce::ValueTree& /*treeWhichHasBeenChanged*/) override;

    void updateSubItems();
    void deselectAll();

    juce::ValueTree tree;
    ValueTreeView* comp;

private:
    juce::UndoManager* um;
    ValueTreePropertySelection& propertySelection;
    juce::Array<juce::Identifier> currentProperties;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Item)

};

/* Main component which fills the window */
class ValueTreeDebuggerMain :
    public juce::Component,
    public juce::ValueTree::Listener
{
public:
    ValueTreeDebuggerMain(juce::UndoManager* undoManager);
    ~ValueTreeDebuggerMain() override;

    // Component
    void resized() override;

    // Value Tree Listener
    void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;
    
    void setTree(juce::ValueTree* newTree);

private:
    void setupToolbar();

    std::unique_ptr<Item> rootItem;

    /* The currently selected property */
    ValueTreePropertySelection selectedProperty;
    
    juce::ValueTree* tree;
    juce::UndoManager* um;
    
    juce::TreeView treeView;
    jcf::MiniToolbar toolbar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueTreeDebuggerMain)
};

/* Window containing the VT debugger */
class ValueTreeDebugger :
    public juce::DocumentWindow,
    public juce::DragAndDropContainer
{
public:
    ValueTreeDebugger();
    ValueTreeDebugger(juce::ValueTree& tree, juce::UndoManager* undoManager);
    ~ValueTreeDebugger() override;

    void closeButtonPressed() override;
    
    void setSource(juce::ValueTree& v);

private:
    void construct();

    std::unique_ptr<jcf::ValueTreeDebuggerMain> main;
    juce::SharedResourcePointer<jcf::ValueTreeDebuggerLookAndFeel> lnf{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueTreeDebugger)
};

