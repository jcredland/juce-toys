/*
  ==============================================================================

    jcf_gui.h
    Created: 21 Jul 2014 12:08:08pm
    Author:  Jim Credland

  ==============================================================================
*/

/*

 BEGIN_JUCE_MODULE_DECLARATION

  ID:               jcf_debug
  vendor:           juce
  version:          4.2.1
  name:             JCF DEBUG
  description:      Debugging utilities for JUCE
  website:          http://www.github.com/jcredland/
  license:          MIT

  dependencies:     juce_core juce_graphics juce_gui_basics juce_gui_extra
  OSXFrameworks:
  iOSFrameworks:

 END_JUCE_MODULE_DECLARATION
 */

#ifndef JCF_DEBUG_H_INCLUDED
#define JCF_DEBUG_H_INCLUDED
#include <map>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace jcf {

using namespace juce;

#include "source/buffer_debugger.h"
#include "source/value_tree_debugger.h"
#include "source/component_debugger.h"
#include "source/font_and_colour_designer.h"

}

#endif  // JCF_DEBUG_H_INCLUDED
