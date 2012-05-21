#ifndef __BIT_ARRAY_H__
#define __BIT_ARRAY_H__
/**
 * @file
 * @brief Definice datové struktury bitové pole.
 *
 * Implementace inspirována bitovým polem v knihovně libucw
 * (http://www.ucw.cz/libucw/).
 */

#include "basic_utils.h"

/// Prázdná struktura, slouží pouze pro typovou kontrolu.
typedef struct {} IsBitArray;

/**
 * Definuje typ bitového pole dané velikosti.
 * Velikost pole je uložena v elementu .size (tento element
 * má nulovou velikost, viz #StaticInt).
 */
#define BitArray(size_) \
  struct { \
    uint32_t data[(size_ + 31) / 32]; \
    StaticInt(size_) size; \
    IsBitArray is_bit_array; \
  }

/// Inicializuje bitové pole (nastaví všechny bity na 0).
#define bit_array_init(array) \
  do { \
    typeof(&*(array)) __array = (array); \
    STATIC_ASSERT(types_equal(typeof(__array->is_bit_array), IsBitArray)); \
    memset(__array->data, 0, sizeof(__array->data)); \
  } while (0)

/// Destruktor, nedělá nic.
#define bit_array_destroy(array) do {} while (0)

/// Vynuluje celé pole
#define bit_array_erase(array) bit_array_init(array)


/// Zjistí velikost bitového pole
#define bit_array_size(array) StaticGetInt((array)->size)
/// Počet slov typu #uint32_t, v nichž je pole uloženo.
#define _bit_array_words(a) (sizeof((a)->data) / sizeof((a)->data[0]))


/// Nastaví bit @c i na 1.
#define bit_array_set(array, i) \
  do { \
    STATIC_ASSERT(types_equal(typeof((array)->is_bit_array), IsBitArray)); \
    ((array)->data)[ i / 32 ] |= (1 << (i % 32)); \
  } while (0)

/// Vrátí hodnotu bitu @c i.
#define bit_array_get(array, i) \
  ({ \
    STATIC_ASSERT(types_equal(typeof((array)->is_bit_array), IsBitArray)); \
    ((array)->data)[ i / 32 ] & (1 << (i % 32)); \
  })

/// Zneguje hodnotu bitu @c i.
#define bit_array_flip(array, i) \
  do { \
    STATIC_ASSERT(types_equal(typeof((array)->is_bit_array), IsBitArray)); \
    ((array)->data)[ i / 32 ] ^= (1 << (i % 32)); \
  } while (0)


/// Spolu s #bitArrayForEnd iteruje přes všechny bity pole s hodnotou 1.
#define bitArrayFor(index, array) \
  do { \
    __label__ _bit_array_break, _bit_array_continue; \
    typeof(&*(array)) __array = (array); \
    STATIC_ASSERT(types_equal(typeof(__array->is_bit_array), IsBitArray)); \
    for (unsigned _ba_word = 0; \
         _ba_word < _bit_array_words(__array); \
         _ba_word++) \
      for (unsigned _ba_bit = 0; _ba_bit < 32; _ba_bit++) { \
        if (__array->data[_ba_word] & (1 << _ba_bit)) { \
          const unsigned index = 32 * _ba_word + _ba_bit;

#define bitArrayForEnd \
         } \
         _bit_array_continue: __attribute__((unused)); \
       } \
    _bit_array_break: __attribute__((unused)); \
  } while (0)

#define bitArrayContinue goto _bit_array_continue
#define bitArrayBreak goto _bit_array_break

#endif

