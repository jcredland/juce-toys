#ifndef ADSR_EDITOR_H_INCLUDED
#define ADSR_EDITOR_H_INCLUDED
/*
The MIT License (MIT)

Copyright (c) 2014, 2015 Jim Credland / Credland Technical Limited. 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "../JuceLibraryCode/JuceHeader.h"

/** A grab-anywhere ADSR envelope that's a little easier to use than the ones
 * you have to grab specific handles on. There are lots of improvements that
 * could be made to this, but it's a useful starting point. */
class ADSREditor
:
public Component
{
public:
    ADSREditor();
    ~ADSREditor() {}
    
    /** Updates the segment positions from ADSR position information,
     call this when the host, or loading a new patch, has caused the 
     values to change.  */
    void update();
    
    struct EnvelopeData
    {
        float attackLevel;
        float attackTime;
        float decay;
        float sustain; /** < 0.0 to 1.0. */
        float release;
        
        void drawDebugInformation(Graphics & g, Rectangle<float> area) const;
    private:
        void show(Graphics & g, const String & title, float value,
                  Rectangle<float> & area) const;
    };

    /** Access the ADSR structure. Call update() if you change the values. */
    EnvelopeData & getData() { return data; }
    
    /** Several of these are arranged horizontally to make up the envelope. */
    class Segment
    :
    public Component
    {
    public:
        Segment(ADSREditor * owner, Segment * segmentOnLeft)
        :
        owner(owner),
        right(nullptr),
        allowDurationChange(true)
        {
            if (segmentOnLeft != nullptr)
                segmentOnLeft->right = this;
            
            left = segmentOnLeft;
        }
        void mouseDown(const MouseEvent &) override;
        void mouseDrag(const MouseEvent &) override;
        void paint(Graphics &) override;
        
        enum ChainDirection
        {
            kLeft,
            kRight,
            kBoth
        };
        /** Returns the level, i.e. the Y amount, for this shape.
         Values are normalised from 0.0 to 1.0. */
        float getLeftLevel() const { return leftLevel; }
        float getRightLevel() const { return rightLevel; }
        
        void setLeftLevel(float newLevel, ChainDirection d = kBoth);
        void setRightLevel(float newLevel, ChainDirection d = kBoth);
        void setDuration(float newDuration) { duration = newDuration; }
        
        void setFixedDuration(float fixedDuration) { duration = fixedDuration; allowDurationChange = false; }
        
        /** 
         Returns the length of this segment.  Values are
         normalised from 0.0 to 1.0.
         */
        float getDuration() const { return duration; }
        
        void setYAxisControls(bool leftSide, bool rightSide);
        
    private:

        

        bool leftRightLinked() const { return controllingLeft && controllingRight; }


        ADSREditor * owner;
        
        Segment * left;
        Segment * right;
        
        float leftLevel;
        float rightLevel;
        float duration;
        
        bool controllingLeft;
        bool controllingRight;
        bool allowDurationChange;
        
        struct MouseDownInfo
        {
            float left;
            float right;
            float duration;
        } mouseDownData;
        
        const float mouseSensitivity = 200.0f; /**< Increase to reduce senstivity. */
    };


private:
    void resized() override;
    /** Called by the segments when some values have changed. */
    void updateFromSegments();
    /** Updates the bounds of the segment components. */
    void updateSegmentPositions();
    
    EnvelopeData data;
    enum SegmentOrder
    {
        kAttack,
        kDecay,
        kSustain,
        kRelease
    };
    OwnedArray<Segment> segments;
};

#endif  // ADSR_EDITOR_H_INCLUDED
