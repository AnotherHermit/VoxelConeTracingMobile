#include <jni.h>
#include "Program.h"

// ----------------------------------------------------------------------------

Program *program = NULL;

extern "C" {
JNIEXPORT void JNICALL Java_se_anotherhermit_voxels_GLESView_init(JNIEnv *env, jobject obj);
JNIEXPORT void JNICALL Java_se_anotherhermit_voxels_GLESView_resize(JNIEnv *env, jobject obj,
                                                                    jint width, jint height);
JNIEXPORT void JNICALL Java_se_anotherhermit_voxels_GLESView_step(JNIEnv *env, jobject obj);
};

JNIEXPORT void JNICALL
Java_se_anotherhermit_voxels_GLESView_init(JNIEnv *env, jobject obj) {
    if (program) {
        delete program;
        program = NULL;
    }

    const char *versionStr = (const char *) glGetString(GL_VERSION);
    if (strstr(versionStr, "OpenGL ES 3.")) {
        program = new Program();
        program->Init();
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

