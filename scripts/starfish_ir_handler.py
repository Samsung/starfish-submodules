#!/usr/bin/env python


class StarfishIRHandler():
  def _change_types(self, parent, type_key):
    type_ir = parent[type_key]
    if type_ir.get('kind') == 'Typeref':
      # Deadlock?
      for td in self.typedefs:
        if type_ir.get('name') == td.get('name'):
          parent[type_key] = td.get('from')
          return
      for dict in self.dictionaries:
        if type_ir.get('name') == dict.get('name'):
          type_ir['kind'] = 'Dictionary'
          type_ir['data'] = dict
          dict['_check'] = True
          return
      for cb in self.callbacks:
        if type_ir.get('name') == cb.get('name'):
          type_ir['kind'] = 'Callback'
          type_ir['data'] = cb
          return
      for enum in self.enums:
        if type_ir.get('name') == enum.get('name'):
          parent[type_key] = enum
          return
    elif type_ir.get('kind') in ['UnionType', 'Sequence', 'Promise']:
      for idx, subtype_ir in enumerate(type_ir.get('data', [])):
        self._change_types(type_ir.get('data'), idx)

  def _check_attr(self, attr):
    self._change_types(attr.get('getter'), 'return')
    if attr.get('setter'):
      self._change_types(attr.get('setter').get('arguments')[0], 'type')

  def _check_operation(self, op):
    for arg in op.get('arguments', []):
      self._change_types(arg, 'type')
    self._change_types(op, 'return')

  def _check_multioperation(self, op):
    unimplemented_count = 0
    for subop in op.get('operations', []):
      if subop.get('unimplemented'):
        unimplemented_count += 1
      self._check_operation(subop)
    if unimplemented_count == len(op.get('operations')):
      op['unimplemented'] = True

  def _check_constructor(self, constructor):
    for arg in constructor.get('arguments', []):
      self._change_types(arg, 'type')

  def _check_interface(self, interface):
    # Check const -> skip
    # Check attr
    for attr in interface.get('attributes'):
      self._check_attr(attr)
    # Check operation
    for fn in interface.get('functions'):
      if fn.get('kind') == 'Operation':
        self._check_operation(fn)
      elif fn.get('kind') == 'MultiOperation':
        self._check_multioperation(fn)
    # Check constructor
    constructor = interface.get('constructor', None)
    if constructor:
      self._check_constructor(constructor)
    # Update used dictionary info
    self.used_dictionary = []
    for dictionary in self.dictionaries:
      if dictionary.pop('_check', None):
        self.used_dictionary.append(dictionary)
    if len(self.used_dictionary) > 0:
      interface['used_dictionary'] = self.used_dictionary

  def _check_typedef(self, typedef):
    self._change_types(typedef, 'from')

  def _check_dictionary(self, dictionary):
    for key in dictionary.get('keys', []):
      self._change_types(key, 'type')

  def get_typed_interfaces(self, irs):
    self.dictionaries = []
    self.interfaces = []
    self.callbacks = []
    self.typedefs = []
    self.enums = []
    for ir in irs:
      kind = ir['kind']
      if kind == 'Dictionary':
        self.dictionaries.append(ir)
      elif kind == 'Interface':
        self.interfaces.append(ir)
      elif kind == 'Callback':
        self.callbacks.append(ir)
      elif kind == 'Typedef':
        self.typedefs.append(ir)
      elif kind == 'Enum':
        self.enums.append(ir)
    if len(self.dictionaries) + len(self.callbacks) + \
       len(self.typedefs) + len(self.enums) == 0:
      return
    # Check typedefs
    for typedef in self.typedefs:
      self._check_typedef(typedef)
    # Check callbacks
    for cb in self.callbacks:
      self._check_operation(cb)
    # Check dictionaries
    for dict in self.dictionaries:
      self._check_dictionary(dict)
    # Check interfaces
    for interface in self.interfaces:
      self._check_interface(interface)



      