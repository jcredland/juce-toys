juce-toys
=========

## NATVIS

A NatVis file is provided to help with debugging in Visual Studio.  You won't want to be without it.  Refer to microsofts NatVis instructions for installation.  It'll need to go in the right folder. 

## LLDB

An equivalent LLDB file is provided.  Installation instructions are in the comments at the top of the file.  See juce_lldb_xcode.py.

## JCF_DEBUG: JUCE Debugging Module

There are four development debugging utilities in the juce module jcf_debug.

I use these all the time.  They are: 
- credland_component_debugger - attach one of these to a component and get a list of all its children as a tree in a separate window.  Problem components are marked in grey (not visible), yellow (off screen) or red (zero size). (See the screenshot png in the root folder)
- value_tree_editor.h - attach this to a valuetree and a separate window will open where you can view (and change properties) of the tree. 
- buffer_visualiser.h - one of these can be used to view a buffer (typically an array of floats) when debugging DSP code.  You can put a macros into your code at places you want to be able to inspect the buffer contents.  I wrote it while debugging some auto-correlation code for a pitch shifter and it's been a life safer a couple of times since. 
- font and colour designer - which allows you to easily flick between colours and fonts for a component.  Handy when you are designing a UI. 

The debuggers aren't pretty - but they are functional and shouldn't crash!  Let me know if you get any problems!


** Other

A collection of parts and ideas which might be useful to people using JUCE. 

- adsr_editor.cpp, adsr_editor.h - is a basic, but nice looking envelope editor. 
- advanced_leak_detector.h - is a utility you can add to a class when you have a memory leak from an object.  it returns the stack back-trace of the leaked objects allowing you to find out where in your maze of source-code-complexity you created them!
- garbage_collected_object - is a garbage collector i use for handling the deletion of objects on the message thread when the audio thread has finished with them. 
- nonblocking_call_queue.h - provides a lock-free mechanism for inter-thread function calls.  very useful in conjunction with the garbage collector.
- value_tree_clone.h - jules may have made this a relic of history with recent changes to JUCE, however this is the class I use for cloning a ValueTree from my message thread to my audio thread without locks (works in conjunction with the nonblocking call queue and the garbage collector). 

