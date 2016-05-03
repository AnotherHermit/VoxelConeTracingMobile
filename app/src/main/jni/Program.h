///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#ifndef PROGRAM_H
#define PROGRAM_H

#include <GLES3/gl3.h>

#include "Camera.h"
#include "Scene.h"

#include "common/GL_utilities.h"

struct ProgramStruct {
    GLfloat currentT;
    GLfloat deltaT;
};

class Program {
private:
    GLint winWidth, winHeight;

    bool isRunning;

    Timer time;
    GLfloat FPS;
    ProgramStruct param;
    GLuint programBuffer;

    bool useOrtho;
    bool drawVoxelOverlay;
    Camera *cam;

    // Shaders
    ShaderList shaders;

    // Model
    GLuint sceneSelect;
    std::vector<Scene *> scenes;

    // Program params
    glm::vec3 cameraStartPos;
    GLfloat cameraFrustumFar;

    // Methods
    void UploadParams();

    Scene *GetCurrentScene() { return scenes[sceneSelect]; }

public:
    Program();

    int Execute();

    void timeUpdate();

    bool Init();

    //void OnEvent(SDL_Event *Event);
    //void OnKeypress(SDL_Event *Event);
    //void OnMouseMove(SDL_Event *Event);
    //void CheckKeyDowns();

    void Update();

    void Render();

    void Clean();

};

#endif // PROGRAM_H
