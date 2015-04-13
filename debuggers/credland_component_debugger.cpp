
/* You may need to uncomment the following line. */
//#include "credland_component_debugger.h"


ComponentDebugger::ComponentTreeViewItem::ComponentTreeViewItem(Debugger * owner, Component * c)
:
owner(owner),
outsideBoundsFlag(false)
{
    numChildren = c->getNumChildComponents();
    name = c->getName() + "[id:" + c->getComponentID() + "]";
    type = typeid(*c).name();
    isVisible = c->isVisible();
    bounds = c->getBounds().toString();
    location = owner->getLocationOf(c);

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


void ComponentDebugger::ComponentTreeViewItem::itemSelectionChanged(bool nowSelected)
{
    if (nowSelected)
        owner->setHighlight(location);
}