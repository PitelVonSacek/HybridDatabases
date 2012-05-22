#ifndef __ALLOCATORS_H__
#define __ALLOCATORS_H__

/**
 * @file
 * @brief Hlavičkový soubor obsahující všechny alokátory.
 *
 * Alokátory, které realizují opožděné uvolňování paměti ho realizují
 * pomocí spojového seznamu. Do něj přidávají paměť k uvolnění, a když
 * velikost seznamu přeroste určitou mez (<tt>.gc_threshold</tt>), tak 
 * zjistí současný čas voláním <tt>.get_time</tt> a projdou a uvolní ty bloky,
 * které je možné uvolnit.
 *
 * @see generic_allocator.h
 * @see node_allocator.h
 * @see page_allocator.h
 * @see vpage_allocator.h
 * @see simple_allocator.h
 */

#include "generic_allocator.h"
#include "node_allocator.h"
#include "page_allocator.h"
#include "vpage_allocator.h"
#include "simple_allocator.h"

#endif

