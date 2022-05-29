#version 450

vec2 debugTriangle[3] = vec2[](
	vec2( 0.0, -0.5),
	vec2( 0.5,  0.5),
	vec2(-0.5,  0.5)
);

void main() {

	// gl_VertexIndex is specific for vulkan (OpenGL uses gl_VertexID)
	gl_Position = vec4(debugTriangle[gl_VertexIndex], 0.0, 1.0);

}