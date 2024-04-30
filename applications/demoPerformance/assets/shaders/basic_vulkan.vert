#version 450

/* Input | Out */
layout(location = 0) in vec3	pos;
layout(location = 1) in vec3	normal;
layout(location = 2) in vec2	uv;
layout(location = 3) in uvec2	boneAttrib;		// vec2{offset, count}

layout(location = 0) out VertexOut {
	vec3 normal;
	vec2 uv;
} vertexOut;

/* Shader Types */
struct BoneReference {
	uint	boneID;		// index into boneMatrices
	float	weight;		// value between 0 and 1
};

/* Storage Buffers */
/* set = DescriptorSetLayout | binding = VkDescriptorSetLayoutBinding of DescriptorSetLayout */
/* layout convention => std140 (vec4 layout) | std430 (no vec4 layout) */
layout(set = 0, binding = 0, std140) uniform Uniforms {
	mat4 mvp;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} uniforms;

layout(set = 0, binding = 1, std430) readonly buffer BoneRefs {
	BoneRef reference[];	// references into boneMatrices
} boneRefs;

layout(set = 0, binding = 2, std430) readonly buffer BoneMatrices {
	mat4 matrix[];			// animationPoses
} boneMatrices;

/* ENTRY POINT */
void main() {

}