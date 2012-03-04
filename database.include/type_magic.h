#ifndef __DATABASE_INCLUDE__TYPE_MAGIC_H__
#define __DATABASE_INCLUDE__TYPE_MAGIC_H__

#define database_close(D) database_close(typeUncast(D))
#define database_dump(D) database_dump(typeUncast(D))
#define database_wait_for_dump(D) database_wait_for_dump(typeUncast(D))
#define database_create_new_file(D) database_create_new_file(typeUncast(D))

#define db_handler_create(D) db_handler_create(typeUncast(D))
#define db_handler_free(H) db_handler_free(typeUncast(H))
#define db_handler_init(D, H) db_handler_init(typeUncast(D), typeUncast(H))
#define db_handler_destroy(H) db_handler_init(typeUncast(H))

#define tr_begin(H) tr_begin(typeUncast(H))
#define tr_abort(H) tr_abort(typeUncast(H))
#define tr_commit(H, commit_type) tr_commit(typeUncast(H), commit_type)
#define tr_hard_abort(H) tr_hard_abort(typeUncast(H))
#define tr_is_main(H) tr_is_main(typeUncast(H))
#define tr_validate(H) tr_validate(typeUncast(H))

#define tr_node_create(H, type) tr_node_create(typeUncast(H), type)
#define tr_node_delete(H, node) tr_node_delete(typeUncast(H), typeUncast(node))

#define tr_node_read(H, node, attr, buffer) \
  tr_node_read(typeUncast(H), typeUncast(node), attr, buffer)
#define tr_node_write(H, node, attr, value) \
  tr_node_write(typeUncast(H), typeUncast(node), attr, value)
#define tr_node_update_indexies(H, node) \
  tr_node_update_indexies(typeUncast(H), typeUncast(node))

#define tr_node_check(H, node) \
  tr_node_check(typeUncast(H), typeUncast(node))

#define tr_node_get_type(node) tr_node_get_type(typeUncast(node))

#endif

