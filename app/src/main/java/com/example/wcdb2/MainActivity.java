package com.example.wcdb2;

import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
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
    private Button btn1;
    private Button btn2;
    private Button btn3;
    private DaemonSocketServerThread socketServerThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        btn1 = findViewById(R.id.btn1);
        btn2 = findViewById(R.id.btn2);
        btn3 = findViewById(R.id.btn3);
        btn1.setText("call wcdb");
        btn2.setText("call wcdb (in new thread)");
        btn3.setText("test parse message");
        btn1.setOnClickListener(this::onBtn1Click);
        btn2.setOnClickListener(this::onBtn2Click);
        btn3.setOnClickListener(this::onBtn3Click);

//        Log.d(TAG, "nativeInit");
//        NativeUtil.nativeInit();

//        Log.d(TAG, "start socket service");
//        socketServerThread = new DaemonSocketServerThread();
//        socketServerThread.start();
    }

    private void onBtn1Click(View view) {
        Log.d(TAG, "btn1 clicked");
        // nativeTestWcdb();
        doTestWcdb();
    }

    private void onBtn2Click(View view) {
        Log.d(TAG, "btn2 clicked");
        testWcdb();
    }

    private void onBtn3Click(View view) {
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

//    private String[] parseMessage(String message, int size) {
//        String[] array = new String[size];
//        int length = message.length();
//        int start = 0;
//        for (int i = 0; i < size; i++) {
//            if (i == size - 1) {
//                array[i] = message.substring(start);
//            } else {
//                for (int j = start; j < length; j++) {
//                    if (message.charAt(j) == ':') {
//                        array[i] = message.substring(start, j);
//                        start = j + 1;
//                        break;
//                    }
//                }
//            }
//        }
//        return array;
//    }

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

        File dir = new File(getBaseContext().getDataDir(), "com.tencent.mm/MicroMsg/00000000");
        Log.d(TAG, String.format("dir: %s", dir.getPath()));
        if (!dir.exists()) {
            dir.mkdirs();
        }

        File file = new File(dir, "EnMicroMsg.db");
        Log.d(TAG, String.format("file: %s", file.getPath()));

        path = file.getPath();
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

}