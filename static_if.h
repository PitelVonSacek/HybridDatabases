#ifndef STATIC_IF_H__
#define STATIC_IF_H__

#define static_if(cond, a, b) __builtin_choose_expr((cond), (a), (b)) 

#define types_equal(t1, t2) __builtin_types_compatible_p(t1, t2)

#define type_switch(expr, opts) ({ \
    __label__ _type_switch_break_label; \
    typeof(expr) _type_switch_dummy; \
    \
    opts \
    \
    _type_switch_break_label: ; \
  })

#define type_case(type, action) static_if( \
  types_equal(typeof(_type_switch_dummy), type), \
  ({ action; goto _type_switch_break_label; }), \
  (void)0);


#endif

