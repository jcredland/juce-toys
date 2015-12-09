/*
  ==============================================================================

    jcf_gui.h
    Created: 21 Jul 2014 12:08:08pm
    Author:  Jim Credland

  ==============================================================================
*/

#ifndef JCF_DEBUG_H_INCLUDED
#define JCF_DEBUG_H_INCLUDED

#include "AppConfig.h"
#include <map>
#include <modules/juce_core/juce_core.h>
#include <modules/juce_graphics/juce_graphics.h>
#include <modules/juce_gui_basics/juce_gui_basics.h>
#include <modules/juce_gui_extra/juce_gui_extra.h>

namespace jcf {
    
using namespace juce;

#include "source/buffer_debugger.h"
#include "source/value_tree_debugger.h"
#include "source/component_debugger.h"
#include "source/font_and_colour_designer.h"
#include "source/advanced_leak_detector.h"

}

#endif  // JCF_DEBUG_H_INCLUDED
