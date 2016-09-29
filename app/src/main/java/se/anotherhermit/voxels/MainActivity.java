package se.anotherhermit.voxels;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity {

	GLESView mView;

	@Override
	protected void onCreate(Bundle icicle) {
		super.onCreate(icicle);
		mView = new GLESView(getApplication());

		mView.getHolder().setFixedSize(450, 800);

		setContentView(mView);
	}

	static {
		try {
			System.loadLibrary("MGD");
		} catch (UnsatisfiedLinkError e) {
			// Feel free to remove this log message.
			Log.i("[ MGD ]", "libMGD.so not loaded.");
		}
	}

	@Override
	protected void onPause() {
		super.onPause();
		mView.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
		mView.onResume();
	}
}
