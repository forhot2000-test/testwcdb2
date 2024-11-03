package com.example.wcdb2;

import android.os.IBinder;

public class NativeUtil {
    static {
        try {
            System.loadLibrary("mywcdb2");
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    public native static void nativeTestWcdb(String file);
}
