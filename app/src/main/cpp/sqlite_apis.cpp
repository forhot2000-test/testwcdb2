#include <string.h>
#include <dlfcn.h>
#include <android/log.h>
#include <jni.h>
#include <string>
#include <future>
#include <xhook.h>

#include "sqlite_apis.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat"

#define TAG "wcdb.cpp"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG, ##__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)

#define STATUS_OK    0
#define STATUS_ERROR 1

static bool g_libwcdb_loaded = false;

int (*p_sqlite3_open_v2)(const char *filename, void **db, int flags, const char *zVfs);

int (*p_sqlite3_prepare_v2)(void *db, const char *sql, int n, void **stmt, const char **tail);

int (*p_sqlite3_exec)(void *, const char *sql, void *callback, void *, char **errmsg);

int (*p_sqlite3_step)(void *);

const char *(*p_sqlite3_sql)(void *stmt);

char *(*p_sqlite3_expanded_sql)(void *stmt);

void *(*p_sqlite3_update_hook)(void *db, void *callback, void *data);

const char *(*p_sqlite3_db_filename)(void *db, const char *zDbName);

void *(*p_sqlite3_db_handle)(void *);

const char *(*p_sqlite3_errmsg)(void *db);

int (*p_sqlite3_bind_blob)(void *, int, const void *, int n, void(*)(void *));

int (*p_sqlite3_bind_double)(void *, int, double);

int (*p_sqlite3_bind_int)(void *stmt, int index, int value);

int (*p_sqlite3_bind_null)(void *, int);

int (*p_sqlite3_bind_text)(void *, int, const char *, int, void(*)(void *));

int (*p_sqlite3_bind_text16)(void *, int, const void *, int, void(*)(void *));

int (*p_sqlite3_bind_value)(void *, int, const char *);

int (*p_sqlite3_bind_pointer)(void *, int, void *, const char *, void(*)(void *));

int (*p_sqlite3_bind_zeroblob)(void *, int, int n);

int (*p_sqlite3_bind_zeroblob64)(void *, int, uint64_t);

void (*p_sqlite3_free)(void *ptr);

int (*p_sqlite3_trace_v2)(void *, unsigned uMask, void *, void *pCtx);


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
}

int load_libwcdb() {
    if (g_libwcdb_loaded) {
        return LIB_LOAD_SUCCESS;
    }

    void *handle = dlopen("libWCDB.so", RTLD_NOW);

    if (!handle) {
        return LIB_LOAD_FAILED;
    }
    ALOGV("[wcdb] dlopen libWCDB.so: %p", handle);

    x_register(handle, "sqlite3_open_v2", reinterpret_cast<void **>(&p_sqlite3_open_v2));
    x_register(handle, "sqlite3_sql", reinterpret_cast<void **>(&p_sqlite3_sql));
    x_register(handle, "sqlite3_expanded_sql", reinterpret_cast<void **>(&p_sqlite3_expanded_sql));
    x_register(handle, "sqlite3_prepare_v2", reinterpret_cast<void **>(&p_sqlite3_prepare_v2));
    x_register(handle, "sqlite3_exec", reinterpret_cast<void **>(&p_sqlite3_exec));
    x_register(handle, "sqlite3_step", reinterpret_cast<void **>(&p_sqlite3_step));
    x_register(handle, "sqlite3_errmsg", reinterpret_cast<void **>(&p_sqlite3_errmsg));
    x_register(handle, "sqlite3_bind_int", reinterpret_cast<void **>(&p_sqlite3_bind_int));
    x_register(handle, "sqlite3_db_filename", reinterpret_cast<void **>(&p_sqlite3_db_filename));
    x_register(handle, "sqlite3_db_handle", reinterpret_cast<void **>(&p_sqlite3_db_handle));
    x_register(handle, "sqlite3_update_hook", reinterpret_cast<void **>(&p_sqlite3_update_hook));
    x_register(handle, "sqlite3_free", reinterpret_cast<void **>(&p_sqlite3_free));
    x_register(handle, "sqlite3_trace_v2", reinterpret_cast<void **>(&p_sqlite3_trace_v2));

    g_libwcdb_loaded = true;
    return LIB_LOAD_SUCCESS;
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

#pragma clang diagnostic pop