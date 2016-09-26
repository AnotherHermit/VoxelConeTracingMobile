package se.anotherhermit.voxels;

import android.content.Context;
import android.content.Loader;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLESView extends GLSurfaceView {
    private static final String TAG = "GLESView";
    private static final boolean DEBUG = true;
    private AssetManager mgr;
		private GestureDetector mDetector;
		private ScaleGestureDetector mScaleDetector;

    public GLESView(Context context) {
        super(context);
        mgr = context.getAssets();

        // Pick an EGLConfig with RGB8 color, 16-bit depth, no stencil,
        // supporting OpenGL ES 2.0 or later backwards-compatible versions.
        setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        setEGLContextClientVersion(2);
        setRenderer(new Renderer(mgr));
        String path = Environment.getExternalStorageDirectory().getPath();
        setRenderMode(RENDERMODE_CONTINUOUSLY);
				mDetector = new GestureDetector(context, new MyGestureListener());
				mScaleDetector = new ScaleGestureDetector(context, new MyGestureListener());
    }


	@Override
	public boolean onTouchEvent(MotionEvent event) {
		mDetector.onTouchEvent(event);
		return true; // super.onTouchEvent(event);
	}

	private static class Renderer implements GLSurfaceView.Renderer {
        AssetManager mgr;

        public Renderer(AssetManager mgr) {
            this.mgr = mgr;
        }

        public void onDrawFrame(GL10 gl) {
            step();
        }

        public void onSurfaceChanged(GL10 gl, int width, int height) {
            resize(width, height);
        }

        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            init(mgr);
        }


    }

    static {
        System.loadLibrary("NativeGlue");
    }

    public static native void init(Object mgr);
    public static native void resize(int width, int height);
    public static native void step();

		public static native void touch(float dx, float dy);

	private class MyGestureListener extends GestureDetector.SimpleOnGestureListener implements ScaleGestureDetector.OnScaleGestureListener {
		private static final String DEBUG_TAG = "Gestures";

		@Override
		public boolean onDown(MotionEvent e) {

			Log.d(DEBUG_TAG, "onDown");
			return true;
		}

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
			Log.d(DEBUG_TAG, "onScroll");

			touch(distanceX, distanceY);
			return super.onScroll(e1, e2, distanceX, distanceY);
		}

		@Override
		public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
			Log.d(DEBUG_TAG, "onFling");

			return super.onFling(e1, e2, velocityX, velocityY);
		}

		@Override
		public boolean onScale(ScaleGestureDetector detector) {
			return false;
		}

		@Override
		public boolean onScaleBegin(ScaleGestureDetector detector) {
			return false;
		}

		@Override
		public void onScaleEnd(ScaleGestureDetector detector) {

		}
	}
}
