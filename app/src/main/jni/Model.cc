///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#include "Model.h"

#include "GL_utilities.h"

Model::Model() {
    subDrawID = 0;
    subVoxelizeID = 0;
    diffuseID = 0;
    maskID = 0;
    diffColor = glm::vec3(1.0f, 0.0f, 0.0f);

    vao = 0;
}

void Model::SetMaterial(TextureData *textureData) {
    subDrawID = textureData->subID;
    subVoxelizeID = (GLuint) (subDrawID != 0);
    diffuseID = textureData->diffuseID;
    maskID = textureData->maskID;
    diffColor = textureData->diffColor;
}

void Model::SetStandardData(size_t numVertices, GLfloat *verticeData,
                            size_t numNormals, GLfloat *normalData,
                            size_t numIndices, GLuint *indexData,
                            size_t numTangents, GLfloat *tangentData,
                            size_t numBiTangents, GLfloat *biTangentData) {

    nIndices = numIndices;
    // Create buffers
    if (vao == 0) {
        GL_CHECK(glGenVertexArrays(1, &vao));
    }

    GL_CHECK(glGenBuffers(5, meshBuffers));

    // Allocate enough memory for instanced drawing buffers
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[0]));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVertices, verticeData, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[1]));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numNormals, normalData, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[2]));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numTangents, tangentData, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[3]));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numBiTangents, biTangentData, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffers[4]));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * numIndices, indexData, GL_STATIC_DRAW));

    // Set the GPU pointers for drawing
    GL_CHECK(glBindVertexArray(vao));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[0]));
    GL_CHECK(glEnableVertexAttribArray(VERT_POS));
    GL_CHECK(glVertexAttribPointer(VERT_POS, 3, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[1]));
    GL_CHECK(glEnableVertexAttribArray(VERT_NORMAL));
    GL_CHECK(glVertexAttribPointer(VERT_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[2]));
    GL_CHECK(glEnableVertexAttribArray(VERT_TANGENT));
    GL_CHECK(glVertexAttribPointer(VERT_TANGENT, 3, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, meshBuffers[3]));
    GL_CHECK(glEnableVertexAttribArray(VERT_BITANGENT));
    GL_CHECK(glVertexAttribPointer(VERT_BITANGENT, 3, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffers[4]));
    GL_CHECK(glBindVertexArray(0));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void Model::SetTextureData(size_t numTexCoords, GLfloat *texCoordData) {
    if (vao == 0) {
        GL_CHECK(glGenVertexArrays(1, &vao));
    }
    GL_CHECK(glGenBuffers(1, &texbufferID));

    // Allocate enough memory for instanced drawing buffers
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, texbufferID));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numTexCoords, texCoordData, GL_STATIC_DRAW));

    // Set the data pointer for the draw program
    GL_CHECK(glBindVertexArray(vao));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, texbufferID));
    GL_CHECK(glEnableVertexAttribArray(VERT_TEX_COORD));
    GL_CHECK(glVertexAttribPointer(VERT_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 0, 0));

    GL_CHECK(glBindVertexArray(0));
}

void Model::SetPositionData(GLuint positionBufferID) {
    if (vao == 0) {
        GL_CHECK(glGenVertexArrays(1, &vao));
    }
    GL_CHECK(glBindVertexArray(vao));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, positionBufferID));
    GL_CHECK(glEnableVertexAttribArray(DATA_POS));
    GL_CHECK(glVertexAttribIPointer(DATA_POS, 1, GL_UNSIGNED_INT, 0, 0));
    GL_CHECK(glVertexAttribDivisor(DATA_POS, 1));

    GL_CHECK(glBindVertexArray(0));
}

bool Model::hasDiffuseTex() {
    return diffuseID != 0;
}

bool Model::hasMaskTex() {
    return maskID != 0;
}

void Model::Voxelize() {
    GL_CHECK(glUniform3f(DIFF_COLOR, diffColor.r, diffColor.g, diffColor.b));

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, diffuseID));

    GL_CHECK(glBindVertexArray(vao));
    GL_CHECK(glUniform1i(SUBROUTINE, subVoxelizeID));

    GL_CHECK(glDrawElements(GL_TRIANGLES, (GLsizei) nIndices, GL_UNSIGNED_INT, 0L));
}

void Model::ShadowMap() {
    GL_CHECK(glBindVertexArray(vao));
    GL_CHECK(glDrawElements(GL_TRIANGLES, (GLsizei) nIndices, GL_UNSIGNED_INT, 0L));
}

void Model::Draw() {
    GL_CHECK(glUniform3f(DIFF_COLOR, diffColor.r, diffColor.g, diffColor.b));
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, diffuseID));
    GL_CHECK(glActiveTexture(GL_TEXTURE1));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, maskID));
    GL_CHECK(glBindVertexArray(vao));
    GL_CHECK(glUniform1i(SUBROUTINE, subDrawID));
    // Disable cull faces for transparent models
    if (hasMaskTex()) {
        GL_CHECK(glDisable(GL_CULL_FACE));
    } else {
        GL_CHECK(glEnable(GL_CULL_FACE));
    }
    GL_CHECK(glDrawElements(GL_TRIANGLES, (GLsizei) nIndices, GL_UNSIGNED_INT, 0L));
}
