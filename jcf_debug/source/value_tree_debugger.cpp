#include "value_tree_debugger.h"

using namespace juce;

constexpr int padding{ 5 };
constexpr float paddingF{ 5.f };
constexpr float rowHeightF{ 20.f };
constexpr int rowHeight{ 20 };
constexpr int toolbarHeight{ 30 };
constexpr float toolbarHeightF{ 30.f };
constexpr int toolbarWidth{ 130 };
constexpr float toolbarWidthF{ 130.f };
constexpr int buttonWidth{ 20 };

static juce::Font theFontLarge() { return juce::FontOptions{}.withPointHeight(20.f); }
static juce::Font theFontSmall() { return juce::FontOptions{}.withPointHeight(11.f); }
static juce::Font theFontMini() { return juce::FontOptions{}.withPointHeight(10.f); }

const juce::Colour widgetBackgroundColour{ juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.10f, 1.f) };
const juce::Colour outlineColour{ Colour::fromHSL(240.f / 256.f, 0.00f, 0.90f, 1.f) };
const juce::Colour errorColour{ juce::Colour::fromHSL(1.f / 256.f, 0.40f, 0.50f, 1.f) };
const juce::Colour typeTextColour{ juce::Colour::fromHSL(120.f / 256.f, 0.20f, 0.75f, 1.f) };
const juce::Colour propTextColour{ juce::Colour::fromHSL(240.f / 256.f, 0.20f, 0.75f, 1.f) };
const juce::Colour hintTextColour{ juce::Colour::fromHSL(240.f / 256.f, 0.00f, 0.55f, 1.f) };
const juce::Colour highlightedTextColour{ juce::Colour::fromHSL(240.f / 256.f, 0.00f, 0.85f, 1.f) };
const juce::Colour highlightedTextColourBg{ juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.55f, 1.f) };
const juce::Colour selectedBgColour{ juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.20f, 1.f) };
const juce::Colour selectedBgColourProp{ selectedBgColour.brighter(0.2f) };
const juce::Colour hoverBgColour{ selectedBgColour.brighter(0.2f) };
const juce::Colour hoverBgColourProp{ selectedBgColourProp.brighter(0.2f) };

const juce::var dragAndDropId{ "ValueTreeDebugger_dragndrop_id" };

static bool getBoolValue(const juce::String& string)
{
    auto lowerString = string.toLowerCase();
    static StringArray positiveSentiments{
        "true",
        "y"
        "yes",
        "definitely",
        "1",
        "1.0",
    };
    if (positiveSentiments.contains(lowerString)) return true;
    return false;
}

static juce::String getTypeOfVar(const juce::var theVar)
{
    if (theVar.isVoid()) return "Void";
    if (theVar.isUndefined()) return "Undefined";
    if (theVar.isInt()) return "Int";
    if (theVar.isInt64()) return "Int64";
    if (theVar.isBool()) return "Bool";
    if (theVar.isDouble()) return "Double";
    if (theVar.isString()) return "String";
    if (theVar.isObject()) return "Object";
    if (theVar.isArray()) return "Array";
    if (theVar.isBinaryData()) return "BinaryData";
    if (theVar.isMethod()) return "Method";

    jassertfalse;
    return "Error";
}

static void moveItems(
    TreeView& treeView,
    const OwnedArray<ValueTree>& items,
    ValueTree newParent,
    int insertIndex,
    UndoManager* undoManager
)
{
    if (items.size() > 0)
    {
        std::unique_ptr<XmlElement> oldOpenness(treeView.getOpennessState(false));

        for (auto* v : items)
        {
            if (v->getParent().isValid() && newParent != *v && !newParent.isAChildOf(*v))
            {
                if (v->getParent() == newParent && newParent.indexOf(*v) < insertIndex)
                    --insertIndex;

                v->getParent().removeChild(*v, undoManager);
                newParent.addChild(*v, insertIndex, undoManager);
            }
        }

        if (oldOpenness != nullptr)
            treeView.restoreOpennessState(*oldOpenness, false);
    }
}

static void getSelectedTreeViewItems(TreeView& treeView, OwnedArray<ValueTree>& items)
{
    auto numSelected = treeView.getNumSelectedItems();

    for (int i = 0; i < numSelected; ++i)
        if (auto* vti = dynamic_cast<jcf::Item*>(treeView.getSelectedItem(i)))
            items.add(new ValueTree(vti->tree));
}

static juce::String getItemIdString(ValueTree tree)
{
    String itemIdString{};
    auto parent = tree.getParent();
    auto child = tree;
    while (parent.isValid())
    {
        itemIdString = String{ parent.indexOf(child) } + "/" + itemIdString;
        child = parent;
        parent = parent.getParent();
    }
    itemIdString = "/1/" + itemIdString;
    itemIdString = itemIdString.dropLastCharacters(1);
    return itemIdString;
}

namespace ButtonText
{
const String addProp{ "Set property" };
const String delProp{ "Delete property" };
const String addNode{ "Add child" };
const String delNode{ "Delete node" };
const String undo{ juce::CharPointer_UTF8("\xe2\xa4\xba") };
const String redo{ juce::CharPointer_UTF8("\xe2\xa4\xbb") };
}

namespace ComboVarType
{
/* Types of property which can be created */
const StringArray comboTypeList
{
    "Void",
    "Undefined",
    "Int",
    "Int64",
    "Bool",
    "Double",
    "String",
    //"Object",     not implemented
    //"Array",      not implemented
    //"BinaryData", not implemented
    //"Method"      not implemented
};

/* Lookup for the selection from the drop-down menu */
enum comboTypeIndex
{
    NoSelection = 0,
    Void,
    Undefined,
    Int,
    Int64,
    Bool,
    Double,
    String,
    Object,
    Array,
    BinaryData,
    Method,
};
}

// ============================================================================

ValueTreeDebuggerLookAndFeel::ValueTreeDebuggerLookAndFeel() :
    juce::LookAndFeel_V4
    ({
        juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.05f, 1.f), // windowBackground
        widgetBackgroundColour,                                  // widgetBackground
        juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.20f, 1.f), // menuBackground
        outlineColour,                                           // outline
        juce::Colour::fromHSL(240.f / 256.f, 0.00f, 0.80f, 1.f), // defaultText
        juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.15f, 1.f), // defaultFill
        highlightedTextColour,                                   // highlightedText
        highlightedTextColourBg,                                 // highlightedFill
        juce::Colour::fromHSL(240.f / 256.f, 0.05f, 0.70f, 1.f)  // menuText
    })
{
}

juce::Font ValueTreeDebuggerLookAndFeel::getTextButtonFont(juce::TextButton&, int /*buttonHeight*/)
{
    return theFontSmall();
}

juce::Font ValueTreeDebuggerLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return theFontSmall();
}

juce::Font ValueTreeDebuggerLookAndFeel::getPopupMenuFont()
{
    return theFontSmall();
}

void ValueTreeDebuggerLookAndFeel::positionComboBoxText(ComboBox& box, Label& label)
{
    label.setBounds(padding, 1,
        box.getWidth() - comboBoxArrowAreaWidth,
        box.getHeight() - 2);

    label.setFont(getComboBoxFont(box));
}

void ValueTreeDebuggerLookAndFeel::drawComboBox(
    juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box)
{
    Rectangle<int> boxBounds(0, 0, width, height);

    g.setColour(box.findColour(ComboBox::backgroundColourId));
    g.fillRect(boxBounds.toFloat());

    g.setColour(box.findColour(ComboBox::outlineColourId));
    g.drawRect(boxBounds.toFloat(), 1.0f);

    Rectangle<int> arrowZone(width - comboBoxArrowAreaWidth, 0, 16, height);
    Path path;
    path.startNewSubPath((float)arrowZone.getX() + 3.0f, (float)arrowZone.getCentreY() - 2.0f);
    path.lineTo((float)arrowZone.getCentreX(), (float)arrowZone.getCentreY() + 3.0f);
    path.lineTo((float)arrowZone.getRight() - 3.0f, (float)arrowZone.getCentreY() - 2.0f);

    g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 0.9f : 0.2f)));
    g.strokePath(path, PathStrokeType(1.0f));
}

void ValueTreeDebuggerLookAndFeel::drawComboBoxTextWhenNothingSelected(juce::Graphics& g, juce::ComboBox& box, juce::Label& label)
{
    g.setColour(findColour(ComboBox::textColourId).withMultipliedAlpha(0.5f));

    auto font = label.getLookAndFeel().getLabelFont(label);

    g.setFont(font);

    auto textAreaI = label.getLocalBounds();
    textAreaI.removeFromLeft(padding);
    textAreaI = getLabelBorderSize(label).subtractedFrom(textAreaI);
    auto textAreaF = textAreaI.toFloat();

    GlyphArrangement textToDraw{};
    textToDraw.addFittedText(
        theFontSmall(),
        box.getTextWhenNothingSelected(),
        textAreaF.getX(),
        textAreaF.getY(),
        textAreaF.getWidth(),
        textAreaF.getHeight(),
        Justification::centred,
        1, // Max lines to use
        1.f, // Min horizontal scale,
        GlyphArrangementOptions{}
    );
    textToDraw.draw(g);
}

void ValueTreeDebuggerLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& textEditor)
{
    if (dynamic_cast<AlertWindow*> (textEditor.getParentComponent()) == nullptr)
    {
        if (textEditor.isEnabled())
        {
            if (textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly())
            {
                g.setColour(textEditor.findColour(TextEditor::focusedOutlineColourId));
                g.drawRect(0, 0, width, height, 2);
            }
            else
            {
                g.setColour(textEditor.findColour(TextEditor::outlineColourId));
                g.drawRect(0, 0, width, height, 1);
            }
        }
    }
}

void ValueTreeDebuggerLookAndFeel::drawTreeviewPlusMinusBox(juce::Graphics& g, const juce::Rectangle<float>& area, juce::Colour backgroundColour, bool isItemOpen, bool isMouseOver)
{
    Path p;
    p.addTriangle(0.0f, 0.0f, 1.0f, isItemOpen ? 0.0f : 0.5f, isItemOpen ? 0.5f : 0.0f, 1.0f);

    g.setColour(backgroundColour.contrasting().withAlpha(isMouseOver ? 0.5f : 0.3f));
    const Rectangle<float> bounds{area.getX() + rowHeightF / 4.f, area.getY() + rowHeightF / 4.f, rowHeightF / 2.f, rowHeightF / 2.f};
    g.fillPath(p, p.getTransformToScaleToFit(bounds, true));
}

juce::Font TextButtonSmallLookAndFeel::getTextButtonFont(juce::TextButton&, int)
{
    return theFontMini();
}

juce::Font TextButtonLargeLookAndFeel::getTextButtonFont(juce::TextButton&, int)
{
    return theFontLarge();
}

// ============================================================================

void ValueTreePropertySelection::select(juce::ValueTree selectedTree, juce::Identifier selectedPropertyName)
{
    selected = true;
    tree = selectedTree;
    propertyName = selectedPropertyName;
    sendChangeMessage();
}

void ValueTreePropertySelection::deselect()
{
    selected = false;
    tree = ValueTree{};
    propertyName = Identifier{};
    sendChangeMessage();
}

bool ValueTreePropertySelection::matchesAndIsSelected(juce::ValueTree selectedTree, juce::Identifier selectedPropertyName) const
{
    return
        selected &&
        selectedPropertyName == propertyName &&
        selectedTree == tree
    ;
}

// ============================================================================

MiniToolbar::MiniToolbar()
{
    butAddProp.setButtonText(ButtonText::addProp);
    butDelProp.setButtonText(ButtonText::delProp);
    butAddNode.setButtonText(ButtonText::addNode);
    butDelNode.setButtonText(ButtonText::delNode);
    butUndo.setButtonText(ButtonText::undo);
    butRedo.setButtonText(ButtonText::redo);
    butUndo.setLookAndFeel(&largeTextLnf);
    butRedo.setLookAndFeel(&largeTextLnf);

    butAddNode.setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnBottom);
    butAddProp.setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnTop);
    butUndo.setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnRight);
    butRedo.setConnectedEdges(Button::ConnectedEdgeFlags::ConnectedOnLeft);
    entryToAdd.setJustification(Justification::centred);
    entryToAdd.setTextToShowWhenEmpty("ID to add", hintTextColour);
    entryToAdd.setColour(TextEditor::ColourIds::highlightedTextColourId, highlightedTextColour);
    entryToAdd.setColour(TextEditor::ColourIds::highlightColourId, highlightedTextColourBg);
    entryToAdd.setFont(theFontSmall());
    comboPropType.addItemList(ComboVarType::comboTypeList, ComboVarType::comboTypeIndex::Void);
    comboPropType.setTextWhenNothingSelected("Property type");
    entryNewValue.setJustification(Justification::centred);
    entryNewValue.setTextToShowWhenEmpty("New value", hintTextColour);
    entryNewValue.setColour(TextEditor::ColourIds::highlightedTextColourId, highlightedTextColour);
    entryNewValue.setColour(TextEditor::ColourIds::highlightColourId, highlightedTextColourBg);
    entryNewValue.setFont(theFontSmall());

    fb.items.add(FlexItem{ paddingF, paddingF }.withWidth(toolbarWidthF));
    addButtonToToolbar(butAddNode);
    addButtonToToolbar(entryToAdd);
    addButtonToToolbar(comboPropType);
    addButtonToToolbar(entryNewValue);
    addButtonToToolbar(butAddProp);
    fb.items.add(FlexItem{ paddingF, paddingF }.withWidth(toolbarWidthF));
    addButtonToToolbar(butDelNode);
    addButtonToToolbar(butDelProp);
    fb.items.add(FlexItem{ paddingF, paddingF }.withWidth(toolbarWidthF));

    // Undo + redo can share a row
    addAndMakeVisible(butUndo);
    auto butUndoItem = juce::FlexItem{ butUndo }.withFlex(1.f, 1.f, buttonWidth).withHeight(toolbarHeightF);
    fb.items.add(butUndoItem);
    addAndMakeVisible(butRedo);
    auto butRedoItem = juce::FlexItem{ butRedo }.withFlex(1.f, 1.f, buttonWidth).withHeight(toolbarHeightF);
    fb.items.add(butRedoItem);

    fb.justifyContent = FlexBox::JustifyContent::center;
    fb.alignContent = FlexBox::AlignContent::flexStart;
    fb.flexWrap = FlexBox::Wrap::wrap;
}

MiniToolbar::~MiniToolbar()
{
    butUndo.setLookAndFeel(nullptr);
    butRedo.setLookAndFeel(nullptr);
}

void MiniToolbar::resized()
{
    auto bounds = getLocalBounds();
    fb.performLayout(bounds);
}

void MiniToolbar::textEditorTextChanged(juce::TextEditor& textEditor)
{
    const auto newName = textEditor.getText();
    if (Identifier::isValidIdentifier(newName))
    {
        textEditor.setColour(TextEditor::ColourIds::outlineColourId, outlineColour);
        textEditor.setColour(TextEditor::ColourIds::focusedOutlineColourId, outlineColour.brighter(0.2f));
    }
    else
    {
        textEditor.setColour(TextEditor::ColourIds::outlineColourId, errorColour);
        textEditor.setColour(TextEditor::ColourIds::focusedOutlineColourId, errorColour);
    }
}

void MiniToolbar::addButtonToToolbar(juce::Component& but)
{
    addAndMakeVisible(but);
    auto butItem = juce::FlexItem{ but }.withFlex(1.f, 1.f, toolbarWidthF).withHeight(toolbarHeightF);
    fb.items.add(butItem);
}

// ============================================================================

DynamicValueView::DynamicValueView(const juce::ValueTree parentOfValue, const juce::Identifier nameOfProperty, juce::UndoManager* undoManager) :
    tree(parentOfValue),
    propertyName(nameOfProperty),
    um(undoManager)
{
    lbl.setText(value().toString(), NotificationType::dontSendNotification);
    lbl.setEditable(false, true, false);

    butPlus.setLookAndFeel(textButtonLnf);
    butMinus.setLookAndFeel(textButtonLnf);

    addChildComponent(lbl);
    addChildComponent(butPlus);
    addChildComponent(butMinus);
    addChildComponent(butToggle);

    setVisibility();
    setCallbacks();

    tree.addListener(this);
    lbl.addListener(this);
    tree.sendPropertyChangeMessage(propertyName);
}

DynamicValueView::~DynamicValueView()
{
    butPlus.setLookAndFeel(nullptr);
    butMinus.setLookAndFeel(nullptr);
}

void DynamicValueView::resized()
{
    const auto val = value();

    if (val.isInt() || val.isInt64())
    {
        resizedInt();
    }
    else if (val.isBool())
    {
        resizedBool();
    }
    else
    {
        resizedDefault();
    }
}

void DynamicValueView::valueTreePropertyChanged(juce::ValueTree& changedTree, const juce::Identifier& prop)
{
    if (tree == changedTree && prop == propertyName)
    {
        setVisibility();
        lbl.setText(value().toString(), NotificationType::dontSendNotification);
        butToggle.setToggleState(bool(value()), NotificationType::dontSendNotification);
    }
}

void DynamicValueView::labelTextChanged(juce::Label* labelThatHasChanged)
{
    auto text = labelThatHasChanged->getText();
    var oldVal{ value() };
    var newVal;

    if (oldVal.isInt()) newVal = text.getIntValue();
    if (oldVal.isInt64()) newVal = text.getIntValue();
    if (oldVal.isBool()) newVal = getBoolValue(text);
    if (oldVal.isDouble()) newVal = text.getDoubleValue();
    if (oldVal.isString()) newVal = text;

    if (newVal.isVoid())
    {
        // None of the above happened - we cannot parse the label text
        newVal = oldVal;
    }

    setValue(newVal);
}

void DynamicValueView::setVisibility()
{
    const auto val = value();

    lbl.setVisible(false);
    butPlus.setVisible(false);
    butMinus.setVisible(false);
    butToggle.setVisible(false);

    if (val.isInt() || val.isInt64())
    {
        butPlus.setVisible(true);
        butMinus.setVisible(true);
        lbl.setVisible(true);
    }
    else if (val.isBool())
    {
        butToggle.setVisible(true);
    }
    else
    {
        lbl.setVisible(true);
    }
}

void DynamicValueView::setCallbacks()
{
    const auto val = value();
    butPlus.onClick = [&]()
    {
        jassert(
            value().isInt() ||
            value().isInt64()
        );
        setValue(int(value()) + 1);
    };
    butMinus.onClick = [&]()
    {
        jassert(
            value().isInt() ||
            value().isInt64()
        );
        setValue(int(value()) - 1);
    };
    butToggle.onClick = [&]()
    {
        jassert(value().isBool());
        setValue(butToggle.getToggleState());
    };
}

void DynamicValueView::resizedInt()
{
    auto bounds = getLocalBounds();
    auto butPlusRect = bounds.removeFromLeft(buttonWidth);
    auto butMinusRect = bounds.removeFromLeft(buttonWidth);

    butPlus.setBounds(butPlusRect);
    butMinus.setBounds(butMinusRect);
    lbl.setBounds(bounds);
}

void DynamicValueView::resizedBool()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(padding);
    butToggle.setBounds(bounds);
}

void DynamicValueView::resizedDefault()
{
    auto bounds = getLocalBounds();
    lbl.setBounds(bounds);
}

juce::var DynamicValueView::value()
{
    return tree.getProperty(propertyName);
}

void DynamicValueView::setValue(const juce::var newValue)
{
    tree.setProperty(propertyName, newValue, um);
    if (um) um->beginNewTransaction();
}

// ============================================================================

ValueTreePropertyView::ValueTreePropertyView(const juce::ValueTree parentOfProperty, const juce::Identifier nameOfProperty, juce::UndoManager* undoManager, ValueTreePropertySelection& treeviewPropertySelection) :
    valView(parentOfProperty, nameOfProperty, undoManager),
    tree(parentOfProperty),
    propertyName(nameOfProperty),
    propertySelection(treeviewPropertySelection)
{
    tree.addListener(this);
    propertySelection.addChangeListener(this);

    propNameLbl.setText(nameOfProperty.toString(), NotificationType::dontSendNotification);
    propNameLbl.setMinimumHorizontalScale(1.f);
    propNameLbl.setColour(Label::ColourIds::textColourId, propTextColour);

    propTypeLbl.setText(getTypeOfVar(parentOfProperty[nameOfProperty]), NotificationType::dontSendNotification);
    propTypeLbl.setMinimumHorizontalScale(1.f);

    addAndMakeVisible(propNameLbl);
    addAndMakeVisible(propTypeLbl);
    addAndMakeVisible(valView);
}

ValueTreePropertyView::~ValueTreePropertyView()
{
    propertySelection.removeChangeListener(this);
}

void ValueTreePropertyView::resized()
{
    auto bounds = getLocalBounds();
    const auto propLblRect = bounds.removeFromLeft(propNameLabelWidth);
    bounds.removeFromLeft(padding);
    const auto propTypeRect = bounds.removeFromLeft(propTypeLabelWidth);
    bounds.removeFromLeft(padding);

    propNameLbl.setBounds(propLblRect);
    propTypeLbl.setBounds(propTypeRect);
    valView.setBounds(bounds);
}

void ValueTreePropertyView::paint(juce::Graphics& g)
{
    if (selected)
    {
        g.fillAll(selectedBgColourProp);
    }
    else if (isMouseOver(true))
    {
        g.fillAll(hoverBgColourProp);
    }
}

void ValueTreePropertyView::valueTreePropertyChanged(juce::ValueTree& changedTree, const juce::Identifier& prop)
{
    if (tree == changedTree && prop == propertyName)
    {
        propTypeLbl.setText(getTypeOfVar(tree[prop]), NotificationType::dontSendNotification);
    }
}

void ValueTreePropertyView::changeListenerCallback(ChangeBroadcaster*)
{
    selected = propertySelection.matchesAndIsSelected(tree, propertyName);
    getParentComponent()->repaint();
}

// ============================================================================

ValueTreeView::ValueTreeView(juce::String componentName, juce::ValueTree tree, Item& parentItem, juce::UndoManager* undoManager, ValueTreePropertySelection& treeviewPropertySelection) :
    juce::Component(componentName),
    parent(parentItem),
    um(undoManager),
    propertySelection(treeviewPropertySelection)
{
    setLookAndFeel(lnf);
    addMouseListener(this, true);

    lblType.setText(tree.getType().toString(), NotificationType::dontSendNotification);
    lblType.setMinimumHorizontalScale(1.f);
    lblType.setColour(Label::ColourIds::textColourId, typeTextColour);
    addAndMakeVisible(lblType);

    createPropertyComponents();
}

ValueTreeView::~ValueTreeView()
{
    setLookAndFeel(nullptr);
}

void ValueTreeView::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromRight(padding);
    const auto rectType = bounds.removeFromLeft(treeTypeLabelWidth).removeFromTop(rowHeight);
    lblType.setBounds(rectType);
    bounds.removeFromLeft(padding);
    propsArea = bounds;

    for (auto* prop : props)
    {
        const auto propRect = bounds.removeFromTop(rowHeight);
        prop->setBounds(propRect);
    }
}

void ValueTreeView::paint(juce::Graphics& g)
{
    if (parent.isSelected())
    {
        g.fillAll(selectedBgColour);
    }
    else if(isMouseOver(true))
    {
        g.fillAll(hoverBgColour);
    }
}

void ValueTreeView::mouseUp(const juce::MouseEvent& evt)
{
    if (auto* propView = propertyMoused(evt))
    {
        propertySelection.select(propView->tree, propView->propertyName);
    }
    else
    {
        propertySelection.deselect();
    }
}

void ValueTreeView::mouseEnter(const juce::MouseEvent&)
{
    repaint();
}

void ValueTreeView::mouseExit(const juce::MouseEvent&)
{
    repaint();
}

void ValueTreeView::createPropertyComponents()
{
    props.clear(true);
    for (int i = 0; i < parent.tree.getNumProperties(); ++i)
    {
        const juce::Identifier name = parent.tree.getPropertyName(i);
        auto latest = props.add(std::make_unique<ValueTreePropertyView>(parent.tree, name, um, propertySelection));
        addAndMakeVisible(*latest);
    }
    resized();
}

ValueTreePropertyView* ValueTreeView::propertyMoused(const juce::MouseEvent& evt)
{
    for (auto* prop : props)
    {
        const auto relEvt = evt.getEventRelativeTo(prop);
        if (prop->contains(relEvt.getPosition()))
        {
            return prop;
        }
    }
    return nullptr;
}

// ============================================================================

Item::Item(juce::ValueTree treeToUse, juce::UndoManager* undoManager, ValueTreePropertySelection& treeviewPropertySelection) :
    tree(treeToUse),
    um(undoManager),
    propertySelection(treeviewPropertySelection)
{
    tree.addListener(this);
}

Item::~Item()
{
    clearSubItems();
}

bool Item::mightContainSubItems()
{
    return tree.getNumChildren() > 0;
}

juce::String Item::getUniqueName() const
{
    if (!tree.getParent().isValid()) return "1";

    return String(tree.getParent().indexOf(tree));
}

void Item::itemOpennessChanged(bool isNowOpen)
{
    if (isNowOpen) updateSubItems();
}

int Item::getItemHeight() const
{
    return jmax(rowHeight, rowHeight * tree.getNumProperties());
}

std::unique_ptr<juce::Component> Item::createItemComponent()
{
    auto ret = std::make_unique<ValueTreeView>(getUniqueName(), tree, *this, um, propertySelection);
    comp = ret.get();
    return ret;
}

bool Item::customComponentUsesTreeViewMouseHandler() const
{
    return true;
}

juce::var Item::getDragSourceDescription()
{
    return dragAndDropId;
}

bool Item::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails)
{
    return dragSourceDetails.description == dragAndDropId;
}

inline void Item::itemDropped(const juce::DragAndDropTarget::SourceDetails&, int insertIndex)
{
    OwnedArray<ValueTree> selectedTrees;
    getSelectedTreeViewItems(*getOwnerView(), selectedTrees);

    moveItems(*getOwnerView(), selectedTrees, tree, insertIndex, um);
    if (um) um->beginNewTransaction();
}

void Item::valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree&)
{
    if (parentTree == tree)
        updateSubItems();

    treeHasChanged();
}

void Item::valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree&, int)
{
    if (parentTree == tree)
        updateSubItems();

    treeHasChanged();
}

void Item::valueTreeChildOrderChanged(juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int, int)
{
    if (parentTreeWhoseChildrenHaveMoved == tree)
        updateSubItems();

    treeHasChanged();
}

void Item::valueTreeParentChanged(juce::ValueTree&)
{
    treeHasChanged();
}

void Item::valueTreeRedirected(juce::ValueTree&)
{
    // This should be handled by the ValueTreeDebuggerMain
    jassertfalse;
    
    updateSubItems();
    treeHasChanged();
}

void Item::updateSubItems()
{
    std::unique_ptr<XmlElement> opennessXml = getOpennessState();
    clearSubItems();
    int children = tree.getNumChildren();

    for (int i = 0; i < children; ++i)
        addSubItem(new Item(tree.getChild(i), um, propertySelection));

    if (opennessXml.get())
        restoreOpennessState(*opennessXml.get());
}

void Item::deselectAll()
{
    for (auto* prop : comp->props)
    {
        prop->selected = false;
    }
}

// ============================================================================

ValueTreeDebuggerMain::ValueTreeDebuggerMain(juce::UndoManager* undoManager) :
    um(undoManager)
{
    treeView.setDefaultOpenness(true);
    treeView.setColour(TreeView::ColourIds::backgroundColourId, widgetBackgroundColour);

    setSize(800, 600);

    addAndMakeVisible(toolbar);
    addAndMakeVisible(treeView);

    setupToolbar();
}

ValueTreeDebuggerMain::~ValueTreeDebuggerMain()
{
    treeView.setRootItem(nullptr);
}

void ValueTreeDebuggerMain::resized()
{
    auto bounds = getLocalBounds();
    auto toolbarRect = bounds.removeFromLeft(toolbarWidth);
    toolbar.setBounds(toolbarRect);
    treeView.setBounds(bounds);
}

void ValueTreeDebuggerMain::valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged)
{
    // The address of the value tree does not change, just the shared object the value tree is referencing
    jassert(tree == &treeWhichHasBeenChanged);
    setTree(&treeWhichHasBeenChanged);
}

void ValueTreeDebuggerMain::setTree(juce::ValueTree* newTree)
{
    treeView.setRootItem(nullptr);
    if (newTree == nullptr) return;
    
    tree = newTree;
    tree->addListener(this);
    
    rootItem = std::make_unique<Item>(*tree, um, selectedProperty);
    treeView.setRootItem(rootItem.get());
    rootItem->updateSubItems();
    rootItem->treeHasChanged();
}

void ValueTreeDebuggerMain::setupToolbar()
{
    toolbar.butUndo.onClick = [&]()
    {
        if (um) um->undo();
        rootItem->updateSubItems();
    };
    toolbar.butRedo.onClick = [&]()
    {
        if (um) um->redo();
        rootItem->updateSubItems();
    };
    toolbar.butAddProp.onClick = [&]()
    {
        if (auto* selectedItem = dynamic_cast<Item*>(treeView.getSelectedItem(0)))
        {
            using ComboVarType::comboTypeIndex;
            auto newName = toolbar.entryToAdd.getText();
            auto newTypeIndex = static_cast<comboTypeIndex>(toolbar.comboPropType.getSelectedId());
            auto newValText = toolbar.entryNewValue.getText();

            if (Identifier::isValidIdentifier(newName))
            {
                var newVal;

                switch (newTypeIndex)
                {
                case 0:
                    // Nothing selected
                    break;

                case comboTypeIndex::Void:
                    break;
                    
                case comboTypeIndex::Undefined:
                    newVal = var::undefined();
                    break;
                    
                case comboTypeIndex::Int:
                    newVal = newValText.getIntValue();
                    break;
                    
                case comboTypeIndex::Int64:
                    newVal = newValText.getIntValue();
                    break;
                    
                case comboTypeIndex::Bool:
                    newVal = getBoolValue(newValText);
                    break;
                    
                case comboTypeIndex::Double:
                    newVal = newValText.getDoubleValue();
                    break;
                    
                case comboTypeIndex::String:
                    newVal = newValText;
                    break;
                    
                case comboTypeIndex::Object:
                    // Not implemented
                    jassertfalse;
                    break;
                    
                case comboTypeIndex::Array:
                    // Not implemented
                    jassertfalse;
                    break;
                    
                case comboTypeIndex::BinaryData:
                    // Not implemented
                    jassertfalse;
                    break;
                    
                case comboTypeIndex::Method:
                    // Not implemented
                    jassertfalse;
                    break;

                default:
                    jassertfalse;
                    break;

                };

                selectedItem->tree.setProperty(newName, newVal, um);
                if (um) um->beginNewTransaction();
                selectedItem->comp->createPropertyComponents();
                selectedItem->treeHasChanged();
            }
        }
    };
    toolbar.butAddNode.onClick = [&]()
    {
        if (auto* selectedItem = dynamic_cast<Item*>(treeView.getSelectedItem(0)))
        {
            auto newName = toolbar.entryToAdd.getText();
            if (Identifier::isValidIdentifier(newName))
            {
                selectedItem->tree.addChild(ValueTree{ newName }, -1, um);
                if (um) um->beginNewTransaction();
                selectedItem->treeHasChanged();
            }
        }
    };
    toolbar.butDelNode.onClick = [&]()
    {
        const auto numSelected = treeView.getNumSelectedItems();
        for (int i = 0; i < numSelected; ++i)
        {
            if (auto* item = dynamic_cast<jcf::Item*>(treeView.getSelectedItem(i)))
            {
                auto parent = item->tree.getParent();
                if (parent.isValid())
                {
                    parent.removeChild(item->tree, um);
                    if (um) um->beginNewTransaction();
                }
            }
        }
    };
    toolbar.butDelProp.onClick = [&]()
    {
        selectedProperty.tree.removeProperty(selectedProperty.propertyName, um);
        if (um) um->beginNewTransaction();
        
        const auto itemIdString = getItemIdString(selectedProperty.tree);

        if (auto* treeItem = treeView.findItemFromIdentifierString(itemIdString))
        {
            if (auto* item = dynamic_cast<Item*>(treeItem))
            {
                item->comp->createPropertyComponents();
                treeView.getSelectedItem(0)->treeHasChanged();
            }
        }
    };
}

// ============================================================================

ValueTreeDebugger::ValueTreeDebugger() :
    DocumentWindow(
        "Value Tree Debugger",
        juce::Colours::transparentBlack,
        DocumentWindow::allButtons
    )
{
    setLookAndFeel(lnf);
    setBackgroundColour(lnf->getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground));
    main = std::make_unique<jcf::ValueTreeDebuggerMain>(nullptr);
    construct();
}

ValueTreeDebugger::ValueTreeDebugger(juce::ValueTree& tree, juce::UndoManager* undoManager) :
    DocumentWindow(
        "Value Tree Debugger",
        juce::Colours::transparentBlack,
        DocumentWindow::allButtons
    )
{
    setLookAndFeel(lnf);
    setBackgroundColour(lnf->getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground));
    main = std::make_unique<jcf::ValueTreeDebuggerMain>(undoManager);
    construct();
    setSource(tree);
}

ValueTreeDebugger::~ValueTreeDebugger()
{
    setLookAndFeel(nullptr);
    main->setTree(nullptr);
}

void ValueTreeDebugger::closeButtonPressed()
{
    setVisible(false);
}

void ValueTreeDebugger::setSource(juce::ValueTree& v)
{
    main->setTree(&v);
}

void ValueTreeDebugger::construct()
{
    setContentNonOwned(main.get(), true);
    setResizable(true, false);
    setResizeLimits(200, 100, 1920, 1080);
    setUsingNativeTitleBar(true);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

