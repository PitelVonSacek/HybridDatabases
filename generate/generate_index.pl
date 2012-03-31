#!/bin/perl

use strict;
use warnings;

sub index_interface {
  my %types = %{$_[0]};
 
  for my $t (keys %types) {
    my %i = %{ $types{$t} };

    my $methods = join "\n", map {
      my ($name, $args, $ret, $body) = @{$_};
      $args &&= ", $args";
      "  bool (*$name)(const ${t}_context_t *context, Handler *H, $ret *value $args);\n".
      "  $ret ${name}_return_t[0];"
    } @{$i{methods}};

    print <<EOF;

typedef $i{type} ${t}_context_t;

typedef struct {
$methods
} ${t}_functions;

typedef struct {
  IndexType desc;
  ${t}_functions functions;
} ${t}_desc_t;

typedef struct {
  ${t}_context_t context;
  bool (*callback)(void *context, Handler *H, enum CallbackEvent event, Node *node);
  ${t}_functions functions;
} ${t}_handler_t;

extern const ${t}_desc_t ${t}_desc;

EOF
  }

}


sub index_implementation {
  my %types = %{$_[0]};
  
  for my $t (keys %types) {
    my %i = %{ $types{$t} };

    # init
    my $init = "";
    my $init_ptr = ${$i{init}}[1];
    if (${$i{init}}[0] eq 'b') {
      $init = <<EOF;
void ${t}_ctx_init(${t}_context_t *context,
                   struct GenericAllocator *allocator) {
$init_ptr;
#if INPLACE_INDEX_LOCKS
  l_init(&context->lock);
#endif
}
EOF
      $init_ptr = "(void(*)(void*, struct GenericAllocator*))&${t}_ctx_init";
    }

    # destroy
    my $destroy = "";
    my $destroy_ptr = ${$i{destroy}}[1];
    if (${$i{destroy}}[0] eq 'b') {
      $destroy = <<EOF;
void ${t}_ctx_destroy(${t}_context_t *context,
                      struct GenericAllocator *allocator) {
$destroy_ptr;
}
EOF
      $destroy_ptr = "(void(*)(void*, struct GenericAllocator*))&${t}_ctx_destroy";
    }

    # update
    my $update = "";
    my $update_ptr = ${$i{update}}[1];
    if (${$i{update}}[0] eq 'b') {
      $update = <<EOF;
bool ${t}_ctx_update(${t}_context_t *context, Handler *H, 
        enum CallbackEvent event, Node *node) {
$update_ptr;
  return true;
  tr_failed: __attribute__((unused));
  return false;
}
EOF
      $update_ptr = "(bool(*)(void*,Handler*,enum CallbackEvent,Node*))&${t}_ctx_update";
    }

    my $methods = join "", map {
      my ($name, $args, $ret, $body) = @{$_};
      $args &&= ", $args";
      <<EOF;
static bool ${t}_method_$name(const ${t}_context_t *context, Handler *H, 
        $ret *value $args) {
  $body;

  return true;
}

EOF
    } @{$i{methods}};

    my $method_ptrs = join ",\n", map {
      my ($name, $args, $ret, $body) = @{$_};
      "    &${t}_method_$name, {}"
    } @{$i{methods}};

    print <<EOF;

$init
$destroy
$update

$methods

const ${t}_desc_t ${t}_desc = {
  {
    .name = "$t",
    .callback = $update_ptr,

    .context_size = sizeof(${t}_context_t),
    .node_context_size = 0,

    .context_init = $init_ptr,
    .context_destroy = $destroy_ptr
  },

  {
$method_ptrs
  }
};

EOF
  }

}

1;

