#!/usr/bin/env python

import argparse
import os
import pprint
import json
import sys
from math import log10

from starfish_idl_reader import gen_ir_from_file, merge_irs, apply_types
from starfish_interface_collection import gen_interface_collection

try:
  from jinja2 import Environment, FileSystemLoader
except ImportError:
  print("Error: Jinja2 not found")
  print("Exiting...")
  os._exit(0)

STRICT_MODE = False

IR_EXT = ".txt"
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
TEMPLATES_PATH = os.path.join(SCRIPT_PATH, 'templates')
STARFISH_PATH = os.path.join(SCRIPT_PATH, '..', '..')

NON_NULLABLE_TYPE_KINDS = ['StringType', 'PrimitiveType', 'Dictionary', 'SequenceOf', 'UnionType', 'Enum']
STRING_TYPE_KINDS = ['StringType', 'Enum']
POINTER_TYPE_KINDS = ['Typeref', 'Callback', 'Promise', 'SpecialType']
NUMBER_TYPE_NAMES = ['short', 'long', 'long long', 'float', 'double', 'unsigned long', 'unsigned short', 'unsigned long long']
STRONG_TYPE_KINDS = ['Sequence', 'Dictionary', 'Typeref', 'Promise', 'SpecialType']
HAS_SEQUENCE_TYPE_KINDS = ['MediaTrackConstraints']

_root_dir=None
_out_dir=None

def print_skip_msg(name, reason):
  print("> Skip generating code for '" + name + "': " + reason)

def generate_code(ir, args):
  print("Generating binding code...")
  interfaces = ir['interfaces']

  gen_interface_collection(interfaces, os.path.join(STARFISH_PATH, args.out_path), STRICT_MODE, args.exposed)

  for key in interfaces:
    interface = interfaces[key]
    if interface.get('unimplemented', False):
      continue
    if interface.get('partial_interface', False):
      continue
    template = 'base_module.cpp'
    generate_code_with_template(interface, interface['name'] + 'Binding.cpp', template, args)

  dictionaries = ir['dictionaries']
  for key in dictionaries:
    dictionary = dictionaries[key]
    if dictionary.get('unimplemented', False):
      # print_skip_msg(dictionary.get('name'), "Unimplemented dictionary")
      continue
    generate_code_with_template(dictionary, dictionary['name'] + 'Binding.cpp', 'base_dictionary.cpp', args)

  for key in ir['unions']:
    union = ir['unions'][key]
    if union.get('unimplemented', False):
      continue
    generate_code_with_template(union, union['name'] + 'Union.h', 'base_union.h', args)
    generate_code_with_template(union, union['name'] + 'Binding.cpp', 'base_union.cpp', args)

def generate_code_with_template(ir, out_name, template, args):
  ir['args'] = args

  template = env.get_template(template)

  binding_path = os.path.join(STARFISH_PATH, args.out_path)
  if not os.path.exists(binding_path):
    raise Exception("Out path \"" + binding_path + "\" doesn't exist")

  with open(os.path.join(binding_path, out_name), 'w') as w:
    ret = template.render(**ir)
    w.write(ret)
    print(("> Generated Code \"{}\"".format(out_name)))
    # print(ret)

  if args.log_idl:
    path = ir['name'] + 'Idl' + IR_EXT
    with open(os.path.join(binding_path, path), 'w') as w:
      w.write(pprint.pformat(ir))
      print(("Logged IR to \"{}\"".format(path)))

def prerun_all(dir_path, file_alone=None):
  result = {}
  file_result = None
  for (root, dirs, files) in os.walk(dir_path):
    for f in files:
      if os.path.splitext(f)[-1] != '.idl':
        continue
      file_path = os.path.join(root, f)
      if 'unimpl_' in f:
        if STRICT_MODE:
          ir = gen_ir_from_file(file_path, treat_as_unimpl=True)
          merge_irs(result, ir)
        continue
      ir = gen_ir_from_file(file_path)
      merge_irs(result, ir)
      if file_path == file_alone:
        print(("Generated IR from {}".format(file_path)))
        file_result = ir
  apply_types(result, result)
  # pprint.pprint(result, indent='2')
  if file_result:
    apply_types(file_result, file_result)
    return result, file_result
  else:
    return result

def filter_assert_true(errmsg, v):
  if v:
    print(errmsg)
    sys.exit(1)
  return ''

def filter_assert_false(errmsg, v):
  return filter_assert_true(errmsg, not v)

def filter_to_argument_syntax(prefix, start, end):
  if start == end:
    return ''
  result = prefix + str(start)
  for idx in range(start + 1, end):
    result = result + ', ' + prefix + str(idx)
  return result

def filter_first_word_capitalize(word):
  if not (isinstance(word, str) or isinstance(word, str)):
    return word

  if len(word) == 0:
    return word

  return word[0].upper() + word[1:]

def filter_to_header_path(inputtxt):
  return inputtxt.replace(_root_dir, '') + '.h'

def filter_to_union_header_path(union_name):
  return _out_dir.replace(_root_dir, '') + union_name + 'Union.h'

def filter_digit(num):
  return int(log10(num)) + 1

if __name__ == "__main__":
  argparser = argparse.ArgumentParser()
  argparser.add_argument("root_path", help="root directiory to start")
  argparser.add_argument("out_path", help="out directiory to write file")
  argparser.add_argument("-f", "--file", help="specify an idl file")
  argparser.add_argument("-l", "--log-idl", action='store_true',
                         dest="log_idl", help="flag to log idl")
  argparser.add_argument("--exposed", default="Window", choices=["Window", "Worker"],
                         help="collect interfaces of the exposed module(Window or Worker)")

  args = argparser.parse_args()
  # Argument validation
  if not os.path.isdir(args.root_path):
    print('ERR: Invalid root path \'' + args.root_path + '\'')
    sys.exit(1)
  if args.file is not None and \
     (not os.path.isfile(args.file) or not args.file.endswith('.idl')):
    print('ERR: Invalid file \'' + args.file + '\'')
    sys.exit(1)
  if not os.path.exists(args.out_path):
    os.makedirs(args.out_path)

  env = Environment(loader=FileSystemLoader(TEMPLATES_PATH), trim_blocks=True,
                    lstrip_blocks=True)
  # Set custom filters
  env.filters['assert_true'] = filter_assert_true
  env.filters['assert_false'] = filter_assert_false
  env.filters['to_arg_syntax'] = filter_to_argument_syntax
  env.filters['first_word_capitalize'] = filter_first_word_capitalize
  env.filters['to_h_path'] = filter_to_header_path
  env.filters['to_union_h_path'] = filter_to_union_header_path
  env.filters['digit'] = filter_digit

  # Set globals
  env.globals['non_nullable_type_kinds'] = NON_NULLABLE_TYPE_KINDS
  env.globals['string_type_kinds'] = STRING_TYPE_KINDS
  env.globals['pointer_type_kinds'] = POINTER_TYPE_KINDS
  env.globals['strong_type_kinds'] = STRONG_TYPE_KINDS
  env.globals['number_type_names'] = NUMBER_TYPE_NAMES
  env.globals['strict_mode'] = STRICT_MODE
  env.globals['has_sequence_type_kinds']= HAS_SEQUENCE_TYPE_KINDS

  # with open(MODULES_FILE, 'r') as r:
  #  sf_modules = json.loads(r.read())
  _root_dir = args.root_path
  if not _root_dir.endswith('/'):
    _root_dir = _root_dir + '/'

  _out_dir = args.out_path
  if not _out_dir.endswith('/'):
    _out_dir = _out_dir + '/'

  if args.file is not None:
    all_irs, file_ir = prerun_all(args.root_path, args.file)
    generate_code(file_ir, args)
  else:
    all_irs = prerun_all(args.root_path)
    generate_code(all_irs, args)
