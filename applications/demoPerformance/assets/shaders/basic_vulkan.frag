#version 450

/* Input | Output */
layout(location = 0) in VertexIn {
	vec3 normal;
	vec2 uv;
} vertexIn;

layout(location = 0) out vec4 color;

/* Sampler */
layout(set = 0, binding = 3) uniform sampler2D texSampler2D;

/* Entry Point */
void main() {
	/* basic shading for demo */
	vec3	nNormal		= normalize(vertexIn.normal);
	float	cosTheta	= max(nNormal.z, 0.0f);			// TODO( dot(nNormal, cam_vec3) )

	/* discarding fully transparent fragments to avoid wrong color blending */
	color = texture(texSampler2D, vertexIn.uv);
	if (color.a == 0.0f) {
		discard;
	}

	/* fragment shader output */
	color = vec4(color.rgb * cosTheta, color.a);
}