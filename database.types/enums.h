#ifndef __DATABASE_ENUMS_H__
#define __DATABASE_ENUMS_H__

/**
 * @file
 * Definice konstant a výčtových datových typů.
 */

#define SINGLE_SERVICE_THREAD
// #define LOCKLESS_COMMIT

/**
 * @brief Počet zámků transakční paměti
 *
 * Transakční paměť využívá sdílených zámků a toto
 * makro nastavuje jejich počet. Hodnota
 * by měla být prvočíslo, jinak hrozí, že některé
 * zámky nebou využívany. @see hash_ptr()
 */
#define DB_LOCKS 251

/**
 * Nejmenší datový typ do kterého se vejde konstanta #DB_LOCKS
 */
#define DB_LOCKS_NR_TYPE unsigned char


/**
 * Konstanta udávající kolik uzlů je při dumpu vypsáno
 * na zpracování jednoho požadavku z fronty. @ref service_thread
 */
#define DUMP__NODES_PER_TRANSACTION 5


/**
 * Počet stránek, které udržujě v cache <tt>vpage allocator</tt>.
 * Je-li příliš nízký způsobuje časté volání get_time().
 */
#define DB_VPAGE_ALLOCATOR_CACHE 128

/**
 * Počet položek v seznamu <tt>generic allocatoru</tt> než
 * začne uvolňovat paměť. Je-li příliš nízký způsobuje
 * časté volání get_time().
 */
#define DB_GENERIC_ALLOCATOR_CACHE 4096


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

enum DbFlags {
  DB_READ_ONLY = 1, ///< Databáze je otevřena pouze pro čtení,
                    //   zápis probíhá do /dev/null
  DB_CREATE = 2     ///< Pokud databáze nexistuje, bude vytvořena
};

enum DbService {
    DB_SERVICE__COMMIT = 1,
    DB_SERVICE__SYNC_COMMIT,
    DB_SERVICE__SYNC_ME,
    DB_SERVICE__START_DUMP,
    DB_SERVICE__CREATE_NEW_FILE,
    DB_SERVICE__COLLECT_GARBAGE,
    DB_SERVICE__PAUSE,
    DB_SERVICE__HANDLER_REGISTER,
    DB_SERVICE__HANDLER_UNREGISTER
};

#endif

