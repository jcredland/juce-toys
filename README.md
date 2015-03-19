juce-toys
=========

A collection of parts and ideas which might be useful to people using JUCE. 

- adsr_editor.cpp, adsr_editor.h - is a basic, but nice looking envelope editor. 
- advanced_leak_detector.h - is a utility you can add to a class when you have a memory leak from an object.  it returns the stack back-trace of the leaked objects allowing you to find out where in your maze of source-code-complexity you created them!
- garbage_collected_object - is a garbage collector i use for handling the deletion of objects on the message thread when the audio thread has finished with them. 
- nonblocking_call_queue.h - provides a lock-free mechanism for inter-thread function calls.  very useful in conjunction with the garbage collector.
- value_tree_clone.h - jules may have made this a relic of history with recent changes to JUCE, however this is the class I use for cloning a ValueTree from my message thread to my audio thread without locks (works in conjunction with the nonblocking call queue and the garbage collector). 
