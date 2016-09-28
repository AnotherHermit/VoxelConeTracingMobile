///////////////////////////////////////
//
//	Computer Graphics TSBK03
//	Conrad Wahlén - conwa099
//
///////////////////////////////////////

#ifndef MODEL_H
#define MODEL_H

#include <GLES3/gl31.h>
#include "glm/glm.hpp"

// ===== Texture Struct =====

struct TextureData {
	GLuint subID;
	GLuint diffuseID;
	GLuint maskID;
	glm::vec3 diffColor;
};

// ===== Model Class =====

class Model {
public:
	Model();

	void SetStandardData(size_t numVertices, GLfloat *verticeData,
											 size_t numNormals, GLfloat *normalData,
											 size_t numIndices, GLuint *indexData,
											 size_t numTangents, GLfloat *tangentData,
											 size_t numBiTangents, GLfloat *biTangentData);

	void SetMaterial(TextureData *textureData);

	void SetTextureData(size_t numTexCoords, GLfloat *texCoordData);

	// Set position buffer data, only for voxel model
	void SetPositionData(GLuint positionBufferID);

	bool hasDiffuseTex();

	bool hasMaskTex();

	void Draw();

	void Voxelize();

	void ShadowMap();

	GLuint GetVAO() { return vao; }

	GLuint GetDiffuseID() { return diffuseID; }

	GLuint GetMaskID() { return maskID; }

	glm::vec3 GetDiffColor() { return diffColor; }

	size_t GetNumIndices() { return nIndices; }

private:
	GLuint vao;
	GLuint meshBuffers[5];
	size_t nIndices;

	// Only used by textured models
	GLuint diffuseID;
	GLuint maskID;
	GLuint texbufferID;
	GLuint subDrawID;
	GLuint subVoxelizeID;

	//
	glm::vec3 diffColor;
};

#endif // MODEL_H
