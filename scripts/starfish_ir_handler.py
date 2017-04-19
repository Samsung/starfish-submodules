#!/usr/bin/env python
import sys

def _extend_property_array(target, refer, propname):
  target_array = target.get(propname)
  ref_array = refer.get(propname)
  for item in ref_array:
    target_array.append(item)

def _append_if_new(to_array, item):
  if not (item in to_array):
    to_array.append(item)

class StarfishIRHandler():
  def _change_types(self, parent, type_key, unimpl):
    type_ir = parent[type_key]
    name = type_ir.get('name')
    kind = type_ir.get('kind')
    if kind == 'Typeref':
      if name in self.typedefs:
        parent[type_key] = self.typedefs[name].get('from')
        return
      if name in self.dictionaries:
        type_ir['kind'] = 'Dictionary'
        type_ir['data'] = self.dictionaries[name]
        if not unimpl:
          _append_if_new(self.used_dictionary, self.dictionaries[name])
        return
      if name in self.callbacks:
        type_ir['kind'] = 'Callback'
        type_ir['data'] = self.callbacks[name]
        return
      if name in self.enums:
        parent[type_key] = self.enums[name]
        return
      if not unimpl:
        _append_if_new(self.used_typeref, name)
    elif kind == 'UnionType':
      for idx, subtype_ir in enumerate(type_ir.get('data', [])):
        self._change_types(type_ir.get('data'), idx, unimpl)
    elif kind in ['Sequence', 'Promise']:
      self._change_types(type_ir, 'data', unimpl)

  def _check_attr(self, attr):
    unimpl = attr.get('unimplemented', False)
    self._change_types(attr.get('getter'), 'return', unimpl)
    if attr.get('setter'):
      self._change_types(attr.get('setter').get('arguments')[0], 'type', unimpl)

  def _check_operation(self, op):
    unimpl = op.get('unimplemented', False)
    for arg in op.get('arguments', []):
      self._change_types(arg, 'type', unimpl)
    self._change_types(op, 'return', unimpl)

  def _check_multioperation(self, op):
    unimplemented_count = 0
    for subop in op.get('operations', []):
      if subop.get('unimplemented'):
        unimplemented_count += 1
      self._check_operation(subop)
    if unimplemented_count == len(op.get('operations')):
      op['unimplemented'] = True

  def _check_constructor(self, constructor):
    unimpl = constructor.get('unimplemented', False)
    for arg in constructor.get('arguments', []):
      self._change_types(arg, 'type', unimpl)

  def _check_interface(self, interface):
    self.used_dictionary = []
    self.used_typeref = []
    # Check constructor
    constructor = interface.get('constructor', None)
    if constructor:
      self._check_constructor(constructor)
      if not constructor.get('unimplemented', False):
        call_with = constructor.get('call_with', False)
    # Check attr
    for attr in interface.get('attributes'):
      self._check_attr(attr)
    # Check operation
    for fn in interface.get('functions'):
      if fn.get('kind') == 'Operation':
        self._check_operation(fn)
      elif fn.get('kind') == 'MultiOperation':
        self._check_multioperation(fn)
    # Update used dictionary and typeref info
    interface['used_dictionary'] = self._resolve_used_dictionary()
    interface['used_typeref'] = self._resolve_used_typeref(interface.get('name'))
    # Handle implements
    finished = []
    for impl_name in interface.get('implements', []):
      if impl_name in self.interfaces:
        refer = self.interfaces[impl_name]
        _extend_property_array(interface, refer, 'attributes')
        _extend_property_array(interface, refer, 'functions')
        _extend_property_array(interface, refer, 'constants')
        finished.append(impl_name)
    for impl_name in finished:
      interface.get('implements').remove(impl_name)

  def _check_typedef(self, typedef):
    self._change_types(typedef, 'from', False)

  def _handle_dictionary_parent(self, dictionary):
    parent = dictionary.get('parent', False)
    resolved_parent = True
    parent_ir = None
    if parent:
      if parent in self.dictionaries:
        parent_ir = self.dictionaries[parent]
        resolved_parent = self._handle_dictionary_parent(parent_ir)
      else:
        resolved_parent = False
        
    if resolved_parent and parent_ir is not None:
      dictionary['members'] = dictionary['members'] + parent_ir['members']
    return resolved_parent

  def _check_dictionary(self, dictionary):
    # Handle parent
    if self._handle_dictionary_parent(dictionary):
      dictionary.pop('parent', None)
    self.used_dictionary = []
    self.used_typeref = []
    unimpl = dictionary.get('unimplemented', False)
    for key in dictionary.get('members', []):
      self._change_types(key, 'type', unimpl)
    # Update used dictionary and typeref info
    dictionary['used_dictionary'] = self._resolve_used_dictionary(dictionary)
    dictionary['used_typeref'] = self._resolve_used_typeref()

  def _resolve_used_dictionary(self, except_dict=None):
    result = []
    for dict in self.used_dictionary:
      if except_dict is dict:
        continue
      result.append(dict)
    return result

  def _resolve_used_typeref(self, except_name=None):
    result = []
    for name in self.used_typeref:
      if except_name == name:
        continue
      if name in self.interfaces:
        result.append(self.interfaces[name]['file_path'])
    self.used_typeref = []
    return result

  def apply_types(self, type_ir, to_ir):
    self.dictionaries = {}
    self.interfaces = {}
    self.callbacks = {}
    self.typedefs = {}
    self.enums = {}
    self.interfaces.update(type_ir.get('interfaces', {}))
    self.dictionaries.update(type_ir.get('dictionaries', {}))      
    self.callbacks.update(type_ir.get('callbacks', {}))
    self.typedefs.update(type_ir.get('typedefs', {}))
    self.enums.update(type_ir.get('enums', {}))
    # Check typedefs
    for key, value in to_ir.get('typedefs', {}).iteritems():
      self._check_typedef(value)
    # Check callbacks
    for key, value in to_ir.get('callbacks', {}).iteritems():
      self._check_operation(value)
    # Check dictionaries
    for key, value in to_ir.get('dictionaries', {}).iteritems():
      self._check_dictionary(value)
    # Check interfaces
    for key, value in to_ir.get('interfaces', {}).iteritems():
      self._check_interface(value)
    # cleaning dictionaries
    for key, value in self.dictionaries.iteritems():
      value.pop('_check', None)

  def __init__(self):
    self.used_dictionary = []
    self.used_typeref = []



      