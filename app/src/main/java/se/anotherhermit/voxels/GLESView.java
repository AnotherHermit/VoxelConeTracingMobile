package se.anotherhermit.voxels;

import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Environment;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GLESView extends GLSurfaceView {
    private static final String TAG = "GLESView";
    private static final boolean DEBUG = true;
    private AssetManager mgr;

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
}
