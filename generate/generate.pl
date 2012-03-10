#!/usr/bin/perl

use strict;
use warnings;

use Parse::RecDescent;
use Data::Dumper;
use File::Basename;

my ($_, $basedir) = fileparse($0);
require $basedir . 'generate_node.pl';
require $basedir . 'generate_index.pl';
require $basedir . 'generate_database.pl';


my $name = $ARGV[0];
my $interface = $ARGV[1];
my $implementation = $ARGV[2];
my $silent = $ARGV[3];

our %node_types = ();
our %database_types = ();
our %index_types = ();
our $int = "";
our $impl = "";

$Parse::RecDescent::skip = '(\s|;)*';

my $grammar = q{
  { my @c = (); my %c = (); my $fnc; my $fnc_t; my $signature  }

  root: ( node_type | index_type | database_type | int | impl | 
          comment | <error> )(s?) /^\\Z/ { 1 }
 
  comment: /#[^\\n]*/
  int: 'Interface' block { $::int = $::int . $item[2] }
  impl: 'Implementation' block { $::impl = $::impl . $item[2] }

  node_type: 'NodeType' name '{' node_attribute(s?) '}'
    { $::node_types{$item{name}} = $item[4] }
  node_attribute: 'Attribute' name ':' type
    { [ $item{name}, $item{type} ] }

  database_type: 'DatabaseType' name '(' identifier ')' '{' (node|index)(s?) '}'
    { $c{version} = $item{identifier};
      $::database_types{$item{name}} = { %c }; %c = () }
  node:  'NodeType'  name 
    { push @{$c{node_types}}, $item{name} }
  index: 'Index' name 'for' identifier ':' type 
    { push @{$c{index_types}}, [ $item{name}, $item{type}, $item{identifier} ] }

  index_type: 'IndexType' name ':' c_type '{' index_i index_d index_u index_e(s?) '}'
    { $c{type} = $item{c_type}; $::index_types{$item{name}} = { %c }; %c = () }
  index_i: 'Init' fnc_or_block { $c{init} = [ $fnc_t, $fnc ] }
  index_d: 'Destroy' fnc_or_block { $c{destroy} = [ $fnc_t, $fnc ] }
  index_u: 'Update' fnc_or_block { $c{update} = [ $fnc_t, $fnc ] }
  index_e: 'Method' name args(?) ':' c_type block
    { push @{$c{methods}}, 
      [ $item{name}, ${$item[3]}[0] || "", $item{c_type}, $item{block} ] }

  args: '(' /[^)]*/ ')' { $item[2] }
  fnc_or_block: function | block
  function: 'function' /\\w+/ { $fnc_t = 'f'; $fnc = $item[2] }
  block: /@\\s*([^@]*)@/ { $fnc_t = 'b'; $fnc = $1; $1 }
  name: identifier   { $item[1] }
  type: identifier   { $item[1] }
  c_type: /(union|struct)\\s*{[^\}]*}|(\\w+\\s*\\**)+/ { $item[1] }
  identifier: /\\w+/ { $item[1] }
};

my $parser = new Parse::RecDescent($grammar) or die "Error\n";
my @input = <STDIN>;
my $input = join(" ", @input);
$parser->root($input) or die "Parse error!";

if (not $silent) {
  print STDERR Dumper(\%node_types);
  print STDERR Dumper(\%database_types);
  print STDERR Dumper(\%index_types);

  print STDERR $int;
  print STDERR $impl;
}


if ($interface) {
  print <<EOF;
#ifndef __GENERATED__${name}__INTERFACE__
#define __GENERATED__${name}__INTERFACE__

/*
 * This file is generated by generate.pl
 * Don't edit this file
 */

$int

EOF

  node_interface(\%node_types);
  index_interface(\%index_types);
  database_interface(\%database_types);

  print "\n#endif\n\n";
}


if ($implementation) { 
  print <<EOF;
/*
 * This file is generated by generate.pl
 * Don't edit this file
 */

$impl

#pragma push_macro("trFail")
#undef trFail
#define trFail return false

EOF

  node_implementation(\%node_types);
  index_implementation(\%index_types);
  database_implementation(\%database_types);

  print "#pragma pop_macro(\"trFail\")\n\n";
}


