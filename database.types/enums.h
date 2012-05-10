#ifndef __DATABASE_ENUMS_H__
#define __DATABASE_ENUMS_H__

#include "../config.h"
#include "../utils/basic_utils.h"

/**
 * @file
 * Definice konstant a výčtových datových typů.
 */


/**
 * @brief Vysune validaci readsetu mimo kritickou sekci.
 *
 * Za cenu vyšší mezivláknové komunikace vysune validaci
 * readsetu mimo kritickou sekci. Může být vhodné především
 * v kombinaci s #INPLACE_NODE_LOCKS a #INPLACE_INDEX_LOCKS,
 * jelikož v těchto případech není velikost readsetu omezena
 * konstantou.
 */
#ifndef FAST_COMMIT
#define FAST_COMMIT 0
#endif


/**
 * @brief Přesune většinu práce pryč ze servisního vlákna.
 *
 * Přesune zpracování transakčního logu ze servisního vlákna
 * do vlákna, které provádí commit. To zvýší výkon při práci
 * z více vláken.
 */
#ifndef SIMPLE_SERVICE_THREAD
#define SIMPLE_SERVICE_THREAD 0
#endif


/**
 * @brief Přesune dealokaci objektů mimo zpracování transakčního logu.
 *
 * Pokud není zapnuto #SIMPLE_SERVICE_THREAD je tato volba ignorována.
 */
#ifndef DB_DEFER_DEALLOC
#define DB_DEFER_DEALLOC 1
#endif


/**
 * @brief Zámky uzlů jsou umístěny v nich místo v centrální tabulce.
 */
#ifndef INPLACE_NODE_LOCKS
#define INPLACE_NODE_LOCKS 1
#endif


/**
 * @brief Zámky indexů jsou umístěny v nich místo v centrální tabulce.
 *
 * Je-li tato volba zapnuta, měly by všechny indexy obsahovat položku
 * @c lock type @c IndexLock. (Je-li tato volba vypnuta, je @c IndexLock
 * definován jako prázná struktura, takže nezabírá žádné místo.)
 */
#ifndef INPLACE_INDEX_LOCKS
#define INPLACE_INDEX_LOCKS 1
#endif


/**
 * @brief Počet zámků transakční paměti
 *
 * Transakční paměť využívá sdílených zámků a toto
 * makro nastavuje jejich počet. Hodnota
 * by měla být prvočíslo, jinak hrozí, že některé
 * zámky nebou využívany. @see hash_ptr()
 *
 * Nepoužito pokud #INPLACE_NODE_LOCKS && #INPLACE_INDEX_LOCKS
 */
#ifndef DB_LOCKS
#define DB_LOCKS 251
#endif

/**
 * Nejmenší datový typ do kterého se vejde konstanta #DB_LOCKS
 *
 * Nepoužito pokud #INPLACE_NODE_LOCKS && #INPLACE_INDEX_LOCKS
 */
#ifndef DB_LOCKS_NR_TYPE
#define DB_LOCKS_NR_TYPE \
  StaticIf(DB_LOCKS < 0x100, unsigned char, unsigned short)
#endif


/**
 * Maximální počet nevyřízených transakcí, které mohou náležet
 * jednomu handleru.
 */
#ifndef BD_MAX_PENDING_TRANSACTIONS_PER_HANDLER
#define BD_MAX_PENDING_TRANSACTIONS_PER_HANDLER 128
#endif

/**
 * Konstanta udávající kolik uzlů je při dumpu vypsáno
 * na zpracování jednoho požadavku z fronty. @ref service_thread
 */
#ifndef DUMP__NODES_PER_TRANSACTION
#define DUMP__NODES_PER_TRANSACTION 5
#endif


/**
 * Počet stránek, které udržujě v cache <tt>vpage allocator</tt>.
 * Je-li příliš nízký způsobuje časté volání get_time().
 */
#ifndef DB_VPAGE_ALLOCATOR_CACHE
#define DB_VPAGE_ALLOCATOR_CACHE 128
#endif

/**
 * Počet položek v seznamu <tt>generic allocatoru</tt> než
 * začne uvolňovat paměť. Je-li příliš nízký způsobuje
 * časté volání get_time().
 */
#ifndef DB_GENERIC_ALLOCATOR_CACHE
#define DB_GENERIC_ALLOCATOR_CACHE 4096
#endif


/**
 * Perioda, kdy má být volán fsync().
 * @see database_set_sync_period()
 */
#ifndef DB_SYNC_PERIOD
#define DB_SYNC_PERIOD 5
#endif


/**
 * Velikost výstupního bufferu.
 */
#ifndef DB_WRITE_BUFFER_SIZE
#define DB_WRITE_BUFFER_SIZE 4096
#endif


/**
 * Maximalní počet objektů výstupní fronty, které alokátor cachuje.
 *
 * Tento alokátor je společný pro všechny databáze v systému.
 */
#ifndef DB_OUTPUT_LIST_CACHE
#define DB_OUTPUT_LIST_CACHE 320
#endif


/**
 * @brief Typ commitu.
 * Určuje zda se funkce tr_commit() (či makro trCommit()) vrátí
 * hned po zařazení transakce do fronty či až po jejím zapsání
 * na disk (a provedení fflush()).
 */
enum CommitType {
  CT_SYNC = 1,        ///< Vrátí se až po zapsání na disk.
  CT_ASYNC = 0,       ///< Vrátí se hned, pokud vnořená transakce nebyla synchonní.
  CT_FORCE_ASYNC = -1 ///< Vždy se vrátí ihned bez ohledu na vnořené transakce.
};


/**
 * Druh události, na kteru má index reagovat.
 */
enum CallbackEvent {
  CBE_NODE_CREATED,  ///< Uzel byl vytvořen.
  CBE_NODE_MODIFIED, ///< Uzel byl modifikován.
  CBE_NODE_DELETED,  ///< Uzel byl smázán. (V době volání jště existuje i s obsahem.)
  CBE_NODE_LOADED    ///< Uzel byl načten z disku, volá se při načítní databáze.
};


/**
 * Typ události v transakčním logu. @see #LogItem
 */
enum {
  LI_TYPE_RAW,                 ///< Zápis do transakční paměti.
  LI_TYPE_NODE_MODIFY,         ///< Změna hodnoty atributu nějakého uzlu.

  LI_TYPE_ATOMIC_RAW,          ///< Atomický zápis do transakční paměti. Nepoužíváno.
  LI_TYPE_ATOMIC_NODE_MODIFY,  ///< Atomická změna hodnoty atributu nějakého uzlu.
                               ///  Nepoužíváno.

  LI_TYPE_NODE_ALLOC,          ///< Vytvoření uzlu.
  LI_TYPE_NODE_DELETE,         ///< Smazání uzlu.
  LI_TYPE_MEMORY_ALLOC,        ///< Alokace transakční paměti.
  LI_TYPE_MEMORY_DELETE        ///< Uvolnění transakční paměti.
};


/**
 * Chybový status
 */
enum DbError {
  DB_SUCCESS = 0,                   ///< Úspěch.
  DB_ERROR__DUMP_RUNNING,           ///< Operace selhala, protože běží výpis databáze 
                                    ///  na disk.
  DB_ERROR__CANNOT_CREATE_NEW_FILE  ///< Operace selhala, protože nelze vytvořit nový
                                    ///  datový soubor.
};


/**
 * Příznaky databáze
 */
enum DbFlags {
  DB_READ_ONLY = 1, ///< Databáze je otevřena pouze pro čtení,
                    ///   zápis probíhá do /dev/null.
  DB_CREATE = 2     ///< Pokud databáze nexistuje, bude vytvořena
};


/**
 * Typy zpráv pro servisní vlákno.
 */
enum DbService {
  DB_SERVICE__COMMIT = 1,
  DB_SERVICE__SYNC_COMMIT,
  DB_SERVICE__SYNC_ME,
  DB_SERVICE__START_DUMP,
  DB_SERVICE__CREATE_NEW_FILE,
  DB_SERVICE__COLLECT_GARBAGE,
  DB_SERVICE__PAUSE,
  DB_DUMMY_SERVICE
};

#endif

