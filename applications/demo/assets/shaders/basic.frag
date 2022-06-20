#version 450

// location = outputIndex AND NOT attributeIndex as in vertex shader
layout(location = 0) out vec4 color;

void main() {
	
	color = vec4(0.0, vec3(1.0));

}