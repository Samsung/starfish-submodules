#!/usr/bin/env python
import os.path
import sys

ROOT_DIR = os.path.join(os.path.dirname(__file__), os.pardir)
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))
from ply import lex

from idl_lexer import IDLLexer

class StarfishIDLLexer(IDLLexer):
  def t_COMMENT(self, t):
    r'(/\*(.|\n)*?\*/)|(//.*(\n[ \t]*//.*)*)'
    t.lexer.lineno += t.value.count("\n")

  def __init__(self, debug=False):
    IDLLexer.__init__(self)
    self._lexobj = lex.lex(object=self,
                           debug=debug,
                           lextab=None,
                           optimize=(not debug))
    self.tokens.remove('COMMENT')

if __name__ == '__main__':
  import types
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument("file_path")
  args = parser.parse_args()
  data = open(args.file_path).read()

  sf_lexer = StarfishIDLLexer(debug=True)
  sf_lexer.Tokenize(data)
  token = sf_lexer.token()

  while token:
    print token
    # print token.type + ": " + token.value
    token = sf_lexer.token()