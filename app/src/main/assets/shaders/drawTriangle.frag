#version 310 es
#define M_PI 3.1415926535897932384626433832795

precision highp float;
precision highp int;

in vec2 exTexCoords;
out vec4 outColor;

layout(binding = 2) uniform highp usampler2DArray voxelTextures;
layout(binding = 3) uniform highp usampler3D voxelData;
layout(binding = 5) uniform highp sampler2D shadowMap;
layout(binding = 6) uniform highp sampler2DArray sceneTex;
layout(binding = 7) uniform highp sampler2D sceneDepth;
layout(location = 10) uniform int texNumber;

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

struct VoxelData {
	vec4 color;
	uint light;
	uint count;
};

VoxelData unpackARGB8(uint bytesIn) {
	VoxelData data;
	uvec3 uiColor;

	// Put a first to improve max operation but it should not be very noticable
	data.light = (bytesIn & uint(0xF0000000)) >> 28;
	data.count = (bytesIn & uint(0x0F000000)) >> 24;
	uiColor.r =  (bytesIn & uint(0x00FF0000)) >> 16;
	uiColor.g =  (bytesIn & uint(0x0000FF00)) >> 8;
	uiColor.b =  (bytesIn & uint(0x000000FF));


	float div[2];
	div[0] = 0.0f;
	div[1] = 1.0f / float(data.count);
	int index = int(data.count != uint(0));

	data.color.rgb = vec3(uiColor) / 31.0f * div[index];
	data.color.a = float(sign(int(data.count)));

	return data;
}

//subroutine vec4 DrawTexture();

//layout(index = 0) subroutine(DrawTexture)
vec4 ProjectionTexture() {
	return unpackARGB8(texture(voxelTextures, vec3(exTexCoords, float(scene.view))).r).color;
}

//layout(index = 1) subroutine(DrawTexture)
vec4 VoxelTexure() {
	float size = float(scene.voxelRes >> scene.mipLevel);
	float depth = float(scene.voxelLayer) / size;
	return unpackARGB8(textureLod(voxelData, vec3(exTexCoords, depth), float(scene.mipLevel)).r).color;
}

//layout(index = 2) subroutine(DrawTexture)
vec4 ShadowTexture() {
	return texture(shadowMap, exTexCoords);
}

//layout(index = 3) subroutine(DrawTexture)
vec4 SceneColor() {
	return texture(sceneTex, vec3(exTexCoords, 0.0f));
}

//layout(index = 4) subroutine(DrawTexture)
vec4 ScenePosition() {
	return texture(sceneTex, vec3(exTexCoords, 1.0f));
}

//layout(index = 5) subroutine(DrawTexture)
vec4 SceneNormal() {
	vec4 normal = texture(sceneTex, vec3(exTexCoords, 2.0f));
//	normal.xyz /= 2.0f;
//	normal.xyz += vec3(0.5f);
	return normal;
}

//layout(index = 6) subroutine(DrawTexture)
vec4 SceneTangent() {
	vec4 tangent = texture(sceneTex, vec3(exTexCoords, 3.0f));
//	tangent.xyz /= 2.0f;
//	tangent.xyz += vec3(0.5f);
	return tangent;
}

//layout(index = 7) subroutine(DrawTexture)
vec4 SceneBiTangent() {
	vec4 biTangent = texture(sceneTex, vec3(exTexCoords, 4.0f));
//	biTangent.xyz /= 2;
//	biTangent.xyz += vec3(0.5f);
	return biTangent;
}

//layout(index = 8) subroutine(DrawTexture)
vec4 SceneDepth() {
	return texture(sceneDepth, exTexCoords);
}



vec4 voxelSampleLevel(vec3 position, float level) {
	float mip = round(level);
	float scale = float(scene.voxelRes >> int(mip));
	position *= scale;
	position = position - vec3(0.5f);
	vec3 positionWhole = floor(position);
	vec3 intpol = position - positionWhole;
	vec3 offsets[8];
	offsets[0] = vec3(0.0f, 0.0f, 0.0f);
	offsets[1] = vec3(0.0f, 0.0f, 1.0f);
	offsets[2] = vec3(0.0f, 1.0f, 0.0f);
	offsets[3] = vec3(0.0f, 1.0f, 1.0f);
	offsets[4] = vec3(1.0f, 0.0f, 0.0f);
	offsets[5] = vec3(1.0f, 0.0f, 1.0f);
	offsets[6] = vec3(1.0f, 1.0f, 0.0f);
	offsets[7] = vec3(1.0f, 1.0f, 1.0f);
	
	//vec4 total = vec4(0.0f);
	VoxelData total;
	total.color = vec4(0.0f);
	total.count = uint(0);
	total.light = uint(0);
	float count = 0.0f;
	for(int i = 0; i < 8; i++) {
		vec3 voxelPos = (positionWhole + offsets[i]) / scale;
		VoxelData voxel = unpackARGB8(textureLod(voxelData, voxelPos, mip).r);
		vec3 temp = (1.0f - offsets[i]) * (1.0f - intpol) + offsets[i] * intpol; 
		total.color += voxel.color * temp.x * temp.y * temp.z * float(sign(int(voxel.light)));
		count += float(voxel.light) * float(sign(int(voxel.count))) * temp.x * temp.y * temp.z;
	}

	return total.color;//vec4(count);
}

vec4 voxelSampleBetween(vec3 position, float level) {
	float levelLow = floor(level);
	float levelHigh = levelLow + 1.0f;
	float intPol = level - levelLow;

	vec4 voxelLow = voxelSampleLevel(position, levelLow);
	vec4 voxelHigh = voxelSampleLevel(position, levelHigh);

	return voxelLow * (1.0f - intPol) + voxelHigh * intPol;
}

vec4 ConeTrace(vec3 startPos, vec3 dir, float coneRatio, float maxDist,	float voxelSize) {

	vec4 accum = vec4(0.0f);
	vec3 samplePos;
	vec4 sampleValue;
	float sampleRadius;
	float sampleDiameter;
	float sampleWeight;
	float sampleLOD = 0.0f;
	
	for(float dist = voxelSize; dist < maxDist && accum.a < 1.0f;) {
		sampleRadius = coneRatio * dist;
		sampleDiameter = max(2.0f * sampleRadius, voxelSize);
		sampleLOD = log2(sampleDiameter * float(scene.voxelRes));
		samplePos = startPos + dir * (dist + sampleRadius);
		sampleValue = voxelSampleBetween(samplePos, sampleLOD);
		sampleWeight = 1.0f - accum.a;
		accum += sampleValue * sampleWeight;
		dist += sampleRadius;
	}

	return accum;
}

vec4 ConeTrace60(vec3 startPos, vec3 dir, float aoDist, float maxDist, float voxelSize) {
	
	vec4 accum = vec4(0.0f);
	float opacity = 0.0f;
	vec3 samplePos;
	vec4 sampleValue;
	float sampleWeight;
	float sampleLOD = 0.0f;
	
	for(float dist = 2.0f*voxelSize; dist < maxDist && accum.a < 1.0f;) {
		samplePos = startPos + dir * dist;
		sampleValue = voxelSampleLevel(samplePos, sampleLOD);
		sampleWeight = 1.0f - accum.a;
		accum += sampleValue * sampleWeight;
		opacity = (dist < aoDist) ? accum.a : opacity;
		sampleLOD += 1.0f;
		dist *= 2.0f;
	}

	return vec4(accum.rgb, 1.0f - opacity);
}

vec4 DiffuseTrace () {
	vec3 dir[6];
	dir[0] = vec3( 0.000000f, 1.000000f,  0.000000f);
	dir[1] = vec3( 0.000000f, 0.500000f,  0.866025f);
	dir[2] = vec3( 0.823639f, 0.500000f,  0.267617f);
	dir[3] = vec3( 0.509037f, 0.500000f, -0.700629f);
	dir[4] = vec3(-0.509037f, 0.500000f, -0.700629f);
	dir[5] = vec3(-0.823639f, 0.500000f,  0.267617f);

	float sideWeight = 2.0f / 20.0f;
	float weight[2];
	weight[0] = 10.0f / 20.0f;
	weight[1] = sideWeight;
	
	float voxelSize = 1.0f / float(scene.voxelRes);
	float maxDistance = 1.00f;
	float aoDistance = 0.015f;
	vec3 pos = ScenePosition().xyz;
	vec3 norm = SceneNormal().xyz;
	vec3 tang = SceneTangent().xyz;
	vec3 bitang = normalize(cross(norm, tang)); //SceneBiTangent().xyz;

	pos += norm * voxelSize * 10.0f;

	vec4 total = vec4(0.0f);
	for(int i = 0; i < 6; i++) {
		vec3 direction = dir[i].x * tang + dir[i].y * norm + dir[i].z * bitang;
		total += weight[int(i != 0)] * ConeTrace60(pos, direction, aoDistance, maxDistance, voxelSize);
	}

	return total;
}

vec4 AngleTrace(vec3 dir, float theta) {
	float voxelSize = 1.0f / float(scene.voxelRes);
	float maxDistance = sqrt(3.0f);
	
	vec3 pos = ScenePosition().xyz;
	vec3 norm = SceneNormal().xyz;
	pos += norm * voxelSize * 2.0f;

	
	float halfTheta = sin(radians(theta)/2.0f);
	float coneRatio = halfTheta / (1.0f - halfTheta);
	return ConeTrace(pos, dir, coneRatio, maxDistance, voxelSize);
}

//layout(index = 9) subroutine(DrawTexture)
vec4 AmbientOcclusion() {
	return vec4(vec3(DiffuseTrace().w), 1.0f);
}

//layout(index = 10) subroutine(DrawTexture)
vec4 DiffuseBounce() {
	return vec4(DiffuseTrace().rgb, 1.0f);
}

//layout(index = 11) subroutine(DrawTexture)
vec4 SoftShadows() {
	vec4 result = AngleTrace(scene.lightDir, 5.0f); 
	vec4 color = SceneColor();
	return vec4(color.rgb * (1.0f - result.a), 1.0f);
}

//layout(index = 12) subroutine(DrawTexture)
vec4 Combination() {
	vec4 diffuse = DiffuseTrace();
	vec4 shadows = AngleTrace(scene.lightDir, 5.0f);
	vec4 color = SceneColor();
	return vec4((color.rgb * 0.9f + diffuse.rgb * 0.4f) * (1.0f - shadows.a) + diffuse.rgb * shadows.a * diffuse.a * 0.6f , 1.0f);
}

//layout(location = 0) subroutine uniform DrawTexture SampleTexture;

vec4 SampleTexture() {
    switch(texNumber) {
        case 0: return ProjectionTexture();
        case 1: return VoxelTexure();
        case 2: return ShadowTexture();
        case 3: return SceneColor();
        case 4: return ScenePosition();
        case 5: return SceneNormal();
        case 6: return SceneTangent();
        case 7: return SceneBiTangent();
        case 8: return SceneDepth();
        case 9: return AmbientOcclusion();
        case 10: return DiffuseBounce();
        case 11: return SoftShadows();
        case 12: return Combination();
        default: return vec4(1.0f, 0.0f,0.0f,1.0f);
    }
    return vec4(1.0f, 0.0f,0.0f,1.0f);
}

void main()
{	
	outColor = SampleTexture();
}
