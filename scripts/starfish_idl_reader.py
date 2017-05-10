#!/usr/bin/env python
import os
import sys
import types
from functools import partial

from starfish_idl_lexer import StarfishIDLLexer
from starfish_idl_parser import StarfishIDLParser
from starfish_ir_handler import StarfishIRHandler

def CHECK_STRING_T(v, msg=""):
  if type(v) is not types.StringType:
    raise RuntimeError(msg)

def CHECK_BOOL_T(v, msg=""):
  if type(v) is not types.BooleanType:
    raise RuntimeError(msg)

def CHECK_LIST_T(v, msg=""):
  if type(v) is not types.ListType:
    raise RuntimeError(msg)

def CHECK_NOT_NONE(v, msg=""):
  if v is None:
    raise RuntimeError(msg)

def CHECK_HAS_LENGTH(v, msg=""):
  if len(v) is 0:
    raise RuntimeError(msg)

##### Reserved Class Names
_C_CONST = 'Const'
_C_ARGUMENT = 'Argument'
_C_ARGUMENTS = 'Arguments'
_C_ATTRIBUTE = 'Attribute'
_C_EXTATTRIBUTES = 'ExtAttributes'
_C_INHERIT = 'Inherit'
_C_OPERATION = 'Operation'
_C_PRIMITIVE_TYPE = 'PrimitiveType'
_C_TYPE = 'Type'
_C_VALUE = 'Value'
##### Reserved Property Names
_P_NAME = 'NAME'
_P_NULLABLE = 'NULLABLE'
_P_READONLY = 'READONLY'
##### Expected IR Property Names
_K_ATTRIBUTES = 'attributes'
_K_CONSTRUCTOR = 'constructor'
_K_FLAG = 'flag'
_K_FUNCTIONS = 'functions'
_K_LENGTH = 'length'
_K_MODULE = 'module'
_K_NAME = 'name'
_K_PARENT = 'parent'
_K_READONLY = 'readonly'
_K_TYPE = 'type'
_K_UNIMPL = 'unimplemented'
_K_VALUE = 'value'
_K_VALUESTR = 'valuestr'

##### Extended Attr
_EXT_CONSTRUCTOR = 'Constructor'
##### Extended Attr for Starfish
_EXTSF_ENABLE_TEST = 'STARFISH_ENABLE_TEST'
_EXTSF_UNIMPL = 'Unimplemented'
#####


##########################################################
# Please remove these functions before deploy

def __pre_br(text):
    return '\n' + text

def __indent(text):
    return ''.join('   '+line for line in text.splitlines(True))

__SKIP_DUMP_NAMES = [
  'ERRORS', 'WARNINGS', 'FILENAME', 'POSSITION', 'LINENO'
]
def dump_node(node):
  result = '[%s]' % node.GetClass()
  for key, value in node.GetProperties().iteritems():
    if not key in __SKIP_DUMP_NAMES:
      result += __indent(__pre_br('%s: %s' % (key, value)))
  children = node.GetChildren()
  if len(children) > 0:
    result += __indent(__pre_br('CHILDREN:'))
    subresult = ''
    for child_node in children:
      subresult += __pre_br(dump_node(child_node))
    result += __indent(__indent(subresult))
  return result

##########################################################
def _is_class(node, class_name):
  return node.GetClass() == class_name

def _value_to_str(value, type):
  if 'String' in type:
    return '\"' + str(value) + '\"'
  elif type == 'boolean':
    return str(value).lower()
  elif type == 'NULL':
    return 'nullptr'
  return value

def _set_prop_to_dict(target, prop_name, v):
  if v is not None:
    target[prop_name] = v

def _set_boolean_prop(target, node, pro_name_from, pro_name_to):
  if node.GetProperty(pro_name_from):
    target[pro_name_to] = True

def _set_value_prop(target, node, pro_name_from, pro_name_to):
  value = node.GetProperty(pro_name_from)
  if value is not None:
    target[pro_name_to] = value

def _handle_extattrs(target, extattrs, handlers):
  for extattr in extattrs:
    for handler in handlers:
      if handler(target, extattr):
        break;

def _hd_extattr_unimplemented(target, extattr):
  if extattr.GetName() == _EXTSF_UNIMPL:
    target[_K_UNIMPL] = True
    return True
  return False

def _hd_extattr_treatnull(target, extattr):
  if extattr.GetName() == 'TreatNullAs':
    target['treat_null_as'] = extattr.GetProperty('VALUE')
    return True
  return False

def _hd_extattr_flags(target, extattr):
  if extattr.GetName() == 'STARFISH_TC_COVERAGE':
    target['check_tc_coverage'] = True
    return True
  elif 'STARFISH_' in extattr.GetName():
    # STARFISH_ENABLE_TEST
    # STARFISH_EXP
    # STARFISH_ENABLE_WASU
    if target.get('flags') is None:
      target['flags'] = []
    target.get('flags').append(extattr.GetName())
    return True
  return False

def _hd_extattr_clamp(target, extattr):
  if extattr.GetName() == 'Clamp':
    target['clamp'] = True
    return True
  return False

def _hd_extattr_cereactions(target, extattr):
  if extattr.GetName() == 'CEReactions':
    target['cereactions'] = True
    return True
  return False

def _hd_extattr_object_opt(target, extattr):
  if extattr.GetName() == 'NewObject' or extattr.GetName() == 'SameObject':
    target['object_option'] = extattr.GetName()
    return True
  return False

def _hd_extattr_rename(target, extattr):
  if extattr.GetName() == 'Rename':
    target['rename'] = extattr.GetProperty('VALUE')
    return True
  return False

def _hd_extattr_unforgeable(target, extattr):
  if extattr.GetName() == 'Unforgeable':
    target['unforgeable'] = True
    return True
  return False

def _hd_extattr_notenumerable(target, extattr):
  if extattr.GetName() == 'NotEnumerable':
    target['enumerable'] = False
    return True
  return False

def _hd_extattr_no_interfaceobj(target, extattr):
  if extattr.GetName() == 'NoInterfaceObject':
    target['no_interface'] = True
    return True
  return False

def _hd_extattr_custom(target, extattr):
  if extattr.GetName() == 'Custom':
    target['custom'] = extattr.GetProperty('VALUE', True)
    return True
  return False

def _hd_extattr_putforward(target, extattr):
  if extattr.GetName() == 'PutForwards':
    _set_prop_to_dict(target, 'put_forwards', extattr.GetProperty('VALUE', None))
    return True
  return False

def _hd_extattr_raise_expection(target, extattr):
  if extattr.GetName() == 'RaisesException':
    target['raises_exception'] = extattr.GetProperty('VALUE', True)
    return True
  return False

def _hd_extattr_force_deny_strict(target, extattr):
  if extattr.GetName() == 'ForceDenyStrictMode':
    target['force_deny_strict'] = True
    return True
  return False

def _hd_extattr_callwith(target, extattr):
  if extattr.GetName() == 'CallWith':
    target['call_with'] = extattr.GetProperty('VALUE')
    return True
  return False

def _hd_extattr_primary_global(target, extattr):
  if extattr.GetName() == 'PrimaryGlobal':
    target['primary_global'] = True
    return True
  return False

def _gen_basic_named(node):
  return {
    'name': node.GetName(),
    'kind': node.GetClass(),
  }

NODE_KIND = ['Interface', 'Dictionary', 'Typedef', 'Enum', 'Callback']
def _propname_from_kind(kind):
  if kind == 'Interface':
    return 'interfaces'
  elif kind == 'Dictionary':
    return 'dictionaries'
  elif kind == 'Typedef':
    return 'typedefs'
  elif kind == 'Enum':
    return 'enums'
  elif kind == 'Callback':
    return 'callbacks'
  return 'errors'

##########################################################
class StarfishIDLReader():
  def _hd_extattr_constructor(self, target, extattr):
    if extattr.GetName() == 'NamedConstructor':
      # NamedConstructor
      for child in extattr.GetChildren():
        if _is_class(child, 'Call'):
          constructor = self._gen_ir_constructor(child)
          _set_prop_to_dict(target, 'constructor', constructor)
          return True
    elif extattr.GetName() == 'HTMLConstructor':
      target['HTMLConstructor'] = True
      return True
    elif extattr.GetName() == 'ConstructorCallWith':
      target['_call_with'] = extattr.GetProperty('VALUE')
      return True
    elif 'Constructor' in extattr.GetName():
      # Constructor
      prototype = 'Error' if 'Exception' in extattr.GetName() else 'Object'
      constructor = self._gen_ir_constructor(extattr, prototype)
      _set_prop_to_dict(constructor, 'name', '')
      _set_prop_to_dict(target, 'constructor', constructor)
      return True
    return False

  def _gen_ir_constructor(self, node, prototype='Object'):
    constructor = self._gen_ir_operation(node)
    _set_prop_to_dict(constructor, 'prototype', prototype)
    _set_prop_to_dict(constructor, 'kind', 'Operation')
    if 'Custom' in node.GetName():
      _set_prop_to_dict(constructor, 'custom', True)
    elif 'Unimplemented' in node.GetName():
      _set_prop_to_dict(constructor, 'unimplemented', True)
    return constructor

  def _gen_ir_not_implemented(self, node):
    result = { 'kind': 'NOT_IMPLEMENTED' }
    self.errors.append(result)
    return result

  def _gen_ir_dictionary(self, node):
    # print dump_node(node)
    result = _gen_basic_named(node)
    keys = []
    for child in node.GetChildren():
      if _is_class(child, 'Key'):
        keys.append(self._gen_ir_argument(child))
      elif _is_class(child, 'Inherit'):
        result['parent'] = child.GetName()
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags,
                          _hd_extattr_unimplemented])
    _set_prop_to_dict(result, 'members', keys)
    _set_prop_to_dict(result, 'file_path', self.file_path)
    self.dictionaries[node.GetName()] = result
    return result

  def _gen_ir_typedef(self, node):
    result = _gen_basic_named(node)
    for child in node.GetChildren():
      if _is_class(child, 'Type'):
        result['from'] = self._gen_ir_type(child)
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags])
    self.typedefs[node.GetName()] = result
    return result

  def _gen_ir_enum(self, node):
    result = _gen_basic_named(node)
    items = []
    for child in node.GetChildren():
      if _is_class(child, 'EnumItem'):
        items.append(child.GetName())
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags])
    result['data'] = items
    self.enums[node.GetName()] = result
    return result

  def _gen_ir_type(self, node):
    # print dump_node(node)
    result = {}
    _set_boolean_prop(result, node, 'NULLABLE', 'nullable')
    for child in node.GetChildren():
      if _is_class(child, 'Sequence') or\
         _is_class(child, 'Promise'):
        # Sequence | Promise
        result['kind'] = child.GetClass()
        result['name'] = child.GetClass()
        for subt in child.GetChildren():
          if subt.GetClass() == 'Type':
            result['data'] = self._gen_ir_type(subt)
            break
      elif _is_class(child, 'UnionType'):
        result['kind'] = child.GetClass()
        result['name'] = child.GetClass()
        subtypes = []
        for subt in child.GetChildren():
          if subt.GetClass() == 'Type':
            subtypes.append(self._gen_ir_type(subt))
        result['data'] = subtypes
      elif _is_class(child, 'Any'):
        result['kind'] = child.GetClass()
        result['name'] = child.GetClass()
      elif 'Type' in child.GetClass():
        # PrimitiveType | StringType | Typeref
        result['kind'] = child.GetClass()
        result['name'] = child.GetName()
        if 'UNRESTRICTED' in child.GetProperties():
          result['unrestricted'] = child.GetProperties()['UNRESTRICTED']
      # TODO
      # elif _is_class(child, 'ExtAttributes'):

    return result

  def _gen_ir_argument(self, node):
    # print dump_node(node)
    result = _gen_basic_named(node)
    _set_boolean_prop(result, node, 'OPTIONAL', 'optional')
    for child in node.GetChildren():
      if _is_class(child, 'Type'):
        result['type'] = self._gen_ir_type(child)
      elif _is_class(child, 'Default'):
        value = child.GetName()
        if value is None:
          value = child.GetProperty('VALUE')
        _set_prop_to_dict(result, 'default',
                          _value_to_str(value, child.GetProperty('TYPE')))
      elif _is_class(child, 'Argument'):
        _set_boolean_prop(result, child, 'ELLIPSIS', 'ellipsis')
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_clamp,
                          _hd_extattr_treatnull])
    return result

  def _gen_ir_arguments(self, node):
    result = []
    for child in node.GetChildren():
      if _is_class(child, 'Argument'):
        result.append(self._gen_ir_argument(child))
    return result

  def _gen_ir_callback(self, node):
    result = self._gen_ir_operation(node)
    _set_prop_to_dict(result, 'file_path', self.file_path)
    self.callbacks[node.GetName()] = result
    return result

  def _gen_ir_const(self, node):
    result = _gen_basic_named(node)
    result['type'] = self._gen_ir_type(node)
    for child in node.GetChildren():
      if _is_class(child, 'Value'):
        _set_value_prop(result, child, 'NAME', 'value')
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags,
                          _hd_extattr_unimplemented,
                          _hd_extattr_rename])
    return result

  def _gen_ir_attribute(self, node):
    # print dump_node(node)
    result = _gen_basic_named(node)
    _set_boolean_prop(result, node, 'INHERIT', 'inherit')
    has_setter = False if node.GetProperty('READONLY') else True
    type_ir = None

    for child in node.GetChildren():
      if _is_class(child, 'Type'):
        type_ir = self._gen_ir_type(child)
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags,
                          _hd_extattr_unimplemented,
                          _hd_extattr_treatnull,
                          _hd_extattr_object_opt,
                          _hd_extattr_cereactions,
                          _hd_extattr_rename,
                          _hd_extattr_unforgeable,
                          _hd_extattr_custom,
                          _hd_extattr_raise_expection,
                          _hd_extattr_putforward])
    excp = result.pop('raises_exception', None)
    getter_excp = True if (excp is True or excp and 'Getter' in excp) else None
    setter_excp = True if (excp is True or excp and 'Setter' in excp) else None
    custom = result.pop('custom', None)
    getter_custom = True if (custom is True or custom and 'Getter' in custom) else None
    setter_custom = True if (custom is True or custom and 'Setter' in custom) else None
    # genarate getter ir
    getter = {}
    CHECK_NOT_NONE(type_ir)
    _set_prop_to_dict(getter, 'kind', 'Operation')
    _set_prop_to_dict(getter, 'name', '')
    _set_prop_to_dict(getter, 'arguments', [])
    _set_prop_to_dict(getter, 'raises_exception', getter_excp)
    _set_prop_to_dict(getter, 'custom', getter_custom)
    _set_prop_to_dict(type_ir, 'object_option', result.pop('object_option', None))
    _set_prop_to_dict(getter, 'return', type_ir)
    _set_prop_to_dict(result, 'getter', getter)

    # genarate setter ir
    if has_setter:
      setter = {}
      arg_ir = {}
      _set_prop_to_dict(arg_ir, 'name', 'value')
      _set_prop_to_dict(arg_ir, 'type', dict(type_ir))
      _set_prop_to_dict(arg_ir, 'treat_null_as', result.pop('treat_null_as', None))
      _set_prop_to_dict(setter, 'kind', 'Operation')
      _set_prop_to_dict(setter, 'name', '')
      _set_prop_to_dict(setter, 'return', {'kind': 'PrimitiveType', 'name': 'void'})
      _set_prop_to_dict(setter, 'arguments', [arg_ir])
      _set_prop_to_dict(setter, 'raises_exception', setter_excp)
      _set_prop_to_dict(setter, 'custom', setter_custom)
      _set_prop_to_dict(result, 'setter', setter)
    return result

  def _gen_ir_operation(self, node):
    result = _gen_basic_named(node)
    _set_boolean_prop(result, node, 'STATIC', 'static')
    _set_boolean_prop(result, node, 'GETTER', 'is_item_getter')
    return_ir = None
    args_ir = None
    for child in node.GetChildren():
      if _is_class(child, 'Arguments'):
        args_ir = self._gen_ir_arguments(child)
      elif _is_class(child, 'Type'):
        return_ir = self._gen_ir_type(child)
      elif _is_class(child, 'ExtAttributes'):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags,
                          _hd_extattr_unimplemented,
                          _hd_extattr_object_opt,
                          _hd_extattr_cereactions,
                          _hd_extattr_rename,
                          _hd_extattr_unforgeable,
                          _hd_extattr_notenumerable,
                          _hd_extattr_custom,
                          _hd_extattr_raise_expection,
                          _hd_extattr_force_deny_strict,
                          _hd_extattr_callwith])

    if return_ir is not None:
      _set_prop_to_dict(return_ir, 'object_option', result.pop('object_option', None))
      result['return'] = return_ir

    result['arguments'] = args_ir if args_ir else []
    min_passing_count = None;
    min_passed_count = None;
    if not args_ir is None:
      min_passing_count = 0;
      min_passed_count = 0
      for arg in args_ir:
        if (not arg.get('optional')):
          min_passing_count += 1
          min_passed_count += 1
        elif arg.get('default'):
          min_passing_count += 1
    _set_prop_to_dict(result, 'min_passing_count', min_passing_count)
    _set_prop_to_dict(result, 'min_passed_count', min_passed_count)
    return result

  def _gen_ir_tostring(self, forward=None):
    result = {}
    _set_prop_to_dict(result, 'kind', 'Operation')
    _set_prop_to_dict(result, 'arguments', [])
    _set_prop_to_dict(result, 'min_passed_count', 0)
    _set_prop_to_dict(result, 'min_passing_count', 0)
    _set_prop_to_dict(result, 'return', {'name':'DOMString', 'kind':'StringType'})
    _set_prop_to_dict(result, 'name', 'toString')
    _set_prop_to_dict(result, 'kind', 'Stringifier')
    _set_prop_to_dict(result, 'forward', forward)
    return result

  def _gen_ir_stringifier(self, node):
    forward = None
    stringifier = None
    for child in node.GetChildren():
      if _is_class(child, 'Attribute'):
        forward = self._gen_ir_attribute(child)
        _set_prop_to_dict(forward, 'stringifier', True)
      elif _is_class(child, 'Operation'):
        forward = self._gen_ir_operation(child)
        _set_prop_to_dict(forward, 'stringifier', True)
    stringifier = self._gen_ir_tostring(forward)
    if forward is not None:
      for child in node.GetChildren():
        if _is_class(child, 'ExtAttributes'):
          _handle_extattrs(forward,
                           child.GetChildren(),
                           [_hd_extattr_flags,
                            _hd_extattr_unimplemented,
                            _hd_extattr_treatnull,
                            _hd_extattr_object_opt,
                            _hd_extattr_cereactions,
                            _hd_extattr_rename,
                            _hd_extattr_unforgeable,
                            _hd_extattr_notenumerable,
                            _hd_extattr_custom,
                            _hd_extattr_raise_expection,
                            _hd_extattr_force_deny_strict,
                            _hd_extattr_callwith])
      _set_prop_to_dict(stringifier, 'flags', forward.get('flags', None))
      _set_prop_to_dict(stringifier, 'unimplemented', forward.get('unimplemented', None))
    else:
      for child in node.GetChildren():
        if _is_class(child, 'ExtAttributes'):
          _handle_extattrs(stringifier,
                           child.GetChildren(),
                           [_hd_extattr_flags,
                            _hd_extattr_unimplemented])
    return stringifier, forward

  def _append_to_functions(self, obj, fns):
    if obj.get('unimplemented', False):
      fns.append(obj)
      return
    index = None
    for idx, fn in enumerate(fns):
      if obj.get('name') == fn.get('name') and\
         not fn.get('unimplemented', False):
        index = idx
        break
    if index is not None:
      if fns[index].get('kind') == 'Operation':
        newFn = {}
        fns[index]['id'] = 1
        obj['id'] = 2
        _set_prop_to_dict(newFn, 'kind', 'MultiOperation')
        _set_prop_to_dict(newFn, 'name', fns[index].get('name'))
        _set_prop_to_dict(newFn, 'operations', [fns[index], obj])
        fns[index] = newFn
      elif fns[index].get('kind') == 'MultiOperation':
        obj['id'] = len(fns[index].get('operations')) + 1
        fns[index].get('operations').append(obj)
    else:
      fns.append(obj)

  def _gen_ir_interface(self, node):
    # print dump_node(node)
    result = _gen_basic_named(node)
    _set_prop_to_dict(result, 'no_interface', False)
    constants = []
    attributes = []
    functions = []
    item_getters = []
    for child in node.GetChildren():
      if _is_class(child, _C_CONST):
        constants.append(self._gen_ir_node(child))
      elif _is_class(child, _C_ATTRIBUTE):
        attributes.append(self._gen_ir_node(child))
      elif _is_class(child, _C_OPERATION):
        op_ir = self._gen_ir_node(child)
        if op_ir.get('is_item_getter') is not None:
          if op_ir.get('enumerable') is None:
            op_ir['enumerable'] = True
          item_getters.append(op_ir)
        self._append_to_functions(op_ir, functions)
      elif _is_class(child, 'Stringifier'):
        strgf, forward = self._gen_ir_stringifier(child)
        functions.append(strgf)
        if forward is None:
          continue
        elif forward['kind'] == 'Attribute':
          attributes.append(forward)
        elif forward['kind'] == 'Operation':
          # TODO Use _append_to_functions if need
          functions.append(forward)
      elif _is_class(child, 'Serializer'):
        _set_prop_to_dict(result, 'serializer', self._gen_ir_node(child))
      elif _is_class(child, 'Iterable'):
        _set_prop_to_dict(result, 'iterable', self._gen_ir_node(child.GetChildren()[0]))
      elif _is_class(child, _C_INHERIT):
        _set_prop_to_dict(result, 'parent', child.GetName())
      elif _is_class(child, _C_EXTATTRIBUTES):
        _handle_extattrs(result,
                         child.GetChildren(),
                         [_hd_extattr_flags,
                          self._hd_extattr_constructor,
                          _hd_extattr_unimplemented,
                          _hd_extattr_no_interfaceobj,
                          _hd_extattr_primary_global])

    call_with = result.pop('_call_with', None)
    constructor = result.get('constructor')
    if constructor:
      _set_prop_to_dict(constructor, 'call_with', call_with)
    _set_prop_to_dict(result, 'constants', constants)
    _set_prop_to_dict(result, 'attributes', attributes)
    _set_prop_to_dict(result, 'functions', functions)
    if len(item_getters) > 0:
      _set_prop_to_dict(result, 'item_getters', item_getters)
    _set_prop_to_dict(result, 'file_path', self.file_path)
    self.interfaces[node.GetName()] = result
    return result

  def _gen_ir_implements(self, node):
    # print dump_node(node)
    result = _gen_basic_named(node)
    _set_prop_to_dict(result, 'refer_name', node.GetProperty('REFERENCE'))
    self.implements.append(result)
    return result

  def _gen_ir_node(self, node):
    # print dump_node(node)
    class_name = node.GetClass().lower()
    func_name = '_gen_ir_' + class_name
    try:
      return getattr(self, func_name)(node)
    except AttributeError:
      return self._gen_ir_not_implemented(node)

  def gen_ir(self, top_nodes):
    result = {}
    for node in top_nodes.GetChildren():
      self._gen_ir_node(node)
    # Handle implements
    for impl in self.implements:
      name = impl['name']
      if name in self.interfaces:
        if self.interfaces[name].get('implements', None) is None:
          self.interfaces[name]['implements'] = []
        self.interfaces[name]['implements'].append(impl['refer_name'])

    for kind in NODE_KIND:
      propname = _propname_from_kind(kind)
      _set_prop_to_dict(result, propname, getattr(self, propname))
    return result

  def __init__(self, file_path):
    self.file_path = os.path.splitext(file_path)[0]
    self.interfaces = {}
    self.dictionaries = {}
    self.implements = []
    self.enums = {}
    self.typedefs = {}
    self.callbacks = {}
    self.errors = []

##########################################################

def merge_irs(from_ir, to_ir):
  for key in NODE_KIND:
    name = _propname_from_kind(key)
    if from_ir.get(name, False):
      from_ir[name].update(to_ir.get(name, {}))
    elif to_ir.get(name, False):
      from_ir[name] = {}
      from_ir[name].update(to_ir.get(name))

def apply_types(type_ir, to_ir):
  handler = StarfishIRHandler()
  handler.apply_types(type_ir, to_ir)

def gen_ir_from_file(file_path, debug=False):
  # TODO Use singleton lexer, parser
  lexer = StarfishIDLLexer(debug=debug)
  parser = StarfishIDLParser(lexer, debug=debug)
  reader = StarfishIDLReader(file_path)
  top_nodes = parser.parse_file(file_path)
  result = reader.gen_ir(top_nodes)
  apply_types(result, result)
  return result

##########################################################
if __name__ == '__main__':
  import argparse
  from pprint import pprint
  argparser = argparse.ArgumentParser()
  argparser.add_argument("file_path")
  args = argparser.parse_args()

  result = gen_ir_from_file(args.file_path, debug=True)
  print "\n[IR RESULT]================="
  pprint(result, indent='2')
