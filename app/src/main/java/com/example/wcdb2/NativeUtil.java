package com.example.wcdb2;

public class NativeUtil {
    static {
        try {
            System.loadLibrary("mywcdb2");
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    public native static void nativeInit();

    public native static void nativeTestWcdb(String file);

    public native static void nativeTestSocket();

    public native static void nativeTestStringCompare();
}
