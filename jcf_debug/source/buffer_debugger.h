#ifndef BUFFER_VISUALISER_H_INCLUDED
#define BUFFER_VISUALISER_H_INCLUDED

/*
 * https://github.com/jcredland/juce-toys/
 */


/**
@file

This class lets you look at graphs of small buffers (of floats) during
execution.

Buffer visualiser allows objects to register buffers which might be of
interest to the developer.  The buffers are presumed to be smallish so
we do not provide zoom and scroll facilities.

The developer can select the named buffers from a list.

A trigger can be called to cause a particular buffer to be saved for
debugging.

The visualiser makes no attempt to be thread safe and just dips into
data owned by other objects and threads.

Using?

1. Insert one of these:

jcf::BufferVisualiser::capture(name, const float * data, size, min, max)

like this:

@code
jcf::BufferVisualiser::capture("DelayBuffer", delayBuffer, 1024, -1.0f, 1.0f);
@endcode

Somewhere in your code. Where delayBuffer is something like float
delayBuffer[1024] and contains the data you want to inspect.

2. And instantiate a BufferVisualiser::Visualiser somewhere,
perhaps in your MainComponent private member variables:

BufferDebugger visualiser;

@note
This code isn't pretty, and the UI isn't pretty - it's for debugging and
definitely not production use!

In the future I might modify it to support buffers of doubles and
other things.
*/

class BufferDebuggerMain;

/** Add one of these to your program to see the debug buffers. It
 will pop up an additional document window where buffers captured
 with 'capture' can be examined. */
class BufferDebugger
    :
    public DocumentWindow
{
public:
    BufferDebugger();

    ~BufferDebugger();

    /** Call this function to record a buffer. */
    static void capture (const String& name, const float* dataToCapture,
                         int size, float min, float max);
private:
    std::unique_ptr<BufferDebuggerMain> mainComponent;
};

#endif  // BUFFER_VISUALISER_H_INCLUDED
