#version 450

/* Input | Output */
layout(location = 0) in vec3	pos;
layout(location = 1) in vec3	normal;
layout(location = 2) in vec2	uv;
layout(location = 3) in uvec2	boneAttrib;		// uvec2{offset, count}

struct VertexData {
	vec3 normal;
	vec2 uv;
};
out VertexData vertexData;

/* additional Shader Types */
struct BoneReference {
	uint	boneId;		// index into boneMatrices
	float	weight;		// value between 0 and 1
};

/* Uniform Buffers */
layout(binding = 0, std140) uniform Matrices {
	mat4 mvp;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} matrices;

/* Storage Buffers */
layout(binding = 0, std430) readonly buffer BoneRefs {
	BoneReference reference[];	// references into boneMatrices
} boneRefs;

layout(binding = 1, std140) readonly buffer BoneMatrices {
	mat4 matrix[];				// O_i' x O_i^-1 : modelspace => bonespace => modelspace
} boneMatrices;

/* ENTRY POINT */
void main() {
	/* either uses attributes in restpose or calculates them for animationpose in upcoming for-loop */
	vec4 animationPos_weighted	= boneAttrib[1] == 0 ? vec4(pos, 1.0f)	: vec4(0.0f);
	vertexData.normal			= boneAttrib[1] == 0 ? vec3(normal)		: vec3(0.0f);

	/* boneAttrib[1] holds number of bones connected to vertex */
	for (uint currentBone = 0; currentBone < boneAttrib[1]; currentBone++) {
		/* puts vertex into bonespace to calculate new position in modelspace | calculates weighted animationPos */
		vec4 animationPos = boneMatrices.matrix[boneRefs.reference[boneAttrib[0] + currentBone].boneId] * vec4(pos, 1.0f);
		animationPos_weighted += boneRefs.reference[boneAttrib[0] + currentBone].weight * animationPos;

		/* transpose(inverse(<matrix>)) to preserve normal | mat3 => rotation matrix */
		vertexData.normal +=
			boneRefs.reference[boneAttrib[0] + currentBone].weight * 
			transpose(inverse(mat3(boneMatrices.matrix[boneRefs.reference[boneAttrib[0] + currentBone].boneId]))) * 
			vec3(normal);
	}

	/* per Instance logic => gl_InstanceID */
	vec4 posOut = matrices.modelMatrix * vec4(animationPos_weighted.xyz, 1.0f);
	posOut.z += gl_InstanceID * 1.5f;

	/* vertex shader output */
	gl_Position = matrices.projectionMatrix * matrices.viewMatrix * posOut;

	/* transpose(inverse(<matrix>)) to preserve normal */
	vertexData.normal = 
		(matrices.viewMatrix * transpose(inverse(matrices.modelMatrix)) * 
		vec4(vertexData.normal, 0.0f)).xyz;

	vertexData.uv = uv;
}