#include <string.h>
#include <dlfcn.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <future>
#include <xhook.h>

#include "wcdb.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

#define TAG "wcdb.cpp"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, ##__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)

#define STATUS_OK    0
#define STATUS_ERROR 1

static bool g_lib_wcdb_loaded = false;

static int (*ori_sqlite3_open_v2)(const char *filename, void **db, int flags, const char *zVfs);

static int
(*ori_sqlite3_prepare_v2)(void *db, const char *sql, int n, void **stmt, const char **tail);

static int (*ori_sqlite3_exec)(void *, const char *sql, void *callback, void *, char **errmsg);

static const char *(*ori_sqlite3_sql)(void *stmt);

static const char *(*ori_sqlite3_expanded_sql)(void *stmt);

static const char *(*ori_sqlite3_errmsg)(void *db);

static int (*ori_sqlite3_bind_int)(void *, int, int);

static const char *(*ori_sqlite3_db_filename)(void *db, const char *zDbName);

static void *(*ori_sqlite3_db_handle)(void *);

static void *(*ori_sqlite3_update_hook)(void *, void *, void *);

static void (*ori_sqlite3_free)(void *);

static int (*ori_sqlite3_trace_v2)(void *, unsigned uMask, void *, void *pCtx);


void x_register(void *handle, const char *name, void **p) {
    // 使用 dlsym 获取符号地址，并将结果存储在 p 指向的位置  
    *(p) = dlsym(handle, name);
    // 检查 dlsym 是否成功，并打印日志  
//    const char *error = dlerror();
//    if (error != NULL) {
//        ALOGE("[wcdb] dlsym %s failed: %s", name, error);
//    } else {
//        ALOGV("[wcdb] dlsym %s: %p", name, *(p));
//    }
    ALOGV("[wcdb] dlsym %s: %p", name, *(p));
}

int load_lib_wcdb() {
    if (g_lib_wcdb_loaded) {
        return LIB_LOAD_SUCCESS;
    }

    std::string ns = "xxx_libWCDB.so";
    ALOGD("ns=%s", ns.c_str());
    if (ns.length() >= 10) {
        std::string ns2 = ns.substr(ns.length() - 10, 10);
        std::transform(ns2.begin(), ns2.end(), ns2.begin(), ::tolower);
        ALOGD("ns2=%s, compare_result=%d", ns2.c_str(), ns2.compare("libwcdb.so"));
    }

    void *handle = dlopen("libWCDB.so", RTLD_NOW);

    if (!handle) {
        return LIB_LOAD_FAILED;
    }
    ALOGV("[wcdb] dlopen libWCDB.so: %p", handle);

    x_register(handle, "sqlite3_open_v2", reinterpret_cast<void **>(&ori_sqlite3_open_v2));
    x_register(handle, "sqlite3_sql", reinterpret_cast<void **>(&ori_sqlite3_sql));
    x_register(handle, "sqlite3_expanded_sql",
               reinterpret_cast<void **>(&ori_sqlite3_expanded_sql));
    x_register(handle, "sqlite3_prepare_v2", reinterpret_cast<void **>(&ori_sqlite3_prepare_v2));
    x_register(handle, "sqlite3_exec", reinterpret_cast<void **>(&ori_sqlite3_exec));
    x_register(handle, "sqlite3_errmsg", reinterpret_cast<void **>(&ori_sqlite3_errmsg));
    x_register(handle, "sqlite3_bind_int", reinterpret_cast<void **>(&ori_sqlite3_bind_int));
    x_register(handle, "sqlite3_db_filename", reinterpret_cast<void **>(&ori_sqlite3_db_filename));
    x_register(handle, "sqlite3_db_handle", reinterpret_cast<void **>(&ori_sqlite3_db_handle));
    x_register(handle, "sqlite3_update_hook", reinterpret_cast<void **>(&ori_sqlite3_update_hook));
    x_register(handle, "sqlite3_free", reinterpret_cast<void **>(&ori_sqlite3_free));
    x_register(handle, "sqlite3_trace_v2", reinterpret_cast<void **>(&ori_sqlite3_trace_v2));

    g_lib_wcdb_loaded = true;
    return LIB_LOAD_SUCCESS;
}

int x_sqlite3_open_v2(const char *filename, void **db, int flags, const char *zVfs) {
    // ALOGD("open_v2: %s", filename);
    return ori_sqlite3_open_v2(filename, db, flags, zVfs);
}

int x_sqlite3_prepare_v2(void *db, const char *sql, int n, void **stmt, const char **tail) {
    return ori_sqlite3_prepare_v2(db, sql, n, stmt, tail);
}

int x_sqlite3_exec(void *db, const char *sql, void *callback, void *data, char **errmsg) {
    if (!ori_sqlite3_exec) {
        *errmsg = new char[22];
        strcpy(*errmsg, "exec method not found");
        return STATUS_ERROR;
    }
    return ori_sqlite3_exec(db, sql, callback, data, errmsg);
}

const char *x_sqlite3_sql(void *stmt) {
    return ori_sqlite3_sql(stmt);
}

const char *x_sqlite3_expanded_sql(void *stmt) {
    return ori_sqlite3_expanded_sql(stmt);
}

const char *x_sqlite3_db_filename(void *db, const char *zDbName) {
    return ori_sqlite3_db_filename(db, zDbName);
}

void *x_sqlite3_db_handle(void *stmt) {
    return ori_sqlite3_db_handle(stmt);
}

const char *x_sqlite3_errmsg(void *db) {
    return ori_sqlite3_errmsg(db);
}

int x_sqlite3_bind_int(void *stmt, int index, int value) {
    return ori_sqlite3_bind_int(stmt, index, value);
}

void *x_sqlite3_update_hook(void *db, void *callback, void *data) {
    return ori_sqlite3_update_hook(db, callback, data);
}

void x_sqlite3_free(void *ptr) {
    ori_sqlite3_free(ptr);
}


//static void
//(*ori_nativeExecute)(JNIEnv *env, jclass clazz, jlong connectionPtr, jlong statementPtr);
//
//static void k_nativeExecute(JNIEnv *env, jclass clazz, jlong connectionPtr, jlong statementPtr) {
//    ori_nativeExecute(env, clazz, connectionPtr, statementPtr);
//    void *stmt = (void *) ((intptr_t) statementPtr);
//    void *db = x_sqlite3_db_handle(stmt);
//    const char *sql = x_sqlite3_expanded_sql(stmt);
//    const char *filename = x_sqlite3_db_filename(db, "main");
//    ALOGV("[wcdb] handle exec sql: %s (filename=%s)", sql, filename);
//    /*onExecSql(env, sql, filename);*/
//    /*ALOGV("[wcdb] nativeExecute end");*/
//}


static const char *OP_INSERT_OR_REPLACE = "insert/replace";
static const char *OP_UPDATE = "update";
static const char *OP_DELETE = "delete";

/**
 *
 * @param ud        An application-defined user-data pointer. This value is passed from the 3rd arg of x_sqlite3_update_hook function
 * @param op        The type of database update. Possible values are SQLITE_INSERT, SQLITE_UPDATE, and SQLITE_DELETE.
 * @param dbName    The logical name of the database that is being modified. Names include main, temp, or any name passed to ATTACH DATABASE.
 * @param tableName The name of the table that is being modified.
 * @param rowid     The ROWID of the row being modified. In the case of an UPDATE, this is the ROWID value after the modification has taken place.
 */
void wcdbUpdateHookCallback(
        void *ud,
        int op,
        const char *dbName,
        const char *tableName,
        int64_t rowid) {
    char *data = (char *) ud;
    const char *opName = nullptr;
    switch (op) {
        case SQLITE_INSERT:
            // insert or replace
            opName = OP_INSERT_OR_REPLACE;
            break;
        case SQLITE_UPDATE:
            opName = OP_UPDATE;
            break;
        case SQLITE_DELETE:
            opName = OP_DELETE;
            break;
    }
    ALOGD ("update hook: op=%s db=%s table=%s rowid=%d data=%s",
           opName, dbName, tableName, rowid, data);
}

int wcdbExecCallback(void *data, int argc, char **argv, char **azColName) {
    ALOGD ("exec callback: %s", data);
    return 0;
}


#pragma clang diagnostic pop