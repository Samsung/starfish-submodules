#!/usr/bin/env python
import sys
import types

def _extend_property_array(target, refer, propname):
  target_array = target.get(propname)
  ref_array = refer.get(propname)
  for item in ref_array:
    if not item in target_array:
      target_array.append(item)

class StarfishIRHandler():
  def _add_used_typeref(self, name):
    if self.processing and self.processing['name'] == name:
      return
    if name in self.interfaces:
      self.include_paths.add(self.interfaces[name]['file_path'])

  def _add_used_dictionary(self, dictionary):
    if self.processing and self.processing['name'] == dictionary['name']:
      return
    if dictionary.get('unimplemented', False):
      return
    if dictionary in self.used_dictionaries:
      return
    self.used_dictionaries.append(dictionary)
    self.include_paths.add(dictionary['file_path'])

  def _change_types(self, parent, type_key, unimpl):
    type_ir = parent[type_key]
    name = type_ir.get('name')
    kind = type_ir.get('kind')
    if kind == 'Typeref':
      if name in self.typedefs:
        parent[type_key] = self.typedefs[name].get('from')
        return
      if name in self.dictionaries:
        dictionary = self.dictionaries[name]
        type_ir['kind'] = 'Dictionary'
        type_ir['data'] = dictionary
        if not unimpl:
          self._add_used_dictionary(dictionary)
        return
      if name in self.callbacks:
        type_ir['kind'] = 'Callback'
        type_ir['data'] = self.callbacks[name]
        return
      if name in self.enums:
        parent[type_key] = self.enums[name]
        return
      if not unimpl:
        self._add_used_typeref(name)
    elif kind == 'UnionType':
      for idx, subtype_ir in enumerate(type_ir.get('data', [])):
        self._change_types(type_ir.get('data'), idx, unimpl)
    elif kind in ['Sequence', 'Promise']:
      self._change_types(type_ir, 'data', unimpl)

  def _udpate_has_exception(self, op):
    if op.get('raises_exception'):
       self.has_exception = True
    return

  def _check_attr(self, attr):
    unimpl = attr.get('unimplemented', False)
    self._change_types(attr.get('getter'), 'return', unimpl)
    self._udpate_has_exception(attr['getter'])
    if attr.get('setter'):
      self._change_types(attr.get('setter').get('arguments')[0], 'type', unimpl)
      self._udpate_has_exception(attr['setter'])
    if attr.get('put_forwards', False) and \
       type(attr['put_forwards']) is types.StringType:
      ref_name = attr['getter']['return']['name']
      if ref_name in self.interfaces:
        ref_interface = self.interfaces[ref_name]
        forward_name = attr['put_forwards']
        for ref_attr in ref_interface['attributes']:
          if ref_attr['name'] == forward_name:
            attr['put_forwards'] = ref_attr
            if ref_attr.get('setter'):
              self._udpate_has_exception(ref_attr['setter'])
            break;

  def _check_operation(self, op):
    if not op:
      return
    unimpl = op.get('unimplemented', False)
    self._udpate_has_exception(op)
    for arg in op.get('arguments', []):
      self._change_types(arg, 'type', unimpl)
    self._change_types(op, 'return', unimpl)

  def _check_multioperation(self, op):
    operations = op.get('operations', [])
    def sort_op(item):
      return item['min_passed_count']

    for subop in operations:
      self._check_operation(subop)
      conditions = []
      for key, arg in enumerate(subop.get('arguments', [])):
        if arg.get('optional') or arg.get('ellipsis'):
          continue
        if arg['type'].get('nullable', False):
          continue
        # TODO May add Dictionary here
        if arg['type']['kind'] in ['Typeref']:
          conditions.append(key)
      subop['conditions'] = conditions

    # Sort operations by max 'min_passed_count' order
    op['operations'] = sorted(operations, key=sort_op, reverse=True)

  def _check_constructor(self, constructor):
    unimpl = constructor.get('unimplemented', False)
    for arg in constructor.get('arguments', []):
      self._change_types(arg, 'type', unimpl)

  def _check_interface(self, interface):
    self._init_using_info(interface)
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
    descriptor = interface.get('descriptor')
    if descriptor and not descriptor.get('custom'):
      self._check_operation(descriptor.get('indexed_getter'))
      self._check_operation(descriptor.get('named_getter'))
      self._check_operation(descriptor.get('setter'))

    # Update used dictionary and typeref info
    self._flush_using_info(interface)

  def _check_implement(self, interface):
    # Handle implements
    finished = []
    for impl_name in interface.get('implements', []):
      if impl_name in self.interfaces:
        refer = self.interfaces[impl_name]
        _extend_property_array(interface, refer, 'attributes')
        _extend_property_array(interface, refer, 'functions')
        _extend_property_array(interface, refer, 'constants')
        _extend_property_array(interface, refer, 'used_dictionaries')
        interface['include_paths'] |= refer['include_paths']
        finished.append(impl_name)
    for impl_name in finished:
      interface.get('implements').remove(impl_name)

  def _check_typedef(self, typedef):
    self._change_types(typedef, 'from', False)

  def _check_dictionary(self, dictionary):
    self._init_using_info(dictionary)
    # Handle parent
    self._handle_dictionary_parent(dictionary)
    unimpl = dictionary.get('unimplemented', False)
    for key in dictionary.get('members', []):
      self._change_types(key, 'type', unimpl)
    # Update used dictionary and typeref info
    self._flush_using_info(dictionary)

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
      dictionary.pop('parent', None)
    return resolved_parent

  def _init_using_info(self, from_obj):
    self.processing = from_obj
    if from_obj.get('used_dictionaries', None) is None:
      from_obj['used_dictionaries'] = []
    if from_obj.get('include_paths', None) is None:
      from_obj['include_paths'] = set()
    self.used_dictionaries = from_obj['used_dictionaries']
    self.include_paths = from_obj['include_paths']
    self.has_exception = False

  def _flush_using_info(self, to_obj):
    if self.has_exception:
      self.include_paths.add('core/dom/DOMException')
    self.include_paths.discard(self.processing['file_path'])
    self.used_dictionaries = []
    self.include_paths = set()
    self.processing = None

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
    # Check implementes in interfaces
    for key, value in to_ir.get('interfaces', {}).iteritems():
      self._check_implement(value)

  def __init__(self):
    self.processing = None
    self.used_dictionaries = []
    self.include_paths = set()
    self.has_exception = False



