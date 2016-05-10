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

    param.lightDir = glm::vec3(0.58f, 0.58f, 0.58f);
    param.voxelRes = 256;
    param.voxelLayer = 0;
    param.voxelDraw = 4;
    param.view = 2;
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
    glGenFramebuffers(1, &voxelFBO);

    // Set non-constant uniforms for all programs
    glGenBuffers(1, &sceneBuffer);
    glBindBufferBase(GL_UNIFORM_BUFFER, SCENE, sceneBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneParam), NULL, GL_STREAM_DRAW);

    // Set up the sparse active voxel buffer
    glGenBuffers(1, &sparseListBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPARSE_LIST, sparseListBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (sizeof(GLuint) * MAX_SPARSE_BUFFER_SIZE), NULL,
                 GL_STREAM_DRAW);

    printError("Scene::InitBuffers\n");
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
    param.MTOmatrix[0] =
            glm::rotate(-glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)) * param.MTOmatrix[2];
    param.MTOmatrix[1] =
            glm::rotate(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) * param.MTOmatrix[2];

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
    glGenBuffers(1, &drawIndBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_IND, drawIndBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(drawIndCmd), drawIndCmd, GL_STREAM_DRAW);

    printError("Scene::SetupDrawInd");
}

void Scene::SetupCompInd() {
    // Initialize the indirect compute buffer
    for (size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++) {
        compIndCmd[i].workGroupSizeX = 0;
        compIndCmd[i].workGroupSizeY = 1;
        compIndCmd[i].workGroupSizeZ = 1;
    }

    // Draw Indirect Command buffer for drawing voxels
    glGenBuffers(1, &compIndBuffer);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, compIndBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COMPUTE_IND, compIndBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(compIndCmd), compIndCmd, GL_STREAM_DRAW);
    printError("Scene::SetupCompInd");
}

void Scene::SetupVoxelTextures() {

    // Generate textures for render to texture, only for debugging purposes
    if (voxel2DTex != 0) {
        glDeleteTextures(1, &voxel2DTex);
    }
    glGenTextures(1, &voxel2DTex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, voxel2DTex);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, param.voxelRes, param.voxelRes, 3);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    printError("Scene::SetupVoxelTextures 2D Array");

    // Create the 3D texture that contains the voxel data
    if (voxelTex != 0) {
        glDeleteTextures(1, &voxelTex);
    }
    glGenTextures(1, &voxelTex);
    glBindTexture(GL_TEXTURE_3D, voxelTex);
    glTexStorage3D(GL_TEXTURE_3D, param.numMipLevels + 1, GL_R32UI, param.voxelRes, param.voxelRes,
                   param.voxelRes);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // TODO: Should be GL_CLAMP_TO_BORDER, since edge should not be repeated
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    printError("Scene::SetupVoxelTextures 3D");
}

void Scene::SetupSceneTextures() {
    GLint origViewportSize[4];
    glGetIntegerv(GL_VIEWPORT, origViewportSize);

    if (sceneTex[0] != 0) {
        glDeleteTextures(2, sceneTex);
    }
    glGenTextures(2, sceneTex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTex[0]);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA16F, origViewportSize[2], origViewportSize[3], 4);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    printError("Scene::SetupSceneTextures 2D Array");

    glBindTexture(GL_TEXTURE_2D, sceneTex[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, origViewportSize[2], origViewportSize[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, 0);
    printError("Scene::SetupSceneTextures Depth");

    if (sceneFBO == 0) {
        glGenFramebuffers(1, &sceneFBO);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sceneTex[0], 0, 0);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, sceneTex[0], 0, 1);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, sceneTex[0], 0, 2);
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, sceneTex[0], 0, 3);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneTex[1], 0);

    GLenum DrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
                            GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, DrawBuffers);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer error");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    printError("Scene::SetupSceneTextures FBO");
}

void Scene::SetupShadowTexture() {

    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, options.shadowRes, options.shadowRes);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);

    printError("Scene::SetupShadowTexture Texture");

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer error");
    }

//    GLenum DrawBuffers[] = {GL_NONE};
//    glDrawBuffers(1, DrawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    printError("Scene::SetupShadowTexture FBO");

}

void Scene::SetupShadowMatrix() {
    param.lightDir = glm::normalize(param.lightDir);
    glm::vec3 z = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 l = param.lightDir;
    glm::vec3 axis = glm::cross(l, z);
    // TODO: Make sure this is correct
    float isSame = (float)sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (isSame < glm::epsilon<float>()) {
        param.MTShadowMatrix = glm::scale(glm::vec3(1.0f / (float)sqrt(3.0f))) * param.MTOmatrix[2];
    } else {
        axis = normalize(axis);
        GLfloat angle = acos(glm::dot(z, l));

        param.MTShadowMatrix = glm::rotate(angle, axis) * glm::scale(glm::vec3(1.0f / (float)sqrt(3.0f)));
    }

    // TODO: Only update the buffer after light direction has actually changed
    // Upload new params to GPU
    glBindBufferBase(GL_UNIFORM_BUFFER, SCENE, sceneBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(SceneParam), &param);

    printError("Scene::SetupShadowMatrix");
}

void Scene::UpdateBuffers() {
    // Upload new params to GPU
    glBindBufferBase(GL_UNIFORM_BUFFER, SCENE, sceneBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, NULL, sizeof(SceneParam), &param);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, drawIndBuffer);
    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, compIndBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_IND, drawIndBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, COMPUTE_IND, compIndBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SPARSE_LIST, sparseListBuffer);

    printError("Scene::UpdateBuffers");
}

void Scene::CreateShadow() {
    // Update the direction of the light
    SetupShadowMatrix();

    // Setup framebuffer for rendering offscreen
    GLint origViewportSize[4];
    glGetIntegerv(GL_VIEWPORT, origViewportSize);
    glViewport(0, 0, options.shadowRes, options.shadowRes);

    // Enable rendering to framebuffer with shadow map resolution
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

    // Clear the last shadow map
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Light should also hit backsides (especially for cornell)
    glDisable(GL_CULL_FACE);

    glUseProgram(shaders->shadowMap);

    printError("Scene::CreateShadow Init");

    // Create the shadow map texture
    for (auto model = models->begin(); model != models->end(); model++) {

        // Don't draw models without texture
        if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
            continue;
        }

        (*model)->ShadowMap();

    }

    // Restore the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(origViewportSize[0], origViewportSize[1], origViewportSize[2], origViewportSize[3]);

    printError("Scene::CreateShadow After");
}

void Scene::RenderData() {
    // Enable rendering to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    // Clear the last scene data
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);

    glUseProgram(shaders->drawData);

    printError("Scene::RenderData Init");

    // Create the shadow map texture
    for (auto model = models->begin(); model != models->end(); model++) {

        // Don't draw models without texture
        if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
            continue;
        }

        (*model)->Draw();
    }

    // Restore the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    printError("Scene::RenderData After");
}

void Scene::Voxelize() {
    // Setup framebuffer for rendering offscreen
    GLint origViewportSize[4];
    glGetIntegerv(GL_VIEWPORT, origViewportSize);

    // Enable rendering to framebuffer with voxelRes resolution
    glBindFramebuffer(GL_FRAMEBUFFER, voxelFBO);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, param.voxelRes);
    glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, param.voxelRes);
    glViewport(0, 0, param.voxelRes, param.voxelRes);


    // Clear the last voxelization data
    // TODO: Clear the textures somehow (only interesting for dynamic updates)
    /*glClearTexImage(voxel2DTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    for(size_t i = 0; i <= param.numMipLevels; i++) {
        glClearTexImage(voxelTex, (GLint)i, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
    }*/

    GLuint reset = 0;
    // Reset the sparse voxel count
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawIndBuffer);
    for (size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++) {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                        i * sizeof(DrawElementsIndirectCommand) + sizeof(GLuint), sizeof(GLuint),
                        &reset); // Clear data before since data is used when drawing
    }

    // Reset the sparse voxel count for compute shader
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, compIndBuffer);
    for (size_t i = 0; i <= MAX_MIP_MAP_LEVELS; i++) {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, i * sizeof(ComputeIndirectCommand),
                        sizeof(GLuint),
                        &reset); // Clear data before since data is used when drawing
    }

    // Bind the textures used to hold the voxelization data
    glBindImageTexture(2, voxel2DTex, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glBindImageTexture(3, voxelTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTex);

    // All faces must be rendered
    glDisable(GL_CULL_FACE);

    glUseProgram(shaders->voxelize);

    printError("Scene::Voxelize Init");

    for (auto model = models->begin(); model != models->end(); model++) {
        // Don't draw models without texture if set to skip
        if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
            continue;
        }

        (*model)->Voxelize();

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Restore the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(origViewportSize[0], origViewportSize[1], origViewportSize[2], origViewportSize[3]);

    printError("Scene::Voxelize After");
}

void Scene::MipMap() {
    glUseProgram(shaders->mipmap);

    printError("Scene::Mipmap Init");

    for (GLuint level = 0; level < param.numMipLevels; level++) {
        glBindImageTexture(3, voxelTex, level, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
        glBindImageTexture(4, voxelTex, level + 1, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

        glUniform1ui(CURRENT_LEVEL, level);

        glDispatchComputeIndirect(NULL);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT |
                        GL_COMMAND_BARRIER_BIT);
    }
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);

    printError("Scene::Mipmap after");
}

void Scene::Draw() {
    if (options.drawTextures) DrawTextures();
    else {
        if (options.drawModels) DrawScene();
        if (options.drawVoxels) DrawVoxels();
    }
}

void Scene::DrawTextures() {
    glUseProgram(shaders->singleTriangle);
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, voxel2DTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, voxelTex);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D_ARRAY, sceneTex[0]);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, sceneTex[1]);

    glUniform1i(SUBROUTINE, param.voxelDraw);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    printError("Scene::DrawTextures");
}

void Scene::DrawScene() {
    glUseProgram(shaders->drawScene);

    for (auto model = models->begin(); model != models->end(); model++) {

        // Don't draw models without texture
        if (options.skipNoTexture && !(*model)->hasDiffuseTex()) {
            continue;
        }

        (*model)->Draw();
    }

    printError("Scene::DrawScene");
}

void Scene::DrawVoxels() {
    glEnable(GL_CULL_FACE);

    glUseProgram(shaders->voxel);
    glBindVertexArray(voxelModel->GetVAO());

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_3D, voxelTex);

    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT,
                           (void *) (sizeof(DrawElementsIndirectCommand) * param.mipLevel));

    printError("Scene::DrawVoxels");
}
