#version 450

vec2 debugTriangle[3] = vec2[](
	vec2( 1.0, -1.0),
	vec2( 0.5,  0.5),
	vec2(-0.5,  0.5)
);

vec3 triangleColors[3] = vec3[](
	vec3(5.0, 0.0, 0.0),
	vec3(0.0, 5.0, 0.0),
	vec3(0.0, 0.0, 5.0)
);

/* Vertex Input Buffer => pos and color per vertex (no gl_VertexIndex needed) */
//layout(location = 0) in vec2 pos;
//layout(location = 0) in vec3 color;

layout(location = 0) out vec3 fragColor;

void main() {
	gl_Position = vec4(debugTriangle[gl_VertexIndex], 0.0, 1.0);
	fragColor = triangleColors[gl_VertexIndex];

	//gl_Position = vec4(pos, 0.0, 1.0);
	//fragColor = color;
}