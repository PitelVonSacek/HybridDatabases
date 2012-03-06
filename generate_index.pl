#!/bin/perl

use strict;
use warnings;

sub index_interface {
  my %types = %{$_[0]};
 
  for my $t (keys %types) {
    my %i = %{ $types{$t} };

    my $methods = "";

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
void ${t}_ctx_init(${t}_context_t *context) {
$init_ptr;
}
EOF
      $init_ptr = "(void(*)(void*))&${t}_ctx_init";
    }

    # destroy
    my $destroy = "";
    my $destroy_ptr = ${$i{destroy}}[1];
    if (${$i{destroy}}[0] eq 'b') {
      $destroy = <<EOF;
void ${t}_ctx_destroy(${t}_context_t *context) {
$destroy_ptr;
}
EOF
      $destroy_ptr = "(void(*)(void*))&${t}_ctx_destroy";
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
  tr_failed:
  return false;
}
EOF
      $update_ptr = "(bool(*)(void*,Handler*,enum CallbackEvent,Node*))&${t}_ctx_update";
    }



    my $methods = "";
    my $method_ptrs = "";

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
