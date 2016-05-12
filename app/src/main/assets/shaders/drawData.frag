#version 310 es

precision highp float;
precision highp int;

in vec3 exNormal;
in vec4 exPosition;
in vec2 exTexCoords;
in vec3 exTangent;
//in vec3 exBiTangent;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec4 outTangent;
// TODO: Maximum value is 3, need to fix depenencies
//layout(location = 4) out vec4 outBiTangent;

layout(location = 0) uniform vec3 diffColor;
layout(binding = 0) uniform sampler2D diffuseUnit;
layout(binding = 1) uniform sampler2D maskUnit;
layout(location = 10) uniform int colorPicker;

//subroutine vec4 SampleColor();

//layout(index = 0) subroutine(SampleColor)
vec4 DiffuseColor() {
	return vec4(diffColor, 1.0f);
}

//layout(index = 1) subroutine(SampleColor)
vec4 TextureColor() {
	return vec4(texture(diffuseUnit, exTexCoords).rgb, 1.0f);
}

//layout(index = 2) subroutine(SampleColor)
vec4 MaskColor() {
	return vec4(texture(diffuseUnit, exTexCoords).rgb, texture(maskUnit, exTexCoords).r);
}

//layout(location = 0) subroutine uniform SampleColor GetColor;

vec4 GetColor() {
    switch(colorPicker) {
        case 0: return DiffuseColor();
        case 1: return TextureColor();
        case 2: return MaskColor();
        default: return vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    return vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

void main()
{	
	outNormal = vec4(normalize(exNormal), 1.0f);
	outTangent = vec4(normalize(exTangent), 1.0f);
//	outBiTangent = vec4(normalize(exBiTangent), 1.0f);
	outPosition = exPosition / exPosition.w;
	outColor = GetColor();
}