#!/usr/bin/env python
import sys
import types

class StarfishIRHandler():
  def _add_used_typeref(self, name):
    if self.processing and self.processing['name'] == name:
      return
    if name in self.interfaces and not self.interfaces[name].get('unimplemented'):
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
        typedef = self.typedefs[name]
        self._change_types(typedef, 'from', unimpl)
        from_ir = typedef['from'];
        # To preserve nullability,
        # do not connect reference directly here
        # e.g. parent[type_key] = from_ir
        type_ir['name'] = from_ir['name']
        type_ir['kind'] = from_ir['kind']
        if from_ir.get('data'):
          type_ir['data'] = from_ir['data']
        if from_ir.get('nullable'):
          type_ir['nullable'] = from_ir['nullable']
        if from_ir.get('unrestricted'):
          type_ir['unrestricted'] = from_ir['unrestricted']
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
      union_name = self._create_union_name(type_ir)
      type_ir['name'] = union_name
      self.used_unions.add(union_name)
      if union_name in self.unions:
        type_ir['data'] = self.unions[union_name]['data']
      else:
        self.unions[union_name] = type_ir
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
    if attr.get('reflect') and attr.get('setter'):
      self.has_exception = True
    # Validation
    if attr.get('reflect') and \
      (attr['getter']['return'].get('nullable') or \
       not self.processing.get('_inherited_element') or \
       not (attr['getter']['return']['kind'] == 'StringType' or \
            attr['getter']['return']['name'] == 'boolean')):
      print 'Wrong use of "Reflect" on ' + self.processing['name'] + '.' + attr['name']
      print 'Check below conditions'
      print '> Interface should inherited Element'
      print '> Attribute type should be String or Boolean'
      print '> Attribute type is not nullable'
      sys.exit(1)

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
      condition_c = 0
      if item.get('conditions'):
        condition_c = len(item['conditions'])
      return item['min_passed_count'] * 100 + condition_c

    for subop in operations:
      self._check_operation(subop)
      conditions = []
      strong_condition_count = 0
      for key, arg in enumerate(subop.get('arguments', [])):
        if arg.get('optional') or arg.get('ellipsis'):
          continue
        if arg['type'].get('nullable', False):
          continue
        # TODO May add Dictionary here
        if not arg['type']['kind'] in ['Any', 'UnionType']:
          conditions.append(key)
          if arg['type']['kind'] in ['Sequence', 'Dictionary', 'Typeref', 'Callback', 'Promise']:
            strong_condition_count += 1
      subop['conditions'] = conditions
      subop['strong_condition_count'] = strong_condition_count

    # Sort operations by max 'min_passed_count' order
    op['operations'] = sorted(operations, key=sort_op, reverse=True)

  def _check_constructor(self, constructor):
    self._udpate_has_exception(constructor)
    unimpl = constructor.get('unimplemented', False)
    for arg in constructor.get('arguments', []):
      self._change_types(arg, 'type', unimpl)

  def _check_interface(self, interface):
    self._init_using_info(interface)
    # Resolve parent
    self._handle_interface_parent(interface)
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
      self._check_operation(descriptor.get('indexed_setter'))
      self._check_operation(descriptor.get('named_setter'))
      self._check_operation(descriptor.get('indexed_deleter'))
      self._check_operation(descriptor.get('named_deleter'))

    # Update used dictionary and typeref info
    self._flush_using_info(interface)

  def _handle_interface_parent(self, interface):
    parent = interface.get('parent')
    if parent and type(parent) == types.StringType:
      if parent in self.interfaces:
        parent = self.interfaces[parent]
        if parent.get('static_interface'):
          del parent['static_interface']
        interface['parent'] = parent
        self._handle_interface_parent(parent)
        if parent.get('has_unforgeable'):
          interface['has_unforgeable'] = True
        self._inherit_descriptor(interface, parent)
        if parent['name'] == 'Element' or parent.get('_inherited_element'):
          interface['_inherited_element'] = True
      else:
        print interface.get('name') + ': Wrong parent interface "' + parent + '"'
        sys.exit(1)

  def _inherit_descriptor(self, child, parent):
    p_desc = parent.get('descriptor')
    if not p_desc:
      return
    c_desc = child.get('descriptor')
    if not c_desc:
      c_desc = {}
      child['descriptor'] = c_desc
    def _set_if_necessary(prop_name):
      if p_desc.get(prop_name) and not c_desc.get(prop_name):
        c_desc[prop_name] = p_desc[prop_name]
    for key in ['indexed_getter', 'named_getter', 'setter', 'custom']:
      _set_if_necessary(key)

  def _check_implement(self, interface):
    # Handle implements
    finished = []
    for impl_name in interface.get('implements', []):
      if impl_name in self.interfaces:
        refer = self.interfaces[impl_name]
        # append to target
        copy_list = ['constants', 'attributes', 'functions', 'used_dictionaries']
        for key in copy_list:
          interface[key] += refer[key]
        interface['has_unforgeable'] |= refer['has_unforgeable']
        interface['include_paths'] |= refer['include_paths']
        finished.append(impl_name)
    for impl_name in finished:
      interface.get('implements').remove(impl_name)

  def _check_partial_interface(self, interface):
    if interface.get('partial_interface') and interface.get('_partial_target'):
      # print ">> to " + interface.get('_partial_target') + " from " + interface['name']
      if interface.get('_partial_target') in self.interfaces:
        target = self.interfaces[interface.pop('_partial_target')]
        # validate
        copy_list = ['constants', 'attributes', 'functions', 'used_dictionaries']
        for key in copy_list:
          for prop_a in interface[key]:
            for prop_b in target[key]:
              if prop_a['name'] in prop_b['name']:
                print 'Duplicate ' + key + ": " + prop_a['name']
                print '* ' + target['file_path'] + '.idl'
                print '* ' + interface['file_path'] + '.idl'
                sys.exit(1)
        # append to target
        for key in copy_list:
          target[key] += interface[key]
        target['include_paths'] |= interface['include_paths']
        target['has_unforgeable'] |= interface['has_unforgeable']

  def _check_union(self, union_ir):
    self._init_using_info(union_ir)
    for idx, subtype_ir in enumerate(union_ir.get('data', [])):
      self._change_types(union_ir['data'], idx, False)
      if subtype_ir['name'] in self.interfaces:
        target = self.interfaces[subtype_ir['name']]
        if target.get('flags'):
          if not union_ir.get('flags'):
            union_ir['flags'] = set()
          union_ir['flags'] |= target['flags']
    self._flush_using_info(union_ir)

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
    if from_obj == None:
      return
    self.processing = from_obj
    if from_obj.get('used_dictionaries', None) is None:
      from_obj['used_dictionaries'] = []
    if from_obj.get('used_unions', None) is None:
      from_obj['used_unions'] = set()
    if from_obj.get('include_paths', None) is None:
      from_obj['include_paths'] = set()
    self.used_dictionaries = from_obj['used_dictionaries']
    self.used_unions = from_obj['used_unions']
    self.include_paths = from_obj['include_paths']
    self.has_exception = False

  def _flush_using_info(self, to_obj):
    if to_obj == None:
      return
    if self.has_exception:
      self.include_paths.add('core/dom/DOMException')
    if self.processing.get('file_path'):
      self.include_paths.discard(self.processing['file_path'])
    self.used_dictionaries = []
    self.used_unions = set()
    self.include_paths = set()
    self.processing = None

  def _create_union_name(self, uniontype_ir):
    result = ''
    for subtype_ir in uniontype_ir.get('data', []):
      if subtype_ir.get('kind') == 'UnionType':
        result = result + 'Or' + self._create_union_name(subtype_ir)
      elif not subtype_ir.get('name'):
        print 'Subtype of Union should have type name'
        sys.exit(1)
      else:
        result = result + 'Or' + subtype_ir['name']
    return result[2:]

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
    for key in to_ir.get('typedefs', {}):
      self._change_types(to_ir['typedefs'][key], 'from', False)
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
    # Check extras in interfaces
    for key, value in to_ir.get('interfaces', {}).iteritems():
      self._check_implement(value)
      self._check_partial_interface(value)
    # Check unions
    for key in self.unions:
      self._check_union(self.unions[key])
    to_ir['unions'] = self.unions

  def __init__(self):
    self.processing = None
    self.used_dictionaries = []
    self.used_unions = set()
    self.include_paths = set()
    self.has_exception = False
    self.unions = {}



