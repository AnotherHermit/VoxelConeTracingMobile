package se.anotherhermit.voxels;

import android.os.Bundle;

public class MyNativeActivity extends android.app.NativeActivity {

    static {
        //System.loadLibrary("main");

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        System.loadLibrary("voxels");
        super.onCreate(savedInstanceState);
    }
}