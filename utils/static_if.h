#ifndef __STATIC_IF_H__
#define __STATIC_IF_H__

/**
 * @file
 * @brief Drobnosti pro vyhodnocování výrazů při překladu.
 *
 * Inspirováno zvrhlostmi z knihovny Boost.
 */


/**
 * Vrátí datový typ, v němž je ulžené číslo @c val.
 * Proměnná totho typu má nulovou velikost.
 */
#define StaticInt(val) \
  struct { struct { char value[val]; } _static_int[0]; }
/// Z proměnné typu StaticInt() vytáhne hodnotu.
#define StaticGetInt(var) sizeof((var)._static_int->value)


/**
 * Typový if.
 *
 * V závyslosti na pravdivostní hodnotě podmíky, vrací typ A nebo B.
 */
#define StaticIf(cond, A, B) \
  typeof(static_if(cond, *((typeof(A)*)0), *((typeof(B)*)0)))


/// Assert vyhodnocený při překladu.
#define STATIC_ASSERT(x) switch (0) { case 0: case (x): ; }


/**
 * Konstrukce if vyhodnocená při překladu.
 *
 * Na rozdíl od ?: může mít každá z větví jiný typ.
 */
#define static_if(cond, a, b) __builtin_choose_expr((cond), (a), (b)) 


/// Vrací @c true, pokud jsou typy shodné.
#define types_equal(t1, t2) __builtin_types_compatible_p(t1, t2)

#endif

