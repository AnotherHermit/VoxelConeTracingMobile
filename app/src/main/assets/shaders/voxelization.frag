#version 310 es
#extension GL_OES_shader_image_atomic : enable

precision highp float;
precision highp int;

in vec2 intTexCoords;
in vec4 shadowCoord;
in vec3 exNormal;

flat in uint domInd;

layout(location = 0) uniform vec3 diffColor;
layout(location = 10) uniform int colorPicker;

layout(binding = 0) uniform sampler2D diffuseUnit;
layout(binding = 2, r32ui) uniform highp uimage2DArray voxelTextures;
layout(binding = 3, r32ui) uniform highp uimage3D voxelData;
layout(binding = 5) uniform highp sampler2DShadow shadowMap;

struct SceneParams {
	mat4 MTOmatrix[3];
	mat4 MTWmatrix;
	mat4 MTShadowMatrix;
	vec3 lightDir;
	uint voxelDraw;
	uint view;
	uint voxelRes;
	uint voxelLayer;
	uint numMipLevels;
	uint mipLevel;
};

layout (std140, binding = 1) uniform SceneBuffer {
	SceneParams scene;
};

struct DrawElementsIndirectCommand {
	uint vertexCount;
	uint instanceCount;
	uint firstVertex;
	uint baseVertex;
	uint baseInstance;
};

layout(std430, binding = 0) buffer DrawCmdBuffer {
	DrawElementsIndirectCommand drawCmd[10];
};

struct ComputeIndirectCommand {
	uint workGroupSizeX;
	uint workGroupSizeY;
	uint workGroupSizeZ;
};

layout(std430, binding = 1) buffer ComputeCmdBuffer {
	ComputeIndirectCommand compCmd[10];
};

layout(std430, binding = 2) writeonly buffer SparseBuffer {
	uint sparseList[];
};

struct VoxelData {
	vec4 color;
	uint light;
	uint count;
};

uint packARGB8(VoxelData dataIn) {
	uint result = uint(0);

	uvec3 uiColor = uvec3(dataIn.color.rgb * 31.0f * float(dataIn.count));

	result |= (dataIn.light & uint(0x0F)) << 28;
	result |= (dataIn.count & uint(0x0F)) << 24;
	result |= (uiColor.r & uint(0xFF)) << 16;
	result |= (uiColor.g & uint(0xFF)) << 8;
	result |= (uiColor.b & uint(0xFF));

	return result;
}

uint packRG11B10(uvec3 dataIn) {
	uint result = uint(0);

	result |= (dataIn.r & uint(0x7FF)) << 21;
	result |= (dataIn.g & uint(0x7FF)) << 10;
	result |= (dataIn.b & uint(0x3FF));

	return result;
}

//subroutine vec4 SampleColor();

//layout(index = 0) subroutine(SampleColor)
vec4 DiffuseColor() {
	return vec4(diffColor, 1.0f);
}

//layout(index = 1) subroutine(SampleColor)
vec4 TextureColor() {
	return vec4(texture(diffuseUnit, intTexCoords).rgb, 1.0f);
}

//layout(location = 0) subroutine uniform SampleColor GetColor;

vec4 GetColor() {
    switch(colorPicker) {
        case 0: return DiffuseColor();
        case 1: return TextureColor();
        default: return vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    return vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

void main()
{	
	// Set constant color for textured models
	VoxelData data = VoxelData(vec4(0.0f), uint(0x0), uint(0x8));

	ivec3 voxelCoord;
	float res = float(scene.voxelRes);
	float depthCoord = gl_FragCoord.z * res;


	if(domInd == uint(0)) {
		voxelCoord = ivec3(depthCoord, gl_FragCoord.y, res - gl_FragCoord.x);
	} else if (domInd == uint(1)) {
		voxelCoord = ivec3(gl_FragCoord.x, depthCoord, res - gl_FragCoord.y);
	} else {
		voxelCoord = ivec3(gl_FragCoord.x, gl_FragCoord.y, depthCoord);
	}

	// Calculate shadows
	vec3 s = shadowCoord.xyz * 0.5f + 0.5f;
    vec3 l = scene.lightDir;
    float cosTheta = max(dot(l,normalize(exNormal)), 0.0f);
    float b = 0.005f*tan(acos(cosTheta));
    b = clamp(b, 0.0f,0.02f);
    s.z = s.z - b;

	float shadow = texture(shadowMap, s.xyz);
	data.light = uint(8.0f * (1.0f - shadow));

	// Set color
	data.color = GetColor() * cosTheta;

	uint outData = packARGB8(data);

	imageAtomicMax(voxelTextures, ivec3(ivec2(gl_FragCoord.xy), domInd), outData);
	uint prevData = imageAtomicMax(voxelData, voxelCoord, outData);

	// Check if this voxel was empty before
	if(prevData == uint(0)) {
		// Write to number of voxels list
		uint nextIndex = atomicAdd(drawCmd[0].instanceCount, uint(1));
		
		// Calculate and store number of workgroups needed
		uint compWorkGroups = ((nextIndex + uint(1)) >> 6) + uint(1); // 6 = log2(workGroupSize = 64)
		atomicMax(compCmd[0].workGroupSizeX, compWorkGroups);

		// Write to position buffer
		sparseList[nextIndex + drawCmd[0].baseInstance] = packRG11B10(uvec3(voxelCoord));
	}
}