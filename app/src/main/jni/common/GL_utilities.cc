// GL utilities, bare essentials
// By Ingemar Ragnemalm

// August 2012:
// FBO creation/usage routines.
// Geometry shader support synched with preliminary version.
// September 2012: Improved infolog printouts with file names.
// 120910: Clarified error messages from shader loader.
// 120913: Re-activated automatic framebuffer checks for UseFBO().
// Fixed FUBAR in InitFBO().
// 130228: Changed most printf's to stderr.
// 131014: Added tesselation shader support

//#define GL3_PROTOTYPES
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "GL_utilities.h"


AAssetManager* mgr;

void SetAssetsManager(AAssetManager* initMgr) {
    mgr = initMgr;
}

// Load asset folder to file

void LoadAssetFolder(const char* folderName) {
    AAssetDir* assetDir = AAssetManager_openDir(mgr, folderName);
    const char* filename;
    char* buf;
    size_t length;
    char completePath[256];
    char externalPath[256];

    while((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
        snprintf(completePath, sizeof(completePath), "%s/%s",folderName, filename);
        AAsset* asset = AAssetManager_open(mgr, completePath, AASSET_MODE_STREAMING);
        length = (size_t)AAsset_getLength(asset);
        buf = (char *) malloc(length);
        AAsset_read(asset, buf, length);

        snprintf(externalPath, sizeof(externalPath), MODEL_PATH("%s"), filename);
        FILE* out = fopen(externalPath, "w");
        fwrite(buf, length, 1, out);
        fclose(out);

        AAsset_close(asset);
    }

    AAssetDir_close(assetDir);
}

// Shader loader

char *readFile(const char *file) {
    size_t length;
    char *buf;
    AAsset* asset;

     if (file == NULL) {
        return NULL;
    }

    asset = AAssetManager_open(mgr, file, AASSET_MODE_STREAMING);

    if (!asset) {
        LOGE("Could not open file: %s!\n", file);
        return NULL;// Return NULL on failure
    }

    length = (size_t)AAsset_getLength(asset);
    if (length < 0) {
        LOGE("ftell reported negative length!\n");
        return NULL;
    }

    buf = (char *) malloc(length +
                          1); // Allocate a buffer for the entire length of the file and a null terminator
    if (buf == NULL) {
        LOGE("Could not allocate space for readFile buffer!\n");
        return NULL;
    }

    AAsset_read(asset, buf, length); // Read the contents of the file in to the buffer
    AAsset_close(asset); // Close the file
    buf[length] = 0; // Null terminator

    return buf; // Return the buffer
}

// Infolog: Show result of shader compilation
GLint printShaderInfoLog(GLuint obj, const char *fn) {
    GLint infologLength = 0;
    GLint charsWritten = 0;
    char *infoLog;

    GLint wasError = 0;

    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

    if (infologLength > 2) {
        LOGE("[From %s:]\n", fn);
        infoLog = (char *) malloc((size_t)infologLength);
        glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
        LOGE("%s\n", infoLog);
        free(infoLog);
        wasError = 1;
    }

    return wasError;
}

GLint printProgramInfoLog(GLuint obj, const char *vfn, const char *ffn,
                          const char *gfn, const char *tcfn, const char *tefn) {
    GLint infologLength = 0;
    GLint charsWritten = 0;
    char *infoLog;

    GLint wasError = 0;

    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

    if (infologLength > 2) {
        LOGE("[From %s", vfn);
        if (ffn != NULL)
            LOGE("+%s", ffn);
        if (gfn != NULL)
            LOGE("+%s", gfn);
        if (tcfn != NULL && tefn != NULL)
            LOGE("+%s+%s", tcfn, tefn);
        LOGE(":]\n");

        infoLog = (char *) malloc((size_t)infologLength);
        glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
        LOGE("%s\n", infoLog);
        free(infoLog);

        wasError = 1;
    }

    return wasError;
}

// Compile a shader, return reference to it
GLuint compileShaders(const char *vs, const char *fs, const char *gs, const char *tcs,
                      const char *tes,
                      const char *vfn, const char *ffn, const char *gfn, const char *tcfn,
                      const char *tefn) {
    GLuint v, f, g, tc, te, p;

    p = glCreateProgram();

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    g = glCreateShader(GL_GEOMETRY_SHADER);
    tc = glCreateShader(GL_TESS_CONTROL_SHADER);
    te = glCreateShader(GL_TESS_EVALUATION_SHADER);

    glShaderSource(v, 1, &vs, NULL);
    glCompileShader(v);
    glAttachShader(p, v);
    glDeleteShader(v);
    printShaderInfoLog(v, vfn);

    if (fs != NULL) {
        glShaderSource(f, 1, &fs, NULL);
        glCompileShader(f);
        glAttachShader(p, f);
        glDeleteShader(v);
        printShaderInfoLog(f, ffn);
    }
    if (gs != NULL) {
        glShaderSource(g, 1, &gs, NULL);
        glCompileShader(g);
        glAttachShader(p, g);
        glDeleteShader(g);
        printShaderInfoLog(g, gfn);
    }
#ifdef GL_TESS_CONTROL_SHADER_EXT
    if (tcs != NULL) {
        glShaderSource(tc, 1, &tcs, NULL);
        glCompileShader(tc);
        glAttachShader(p, tc);
        glDeleteShader(tc);
        printShaderInfoLog(tc, tcfn);
    }
    if (tes != NULL) {
        glShaderSource(te, 1, &tes, NULL);
        glCompileShader(te);
        glAttachShader(p, te);
        glDeleteShader(te);
        printShaderInfoLog(te, tefn);
    }
#endif

    if (fs != NULL) {
        glLinkProgram(p);
        glUseProgram(p);
        printProgramInfoLog(p, vfn, ffn, gfn, tcfn, tefn);
    }

    if (vs != NULL) glDetachShader(p, v);
    if (fs != NULL) glDetachShader(p, f);
    if (gs != NULL) glDetachShader(p, g);
    if (tcs != NULL) glDetachShader(p, tc);
    if (tes != NULL) glDetachShader(p, te);

    return p;
}

GLuint loadShaders(const char *vertFileName, const char *fragFileName) {
    return loadShadersGT(vertFileName, fragFileName, NULL, NULL, NULL);
}

GLuint loadShadersG(const char *vertFileName, const char *fragFileName, const char *geomFileName)
// With geometry shader support
{
    return loadShadersGT(vertFileName, fragFileName, geomFileName, NULL, NULL);
}

GLuint loadShadersGT(const char *vertFileName, const char *fragFileName, const char *geomFileName,
                     const char *tcFileName, const char *teFileName)
// With tesselation shader support
{
    char *vs, *fs, *gs, *tcs, *tes;
    GLuint p = 0;

    vs = readFile((char *) vertFileName);
    fs = readFile((char *) fragFileName);
    gs = readFile((char *) geomFileName);
    tcs = readFile((char *) tcFileName);
    tes = readFile((char *) teFileName);
    if (vs == NULL)
        LOGE("Failed to read %s from disk.\n", vertFileName);
    if ((fs == NULL) && (fragFileName != NULL))
        LOGE("Failed to read %s from disk.\n", fragFileName);
    if ((gs == NULL) && (geomFileName != NULL))
        LOGE("Failed to read %s from disk.\n", geomFileName);
    if ((tcs == NULL) && (tcFileName != NULL))
        LOGE("Failed to read %s from disk.\n", tcFileName);
    if ((tes == NULL) && (teFileName != NULL))
        LOGE("Failed to read %s from disk.\n", teFileName);
    if (vs != NULL)
        p = compileShaders(vs, fs, gs, tcs, tes, vertFileName, fragFileName, geomFileName,
                           tcFileName, teFileName);
    if (vs != NULL) free(vs);
    if (fs != NULL) free(fs);
    if (gs != NULL) free(gs);
    if (tcs != NULL) free(tcs);
    if (tes != NULL) free(tes);
    return p;
}

GLuint CompileComputeShader(const char *compFileName) {
    GLuint program = glCreateProgram();
    GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

    const char *cs = readFile((char *) compFileName);
    if (cs == NULL) {
        printf("Error reading shader!\n");
    }

    glShaderSource(computeShader, 1, &cs, NULL);
    glCompileShader(computeShader);

    printShaderInfoLog(computeShader, compFileName);

    glAttachShader(program, computeShader);
    glDeleteShader(computeShader);
    glLinkProgram(program);
    glDetachShader(program, computeShader);

    printProgramInfoLog(program, compFileName, NULL, NULL, NULL, NULL);

    return program;
}

// End of Shader loader

void dumpInfo(void) {
    LOGI("Vendor: %s\n", glGetString(GL_VENDOR));
    LOGI("Renderer: %s\n", glGetString(GL_RENDERER));
    LOGI("Version: %s\n", glGetString(GL_VERSION));
    LOGI("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    LOGI("Extensions: %s\n", glGetString(GL_EXTENSIONS));
    printError("dumpInfo");
}

static GLenum lastError = 0;
static char lastErrorFunction[1024] = "";


const char* getGLErrorMsg(GLenum error) {
    switch (error) {
        case 0:
            return "NO_ERROR";
        case 0x0500:
            return "INVALID_ENUM";
        case 0x0501:
            return "INVALID_VALUE";
        case 0x0502:
            return "INVALID_OPERATION";
        case 0x0503:
            return "STACK_OVERFLOW";
        case 0x0504:
            return "STACK_UNDERFLOW";
        case 0x0505:
            return "OUT_OF_MEMORY";
        case 0x0506:
            return "INVALID_FRAMEBUFFER_OPERATION";
        default:
            return "UNKNOWN";
    }
}

/* report GL errors, if any, to stderr */
GLint printError(const char *functionName) {
    GLenum error;
    GLint wasError = 0;
    while ((error = glGetError()) != GL_NO_ERROR) {
        if ((lastError != error) || (strcmp(functionName, lastErrorFunction))) {
            LOGE("GL error %s detected in %s\n", getGLErrorMsg(error), functionName);
            strcpy(lastErrorFunction, functionName);
            lastError = error;
            wasError = 1;
        }
    }
    return wasError;
}


// Keymap mini manager
// Important! Uses glutKeyboardFunc/glutKeyboardUpFunc so you can't use them
// elsewhere or they will conflict.
// (This functionality is built-in in MicroGlut, as "glutKeyIsDown" where this conflict should not exist.)

char keymap[256];
char keydownmap[256];
char keyupmap[256];

char keyIsDown(unsigned char c) {
    return keymap[(unsigned int) c];
}

char keyPressed(unsigned char c) {
    char is_pressed = keydownmap[(unsigned int) c];
    keydownmap[(unsigned int) c] = 0;
    return is_pressed;
}

void keyUp(unsigned char key, int x, int y) {
    keymap[(unsigned int) key] = 0;
    keydownmap[(unsigned int) key] = 0;
    keyupmap[(unsigned int) key] = 1;
}

void keyDown(unsigned char key, int x, int y) {
    keymap[(unsigned int) key] = 1;
    keydownmap[(unsigned int) key] = keyupmap[(unsigned int) key] != 0;
    keyupmap[(unsigned int) key] = 0;
}

void initKeymapManager() {
    int i;
    for (i = 0; i < 256; i++) keymap[i] = 0;
    for (i = 0; i < 256; i++) keydownmap[i] = 0;
    for (i = 0; i < 256; i++) keyupmap[i] = 1;

//	glutKeyboardFunc(keyDown);
//	glutKeyboardUpFunc(keyUp);
}


// FBO

//----------------------------------FBO functions-----------------------------------
void CHECK_FRAMEBUFFER_STATUS() {
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        LOGE("Framebuffer not complete\n");
}

// create FBO
// FP buffer, suitable for HDR
FBOstruct *initFBO(int width, int height, int int_method) {
    FBOstruct *fbo = (FBOstruct *) malloc(sizeof(FBOstruct));

    if (fbo == NULL) {
        LOGE("initFBO could not allocate memory for FBO!\n");
        return NULL;
    }

    fbo->width = width;
    fbo->height = height;

    // create objects
    glGenFramebuffers(1, &fbo->fb); // frame buffer id
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
    glGenTextures(1, &fbo->texid);
    LOGE("%u \n", fbo->texid);
    glBindTexture(GL_TEXTURE_2D, fbo->texid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (int_method == 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->texid, 0);

    // Renderbuffer
    // initialize depth renderbuffer
    glGenRenderbuffers(1, &fbo->rb);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbo->width, fbo->height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo->rb);
    CHECK_FRAMEBUFFER_STATUS();

    LOGE("Framebuffer object %u\n", fbo->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

// create FBO, optionally with depth
// Integer buffer, not suitable for HDR!
FBOstruct *initFBO2(int width, int height, int int_method, int create_depthimage) {
    FBOstruct *fbo = (FBOstruct *) malloc(sizeof(FBOstruct));

    if (fbo == NULL) {
        LOGE("initFBO could not allocate memory for FBO!\n");
        return NULL;
    }

    fbo->width = width;
    fbo->height = height;

    // create objects
    glGenRenderbuffers(1, &fbo->rb);
    glGenFramebuffers(1, &fbo->fb); // frame buffer id
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->fb);
    glGenTextures(1, &fbo->texid);
    LOGE("%u \n", fbo->texid);
    glBindTexture(GL_TEXTURE_2D, fbo->texid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    if (int_method == 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo->texid, 0);
    if (create_depthimage != 0) {
        glGenTextures(1, &fbo->depth);
        glBindTexture(GL_TEXTURE_2D, fbo->depth);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT,
                     GL_UNSIGNED_BYTE, 0L);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo->depth, 0);
        LOGE("depthtexture: %u", fbo->depth);
    }

    // Renderbuffer
    // initialize depth renderbuffer
    glBindRenderbuffer(GL_RENDERBUFFER, fbo->rb);
    CHECK_FRAMEBUFFER_STATUS();

    LOGE("Framebuffer object %u\n", fbo->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

static int lastw = 0;
static int lasth = 0;

// Obsolete
void updateScreenSizeForFBOHandler(int w, int h) {
    lastw = w;
    lasth = h;
}

// choose input (textures) and output (FBO)
void useFBO(FBOstruct *out, FBOstruct *in1, FBOstruct *in2) {
    GLint curfbo;

// This was supposed to catch changes in viewport size and update lastw/lasth.
// It worked for me in the past, but now it causes problems to I have to
// fall back to manual updating.
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &curfbo);
    if (curfbo == 0) {
        GLint viewport[4] = {0, 0, 0, 0};
        GLint w, h;
        glGetIntegerv(GL_VIEWPORT, viewport);
        w = viewport[2] - viewport[0];
        h = viewport[3] - viewport[1];
        if ((w > 0) && (h > 0) && (w < 65536) &&
            (h < 65536)) // I don't believe in 64k pixel wide frame buffers for quite some time
        {
            lastw = viewport[2] - viewport[0];
            lasth = viewport[3] - viewport[1];
        }
    }

    if (out != 0L)
        glViewport(0, 0, out->width, out->height);
    else
        glViewport(0, 0, lastw, lasth);

    if (out != 0L) {
        glBindFramebuffer(GL_FRAMEBUFFER, out->fb);
        glViewport(0, 0, out->width, out->height);
    } else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE1);
    if (in2 != 0L)
        glBindTexture(GL_TEXTURE_2D, in2->texid);
    else
        glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    if (in1 != 0L)
        glBindTexture(GL_TEXTURE_2D, in1->texid);
    else
        glBindTexture(GL_TEXTURE_2D, 0);
}


// ===== Timer =====

Timer::Timer() {
    time = 0;
    lapTime = 0;
}

void Timer::startTimer() {
    startTime = myTime::now();
}

void Timer::endTimer() {
    GLfloat tempTime = fsec(myTime::now() - startTime).count();
    lapTime = tempTime - time;
    time = tempTime;
}

GLfloat Timer::getTime() {
    return time;
}

GLfloat Timer::getTimeMS() {
    return time * 1000.0f;
}

GLfloat Timer::getLapTime() {
    return lapTime;
}
