#!/bin/perl

use strict;
use warnings;

sub node_interface {
  my %node_types = %{ $_[0] };

  for my $node_type (keys %node_types) {
    my $attrs = join "\n", map {
      sprintf "  %s_t %s;", ${$_}[1], ${$_}[0]
    } @{ $node_types{$node_type} };

    my $i = 0;
    my $attr_indexes = join "\n", map {
      sprintf "  StaticInt(%i) _internal_%s_index;", $i++, ${$_}[0]
    } @{ $node_types{$node_type} };

    print <<EOF;
typedef struct {
  Node __ancestor;
  
$attrs

$attr_indexes
} $node_type;

extern const NodeType ${node_type}_desc;

EOF
  }
}


sub node_implementation {
  sub node_type_mk_fce {
    my ($fce, $args_long, $args, $node_type, $attrs_) = @_;
  
    my $attrs = join "\n", map {
      sprintf "%s", "  attribute_${fce}_${$_}[1]($args &node->${$_}[0]);"
    } @{$attrs_};

    printf "%s", <<EOF
static void ${node_type}_$fce($args_long) {
$attrs
}

EOF
  };

  my %node_types = %{ $_[0] };
 
  for my $node_type (keys %node_types) {
    my @attrs = @{$node_types{$node_type}};

    print "static const struct NodeAttribute ${node_type}_desc_attributes".
          "[$#attrs + 2] = {\n";

    my $i = 0;
    for my $attr (@attrs) {
      print <<EOF;
  { 
    .name = "${$attr}[0]",
    .type = ${$attr}[1],
    .index = $i,
    .offset = __builtin_offsetof($node_type, ${$attr}[0])
  },
EOF
      $i++;
    }
 
    print "  {}\n};\n\n";
 
    # load
    print "static bool ${node_type}_load(Reader *R, struct " .
          "GenericAllocatorInfo *allocator, $node_type *node) {\n" .
          "  if (\n";
 
    for my $attr (@attrs) {
      print "    !attribute_load_${$attr}[1](R, allocator, &node->${$attr}[0]) ||\n";
    }
     
    print "    false\n  ) return false;\n" .
          "  return true;\n}\n\n";

    # store
    print "static void ${node_type}_store(Writer *W, $node_type *node) {\n";
    for my $attr (@attrs) {
      print "  attribute_store_${$attr}[1](W, &node->${$attr}[0].value);\n";
    }
    print "}\n\n";
    # init_pointers
    print "static bool ${node_type}_init_pointers(IdToNode *nodes, $node_type *node) {\n";
 
    for my $attr (@attrs) {
      print "  node->${$attr}[0].value = node->${$attr}[0].id ? \n" .
            "    ndict_at(nodes, node->${$attr}[0].id) : 0;\n" .
            "  if (node->${$attr}[0].value) node->${$attr}[0].value->ref_count++;\n"
        if ${$attr}[1] eq "Pointer";
    }
     
    print "}\n\n";
 
    # destroy_pointers
    print "static bool ${node_type}_destroy_pointers(Handler *H, $node_type *node) {\n" .
          "  Node *zero = 0;\n" .
          "  if (\n";
 
    for my $attr (@attrs) {
      print "    !attribute_write_Pointer(H, &node->${$attr}[0], &zero) ||\n" 
        if ${$attr}[1] eq "Pointer";
    }
   
    print "    false\n  ) return false;\n  return true;\n}\n\n";


    node_type_mk_fce("init", "$node_type *node", "", 
                     $node_type, \@attrs);
  
    node_type_mk_fce("destroy", "struct GenericAllocatorInfo *a, $node_type *node," .
                     "uint64_t end_time", "a, end_time,", $node_type, \@attrs);

    print <<EOF;
const NodeType ${node_type}_desc = {
  .name = "$node_type",

  .load = (bool(*)(Reader*, struct GenericAllocatorInfo*, Node*))&${node_type}_load,
  .store = (void(*)(Writer*, Node*))&${node_type}_store,

  .init_pointers = (void(*)(IdToNode*, Node*))&${node_type}_init_pointers,
  .destroy_pointers = (int(*)(Handler*, Node*))&${node_type}_destroy_pointers,

  .init = (void(*)(Node*))&${node_type}_init,
  .destroy = (void(*)(struct GenericAllocatorInfo*, Node*, uint64_t))&${node_type}_destroy,

  { sizeof($node_type), 0, 0 },

  .update_indexes = 0,

  .size = sizeof($node_type),
  .extra_space = 0,
  .id = 0,

  .attributes_count = $#attrs + 1,
  .attributes = ${node_type}_desc_attributes
};

EOF
  }
}


1;

