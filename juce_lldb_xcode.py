import lldb
#
# ABOUT
# 
# This file allows LLDB to decode some common JUCE structures, 
# in particular Arrays, Strings, var objects and ValueTrees.
#
# The facilities aren't quite as nice as those in VisualStudio 
# so if you do find you need to disable any functionality you 
# can do so in the __lldb_init_module function by commentting
# out the appropriate debugger.* line.
#
# Send bug reports, feature requests and issues to jim@credland.net
#
# USAGE
#
# Put this line in your ~/.lldbinit file: 
#
#  command script import [path]
#
# Where [path] is the full path to this file. For example:
# 
#  command script import /Users/me/juce-toys/juce_lldb_xcode.py
#
#
# (c) Credland Technical Limited.
# MIT License
#
# JCF_DEBUG - Debugging helpers for JUCE.  Demo application.
#
# Don't forget to install the VisualStudio or Xcode debug scripts as
# well.  These ensure that your IDEs debugger displays the contents
# of ValueTrees, Strings and Arrays in a useful way!
#
#
# Credland Technical Limited provide a range of consultancy and contract
# services including:
# - JUCE software development and support
# - information security consultancy
# 
# Contact jim@credland.net
# 
###################################
#
# Much Python/LLDB magic...

def __lldb_init_module(debugger, dict):
    print "-- juce decoding modules loaded.  www.credland.net"
    print " - refer to the help for the 'type' command"

    # ValueTree<*>
    debugger.HandleCommand('type synthetic add "juce::ValueTree" --python-class juce_lldb_xcode.ValueTreeChildrenProvider -w juce')

    # Array<*>
    debugger.HandleCommand('type synthetic add -x "^juce::(ReferenceCounted)?Array<.*>" --python-class juce_lldb_xcode.ArrayChildrenProvider -w juce')

    # Component
    debugger.HandleCommand('type summary add juce::Component -F juce_lldb_xcode.ComponentSummary -w juce')

    # String
    debugger.HandleCommand('type summary add juce::String -F juce_lldb_xcode.string_summary -w juce')

    # var
    debugger.HandleCommand('type summary add juce::var -F juce_lldb_xcode.var_summary -w juce')

    # Identifier
    debugger.HandleCommand('type summary add juce::Identifier -F juce_lldb_xcode.identifier_summary -w juce')

    # ScopedPointer
    debugger.HandleCommand('type summary add -x "^juce::ScopedPointer<.*>" --summary-string "ScopedPointer<>=${*var.object}" -w juce')

    # Rectangle<*>
    debugger.HandleCommand('type summary add -x "^juce::Rectangle<.*>" --summary-string "${var.pos.x} ${var.pos.y} ${var.w} ${var.h}" -w juce')
    debugger.HandleCommand('type category enable juce')


def string_summary(valueObject, dictionary):
    #print dir(lldb.frame)
    s = valueObject.GetChildMemberWithName('text').GetChildMemberWithName('data').GetSummary()
    return s

def identifier_summary(valueObject, dictionary):
    s = valueObject.GetChildMemberWithName('name').GetChildMemberWithName('text').GetChildMemberWithName('data').GetSummary()
    if s is None:
        return "((uninitalized))"
    else:
        return s
    
def rect_summary(valueObject, dictionary):
    w = valueObject.GetChildMemberWithName('w').GetValue()
    h = valueObject.GetChildMemberWithName('h').GetValue()
    pos = valueObject.GetChildMemberWithName('pos')
    x = pos.GetChildMemberWithName('x').GetValue()
    y = pos.GetChildMemberWithName('y').GetValue()

    s = "x=" + str(x) + " y=" + str(y)

    s = s + " w=" + str(w) + " h=" + str(h)

    return s

# Types of VAR
#    class VariantType;            friend class VariantType;
#
#    class VariantType_Void;       friend class VariantType_Void;
#    class VariantType_Undefined;  friend class VariantType_Undefined;
#    class VariantType_Int;        friend class VariantType_Int;
#
#    class VariantType_Int64;      friend class VariantType_Int64;
#    class VariantType_Double;     friend class VariantType_Double;
#    class VariantType_Bool;       friend class VariantType_Bool;
#
#    class VariantType_String;     friend class VariantType_String;
#    class VariantType_Object;     friend class VariantType_Object;
#    class VariantType_Array;      friend class VariantType_Array;
#
#    class VariantType_Binary;     friend class VariantType_Binary;
#    class VariantType_Method;     friend class VariantType_Method;

def var_summary(valueObject, dictionary):
    varType = valueObject.GetChildMemberWithName('type').GetType()
    varValue = valueObject.GetChildMemberWithName('value')
    s = "t"
    if varType.GetName() == "juce::var::VariantType_String *":
        stringValue = varValue.GetChildMemberWithName('stringValue')
        stringPointerType = valueObject.GetFrame().GetModule().FindFirstType('juce::String').GetPointerType()
        print stringPointerType
        stringPointer = stringValue.Cast(stringPointerType)
        print stringPointer
        s = 'string=' + stringPointer.Dereference().GetSummary() + stringPointer.GetValue()
        # now we have to convert the .value to a pointer ... 
    if varType.GetName() == "juce::var::VariantType_Int *":
        s = 'int=' + varValue.GetSummary()
    return s

def ComponentSummary(valueObject, dictionary):
    # Component 'name' parent=None visible=True location={ 0, 0, 20, 20 } opaque=False
    print int(valueObject.GetChildMemberWithName('parentComponent').GetValue(), 16) 
    hasParent = int(valueObject.GetChildMemberWithName('parentComponent').GetValue(), 16) == 0
    isVisible = valueObject.GetChildMemberWithName('flags').GetChildMemberWithName('visibleFlag').GetValue()
    print type(isVisible)
    name = string_summary(valueObject.GetChildMemberWithName('componentName'), dictionary)

    s = name + " hasParent=" + str(hasParent) + " isVisible=" + str(isVisible)

    return s




class ValueTreeChildrenProvider:
    def __init__(self, valueObject, internalDict):
        # this call should initialize the Python object using valobj as the
        # variable to provide synthetic children for. 
        #
        # JCF_ Most of the actual work is done in update()
        print "VTCP with " + valueObject.GetName()
        self.v = valueObject
        self.count = 0
        self.update()
        return

    def num_children(self): 
        # Return (a) properties and (b) children
        return 4 

    def get_child_index(self, name): 
        # this call should return the index of the synthetic child whose name is
        # given as argument 
        print "get_child_index: " + name
        try:
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self, index): 
        if index == 0:
            return self.ro.GetChildMemberWithName('properties').GetChildMemberWithName('values')

        if index == 1:
            return self.ro.GetChildMemberWithName('children')
        
        if index == 2:
            return self.ro.GetChildMemberWithName('parent')

        if index == 3:
            return self.ro.GetChildMemberWithName('type')

        return None

    def update(self): 
        # this call should be used to update the internal state of this Python
        # object whenever the state of the variables in LLDB changes.[1]
        self.ro = self.v.GetChildMemberWithName('object').GetChildMemberWithName('referencedObject')
        return 

    def has_children(self): 
        # this call should return True if this object might have children, and False if
        # this object can be guaranteed not to have children.[2]
        return True


class ArrayChildrenProvider:
    def __init__(self, valueObject, internalDict):
        # this call should initialize the Python object using valobj as the
        # variable to provide synthetic children for. 
        #
        # JCF_ Most of the actual work is done in update()
        print "ACP with " + valueObject.GetName()
        self.v = valueObject
        self.count = 0
        self.update()
        return

    def num_children(self): 
        # this call should return the number of children that you want your
        # object to have 
        #
        # JC - no matter what we do here it seems to call get_child_at_index
        # target.max-children times
        if (self.count > 100):
            print "WARNING count = " + self.count
        return int(self.count)

    def get_child_index(self, name): 
        # this call should return the index of the synthetic child whose name is
        # given as argument 
        print "get_child_index: " + name
        try:
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self, index): 
        # this call should return a new LLDB SBValue object representing 
        # the child at the index given as argument 
        if index < 0:
            return None
        
        if index >= self.count:
            return None

        try:
            offset = index * self.data_size
            return self.first_element.CreateChildAtOffset('[' + str(index) + ']', offset, self.data_type)
        except:
            print "Array<> error"
            return None

    def update(self): 
        # this call should be used to update the internal state of this Python
        # object whenever the state of the variables in LLDB changes.[1]
        data = self.v.GetChildMemberWithName('data')
        self.first_element = data.GetChildMemberWithName('elements').GetChildMemberWithName('data')
        self.data_type = self.first_element.GetType().GetPointeeType()
        self.data_size = self.data_type.GetByteSize()

        if self.first_element.IsValid():
            # GetValue() returns a string(!) so we have to cast to an integer here.
            try:
                self.count = int(self.v.GetChildMemberWithName('numUsed').GetValue())
            except TypeError:
                print "ACP invalid numUsed.  Set count to 0"
                self.count = 0
        else:
            self.count = 0

        print "ACP set self.count to " + str(self.count)

        return 

    def has_children(self): 
        # this call should return True if this object might have children, and False if
        # this object can be guaranteed not to have children.[2]
        return self.count > 0

