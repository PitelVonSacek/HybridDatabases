#!/usr/bin/perl

use strict;
use warnings;

use Parse::RecDescent;

my $interface = $ARGV[0];
my $implementation = $ARGV[1];

our @attr_types;
our $int = "";
our $impl = "";

$Parse::RecDescent::skip = "(\\s|;|#[^\n]*\n)*";

my $grammar = q{
  { my %c; }

  root: ( int | impl | attr_type | <error> )(s?) /^\\Z/ { 1 }

  int: 'Interface' block { $::int = $::int . $item[2] }
  impl: 'Implementation' block { $::impl = $::impl . $item[2] }

  attr_type: 'AttributeType' id ':' c_type '{'  ( is_primitive |  init(?) 
    destroy(?)  write(?) )(?)  load store '}'
    { $c{name} = $item{id}; $c{type} = $item{c_type};
      push @::attr_types, { %c }; %c = () }

  is_primitive: 'IsPrimitive' bool { $c{is_primitive} = $item[2] }

  init: 'Init' block { $c{init} = $item[2] }
  destroy: 'Destroy' block { $c{destroy} = $item[2] }
  write: 'Write' block { $c{write} = $item[2] }
  load: 'Load' block { $c{load} = $item[2] }
  store: 'Store' block { $c{store} = $item[2] }

  block: /{([^{}]*(:?{(?1)}[^{}]*)*)}/s { $1 }
  bool: id { if ($item[1] =~ /true|yes|1/i) { 1 } else { 0 } }
  c_type: /(union|struct)\\s*{[^\}]*}|(\\w+\\s*\\**)+/ 
    { if ($item[1] =~ /^(struct|union)/) { $item[1] } else 
      { "struct { $item[1] value; }" } }
  id: /\\w+/ { $item[1] }
};

my $parser = new Parse::RecDescent($grammar) or die "Error\n";
my @input = <STDIN>;
my $input = join(" ", @input);
$parser->root($input) or die 'Parse error!';

if ($interface) {
  print <<EOF;
#ifndef __ATTRIBUTES_GEN_INTERFACE___
#define __ATTRIBUTES_GEN_INTERFACE___

/*
 * Attributes interface
 * generated file, don't edit
 */

$int

enum {
EOF

  for my $attr_type (@attr_types) {
    ${$attr_type}{is_primitive} = 1 unless 
      defined ${$attr_type}{is_primitive} or 
      defined ${$attr_type}{init} or
      defined ${$attr_type}{destroy} or
      defined ${$attr_type}{write};

    ${$attr_type}{is_primitive} ||= 0;

    print "  ${$attr_type}{name},\n";
  }
  print "  attribute_type_count\n};\n\n";

  for my $a (@attr_types) {
    print "typedef ${$a}{type} ${$a}{name}_t;\n" .
          "  /// \@cond 0\n".
          "typedef typeof(((${$a}{name}_t*)0)->value) ${$a}{name}_value_t;\n".
          "enum { ${$a}{name}_is_primitive = ${$a}{is_primitive} };\n".
          "  /// \@endcond\n\n";
  }

  print "union UniversalAttribute {\n";
  for my $a (@attr_types) {
    print "  ${$a}{name}_t ${$a}{name}_value;\n"
  }
  print "};\n\n";
  
  sub type_switch;
  sub type_switch {
    my ($cond, $expr, $type, @rest) = @_;
    
    if ($type) {
      printf "  static_if($cond, \\\n", $type;
      printf "  $expr, \\\n", $type;
      type_switch $cond, $expr, @rest;
      print  ")";
    } else {
      print "  (void)0 \\\n  ";
    }
  };

  print "#define attributeSize(Type) \\\n";
  type_switch "types_equal(typeof(Type), %s_t)", "sizeof(%s_t)", 
    map { ${$_}{name} } @attr_types;
  print "\n\n\n";

  print "#define attributeIsPrimitive(Type) \\\n";
  type_switch "types_equal(typeof(Type), %s_t)", "%s_is_primitive", 
    map { ${$_}{name} } @attr_types;
  print "\n\n\n";
 
  print "#define attributeTypeId(Type) \\\n";
  type_switch "types_equal(typeof(Type), %s_t)", "%s", 
    map { ${$_}{name} } @attr_types;
  print "\n\n\n";


  print "#endif\n\n";
}

if ($implementation) {
  print <<EOF;
#ifndef __ATTRIBUTES_GEN_IMPLEMENTATION___
#define __ATTRIBUTES_GEN_IMPLEMENTATION___

/*
 * Attributes implementation
 * generated file, don't edit
 */

$impl

#pragma push_macro("readFailed")
#undef readFailed
#define readFailed return false

EOF
  
  for my $a (@attr_types) {
    ${$a}{init}    ||= '*attr = (typeof(*attr)){ 0 }';
    ${$a}{destroy} ||= '*attr = (typeof(*attr)){ 0 }';
    ${$a}{write}   ||= 'attr->value = *value';

    print <<EOF;
static inline void attribute_init_${$a}{name}(${$a}{name}_t *attr) {
  ${$a}{init};
}

static inline void attribute_destroy_${$a}{name}(
        struct GenericAllocator *allocator,
        uint64_t end_time, ${$a}{name}_t *attr) {
  ${$a}{destroy};
}

static inline bool attribute_write_${$a}{name}(Handler *H, 
        ${$a}{name}_t * restrict attr, const ${$a}{name}_value_t * restrict value) {
  ${$a}{write};
  return true;
}

static inline bool attribute_load_${$a}{name}(Reader *R, 
        struct GenericAllocator *allocator, ${$a}{name}_t *attr) {
  ${$a}{load};
  return true;
}

static inline void attribute_store_${$a}{name}(Writer *W, const ${$a}{name}_value_t *value) {
  ${$a}{store};
}

EOF
  }

  print "#pragma pop_macro(\"readFailed\")\n\n";


  sub for_each {
    my $arg = shift;
    for my $a (@attr_types) {
      printf "    case ${$a}{name}: return $arg;\n", ${$a}{name};
    }
  };

  # init
  print "void attribute_init(int type, void *attr) {\n" .
        "  switch (type) {\n";
  for_each "attribute_init_%s(attr)";
  print "    default: assert(0);\n  }\n}\n\n";

  #destroy
  print "void attribute_destroy(int type, struct GenericAllocator *allocator,\n".
        "                       uint64_t end_time, void *attr) {\n" .
        "  switch (type) {\n";
  for_each "attribute_destroy_%s(allocator, end_time, attr)";
  print "    default: assert(0);\n  }\n}\n\n";

  # write
  print "bool attribute_write(int type, Handler *H,\n".
        "                     void * restrict attr, const void * restrict value) {\n".
        "  switch (type) {\n";
  for_each "attribute_write_%s(H, attr, value)";
  print "    default: assert(0);\n  }\n}\n\n";

  # load
  print "bool attribute_load(int type, Reader *R,\n".
        "                    struct GenericAllocator *allocator,\n".
        "                    void *attr) {\n".
        "  switch (type) {\n";
  for_each "attribute_load_%s(R, allocator, attr)";
  print "    default: assert(0);\n  }\n}\n\n";

  # store
  print "void attribute_store(int type, Writer *W, const void *value) {\n" .
        "  switch (type) {\n";
  for_each "attribute_store_%s(W, value)";
  print "    default: assert(0);\n  }\n}\n\n";

  # size
  print "size_t attribute_size(int type) {\n" .
        "  switch (type) {\n";
  for_each "sizeof(%s_t)";
  print "    default: assert(0);\n  }\n}\n\n";

  print "#endif\n\n";

}

