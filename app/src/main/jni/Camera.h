///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#ifndef CAMERA_H
#define CAMERA_H


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <GLES3/gl31.h>

// Uniform struct, needs to be arranged in multiples of 4 * 4 B for tight packing on GPU
struct CameraParam {
    glm::mat4 WTVmatrix;    // 16 * 4 B ->   0 -  15
    glm::mat4 VTPmatrix;    // 16 * 4 B ->  16 -  31
    glm::vec3 position;        //  3 * 4 B ->  32 -  34
};

class Camera {
private:
    glm::vec3 lookp, yvec;
    glm::vec3 heading, side, up;
    GLfloat mspeed, rspeed, phi, theta;
    GLfloat frustumFar;

    bool isPaused;
    bool needUpdate;

    GLuint cameraBuffer;
    GLint *winWidth, *winHeight;

    CameraParam param;

    void Update();

    void UploadParams();

public:
    Camera(glm::vec3 startpos, GLint *screenWidth, GLint *screenHeight, GLfloat farInit);

    bool Init();

    void SetFrustum();

    void ResetCamera(glm::vec3 pos);

    void MoveForward(GLfloat deltaT);

    void MoveRight(GLfloat deltaT);

    void MoveUp(GLfloat deltaT);

    void RotateCamera(GLint dx, GLint dy);

    void UpdateCamera();

    void TogglePause() { isPaused = !isPaused; }

    GLfloat *GetSpeedPtr() { return &mspeed; }

    GLfloat *GetRotSpeedPtr() { return &rspeed; }

    CameraParam *GetCameraInfo() { return &param; }
};

#endif // CAMERA_H
