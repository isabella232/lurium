package com.anders_e.lurium;

import android.os.Handler;
import android.os.Message;

import java.io.UnsupportedEncodingException;

public abstract class MainLoop {

    public static final int SetWelcomeTextboxPosition = 1;
    public static final int SetWelcomeTextboxVisibility = 2;

    private long instance;
    private long state;
    private Handler handler;

    public MainLoop() {
    }

    public void SetHandler(Handler uiHandler) {
        handler = uiHandler;
    }

    public void CreateState() {
        state = InternalCreateState();
    }
    public void DestroyState() {
        InternalDestroyState(state);
    }

    public void CreateMainLoop() {
        instance = InternalCreateMainLoop(state);
    }

    public void DestroyMainLoop() {
        InternalDestroyMainLoop(instance);
        instance = 0;
    }

    public void Render() {
        InternalRender(instance);
    }

    public void Resize(int width, int height) {
        InternalResize(instance, width, height);
    }

    public void LbuttonDown(int id, float x, float y) {
        InternalLbuttonDown(instance, id, x, y);
    }

    public void LbuttonUp(int id, float x, float y) {
        InternalLbuttonUp(instance, id, x, y);
    }

    public void MouseMove(int id, float x, float y) {
        InternalMouseMove(instance, id, x, y);
    }

    public void SetWelcomeTextboxVisibility(boolean visible) {
        // called from c++, gl render thread, queue into ui thread
        Message msg = new Message();
        msg.what = SetWelcomeTextboxVisibility;
        msg.obj = visible;
        handler.sendMessage(msg);
    }

    public void SetWelcomeTextboxPosition(int x, int y, int width, int height) {
        // called from c++, gl render thread, queue into ui thread
        Message msg = new Message();
        msg.what = SetWelcomeTextboxPosition;
        msg.obj = new android.graphics.Rect(x, y, x + width, y + height);
        handler.sendMessage(msg);
    }

    public void SetWelcomeText(String text) {
        // called from gl render thread, originated in ui thread as a onTextChanged event
        // NOTE: c++ side cannot set welcome text on ui side
        try {
            byte[] bytes = text.getBytes("UTF-8");
            InternalSetWelcomeText(instance, bytes);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }

    public native long InternalCreateState();
    public native long InternalDestroyState(long state);
    public native long InternalCreateMainLoop(long state);
    public native long InternalDestroyMainLoop(long loop);
    public native void InternalRender(long loop);
    public native void InternalResize(long loop, int width, int height);
    public native void InternalMouseMove(long loop, int id, float x, float y);
    public native void InternalLbuttonDown(long loop, int id, float x, float y);
    public native void InternalLbuttonUp(long loop, int id, float x, float y);
    public native void InternalSetWelcomeText(long loop, byte[] text);
}
