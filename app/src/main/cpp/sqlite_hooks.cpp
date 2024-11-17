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

static const char *k_MicroMsg = "/com.tencent.mm/MicroMsg/";
static const char *k_EnMicroMsgDb = "/EnMicroMsg.db";
static const char *k_WxFileIndexDb = "/WxFileIndex.db";
static char *k_LastAuthUin = new char[50];

int (*ori_sqlite3_open_v2)(const char *filename, void **db, int flags, const char *zVfs) = nullptr;

int (*ori_sqlite3_exec)(void *db, const char *sql, void *callback, void *, char **errmsg) = nullptr;

int (*ori_sqlite3_step)(void *stmt) = nullptr;

char *(*ori_sqlite3_expanded_sql)(void *stmt) = nullptr;

const char *(*ori_sqlite3_db_filename)(void *db, const char *zDbName) = nullptr;

void *(*ori_sqlite3_db_handle)(void *) = nullptr;

void (*ori_sqlite3_free)(void *) = nullptr;

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

static int32_t k_message_count = 0;

static int hook_sqlite3_step(void *stmt) {
    if (ori_sqlite3_expanded_sql) {

        void *db = ori_sqlite3_db_handle(stmt);
        const char *filename = ori_sqlite3_db_filename(db, "main");
        std::string s_filename(filename);

        size_t a = s_filename.find(k_MicroMsg);
        if (a != std::string::npos) {
            a += strlen(k_MicroMsg);

            size_t b = s_filename.find(k_EnMicroMsgDb);
            if (b == std::string::npos) {
                b = s_filename.find(k_WxFileIndexDb);
            }

            if (b != std::string::npos) {
                std::string s_auth_uin = s_filename.substr(a, b - a);
                ALOGD("#%d auth_uin: (%d) %s", k_message_count++, s_auth_uin.length(),
                      s_auth_uin.c_str());

                if (s_auth_uin != k_LastAuthUin) {
                    strcpy(k_LastAuthUin, s_auth_uin.c_str());
                    ALOGD("#%d last_auth_uin: (%d) %s", k_message_count++, strlen(k_LastAuthUin),
                          k_LastAuthUin);
                }

                char *sql = ori_sqlite3_expanded_sql(stmt);
                if (sql) {
                    std::string s_relative_filename = s_filename.substr(a, s_filename.size() - a);
                    std::string s_msg = s_relative_filename + ":\n" + sql;
                    ALOGD("#%d hook_step: (%d) %s", k_message_count++, s_msg.length(),
                          s_msg.c_str());
                }
                ori_sqlite3_free(reinterpret_cast<void *>(sql));
            } else {
                // ALOGD("[auth_uin] Not EnMicroMsg.db or WxFileIndex.db, %s", s_filename.c_str());
            }
        } else {
            ALOGD("Not contains MicroMsg, %s", s_filename.c_str());
        }
    }

    return ori_sqlite3_step(stmt);
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

    xhook_refresh(1);
}
