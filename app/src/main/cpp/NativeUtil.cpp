#include <jni.h>
#include <string>
#include <cstdio>
#include <unistd.h>
#include <regex.h>
#include <android/log.h>
#include <xhook.h>
#include <dlfcn.h>

#include "wcdb.h"

#define TAG "NativeUtil.cpp"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)


static JavaVM *gvm;

static const char *OP_INSERT_OR_REPLACE = "insert/replace";
static const char *OP_UPDATE = "update";
static const char *OP_DELETE = "delete";

static void wcdbUpdateHookCallback(void *ud, int op, const char *dbName, const char *tableName,
                                   int64_t rowid);

static int wcdbExecCallback(void *data, int argc, char **argv, char **azColName);

static int (*ori_sqlite3_open_v2)(const char *filename, void **db, int flags,
                                  const char *zVfs) = nullptr;

static int (*ori_sqlite3_exec)(void *db, const char *sql, void *callback, void *,
                               char **errmsg) = nullptr;

static int (*ori_sqlite3_step)(void *stmt) = nullptr;

static const char *(*ori_sqlite3_expanded_sql)(void *stmt);

static const char *(*ori_sqlite3_db_filename)(void *db, const char *zDbName);

static void *(*ori_sqlite3_db_handle)(void *);


static int hook_sqlite3_open_v2(const char *filename, void **db, int flags, const char *zVfs) {
    ALOGD("hook_open_v2: filename=%s", filename);
    return ori_sqlite3_open_v2(filename, db, flags, zVfs);
}

static int hook_sqlite3_exec(void *db, const char *sql, void *callback, void *data,
                             char **errmsg) {
    if (ori_sqlite3_db_filename) {
        const char *filename = ori_sqlite3_db_filename(db, "main");
        ALOGD("hook_exec: sql=%s, filename=%s", sql, filename);
    }

    return ori_sqlite3_exec(db, sql, callback, data, errmsg);
}

static std::string k_MicroMsg("/com.tencent.mm/MicroMsg/");
static std::string k_EnMicroMsgDb("/EnMicroMsg.db");
static std::string k_WxFileIndexDb("/WxFileIndex.db");
static std::string k_AuthUin;

static int get_auth_uin(std::string s_filename) {
    int a = s_filename.find(k_MicroMsg);
    if (a < 0) {
        // ALOGD("[auth_uin] Not in MicroMsg, %s", s_filename.c_str());
        return 1;
    }
    a += k_MicroMsg.length();

    int b = s_filename.find(k_EnMicroMsgDb);
    if (b < 0) {
        b = s_filename.find(k_WxFileIndexDb);
    }
    if (b < 0) {
        // ALOGD("[auth_uin] Not EnMicroMsg.db or WxFileIndex.db, %s", s_filename.c_str());
        return 2;
    }

    std::string auth_uin = s_filename.substr(a, b - a);
    if (auth_uin != k_AuthUin) {
        k_AuthUin = auth_uin;
        return 0;
    }

    // ALOGD("[auth_uin] auth_uin is not changed.");
    return -1;
}

static int get_relative_filename(std::string s_filename, std::string &s_relative_filename) {
    int a = s_filename.find(k_MicroMsg);
    if (a < 0) {
        // ALOGD("[auth_uin] Not in MicroMsg, %s", s_filename.c_str());
        return 1;
    }
    a += k_MicroMsg.length();

    s_relative_filename = s_filename.substr(a, s_filename.size() - a);
    return 0;
}

static int hook_sqlite3_step(void *stmt) {
    if (ori_sqlite3_expanded_sql) {

        void *db = ori_sqlite3_db_handle(stmt);
        const char *sql = ori_sqlite3_expanded_sql(stmt);
        const char *filename = ori_sqlite3_db_filename(db, "main");

        if (sql) {
            std::string s_sql(sql);
            std::string s_filename(filename);
            std::string s_relative_filename;

            if (get_auth_uin(s_filename) == 0) {
                std::string msg = "1:" + k_AuthUin;
                ALOGD("auth_uin: %s", msg.c_str());
            }

            if (get_relative_filename(s_filename, s_relative_filename) == 0) {
                std::string msg = "2:" + s_relative_filename + ":\n" + sql;
                ALOGD("hook_step: %s", msg.c_str());
            }
        }
    }

    return ori_sqlite3_step(stmt);
}

static const char *hook_sqlite3_expanded_sql(void *stmt) {
    return ori_sqlite3_expanded_sql(stmt);
}

static const char *hook_sqlite3_db_filename(void *db, const char *name) {
    return ori_sqlite3_db_filename(db, name);
}

static void *hook_sqlite3_db_handle(void *stmt) {
    return ori_sqlite3_db_handle(stmt);
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


extern "C" JNIEXPORT void JNICALL
Java_com_example_wcdb2_NativeUtil_nativeInit(JNIEnv *env, jclass clazz) {

    ALOGD("dlopen libWCDB.so");
    dlopen("libWCDB.so", RTLD_NOW);

    xhook_clear();
    xhook_enable_debug(1);
    xhook_register("/data/.*\\.so$", "sqlite3_open_v2",
                   reinterpret_cast<void *>(hook_sqlite3_open_v2),
                   reinterpret_cast<void **>(&ori_sqlite3_open_v2));
    xhook_register("/data/.*\\.so$", "sqlite3_exec",
                   reinterpret_cast<void *>(hook_sqlite3_exec),
                   reinterpret_cast<void **>(&ori_sqlite3_exec));
    xhook_register("/data/.*\\.so$", "sqlite3_step",
                   reinterpret_cast<void *>(hook_sqlite3_step),
                   reinterpret_cast<void **>(&ori_sqlite3_step));
    xhook_register("/data/.*\\.so$", "sqlite3_expanded_sql",
                   reinterpret_cast<void *>(hook_sqlite3_expanded_sql),
                   reinterpret_cast<void **>(&ori_sqlite3_expanded_sql));
    xhook_register("/data/.*\\.so$", "sqlite3_db_filename",
                   reinterpret_cast<void *>(hook_sqlite3_db_filename),
                   reinterpret_cast<void **>(&ori_sqlite3_db_filename));
    xhook_register("/data/.*\\.so$", "sqlite3_db_handle",
                   reinterpret_cast<void *>(hook_sqlite3_db_handle),
                   reinterpret_cast<void **>(&ori_sqlite3_db_handle));

    xhook_refresh(1);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_wcdb2_NativeUtil_nativeTestWcdb(JNIEnv *env, jclass clazz, jstring jfile) {

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

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    gvm = vm;
    ALOGD("gvm: %p", gvm);

    JNIEnv *env;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // 如果无法获取JNIEnv，返回错误码
    }

    ALOGD("env: %p", env);

//    jclass helloJniClass = env->FindClass("com/example/HelloJni"); // 查找Java类
//    if (helloJniClass == NULL) {
//        return JNI_ERR; // 类找不到，返回错误码
//    }
//
//    // 定义方法签名和本地方法实现
//    JNINativeMethod methods[] = {
//            {"nativeMethod", "(I)V", (void*)nativeMethod} // 假设有一个int参数，无返回值的本地方法
//    };
//
//    // 动态注册本地方法
//    jint result = env->RegisterNatives(helloJniClass, methods, sizeof(methods) / sizeof(methods[0]));
//    if (result != JNI_OK) {
//        return result; // 注册失败，返回错误码
//    }

    return JNI_VERSION_1_6; // 返回使用的JNI版本
}
