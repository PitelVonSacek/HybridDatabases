#ifndef __DATABASE_INTERFACE_H__
#define __DATABASE_INTERFACE_H__

/**
 * @file
 * @brief Hlavní hlavičkový soubor
 *
 * Funkce / makra, které nejsou dokumentovány zde ani nikde v database.types/,
 * jsou považovány za vnitřní detaily a uživatel by na nich neměl záviset.
 *
 * Všechny zde uvedené funkce (není-li uvedeno jinak) jsou v souboru
 * @ref database.include/type_magic.h obaleny makry tak, aby kromě typů, se kterými
 * jsou zde deklarovány, braly za parametry i jejich podtřídy.
 *
 * V souboru @ref database.types/enums.h jsou definovány výčtové typy
 * a konfigurační volby.
 *
 * Definice typů viz:
 * @ref database.types/attributes.h
 * @ref database.types/database.h
 * @ref database.types/handle.h
 * @ref database.types/index.h
 * @ref database.types/node.h
 * @ref database.types/node_type.h
 *
 * Implementace inline funkcí a marek se nachází v @ref database.include/macros.h
 * a @ref database.include/inline.h.
 *
 * Implementace inline funkcí atributů v @ref attributes/attributes.inline.h
 */

#include "database.types/attributes.h"
#include "database.types/database.h"
#include "database.types/enums.h"
#include "database.types/handle.h"
#include "database.types/index.h"
#include "database.types/node.h"
#include "database.types/node_type.h"
#include "utils/utils.h"



/*************************
 *  Database functions   *
 *************************/

/** @{ @name Databáze */

/**
 * @brief Vyrobí novou instanci třídy Database.
 *
 * Tato funkce není polymorfní jako ostatní, ale existuje zde
 * makro dbCreate().
 *
 * @param type   Deskriptor typu databáze.
 * @param file   Cesta k souboru obsahujícímu databázi bez koncovky.
 *               (Schéma se tedy bude jmenovat @a file.schema a datové
 *               soubory @a file.N pro N = 1, 2, ...)
 * @param flags  Konfigurace databáze
 * @returns V případě úspěchu vrací instanci Database. Pokud se při načítání
 *          databáze vyskytly chyby, které nebyly fatální, může nastavit
 *          příznak DB_READ_ONLY. V případě fatální chyby vrací 0.
 */
Database *database_create(const DatabaseType *type, const char *file, enum DbFlags flags);

/**
 * @brief Uzavře databázi a uvolní s ní asociované zdroje.
 * @remark Uživatel musí zajistit, že při volání této funkce již neběží
 *         žádná transakce. Pokud to nezajistí je výsledné chování nedefinované.
 * @param D Databáze, která ma být uzavřena.
 */
void database_close(Database *D);


/**
 * @brief Vrátí aktuálně nastavené příznaky.
 * @param D Databáze.
 * @returns Příznaky databáze D.
 */
static inline enum DbFlags database_get_flags(Database *D);


/**
 * @brief Vypíše databázi na disk.
 *
 * Založí nový datový soubor a do něj začne zapisovat obsah databáze.
 * Zápis probíhá s servisním vláknu, funkce se vrátí v okamžiku,
 * kdy je nový soubor založen. Probíhající transakce jsou zapisovány
 * do nového souboru. Ve chvíli, kdy je vypsání databáze dokončeno,
 * je možné smazat starší datové soubory bez ztráty dat.
 *
 * @param D Databáze.
 * @returns Chybov status.
 */
enum DbError database_dump(Database* D);

/**
 * @brief Počká na dokončení probíhajícího výpisu databáze na disk.
 *
 * Pokud vypsání neprobíhá, vrátí se okamžitě.
 *
 * @param D Databáze.
 */
void database_wait_for_dump(Database *D);


/**
 * @brief Založí nový datový soubor.
 *
 * Transakce se začnou zapisovat do nového souboru, ale
 * stará data do něj zapsána nejsou, takže starší datové soubory
 * není možné smazat.
 *
 * @param D Databáze.
 * @returns Chybový status.
 */
enum DbError database_create_new_file(Database *D);


/**
 * @brief Pokusí se uvolnit přebytečnou paměť.
 *
 * Pošle zprávu servisnímu vláknu, aby uvolnilo přebytečnou paměť.
 * Vrátí se až po dokončení úklidu.
 *
 * @param D Databáze.
 */
void database_collect_garbage(Database *D);


/**
 * @brief Pozastaví servisní vlákno.
 *
 * @warning Tato funkce může při nevhodném užití způsobit dead-lock.
 *
 * Pošle zprávu servisnímu vláknu, které se po jejím přijetí zastaví
 * dokud neobdrží odpovídající volání database_resume_service_thread().
 *
 * @param D Databáze.
 */
void database_pause_service_thread(Database *D);

/**
 * @brief Probudí servisní vlákno.
 *
 * Probudí servisní vlákno zastavené funkcí database_pause_service_thread().
 *
 * @param D Databáze.
 */
void database_resume_service_thread(Database *D);


/**
 * @brief Vrací čas (v sekundách), kdy se periodicky provede fsync().
 *
 * Je-li tato funkce vypnuta vrací 0.0.
 *
 */
double database_get_sync_period(Database *D);


/**
 * @brief Nastaví čas (v sekundách), kdy se periodicky provede fsync().
 *
 * Hodnota 0.0 vypne tuto funkci. Hodnoty menší 0.001 jsou zaokrouhleny na 0.
 */
void database_set_sync_period(Database *D, double period);


/**
 * @brief Vytvoří novou databázi.
 *
 * Makro nad funkcí database_create(). Na rozdíl od ní nebere jako první
 * parametr deskriptor vytvářené databáze ale její typ.
 *
 * @param Type  Typ vytvářené databáze.
 * @param file  Název souboru obsahujícího databázi (bez přípony)
 * @param flags Příznaky.
 * @returns Vytvořená databáze. Návratový typ je Type*, ne Database*.
*/
#define dbCreate(Type, file, flags)
#undef dbCreate

/** @} */



/*************************
 *   Handle functions   *
 *************************/

/** @{ @name Handle */

/**
 * @brief Vytvoří nový handle pro databázi D.
 * @param D Databáze.
 * @returns Handle pro databázi D.
 */
Handle *db_handle_create(Database *D);

/**
 * @brief Uvolní handle vytvořený pomocí do_handle_create().
 * @param H Handle.
 */
void db_handle_free(Handle *H);


/**
 * @brief Inicializuje handle H jako handle pro databázi D.
 * @param D Databáze.
 * @param H Handle.
 * @returns H
 */
Handle *db_handle_init(Database *D, Handle *H);

/**
 * @brief Uvolní handle vyjma struktury H samotné.
 * @param H Handle.
 */
void db_handle_destroy(Handle *H);


/**
 * @brief Makro kolem funkce db_handle_create(), které
 *        má správný návratový typ.
 * @param D Databáze.
 * @returns Handle pro databázi D. Návratový typ závisí na typu databáze.
 */
#define dbHandleCreate(D)
#undef dbHandleCreate

/** @} */



/*************************
 * Transaction functions *
 *************************/

/** @{ @name Transakce */

/**
 * @brief Započne transakci.
 * @param H Handle.
 */
static inline void tr_begin(Handle *H);

/**
 * @brief Zruší aktuální transakci.
 * @param H Handle.
 */
static inline void tr_abort(Handle *H);

/**
 * @brief Pokusí se provést commit aktuální transakce.
 * @param H Handle.
 * @param commit_type Typ commitu.
 * @returns true pokud se commit zdařil, false pokud došlo ke kolizi
 *          a následnému rollbacku.
 */
static inline bool tr_commit(Handle *H, enum CommitType commit_type);

/**
 * @brief Zruší aktuální transakci i všechny transakce v nichž je vnořená.
 * @param H Handle.
 */
static inline void tr_hard_abort(Handle *H);


/**
 * @brief Zjistí, zda jsou současná transakce vnořena v jiné.
 * @returns true pokud je současná transakce hlavní (tedy není vnořená).
 */
static inline bool tr_is_main(Handle *H);


/**
 * @brief Zkontroluje zda data přečtená současnou transakcí nebyla
 *        jinou transakcí změněna.
 * @returns true pokud data nebyla změněna.
 */
static inline bool tr_validate(Handle *H);


/**
 * @brief Započne transakci.
 *
 * Spolu s makrem trCommit() funguje jako závorky. Navíc oproti funkci
 * tr_begin() se při selhání z důvodu kolize (ne když uživatel
 * zavolá #trAbort) transakce sama restartuje. Restart se provede po náhodné
 * prodlevě, přičemž tato prodleva s každým dalším pokusem roste.
 *
 * Stejně jako jiná makra tvaru tr*, předpokládá existenci handlu
 * jménem H, pokud je potřeba použít handle jiného jména, lze
 * za název makra připojit _ a handle předat jako první parametr.
 *
 */
#define trBegin
#undef trBegin

/**
 * @brief Pokusí se provést commit transakce zahájené pomocí #trBegin.
 *
 * Funguje jako uzavírací závorka pro #trBegin.
 *
 * @param commit_type Typ commitu.
 * @param nested_restart_action Nepovinný parametr. Kousek kódu, který
 *   se provede pokud tato transakce selže a zároveň je vnořena v jiné
 *   transakci (a tedy je třeba toto selhání propagovat). Jelikož
 *   implementace nepoužívá výjimky a neexistuje tedy obecná cesta, jak
 *   propagovat selhání, je defaultní akcí vypsání chyby a zabití aplikace.
 */
#define trCommit(commit_type, nested_restart_action)
#undef trCommit

/**
 * @brief Zruší aktuální transakci.
 *
 * Transakce není automaticky restartována.
 */
#define trAbort
#undef trAbort


/**
 * @brief Pomocné makro. Je vykonáno jinými tr* makry, pokud ta potřebují
 *        propagovat neúspěch transakce.
 *
 * Defaultní hodnota je goto tr_failed, ale uživatelský kód si ho může
 * předefinovat jak potřebuje.
 */
#define trFail goto tr_failed

/** @} */



/*************************
 *     Node functions    *
 *************************/

/** @{ @name Uzly */

/**
 * @brief Vytvoří nový uzel.
 *
 * Pokud daný typ uzlu není součástí databáze, k níž patří handle @a H,
 * je chování této funkce nedefinované.
 *
 * @param H        Handle, určuje transakci v jejímž rámci se má uzel vytvořit.
 * @param type     Deskriptor typu uzlu.
 *
 * @returns Nový uzel, nebo 0 pokud vytvoření uzlu selže (z důvodu kolize
 *          s jinou transakcí).
 */
Node *tr_node_create(Handle *H, NodeType *type);

/**
 * @brief Pokusí se smazat uzel.
 *
 * Smazání uzlu může selhat jak z důvodu kolize s jinou transakcí,
 * tak proto, že na uzel odkazují jiné uzly.
 *
 * @param H        Handle.
 * @param node     Uzel, který má být smazán.
 *
 * @returns true pokud se uzel povedlo smazat, jinak false.
 */
bool tr_node_delete(Handle *H, Node *node);


/**
 * @brief Přečte honotu atributu.
 *
 * Oproti makrům pomalá a nemá typovou informaci. Pokud je to možné
 * požijte makro trRead().
 *
 * @param H        Handle.
 * @param node     Uzel jehož atribut se bude číst.
 * @param attr     Index atributu.
 * @param buffer   Buffer, kam bude zapsána hodnota atributu.
 *
 * @returns true v případě úspěchu. Vrátí-li false je třeba restartovat
 *          transakci.
 */
bool tr_node_read(Handle *H, Node *node, int attr, void *buffer);

/**
 * @brief Zapíše hodnotu do atributu.
 *
 * Oproti makrům pomalá a nemá typovou informaci. Pokud je to možné
 * požijte makro trWrite().
 *
 * @warning Na modifikovaný uzel je před commit nutné zavolat funkci
 * tr_node_update_indexes(), jinak budou indexy v nekonzistentním stavu.
 *
 * @param H        Handle.
 * @param node     Uzel jehož atribut se bude měnit.
 * @param attr     Index modifikovaného atributu.
 * @param value Hodnota, která se do atributu zapíše.
 *
 * @returns true v případě úspěchu. Vrátí-li false je třeba restartovat
 *          transakci.
 */
bool tr_node_write(Handle *H, Node *node, int attr, const void *value);


/**
 * @brief Ověří zda uzel nebyl modifikován jinou transakcí.
 *
 * @param H        Handle.
 * @param node     Uzel.
 *
 * @returns true pokud nebyl modifikován.
 */
static inline bool tr_node_check(Handle *H, Node *node);


/**
 * @brief Přetypuje node na typ @a Type
 *
 * @param Type     Node nebo jeho podtřída.
 * @param node     Uzel, který má být přetypován
 *
 * @returns @a node přetypovaný na @a Type, pokud @a node
 *          není typu @a Type, tak 0
 */
#define nodeCast(Type, node)
#undef nodeCast


/**
 * @brief Přečte hodnotu atributu.
 *
 * Pokud se čtení nezdaří, provede makro #trFail. @see trRead()
 *
 * @param H        Handle.
 * @param node     Uzel.
 * @param AttrName Jméno atributu.
 *
 * @returns hodnota atributu @a AttrName uzlu @a node.
 */
#define trRead_(H, node, AttrName)
#undef trRead_
/// Odpovídá trRead_(H, node, AttrName).
#define trRead(node, AttrName)
#undef trRead


/**
 * @brief Přečte hodnotu atributu, ale neprovede kontrolu zda je přečtená
 *        hodnota validní.
 *
 * Je rychlejší než trRead_(), ale nekontroluje zda je načtená hodnota validní.
 * Protože načítání hodnoty není nutně atomické, může být načtená hodnota
 * různé od jakékoli hodnoty, která kdy byla do daného atributu zapsána.
 * Speciálně je-li načtená hodnota ukazatel, je třeba ji ověřit před
 * dereferencováním.
 *
 * Pokud se čtení nezdaří, provede makro #trFail. @see trUncheckedRead()
 *
 * @param H        Handle.
 * @param node     Uzel.
 * @param AttrName Jméno atributu.
 *
 * @returns hodnota atributu @a AttrName uzlu @a node.
 */
#define trUncheckedRead_(H, node, AttrName)
#undef trUncheckedRead_
/// Odpovídá trUncheckedRead_(H, node, AttrName).
#define trUncheckedRead(node, AttrName)
#undef trUncheckedRead


/**
 * @brief Přečte hodnotu atributu, ale nekontroluje validitu hodnoty ani daný
 *        uzel nepřidá do readsetu.
 *
 * Předpokládá, že příslušný uzel již je součástí readsetu nebo je zamknut současnou
 * transakcí.
 *
 * Rychlejší než trUncheckedRead_().
 *
 * @warning Nesprávné použití může vést k nekonzistenci databáze, protože může
 *          zmařit detekci kolize transakcí.
 */
#define trInternalRead_(H, node, AttrName)
#undef trInternalRead_
/// Odpovídá trRead_(H, node, AttrName).
#define trInternalRead(node, AttrName)
#undef trInternalRead


/**
 * @brief Zapíše hodnotu atributu.
 *
 * @warning
 * Po modifikaci uzlu je třeba na něj před commitem zavolat
 * trUpdateIndexes_(), jinak budou indexy v nekonzistentním stavu.
 *
 * Pokud se zápis nezdaří, provede makro #trFail. @see trWrite()
 *
 * @param H        Handle.
 * @param node     Uzel.
 * @param AttrName Jméno atributu.
 * @param value    Nová hodnota.
 */
#define trWrite_(H, node, AttrName, value)
#undef trWrite_
/// Odpovídá trWrite_(H, node, AttrName).
#define trWrite(node, AttrName, value)
#undef trWrite


/**
 * @brief Zapíše hodnotu atributu, předpokládá, že transakce již drží potřebný zámek.
 *
 * Na rozdíl od trWrite_() se nepokouší zabrat zámek potřebný k zápisu do daného
 * uzlu, ale předpokládá, že ho transakce již vlastní.
 *
 * Rychlejší něž trWrite().
 *
 * @warning Používejte obezřetně, špatné použití může způsobit nekonzistenci databáze.
 */
#define trInternalWrite_(H, node, AttrName, value)
#undef trInternalWrite_
/// Odpovídá trInternalWrite_(H, node, AttrName).
#define trInternalWrite(node, AttrName, value)
#undef trInternalWrite


/**
 * @brief Ověří zda uzel nebyl modifikován jinou transakcí.
 *
 * Byl-li modifikován provede makro #trFail. @see trCheck()
 *
 * @param H        Handle.
 * @param node     Uzel.
 */
#define trCheck_(H, node)
#undef trCheck_
/// Odpovídá trCheck_(H, node).
#define trCheck(node)
#undef trCheck


/**
 * @brief Vytvoří uzel.
 *
 * Selže-li tvorba uzlu, provede makro #trFail.
 *
 * @see trNodeCreate() @see tr_node_create()
 *
 * @param H        Handle.
 * @param NodeType Typ vytvářeného uzlu.
 *
 * @returns Vytvořený uzel.
 */
#define trNodeCreate_(H, NodeType)
#undef trNodeCreate_
/// Odpovídá trNodeCreate_(H, NodeType).
#define trNodeCreate(NodeType)
#undef trNodeCreate


/**
 * @brief Smaže uzel.
 *
 * Selže-li mazání (z jakéhokoli důvodu) uzlu, provede makro #trFail.
 *
 * @see trNodeDelete() @see tr_node_delete()
 *
 * @param H        Handle.
 * @param node     Mazaný uzel.
 *
 * @returns Vytvořený uzel.
 */
#define trNodeDelete_(H, node)
#undef trNodeDelete_
/// Odpovídá trNodeDelete_(H, node).
#define trNodeDelete(node)
#undef trNodeDelete


/**
 * @brief Zamkne pro zápis uzel @a node.
 *
 * Po provedení toho makra je bezpečné k příslušnému uzlu přistupovat
 * pomocí trInternalRead() a trInternalWrite().
 */
#define trLock_(H, node)
#undef trLock_
/// Odpovídá trLock_(H, node).
#define trLock(node)
#undef trLock

/** @} */



/*************************
 *         Indexy        *
 *************************/

/** @{ @name Indexy */

/**
 * @brief Alokuje transakční paměť.
 *
 * Pouze pro použití v indexech.
 *
 * Funkce pro alokaci paměti v rámci indexů.
 * Pro uvolnění použijte funkci tr_memory_free().
 *
 * Pokud potřebujete alokovat paměť v @c Init metodě
 * indexu použijte tr_memory_early_alloc(),
 * pro uvolnění paměti v metodě @c Destroy použijte
 * tr_memory_late_free(). Obě mají jako první parametr #GenericAllocator.
 * Ten je parametrem jak metody @c Init, tak @c Destroy. V obou případech
 * pod jménem @c allocator.
 */
static inline void *tr_memory_alloc(Handle *H, size_t size);


/**
 * @brief Uvolní transakční paměť.
 * @see tr_memory_alloc()
 */
static inline void tr_memory_free(Handle *H, void *ptr);


/**
 * @brief Alokuje transakční paměť při inicializaci indexu.
 * @see tr_memory_alloc()
 */
static inline void *tr_memory_early_alloc(struct GenericAllocator *A, size_t size);


/**
 * @brief Uvolní transakční paměť při prušení indexu.
 * @see tr_memory_alloc()
 */
static inline void tr_memory_late_free(struct GenericAllocator *A, void *ptr);


/**
 * @brief Upraví indexy s ohledem na modifikaci uzlu @a node.
 *
 * @param H        Handle.
 * @param node     Modifikovaný uzel.
 *
 * @returns true v případě úspěchu. Vrátí-li false je třeba restartovat
 *          transakci.
 */
static inline bool tr_node_update_indexes(Handle *H, Node *node);


/**
 * @brief Aktualizuje indexy.
 *
 * Pokud se aktualizace nezdaří, provede makro #trFail. @see trUpdateIndexes()
 *
 * @param H        Handle.
 * @param node     Uzel, který byl modifikován.
 */
#define trUpdateIndexes_(H, node)
#undef trUpdateIndexes_
/// Odpovídá trUpdateIndexes_(H, node)
#define trUpdateIndexes(node)
#undef trUpdateIndexes


/**
 * @brief Zavolá metodu indexu.
 *
 * Dojde-li v průběhu volání ke kolizi transakce, provede makro #trFail.
 * @see trIndex()
 *
 * @param H        Handle.
 * @param index    Jméno indexu.
 * @param Method   Metoda indexu, která se má provést.
 * @param ...      Další parametry pro metodu indexu.
 *
 * @returns Návratová hodnota (i její typ) závisí na konkrétní metodě
 *          daného indexu.
 */
#define trIndex_(H, index, Method, ...)
#undef trIndex_
/// Odpovídá trIndex_(H, index, Method, ...).
#define trIndex(index, Method, ...)
#undef trIndex


/**
 * @brief Načte hodnotu z místa v paměti @a object @a Attr.
 *
 * Toto makro je určeno pro práci s pamětí v rámci implementace indexů.
 * Použití tohoto makra mimo metody indexů, může způsobit nedefinované
 * chovaní. Taktéž jeho použití na jinou paměť než kontext odpovídajícího
 * indexu a paměť alokovanou pomocí tr_memory_alloc().
 *
 * Rozdělení na objekt a atribut místo přímého čtení hodnoty ukazatele
 * je nutné pro lepší zamykání. Zamyká se vždy dle hodnoty @a object
 * bez ohledu na @a Attr. (Z toho plyne, že přístup k jedněm datům
 * pomocí různých ukazatelů @a object je chyba.)
 *
 * Pokud se čtení nezdaří, provede makro #trFail. @see trMemoryRead()
 *
 * @param H        Handle.
 * @param object   Ukazatel.
 * @param Attr     Designátor nad ukazatelem @a object obsahující právě jednu
 *                 dereferenci.
 *
 * @return Hodnota @a object @a Attr.
 */
#define trMemoryRead_(H, object, Attr)
#undef trMemoryRead_
/// Odpovídá trMemoryRead_(H, object, Attribute).
#define trMemoryRead(object, Attribute)
#undef trMemoryRead


/**
 * @brief Načte hodnotu z místa v paměti @a object @a Attr, ale nekontroluje
 *        správnost načtené hodnoty.
 *
 * @see trMemoryRead_()
 * @see trRead_()
 * @see trUncheckedRead()
 */
#define trMemoryUncheckedRead_(H, object, Attr)
#undef trMemoryUncheckedRead_
/// Odpovídá trMemoryUncheckedRead_(H, object, Attribute).
#define trMemoryUncheckedRead(object, Attribute)
#undef trMemoryUncheckedRead


/**
 * @brief Načte hodnotu z místa v paměti @a ptr, ale nekontroluje
 *        správnost načtené hodnoty ani ji nepřidá do readsetu.
 *
 * @warning Nebezpečné.
 *
 * @see trMemoryRead_()
 * @see trRead_()
 * @see trUncheckedRead_()
 * @see trInternalRead_()
 */
#define trMemoryInternalRead_(H, ptr)
#undef trMemoryInternalRead_
/// Odpovídá trMemoryInternalRead_(H, ptr).
#define trMemoryInternalRead(ptr)
#undef trMemoryInternalRead


/**
 * @brief Zapíše hodnotu @a value do @a object @a Attr.
 *
 * @see trMemoryRead_()
 * @see trRead_()
 * @see trWrite_()
 */
#define trMemoryWrite_(H, object, Attr, value)
#undef trMemoryWrite_
/// Odpovídá trMemoryWrite_(H, object, Attribute, value).
#define trMemoryWrite(object, Attribute, value)
#undef trMemoryWrite


/**
 * @brief Zapíše hodnotu @a value do @a ptr, předpokládá, že transakce již
 *        vlastní příslušný zámek.
 *
 * @warning Nebezpečné.
 *
 * @see trMemoryRead_()
 * @see trRead_()
 * @see trWrite_()
 * @see trInternalWrite_()
 */
#define trMemoryInternalWrite_(H, ptr, value)
#undef trMemoryInternalWrite_
/// Odpovídá trMemoryInternalWrite_(H, ptr, value).
#define trMemoryInternalWrite(ptr, value)
#undef trMemoryInternalWrite


/**
 * @brief Zamkne paměť @a ptr pro zápis.
 *
 * Je-li zapnuta volba #INPLACE_INDEX_LOCKS, musí být @a ptr ukazatel
 * na strukturu, která obsahuje prvek @c lock typu #IndexLock.
 *
 * Po provedení toho makra je bezpečné k příslušné paměti přistupovat
 * pomocí trMemoryInternalRead() a trMemoryInternalWrite().
 */
#define trIndexLock_(H, ptr)
#undef trIndexLock_
/// Odpovídá trLock_(H, ptr).
#define trIndexLock(ptr)
#undef trIndexLock

/** @} */


/*************************
 *  Attribute functions  *
 *************************/

/** @{ @name Atributy */

/**
 * @brief Zjistí počet atributů.
 * @param node_type Deskriptor typu uzlu.
 * @returns Počet atributů uzlů typu @a node_type.
 */
int tr_attr_count(NodeType *node_type);

/**
 * @brief Zjistí jméno atributu.
 * @param node_type Deskriptor typu uzlu.
 * @param index     Index atributu.
 * @returns Název atributu na indexu @a index v uzlech typu @a node_type.
 */
const char *tr_attr_get_name(NodeType *node_type, int index);

/**
 * @brief Zjistí index atributu. Složitost O(# atributů v uzlu typu @a node_type).
 * @param node_type Deskriptor typu uzlu.
 * @param attr      Jméno atributu.
 * @return Index atributu @a attr v uzlech typu @a node_type.
 */
int tr_attr_get_index(NodeType *node_type, const char *attr);

/**
 * @brief Zjistí typ uzlu.
 * @param node Uzel.
 * @returns Deskriptor typu uzlu @a node.
 */
const NodeType *tr_node_get_type(Node *node);

/**
 * @brief Zjistí typ atributu.
 * @param node_type Deskriptor typu uzlu.
 * @param index     Index atributu.
 * @returns Typ atributu s indexem @a index v uzlech typu @a node_type.
 */
const int tr_attr_get_type(NodeType *node_type, int index);

/** @} */



#include "database.include/macros.h"
#include "database.include/inline.h"
// must be AFTER inline.h
#include "database.include/type_magic.h"
#include "attributes/attributes.inline.h"

#endif

