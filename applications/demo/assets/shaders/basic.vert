#version 450

/* Vertex Input Buffer => pos and color per vertex (no gl_VertexIndex needed) */
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 color;

/* Uniforms */
layout(set = 0, binding = 0) uniform MVP_Matrix {
	mat4 matrix;
} mvpMatrix;

/* data for fragment shader */
layout(location = 0) out vec3 fragColor;

void main() {
	gl_Position = mvpMatrix.matrix * vec4(pos, 1.0);
	fragColor = color;
}