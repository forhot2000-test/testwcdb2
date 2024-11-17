#include <jni.h>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <regex.h>
#include <android/log.h>
#include <dlfcn.h>
#include <mutex>

#include "sqlite_apis.h"
#include "sqlite_hooks.h"
#include "rakan/rirud.h"

#define TAG "NativeUtil.cpp"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)


enum class Action : uint32_t {
    SET_AUTH_UIN = 1,
    SEND_MESSAGE = 2,
};

RakanSocket &getGlobalRakanSocket();

static JavaVM *gvm;

static const char *OP_INSERT_OR_REPLACE = "insert/replace";
static const char *OP_UPDATE = "update";
static const char *OP_DELETE = "delete";

static void wcdbUpdateHookCallback(void *ud, int op, const char *dbName, const char *tableName,
                                   int64_t rowid);

static int wcdbExecCallback(void *data, int argc, char **argv, char **azColName);

RakanSocket &getGlobalRakanSocket() {
    static RakanSocket socket(5);  // 5 是重试次数
    static std::once_flag initFlag;

    std::call_once(initFlag, []() {
        if (!socket.valid()) {
            __android_log_print(ANDROID_LOG_ERROR, "DB_KOO",
                                "Failed to initialize globalRakanSocket");
        }
    });

    return socket;
}


static void wcdbUpdateHookCallback(void *ud, int op, const char *dbName, const char *tableName,
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
    ALOGD ("[native] update hook: op=%s db=%s table=%s rowid=%d data=%s",
           opName, dbName, tableName, rowid, data);
}

static int wcdbExecCallback(void *data, int argc, char **argv, char **azColName) {
    ALOGD ("[native] exec callback: %s", data);
    return 0;
}


void x_register_2(void *handle, const char *name, void **p) {
    // 使用 dlsym 获取符号地址，并将结果存储在 p 指向的位置
    *p = dlsym(handle, name);
}

void nativeInit(JNIEnv *env, jclass clazz) {

    ALOGD("dlopen libWCDB.so");
    void *handle = dlopen("libWCDB.so", RTLD_NOW);

    ALOGD("dlopen libWCDB_legacy.so");
    dlopen("libWCDB_legacy.so", RTLD_NOW);

    x_register_2(handle, "sqlite3_expanded_sql",
                 reinterpret_cast<void **>(&ori_sqlite3_expanded_sql));
    x_register_2(handle, "sqlite3_db_filename",
                 reinterpret_cast<void **>(&ori_sqlite3_db_filename));
    x_register_2(handle, "sqlite3_db_handle",
                 reinterpret_cast<void **>(&ori_sqlite3_db_handle));
    x_register_2(handle, "sqlite3_free",
                 reinterpret_cast<void **>(&ori_sqlite3_free));

    registerHooks();
}

void nativeTestWcdb(JNIEnv *env, jclass clazz, jstring jfile) {

    int result;

    result = load_libwcdb();
    if (result != LIB_LOAD_SUCCESS) {
        ALOGE("[native] load libWCDB.so failed");
        return;
    }

//    ALOGD("[native] sqlite3_open_v: %p, %p, %p",
//          hook_sqlite3_open_v2, ori_sqlite3_open_v2, p_sqlite3_open_v2);
//    ALOGD("[native] sqlite3_exec: %p, %p, %p",
//          hook_sqlite3_exec, ori_sqlite3_exec, p_sqlite3_exec);
//    ALOGD("[native] sqlite3_step: %p, %p, %p",
//          hook_sqlite3_step, ori_sqlite3_step, p_sqlite3_step);
//    ALOGD("[native] sqlite3_expanded_sql: %p, %p, %p",
//          hook_sqlite3_expanded_sql, ori_sqlite3_expanded_sql, p_sqlite3_expanded_sql);
//    ALOGD("[native] sqlite3_db_filename: %p, %p, %p",
//          hook_sqlite3_db_filename, ori_sqlite3_db_filename, p_sqlite3_db_filename);
//    ALOGD("[native] sqlite3_db_handle: %p, %p, %p",
//          hook_sqlite3_db_handle, ori_sqlite3_db_handle, p_sqlite3_db_handle);

    const char *filename = env->GetStringUTFChars(jfile, nullptr);
    void *db;
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI;


    // ALOGD("[native] filename: %s", filename);
    result = p_sqlite3_open_v2(filename, &db, flags, nullptr);
    if (result != SQLITE_OK) {
        const char *err = p_sqlite3_errmsg(db);
        ALOGE("[native] open db failed: (%d) %s", result, err);
    } else {
        ALOGD("[native] open db success");
    }

//    if (result == SQLITE_OK) {
//        const char *n = p_sqlite3_db_filename(db, "main");
//        // ALOGD("[native] get db filename: %s", n);
//    }


//    // ERROR: bind update hook callback error
//    if (result == SQLITE_OK) {
//        p_sqlite3_update_hook(db, wcdbUpdateHookCallback, (void *) filename);
//    }

//    if (result == SQLITE_OK) {
//        // p_sqlite3_trace(db, 0xffff, callback, ctx);
//    }


    if (result == SQLITE_OK) {
        ALOGD("[native] start exec select ?");
        const char *sql = "select ?";
        void *stmt;

        result = p_sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
        if (result != SQLITE_OK) {
            const char *err = p_sqlite3_errmsg(db);
            ALOGE("[native] prepare statement failed: (%d) %s", result, err);
        }

        if (result == SQLITE_OK) {
            result = p_sqlite3_bind_int(stmt, 1, 10);
            if (result != SQLITE_OK) {
                const char *err = p_sqlite3_errmsg(db);
                ALOGE("[native] bind parameter failed: (%d) %s", result, err);
            }
        }

//        if (result == SQLITE_OK) {
//            const char *s;
//            s = p_sqlite3_sql(stmt);
//            ALOGD("[native] sql: %s", s);
//            s = p_sqlite3_expanded_sql(stmt);
//            ALOGD("[native] expanded sql: %s", s);
//            void *db = p_sqlite3_db_handle(stmt);
//            s = p_sqlite3_db_filename(db, "main");
//            ALOGD("[native] filename from stmt: %s", s);
//        }
    }


    if (result == SQLITE_OK) {
        ALOGD("[native] start exec select 1");
        const char *sql = "select 1";
        char *err;
        result = p_sqlite3_exec(db, sql, (void *) wcdbExecCallback, (void *) sql, &err);
        if (result != SQLITE_OK) {
            ALOGE("[native] exec select 1 failed: (%d) %s", result, err);
            p_sqlite3_free(err);
        }
    }

    if (result == SQLITE_OK) {
        ALOGD("[native] start exec create table test1");
        const char *sql = "create table if not exists test1 (value text)";
        char *err;
        result = p_sqlite3_exec(db, sql, (void *) wcdbExecCallback, (void *) sql, &err);
        if (result != SQLITE_OK) {
            ALOGE("[native] create table test1 failed: (%d) %s", result, err);
            p_sqlite3_free(err);
        }
    }

    // ERROR: sql error
    // if (result == SQLITE_OK) {
    //     ALOGD("[native] start show tables");
    //     const char *sql = ".tables";
    //     char *err;
    //     result = p_sqlite3_exec(db, sql, wcdbExecCallback, (void *) sql, &err);
    //     if (result != SQLITE_OK) {
    //         ALOGE("[native] show tables failed: (%d) %s", result, err);
    //         p_sqlite3_free(err);
    //     } else {
    //         ALOGD("[native] show tables success");
    //     }
    // }

    if (result == SQLITE_OK) {
        ALOGD("[native] start exec insert into test1");
        for (int i = 0; i < 3; ++i) {
            if (result == SQLITE_OK) {
                const char *sql = "insert into test1 (value) values ('hello, world!')";
                char *err;
                result = p_sqlite3_exec(db, sql, (void *) wcdbExecCallback, (void *) sql, &err);
                if (result != SQLITE_OK) {
                    ALOGE("[native] insert into test1 failed: (%d) %s", result, err);
                    p_sqlite3_free(err);
                }
            }
        }
    }


    env->ReleaseStringUTFChars(jfile, filename);
}

void nativeTestSocket(JNIEnv *env, jclass clazz) {
    RakanSocket &socket = getGlobalRakanSocket();
    if (socket.valid()) {
        std::string s_auth_uin = "xxxxxx";
        ALOGD("set auth_uin: %s", s_auth_uin.c_str());
        socket.Write(Action::SET_AUTH_UIN);
        socket.Write(s_auth_uin);

        for (int i = 0; i < 10; ++i) {
            std::string msg = "message_" + std::to_string(i);
            ALOGD("send message: %s", msg.c_str());
            socket.Write(Action::SEND_MESSAGE);
            socket.Write(msg);
        }
    }
}

void nativeTestStringCompare(JNIEnv *env, jclass clazz) {
    const char *filename = "xxx_libWCDB_legacy.so";
    std::string ns(filename);
    std::transform(ns.begin(), ns.end(), ns.begin(), ::tolower);
    ALOGD("ns=%s", ns.c_str());
    if (ns.length() >= 10 && ns.compare(ns.length() - 10, 10, "libwcdb.so") == 0) {
        ALOGD("%s is libwcdb", filename);
    } else if (ns.length() >= 17 && ns.compare(ns.length() - 17, 17, "libwcdb_legacy.so") == 0) {
        ALOGD("%s is legacy libwcdb", filename);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    gvm = vm;
    ALOGD("gvm: %p", gvm);

    JNIEnv *env;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // 如果无法获取JNIEnv，返回错误码
    }

    ALOGD("env: %p", env);

    jclass aClass = env->FindClass("com/example/wcdb2/NativeUtil"); // 查找Java类
    if (aClass == NULL) {
        return JNI_ERR; // 类找不到，返回错误码
    }

    // 定义方法签名和本地方法实现
    JNINativeMethod methods[] = {
            {"nativeInit",              "()V",                   (void *) nativeInit},
            {"nativeTestSocket",        "()V",                   (void *) nativeTestSocket},
            {"nativeTestStringCompare", "()V",                   (void *) nativeTestStringCompare},
            {"nativeTestWcdb",          "(Ljava/lang/String;)V", (void *) nativeTestWcdb},
    };

    // 动态注册本地方法
    jint result = env->RegisterNatives(aClass, methods, sizeof(methods) / sizeof(methods[0]));
    if (result != JNI_OK) {
        return result; // 注册失败，返回错误码
    }

    return JNI_VERSION_1_6; // 返回使用的JNI版本
}
