#ifndef STATIC_IF_H__
#define STATIC_IF_H__

#define STATIC_ASSERT(x) switch (0) { case 0: case (x): ; }

#define static_if(cond, a, b) __builtin_choose_expr((cond), (a), (b)) 

#define types_equal(t1, t2) __builtin_types_compatible_p(t1, t2)

#define type_switch(expr, opt) ({ \
    typeof(expr) _type_switch_dummy; \
    \
    opt; \
  })

#define type_case(type, action, else_) static_if( \
  types_equal(typeof(_type_switch_dummy), type), \
  action, else_)


#endif

