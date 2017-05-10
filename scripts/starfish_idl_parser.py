#!/usr/bin/env python
import os.path
import sys

from starfish_idl_lexer import StarfishIDLLexer
from idl_parser import IDLParser, ParseFile

ROOT_DIR = os.path.join(os.path.dirname(__file__), os.pardir)
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))
from ply import yacc

REMOVED_RULES = [
  # Add rule name to remove here
]
for rule in REMOVED_RULES:
  name = 'p_' + rule
  delattr(IDLParser, name)

class StarfishIDLParser(IDLParser):
  def parse_file(self, file_path):
    return ParseFile(self, file_path)

  def __init__(self, lexer, debug=False):
    self.lexer = lexer
    self.tokens = lexer.KnownTokens()
    self.yaccobj = yacc.yacc(module=self,
                             start='Definitions',
                             method='SLR',
                             debug=debug,
                             optimize=(not debug),
                             write_tables=True)
    self.parse_debug = debug
    self.verbose = debug
    self.mute_error = (not debug)
    self._parse_errors = 0
    self._parse_warnings = 0
    self._last_error_msg = None
    self._last_error_lineno = 0
    self._last_error_pos = 0


if __name__ == '__main__':
  import argparse
  argparser = argparse.ArgumentParser()
  argparser.add_argument("file_path")
  args = argparser.parse_args()

  idlparser = StarfishIDLParser(StarfishIDLLexer(debug=True), debug=True)
  top = idlparser.parse_file(args.file_path)
  # print "================================="
  # tmp = top.GetChildren()
  # for child in tmp:
  #   print "child"
  #   print "  CLASS: " + child.GetClass()

  # print "================================="
  print '\n'.join(top.Tree(accept_props=['PROD']))
