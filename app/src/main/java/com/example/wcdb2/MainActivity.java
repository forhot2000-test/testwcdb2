package com.example.wcdb2;

import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Button;

import com.tencent.wcdb.chaincall.Delete;
import com.tencent.wcdb.chaincall.Select;
import com.tencent.wcdb.core.Database;
import com.tencent.wcdb.core.Handle;
import com.tencent.wcdb.core.PreparedStatement;
import com.tencent.wcdb.winq.StatementInsert;
import com.tencent.wcdb.winq.StatementSelect;

import java.io.File;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = MainActivity.class.getSimpleName();

    private String path;
    private Database database;
    private DaemonSocketServerThread socketServerThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button btn1 = findViewById(R.id.btn1);
        btn1.setText("call wcdb");
        btn1.setOnClickListener(view -> {
            Log.d(TAG, "btn1 clicked");
            doTestWcdb();
        });

        Button btn2 = findViewById(R.id.btn2);
        btn2.setText("call wcdb (in new thread)");
        btn2.setOnClickListener(view -> {
            Log.d(TAG, "btn2 clicked");
            testWcdb();
        });

        Button btn3 = findViewById(R.id.btn3);
        btn3.setText("test parse message");
        btn3.setOnClickListener(view -> {
            Log.d(TAG, "btn3 clicked");
            testParseMessages();
        });

        Button btn4 = findViewById(R.id.btn4);
        btn4.setText("test wcdb from c++");
        btn4.setOnClickListener(view -> {
            Log.d(TAG, "btn4 clicked");
            nativeTestWcdb();
        });

        Button btn5 = findViewById(R.id.btn5);
        btn5.setText("test wcdb legacy");
        btn5.setOnClickListener(view -> {
            Log.d(TAG, "btn5 clicked");
            testWcdbLegacy();
        });

        Button btn6 = findViewById(R.id.btn6);
        btn6.setText("test socket from c++");
        btn6.setOnClickListener(view -> {
            Log.d(TAG, "btn6 clicked");
            nativeTestSocket();
        });

        Button btn7 = findViewById(R.id.btn7);
        btn7.setText("test c++ string compare");
        btn7.setOnClickListener(view -> {
            Log.d(TAG, "btn7 clicked");
            nativeTestStringCompare();
        });

        Log.d(TAG, "nativeInit");
        NativeUtil.nativeInit();

        startSocketServer();
    }

    private void startSocketServer() {
        Log.d(TAG, "start socket server");
        socketServerThread = new DaemonSocketServerThread();
        socketServerThread.start();
    }

    private void testParseMessages() {
        String[] messages = {
                "1:123456",
                "1:",
                "2:xxx.db:select * from t",
                "2:xxx.db:",
                "2:",
                "3:xxx",
                "",
        };

        for (String msg : messages) {
            if (TextUtils.isEmpty(msg)) {
                Log.d(TAG, "(empty string)");
            } else if (msg.startsWith("1:")) {
                String auth_uin = msg.substring(2);
                Log.d(TAG, String.format("%s: %s", "set_auth_uin", auth_uin));
            } else if (msg.startsWith("2:")) {
                String value = msg.substring(2);
                Log.d(TAG, String.format("%s: %s", "send_message", value));
            } else {
                Log.d(TAG, String.format("%s: %s", "unknown", msg));
            }
        }

    }

    private void nativeTestWcdb() {
        String dir = getFilesDir().getAbsolutePath();
        String file = Paths.get(dir, "native.db").toString();
        NativeUtil.nativeTestWcdb("file://" + file);
    }

    private void testWcdb() {
        // Log.d(TAG, "test wcdb2");

        AsyncTask.execute(this::doTestWcdb);
    }

    private void doTestWcdb() {

        String path = getDatabasePath();
        database = new Database(path);

        database.createTable("sampleTable", DBSample.INSTANCE);

        testCreate();
        testQuery();
        testUpdate();
        testExecute();
        testPreparedInsert();
        testPreparedSelect();

        database.close();
    }

    private String getDatabasePath() {
        File dir = new File(getBaseContext().getDataDir(), "com.tencent.mm/MicroMsg/00000000");
        Log.d(TAG, String.format("dir: %s", dir.getPath()));
        if (!dir.exists()) {
            dir.mkdirs();
        }

        File file = new File(dir, "EnMicroMsg.db");
        Log.d(TAG, String.format("file: %s", file.getPath()));

        path = file.getPath();
        return path;
    }

    private void testCreate() {
        Log.d(TAG, "test create");
        //Prepare data
        Sample sample = new Sample();
        sample.id = 1;
        sample.content = "sample_insert";
        //Insert
        database.insertObject(sample, DBSample.allFields(), "sampleTable");
    }

    private void testQuery() {
        Log.d(TAG, "test query");
        List<Sample> samples = database.getAllObjects(DBSample.allFields(), "sampleTable");
    }

    private void testUpdate() {
        Log.d(TAG, "test update");
        //Prepare data
        Sample sample = new Sample();
        sample.content = "sample_update";
        //Update
        //等效于 SQL：UPDATE sampleTable SET content='sampleTable' where id > 0
        database.updateObject(sample, DBSample.content, "sampleTable", DBSample.id.gt(0));
    }

    private void testExecute() {
        Log.d(TAG, "test execute");
        //Java
        Select<Sample> select = database.<Sample>prepareSelect().select(DBSample.allFields()).from("sampleTable");
        List<Sample> objects = select.where(DBSample.id.gt(1)).limit(10).allObjects();
        // Log.v(TAG, "selected " + objects.size() + " rows");// 获取该操作删除的行数

        Delete delete = database.prepareDelete().fromTable("sampleTable").where(DBSample.id.notEq(0));
        delete.execute();
        // Log.v(TAG, "deleted " + delete.getChanges() + " rows");// 获取该操作删除的行数
    }

    private void testPreparedInsert() {
        Log.d(TAG, "test prepared insert");
        // 获取handle和建表
        Handle handle = database.getHandle();

        // 先 prepare statement, 其实是sqlite3_prepare函数的封装。
        PreparedStatement insertStatement = handle.preparedWithMainStatement(
                new StatementInsert()
                        .insertInto("sampleTable")
                        .columns(DBSample.allFields())
                        .valuesWithBindParameters(DBSample.allFields().length));

        Sample object = new Sample();
        for (int i = 0; i < 10; i++) {
            // 先 reset，其实是sqlite3_reset函数的封装。
            insertStatement.reset();

            //1. 可以直接使用对象来bind，会逐个属性调用sqlite3_bind系列接口
            object.id = i;
            object.content = "item " + i;
            insertStatement.bindObject(object, DBSample.allFields());

            // //2. 也可以逐个字段bind，更高效一点
            // insertStatement.bindInteger(i, 1);
            // insertStatement.bindNull(2);

            insertStatement.step();

            // Log.v(TAG, "inserted " + object.content);
        }

        //一个statement用完之后需要调用finalizeStatement，底下会调用sqlite3_finalize函数
        insertStatement.finalizeStatement();

        //handle用完之后也需要调用一下invalidate来回收
        handle.invalidate();
    }

    private void testPreparedSelect() {
        Log.d(TAG, "test prepared select");
        // 获取handle和建表
        Handle handle = database.getHandle();

        ArrayList<Sample> objects = new ArrayList<>();
        // prepare 新的 statement, 其实是sqlite3_prepare函数的封装。
        PreparedStatement selectStatement = handle.preparedWithMainStatement(
                new StatementSelect().select(DBSample.allFields()).from("sampleTable"));
        selectStatement.step();
        while (!selectStatement.isDone()) {
            //1. 可以直接读取对象，会逐个属性来调用sqlite3_column系列接口来读取数据，并赋值给对象
            Sample object = selectStatement.getOneObject(DBSample.allFields());

            // //2. 也可以逐个字段读取，更灵活一点
            // Sample object = new Sample();
            // object.id = selectStatement.getInt(0);
            // object.content = selectStatement.getText(1);

            // Log.v(TAG, "select " + object.content);

            objects.add(object);

            selectStatement.step();
        }

        //一个statement用完之后需要调用finalize，底下会调用sqlite3_finalize函数
        selectStatement.finalizeStatement();
        //handle用完之后也需要调用一下invalidate来回收
        handle.invalidate();
    }

    private void testWcdbLegacy() {
        String path = getDatabasePath();
        com.tencent.wcdb.compat.SQLiteDatabase db = com.tencent.wcdb.compat.SQLiteDatabase.openDatabase(path);
        db.execSQL("select 1");
        db.execSQL("select 2");
        db.execSQL("create table if not exists FinderAccount (" +
                "username varchar(50), " +
                "nickname varchar(50), " +
                "signature varchar(50), " +
                "avatarUrl varchar(50), " +
                "coverUrl varchar(50), " +
                "liveCoverImgUrl varchar(50), " +
                "sex varchar(50), " +
                "country varchar(50), " +
                "province varchar(50), " +
                "city varchar(50), " +
                "userVersion varchar(50), " +
                "seq varchar(50), " +
                "extFlag varchar(50), " +
                "fansAddCount varchar(50), " +
                "contact_user_flag varchar(50), " +
                "spamStatus varchar(50), " +
                "authInfo varchar(50), " +
                "finder_version_username_history varchar(50), " +
                "username_history_version varchar(50), " +
                "messageLikeBuf varchar(50), " +
                "messageCommentBuf varchar(50), " +
                "messageFollowBuf varchar(50), " +
                "liveInfo varchar(50), " +
                "originalInfo varchar(50), " +
                "originalFlag varchar(50), " +
                "originalEntranceFlag varchar(50), " +
                "prepareFinder varchar(50), " +
                "systemMsgCount varchar(50), " +
                "svrIndex varchar(50), " +
                "memberMessageLikeBuf varchar(50), " +
                "memberMessageCommentBuf varchar(50), " +
                "memberMessageJoinBuf varchar(50), " +
                "myFinderTabShowWordingExt varchar(50))");

        // test memory issue
        for (int i = 0; i < 1000; i++) {
            db.execSQL("SELECT username, nickname, signature, avatarUrl, coverUrl, liveCoverImgUrl, sex, " +
                    "country, province, city, userVersion, seq, extFlag, fansAddCount, contact_user_flag, " +
                    "spamStatus, authInfo, finder_version_username_history, username_history_version, " +
                    "messageLikeBuf, messageCommentBuf, messageFollowBuf, liveInfo, originalInfo, " +
                    "originalFlag, originalEntranceFlag, prepareFinder, systemMsgCount, svrIndex, " +
                    "memberMessageLikeBuf, memberMessageCommentBuf, memberMessageJoinBuf, " +
                    "myFinderTabShowWordingExt, rowid " +
                    "FROM FinderAccount WHERE username = '' AND  1=1");
        }

        db.close();
    }

    private void nativeTestSocket() {
        NativeUtil.nativeTestSocket();
    }

    private void nativeTestStringCompare() {
        NativeUtil.nativeTestStringCompare();
    }
}