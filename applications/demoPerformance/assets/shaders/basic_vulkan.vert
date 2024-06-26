#version 450

/* Input | Output */
layout(location = 0) in vec3	pos;
layout(location = 1) in vec3	normal;
layout(location = 2) in vec2	uv;
layout(location = 3) in uvec2	boneAttrib;		// uvec2{offset, count}

layout(location = 0) out VertexOut {
	vec3 normal;
	vec2 uv;
} vertexOut;

/* additional Shader Types */
struct BoneReference {
	uint	boneId;		// index into boneMatrices
	float	weight;		// value between 0 and 1
};

/* Vulkan-specific part: */
// set = DescriptorSetLayout | binding = VkDescriptorSetLayoutBinding of DescriptorSetLayout
// std = layout convention => std140 (vec4 layout) | std430 (no vec4 layout)

/* Uniform Buffers */
layout(set = 0, binding = 0, std140) uniform Matrices {
	mat4 mvp;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} matrices;

/* Storage Buffers */
layout(set = 0, binding = 1, std430) readonly buffer BoneRefs {
	BoneReference reference[];	// references into boneMatrices
} boneRefs;

layout(set = 0, binding = 2, std140) readonly buffer BoneMatrices {
	mat4 matrix[];				// O_i' x O_i^-1 : modelspace => bonespace => modelspace
} boneMatrices;

/* ENTRY POINT */
void main() {
	/* either uses attributes in restpose or calculates them for animationpose in upcoming for-loop */
	vec4 animationPos_weighted	= boneAttrib[1] == 0 ? vec4(pos, 1.0f)	: vec4(0.0f);
	vertexOut.normal			= boneAttrib[1] == 0 ? vec3(normal)		: vec3(0.0f);

	/* boneAttrib[1] holds number of bones connected to vertex */
	for (uint currentBone = 0; currentBone < boneAttrib[1]; currentBone++) {
		/* puts vertex into bonespace to calculate new position in modelspace | calculates weighted animationPos */
		vec4 animationPos = boneMatrices.matrix[boneRefs.reference[boneAttrib[0] + currentBone].boneId] * vec4(pos, 1.0f);
		animationPos_weighted += boneRefs.reference[boneAttrib[0] + currentBone].weight * animationPos;

		/* transpose(inverse(<matrix>)) to preserve normal | mat3 => rotation matrix */
		vertexOut.normal +=
			boneRefs.reference[boneAttrib[0] + currentBone].weight * 
			transpose(inverse(mat3(boneMatrices.matrix[boneRefs.reference[boneAttrib[0] + currentBone].boneId]))) * 
			vec3(normal);
	}

	/* vertex shader output */
	gl_Position = matrices.mvp * vec4(animationPos_weighted.xyz, 1.0f);
	/* transpose(inverse(<matrix>)) to preserve normal */
	vertexOut.normal = 
		(matrices.viewMatrix * transpose(inverse(matrices.modelMatrix)) * 
		vec4(vertexOut.normal, 0.0f)).xyz;

	vertexOut.uv = uv;
}