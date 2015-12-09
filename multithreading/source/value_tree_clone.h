/*
  ==============================================================================

    value_tree_clone.h
    Created: 4 Aug 2014 2:58:12pm
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef VALUE_TREE_CLONE_H_INCLUDED
#define VALUE_TREE_CLONE_H_INCLUDED



/** @brief A GarbageCollectedObject wrapper around a ValueTree. 
 * You shouldn't have to create one of these directly.  See @CriticalThreadValueTree */
class ValueTreeCopy :
    public GarbageCollectedObject
{
public:
    typedef ReferenceCountedObjectPtr<ValueTreeCopy> Ptr;
    ValueTreeCopy (ValueTree copyFrom)
    {
        t = copyFrom.createCopy();
    }
    ~ValueTreeCopy() { }
    ValueTree& getReference() { return t; }
private:
    ValueTree t;
};

/** @brief Maintain a clone of a ValueTree on a critical thread.
 *
 * Maintains a clone of a value tree for a critical thread. For example for 
 * passing complex configuration from a GUI to an audio processing thread.
 *
 * @note Simple properties (Strings, integers and so on) are updated with a
 * simple message. Adding properties or children results in a re-copy of the
 * whole ValueTree.  This is fine for small trees, but may need some
 * optimisation if things get bigger.  
 *
 * @note JUCE now has a ValueTreeSynchroniser class which may be useful instead
 * of CriticalThreadValueTree
 */
class CriticalThreadValueTree :
    public ValueTree::Listener
{
private:
    /** @internal */
    class ValueTreeLinkCache
    {
    public:
        ValueTree operator[] (const ValueTree& t)
        {
            for (auto& s : updateMap)
            {
                if (s.main == t) return s.copy;
            }

            return ValueTree::invalid;
        }

        void clear()
        {
            updateMap.clear();
        }
        /** @internal */
        void add (const ValueTree& src, const ValueTree& copy)
        {
            MapEntry x;
            x.main = src;
            x.copy = copy;
            updateMap.push_back (x);
        }
        /** @internal This could be a better, faster, structure. */
        struct MapEntry
        {
            ValueTree main, copy;
        };
        std::vector<MapEntry> updateMap;
    };

public:
    /** @brief The output - the clone of the ValueTree. 
     *
     * A read-only copy of the value tree that you can access safely from your
     * critical thread. 
     */
    typename ValueTreeCopy::Ptr readonly;

    /** @brief Configure the CriticalThreadValueTree.
     *
     * Requires a LockFreeCallQueue to be passed which will be used for the
     * synchronisation.  You will need to regularly call synchronise on the
     * LockFreeCallQueue from the critical thread so that the ValueTree gets
     * updated.
     */
    CriticalThreadValueTree (ValueTree source, LockFreeCallQueue& q) :
        jobsForCriticalThread (q)
    {
        readonly = new ValueTreeCopy (ValueTree ("null"));
        setSource (source);
    }

    ~CriticalThreadValueTree()
    {}

    /** @brief set the source tree to copy.  This is set initally by the
     * constructor, so you may not need to call this function. */
    void setSource (ValueTree source)
    {
        sourceTree.removeListener (this);
        sourceTree = source;
        sourceTree.addListener (this);
        syncAll();
    };

    /** @internal */
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
    {
        syncAll();
    }
    /** @internal */
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved)
    {
        syncAll();
    }
    /** @internal */
    void valueTreeChildOrderChanged (ValueTree& parentTreeWhoseChildrenHaveMoved)
    {
        syncAll();
    }
    /** @internal */
    void valueTreeParentChanged (ValueTree& treeWhoseParentHasChanged) {}

    /** @brief Synchronise.  You shouldn't have to call this.  It should be called
     * automatically from the non-critical thread when the ValueTree changes. */
    void syncAll()
    {
        /* Create a deep copy of the value tree.  Pass it to the other thread. */
        ValueTreeCopy* t = new ValueTreeCopy (sourceTree);
        /* Create tree to tree mapping so property updates can happen quickly. */
        updatePropertymap (sourceTree, t->getReference());
        /* Send it over. */
        sendReplaceValueTree (t);
    }


    /** @internal */
    void valueTreePropertyChanged (ValueTree& tree, const Identifier& property)
    {
        /* Send an update property message. Property removals are handled by sending
         a null var() object.

         A property addition however requires that we replace the entire value tree
         to avoid calling malloc on the audio thread.
         */
        if (! isPropertyChangedOperationLockFree (tree, property))
        {
            syncAll();
            return;
        }

        /* Create a new ValueTree that points to the same underlying data. */
        ValueTree target = linkCache[tree];
        /* Create a var object. Note: var manages the reference counting. */
        var v = tree[property];
        /* If it's an object, check that the GarbageCollector knows about it.
         Otherwise it may end up being deleted on the critical thread.
         
         If you hit this assert you probably aren't using a GarbageCollectedObject
         and instead you've got a standard ReferenceCountedObject here.
         */
        jassert (
            (! v.isObject()) ||
            (GarbageCollector::getInstance()->isInList (v.getObject()))
        );
        /* Pass these objects to the critical thread to update the critical thread's
         own ValueTree. */
        sendUpdateProperty (target, property, v);
    }


private:
    bool isPropertyChangedOperationLockFree (ValueTree& tree, const Identifier& property)
    {
        if (tree[property].isString()) return false; /* Might be okay? */

        ValueTree t = linkCache[tree];

        if (t == ValueTree::invalid)
        {
            jassertfalse; /* This shouldn't happen: we should have all the nodes at least. */
            return false;
        }

        return (t.hasProperty (property));
    }
    /** @internal Create a series of ValueTree objects in one tree that map to
     ones in another copy of that tree. The copy must be identical to the
     source otherwise this may fail badly. */
    void updatePropertymap (ValueTree& src, ValueTree& copy)
    {
        linkCache.clear();
        recreatePropertyUpdateMap (sourceTree, copy);
    }
    /* Can this be converted to pass by const reference? */
    void recreatePropertyUpdateMap (ValueTree src, ValueTree copy)
    {
        for (int i = 0; i < src.getNumChildren(); ++i)
        {
            recreatePropertyUpdateMap (src.getChild (i), copy.getChild (i));
        }

        linkCache.add (src, copy);
    }

    void sendUpdateProperty (ValueTree& target, const Identifier& property, const var& value)
    {
        jobsForCriticalThread.callf (std::bind (&CriticalThreadValueTree::updatePropertyOnCriticalThread,
                                                this,
                                                target, property, value));
    }

    void updatePropertyOnCriticalThread (ValueTree target, Identifier property, var value)
    {
        target.setProperty (property, value, nullptr);
    }

    /** Replace the complete tree. */
    void sendReplaceValueTree (ValueTreeCopy* t)
    {
        ValueTreeCopy::Ptr p = t;
        jobsForCriticalThread.callf (std::bind (&CriticalThreadValueTree::replaceValueTree,
                                                this,
                                                p));
    }
    
    void replaceValueTree (typename ValueTreeCopy::Ptr replacementTree)
    {
        /* readonly old one will be deleted on message thread . */
        DBG ("replacing tree with: " + replacementTree->getReference().toXmlString());
        readonly = replacementTree;
    }

    ValueTreeLinkCache linkCache;
    LockFreeCallQueue& jobsForCriticalThread;
    ValueTree sourceTree;
};



#endif  // VALUE_TREE_CLONE_H_INCLUDED
