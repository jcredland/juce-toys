
class FontAndColorContent;

/**
 * https://github.com/jcredland/juce-toys/
 *
 * An instance of FontAndColourDesign will allow you to 
 * control one font and one colour while your application 
 * is running.
 *
 * This is handy if you are indecisive about how something
 * should look whilst writing your UI. 
 */

class FontAndColourDesigner
:
public Component
{
public:
    FontAndColourDesigner(Component & parentComponent,
                          Colour & colourToUpdate,
                          Font & fontToUpdate);

    void paint(Graphics & g) override;
    void mouseUp(const MouseEvent &) override;
private:
    DocumentWindow window;
    ScopedPointer<FontAndColorContent> content;
};

