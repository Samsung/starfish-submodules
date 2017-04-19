#!/usr/bin/env python

import argparse
import os
import pprint
import json
import sys

from jinja2 import Environment, FileSystemLoader
from starfish_idl_reader import gen_ir_from_file, merge_irs, apply_types

CPP_EXT = ".cpp"
H_EXT = ".h"
IR_EXT = ".txt"
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
TEMPLATES_PATH = os.path.join(SCRIPT_PATH, 'templates')
# IDL_PATH = os.path.join(SCRIPT_PATH, '..', 'idl')
STARFISH_PATH = os.path.join(SCRIPT_PATH, '..', '..')
BINDING_PATH = os.path.join(STARFISH_PATH, 'src', 'binding')
# MODULES_FILE = os.path.join(SCRIPT_PATH, "module.json")
# BUIILTINT_MODULE_FILE = os.path.join(IDL_PATH, 'Builtin.idl')
NULLABLE_TYPE_KINDS = ['StringType', 'PrimitiveType', 'Dictionary']
STRING_KINDS = ['StringType', 'Enum']
POINTER_KINDS = ['Typeref', 'Callback', 'Promise']

def generate_code(ir, args): #, sf_modules):
  interfaces = ir['interfaces']
  for key in interfaces:
    interface = interfaces[key]
    if interface.get('partial_interface', False):
      print "Skip generating code for partial interface " +\
             interface.get('name')
      continue
    generate_code_with_template(interface, 'base_module' + CPP_EXT, args)

  dictionaries = ir['dictionaries']
  for key in dictionaries:
    dictionary = dictionaries[key]
    if dictionary.get('unimplemented', False):
      continue
    generate_code_with_template(dictionary, 'base_dictionary' + CPP_EXT, args)

def generate_code_with_template(ir, template, args):
  template = env.get_template(template)

  if not os.path.exists(BINDING_PATH):
    raise Exception("\"[starfish_root]/src/binding\" doesn't exist")

  path = ir['name'] + 'Binding' + CPP_EXT
  with open(os.path.join(BINDING_PATH, path), 'w') as w:
    ret = template.render(**ir)
    w.write(ret)
    print("Generated Code for Module \"{}\"".format(ir['name']))
    # print(ret)

  if args.log_idl:
    path = ir['name'] + 'Idl' + IR_EXT
    with open(os.path.join(BINDING_PATH, path), 'w') as w:
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
      ir = gen_ir_from_file(file_path)
      merge_irs(result, ir)
      if file_path == file_alone:
        print("Generated IR from {}".format(file_path))
        file_result = ir
  if file_result:
    apply_types(result, file_result)
    return result, file_result
  else:
    apply_types(result, result)
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

if __name__ == "__main__":
  argparser = argparse.ArgumentParser()
  argparser.add_argument("root_path", help="root directiory to start")
  argparser.add_argument("-f", "--file", help="specify an idl file")
  argparser.add_argument("-l", "--log-idl", action='store_true',
                         dest="log_idl", help="flag to log idl")
  args = argparser.parse_args()
  # Argument validation
  if not os.path.isdir(args.root_path):
    print 'ERR: Invalid root path \'' + args.root_path + '\''
    sys.exit(1)
  if args.file is not None and \
     not os.path.isfile(args.file) and \
     not args.file.endswith('.idl'):
    print 'ERR: Invalid file \'' + args.file + '\''
    sys.exit(1)

  env = Environment(loader=FileSystemLoader(TEMPLATES_PATH), trim_blocks=True,
                    lstrip_blocks=True)
  # Set custom filters
  env.filters['assert_true'] = filter_assert_true
  env.filters['assert_false'] = filter_assert_false
  env.filters['to_arg_syntax'] = filter_to_argument_syntax
  env.filters['first_word_capitalize'] = filter_first_word_capitalize

  # Set globals
  env.globals['nullable_kinds'] = NULLABLE_TYPE_KINDS
  env.globals['string_kinds'] = STRING_KINDS
  env.globals['pointer_kinds'] = POINTER_KINDS

  # with open(MODULES_FILE, 'r') as r:
  #  sf_modules = json.loads(r.read())

  if args.file is not None:
    all_irs, file_ir = prerun_all(args.root_path, args.file)
    generate_code(file_ir, args)
  else:
    all_irs = prerun_all(args.root_path)
    generate_code(all_irs, args)
