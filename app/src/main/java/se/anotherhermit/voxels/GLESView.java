package se.anotherhermit.voxels;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.widget.Toast;

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
		mScaleDetector.onTouchEvent(event);
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

	public static native void scroll(float dx, float dy);

	public static native void scale(float scale);

	public static native void doubleTap();

	public static native void longPress();

	private class MyGestureListener extends GestureDetector.SimpleOnGestureListener implements ScaleGestureDetector.OnScaleGestureListener {
		private static final String DEBUG_TAG = "Gestures";

		@Override
		public boolean onDown(MotionEvent e) {
			Log.d(DEBUG_TAG, "onDown");

			return true;
		}

		@Override
		public void onLongPress(MotionEvent e) {
			Log.d(DEBUG_TAG, "onLongPress");
			longPress();
			Toast.makeText(getContext(), "Toggled Light/Camera", Toast.LENGTH_SHORT)
					.show();
			super.onLongPress(e);
		}

		@Override
		public boolean onDoubleTap(MotionEvent e) {
			Log.d(DEBUG_TAG, "onDoubleTap");
			doubleTap();
			Toast.makeText(getContext(), "Switched View", Toast.LENGTH_SHORT)
					.show();
			return super.onDoubleTap(e);
		}

		@Override
		public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
			Log.d(DEBUG_TAG, "onScroll");
			scroll(distanceX, distanceY);
			return super.onScroll(e1, e2, distanceX, distanceY);
		}

		@Override
		public boolean onScale(ScaleGestureDetector detector) {
			Log.d(DEBUG_TAG, "onScale");
			scale(detector.getScaleFactor());
			return true;
		}

		@Override
		public boolean onScaleBegin(ScaleGestureDetector detector) {
			Log.d(DEBUG_TAG, "onScaleBegin");

			return true;
		}

		@Override
		public void onScaleEnd(ScaleGestureDetector detector) {
			Log.d(DEBUG_TAG, "onScaleEnd");
		}
	}
}
