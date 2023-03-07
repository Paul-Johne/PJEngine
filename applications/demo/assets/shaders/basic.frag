#version 450

/* ### IN ### */

layout(set = 0, binding = 3) uniform sampler2D texSampler;

// location out != location in (other address)
layout(location = 0) in VIn {
	vec3 fragColor;
	vec3 normal;		// normal in view space
	vec2 texCoord;
} vIn;

/* ### OUT ### */

layout(location = 0) out vec4 color;

void main() {
	vec3 n = normalize(vIn.normal);
	float cosTheta = max(n.z, 0.0); // n.z * 1 <=> dot(n, vec3(0.0, 0.0, 1.0))

	// color = vec4(vIn.fragColor * cosTheta, 1.0);
	color = texture(texSampler, vIn.texCoord).bgra;

	/* Discards current fully transparent fragment to avoid wrong color blending */
	if (color.a == 0.0) {
		discard;
	}

	color = vec4(color.rgb * cosTheta, color.a);
}