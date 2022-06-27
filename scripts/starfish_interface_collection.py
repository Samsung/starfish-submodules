#!/usr/bin/env python
import os
import sys

def gen_group_key(flaglist):
  result = ''
  for flag in flaglist:
    result += flag
  return result

def gen_groups(interfaces):
  groups = {}
  count = 0
  for key in interfaces:
    interface = interfaces[key]

    if interface.get('partial_interface'):
      continue
    if interface.get('unimplemented'):
      continue

    if "flags" in interface:
      flags = interface["flags"]
      groupkey = gen_group_key(sorted(flags))
    else:
      flags = []
      groupkey = "DEFAULT"

    if groups.has_key(groupkey):
      group = groups.get(groupkey)
    else:
      group = {
        'seq': str(count),
        'flags': flags,
        'exposed': {
          'COMMON': set(),
          'WINDOW': set()
        },
        'no_interface': [],
        'nickname': []
      }
      groups[groupkey] = group
      count += 1

    if interface.get('constructor') and \
       len(interface['constructor']['name']) > 0:
      group['nickname'].append(interface['constructor']['name'])

    if interface.get('no_interface'):
      group['no_interface'].append(interface["name"])
    elif interface.get('exposed'):
      if len(interface['exposed']) > 1 and 'Window' in interface['exposed']:
        group['exposed']['COMMON'].add(interface["name"])
        continue
      for key in interface['exposed']:
        exposed_name = key.upper()
        exposed_group = group['exposed']
        if exposed_name not in exposed_group.keys():
          exposed_group[exposed_name] = set()
        exposed_group[exposed_name].add(interface['name'])
    else:
      # If there is no exposed keyword,
      # it is considered to be exposed only to the window.
      group['exposed']['WINDOW'].add(interface["name"])

  return groups

def write_enum_macro(title, list, handle):
  handle.write('\n#define ' + title + '(F)')
  for name in list:
    handle.write(' \\\n    F(' + name + ')')

def gen_interface_collection(interfaces, outpath, mode_strict, exposed_module):
  groups = gen_groups(interfaces)
  with open(os.path.join(outpath, 'Interfaces.h'), 'w') as w:
    w.write(
      '/*\n'
      ' * Copyright (c) 2018-present Samsung Electronics Co., Ltd\n'
      ' *\n'
      ' *  This library is free software; you can redistribute it and/or\n'
      ' *  modify it under the terms of the GNU Lesser General Public\n'
      ' *  License as published by the Free Software Foundation; either\n'
      ' *  version 2.1 of the License, or (at your option) any later version.\n'
      ' *\n'
      ' *  This library is distributed in the hope that it will be useful,\n'
      ' *  but WITHOUT ANY WARRANTY; without even the implied warranty of\n'
      ' *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n'
      ' *  Lesser General Public License for more details.\n'
      ' *\n'
      ' *  You should have received a copy of the GNU Lesser General Public\n'
      ' *  License along with this library; if not, write to the Free Software\n'
      ' *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301\n'
      ' *  USA\n'
      ' */\n\n')
    w.write('#ifndef __StarfishInterfaces__\n')
    w.write('#define __StarfishInterfaces__\n')

    exposed_list = set()
    for groupkey in groups:
      exposed_list.update(groups[groupkey]['exposed'].keys())

    for groupkey in groups:
      flags = groups[groupkey]['flags']
      exposed = groups[groupkey]['exposed']
      no_interface = groups[groupkey]['no_interface']
      nickname = groups[groupkey]['nickname']
      seq = groups[groupkey]['seq']
      w.write('\n// Binding group #' + seq)
      if len(flags) > 0:
        for exposed_key in exposed:
          if exposed_key in exposed_list:
            write_enum_macro('STARFISH_BINDING_GROUP%s_%s' % (seq, exposed_key), [], w)
        if len(no_interface) > 0:
          write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE', [], w)
        if len(nickname) > 0:
          write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NICKNAME', [], w)
      for flag in flags:
        w.write('\n#ifdef ' + flag)
      if len(flags) > 0:
        for exposed_key in exposed:
            if exposed_key in exposed_list:
              w.write('\n#undef STARFISH_BINDING_GROUP%s_%s' % (seq, exposed_key))
        if len(no_interface) > 0:
          w.write('\n#undef STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE')
        if len(nickname) > 0:
          w.write('\n#undef STARFISH_BINDING_GROUP' + seq + '_NICKNAME')
     
      for exposed_key in exposed:
        if exposed_key in exposed_list:
          write_enum_macro('STARFISH_BINDING_GROUP%s_%s' % (seq, exposed_key), sorted(exposed[exposed_key]), w)
      if len(no_interface) > 0:
        write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE', sorted(no_interface), w)
      if len(nickname) > 0:
        write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NICKNAME', sorted(nickname), w)
      for flag in flags:
        w.write('\n#endif')
      w.write('\n')

    # Groups definition
    w.write('\n// Groups')
    for exposed_key in exposed_list:
      w.write('\n#define STARFISH_BINDING_GROUPS_%s(F)' % exposed_key)
      for groupkey in groups:
        seq = groups[groupkey]['seq']
        if exposed_key in groups[groupkey]['exposed'] and len(groups[groupkey]['exposed'][exposed_key]) > 0:
          w.write(' \\\n    STARFISH_BINDING_GROUP%s_%s(F)' % (seq, exposed_key))
    w.write('\n#define STARFISH_BINDING_GROUPS_NOINTERFACE(F)')
    for groupkey in groups:
      seq = groups[groupkey]['seq']
      if len(groups[groupkey]['no_interface']) > 0:
        w.write(' \\\n    STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE(F)')
    w.write('\n#define STARFISH_BINDING_GROUPS_NICKNAME(F)')
    for groupkey in groups:
      seq = groups[groupkey]['seq']
      if len(groups[groupkey]['nickname']) > 0:
        w.write(' \\\n    STARFISH_BINDING_GROUP' + seq + '_NICKNAME(F)')
    w.write('\n')

    # TODO: 'nointerface' should be classified according to 'exposed' keyword.
    # GLOBAL_BINDING_NAMES = exposed + nickname
    # BINDING_NAMES = exposed + nointerface + nickname
    # BINDING_CLASSES = exposed + nointerface

    exposed_module = 'COMMON'
    w.write('\n#define STARFISH_ENUM_GLOBAL_BINDING_%s_NAMES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_COMMON(F)')

    w.write('\n#define STARFISH_ENUM_BINDING_%s_NAMES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_COMMON(F)')

    w.write('\n#define STARFISH_ENUM_BINDING_%s_CLASSES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_COMMON(F)')
    w.write('\n')

    exposed_module = 'WORKER'
    w.write('\n#define STARFISH_ENUM_GLOBAL_BINDING_%s_NAMES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_ENUM_BINDING_COMMON_NAMES(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_%s(F)' % exposed_module)
   
    w.write('\n#define STARFISH_ENUM_BINDING_%s_NAMES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_%s(F)' % exposed_module)

    w.write('\n#define STARFISH_ENUM_BINDING_%s_CLASSES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_%s(F)' % exposed_module)
    w.write('\n')

    exposed_module = 'WINDOW'
    w.write('\n#define STARFISH_ENUM_GLOBAL_BINDING_%s_NAMES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_ENUM_BINDING_COMMON_NAMES(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_%s(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NICKNAME(F)')

    w.write('\n#define STARFISH_ENUM_BINDING_%s_NAMES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_%s(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NOINTERFACE(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NICKNAME(F)')

    w.write('\n#define STARFISH_ENUM_BINDING_%s_CLASSES(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_%s(F)' % exposed_module)
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NOINTERFACE(F)')
    w.write('\n')

    w.write('\n// Combination macros for direct use in Starfish')
    w.write('\n// - GLOBAL_BINDING_NAMES => use STARFISH_ENUM_GLOBAL_BINDING_WINDOW_NAMES or STARFISH_ENUM_GLOBAL_BINDING_WORKER_NAMES')
    w.write('\n// - BINDING_NAMES = COMMON + WINDOW(EXPOSED + NICKNAME + NOINTERFACE) + WORKER(EXPOSED)')
    w.write('\n// - BINDING_CLASSES = COMMON + WINDOW(EXPOSED + NOINTERFACE) + WORKER(EXPOSED)')

    w.write('\n#define STARFISH_ENUM_BINDING_NAMES(F)')
    w.write(' \\\n    STARFISH_ENUM_BINDING_COMMON_NAMES(F)')
    w.write(' \\\n    STARFISH_ENUM_BINDING_WINDOW_NAMES(F)')
    w.write(' \\\n    STARFISH_ENUM_BINDING_WORKER_NAMES(F)')

    w.write('\n#define STARFISH_ENUM_BINDING_CLASSES(F)')
    w.write(' \\\n    STARFISH_ENUM_BINDING_COMMON_CLASSES(F)')
    w.write(' \\\n    STARFISH_ENUM_BINDING_WINDOW_CLASSES(F)')
    w.write(' \\\n    STARFISH_ENUM_BINDING_WORKER_CLASSES(F)')
    w.write('\n')

    # Unimpl (only strict mode)
    w.write("\n#define STARFISH_ENUM_BINDING_UNIMPL_NAMES(F)")
    if mode_strict:
      for key in interfaces:
        if interfaces[key].get('unimplemented') and\
           not interfaces[key].get('partial_interface'):
          w.write(" \\\n    F({})".format(key))
    w.write("\n")

    w.write("\n#endif\n")
