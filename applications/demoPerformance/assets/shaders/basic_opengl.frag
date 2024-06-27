#version 450

/* Input | Output */
struct VertexData {
	vec3 normal;
	vec2 uv;
};
in VertexData vertexData;

out vec4 color;

/* Sampler => implicit location */
uniform sampler2D texSampler2D;

/* Entry Point */
void main() {
	/* basic shading for demo */
	vec3	nNormal		= normalize(vertexData.normal);
	float	cosTheta	= max(nNormal.z, 0.0f);

	/* discarding fully transparent fragments to avoid wrong color blending */
	color = texture(texSampler2D, vertexData.uv);
	if (color.a == 0.0f) {
		discard;
	}

	/* fragment shader output */
	color = vec4(color.rgb * cosTheta, color.a);
}