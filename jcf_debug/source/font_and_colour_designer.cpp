


class DesignerColourSelector
:
public ColourSelector,
public ChangeListener
{
public:
    DesignerColourSelector(Component & componentToRefresh,
                           Colour & colourToUpdate)
    :
    comp(componentToRefresh),
    targetColour(colourToUpdate)
    {
        setSize(300, 400);
        addChangeListener(this);
    }
    void setSwatchColour(int index, const Colour & newColour) const override
    {
        targetColour = newColour;
        comp.repaint();
    }
    void changeListenerCallback(ChangeBroadcaster *)
    {
        targetColour = getCurrentColour();
        comp.repaint();
    }
private:
    Component & comp;
    Colour & targetColour;
};

/** A design support class that displays a small square in the top left
 of a component. When clicked it displays a pop up font selection menu.
 When a font it selected it calls repaint on it's parent. */
class DesignerSystemFontSelector
:
public Component,
public ListBoxModel
{
public:
    DesignerSystemFontSelector(Component & componentToRepaint,
                               Font & fontToControl)
    :
    comp(componentToRepaint),
    font(fontToControl)
    {
        fontNames = Font::findAllTypefaceNames();

        list.setModel(this);
        addAndMakeVisible(list);
    }

    void resized()
    {
        list.setBounds(getLocalBounds());
    }


    void nextFont()
    {
        number++;

        if (number >= fontNames.size())
            number = fontNames.size() - 1;

        updateFont();
    }

    void selectedRowsChanged(int lastRowSelected)
    {
        number = lastRowSelected;
        updateFont();
    }

    void updateFont()
    {
        float h = font.getHeight();
        font = Font(fontNames[number], h, 0);
        comp.repaint();
    }

    int getNumRows() override { return fontNames.size(); }
    void paintListBoxItem (int rowNumber, Graphics &g,
                           int width, int height,
                           bool rowIsSelected) override
    {
        g.fillAll(rowIsSelected ? Colours::red : Colours::black);
        g.setColour(Colours::white);
        g.drawText(fontNames[rowNumber], 0, 0, width, height, Justification::left, false);
    }
private:
    Component & comp;
    int number;
    StringArray fontNames;

    ListBox list;

    Font & font;
};


/** A design support class that displays a small square in the top left
 of a component. When clicked it displays a pop up font selection menu.
 When a font it selected it calls repaint on it's parent. */
class DesignerUserFontSelector
:
public Component,
public Button::Listener,
public FileBrowserListener
{
public:
    DesignerUserFontSelector(Component & componentToRepaint,
                             Font & fontToControl)
    :
    comp(componentToRepaint),
    fileChooser("Select a folder"),
    thread("Directory Contents Scanning Thread"),
    directoryContentsList(nullptr, thread),
    tree(directoryContentsList),
    openButton("Open Folder"),
    font(fontToControl)
    {
        thread.startThread();
        addAndMakeVisible(tree);
        addAndMakeVisible(openButton);

        openButton.addListener(this);
        tree.addListener(this);
    }

    void resized()
    {
        tree        .setBounds(getLocalBounds().withTrimmedBottom(20));
        openButton  .setBounds(getLocalBounds().withTop(getHeight()-20));
    }

    void updateFont(const Typeface::Ptr & typefaceToUse)
    {
        float h = font.getHeight();
        font = Font(newTypeface);
        font.setHeight(h);
        comp.repaint();
    }

    void buttonClicked(Button *)
    {
        bool success = fileChooser.browseForDirectory();

        if (success)
        {
            directoryContentsList.setDirectory(fileChooser.getResult(), true, true);
            tree.refresh();
        }
    }


    void fileClicked(const File & newFile, const MouseEvent &) override
    {
        MemoryBlock mb;
        bool success = newFile.loadFileAsData(mb);

        if (! success)
            return;

        if ( !(newFile.hasFileExtension("ttf") || newFile.hasFileExtension("otf")))
            return;

        newTypeface = Typeface::createSystemTypefaceFor(mb.getData(), mb.getSize());

        updateFont(newTypeface);
    }

    /* Unused overrides from FileBrowserListener. */
    void selectionChanged() override {}
    void fileDoubleClicked(const File &) override {}
    void browserRootChanged(const File &) override {}

private:
    Component & comp;
    FileChooser fileChooser;

    /* Order matters. */
    TimeSliceThread thread;
    DirectoryContentsList directoryContentsList;
    FileTreeComponent tree;
    /* Order ceases to matter. */

    TextButton openButton;
    Typeface::Ptr newTypeface;
    Font & font;
};


class FontAndColorContent
:
public TabbedComponent
{
public:
    FontAndColorContent(Component & parentComponent,
                        Colour & colourToUpdate,
                        Font & fontToUpdate)
    :
    TabbedComponent(TabbedButtonBar::Orientation::TabsAtLeft),
    colourSelector(parentComponent, colourToUpdate),
    userFontSelector(parentComponent, fontToUpdate),
    systemFontSelector(parentComponent, fontToUpdate)
    {
        addTab("Colours",       Colours::darkgrey, &colourSelector,     false);
        addTab("User Fonts",    Colours::darkgrey, &userFontSelector,   false);
        addTab("System Fonts",  Colours::darkgrey, &systemFontSelector, false);

        setSize(300, 400);
    }
private:
    DesignerColourSelector colourSelector;
    DesignerUserFontSelector userFontSelector;
    DesignerSystemFontSelector systemFontSelector;
};

/*****/

FontAndColourDesigner::FontAndColourDesigner(Component & parentComponent,
                      Colour & colourToUpdate,
                      Font & fontToUpdate)
:
window("Font and Colour", Colours::black, DocumentWindow::TitleBarButtons::allButtons, false)
{
    content = new FontAndColorContent(parentComponent, colourToUpdate, fontToUpdate);

    window.setContentComponentSize(content->getWidth(), content->getHeight());
    window.setContentNonOwned(content, true);

    parentComponent.addAndMakeVisible(this);
    setSize(10, 10);
}


void FontAndColourDesigner::paint(Graphics & g)
{
    g.fillAll(Colours::red.withAlpha(0.2f));
}

void FontAndColourDesigner::mouseUp(const MouseEvent &)
{
    window.addToDesktop();
    window.setVisible(true);
}
