#version 450

/* Vertex Input Buffer => pos and color per vertex */
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;

/* Uniforms | layout(DescriptorSetLayout, VkDescriptorSetLayoutBinding of DescriptorSetLayout, layout convention/alignment) */
layout(set = 0, binding = 0, std140) uniform UNIFORMS {
	mat4 mvp;
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} uniforms;

/* data for fragment shader */
layout(location = 0) out VOut {
	vec3 fragColor;
	vec3 normal;		// normal in view space
} vOut;

void main() {
	gl_Position = uniforms.mvp * vec4(pos, 1.0);
	vOut.fragColor = color;
	vOut.normal = (uniforms.viewMatrix * transpose(inverse(uniforms.modelMatrix)) * vec4(normal, 0.0)).xyz;
}