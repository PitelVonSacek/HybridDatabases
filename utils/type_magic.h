#ifndef __TYPE_MAGIC_H__
#define __TYPE_MAGIC_H__

/**
 * @file
 * @brief Implementace podpory pro jednoúrovňovou dědičnost.
 *
 * Každý objekt obsahuje atribut @c __ancestor. Pokud je objekt
 * kořenem hierarchie, je tento atribut typu DummyAncestor.
 * Je-li podtřídou má typ nadřazené třídy a je to první atribut
 * v objektu.
 */

#include "static_if.h"

/// Typ označující, že třída je kořenem objektové hierarchie.
typedef struct {} DummyAncestor;

/// Přetypuje objekt na odpovídající abstraktní třídu.
#define typeUncast(object) \
  static_if( \
    types_equal(typeof((object)->__ancestor), DummyAncestor), \
    (object), \
    &((object)->__ancestor) \
  )

#endif

