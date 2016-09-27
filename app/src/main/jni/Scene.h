///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#ifndef SCENE_H
#define SCENE_H

#include <GLES3/gl3.h>

#include <glm/glm.hpp>
#include "glm/gtc/type_ptr.hpp"

#include "Model.h"
#include "GL_utilities.h"

#include "tiny_obj_loader.h"

#define MAX_MIP_MAP_LEVELS 8
#define MAX_VOXEL_RES 256
// 256 ^ 3
#define MAX_SPARSE_BUFFER_SIZE 16777216

// Values the should exist on the GPU
struct SceneParam {
	glm::mat4 MTOmatrix[3]; // Centers and scales scene to fit inside +-1 from three different rotations
	glm::mat4 MTWmatrix; // Matrix for voxel data
	glm::mat4 MTShadowMatrix; // Matrix that transforms scene to lightview
	glm::vec3 lightDir;
	GLuint voxelDraw; // Which texture to draw
	GLuint view;
	GLuint voxelRes;
	GLuint voxelLayer;
	GLuint numMipLevels;
	GLuint mipLevel;
};

// Scene Options struct
struct SceneOptions {
	bool skipNoTexture;
	bool drawVoxels;
	bool drawModels;
	bool drawTextures;
	GLuint shadowRes;
};

// The different view directions
enum ViewDirection {
	VIEW_X,
	VIEW_Y,
	VIEW_Z
};

// Voxel Resolutions used
enum VoxelResolutions {
	RES16 = 16,
	RES32 = 32,
	RES64 = 64,
	RES128 = 128,
	RES256 = 256
};

// ===== ModelLoader class =====

class Scene {
private:
	// All models contained in the scene
	std::vector<Model *> *models;
	Model *voxelModel;

	// Programs used to draw models
	ShaderList *shaders;

	// Scene Options
	SceneOptions options;

	// Uniform buffer with scene settings
	SceneParam param;
	GLuint sceneBuffer;

	// Draw indirect buffer and struct
	DrawElementsIndirectCommand drawIndCmd[10];
	GLuint drawIndBuffer;

	// Compute indirect buffer and struct
	ComputeIndirectCommand compIndCmd[10];
	GLuint compIndBuffer;

	// Sparse List Buffer
	GLuint sparseListBuffer;

	// FBOs
	GLuint voxelFBO; //		Empty framebuffer for voxelization
	GLuint shadowFBO; //	Framebuffer with depth texture for shadowmap
	GLuint sceneFBO; //		Framebuffer for deferred rendering

	// Scene textures
	GLuint voxel2DTex;
	GLuint voxelTex;
	GLuint shadowTex;
	GLuint sceneTex[2]; // Color array and depth texture

	// Scene information
	glm::vec3 *maxVertex, *minVertex, centerVertex;
	GLfloat scale;

	// Init functions
	void InitBuffers();

	bool InitVoxel();

	// Setup functions
	void SetupDrawInd();

	void SetupCompInd();

	bool SetupScene(const char *path);

	void SetupVoxelTextures();

	void SetupShadowTexture();

	void SetupShadowMatrix();

	// Draw functions
	void DrawTextures();

	void DrawScene();

	void DrawVoxels();

public:
	Scene();

	bool Init(const char *path, ShaderList *initShaders);

	void CreateShadow();

	void RenderData();

	void Voxelize();

	void MipMap();

	void Draw();

	void UpdateBuffers();

	void SetupSceneTextures();

	SceneParam* GetSceneParam() { return &param; }
};

#endif // SCENE_H
