#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>
#include <android/log.h>
#include <xhook.h>
#include <bytehook.h>

#include "wcdb.h"

#define TAG "NativeUtil.cpp"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)


JavaVM *gvm;


static int (*ori_xhook_sqlite3_open_v2)(const char *filename, void **db, int flags,
                                        const char *zVfs) = nullptr;

static int hook_sqlite3_open_v2(const char *filename, void **db, int flags, const char *zVfs) {
    BYTEHOOK_STACK_SCOPE();
    int res = BYTEHOOK_CALL_PREV(hook_sqlite3_open_v2, filename, db, flags, zVfs);
    ALOGD("hook_open_v2: %s", filename);
    return res;
}

static int hook_sqlite3_open_v2_2(const char *filename, void **db, int flags, const char *zVfs) {
    ALOGD("hook_open_v2: %s, %p", filename, ori_xhook_sqlite3_open_v2);
    int res = ori_xhook_sqlite3_open_v2(filename, db, flags, zVfs);
    return res;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_wcdb2_NativeUtil_nativeInit(JNIEnv *env, jclass clazz) {

//    bytehook_init(0, false);
//    bytehook_stub_t stub = bytehook_hook_single(
//            "libWCDB.so",
//            nullptr,
//            "sqlite3_open_v2",
//            reinterpret_cast<void *>(hook_sqlite3_open_v2),
//            nullptr,
//            nullptr
//    );
//    if (stub != nullptr) {
//        ALOGD("hook ok");
//    }

//    xhook_clear();
//    xhook_enable_debug(0);
//    xhook_register("/data/.*\\.so$", "sqlite3_open_v2",
//                   reinterpret_cast<void *>(hook_sqlite3_open_v2_2),
//                   reinterpret_cast<void **>(&ori_xhook_sqlite3_open_v2));
//
//    xhook_refresh(1);
//    ALOGD("xhook: %p", ori_xhook_sqlite3_open_v2);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_wcdb2_NativeUtil_nativeTestWcdb(JNIEnv *env, jclass clazz, jstring jfile) {

//    int result;
//
//    result = load_lib_wcdb();
//    if (result != LIB_LOAD_SUCCESS) {
//        ALOGE("load libWCDB.so failed");
//        return;
//    }

    xhook_clear();
    xhook_enable_debug(1);
    xhook_register("/data/.*\\.so$", "sqlite3_open_v2",
                   reinterpret_cast<void *>(hook_sqlite3_open_v2_2),
                   reinterpret_cast<void **>(&ori_xhook_sqlite3_open_v2));

    xhook_refresh(1);
    ALOGD("xhook: %p", ori_xhook_sqlite3_open_v2);

//    const char *filename = env->GetStringUTFChars(jfile, NULL);
//    void *db;
//    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI;
//    const char *sql = "select ?";
//    void *stmt;
//
//    ALOGD("filename: %s", filename);
//    result = x_sqlite3_open_v2(filename, &db, flags, NULL);
//    if (result != SQLITE_OK) {
//        const char *err = x_sqlite3_errmsg(db);
//        ALOGE("open db failed: (%d) %s", result, err);
//    }
//
//    if (result == SQLITE_OK) {
//        const char *n = x_sqlite3_db_filename(db, "main");
//        ALOGD("get db filename: %s", n);
//    }
//
//
//    // ERROR: bind update hook callback error
//    // if (result == SQLITE_OK) {
//    //     x_sqlite3_update_hook(db, wcdbUpdateHookCallback, (void *) filename);
//    // }
//
//    if (result == SQLITE_OK) {
//        // x_sqlite3_trace(db, 0xffff, callback, ctx);
//    }
//
//
//    if (result == SQLITE_OK) {
//        result = x_sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
//        if (result != SQLITE_OK) {
//            const char *err = x_sqlite3_errmsg(db);
//            ALOGE("prepare statement failed: (%d) %s", result, err);
//        }
//    }
//
//    if (result == SQLITE_OK) {
//        result = x_sqlite3_bind_int(stmt, 1, 10);
//        if (result != SQLITE_OK) {
//            const char *err = x_sqlite3_errmsg(db);
//            ALOGE("bind parameter failed: (%d) %s", result, err);
//        }
//    }
//
//    if (result == SQLITE_OK) {
//        const char *s;
//        s = x_sqlite3_sql(stmt);
//        ALOGD("sql: %s", s);
//        s = x_sqlite3_expanded_sql(stmt);
//        ALOGD("expanded sql: %s", s);
//        void *db = x_sqlite3_db_handle(stmt);
//        s = x_sqlite3_db_filename(db, "main");
//        ALOGD("filename from stmt: %s", s);
//    }
//
//
//    if (result == SQLITE_OK) {
//        ALOGD("start exec select 1");
//        const char *sql = "select 1";
//        char *err;
//        result = x_sqlite3_exec(db, sql, (void *) wcdbExecCallback, (void *) sql, &err);
//        if (result != SQLITE_OK) {
//            ALOGE("exec select 1 failed: (%d) %s", result, err);
//            x_sqlite3_free(err);
//        } else {
//            ALOGD("exec select 1 success");
//        }
//    }
//
//    if (result == SQLITE_OK) {
//        ALOGD("start create table test1");
//        const char *sql = "create table if not exists test1 (value text)";
//        char *err;
//        result = x_sqlite3_exec(db, sql, (void *) wcdbExecCallback, (void *) sql, &err);
//        if (result != SQLITE_OK) {
//            ALOGE("create table test1 failed: (%d) %s", result, err);
//            x_sqlite3_free(err);
//        } else {
//            ALOGD("create table test1 success");
//        }
//    }
//
//    // ERROR: sql error
//    // if (result == SQLITE_OK) {
//    //     ALOGD("start show tables");
//    //     const char *sql = ".tables";
//    //     char *err;
//    //     result = x_sqlite3_exec(db, sql, wcdbExecCallback, (void *) sql, &err);
//    //     if (result != SQLITE_OK) {
//    //         ALOGE("show tables failed: (%d) %s", result, err);
//    //         x_sqlite3_free(err);
//    //     } else {
//    //         ALOGD("show tables success");
//    //     }
//    // }
//
//    for (int i = 0; i < 3; ++i) {
//        if (result == 0) {
//            ALOGD("start insert into test1");
//            const char *sql = "insert into test1 (value) values ('hello, world!')";
//            char *err;
//            result = x_sqlite3_exec(db, sql, (void *) wcdbExecCallback, (void *) sql, &err);
//            if (result != SQLITE_OK) {
//                ALOGE("insert into test1 failed: (%d) %s", result, err);
//                x_sqlite3_free(err);
//            } else {
//                ALOGD("insert into test1 success");
//            }
//        }
//    }
//
//
//    env->ReleaseStringUTFChars(jfile, filename);
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
