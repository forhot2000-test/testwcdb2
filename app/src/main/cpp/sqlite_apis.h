//
// Created by wang on 2022/9/5.
//

#ifndef APP2_WCDB_H
#define APP2_WCDB_H


#define SQLITE_OK           0   /* Successful result */

#define SQLITE_DELETE                9   /* Table Name      NULL            */
#define SQLITE_INSERT               18   /* Table Name      NULL            */
#define SQLITE_UPDATE               23   /* Table Name      Column Name     */

#define SQLITE_OPEN_READWRITE        0x00000002  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_CREATE           0x00000004  /* Ok for sqlite3_open_v2() */
#define SQLITE_OPEN_URI              0x00000040  /* Ok for sqlite3_open_v2() */


const int LIB_LOAD_SUCCESS = 0;
const int LIB_LOAD_FAILED = 1;

extern int (*p_sqlite3_open_v2)(const char *filename, void **db, int flags, const char *zVfs);

extern int (*p_sqlite3_prepare_v2)(void *db, const char *sql, int n, void **stmt,
                                   const char **tail);

extern int (*p_sqlite3_exec)(void *, const char *sql, void *callback, void *, char **errmsg);

extern int(*p_sqlite3_step)(void *);

extern const char *(*p_sqlite3_sql)(void *stmt);

extern char *(*p_sqlite3_expanded_sql)(void *stmt);

extern void *(*p_sqlite3_update_hook)(void *db, void *callback, void *data);

extern const char *(*p_sqlite3_db_filename)(void *db, const char *zDbName);

extern void *(*p_sqlite3_db_handle)(void *);

extern const char *(*p_sqlite3_errmsg)(void *db);

extern int (*p_sqlite3_bind_blob)(void *, int, const void *, int n, void(*)(void *));

extern int (*p_sqlite3_bind_double)(void *, int, double);

extern int (*p_sqlite3_bind_int)(void *stmt, int index, int value);

extern int (*p_sqlite3_bind_null)(void *, int);

extern int (*p_sqlite3_bind_text)(void *, int, const char *, int, void(*)(void *));

extern int (*p_sqlite3_bind_text16)(void *, int, const void *, int, void(*)(void *));

extern int (*p_sqlite3_bind_value)(void *, int, const char *);

extern int (*p_sqlite3_bind_pointer)(void *, int, void *, const char *, void(*)(void *));

extern int (*p_sqlite3_bind_zeroblob)(void *, int, int n);

extern int (*p_sqlite3_bind_zeroblob64)(void *, int, uint64_t);

extern void (*p_sqlite3_free)(void *ptr);

extern int (*p_sqlite3_trace_v2)(void *, unsigned uMask, void *, void *pCtx);

int load_libwcdb();

#endif //APP2_WCDB_H
