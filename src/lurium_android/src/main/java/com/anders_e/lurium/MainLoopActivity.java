package com.anders_e.lurium;

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import java.lang.ref.WeakReference;

public class MainLoopActivity extends Activity {

    // Using a static variable for global lurium application state
    private static MainLoop mLoop;
    private MainLoopSurfaceView mGLView;
    private EditText mEditBox;

    static class MainLoopHandler extends Handler {

        private final WeakReference<MainLoopActivity> activityReference;

        MainLoopHandler(MainLoopActivity parent) {
            activityReference = new WeakReference<MainLoopActivity>(parent);
        }

        @Override
        public void handleMessage(Message msg) {
            MainLoopActivity activity = activityReference.get();
            if (activity != null) {
                activity.handleMessage(msg);
            }
        }
    }

    static class EditBoxWatcher implements TextWatcher {
        private final WeakReference<MainLoopActivity> activityReference;

        EditBoxWatcher(MainLoopActivity parent) {
            activityReference = new WeakReference<MainLoopActivity>(parent);
        }

        @Override
        public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}
        @Override
        public void afterTextChanged(Editable editable) {}

        @Override
        public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {
            MainLoopActivity activity = activityReference.get();
            if (activity != null) {
                activity.onTextChanged();
            }
        }
    }

    public MainLoopActivity(Class mainloopClass) {
        Log.d("MAINLOOP", "MainLoopActivity constructor");
        if (mLoop == null) {
            try {
                Log.d("MAINLOOP", "MainLoopActivity first time");
                mLoop = (MainLoop)mainloopClass.newInstance();
                mLoop.CreateState();
            } catch (InstantiationException e) {
                e.printStackTrace();
            } catch (IllegalAccessException e) {
                e.printStackTrace();
            }
        }
        mLoop.SetHandler(new MainLoopHandler(this));
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Create a GLSurfaceView instance and set it as the ContentView for this Activity
        mGLView = new MainLoopSurfaceView(this);
        mGLView.setFocusable(true);
        mGLView.setFocusableInTouchMode(true);

        mEditBox = new EditText(this);
        mEditBox.setSingleLine(true);
        mEditBox.setLines(1);
        mEditBox.setText("");
        mEditBox.setMaxLines(1);
        mEditBox.addTextChangedListener(new EditBoxWatcher(this));

        setContentView(mGLView);
        addContentView(mEditBox, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
    }

    public void onTextChanged() {
        // called by EditBoxWatcher
        final String text = mEditBox.getText().toString();

        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                mLoop.SetWelcomeText(text);
            }
        });
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        // Checks the orientation of the screen
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            //Toast.makeText(this, "landscape", Toast.LENGTH_SHORT).show();
            Log.d("MAINLOOP", "detected landscape");
        } else if (newConfig.orientation == Configuration.ORIENTATION_PORTRAIT){
            //Toast.makeText(this, "portrait", Toast.LENGTH_SHORT).show();
            Log.d("MAINLOOP", "detected portrait");
        }
    }

    @Override
    protected void onStart() {
        Log.d("MAINLOOP", "onStart in the activity");
        super.onStart();
    }

    @Override
    protected void onStop() {
        Log.d("MAINLOOP", "onStop");
        super.onStop();
    }

    @Override
    protected void onPause() {
        super.onPause();
        // The following call pauses the rendering thread.
        // If your OpenGL application is memory intensive,
        // you should consider de-allocating objects that
        // consume significant memory here.
        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                Log.d("MAINLOOP", "onPause from GL thread");
                mLoop.DestroyMainLoop();
            }
        });

        mGLView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d("MAINLOOP", "onResume from activity");

        // The following call resumes a paused rendering thread.
        // If you de-allocated graphic objects for onPause()
        // this is a good place to re-allocate them.
        mGLView.onResume();
    }

    public void onSurfaceCreated() {
        // called by renderer
        mLoop.CreateMainLoop();
    }

    public void onDrawFrame() {
        // called by renderer
        mLoop.Render();
    }

    public void onSurfaceChanged(int width, int height) {
        // called by renderer
        mLoop.Resize(width, height);
    }

    public boolean onSurfaceTouchEvent(MotionEvent event) {
        // called by surface view
        mGLView.requestFocusFromTouch();

        final int action = event.getActionMasked();
        final int index = event.getActionIndex();
        final int id = event.getPointerId(index);

        final float mouseX = event.getX(index) / mGLView.getSurfaceWidth() * 2 - 1;
        final float mouseY = event.getY(index) / mGLView.getSurfaceHeight() * 2 - 1;

        mGLView.queueEvent(new Runnable() {
            @Override
            public void run() {
                switch (action) {
                    case MotionEvent.ACTION_DOWN:
                        mLoop.LbuttonDown(id, mouseX, mouseY);
                        break;
                    case MotionEvent.ACTION_POINTER_DOWN:
                        mLoop.LbuttonDown(id, mouseX, mouseY);
                        break;
                    case MotionEvent.ACTION_UP:
                        mLoop.LbuttonUp(id, mouseX, mouseY);
                        break;
                    case MotionEvent.ACTION_POINTER_UP:
                        mLoop.LbuttonUp(id, mouseX, mouseY);
                        break;
                    case MotionEvent.ACTION_CANCEL:
                        mLoop.LbuttonUp(id, mouseX, mouseY);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        mLoop.MouseMove(id, mouseX, mouseY);
                        break;
                    default:break;
                }
            }
        });

        return true; // does not receive up events if false
    }

    public void handleMessage(Message msg) {
        // called by main loop handler, queued from gl thread
        switch (msg.what) {
            case MainLoop.SetWelcomeTextboxPosition:
                android.graphics.Rect rect = (android.graphics.Rect)msg.obj;
                //mEditBox.layout(rect.left, rect.top, rect.width(), rect.height());
                mEditBox.setX(rect.left);
                mEditBox.setY(rect.top);
                mEditBox.setWidth(rect.width());
                mEditBox.setHeight(rect.height());
                break;
            case MainLoop.SetWelcomeTextboxVisibility:
                boolean visible = (boolean)msg.obj;
                if (visible) {
                    mEditBox.setVisibility(View.VISIBLE);
                } else {
                    mEditBox.clearFocus();
                    mEditBox.setVisibility(View.GONE);
                }
                break;
        }
    }

}
