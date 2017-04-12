#!/usr/bin/env python
import os.path
import sys
import types
from functools import partial

from starfish_idl_lexer import StarfishIDLLexer
from starfish_idl_parser import StarfishIDLParser

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
def __dump_node(node):
  result = '[%s]' % node.GetClass()
  for key, value in node.GetProperties().iteritems():
    if not key in __SKIP_DUMP_NAMES:
      result += __indent(__pre_br('%s: %s' % (key, value)))
  children = node.GetChildren()
  if len(children) > 0:
    result += __indent(__pre_br('CHILDREN:'))
    subresult = ''
    for child_node in children:
      subresult += __pre_br(__dump_node(child_node))
    result += __indent(__indent(subresult))
  return result

##########################################################
_found_custom_in_interface = False

def _is_class(node, class_name):
  return node.GetClass() == class_name

def _value_to_str(value, type):
  if 'String' in type:
    return '\"' + str(value) + '\"'
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

def _hd_extattr_constructor(target, extattr):
  global _found_custom_in_interface
  if extattr.GetName() == 'NamedConstructor':
    # NamedConstructor
    for child in extattr.GetChildren():
      if _is_class(child, 'Call'):
        constructor = _gen_ir_Operation(child)
        constructor.pop('kind', None)
        target['named_constructor'] = constructor
        return True
  elif extattr.GetName() == 'UnimplementedConstructor':
    target['_unimple_cst'] = True
    return True
  elif 'Constructor' in extattr.GetName():
    # Constructor
    constructor = _gen_ir_Operation(extattr)
    constructor.pop('kind', None)
    _set_prop_to_dict(constructor, 'name', '')
    _set_prop_to_dict(target, 'constructor', constructor)
    if 'Custom' in extattr.GetName():
      _found_custom_in_interface = True
      _set_prop_to_dict(constructor, 'custom', True)
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

def _hd_extattr_htmlconstructor(target, extattr):
  if extattr.GetName() == 'HTMLConstructor':
    target['HTMLConstructor'] = True
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
    target['global_expose'] = False
    return True
  return False

def _hd_extattr_custom(target, extattr):
  global _found_custom_in_interface
  if extattr.GetName() == 'Custom':
    target['custom'] = True
    _found_custom_in_interface = True
    return True
  return False

def _hd_extattr_custom_getter_setter(target, extattr):
  global _found_custom_in_interface
  if extattr.GetName() == 'CustomGetter':
    target['custom_getter'] = True
    _found_custom_in_interface = True
    return True
  elif extattr.GetName() == 'CustomSetter':
    target['custom_setter'] = True
    _found_custom_in_interface = True
    return True
  return False

def _hd_extattr_raise_expection(target, extattr):
  if extattr.GetName() == 'RaisesException':
    target['raises_exception'] = True
    return True
  return False

def _gen_basic_named(node):
  return {
    'name': node.GetName(),
    'kind': node.GetClass(),
  }

##########################################################

def _gen_ir_not_implemented(node):
  return { 'kind': 'NOT_IMPLEMENTED' }

def _gen_ir_Typedef(node):
  result = _gen_basic_named(node)
  for child in node.GetChildren():
    if _is_class(child, 'Type'):
      result['from'] = _gen_ir_Type(child)
    elif _is_class(child, 'ExtAttributes'):
      _handle_extattrs(result,
                       child.GetChildren(),
                       [_hd_extattr_flags])
  return result

def _gen_ir_Enum(node):
  result = _gen_basic_named(node)
  items = []
  for child in node.GetChildren():
    if _is_class(child, 'EnumItem'):
      items.append(child.GetName())
    elif _is_class(child, 'ExtAttributes'):
      _handle_extattrs(result,
                       child.GetChildren(),
                       [_hd_extattr_flags])
  result['items'] = items
  return result

def _gen_ir_Type(node):
  # print __dump_node(node)
  result = {}
  _set_boolean_prop(result, node, 'NULLABLE', 'nullable')
  for child in node.GetChildren():
    if _is_class(child, 'UnionType') or\
       _is_class(child, 'Sequence') or\
       _is_class(child, 'Promise'):
      # Union | Sequence
      result['kind'] = child.GetClass()
      result['name'] = child.GetClass()
      subtypes = []
      for subt in child.GetChildren():
        subtypes.append(_gen_ir_Type(subt))
      result['subtypes'] = subtypes
    elif _is_class(child, 'Any'):
      result['kind'] = child.GetClass()
      result['name'] = child.GetClass()
    elif 'Type' in child.GetClass():
      # PrimitiveType | StringType | Typeref
      result['kind'] = child.GetClass()
      result['name'] = child.GetName()
    # TODO
    # elif _is_class(child, 'ExtAttributes'):

  return result

def _gen_ir_Argument(node):
  # print __dump_node(node)
  result = _gen_basic_named(node)
  _set_boolean_prop(result, node, 'OPTIONAL', 'optional')
  for child in node.GetChildren():
    if _is_class(child, 'Type'):
      result['type'] = _gen_ir_Type(child)
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

def _gen_ir_Arguments(node):
  result = []
  for child in node.GetChildren():
    if _is_class(child, 'Argument'):
      result.append(_gen_ir_Argument(child))
  return result

def _gen_ir_Callback(node):
  return _gen_ir_Operation(node)

def _gen_ir_Const(node):
  result = _gen_basic_named(node)
  result['type'] = _gen_ir_Type(node)
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

def _gen_ir_Attribute(node):
  result = _gen_basic_named(node)
  _set_boolean_prop(result, node, 'INHERIT', 'inherit')
  has_setter = False if node.GetProperty('READONLY') else True
  type_ir = None

  for child in node.GetChildren():
    if _is_class(child, 'Type'):
      type_ir = _gen_ir_Type(child)
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
                        _hd_extattr_custom_getter_setter])

  # genarate getter ir
  getter = {}
  CHECK_NOT_NONE(type_ir)
  _set_prop_to_dict(getter, 'kind', 'Operation')
  _set_prop_to_dict(getter, 'name', '')
  _set_prop_to_dict(getter, 'arguments', [])
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
    _set_prop_to_dict(setter, 'return', {'kind': 'PrimitiveType', 'name': 'void'})
    _set_prop_to_dict(setter, 'arguments', [arg_ir])
    _set_prop_to_dict(result, 'setter', setter)
  return result

def _gen_ir_Operation(node):
  result = _gen_basic_named(node)
  _set_boolean_prop(result, node, 'STATIC', 'static')
  _set_boolean_prop(result, node, 'GETTER', 'is_item_getter')
  return_ir = None
  args_ir = None
  for child in node.GetChildren():
    if _is_class(child, 'Arguments'):
      args_ir = _gen_ir_Arguments(child)
    elif _is_class(child, 'Type'):
      return_ir = _gen_ir_Type(child)
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
                        _hd_extattr_raise_expection])

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

def _gen_ir_Stringifier(node):
  result = {}
  for child in node.GetChildren():
    if _is_class(child, 'Attribute'):
      result = _gen_ir_Attribute(child)
    elif _is_class(child, 'Operation'):
      result = _gen_ir_Operation(child)
    elif _is_class(child, 'ExtAttributes'):
      # Valid when it's placed last
      _handle_extattrs(result,
                       child.GetChildren(),
                       [_hd_extattr_flags,
                        _hd_extattr_unimplemented])

  CHECK_NOT_NONE(result)
  _set_prop_to_dict(result, 'stringifier', True)
  return result

def _gen_ir_Interface(node):
  # print __dump_node(node)
  global _found_custom_in_interface
  _found_custom_in_interface = None
  result = _gen_basic_named(node)
  _set_prop_to_dict(result, 'global_expose', True)
  constants = []
  attributes = []
  functions = []
  item_getters = []
  for child in node.GetChildren():
    if _is_class(child, _C_CONST):
      constants.append(gen_ir(child))
    elif _is_class(child, _C_ATTRIBUTE):
      attributes.append(gen_ir(child))
    elif _is_class(child, _C_OPERATION):
      op_ir = gen_ir(child)
      if op_ir.get('is_item_getter') is not None:
        if op_ir.get('enumerable') is None:
          op_ir['enumerable'] = True
        item_getters.append(op_ir)
      functions.append(op_ir)
    elif _is_class(child, 'Stringifier'):
      child_ir = _gen_ir_Stringifier(child)
      if child_ir['kind'] == 'Attribute':
        attributes.append(child_ir)
      elif child_ir['kind'] == 'Operation':
        functions.append(child_ir)
    elif _is_class(child, 'Serializer'):
      _set_prop_to_dict(result, 'serializer', gen_ir(child))
    elif _is_class(child, 'Iterable'):
      _set_prop_to_dict(result, 'iterable', gen_ir(child.GetChildren()[0]))
    elif _is_class(child, _C_INHERIT):
      _set_prop_to_dict(result, 'parent', child.GetName())
    elif _is_class(child, _C_EXTATTRIBUTES):
      _handle_extattrs(result,
                       child.GetChildren(),
                       [_hd_extattr_flags,
                        _hd_extattr_constructor,
                        _hd_extattr_unimplemented,
                        _hd_extattr_htmlconstructor,
                        _hd_extattr_no_interfaceobj])

  unimple_cst = result.pop('_unimple_cst', None)
  constructor = result.get('constructor')
  if constructor:
    _set_prop_to_dict(constructor, 'unimplemented', unimple_cst)
  _set_prop_to_dict(result, 'constants', constants)
  _set_prop_to_dict(result, 'attributes', attributes)
  _set_prop_to_dict(result, 'functions', functions)
  _set_prop_to_dict(result, 'has_custom', _found_custom_in_interface)
  if len(item_getters) > 0:
    _set_prop_to_dict(result, 'item_getters', item_getters)
  return result

##########################################################
def gen_ir(node):
  # print __dump_node(node)
  class_name = node.GetClass()
  func_name = '_gen_ir_' + class_name
  try:
    return globals()[func_name](node)
  except KeyError:
    return _gen_ir_not_implemented(node)

def gen_ir_from_file(file_path, debug=False):
  # TODO Use singleton lexer, parser
  lexer = StarfishIDLLexer(debug=debug)
  parser = StarfishIDLParser(lexer, debug=debug)
  nodes = parser.parse_file(file_path)
  result = []
  for top_node in nodes.GetChildren():
    result.append(gen_ir(top_node))
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
  for item in result:
    pprint(item, indent='2')
