package com.example.wcdb2;

import static android.system.OsConstants.EINVAL;

import android.net.Credentials;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.text.TextUtils;
import android.util.Log;

import java.io.EOFException;
import java.io.IOException;
import java.net.SocketException;
import java.util.Locale;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;


public class DaemonSocketServerThread extends Thread {

    private static String TAG = "RakanDaemon";

    private static final int ACTION_READ_MESSAGE = 1;
    private static final int ACTION_WRITE_MESSAGE = 2;
    private static final int ACTION_RESERVE = 3;

    private static final Executor EXECUTOR = Executors.newFixedThreadPool(Runtime.getRuntime().availableProcessors() / 4 + 1);

    private LocalServerSocket serverSocket;
    private CountDownLatch countDownLatch;

    static void writeString(LittleEndianDataOutputStream out, String s) throws IOException {
        out.writeInt(s.length());
        out.writeBytes(s);
    }

    static String readString(LittleEndianDataInputStream in) throws IOException {
        int length = in.readInt(); // 读取字符串的长度
        byte[] bytes = new byte[length]; // 创建一个字节数组来存储字符串内容
        in.readFully(bytes); // 读取字符串内容
        return new String(bytes); // 将字节数组转换为字符串
    }

    private void handleAction(LittleEndianDataInputStream in, LittleEndianDataOutputStream out, int action, Credentials credentials) throws IOException {
        // Log.i(TAG, "Action " + action);

        switch (action) {
            case ACTION_READ_MESSAGE: {
                Log.i(TAG, "Action: read message");
                break;
            }
            case ACTION_WRITE_MESSAGE: {
                String msg = readString(in);
                if (TextUtils.isEmpty(msg)) {
                    // out.writeInt(-1);
                } else if (msg.startsWith("1:")) {
                    String auth_uin = msg.substring(2);
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    Log.d(TAG, String.format("%s: %s", "set_auth_uin", auth_uin));
                    // out.writeInt(0);
                } else if (msg.startsWith("2:")) {
                    String value = msg.substring(2);
                    Log.d(TAG, String.format("%s: %s", "send_message", value));
                    // out.writeInt(0);
                } else {
                    Log.d(TAG, String.format("%s: %s", "unknown", msg));
                    // out.writeInt(0);
                }
                break;
            }
            case ACTION_RESERVE: {
                Log.i(TAG, "Action: reserve");
                break;
            }
            default:
                Log.w(TAG, "unknown action");
                break;
        }

        // Log.i(TAG, "Handle action " + action + " finished");
    }

    private void handleSocket(LocalSocket socket) throws IOException {
        int action;
        boolean first = true;
        Credentials credentials = socket.getPeerCredentials();

        try (LittleEndianDataInputStream in = new LittleEndianDataInputStream(socket.getInputStream());
             LittleEndianDataOutputStream out = new LittleEndianDataOutputStream(socket.getOutputStream())) {

            while (true) {
                try {
                    action = in.readInt();
                } catch (EOFException e) {
                    if (first) {
                        Log.e(TAG, "Failed to read next action", e);
                    } else {
                        Log.i(TAG, "No next action, exiting...");
                    }
                    return;
                }
                handleAction(in, out, action, credentials);
                first = false;
            }
        }
    }

    private void startServer() throws IOException {
        serverSocket = new LocalServerSocket("rakan");

        while (true) {
            Log.d(TAG, "Accept");
            LocalSocket socket;
            try {
                socket = serverSocket.accept();
            } catch (IOException e) {
                if ((e.getCause() != null && e.getCause() instanceof ErrnoException && ((ErrnoException) e.getCause()).errno == EINVAL) ||
                        e instanceof SocketException ||
                        (e.getMessage() != null && (e.getMessage().contains("EINVAL") || e.getMessage().contains(String.format(Locale.ROOT, "errno %d", EINVAL))))) {
                    Log.i(TAG, "Server shutdown.");
                    return;
                }
                Log.w(TAG, "Accept failed, server is closed ?", e);
                return;
            }

            Credentials credentials = socket.getPeerCredentials();
            int uid = credentials.getUid();
            int pid = credentials.getPid();

            Log.d(TAG, "Accepted " +
                    "uid=" + uid + ", " +
                    "pid=" + pid);

            EXECUTOR.execute(() -> {
                try {
                    handleSocket(socket);
                } catch (Throwable e) {
                    Log.w(TAG, "Handle socket", e);
                } finally {
                    if (socket != null) {
                        try {
                            socket.close();
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }
            });
        }
    }

    public void stopServer() {
        Log.i(TAG, "Stop server");

        if (serverSocket != null) {
            try {
                Os.shutdown(serverSocket.getFileDescriptor(), OsConstants.SHUT_RD);
                serverSocket.close();
                Log.i(TAG, "Server stopped");
            } catch (IOException | ErrnoException e) {
                Log.w(TAG, "Close server socket", e);
            }
            serverSocket = null;
        } else {
            Log.w(TAG, "Server socket is null while stopping server");
        }
    }

    @Override
    public void run() {

        //noinspection InfiniteLoopStatement
        while (true) {
            Log.d(TAG, "Start server");

            try {
                startServer();
            } catch (Throwable e) {
                Log.w(TAG, "Start server", e);
            }

            countDownLatch = new CountDownLatch(1);
            Log.i(TAG, "Waiting for restart server...");

            try {
                countDownLatch.await();
                Log.i(TAG, "Restart server received");
            } catch (InterruptedException e) {
                Log.w(TAG, Log.getStackTraceString(e));
            }
        }
    }
}
