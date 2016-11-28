///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#include "Scene.h"

#include "ModelLoader.h"

#include <glm/gtx/transform.hpp>

#include <iostream>
#include <sstream>

Scene::Scene() {
	options.skipNoTexture = false;
	options.drawTextures = true;
	options.drawModels = false;
	options.drawVoxels = false;
	options.shadowRes = 512;

	param.lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
	param.voxelRes = RES256;
	param.voxelLayer = 0;
	param.voxelDraw = 4;
	param.view = VIEW_X;
	param.numMipLevels = (GLuint) log2(param.voxelRes);
	param.mipLevel = 0;

	maxVertex = nullptr;
	minVertex = nullptr;

	models = new std::vector<Model *>();
	voxelModel = new Model();

	voxel2DTex = 0;
	voxelTex = 0;
	shadowTex = 0;
	sceneTex[0] = 0;
	sceneTex[1] = 0;

	sceneFBO = 0;
	shadowFBO = 0;
	voxelFBO = 0;
}

bool Scene::Init(const char *path, ShaderList *initShaders) {

	shaders = initShaders;

	InitBuffers();
//    InitMipMap();
	if (!SetupScene(path)) return false;
	if (!InitVoxel()) return false;

	SetupDrawInd();
	SetupCompInd();
	SetupVoxelTextures();
	SetupSceneTextures();
	SetupShadowTexture();
	SetupShadowMatrix();

	UpdateBuffers();

	return true;
}

void Scene::InitBuffers() {
	// Init the framebuffer for drawing
	GL_CHECK(glGenFramebuffers(1, &voxelFBO));

	// Set non-constant uniforms for all programs
	GL_CHECK(glGenBuffers(1, &sceneBuffer));
	GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, SCENE, sceneBuffer));
	GL_CHECK(glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneParam), NULL, GL_STREAM_DRAW));

	// Set up the sparse active voxel buffer
	GL_CHECK(glGenBuffers(1, &sparseListBuffer));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPARSE_LIST, sparseListBuffer));
	GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, (sizeof(GLuint) * MAX_SPARSE_BUFFER_SIZE), NULL, GL_STREAM_DRAW));
}

bool Scene::SetupScene(const char *path) {
	ModelLoader modelLoader;
	if (!modelLoader.LoadScene(path, models, &maxVertex, &minVertex)) {
		LOGE("Failed to load scene: %s\n", path);
		return false;
	}

	// Calculate the scaling of the scene
	glm::vec3 diffVector = (*maxVertex - *minVertex);
	centerVertex = diffVector / 2.0f + *minVertex;
	scale = glm::max(diffVector.x, glm::max(diffVector.y, diffVector.z));

	// Set the matrices for looking at the scene in three different ways
	param.MTOmatrix[2] = glm::scale(glm::vec3(1.999999f / scale)) * glm::translate(-centerVertex);
	param.MTOmatrix[0] = glm::rotate(-glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)) * param.MTOmatrix[2];
	param.MTOmatrix[1] = glm::rotate(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) * param.MTOmatrix[2];

	param.MTWmatrix = glm::inverse(param.MTOmatrix[2]);

	return true;
}

bool Scene::InitVoxel() {
	// Load a model for drawing the voxel
	ModelLoader modelLoader;
	if (!modelLoader.LoadModel(MODEL_PATH("voxelLarge.obj"), voxelModel)) {
		LOGE("Failed to load voxel model");
		return false;
	}

	voxelModel->SetPositionData(sparseListBuffer);

	return true;
}

void Scene::SetupDrawInd() {
	// Initialize the indirect drawing buffer
	for (int i = MAX_MIP_MAP_LEVELS, j = 0; i >= 0; i--, j++) {
		drawIndCmd[i].vertexCount = (GLuint) voxelModel->GetNumIndices();
		drawIndCmd[i].instanceCount = 0;
		drawIndCmd[i].firstVertex = 0;
		drawIndCmd[i].baseVertex = 0;

		if (i == 0) {
			drawIndCmd[i].baseInstance = 0;
		} else if (i == MAX_MIP_MAP_LEVELS) {
			drawIndCmd[i].baseInstance = MAX_SPARSE_BUFFER_SIZE - 1;
		} else {
			drawIndCmd[i].baseInstance = drawIndCmd[i + 1].baseInstance - (1 << (3 * j));
		}
	}

	// Draw Indirect Command buffer for drawing voxels
	GL_CHECK(glGenBuffers(1, &drawIndBuffer));
	GL_CHECK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndBuffer));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_IND, drawIndBuffer));
	GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(drawIndCmd), drawIndCmd, GL_STREAM_DRAW));
}

void Scene::SetupCompInd() {
	// Initialize the indirect compute buffer
	for (size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++) {
		compIndCmd[i].workGroupSizeX = 0;
		compIndCmd[i].workGroupSizeY = 1;
		compIndCmd[i].workGroupSizeZ = 1;
	}

	// Draw Indirect Command buffer for drawing voxels
	GL_CHECK(glGenBuffers(1, &compIndBuffer));
	GL_CHECK(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, compIndBuffer));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COMPUTE_IND, compIndBuffer));
	GL_CHECK(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(compIndCmd), compIndCmd, GL_STREAM_DRAW));
}

void Scene::SetupVoxelTextures() {
	// Generate textures for render to texture, only for debugging purposes
	if (voxel2DTex != 0) {
		GL_CHECK(glDeleteTextures(1, &voxel2DTex));
	}
	GL_CHECK(glGenTextures(1, &voxel2DTex));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, voxel2DTex));
	GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, param.voxelRes, param.voxelRes, 3));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	// Create the 3D texture that contains the voxel data
	if (voxelTex != 0) {
		GL_CHECK(glDeleteTextures(1, &voxelTex));
	}
	GL_CHECK(glGenTextures(1, &voxelTex));
	GL_CHECK(glBindTexture(GL_TEXTURE_3D, voxelTex));
	GL_CHECK(glTexStorage3D(GL_TEXTURE_3D, param.numMipLevels + 1, GL_R32UI, param.voxelRes, param.voxelRes, param.voxelRes));
	GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
	GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	// TODO: Should be GL_CLAMP_TO_BORDER, since edge should not be repeated
	GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
}

void Scene::SetupSceneTextures() {
	GLint origViewportSize[4];
	GL_CHECK(glGetIntegerv(GL_VIEWPORT, origViewportSize));

	if (sceneTex[0] != 0) {
		GL_CHECK(glDeleteTextures(2, sceneTex));
	}

	GL_CHECK(glGenTextures(2, sceneTex));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTex[0]));
	GL_CHECK(glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16F, origViewportSize[2], origViewportSize[3], 4));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GL_CHECK(glBindTexture(GL_TEXTURE_2D, sceneTex[1]));
	GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, origViewportSize[2], origViewportSize[3]));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));

	if (sceneFBO == 0) {
		GL_CHECK(glGenFramebuffers(1, &sceneFBO));
	}
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO));
	GL_CHECK(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sceneTex[0], 0, 0));
	GL_CHECK(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, sceneTex[0], 0, 1));
	GL_CHECK(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, sceneTex[0], 0, 2));
	GL_CHECK(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, sceneTex[0], 0, 3));
	GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneTex[1], 0));

	GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
	GL_CHECK(glDrawBuffers(4, DrawBuffers));

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("Framebuffer error");
	}

	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Scene::SetupShadowTexture() {

	GL_CHECK(glGenTextures(1, &shadowTex));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, shadowTex));
	GL_CHECK(glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, options.shadowRes, options.shadowRes));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));

	GL_CHECK(glGenFramebuffers(1, &shadowFBO));
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO));

	GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0));

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("Framebuffer error");
	}

	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Scene::SetupShadowMatrix() {
	glm::vec3 o = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 y = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 l = glm::normalize(param.lightDir);

	param.MTShadowMatrix = glm::lookAt(o, l, y) * glm::scale(glm::vec3(1.0f / sqrtf(3.0f)));

	// TODO: Only update the buffer after light direction has actually changed
	// Upload new params to GPU
	GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, SCENE, sceneBuffer));
	GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(SceneParam), &param));
}

void Scene::UpdateBuffers() {
	// Upload new params to GPU
	GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, SCENE, sceneBuffer));
	GL_CHECK(glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(SceneParam), &param));

	GL_CHECK(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndBuffer));
	GL_CHECK(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, compIndBuffer));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_IND, drawIndBuffer));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COMPUTE_IND, compIndBuffer));
	GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPARSE_LIST, sparseListBuffer));
}

void Scene::CreateShadow() {
	// Update the direction of the light
	SetupShadowMatrix();

	// Setup framebuffer for rendering offscreen
	GLint origViewportSize[4];
	GL_CHECK(glGetIntegerv(GL_VIEWPORT, origViewportSize));
	GL_CHECK(glViewport(0, 0, options.shadowRes, options.shadowRes));

	// Enable rendering to framebuffer with shadow map resolution
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO));

	// Clear the last shadow map
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	// Light should also hit backsides (especially for cornell)
	GL_CHECK(glDisable(GL_CULL_FACE));

	GL_CHECK(glUseProgram(shaders->shadowMap));

	// Create the shadow map texture
	for (auto model = models->begin(); model != models->end(); model++) {

		// Don't draw models without texture
		if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
			continue;
		}

		(*model)->ShadowMap();

	}

	// Restore the framebuffer
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_CHECK(glViewport(origViewportSize[0], origViewportSize[1], origViewportSize[2], origViewportSize[3]));
}

void Scene::RenderData() {
	// Enable rendering to framebuffer
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO));
	// Clear the last scene data
	GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

	GL_CHECK(glEnable(GL_CULL_FACE));

	GL_CHECK(glUseProgram(shaders->drawData));

	// Create the shadow map texture
	for (auto model = models->begin(); model != models->end(); model++) {

		// Don't draw models without texture
		if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
			continue;
		}

		(*model)->Draw();
	}

	// Restore the framebuffer
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void Scene::Voxelize() {
	// Setup framebuffer for rendering offscreen
	GLint origViewportSize[4];
	GL_CHECK(glGetIntegerv(GL_VIEWPORT, origViewportSize));

	// Enable rendering to framebuffer with voxelRes resolution
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, voxelFBO));
	GL_CHECK(glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, param.voxelRes));
	GL_CHECK(glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, param.voxelRes));
	GL_CHECK(glViewport(0, 0, param.voxelRes, param.voxelRes));

	// Clear the last voxelization data
	// TODO: Clear the textures somehow (only interesting for dynamic updates)
	/*GL_CHECK(glClearTexImage(voxel2DTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL));
	for(size_t i = 0; i <= param.numMipLevels; i++) {
					GL_CHECK(glClearTexImage(voxelTex, (GLint)i, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL));
	}*/

	GLuint reset = 0;
	// Reset the sparse voxel count
	GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawIndBuffer));
	for (size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++) {
		GL_CHECK(glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sizeof(DrawElementsIndirectCommand) + sizeof(GLuint), sizeof(GLuint), &reset)); // Clear data before since data is used when drawing
	}

	// Reset the sparse voxel count for compute shader
	GL_CHECK(glBindBuffer(GL_SHADER_STORAGE_BUFFER, compIndBuffer));
	for (size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++) {
		GL_CHECK(glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sizeof(ComputeIndirectCommand), sizeof(GLuint), &reset)); // Clear data before since data is used when drawing
	}

	// Bind the textures used to hold the voxelization data
	GL_CHECK(glBindImageTexture(2, voxel2DTex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI));
	GL_CHECK(glBindImageTexture(3, voxelTex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI));

	GL_CHECK(glActiveTexture(GL_TEXTURE5));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, shadowTex));

	// All faces must be rendered
	GL_CHECK(glDisable(GL_CULL_FACE));

	GL_CHECK(glUseProgram(shaders->voxelize));

	for (auto model = models->begin(); model != models->end(); model++) {
		// Don't draw models without texture if set to skip
		if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
			continue;
		}

		(*model)->Voxelize();

		GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
	}
	GL_CHECK(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));

	// Restore the framebuffer
	GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	GL_CHECK(glViewport(origViewportSize[0], origViewportSize[1], origViewportSize[2], origViewportSize[3]));

	//GLint *temp = (GLint *) GL_CHECK(glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(GLint) * 5, GL_MAP_READ_BIT));
	//LOGD("Draw Indirect Buffer: %i, %i, %i, %i, %i", temp[0], temp[1], temp[2], temp[3], temp[4]);
	//GL_CHECK(glUnmapBuffer(GL_DRAW_INDIRECT_BUFFER));

}

void Scene::MipMap() {
	GL_CHECK(glUseProgram(shaders->mipmap));

	for (GLuint level = 0; level < param.numMipLevels; level++) {
		GL_CHECK(glBindImageTexture(3, voxelTex, level, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI));
		GL_CHECK(glBindImageTexture(4, voxelTex, level + 1, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI));

		GL_CHECK(glUniform1ui(CURRENT_LEVEL, level));

		GL_CHECK(glDispatchComputeIndirect(NULL));

		GL_CHECK(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT));

		//GLint *temp = (GLint *) GL_CHECK(glMapBufferRange(GL_DISPATCH_INDIRECT_BUFFER, sizeof(ComputeIndirectCommand) * level, sizeof(ComputeIndirectCommand), GL_MAP_READ_BIT));
		//LOGD("Compute Indirect Buffer level %i: %i, %i, %i", level, temp[0], temp[1], temp[2]);
		//GL_CHECK(glUnmapBuffer(GL_DISPATCH_INDIRECT_BUFFER));
	}
	GL_CHECK(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT));
}

void Scene::Draw() {
	if (options.drawTextures) DrawTextures();
	else {
		if (options.drawModels) DrawScene();
		if (options.drawVoxels) DrawVoxels();
	}
}

void Scene::DrawTextures() {
	GL_CHECK(glUseProgram(shaders->singleTriangle));
	GL_CHECK(glBindVertexArray(0));

	GL_CHECK(glActiveTexture(GL_TEXTURE2));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, voxel2DTex));
	GL_CHECK(glActiveTexture(GL_TEXTURE3));
	GL_CHECK(glBindTexture(GL_TEXTURE_3D, voxelTex));
	GL_CHECK(glActiveTexture(GL_TEXTURE5));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, shadowTex));
	GL_CHECK(glActiveTexture(GL_TEXTURE6));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTex[0]));
	GL_CHECK(glActiveTexture(GL_TEXTURE7));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, sceneTex[1]));

	GL_CHECK(glUniform1i(SUBROUTINE, param.voxelDraw));

	GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
}

void Scene::DrawScene() {
	GL_CHECK(glUseProgram(shaders->drawScene));

	for (auto model = models->begin(); model != models->end(); model++) {

		// Don't draw models without texture
		if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
			continue;
		}

		(*model)->Draw();
	}
}

void Scene::DrawVoxels() {
	GL_CHECK(glEnable(GL_CULL_FACE));

	GL_CHECK(glUseProgram(shaders->voxel));
	GL_CHECK(glBindVertexArray(voxelModel->GetVAO()));

	GL_CHECK(glActiveTexture(GL_TEXTURE3));
	GL_CHECK(glBindTexture(GL_TEXTURE_3D, voxelTex));

	GL_CHECK(glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void *) (sizeof(DrawElementsIndirectCommand) * param.mipLevel)));
}

void Scene::PanLight(GLfloat dx, GLfloat dy) {
	GLfloat polar = acos(param.lightDir.y);
	GLfloat azimuth = atan2f(param.lightDir.z, param.lightDir.x);

	float eps = 0.001f;
	float rspeed = 0.001f;

	azimuth += rspeed * dx;
	polar -= rspeed * dy;

	azimuth = (float) fmod(azimuth, 2.0f * (float) M_PI);
	polar = polar < (float) M_PI - eps ? (polar > eps ? polar : eps) : (float) M_PI - eps;

	param.lightDir = glm::vec3(sin(polar) * cos(azimuth),
														 cos(polar),
														 sin(polar) * sin(azimuth));

	UpdateBuffers();
}


