#version 450

/* ### IN ### */

/* Vertex Input Buffer => per vertex */
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 texCoord;
layout(location = 4) in uvec2 boneRange;	// {offset, boneCount}

/* Uniforms | std140 => vec4 convention */
/* layout(DescriptorSetLayout, VkDescriptorSetLayoutBinding of DescriptorSetLayout, layout convention/alignment) */
layout(set = 0, binding = 0, std140) uniform UNIFORMS {
	mat4 mvp;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} uniforms;

struct BoneRef {
	uint boneId;		// global bone index for boneMatrices
	float weight;
};

/* Storage Buffer */
/* => dynamic array sizes possible | std430 => no vec4 convention */
layout(set = 0, binding = 1, std430) readonly buffer BONEREFS {
	BoneRef reference[];
} boneRefs;

/* Storage Buffer */
layout(set = 0, binding = 2, std430) readonly buffer BONEBUFFER {
	mat4 matrix[];
} boneMatrices;

/* ### OUT ### */

/* Data for fragment shader */
layout(location = 0) out VOut {
	vec3 fragColor;
	vec3 normal;
	vec2 texCoord;
} vOut;

void main() {
	/* Either use vertex in model or bone space if bones are available */
	vec4 animPos = boneRange[1] == 0 ? vec4(pos, 1.0f) : vec4(0.0f);

	/* boneRange[1] holds number of bones connected to vertex */
	for (uint b = 0; b < boneRange[1]; b++) {
		/* Puts vertex into bone space of the current bone in question */
		vec4 boneSpacePos = boneMatrices.matrix[boneRefs.reference[boneRange[0] + b].boneId] * vec4(pos, 1.0);
		/* Calculates actual position of vertex after every step of the for loop */
		animPos += boneRefs.reference[boneRange[0] + b].weight * boneSpacePos;
	}

	gl_Position = uniforms.mvp * vec4(animPos.xyz, 1.0);
	vOut.fragColor = color;
	vOut.normal = (uniforms.viewMatrix * transpose(inverse(uniforms.modelMatrix)) * vec4(normal, 0.0)).xyz;
	vOut.texCoord = texCoord;
}