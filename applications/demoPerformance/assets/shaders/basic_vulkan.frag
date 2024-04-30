#version 450

/* Input | Out */
layout(location = 0) in VertexIn {
	vec3 normal;
	vec2 uv;
} vertexIn;

layout(location = 0) out vec4 color;

/* Sampler */
layout(set = 0, binding = 3) uniform sampler2D texSampler;

/* ENTRY POINT */
void main() {

}