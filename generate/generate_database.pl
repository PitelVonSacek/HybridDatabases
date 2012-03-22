#!/bin/perl

use strict;
use warnings;

sub database_interface {
  my %types = %{$_[0]};

  for my $t (keys %types) {
    my %i = %{ $types{$t} };

    my $node_types = join "\n", map {
      sprintf "    NodeType %s_desc;", $_, $_
    } @{ $i{node_types} };

    my $indexes = join "\n", map {
      sprintf "    %s_handler_t %s;", ${$_}[1], ${$_}[0]
    } @{ $i{index_types} };

    print <<EOF;

/*
 *  Database type $t
 */


extern const DatabaseType ${t}_desc;

typedef union {
  Database *database;
  struct ${t}_ *my_database;
  Handler __ancestor;
} ${t}_handler_t;

typedef struct ${t}_ {
  Database __ancestor;

  struct {
$node_types
  } node_types;

  struct {
$indexes
  } indexes;

  ${t}_handler_t __dummy_handler[0];
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
      sprintf "  &%s_desc.desc", ${$_}[1]
    } @{ $i{index_types} };

    my $i = 0;

    # node types
    $i = 0;
    my $layout_check = join "\n", map {
      sprintf "  STATIC_ASSERT(offsetof(%s, __ancestor.node_types[%i]) ==\n" .
              "                offsetof(%s, node_types.%s_desc));",
              $t, $i++, $t, $_
    } @{ $i{node_types} };

    # indexes
    my $init_indexes = join "\n", map {
      sprintf "  %2\$s_desc.desc.context_init(&D->indexes.%1\$s.context, D->tm_allocator);\n".
              "  D->indexes.%1\$s.callback = %2\$s_desc.desc.callback;\n".
              "  D->indexes.%1\$s.functions = %2\$s_desc.functions;\n",
              @{ $_ }
    } @{ $i{index_types} };

    my $destroy_indexes = join "\n", map {
      sprintf "  %2\$s_desc.desc.context_destroy(&D->indexes.%1\$s.context, D->tm_allcator);\n",
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

      sprintf <<EOF, $t, $nt, $t;
static bool %s_update_indexes_%s(Handler *H, 
        enum CallbackEvent event, Node *node) {
  %s *D __attribute__((unused)) = (void*)H->database;
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

$update_indexes

static const NodeType *const ${t}_node_types[] = {
$node_types
};

static const UpdateIndexes ${t}_update_indexes_table[] = {
$update_indexes_table
};

static const IndexType *const ${t}_index_table[] = {
$index_table
};

static void ${t}_init_indexes($t *D);
static void ${t}_destroy_indexes($t *D);

const DatabaseType ${t}_desc = {
  .name = "$t",
  .version = "$i{version}",
  .size = sizeof($t),

  .init_indexes = (void(*)(Database*))&${t}_init_indexes,
  .destroy_indexes = (void(*)(Database*))&${t}_destroy_indexes,

  .node_types_count = $#{ $i{node_types} } + 1,
  .node_types = ${t}_node_types,
  .update_indexes = ${t}_update_indexes_table,

  .indexes_count = $#{ $i{index_types} } + 1,
  .indexes = ${t}_index_table
};


static void ${t}_init_indexes($t *D) {
$init_indexes
}

static void ${t}_destroy_indexes($t *D) {
$destroy_indexes
}

static __attribute__((unused))
void ${t}_struct_layout_check() {
$layout_check
}

EOF
  }
}

1;

