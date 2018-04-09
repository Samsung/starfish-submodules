#!/usr/bin/env python
import os.path
import sys

from starfish_idl_lexer import StarfishIDLLexer
from idl_parser import IDLParser, ParseFile, ListFromConcat

ROOT_DIR = os.path.join(os.path.dirname(__file__), os.pardir)
sys.path.insert(0, os.path.join(ROOT_DIR, 'third_party'))
from ply import yacc

delattr(IDLParser, 'p_Top')
delattr(IDLParser, 'p_Comments')
delattr(IDLParser, 'p_CommentsRest')
# NOTE Temporarily disable to prevent warnings
delattr(IDLParser, 'p_ExceptionField')
delattr(IDLParser, 'p_ExceptionFieldError')

class StarfishIDLParser(IDLParser):
  def p_ExtendedAttributes(self, p):
    """ExtendedAttributes : ExtendedAttributeComma ExtendedAttribute ExtendedAttributes
                          | ExtendedAttributeComma
                          |"""
    if len(p) > 3:
      p[0] = ListFromConcat(p[2], p[3])

  def p_ExtendedAttributeComma(self, p):
    """ExtendedAttributeComma : ','"""
    p[0] = p[1]

  def p_ExtendedAttribute(self, p):
    """ExtendedAttribute : ExtendedAttributeNoArgs
                         | ExtendedAttributeArgList
                         | ExtendedAttributeIdent
                         | ExtendedAttributeIdentList
                         | ExtendedAttributeNamedArgList
                         | ExtendedAttributeString
                         | ExtendedAttributeStringList"""
    p[0] = p[1]

  def p_ExtendedAttributeString(self, p):
    """ExtendedAttributeString : identifier '=' string"""
    value = self.BuildAttribute('VALUE', p[3])
    p[0] = self.BuildNamed('ExtAttribute', p, 1, value)

  def p_ExtendedAttributeStringList(self, p):
    """ExtendedAttributeStringList : identifier '=' '(' StringList ')'"""
    value = self.BuildAttribute('VALUE', p[4])
    p[0] = self.BuildNamed('ExtAttribute', p, 1, value)

  def p_StringList(self, p):
    """StringList : string Strings"""
    p[0] = ListFromConcat(p[1], p[2])

  def p_Strings(self, p):
    """Strings : ',' string Strings
               |"""
    if len(p) > 1:
      p[0] = ListFromConcat(p[2], p[3])

  def p_ExtendedAttributeList(self, p):
    """ExtendedAttributeList : '[' ExtendedAttribute ExtendedAttributes ']'
                               | """
    if len(p) > 3:
      items = ListFromConcat(p[2], p[3])
      p[0] = self.BuildProduction('ExtAttributes', p, 1, items)

  # FIXME when we support 'exception' expression
  def p_ExceptionMember(self, p):
    """ExceptionMember : Const
                       | ReadonlyMember
                       | Operation"""
    p[0] = p[1]

  def parse_file(self, file_path):
    result = ParseFile(self, file_path);
    if self._parse_errors > 0:
      print "PARSE ERROR: " + file_path
      print "> please execute below command to see details"
      print "> ./binding_generator/scripts/starfish_idl_reader.py " + file_path
      sys.exit(1)
    return result

  def __init__(self, lexer, debug=False):
    self.lexer = lexer
    self.tokens = lexer.KnownTokens()
    self.yaccobj = yacc.yacc(module=self,
                             start='Definitions',
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
