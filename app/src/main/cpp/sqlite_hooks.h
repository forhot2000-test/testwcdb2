//
// Created by wang on 2024/11/11.
//

#ifndef WCDB2_SQLITE_HOOKS_H
#define WCDB2_SQLITE_HOOKS_H

extern char *(*ori_sqlite3_expanded_sql)(void *stmt);

extern const char *(*ori_sqlite3_db_filename)(void *db, const char *zDbName);

extern void *(*ori_sqlite3_db_handle)(void *);

extern void (*ori_sqlite3_free)(void *);

void registerHooks();

#endif //WCDB2_SQLITE_HOOKS_H
