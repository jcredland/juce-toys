/*
  ==============================================================================

    value_tree_editor.h
    Created: 8 Aug 2014 10:43:40am
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef VALUE_TREE_EDITOR_H_INCLUDED
#define VALUE_TREE_EDITOR_H_INCLUDED


/* You may need to uncomment the following line. */
//#include "../JuceLibraryCode/JuceHeader.h"

/**
 Display a separate desktop window for viewed and editing a value tree's
 property fields.

 Instantate a ValueTreeEditor instance, then call ValueTreeEditor::setSource(ValueTree &)
 and it'll display your tree.

 For example:

 @code
 valueTreeEditor = new ValueTreeEditor();
 valueTreeEditor->setSource(myTree);

 @note
 This code isn't pretty, and the UI isn't pretty - it's for debugging and definitely not
 production use!
 */
class ValueTreeEditor :
    public DocumentWindow
{
    /** Display properties for a tree. */
    class PropertyEditor :
        public PropertyPanel
    {
    public:
        PropertyEditor()
        {
            noEditValue = "not editable";
        }
        void setSource (ValueTree& tree)
        {
            clear();

            t = tree;

            const int maxChars = 200;

            Array<PropertyComponent*> pc;

            for (int i = 0; i < t.getNumProperties(); ++i)
            {
                const Identifier name = t.getPropertyName (i).toString();
                Value v = t.getPropertyAsValue (name, nullptr);
                TextPropertyComponent* tpc;

                if (v.getValue().isObject())
                {
                    tpc = new TextPropertyComponent (noEditValue, name.toString(), maxChars, false);
                    tpc->setEnabled (false);
                }
                else
                {
                    tpc = new TextPropertyComponent (v, name.toString(), maxChars, false);
                }

                pc.add (tpc);
            }

            addProperties (pc);
        }

    private:
        Value noEditValue;
        ValueTree t;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyEditor);
    };

    class Item :
        public TreeViewItem,
        public ValueTree::Listener
    {
    public:
        Item (PropertyEditor* propertiesEditor, ValueTree tree)
            :
            propertiesEditor (propertiesEditor),
            t (tree)
        {
            t.addListener (this);
        }

        ~Item()
        {
            clearSubItems();
        }

        bool mightContainSubItems()
        {
            return t.getNumChildren() > 0;
        }

        void itemOpennessChanged (bool isNowOpen)
        {
            if (isNowOpen) updateSubItems();
        }

        void updateSubItems()
        {
            ScopedPointer<XmlElement> openness = getOpennessState();
            clearSubItems();
            int children = t.getNumChildren();

            for (int i = 0; i < children; ++i)
                addSubItem (new Item (propertiesEditor, t.getChild (i)));

            if (openness)
                restoreOpennessState (*openness);
        }

        void paintItem (Graphics& g, int w, int h)
        {

            Font font;
            Font smallFont (11.0);


            if (isSelected())
                g.fillAll (Colours::white);


            const float padding = 20.0f;

            String typeName = t.getType().toString();

            const float nameWidth = font.getStringWidthFloat (typeName);
            const float propertyX = padding + nameWidth;

            g.setColour (Colours::black);

            g.setFont (font);

            g.drawText (t.getType().toString(), 0, 0, w, h, Justification::left, false);
            g.setColour (Colours::blue);

            String propertySummary;

            for (int i = 0; i < t.getNumProperties(); ++i)
            {
                const Identifier name = t.getPropertyName (i).toString();
                String propertyValue = t.getProperty (name).toString();;
#ifdef JCF_SERIALIZER

                if (t[name].isObject())
                {
                    ReferenceCountedObject* p = t[name].getObject();

                    if (Serializable* s = dynamic_cast<Serializable*> (p))
                        propertyValue = "[[" + s->getDebugInformation() + "]]";
                }

#endif
                propertySummary += " " + name.toString() + "=" + propertyValue;
            }

            g.drawText (propertySummary, propertyX, 0, w - propertyX, h, Justification::left, true);
        }

        void itemSelectionChanged (bool isNowSelected)
        {
            if (isNowSelected)
            {
                t.removeListener (this);
                propertiesEditor->setSource (t);
                t.addListener (this);
            }
        }


        /* Enormous list of ValueTree::Listener options... */
        void valueTreePropertyChanged (ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
        {
            if (t != treeWhosePropertyHasChanged) return;

            t.removeListener (this);
//            if (isSelected())
//                propertiesEditor->setSource(t);
            repaintItem();
            t.addListener (this);
        }
        void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
        {
            if (parentTree == t)
                updateSubItems();

            treeHasChanged();
        }


        void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int)
        {
            if (parentTree == t)
                updateSubItems();

            treeHasChanged();
        }
        void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved, int, int)
        {
            if (parentTreeWhoseChildrenHaveMoved == t)
                updateSubItems();

            treeHasChanged();
        }
        void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged)
        {
            treeHasChanged();
        }
        void valueTreeRedirected (ValueTree& treeWhichHasBeenChanged)
        {
            if (treeWhichHasBeenChanged == t)
                updateSubItems();

            treeHasChanged();
        }

        /* Works only if the ValueTree isn't updated between calls to getUniqueName. */
        String getUniqueName() const
        {
            if (t.getParent() == ValueTree::invalid) return "1";

            return String (t.getParent().indexOf (t));
        }

    private:
        PropertyEditor* propertiesEditor;
        ValueTree t;
        Array<Identifier> currentProperties;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Item);
    };


    /** Display a tree. */
    class Editor :
        public Component
    {
    public:
        Editor() :
            layoutResizer (&layout, 1, false)
        {
            layout.setItemLayout (0, -0.1, -0.9, -0.6);
            layout.setItemLayout (1, 5, 5, 5);
            layout.setItemLayout (2, -0.1, -0.9, -0.4);

            setSize (1000, 700);
            addAndMakeVisible (treeView);
            addAndMakeVisible (propertyEditor);
            addAndMakeVisible (layoutResizer);
        }
        ~Editor()
        {
            treeView.setRootItem (nullptr);
        }

        void resized()
        {
            Component* comps[] = { &treeView, &layoutResizer, &propertyEditor };
            layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
        }

        void setTree (ValueTree newTree)
        {
            if (newTree == ValueTree::invalid)
            {
                treeView.setRootItem (nullptr);
            }
            else if (tree != newTree)
            {
                tree = newTree;
                rootItem = new Item (&propertyEditor, tree);
                treeView.setRootItem (rootItem);
            }
        }

    public:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Editor);

        ScopedPointer<Item> rootItem;
        ValueTree tree;
        TreeView treeView;
        PropertyEditor propertyEditor;
        StretchableLayoutManager layout;
        StretchableLayoutResizerBar layoutResizer;
    };

public:
    ValueTreeEditor() :
        DocumentWindow ("Value Tree Editor",
                        Colours::lightgrey,
                        DocumentWindow::allButtons)
    {
        editor = new Editor();
        setContentNonOwned (editor, true);
        setResizable (true, false);
        setUsingNativeTitleBar (true);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }
    ~ValueTreeEditor()
    {
        editor->setTree (ValueTree::invalid);
    }

    void closeButtonPressed()
    {
        setVisible (false);
    }

    void setSource (ValueTree& v)
    {
        editor->setTree (v);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeEditor);
    ScopedPointer<Editor> editor;
};


#endif  // VALUE_TREE_EDITOR_H_INCLUDED
