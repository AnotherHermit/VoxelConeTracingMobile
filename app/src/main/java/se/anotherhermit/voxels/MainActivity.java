package se.anotherhermit.voxels;

import android.os.Bundle;
import android.app.Activity;
import android.view.SurfaceHolder;

public class MainActivity extends Activity {

    GLESView mView;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new GLESView(getApplication());

        mView.getHolder().setFixedSize(360,640);

        setContentView(mView);
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }
}
