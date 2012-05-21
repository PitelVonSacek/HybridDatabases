#ifndef __STORAGE_H__
#define __STORAGE_H__

/**
 * @file
 * @brief Hlavní hlavičkový soubor knihovny realizující nižší
 *        vrstvu souborového formátu.
 *
 * Popis souborového formátu:
 * @verbatim
   file := root_element*
   root_element := ST_LEN_BEGIN 'length in 8 bytes LE' ST_LEN_END element
                   (ST_CRC 'crc32_c in 4 bytes little-endian')? ST_DOT
   element := array | string
   array := ST_ARRAY_BEGIN element* ST_ARRAY_END
   string := 'encoded length of string' 'string itself'
   @endverbatim
 *
 * <tt>ST_*</tt> jsou řídicí znaky definované níže. Délka řetězce je
 * kódována následovně: K délce přičte konstanta #ST_STRING a výsledek
 * je uložen způsobem ala utf8. Počet jedniček na začátku prvního bajtu
 * udává počet bajtů obsazených číslem - 1. V prvním bajtu jsou pak uloženy
 * nejvyšší bity čísla, zbylé jsou v následujících srovnané little-endian
 * způsobem.
 *
 * Všechna čtecí makra pracují s Readrem <tt>R</tt>, obdobně zápisová
 * s Writerem <tt>W</tt>.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

#include "../utils/basic_utils.h"

#ifndef storageDebug
#define storageDebug(...)
#endif

/// Řídící znaky diskového formátu.
enum {
  ST_ARRAY_BEGIN = 0x01, ///< Začátek pole
  ST_ARRAY_END = 0x03,   ///< Konec pole
  ST_DOT = 0x02,         ///< Konec elementu kořenové úrovně
  ST_LEN_BEGIN = 0x07,   ///< Začátek délky kořenového elementu
  ST_LEN_END = 0x09,     ///< Konec délky kořenového elementu
  ST_CRC = 0x04,         ///< Značka CRC kód
  ST_STRING = 16,        ///< Offset přičítaný k délkám řetězců

  ST_ERROR = 15          ///< Chybový kód vracený funkcí reader_next()
};

/// Hodnoty vracené callback funkcí typu #ReadFunction.
enum {
  ST_READ_FAIL = 0,      ///< Čtení selhalo
  ST_READ_SUCCESS,       ///< Čtení uspělo
  ST_READ_END_OF_FILE    ///< Načteno bylo 0 bajtů, protože bylo dosaženo konce souboru
};


/**************
 *   Reader   *
 **************/

/**
 * Typ callback funkce užívaný strukturou #Reader.
 * @param length Délka načítaných dat
 * @param output Do @a output bude zapsán ukazatel na buffer s daty.
 * @returns Konstantu <tt>ST_READ_*</tt>
 */
typedef int (*ReadFunction)(void *context, size_t length, void **output);


/// Struktura pro čtení dat.
typedef struct {
  void *context;
  ReadFunction read; 
  void (*destroy_context)(void*);

  unsigned char *begin, *ptr, *end;
  int depth;
} Reader;

/// Vytvoří nový #Reader.
Reader *reader_create(ReadFunction read, void *context, 
                      void (*destroy_context)(void*));

/// Uvolní #Reader.
void reader_free(Reader *R);


/// Inicializuje #Reader.
void reader_init(Reader *R, ReadFunction read, void *context, 
                 void (*destroy_context)(void*));

/// @brief Inicializuje #Reader pro čtení ze souboru @a file.
/// @param close @c true pokud má být soubor na konci práce uzavřen
void file_reader_init(Reader *R, FILE *file, bool close);

/// Uvolní používané prostředky.
void reader_destroy(Reader *R);


/**
 * @brief Načte další kořenový element.
 * @returns <tt>ST_READ_*</tt>.
 */
int reader_begin(Reader *R); // returns ST_READ_*

/// @brief Dokončí čtení kořenového elementu.
/// @param checksum @c true je-li pro úspěšné dokončení požadována přítomnost CRC
bool reader_finish(Reader *R, bool checksum);

/// @brief Přeskočí všechny elementy až do konce současného pole.
/// Uzavírací závorku současného pole již nepřeskočí.
bool reader_skip(Reader *R);


/**
 * @brief Vrátí typ následujícího elementu.
 *
 * Možné hodnoty: #ST_ARRAY_BEGIN, #ST_ARRAY_END, #ST_CRC, #ST_DOT a #ST_STRING.
 * Je-li další element řetězec, vždy vrací #ST_STRING bez ohledu
 * na jeho délku.
 */
static inline size_t reader_next(Reader *R);


/// Načte začátek pole.
static inline bool read_array(Reader *R);
/// Načte konec pole.
static inline bool read_array_end(Reader *R);

/**
 * @brief Načte řetězec.
 *
 * Do @a ptr zapíše ukazatel na jeho obsah a do @a length jeho délku.
 * Ukazatel @a ptr zůstává validní až do ukončení čtení současného kořenového
 * elementu.
 */
static inline bool read_string(Reader *R, const void **ptr, size_t *length);

/// @brief Načte řetězec, který bude interpretovat jako celé bezznaménkové číslo.
/// @see write_number()
static inline bool read_number(Reader *R, uint64_t *value);


/// @brief Vrátí současnou pozici v rámci kořenového elementu.
/// @see reader_set_pos()
static inline size_t reader_get_pos(Reader *R);
/// @brief Nastaví současnou pozici v rámci kořenového elementu.
/// @see reader_get_pos()
static inline void reader_set_pos(Reader *R, size_t pos);


/// @brief Makro vykonané jinými makry pro čtení, pokud čtení selže.
/// Určeno k předefinování uživatelem.
#define readFailed goto read_failed

/// Načte další kořenový element.
#define rBegin
#undef rBegin
/// @brief Načte další kořenový element nebo konec souboru.
/// @returns @c true pokud další element existuje.
#define rMayBegin
#undef rMayBegin

/// Makro obalující reader_next().
#define rNext
#undef rNext
/// Makro obalující reader_skip().
#define rSkip
#undef rSkip

/// Makro obalující read_array(). Funguje jako levá závorka k #rArrayEnd.
#define rArray
#undef rArray
/// Makro obalující read_array_end(). Funguje jako pravá závorka k #rArray.
#define rArrayEnd
#undef rArrayEnd

/// Načte číslo.
#define rNumber
#undef rNumber
/// Načte řetězec délky @a len.
#define rRawString(len)
#undef rRawString

/// Načte číslo a ověří, že je rovno @a num.
#define rCheckNumber(num)
#undef rCheckNumber
/// Načte řetězec a ověří, že je shodný s @a string.
#define rCheckString(string)
#undef rCheckString

/// @brief Ukončí čtení kořenového elementu.
/// @see reader_finish()
#define rFinish(checksum)
#undef rFinish



/**************
 *   Writer   *
 **************/

/// Struktura pro zápis dat.
typedef struct {
  unsigned char *real_begin, *begin, *ptr, *end;
  int depth;
} Writer;

/// Vytvoří #Writer.
Writer* writer_create();
/// Smaže #Writer.
void writer_free(Writer *W);

/// Inicializuje #Writer.
void writer_init(Writer* W);
/// Uvolní prostředky.
void writer_destroy(Writer *W);


/// Začne nové pole.
static inline void write_array(Writer *W);
/// Ukončí současné pole.
static inline void write_array_end(Writer *W);


/**
 * @brief Zapíše řetězec délky @a length.
 * @returns Ukazatel na buffer, do něhož musí být obsah řetězce zapsán.
 *          Ukazatel je validní do dalšího volání nějaké zápisové funkce.
 */
static inline void *write_string(Writer *W, uint64_t length);

/**
 * @brief Zapíše celé číslo.
 *
 * Celé číslo je zapsáno jako řetězec. Je uloženo v little-endian tvaru
 * a nejvýznamnější nulové bajty jsou vynechány. (Tedy např. hodnota 0 je
 * zapsána jako prázdný řetězec.)
 */
static inline void write_number(Writer *W, uint64_t number);


/// Zapíše začátek pole. Je levou závorkou k #wArrayEnd.
#define wArray
#undef wArray
/// Zapíše konec pole. Je pravou závorkou k #wArray.
#define wArrayEnd
#undef wArrayEnd


/// Zapíše číslo. @see write_number()
#define wNumber(number)
#undef wNumber

/// Zapíše řetězec délky @a length. Vrací ukazatel na buffer, kam zapsat obsah.
#define wRawString(length)
#undef wRawString
/// Zapíše řetězec @s string. (@a string musí být ukončen '\0', tento znak se nezapisuje.)
#define wString(string)
#undef wString

/// Ukončí kořenový prvek. @a checksum je @c true, má-li se zapsat CRC kód.
#define wFinish(checksum)
#undef wFinish


/// Ukončí kořenový prvek. @a checksum je @c true, má-li se zapsat CRC kód.
void writer_finish(Writer *W, bool checksum);

/// Smaže z bufferu současný nedokončený kořenový element.
void writer_discart(Writer *W);


/// Vrátí ukazatel na vnitřní buffer.
void *writer_ptr(const Writer *W);
/// Vrátí velikost obsazené části vnitřního bufferu.
size_t writer_length(const Writer *W);

/// Struktura popisující pozici ve vnitřním bufferu.
struct WriterPosition {
  size_t begin_offset;
  size_t ptr_offset;
  int depth;
};

/// Vrátí současnou pozici v bufferu.
void writer_get_position(const Writer *W, struct WriterPosition *pos);

/// @brief Nastaví pozici v bufferu. 
/// Může být použito pouze k odčinění zápisů, ne writer_discart().
void writer_set_position(Writer *W, struct WriterPosition *pos);


/// Zapíše @a length bajtů z @a buffer do vnitřního bufferu.
void writer_direct_write(Writer *W, const void *buffer, size_t length);

#define __STORAGE_INLINE_INCLUDE_H__
#include "storage.include/inline.h"

#define __STORAGE_MACROS_INCLUDE_H__
#include "storage.include/macros.h"

#endif

