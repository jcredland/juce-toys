
/* You may need to uncomment the following line. */
//#include "credland_component_debugger.h"

/**
 @internal - allows the components position to be edited
 using a property panel. 
 */
class ComponentBoundsEditor
:
public Value::Listener,
public Component
{
public:
    ComponentBoundsEditor(ComponentDebugger::Debugger * owner)
    :
    owner(owner)
    {
        Array<PropertyComponent *> p;
        p.add(new TextPropertyComponent(x, "x", 10, false));
        p.add(new TextPropertyComponent(y, "y", 10, false));
        p.add(new TextPropertyComponent(w, "w", 10, false));
        p.add(new TextPropertyComponent(h, "h", 10, false));
        panel.addProperties(p);
        addAndMakeVisible(panel);

        x.addListener(this);
        y.addListener(this);
        w.addListener(this);
        h.addListener(this);
    }

    void resized()
    {
        panel.setBounds(getLocalBounds());
    }

    void setComponent(Component * c)
    {
        component = c;
        auto r = component->getBounds();
        x = r.getX();
        y = r.getY();
        w = r.getWidth();
        h = r.getHeight();
    }

    void valueChanged(Value &) override
    {
        if (component)
        {
            component->setBounds(x.getValue(),
                                 y.getValue(),
                                 h.getValue(),
                                 w.getValue());
            owner->setHighlight(component);
        }
    }
private:
    ComponentDebugger::Debugger * owner;
    WeakReference<Component> component;
    Value x, y, h, w;
    PropertyPanel panel;
};

/**********************************************************/

ComponentDebugger::Debugger::Debugger(Component * rootComponent)
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

    boundsEditor = new ComponentBoundsEditor(this);
    addAndMakeVisible(boundsEditor);
}


ComponentDebugger::Debugger::~Debugger()
{}

void ComponentDebugger::Debugger::buttonClicked(Button *)
{
    refresh();
}

void ComponentDebugger::Debugger::refresh()
{
    tree.setRootItem(new ComponentTreeViewItem(this, root));
}

Rectangle<int> ComponentDebugger::Debugger::getLocationOf(Component * c)
{
    return root->getLocalArea(c, c->getLocalBounds());
}

void ComponentDebugger::Debugger::resized()
{
    const int boundsEditorHeight = 110;

    refreshButton.setBounds(0, 0, 130, 20);
    tree.setBounds(0, 20, getWidth(), getHeight() - boundsEditorHeight - 20);
    boundsEditor->setBounds(0, getHeight() - boundsEditorHeight, getWidth(), boundsEditorHeight);
}


void ComponentDebugger::Debugger::setComponentToEdit(Component * c)
{
    boundsEditor->setComponent(c);
}

/**********************************************************/

ComponentDebugger::ComponentTreeViewItem::ComponentTreeViewItem(Debugger * owner,
                                                                Component * c)
:
owner(owner),
outsideBoundsFlag(false),
component(c)
{
    numChildren = c->getNumChildComponents();
    name = c->getName() + "[id=" + c->getComponentID() + "]";
    type = typeid(*c).name();
    isVisible = c->isVisible();

    auto localBounds = c->getBounds();
    zeroSizeFlag = localBounds.getWidth() == 0 || localBounds.getHeight() == 0;

    auto parent = c->getParentComponent();

    if (parent)
    {
        auto parentBounds = parent->getLocalBounds();
        outsideBoundsFlag = ! (parentBounds.contains(localBounds));
    }

    /* We have to build the whole tree now as the components might be deleted in operation... */
    for (int i = 0; i < numChildren; ++i)
        addSubItem(new ComponentTreeViewItem(owner, c->getChildComponent(i)));
}


bool ComponentDebugger::ComponentTreeViewItem::mightContainSubItems()
{
    return numChildren > 0;
}


void ComponentDebugger::ComponentTreeViewItem::paintItem(Graphics & g, int w, int h)
{
    if (zeroSizeFlag)
        g.fillAll(Colours::red.withAlpha(0.5f));
    else if (outsideBoundsFlag)
        g.fillAll(Colours::yellow.withAlpha(0.5f));

    if (isVisible)
        g.setColour(Colours::black);
    else
        g.setColour(Colours::grey);

    auto bounds = component ? component->getBounds().toString() : "Component Deleted";

    g.drawText(type + "name=" + name  + " bounds=" + bounds,
               0, 0, w, h, Justification::left, true);
}


void ComponentDebugger::ComponentTreeViewItem::itemSelectionChanged(bool nowSelected)
{
    if (nowSelected && component != nullptr)
    {
        owner->setHighlight(component);
        owner->setComponentToEdit(component);
    }
}