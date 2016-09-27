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

#include "GL_utilities.h"
#include "../../../../../../../../Android/android-sdk/ndk-bundle/platforms/android-23/arch-arm/usr/include/android/asset_manager.h"

struct ProgramStruct {
	GLfloat currentT;
	GLfloat deltaT;
};

class Program {
private:
	GLint winWidth, winHeight;

	Timer time;
//    GLTimer glTime;
	GLfloat FPS;
	ProgramStruct param;
	GLuint programBuffer;

	bool useOrtho;
	bool drawVoxelOverlay;
	OrbitCamera *cam;

	// Shaders
	ShaderList shaders;

	// Model
	GLuint sceneSelect;
	std::vector<Scene *> scenes;

	// Program params
	//glm::vec3 cameraStartPos;
	GLfloat cameraStartDistance;
	GLfloat cameraStartAzimuth;
	GLfloat cameraStartPolar;
	glm::vec3 cameraStartTarget;
	GLfloat cameraFrustumFar;

	// Methods
	void UploadParams();

	Scene *GetCurrentScene() { return scenes[sceneSelect]; }

public:
	Program();
	~Program();

	void SetAssetMgr(AAssetManager *mgr);
	bool Init();
	void Resize(int width, int height);
	void Step();

	// Camera controlls
	void Pan(float dx, float dy);
	void Zoom(float scale);
	void ToggleProgram();

	//void timeUpdate();
	//void OnEvent(SDL_Event *Event);
	//void OnKeypress(SDL_Event *Event);
	//void OnMouseMove(SDL_Event *Event);
	//void CheckKeyDowns();

	void Update();
	void Render();

	void Clean();

};

#endif // PROGRAM_H
