///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#include "Program.h"
#include "../../../../../../../../Android/android-sdk/ndk-bundle/platforms/android-23/arch-arm/usr/include/android/asset_manager.h"


Program::Program() {
    // Set all pointers to null
    cam = NULL;

    // Window init size
    winWidth = 0;
    winHeight = 0;

    // Time init
    time.startTimer();

    // Set program parameters
    cameraStartPos = glm::vec3(0.0, 0.0, 2.0);
    cameraFrustumFar = 500.0f;

    sceneSelect = 0;
    useOrtho = false;
    drawVoxelOverlay = false;
}

Program::~Program() {
    Clean();
}

void Program::Step() {
    LOGD("Step called");

    Update();
    Render();
}

void Program::Resize(int width, int height) {
    LOGD("Resize called");

    winWidth = width;
    winHeight = height;
    glViewport(0,0,width,height);
    cam->SetFrustum();
    GetCurrentScene()->SetupSceneTextures();
}

/*
int Program::Execute() {
	if(!Init()) {
		std::cout << "\nInit failed. Press enter to quit ..." << std::endl;
		getchar();
		return -1;
	}

	SDL_Event Event;

	while(isRunning) {
		timeUpdate();
		while(SDL_PollEvent(&Event)) OnEvent(&Event);
		CheckKeyDowns();
		Update();
		Render();
	}

	Clean();

	return 0;
}*/
/*
void Program::timeUpdate() {
    time.endTimer();
    param.deltaT = time.getLapTime();
    param.currentT = time.getTime();
    FPS = 1.0f / time.getLapTime();
}
*/
/*
void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam) {
	(void)source; (void)type; (void)id;
	(void)severity; (void)length; (void)userParam;

	switch(id) {
		case 131185: // Buffer detailed info
			return;
		case 131218: // Nvidia Shader complilation performance warning
			return;
		case 131186: // Memory copied from Device to Host
			return;		
		case 131204: // Texture state 0 is base level inconsistent
			return;
		case 131076: // Vertex attribute optimized away
			return;
		default:
			break;
	}
	
	fprintf(stderr, "ID: %d\n %s\n\n",id, message);
	if(severity == GL_DEBUG_SEVERITY_HIGH) {
		fprintf(stderr, "Aborting...\n");
		abort();
	}
}*/

bool Program::Init() {
    LOGD("Init called");

/*
#ifdef DEBUG
	// Enable the debug callback
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);
#endif // DEBUG
*/
    // Activate depth test and blend for masking textures
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    //glEnable(GL_TEXTURE_3D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.2f, 0.4f, 1.0f);

    dumpInfo();

    glGenBuffers(1, &programBuffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, PROGRAM, programBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProgramStruct), &param, GL_STREAM_DRAW);

    printError("Init program buffer");


    // Load shaders for drawing
    shaders.drawScene = loadShaders(SHADER_PATH("drawModel.vert"), SHADER_PATH("drawModel.frag"));
    shaders.drawData = loadShaders(SHADER_PATH("drawData.vert"), SHADER_PATH("drawData.frag"));

    // Load shaders for voxelization
    shaders.voxelize = loadShadersG(SHADER_PATH("voxelization.vert"),
                                    SHADER_PATH("voxelization.frag"),
                                    SHADER_PATH("voxelization.geom"));

    // Single triangle shader for deferred shading etc.
    shaders.singleTriangle = loadShaders(SHADER_PATH("drawTriangle.vert"),
                                         SHADER_PATH("drawTriangle.frag"));

    // Draw voxels from 3D texture
    shaders.voxel = loadShaders(SHADER_PATH("drawVoxel.vert"), SHADER_PATH("drawVoxel.frag"));

    // Calculate mipmaps
    shaders.mipmap = CompileComputeShader(SHADER_PATH("mipmap.comp"));

    // Create shadowmap
    shaders.shadowMap = loadShaders(SHADER_PATH("shadowMap.vert"), SHADER_PATH("shadowMap.frag"));

    printError("Load shaders");

    // TODO: Make this a separate function
    // Set constant uniforms for the drawing programs
    glUseProgram(shaders.drawScene);
    glUniform1i(DIFF_UNIT, 0);
    glUniform1i(MASK_UNIT, 1);
    printError("Set drawScene Constants");

    // Set constant uniforms for voxel programs
    glUseProgram(shaders.voxelize);
    printError("Set voxelize Program");
    glUniform1i(DIFF_UNIT, 0);
    printError("Set voxelize Constants 1");
    glUniform1i(VOXEL_TEXTURE, 2);
    printError("Set voxelize Constants 2");
    glUniform1i(VOXEL_DATA, 3);
    printError("Set voxelize Constants 3");
    glUniform1i(SHADOW_UNIT, 5);
    printError("Set voxelize Constants 4");

    // Set constant uniforms for simple triangle drawing
    glUseProgram(shaders.singleTriangle);
    glUniform1i(VOXEL_TEXTURE, 2);
    glUniform1i(VOXEL_DATA, 3);
    glUniform1i(SHADOW_UNIT, 5);
    glUniform1i(SCENE_UNIT, 6);
    glUniform1i(SCENE_DEPTH, 7);
    printError("Set singleTriangle Constants");

    // Set constant uniforms for drawing the voxel overlay
    glUseProgram(shaders.voxel);
    glUniform1i(VOXEL_DATA, 3);
    printError("Set voxel Constants");

    // Set constant uniforms for calculating mipmaps
    glUseProgram(shaders.mipmap);
    printError("Set mipmap Program");
    glUniform1i(VOXEL_DATA, 3);
    printError("Set mipmap Constants 1");
    glUniform1i(VOXEL_DATA_NEXT, 4);
    printError("Set mipmap Constants 2");


    // Set up the camera
    cam = new Camera(cameraStartPos, &winWidth, &winHeight, cameraFrustumFar);
    if (!cam->Init()) {
        LOGE("Camera not initialized!");
        return false;
    }

    printError("Init Camera");

    // Load scenes
    Scene* cornell = new Scene();
    if(!cornell->Init(MODEL_PATH("cornell.obj"), &shaders)){
        LOGE("Error loading scene: Cornell!");
        return false;
    }
    scenes.push_back(cornell);

    printError("Init Scene Cornell");


//    Scene *sponza = new Scene();
//    if (!sponza->Init(MODEL_PATH("sponza.obj"), &shaders)) return false;
//    scenes.push_back(sponza);

    // Initial Voxelization of the scenes
//    cornell->CreateShadow();
//    cornell->RenderData();
//    cornell->Voxelize();
//    cornell->MipMap();

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
    cam->UpdateCamera();
}

void Program::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//    GetCurrentScene()->CreateShadow();
//    GetCurrentScene()->RenderData();
//    GetCurrentScene()->Voxelize();
//    GetCurrentScene()->MipMap();
    GetCurrentScene()->Draw();
}

void Program::Clean() {
    glDeleteBuffers(1, &programBuffer);
}

void Program::UploadParams() {
    // Update program parameters
    glBindBufferBase(GL_UNIFORM_BUFFER, PROGRAM, programBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(ProgramStruct), &param);
}
/*
void Program::OnEvent(SDL_Event *Event) {
	switch(Event->type) {
		case SDL_QUIT:
			isRunning = false;
			break;
		case SDL_WINDOWEVENT:
			switch(Event->window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					SDL_SetWindowSize(screen, Event->window.data1, Event->window.data2);
					SDL_GetWindowSize(screen, &winWidth, &winHeight);
					glViewport(0, 0, winWidth, winHeight);
					TwWindowSize(winWidth, winHeight);
					cam->SetFrustum();
					GetCurrentScene()->SetupSceneTextures();
					break;
			}
		case SDL_KEYDOWN:
			OnKeypress(Event);
			break;
		case SDL_MOUSEMOTION:
			OnMouseMove(Event);
			break;
		case SDL_MOUSEBUTTONDOWN:
			TwMouseButton(TW_MOUSE_PRESSED, TW_MOUSE_LEFT);
			break;
		case SDL_MOUSEBUTTONUP:
			TwMouseButton(TW_MOUSE_RELEASED, TW_MOUSE_LEFT);
			break;
		default:
			break;
	}
}

void Program::OnKeypress(SDL_Event *Event) {
	TwKeyPressed(Event->key.keysym.sym, TW_KMOD_NONE);
	switch(Event->key.keysym.sym) {
		case SDLK_ESCAPE:
			isRunning = false;
			break;
		case SDLK_SPACE:
			break;
		case SDLK_f:
			cam->TogglePause();
			SDL_SetRelativeMouseMode(SDL_GetRelativeMouseMode() ? SDL_FALSE : SDL_TRUE);
			break;
		case SDLK_g:
			int isBarHidden;
			TwGetParam(antBar, NULL, "iconified", TW_PARAM_INT32, 1, &isBarHidden);
			if(isBarHidden) {
				TwDefine(" VCT iconified=false ");
			} else {
				TwDefine(" VCT iconified=true ");
			}
			break;
		default:
			break;
	}
}

void Program::OnMouseMove(SDL_Event *Event) {
	if(!SDL_GetRelativeMouseMode())
		TwMouseMotion(Event->motion.x, Event->motion.y);
	else
		cam->RotateCamera(Event->motion.xrel, Event->motion.yrel);
}

void Program::CheckKeyDowns() {
	const Uint8 *keystate = SDL_GetKeyboardState(NULL);
	if(keystate[SDL_SCANCODE_W]) {
		cam->MoveForward(param.deltaT);
	}
	if(keystate[SDL_SCANCODE_S]) {
		cam->MoveForward(-param.deltaT);
	}
	if(keystate[SDL_SCANCODE_A]) {
		cam->MoveRight(-param.deltaT);
	}
	if(keystate[SDL_SCANCODE_D]) {
		cam->MoveRight(param.deltaT);
	}
	if(keystate[SDL_SCANCODE_Q]) {
		cam->MoveUp(param.deltaT);
	}
	if(keystate[SDL_SCANCODE_E]) {
		cam->MoveUp(-param.deltaT);
	}
}*/

void Program::SetAssetMgr(AAssetManager *mgr) {
    SetAssetsManager(mgr);
}
