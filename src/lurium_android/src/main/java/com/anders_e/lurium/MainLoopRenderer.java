package com.anders_e.lurium;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.opengl.GLSurfaceView.Renderer;

public class MainLoopRenderer implements Renderer {

    private final MainLoopActivity mActivity;

    public int mSurfaceWidth, mSurfaceHeight;

    public MainLoopRenderer(MainLoopActivity parent) {
        mActivity = parent;
    }

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        mActivity.onSurfaceCreated();
    }

    @Override
    public void onDrawFrame(GL10 unused) {
        mActivity.onDrawFrame();
    }

    @Override
    public void onSurfaceChanged(GL10 unused, int width, int height) {
        mSurfaceWidth = width;
        mSurfaceHeight = height;
        mActivity.onSurfaceChanged(width, height);
    }

}
