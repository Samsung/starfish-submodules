#!/usr/bin/env python
import types

def set_prop_to_dict(target, prop_name, v):
  if type(target) is not dict:
    return
  if v is not None:
    target[prop_name] = v

def merge_extended_attrs(attrs1, attrs2):
  print(merge_extended_attrs)

def _same_types(type_a, type_b):
  if type_a['name'] != type_b['name']:
    return False
  if type_a['kind'] != type_b['kind']:
    return False
  kind = type_a['kind']
  if kind == 'Enum':
    for idx, item_a in enumerate(type_a['data']):
      if item_a != type_b['data'][idx]:
        return False
  elif kind == 'Promise' or kind == 'Sequence':
    return _same_types(type_a['data'], type_b['data'])
  elif kind == 'Union':
    for idx, item_a in enumerate(type_a['data']):
      if not _same_types(item_a, type_b['data'][idx]):
        return False
  return True

def _same_signatures(op_a, op_b):
  if len(op_a['arguments']) != len(op_b['arguments']):
    return False
  for idx, arg_a in enumerate(op_a['arguments']):
    arg_b = op_b['arguments'][idx]
    if not _same_types(arg_a['type'], arg_b['type']):
      return False
  return True

def _override_prop_if_exist(op_from, op_to, name):
  if op_from.get(name):
    op_to[name] = op_from[name]

def _merge_listprop_if_exist(op_from, op_to, name):
  if op_from.get(name):
    op_to[name] |= op_from[name]

def _merge_operations(op_from, op_to):
  _override_prop_if_exist(op_from, op_to, 'raises_exception')
  _override_prop_if_exist(op_from, op_to, 'cereactions')
  _override_prop_if_exist(op_from, op_to, 'unforgeable')
  _override_prop_if_exist(op_from, op_to, 'rename')
  _merge_listprop_if_exist(op_from, op_to, 'flags')

def _merge_operation_and_operation(op, op_to):
  if op.get('unimplemented'):
    return op_to
  if _same_signatures(op, op_to):
    if op_to.get('unimplemented'):
      return op
    _merge_operations(op, op_to)
    return op_to
  op_to['id'] = 1
  op['id'] = 2
  newFn = {}
  set_prop_to_dict(newFn, 'kind', 'MultiOperation')
  set_prop_to_dict(newFn, 'name', op_to['name'])
  set_prop_to_dict(newFn, 'operations', [op_to, op])
  return newFn


def _merge_operation_and_multioperation(op, multi_op):
  if op.get('unimplemented'):
    return multi_op
  for item in multi_op['operations']:
    if _same_signatures(op, item):
      _merge_operations(op, item)
      return multi_op
  op['id'] = len(multi_op['operations']) + 1
  multi_op['operations'].append(op)
  return multi_op

def append_to_functions(op, fns):
  name = op.get('name')
  if name == '_unnamed_':
    return
  unimplemented = op.get('unimplemented', False)
  index = None
  for idx, fn in enumerate(fns):
    if name == fn.get('name') and op.get('static') == fn.get('static'):
      index = idx
      break
  if index is not None:
    if unimplemented:
      return
    if op['kind'] == 'Operation':
      if fns[index]['kind'] == 'Operation':
        fns[index] = _merge_operation_and_operation(op, fns[index])
      elif fns[index]['kind'] == 'MultiOperation':
        fns[index] = _merge_operation_and_multioperation(op, fns[index])
    elif op['kind'] == 'MultiOperation':
      if fns[index]['kind'] == 'Operation':
        fns[index] = _merge_operation_and_multioperation(fns[index], op)
      elif fns[index]['kind'] == 'MultiOperation':
        for item in op['operations']:
          fns[index] = _merge_operation_and_multioperation(item, fns[index])
  else:
    fns.append(op)

# if __name__ == '__main__':
#   abc
