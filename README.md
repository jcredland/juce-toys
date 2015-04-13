juce-toys
=========

A collection of parts and ideas which might be useful to people using JUCE. 

- adsr_editor.cpp, adsr_editor.h - is a basic, but nice looking envelope editor. 
- advanced_leak_detector.h - is a utility you can add to a class when you have a memory leak from an object.  it returns the stack back-trace of the leaked objects allowing you to find out where in your maze of source-code-complexity you created them!
- garbage_collected_object - is a garbage collector i use for handling the deletion of objects on the message thread when the audio thread has finished with them. 
- nonblocking_call_queue.h - provides a lock-free mechanism for inter-thread function calls.  very useful in conjunction with the garbage collector.
- value_tree_clone.h - jules may have made this a relic of history with recent changes to JUCE, however this is the class I use for cloning a ValueTree from my message thread to my audio thread without locks (works in conjunction with the nonblocking call queue and the garbage collector). 

## Debuggers

There are three debugging classes included that I use all the time.  They are: 
- debuggers/credland_component_debugger - attach one of these to a component and get a list of all its children as a tree in a separate window.  Problem components are marked in grey (not visible), yellow (off screen) or red (zero size). 
- debuggers/value_tree_editor.h - attach this to a valuetree and a separate window will open where you can view (and change properties) of the tree. 
- debuggers/buffer_visualiser.h - one of these can be used to view a buffer (typically an array of floats) when debugging DSP code.  You can put a macros into your code at places you want to be able to inspect the buffer contents.  I wrote it while debugging some auto-correlation code for a pitch shifter and it's been a life safer a couple of times since. 

The debuggers aren't pretty - but they are functional and shouldn't crash!  Let me know if you get any problems!

## Using
You will probably need to insert the #include "../JuceLibraryCode/JuceHeader.h" line into the header files to use them.  In a spare moment I'll turn them all into a JUCE module and this requirement will go away. 


