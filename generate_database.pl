#!/bin/perl

use strict;
use warnings;

sub database_interface {
  my %types = %{$_[0]};

  for my $t (keys %types) {
    my %i = %{ $types{$t} };

    my $node_types = join "\n", map {
      sprintf "    %s %s_desc;", $_, $_
    } @{ $i{node_types} };

    my $indexes = join "\n", map {
      sprintf "    %s_handler_t %s;", ${$_}[1], ${$_}[0]
    } @{ $i{index_types} };

    print <<EOF;

/*
 *  Database type $t
 */


extern const DatabaseType ${t}_desc;

typedef struct {
  DatabaseType __ancestor;

  struct {
$node_types
  } node_types;

  struct {
$indexes
  } indexes;
} $t;

EOF

  }
}


sub database_implementation {
  my %types = %{$_[0]};
  
  for my $t (keys %types) {
    my %i = %{ $types{$t} };

    # tables
    my $node_types = join ",\n", map {
      sprintf "  &%s_desc", $_
    } @{ $i{node_types} };

    my $update_indexes_table = join ",\n", map {
      sprintf "  (UpdateIndexes)&%s_update_indexes_%s", $t, $_
    } @{ $i{node_types} };

    my $index_table = join ",\n", map {
      sprintf "  (const IndexType)&%s_desc", ${$_}[1]
    } @{ $i{index_types} };

    my $i = 0;

    # node types
    my $init_node_types = join "\n", map {
      sprintf "  D->node_types.%1\$s_desc = %1\$s_desc;\n".
              "  D->node_types.%1\$s_desc.update_indexes = ".
              "(UpdateIndexes)&%2\$s_update_indexes_%1\$s;\n".
              "  D->node_types.%1\$s_desc.id = %3\$i;\n",
              $_, $t, $i++
    } @{ $i{node_types} };

    my $destroy_node_types = join "\n", map {
      sprintf "  node_allocator_destroy(&D->node_types.%1\$s_desc.allocator_info);\n",
              $_
    } @{ $i{node_types} };

    # indexes
    my $init_indexes = join "\n", map {
      sprintf "  %2\$s_desc.desc.context_init(&D->indexes.%1\$s.contex);\n".
              "  D->indexes.%1\$s.callback = %2\$s_desc.desc.callback;\n".
              "  D->indexes.%1\$s.functions = %2\$s_desc.functions;\n",
              @{ $_ }
    } @{ $i{index_types} };

    my $destroy_indexes = join "\n", map {
      sprintf "  %2\$s_desc.desc.context_destroy(&D->indexes.%1\$s.contex);\n",
              @{ $_ }
    } @{ $i{index_types} };

    # update indexes
    my $total_indexes = join "\n", map {
      sprintf "    !D->indexes.%s.callback(&D->indexes.%s.context,\n".
              "        H, event, node) ||",
              ${$_}[0], ${$_}[0]
    } grep { ${$_}[2] eq 'all' } @{ $i{index_types} };


    my $update_indexes = join "\n\n", map {
      my $nt = $_;

      my $indexes = join "\n", map {
        sprintf "    !D->indexes.%s.callback(&D->indexes.%s.context,\n".
                "        H, event, node) ||",
                ${$_}[0], ${$_}[0]
      } grep { ${$_}[2] eq $nt } @{ $i{index_types} };

      sprintf <<EOF, $t, $nt;
static bool %s_update_indexes_%s(Handler *H, 
        enum CallbackEvent event, Node *node) {
  if (
$total_indexes
$indexes
    false
  ) return false;

  return true;
}
EOF
    } @{ $i{node_types} };

    print <<EOF;

/*
 *  Database type $t
 */

static const NodeType *const ${t}_node_types[] = {
$node_types
};

static const UpdateIndexes ${t}_update_indexes_table[] = {
$update_indexes_table
};

static const IndexType *const ${t}_index_table[] = {
$index_table
};

static void ${t}_init($t *D);
static void ${t}_destroy($t *D);

const DatabaseType ${t}_desc = {
  .name = "$t",
  .version = "$i{version}",
  .size = sizeof($t),

  .init = (void(*)(Database*))&${t}_init,
  .destroy = (void(*)(Database*))&${t}_destroy,

  .node_types_count = $#{ $i{node_types} } + 1,
  .node_types = ${t}_node_types,
  .update_indexes = ${t}_update_indexes_table,

  .indexes_count = $#{ $i{index_types} } + 1,
  .indexes = ${t}_indexes_table
};


static void ${t}_init($t *D) {
$init_node_types

$init_indexes
}

static void ${t}_destroy($t *D) {
$destroy_node_types

$destroy_indexes
}

$update_indexes

EOF
  }
}

1;

