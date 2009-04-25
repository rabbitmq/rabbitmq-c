##   The contents of this file are subject to the Mozilla Public License
##   Version 1.1 (the "License"); you may not use this file except in
##   compliance with the License. You may obtain a copy of the License at
##   http://www.mozilla.org/MPL/
##
##   Software distributed under the License is distributed on an "AS IS"
##   basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
##   License for the specific language governing rights and limitations
##   under the License.
##
##   The Original Code is RabbitMQ.
##
##   The Initial Developers of the Original Code are LShift Ltd,
##   Cohesive Financial Technologies LLC, and Rabbit Technologies Ltd.
##
##   Portions created before 22-Nov-2008 00:00:00 GMT by LShift Ltd,
##   Cohesive Financial Technologies LLC, or Rabbit Technologies Ltd
##   are Copyright (C) 2007-2008 LShift Ltd, Cohesive Financial
##   Technologies LLC, and Rabbit Technologies Ltd.
##
##   Portions created by LShift Ltd are Copyright (C) 2007-2009 LShift
##   Ltd. Portions created by Cohesive Financial Technologies LLC are
##   Copyright (C) 2007-2009 Cohesive Financial Technologies
##   LLC. Portions created by Rabbit Technologies Ltd are Copyright
##   (C) 2007-2009 Rabbit Technologies Ltd.
##
##   All Rights Reserved.
##
##   Contributor(s): ______________________________________.
##

from __future__ import nested_scopes

from amqp_codegen import *
import string
import re

cTypeMap = {
    'octet': 'uint8_t',
    'shortstr': 'amqp_bytes_t',
    'longstr': 'amqp_bytes_t',
    'short': 'uint16_t',
    'long': 'uint32_t',
    'longlong': 'uint64_t',
    'bit': 'amqp_boolean_t',
    'table': 'amqp_table_t',
    'timestamp': 'uint64_t',
}

def convertTable(d):
    if len(d) == 0:
        return "[]"
    else: raise 'Non-empty table defaults not supported', d

def convertBytes(x):
    return "(amqp_bytes_t) { .length = %d, .bytes = \"%s\" }" % (len(x), x)

defaultValueTypeConvMap = {
    bool : lambda x: x and "1" or "0",
    str : convertBytes,
    int : lambda x: str(x),
    float : lambda x: str(x),
    dict: convertTable,
    unicode: lambda x: convertBytes(x.encode("utf-8"))
}

def c_ize(s):
    s = s.replace('-', '_')
    s = s.replace(' ', '_')
    return s

AmqpMethod.defName = lambda m: cConstantName(c_ize(m.klass.name) + '_' + c_ize(m.name) + "_method")
AmqpMethod.structName = lambda m: "amqp_" + c_ize(m.klass.name) + '_' + c_ize(m.name) + "_t"

def cConstantName(s):
    return 'AMQP_' + '_'.join(re.split('[- ]', s.upper()))

class PackedMethodBitField:
    def __init__(self, index):
        self.index = index
        self.domain = 'bit'
        self.contents = []

    def extend(self, f):
        self.contents.append(f)

    def count(self):
        return len(self.contents)

    def full(self):
        return self.count() == 8
    
def genErl(spec):
    def cType(domain):
        return cTypeMap[spec.resolveDomain(domain)]

    def fieldTypeList(fields):
        return '[' + ', '.join([cType(f.domain) for f in fields]) + ']'

    def fieldNameList(fields):
        return '[' + ', '.join([c_ize(f.name) for f in fields]) + ']'

    def fieldTempList(fields):
        return '[' + ', '.join(['F' + str(f.index) for f in fields]) + ']'

    def fieldMapList(fields):
        return ', '.join([c_ize(f.name) + " = F" + str(f.index) for f in fields])

    def genLookupMethodName(m):
        print '    case %s: return "%s";' % (m.defName(), m.defName())

    def packMethodFields(fields):
        packed = []
        bitfield = None
        for f in fields:
            if cType(f.domain) == 'bit':
                if not(bitfield) or bitfield.full():
                    bitfield = PackedMethodBitField(f.index)
                    packed.append(bitfield)
                bitfield.extend(f)
            else:
                bitfield = None
                packed.append(f)
        return packed

    def methodFieldFragment(f):
        type = cType(f.domain)
        p = 'F' + str(f.index)
        if type == 'shortstr':
            return p+'Len:8/unsigned, '+p+':'+p+'Len/binary'
        elif type == 'longstr':
            return p+'Len:32/unsigned, '+p+':'+p+'Len/binary'
        elif type == 'octet':
            return p+':8/unsigned'
        elif type == 'shortint':
            return p+':16/unsigned'
        elif type == 'longint':
            return p+':32/unsigned'
        elif type == 'longlongint':
            return p+':64/unsigned'
        elif type == 'timestamp':
            return p+':64/unsigned'
        elif type == 'bit':
            return p+'Bits:8'
        elif type == 'table':
            return p+'Len:32/unsigned, '+p+'Tab:'+p+'Len/binary'
        else:
            return 'UNIMPLEMENTED'

    def genFieldPostprocessing(packed):
        for f in packed:
            type = cType(f.domain)
            if type == 'bit':
                for index in range(f.count()):
                    print "  F%d = ((F%dBits band %d) /= 0)," % \
                          (f.index + index,
                           f.index,
                           1 << index)
            elif type == 'table':
                print "  F%d = rabbit_binary_parser:parse_table(F%dTab)," % \
                      (f.index, f.index)
            else:
                pass

    def genDecodeMethodFields(m):
        packedFields = packMethodFields(m.arguments)
        binaryPattern = ', '.join([methodFieldFragment(f) for f in packedFields])
        if binaryPattern:
            restSeparator = ', '
        else:
            restSeparator = ''
        recordConstructorExpr = '#%s{%s}' % (m.structName(), fieldMapList(m.arguments))
        print "decode_method_fields(%s, <<%s>>) ->" % (m.defName(), binaryPattern)
        genFieldPostprocessing(packedFields)
        print "  %s;" % (recordConstructorExpr,)

    def genDecodeProperties(c):
        print "decode_properties(%d, PropBin) ->" % (c.index)
        print "  %s = rabbit_binary_parser:parse_properties(%s, PropBin)," % \
              (fieldTempList(c.fields), fieldTypeList(c.fields))
        print "  #'P_%s'{%s};" % (c_ize(c.name), fieldMapList(c.fields))

    def genFieldPreprocessing(packed):
        for f in packed:
            type = cType(f.domain)
            if type == 'bit':
                print "  F%dBits = (%s)," % \
                      (f.index,
                       ' bor '.join(['(bitvalue(F%d) bsl %d)' % (x.index, x.index - f.index)
                                     for x in f.contents]))
            elif type == 'table':
                print "  F%dTab = rabbit_binary_generator:generate_table(F%d)," % (f.index, f.index)
                print "  F%dLen = size(F%dTab)," % (f.index, f.index)
            elif type in ['shortstr', 'longstr']:
                print "  F%dLen = size(F%d)," % (f.index, f.index)
            else:
                pass

    def genEncodeMethodFields(m):
        packedFields = packMethodFields(m.arguments)
        print "encode_method_fields(#%s{%s}) ->" % (m.structName(), fieldMapList(m.arguments))
        genFieldPreprocessing(packedFields)
        print "  <<%s>>;" % (', '.join([methodFieldFragment(f) for f in packedFields]))

    def genEncodeProperties(c):
        print "encode_properties(#'P_%s'{%s}) ->" % (c_ize(c.name), fieldMapList(c.fields))
        print "  rabbit_binary_generator:encode_properties(%s, %s);" % \
              (fieldTypeList(c.fields), fieldTempList(c.fields))

    def genLookupException(c,v,cls):
        # We do this because 0.8 uses "soft error" and 8.1 uses "soft-error".
        mCls = c_ize(cls).upper()
        if mCls == 'SOFT_ERROR': genLookupException1(c,'false')
        elif mCls == 'HARD_ERROR': genLookupException1(c, 'true')
        elif mCls == '': pass
        else: raise 'Unknown constant class', cls

    def genLookupException1(c,hardErrorBoolStr):
        n = cConstantName(c)
        print 'lookup_amqp_exception(%s) -> {%s, ?%s, <<"%s">>};' % \
              (n.lower(), hardErrorBoolStr, n, n)

    methods = spec.allMethods()

    print '#include <stdlib.h>'
    print '#include <string.h>'
    print '#include <stdio.h>'
    print
    print '#include "amqp_framing.h"'

    print """
char const *amqp_method_name(uint32_t methodNumber) {
  switch (methodNumber) {"""
    for m in methods: genLookupMethodName(m)
    print """    default: return NULL;
  }
}
"""

    print """
amqp_boolean_t amqp_method_has_content(uint32_t methodNumber) {
  switch (methodNumber) {"""
    for m in methods:
        if m.hasContent:
            print '    case %s: return 1;' % (m.defName())
    print """    default: return 0;
  }
}
"""

    for m in methods: genDecodeMethodFields(m)
    print "decode_method_fields(Name, BinaryFields) ->"
    print "  rabbit_misc:frame_error(Name, BinaryFields)."

    for c in spec.allClasses(): genDecodeProperties(c)
    print "decode_properties(ClassId, _BinaryFields) -> exit({unknown_class_id, ClassId})."

    for m in methods: genEncodeMethodFields(m)
    print "encode_method_fields(Record) -> exit({unknown_method_name, element(1, Record)})."

    for c in spec.allClasses(): genEncodeProperties(c)
    print "encode_properties(Record) -> exit({unknown_properties_record, Record})."

    for (c,v,cls) in spec.constants: genLookupException(c,v,cls)
    print "lookup_amqp_exception(Code) ->"
    print "  rabbit_log:warning(\"Unknown AMQP error code '~p'~n\", [Code]),"
    print "  {true, ?INTERNAL_ERROR, <<\"INTERNAL_ERROR\">>}."    


def genHrl(spec):
    def cType(domain):
        return cTypeMap[spec.resolveDomain(domain)]

    def fieldNameList(fields):
        return ', '.join([c_ize(f.name) for f in fields])

    def fieldDeclList(fields):
        return ''.join(["  %s %s;\n" % (cType(f.domain), c_ize(f.name)) for f in fields])

    def propDeclList(fields):
        return ''.join(["  %s %s;\n" % (cType(f.domain), c_ize(f.name))
                        for f in fields
                        if spec.resolveDomain(f.domain) != 'bit'])

    def fieldNameListDefaults(fields):
        def fillField(field):
            result = c_ize(f.name)
            if field.defaultvalue != None:
                conv_fn = defaultValueTypeConvMap[type(field.defaultvalue)]
                result += ' = ' + conv_fn(field.defaultvalue)
            return result
        return ', '.join([fillField(f) for f in fields])
    
    methods = spec.allMethods()

    print "#ifndef LIBRABBITMQ_AMQP_FRAMING_H"
    print "#define LIBRABBITMQ_AMQP_FRAMING_H"
    print
    print "#define AMQP_PROTOCOL_VERSION_MAJOR %d" % (spec.major)
    print "#define AMQP_PROTOCOL_VERSION_MINOR %d" % (spec.minor)
    print "#define AMQP_PROTOCOL_PORT %d" % (spec.port)

    for (c,v,cls) in spec.constants:
        print "#define %s %s" % (cConstantName(c), v)
    print

    print "/* Method field records. */"
    for m in methods:
        methodid = m.klass.index << 16 | m.index
        print "#define %s ((uint32_t) 0x%.08X) /* %d, %d; %d */" % \
              (m.defName(),
               methodid,
               m.klass.index,
               m.index,
               methodid)
        print "typedef struct {\n%s} %s;\n" % (fieldDeclList(m.arguments), m.structName())

    print "/* Class property records. */"
    for c in spec.allClasses():
        index = 0
        for f in c.fields:
            if index % 16 == 15:
                index = index + 1
            shortnum = index / 16
            partialindex = 15 - (index % 16)
            bitindex = shortnum * 16 + partialindex
            print '#define %s_FLAG (1 << %d)' % (cConstantName(c.name + '_' + f.name), bitindex)
            index = index + 1
        print "typedef struct {\n  uint32_t _flags;\n%s} %s;\n" % \
              (fieldDeclList(c.fields), \
               'amqp_%s_properties_t' % (c_ize(c.name),))

    print "#endif"

def generateErl(specPath):
    genErl(AmqpSpec(specPath))

def generateHrl(specPath):
    genHrl(AmqpSpec(specPath))
    
if __name__ == "__main__":
    do_main(generateHrl, generateErl)
