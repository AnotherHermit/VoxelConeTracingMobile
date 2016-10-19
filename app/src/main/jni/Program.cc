///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#include "Program.h"

Program::Program() {
	// Set all pointers to null
	cam = NULL;

	// Window init size
	winWidth = 0;
	winHeight = 0;

	// Time init
	time.startTimer();

	// Set program parameters
	//cameraStartPos = glm::vec3(0.0f, 0.0f, 3.0f);
	cameraStartDistance = 2.9f;
	cameraStartAzimuth = (GLfloat) M_PI_2;//(GLfloat) M_PI_2; (GLfloat) M_PI_4;
	cameraStartPolar = (GLfloat) M_PI / 2.3f;//(GLfloat) M_PI / 2.3f; (GLfloat) M_PI / 1.8f;
	cameraStartTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	cameraStartFov = 60.0f;
	cameraFrustumFar = 20.0f;

	sceneSelect = 0;
	useOrtho = false;
	drawVoxelOverlay = false;
	scrollLight = false;
	takeTime = 0;
	runNumber = 0;
	runScene = 0;
	sceneAverage[0] = 0.0f;
	sceneAverage[1] = 0.0f;
	sceneAverage[2] = 0.0f;
	sceneAverage[3] = 0.0f;
	sceneAverage[4] = 0.0f;
	sceneStatic[0] = 0.0f;
	sceneStatic[1] = 0.0f;
}

Program::~Program() {
	Clean();
}

void Program::Step() {
	//LOGD("Step called");
//    glTime.startTimer();
	Update();
//    glTime.endTimer();
//    LOGD("Update time: %f ms", glTime.getTimeMS());
//    glTime.startTimer();
	Render();
//    glTime.endTimer();
//    LOGD("Draw time: %f ms", glTime.getTimeMS());
}

void Program::Resize(int width, int height) {
	winWidth = width;
	winHeight = height;
	glViewport(0, 0, width, height);
	cam->Resize();
	GetCurrentScene()->SetupSceneTextures();
//    GetCurrentScene()->Voxelize();
}

/*
void Program::timeUpdate() {
    time.endTimer();
    param.deltaT = time.getLapTime();
    param.currentT = time.getTime();
    FPS = 1.0f / time.getLapTime();
}
*/

void Program::Scroll(float dx, float dy) {
	if (scrollLight) GetCurrentScene()->PanLight(dx,dy);
	else cam->Rotate(dx, dy);
}

void Program::Scale(float scale) {
	cam->Zoom(scale);
}

void Program::ToggleProgram() {
	runScene = GetCurrentScene()->GetSceneParam()->voxelDraw;
	runScene++;
	runScene %= 10;
	GetCurrentScene()->GetSceneParam()->voxelDraw = runScene;
	runNumber = 0;
	sceneNum = runScene / 2 + 1;
	sceneAverage[0] = 0.0f;
	sceneAverage[1] = 0.0f;
	sceneAverage[2] = sceneNum > 2 ? sceneStatic[0] : 0.0f;
	sceneAverage[3] = sceneNum > 2 ? sceneStatic[1] : 0.0f;
	sceneAverage[4] = 0.0f;
	if (runScene == 0 && takeTime) {
		takeTime++;
	}

	if (takeTime > 3) {
		takeTime = 0;
	}
}

void Program::ToggleLightTouch() {
	scrollLight = !scrollLight;
}

bool Program::Init() {
	LOGD("Init called");

	// Activate depth test and blend for masking textures
	GL_CHECK(glEnable(GL_DEPTH_TEST));
	GL_CHECK(glEnable(GL_CULL_FACE));
	//glEnable(GL_TEXTURE_3D);
	GL_CHECK(glEnable(GL_BLEND));
	GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
	//GL_CHECK(glClearDepthf(0.0f));

	dumpInfo();

	GL_CHECK(glGenBuffers(1, &programBuffer));
	GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, PROGRAM, programBuffer));
	GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(ProgramStruct), &param, GL_STREAM_DRAW));

	// Load shaders for drawing
	shaders.drawScene = loadShaders(SHADER_PATH("drawModel.vert"),
																	SHADER_PATH("drawModel.frag"));
	shaders.drawData = loadShaders(SHADER_PATH("drawData.vert"),
																 SHADER_PATH("drawData.frag"));

	// Load shaders for voxelization
	shaders.voxelize = loadShadersG(SHADER_PATH("voxelization.vert"),
																	SHADER_PATH("voxelization.frag"),
																	SHADER_PATH("voxelization.geom"));

	// Single triangle shader for deferred shading etc.
	shaders.singleTriangle = loadShaders(SHADER_PATH("drawTriangle.vert"),
																			 SHADER_PATH("drawTriangle.frag"));

	// Draw voxels from 3D texture
	shaders.voxel = loadShaders(SHADER_PATH("drawVoxel.vert"),
															SHADER_PATH("drawVoxel.frag"));

	// Calculate mipmaps
	shaders.mipmap = CompileComputeShader(SHADER_PATH("mipmap.comp"));

	// Create shadowmap
	shaders.shadowMap = loadShaders(SHADER_PATH("shadowMap.vert"),
																	SHADER_PATH("shadowMap.frag"));

	InitTimer();

	// Set up the camera
	cam = new OrbitCamera();
	if (!cam->Init(cameraStartTarget, cameraStartDistance, cameraStartPolar, cameraStartAzimuth, cameraStartFov, &winWidth, &winHeight, cameraFrustumFar)) {
		LOGE("Camera not initialized!");
		return false;
	}

	// Load Asset folders
	LoadAssetFolder("models");

	// Load scenes
	Scene *cornell = new Scene();
	if (!cornell->Init(MODEL_PATH("cornell.obj"), &shaders)) {
		LOGE("Error loading scene: Cornell!");
		return false;
	}
	scenes.push_back(cornell);

//    Scene *sponza = new Scene();
//    if (!sponza->Init(MODEL_PATH("sponza.obj"), &shaders)) return false;
//    scenes.push_back(sponza);


//	GL_CHECK(glFinish());
	// Initial Voxelization of the scenes
//	time.startTimer();
	cornell->CreateShadow();
//	GL_CHECK(glFinish());
//	time.endTimer();
//	LOGD("Create Shadow time: %f ms", time.getTimeMS());

//	time.startTimer();
	cornell->RenderData();
	GL_CHECK(glFinish());
//	time.endTimer();
//	LOGD("Render Data time: %f ms", time.getTimeMS());

	time.startTimer();
	cornell->Voxelize();
	GL_CHECK(glFinish());
	time.endTimer();
	sceneStatic[0] = time.getTimeMS();

	time.startTimer();
	cornell->MipMap();
	GL_CHECK(glFinish());
	time.endTimer();
	sceneStatic[1] = time.getTimeMS();


	LOGD("Time per step logging");
	LOGD("Scene\t  CSA  \t  RDA  \t  V    \t  M    \t  DA");

//    sponza->CreateShadow();
//    sponza->RenderData();
//    sponza->Voxelize();
//    sponza->MipMap();

	return true;
}

void Program::Update() {
	// Upload program params (incl time update)
	UploadParams();

	// Update the camera
	cam->Update(param.deltaT);
}

void Program::Render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (runNumber < 5 && runScene % 2 && takeTime) {
		GL_CHECK(glFinish());
		time.startTimer();
	}

	GetCurrentScene()->CreateShadow();

	if (runNumber < 5 && runScene % 2 && takeTime) {
		GL_CHECK(glFinish());
		time.endTimer();
		sceneAverage[0] += time.getTimeMS() / 5.0f;

		time.startTimer();
	}

	GetCurrentScene()->RenderData();

	if (runNumber < 5 && runScene % 2 && takeTime) {
		GL_CHECK(glFinish());
		time.endTimer();
		sceneAverage[1] += time.getTimeMS() / 5.0f;

		time.startTimer();
	}

//	GetCurrentScene()->Voxelize();
//
//	if (runNumber < 5 && runScene % 2 && takeTime) {
//		GL_CHECK(glFinish());
//		time.endTimer();
//		LOGD("Voxelize time: %f ms", time.getTimeMS());
//
//		time.startTimer();
//	}
//
//	GetCurrentScene()->MipMap();
//
//	if (runNumber < 5 && runScene % 2 && takeTime) {
//		GL_CHECK(glFinish());
//		time.endTimer();
//		LOGD("Mipmap time: %f ms", time.getTimeMS());
//
//		time.startTimer();
//	}

	GetCurrentScene()->Draw();

	if (runNumber < 5 && runScene % 2 && takeTime) {
		GL_CHECK(glFinish());
		time.endTimer();
		sceneAverage[4] += time.getTimeMS() / 5.0f;
		runNumber++;
	}

	if (runNumber == 5 && runScene % 2 && takeTime) {
		LOGD("%4d \t%7.3f\t%7.3f\t%7.3f\t%7.3f\t%7.3f", sceneNum, sceneNum == 1 ? 0.0f : sceneAverage[0], sceneAverage[1], sceneAverage[2], sceneAverage[3], sceneAverage[4]);
		ToggleProgram();
	} else if (runScene % 2 == 0 && takeTime) {
		ToggleProgram();
	}
}

void Program::Clean() {
	GL_CHECK(glDeleteBuffers(1, &programBuffer));
}

void Program::UploadParams() {
	// Update program parameters
	GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, PROGRAM, programBuffer));
	GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(ProgramStruct), &param));
}

void Program::SetAssetMgr(AAssetManager *mgr) {
	SetAssetsManager(mgr);
}



