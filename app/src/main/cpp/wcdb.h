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

int x_sqlite3_open_v2(const char *filename, void **db, int flags, const char *zVfs);

int x_sqlite3_prepare_v2(void *db, const char *sql, int n, void **stmt, const char **tail);

int x_sqlite3_exec(void *, const char *sql, void *callback, void *, char **errmsg);

const char *x_sqlite3_sql(void *stmt);

const char *x_sqlite3_expanded_sql(void *stmt);

void *x_sqlite3_update_hook(void *db, void *callback, void *data);

const char *x_sqlite3_db_filename(void *db, const char *zDbName);

void *x_sqlite3_db_handle(void *);

const char *x_sqlite3_errmsg(void *db);

int x_sqlite3_bind_blob(void *, int, const void *, int n, void(*)(void *));

int x_sqlite3_bind_double(void *, int, double);

int x_sqlite3_bind_int(void *stmt, int index, int value);

int x_sqlite3_bind_null(void *, int);

int x_sqlite3_bind_text(void *, int, const char *, int, void(*)(void *));

int x_sqlite3_bind_text16(void *, int, const void *, int, void(*)(void *));

int x_sqlite3_bind_value(void *, int, const char *);

int x_sqlite3_bind_pointer(void *, int, void *, const char *, void(*)(void *));

int x_sqlite3_bind_zeroblob(void *, int, int n);

int x_sqlite3_bind_zeroblob64(void *, int, uint64_t);

void x_sqlite3_free(void *ptr);

int x_sqlite3_trace_v2(void *, unsigned uMask, void *, void *pCtx);

int load_lib_wcdb();

void wcdbUpdateHookCallback(void *ud, int op, const char *dbName, const char *tableName,
                            int64_t rowid);

int wcdbExecCallback(void *data, int argc, char **argv, char **azColName);

#endif //APP2_WCDB_H
