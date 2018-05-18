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
        'default': [],
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
    else:
      group['default'].append(interface["name"])
  return groups

def write_enum_macro(title, list, handle):
  handle.write('\n#define ' + title + '(F)')
  for name in list:
    handle.write(' \\\n    F(' + name + ')')

def gen_interface_collection(interfaces, outpath, mode_strict):
  groups = gen_groups(interfaces)

  with open(os.path.join(outpath, 'Interfaces.h'), 'w') as w:
    w.write(
      '/*\n'
      ' * Copyright (c) 2018-present Samsung Electronics Co., Ltd\n'
      ' *\n'
      ' *  This library is free software; you can redistribute it and/or\n'
      ' *  modify it under the terms of the GNU Lesser General Public\n'
      ' *  License as published by the Free Software Foundation; either\n'
      ' *  version 2 of the License, or (at your option) any later version.\n'
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
      ' */\n\n');
    w.write('#ifndef __StarFishInterfaces__\n')
    w.write('#define __StarFishInterfaces__\n')

    for groupkey in groups:
      flags = groups[groupkey]['flags']
      default = groups[groupkey]['default']
      no_interface = groups[groupkey]['no_interface']
      nickname = groups[groupkey]['nickname']
      seq = groups[groupkey]['seq']
      w.write('\n// Binding group #' + seq)
      if len(flags) > 0:
        if len(default) > 0:
          write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_DEFAULT', [], w)
        if len(no_interface) > 0:
          write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE', [], w)
        if len(nickname) > 0:
          write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NICKNAME', [], w)
      for flag in flags:
        w.write('\n#ifdef ' + flag)
      if len(flags) > 0:
        if len(default) > 0:
          w.write('\n#undef STARFISH_BINDING_GROUP' + seq + '_DEFAULT')
        if len(no_interface) > 0:
          w.write('\n#undef STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE')
        if len(nickname) > 0:
          w.write('\n#undef STARFISH_BINDING_GROUP' + seq + '_NICKNAME')
      if len(default) > 0:
        write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_DEFAULT', sorted(default), w)
      if len(no_interface) > 0:
        write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NOINTERFACE', sorted(no_interface), w)
      if len(nickname) > 0:
        write_enum_macro('STARFISH_BINDING_GROUP' + seq + '_NICKNAME', sorted(nickname), w)
      for flag in flags:
        w.write('\n#endif')
      w.write('\n')

    # Groups definition
    w.write('\n// Groups')
    w.write('\n#define STARFISH_BINDING_GROUPS_DEFAULT(F)')
    for groupkey in groups:
      seq = groups[groupkey]['seq']
      if len(groups[groupkey]['default']) > 0:
        w.write(' \\\n    STARFISH_BINDING_GROUP' + seq + '_DEFAULT(F)')
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

    # GLOBAL_BINDING_NAMES = default + nickname
    # BINDING_NAMES = default + nointerface + nickname
    # BINDING_CLASSES = default + nointerface
    w.write('\n// Combination macros for direct use in StarFish')
    w.write('\n// - GLOBAL_BINDING_NAMES = DEFAULT + NICKNAME')
    w.write('\n// - BINDING_NAMES = DEFAULT + NICKNAME + NOINTERFACE')
    w.write('\n// - BINDING_CLASSES = DEFAULT + NOINTERFACE')
    w.write('\n#define STARFISH_ENUM_GLOBAL_BINDING_NAMES(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_DEFAULT(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NICKNAME(F)')
    w.write('\n#define STARFISH_ENUM_BINDING_NAMES(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_DEFAULT(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NOINTERFACE(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NICKNAME(F)')
    w.write('\n#define STARFISH_ENUM_BINDING_CLASSES(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_DEFAULT(F)')
    w.write(' \\\n    STARFISH_BINDING_GROUPS_NOINTERFACE(F)')

    # Unimpl (only strict mode)
    w.write("\n#define STARFISH_ENUM_BINDING_UNIMPL_NAMES(F)")
    if mode_strict:
      for key in interfaces:
        if interfaces[key].get('unimplemented') and\
           not interfaces[key].get('partial_interface'):
          w.write(" \\\n    F({})".format(key))
    w.write("\n")

    w.write("\n#endif\n")