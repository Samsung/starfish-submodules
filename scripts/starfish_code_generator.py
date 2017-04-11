#!/usr/bin/env python

import argparse
import os
import pprint
import json
import sys

from jinja2 import Environment, FileSystemLoader
from starfish_idl_reader import gen_ir_from_file

CPP_EXT = ".cpp"
H_EXT = ".h"
IR_EXT = ".txt"
SCRIPT_PATH = os.path.dirname(os.path.abspath(__file__))
TEMPLATES_PATH = os.path.join(SCRIPT_PATH, 'templates')
STARFISH_PATH = os.path.join(SCRIPT_PATH, '..', '..')
BINDING_PATH = os.path.join(STARFISH_PATH, 'src', 'binding')
MODULES_FILE = os.path.join(SCRIPT_PATH, "module.json")

enums = {}

def generate_code(root, f, args, sf_modules):
  irs = gen_ir_from_file(os.path.join(root, f))
  print("Generated IR from {}".format(f))
  global enums

  for module in irs:
    if module['kind'] == 'Enum':
      enums[module['name']] = []
      for item in module['items']:
        enums[module['name']].append(item)

    if module['kind'] != 'Interface':
      continue

    # if 'parent' in  module:
    #   parent = module['parent']
    #   if parent in sf_modules:
    #     module['parent'] = sf_modules[parent]
    #   else:
    #     raise Exception("please write down starfish module for \"{}\", "
    #                     "like starfish module \"node\" for \"Node\" at \"{}\""
    #                       .format(parent, MODULES_FILE))

    template = env.get_template('base_module'+ CPP_EXT)

    if not os.path.exists(BINDING_PATH):
      raise Exception("\"[starfish_root]/src/binding\" doesn't exist")

    module['enums'] = enums
    path = module['name'] + 'Binding' + CPP_EXT
    with open(os.path.join(BINDING_PATH, path), 'w') as w:
      ret = template.render(**module)
      w.write(ret)
      print("Generated Code for Module \"{}\"".format(module['name']))
      # print(ret)

    if args.log_idl:
      path = module['name'] + 'Idl' + IR_EXT
      with open(os.path.join(BINDING_PATH, path), 'w') as w:
        w.write(pprint.pformat(irs))
        print("Logged IR to \"{}\"".format(path))

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
  argparser.add_argument("-p", "--path", help="path to idl")
  argparser.add_argument("-l", "--log-idl", action='store_true',
                         dest="log_idl", help="flag to log idl")
  args = argparser.parse_args()

  env = Environment(loader=FileSystemLoader(TEMPLATES_PATH), trim_blocks=True,
                    lstrip_blocks=True)
  # Set custom filters
  env.filters['assert_true'] = filter_assert_true
  env.filters['assert_false'] = filter_assert_false
  env.filters['to_arg_syntax'] = filter_to_argument_syntax
  env.filters['first_word_capitalize'] = filter_first_word_capitalize

  with open(MODULES_FILE, 'r') as r:
    sf_modules = json.loads(r.read())

    if os.path.isfile(args.path):
      generate_code('.', args.path, args, sf_modules)
    else:
      for (root, dirs, files) in os.walk(args.path):
        for f in files:
          if os.path.splitext(f)[-1] != '.idl':
            continue

          generate_code(root, f, args, sf_modules)
