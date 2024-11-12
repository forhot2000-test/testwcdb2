//
// Created by wang on 2024/11/11.
//

#include <string>
#include <android/log.h>
#include <xhook.h>

#include "sqlite_apis.h"
#include "sqlite_hooks.h"

#define TAG "hooks.cpp"
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, ##__VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, ##__VA_ARGS__)

static std::string k_MicroMsg("/com.tencent.mm/MicroMsg/");
static std::string k_EnMicroMsgDb("/EnMicroMsg.db");
static std::string k_WxFileIndexDb("/WxFileIndex.db");
static std::string k_LastAuthUin;


static int (*ori_sqlite3_open_v2)(const char *filename, void **db, int flags,
                                  const char *zVfs) = nullptr;

static int (*ori_sqlite3_exec)(void *db, const char *sql, void *callback, void *,
                               char **errmsg) = nullptr;

static int (*ori_sqlite3_step)(void *stmt) = nullptr;

static char *(*ori_sqlite3_expanded_sql)(void *stmt);

static const char *(*ori_sqlite3_db_filename)(void *db, const char *zDbName);

static void *(*ori_sqlite3_db_handle)(void *);

static void (*ori_sqlite3_free)(void *);


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

static int hook_sqlite3_step(void *stmt) {
    if (ori_sqlite3_expanded_sql) {

        void *db = ori_sqlite3_db_handle(stmt);
        char *sql = ori_sqlite3_expanded_sql(stmt);
        const char *filename = ori_sqlite3_db_filename(db, "main");
//        ALOGD("step: %s (%s)", sql, filename);

        if (sql) {
            std::string s_sql(sql);
            std::string s_filename(filename);

            size_t a = s_filename.find(k_MicroMsg);
            if (a == std::string::npos) {
                // ALOGD("[auth_uin] Not in MicroMsg, %s", s_filename.c_str());
            } else {
                a += k_MicroMsg.length();

                size_t b = s_filename.find(k_EnMicroMsgDb);
                if (b == std::string::npos) {
                    b = s_filename.find(k_WxFileIndexDb);
                }

                if (b == std::string::npos) {
                    // ALOGD("[auth_uin] Not EnMicroMsg.db or WxFileIndex.db, %s", s_filename.c_str());
                } else {

                    std::string s_auth_uin = s_filename.substr(a, b - a);
                    if (s_auth_uin != k_LastAuthUin) {
                        k_LastAuthUin = s_auth_uin;
                        std::string s_msg = "1:" + s_auth_uin;
                        ALOGD("auth_uin: %s", s_msg.c_str());
                    }

                    std::string s_relative_filename = s_filename.substr(a, s_filename.size() - a);
                    std::string s_msg = "2:" + s_relative_filename + ":\n" + s_sql;
                    ALOGD("hook_step: %s", s_msg.c_str());
                }
            }
        }

        ori_sqlite3_free(reinterpret_cast<void *>(sql));
    }

    return ori_sqlite3_step(stmt);
}

static char *hook_sqlite3_expanded_sql(void *stmt) {
    return ori_sqlite3_expanded_sql(stmt);
}

static const char *hook_sqlite3_db_filename(void *db, const char *name) {
    return ori_sqlite3_db_filename(db, name);
}

static void *hook_sqlite3_db_handle(void *stmt) {
    return ori_sqlite3_db_handle(stmt);
}

static void hook_sqlite3_free(void *obj) {
    return ori_sqlite3_free(obj);
}


void registerHooks() {
    xhook_clear();
    xhook_enable_debug(1);
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_open_v2",
                   reinterpret_cast<void *>(hook_sqlite3_open_v2),
                   reinterpret_cast<void **>(&ori_sqlite3_open_v2));
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_exec",
                   reinterpret_cast<void *>(hook_sqlite3_exec),
                   reinterpret_cast<void **>(&ori_sqlite3_exec));
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_step",
                   reinterpret_cast<void *>(hook_sqlite3_step),
                   reinterpret_cast<void **>(&ori_sqlite3_step));
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_expanded_sql",
                   reinterpret_cast<void *>(hook_sqlite3_expanded_sql),
                   reinterpret_cast<void **>(&ori_sqlite3_expanded_sql));
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_db_filename",
                   reinterpret_cast<void *>(hook_sqlite3_db_filename),
                   reinterpret_cast<void **>(&ori_sqlite3_db_filename));
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_db_handle",
                   reinterpret_cast<void *>(hook_sqlite3_db_handle),
                   reinterpret_cast<void **>(&ori_sqlite3_db_handle));
    xhook_register("/data/.*libWCDB\\.so$", "sqlite3_free",
                   reinterpret_cast<void *>(hook_sqlite3_free),
                   reinterpret_cast<void **>(&ori_sqlite3_free));

    xhook_refresh(1);
}
