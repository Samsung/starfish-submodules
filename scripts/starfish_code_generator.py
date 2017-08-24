#!/usr/bin/env python

import argparse
import os
import pprint
import json
import sys
from math import log10

from jinja2 import Environment, FileSystemLoader
from starfish_idl_reader import gen_ir_from_file, merge_irs, apply_types

STRICT_MODE = False

CPP_EXT = ".cpp"
H_EXT = ".h"
IR_EXT = ".txt"
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
TEMPLATES_PATH = os.path.join(SCRIPT_PATH, 'templates')
STARFISH_PATH = os.path.join(SCRIPT_PATH, '..', '..')

NULLABLE_TYPE_KINDS = ['StringType', 'PrimitiveType', 'Dictionary', 'Sequence']
STRING_KINDS = ['StringType', 'Enum']
POINTER_KINDS = ['Typeref', 'Callback', 'Promise']

_root_dir=None

def print_skip_msg(name, reason):
  print "> Skip generating code for '" + name + "': " + reason

def generate_interface_collection_header(interfaces, args):
  grouped_interfaces = {}
  constructor_nicknames = []
  for key in interfaces:
    interface = interfaces[key]

    if interface.get('partial_interface', False):
      continue
    if interface.get('unimplemented', False):
      continue
    if interface.get('constructor', False) and \
       len(interface['constructor']['name']) > 0:
      constructor_nicknames.append(interface['constructor']['name'])

    # TODO Support multiple flags
    if "flags" in interface:
      flag = interface["flags"][0]
    else:
      flag = "STARFISH_ENABLE_DEFAULT"

    if grouped_interfaces.has_key(flag):
      grouped_interfaces.get(flag).append(interface["name"])
    else:
      grouped_interfaces[flag] = [interface["name"]]

  binding_path = os.path.join(STARFISH_PATH, args.out_path)
  with open(os.path.join(binding_path, "Interfaces.h"), 'w') as w:
    w.write("#ifndef __StarFishInterfaces__\n")
    w.write("#define __StarFishInterfaces__\n")

    for flag in grouped_interfaces:
      if flag != "STARFISH_ENABLE_DEFAULT":
        w.write("\n#ifdef {}".format(flag))
      simple_flag = flag[flag.find("ENABLE_") + 7:]
      w.write("\n#define STARFISH_ENUM_LAZY_BINDING_NAMES_{}(F)".format(simple_flag))
      for name in sorted(grouped_interfaces[flag]):
        w.write(" \\\n    F({})".format(name))
      if flag != "STARFISH_ENABLE_DEFAULT":
        w.write("\n#else")
        w.write("\n#define STARFISH_ENUM_LAZY_BINDING_NAMES_{}(F)".format(simple_flag))
        w.write("\n#endif")
      w.write("\n")
    w.write("\n")

    w.write("\n#define STARFISH_ENUM_LAZY_BINDING_NAMES(F)")
    flags_idx = 0
    flags_len = len(grouped_interfaces.keys())
    for flag in grouped_interfaces:
      flags_idx += 1
      simple_flag = flag[flag.find("ENABLE_") + 7:]
      w.write(" \\\n    STARFISH_ENUM_LAZY_BINDING_NAMES_{}(F)".format(simple_flag))
    w.write("\n")

    w.write("\n#define STARFISH_ENUM_LAZY_BINDING_NICKNAMES(F)")
    for nickname in constructor_nicknames:
      w.write(" \\\n    F({})".format(nickname))
    w.write("\n")

    w.write("\n#define STARFISH_ENUM_LAZY_BINDING_UNIMPL_NAMES(F)")
    if STRICT_MODE:
      for key in interfaces:
        if interfaces[key].get('unimplemented') and\
           not interfaces[key].get('partial_interface'):
          w.write(" \\\n    F({})".format(key))
    w.write("\n")

    w.write("#endif\n")

def generate_code(ir, args):
  print "Generating binding code..."
  interfaces = ir['interfaces']

  generate_interface_collection_header(interfaces, args)

  for key in interfaces:
    interface = interfaces[key]
    if interface.get('unimplemented', False):
      # print_skip_msg(interface.get('name'), "Unimplemented interface")
      continue
    if interface.get('partial_interface', False):
      # print_skip_msg(interface.get('name'), "PartialInterface")
      continue
    generate_code_with_template(interface, 'base_module' + CPP_EXT, args)

  dictionaries = ir['dictionaries']
  for key in dictionaries:
    dictionary = dictionaries[key]
    if dictionary.get('unimplemented', False):
      # print_skip_msg(dictionary.get('name'), "Unimplemented dictionary")
      continue
    generate_code_with_template(dictionary, 'base_dictionary' + CPP_EXT, args)

def generate_code_with_template(ir, template, args):
  template = env.get_template(template)

  binding_path = os.path.join(STARFISH_PATH, args.out_path)
  if not os.path.exists(binding_path):
    raise Exception("\"[starfish_root]/src/binding\" doesn't exist")

  path = ir['name'] + 'Binding' + CPP_EXT
  with open(os.path.join(binding_path, path), 'w') as w:
    ret = template.render(**ir)
    w.write(ret)
    print("> Generated Code for Module \"{}\"".format(ir['name']))
    # print(ret)

  if args.log_idl:
    path = ir['name'] + 'Idl' + IR_EXT
    with open(os.path.join(binding_path, path), 'w') as w:
      w.write(pprint.pformat(ir))
      print("Logged IR to \"{}\"".format(path))

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
        print("Generated IR from {}".format(file_path))
        file_result = ir
  apply_types(result, result)
  if file_result:
    return result, file_result
  else:
    return result

def filter_assert_true(errmsg, v):
  if v:
    print errmsg
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
  if not (isinstance(word, str) or isinstance(word, unicode)):
    return word

  if len(word) == 0:
    return word

  return word[0].upper() + word[1:]

def filter_to_header_path(inputtxt):
  return inputtxt.replace(_root_dir, '') + '.h'

def filter_digit(num):
  return int(log10(num)) + 1

if __name__ == "__main__":
  argparser = argparse.ArgumentParser()
  argparser.add_argument("root_path", help="root directiory to start")
  argparser.add_argument("out_path", help="out directiory to write file")
  argparser.add_argument("-f", "--file", help="specify an idl file")
  argparser.add_argument("-l", "--log-idl", action='store_true',
                         dest="log_idl", help="flag to log idl")
  args = argparser.parse_args()
  # Argument validation
  if not os.path.isdir(args.root_path):
    print 'ERR: Invalid root path \'' + args.root_path + '\''
    sys.exit(1)
  if args.file is not None and \
     (not os.path.isfile(args.file) or not args.file.endswith('.idl')):
    print 'ERR: Invalid file \'' + args.file + '\''
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
  env.filters['to_header_path'] = filter_to_header_path
  env.filters['digit'] = filter_digit

  # Set globals
  env.globals['nullable_kinds'] = NULLABLE_TYPE_KINDS
  env.globals['string_kinds'] = STRING_KINDS
  env.globals['pointer_kinds'] = POINTER_KINDS
  env.globals['strict_mode'] = STRICT_MODE

  # with open(MODULES_FILE, 'r') as r:
  #  sf_modules = json.loads(r.read())
  _root_dir = args.root_path
  if not _root_dir.endswith('/'):
    _root_dir = _root_dir + '/'

  if args.file is not None:
    all_irs, file_ir = prerun_all(args.root_path, args.file)
    generate_code(file_ir, args)
  else:
    all_irs = prerun_all(args.root_path)
    generate_code(all_irs, args)
