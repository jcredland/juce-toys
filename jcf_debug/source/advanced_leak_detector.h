
/** Stick an instance of this in a class that JUCE is reporting memory leaks for
 and you'll get a nice report with a stack trace which should lead you to a
 happy place quite quickly.
 
 It might slow down your program a bit, so don't forget to take it out once your 
 problems are all fixed.  In fact. I've wrapped it in a #ifdef JUCE_DEBUG so you
 can't forget.  Forgetting would be bad.
 
 If the JUCE_LEAK_DETECTOR assert fires first, just push the continue button 
 on your debugger and this should report straight afterwards.
 */
#ifdef JUCE_DEBUG
class AdvancedLeakDetector {
public:
    AdvancedLeakDetector() {
        getBackTraceHash().set((void *)this, SystemStats::getStackBacktrace());
    }
    ~AdvancedLeakDetector() {
        getBackTraceHash().remove((void *) this);
    }


private:
    typedef HashMap<void*, String> BackTraceHash;

    struct HashHolder {
        ~HashHolder() {
            if (traces.size()>0)
            {
                /* Memory leak info. */
                DBG("Found " + String(traces.size()) + " possible leaks");
                for (BackTraceHash::Iterator i (traces); i.next();)
                {
                    DBG("-----");
                    DBG (i.getValue());
                }
                jassertfalse;
            }
        }
        BackTraceHash traces;
    };

    BackTraceHash & getBackTraceHash() {
        static HashHolder holder;
        return holder.traces;
    }

};
#endif
