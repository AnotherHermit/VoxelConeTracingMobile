#include <jni.h>
#include <android/asset_manager_jni.h>
#include <stdlib.h>
#include "Program.h"

// ----------------------------------------------------------------------------

Program *program = NULL;

extern "C" {
JNIEXPORT void JNICALL Java_se_anotherhermit_voxels_GLESView_init(JNIEnv *env, jobject obj, jobject assetMgr);
JNIEXPORT void JNICALL Java_se_anotherhermit_voxels_GLESView_resize(JNIEnv *env, jobject obj,
                                                                    jint width, jint height);
JNIEXPORT void JNICALL Java_se_anotherhermit_voxels_GLESView_step(JNIEnv *env, jobject obj);
};

JNIEXPORT void JNICALL
Java_se_anotherhermit_voxels_GLESView_init(JNIEnv *env, jobject obj, jobject assetMgr) {
    if (program) {
        delete program;
        program = NULL;
    }

    const char *versionStr = (const char *) glGetString(GL_VERSION);
    if (strstr(versionStr, "OpenGL ES 3.")) {
        program = new Program();
        AAssetManager* mgr = AAssetManager_fromJava(env, assetMgr);
        program->SetAssetMgr(mgr);
        if(!program->Init()) {
            LOGE("Init failed!");
            abort();
        }
    } else {
        LOGE("Unsupported OpenGL ES version");
    }
}

JNIEXPORT void JNICALL
Java_se_anotherhermit_voxels_GLESView_resize(JNIEnv *env, jobject obj, jint width, jint height) {
    if (program) {
        program->Resize(width, height);
    }
}

JNIEXPORT void JNICALL
Java_se_anotherhermit_voxels_GLESView_step(JNIEnv *env, jobject obj) {
    if (program) {
        program->Step();
    }
}

