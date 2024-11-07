package com.example.wcdb2;

import android.app.Application;
import android.util.Log;

public class MyApplication extends Application {

    private static final String TAG = "MyApplication";

    private DaemonSocketServerThread socketServerThread;

    @Override
    public void onCreate() {
        super.onCreate();

//        Log.d(TAG, "nativeInit");
//        NativeUtil.nativeInit();

//        Log.d(TAG, "start socket service");
//        socketServerThread = new DaemonSocketServerThread();
//        socketServerThread.start();
    }
}
