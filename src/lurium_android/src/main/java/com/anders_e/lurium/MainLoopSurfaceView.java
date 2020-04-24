package com.anders_e.lurium;

import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

public class MainLoopSurfaceView extends GLSurfaceView {
    private final MainLoopRenderer mRenderer;
    private final MainLoopActivity mActivity;

    public MainLoopSurfaceView(MainLoopActivity activity) {
        super(activity);

        setEGLContextClientVersion(2);
        // http://stackoverflow.com/questions/32042458/all-opengl-es-2-apps-crash-in-visual-studio-emulator-for-android
        setEGLConfigChooser(8, 8, 8, 8, 16, 0);

        mActivity = activity;
        mRenderer = new MainLoopRenderer(mActivity);

        // Set the Renderer for drawing on the GLSurfaceView
        setRenderer(mRenderer);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        super.onTouchEvent(event);
        return mActivity.onSurfaceTouchEvent(event);
    }

    public int getSurfaceWidth() {
        return mRenderer.mSurfaceWidth;
    }

    public int getSurfaceHeight() {
        return mRenderer.mSurfaceHeight;
    }

}
