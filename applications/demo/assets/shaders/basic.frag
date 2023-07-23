#version 450

/* ### IN ### */

layout(set = 0, binding = 3) uniform sampler2D texSampler;

// location out != location in (other address in one shader)
layout(location = 0) in VIn {
	vec3 fragColor;
	vec3 fragNormal;		// normal in view space
	vec2 fragTexCoord;
} vIn;

/* ### OUT ### */

layout(location = 0) out vec4 color;

/* ### ENTRY POINT ### */

void main() {
	vec3 n = normalize(vIn.fragNormal);
	float cosTheta = max(n.z, 0.0f); // n.z * 1.0 <=> dot(n, cam_vec3(0.0, 0.0, 1.0))

	/* 2 options for color source value: */
	// color = vec4(vIn.fragColor * cosTheta, 1.0);
	color = texture(texSampler, vIn.fragTexCoord);

	/* Discards current fully transparent fragment to avoid wrong color blending */
	if (color.a == 0.0f) {
		discard;
	}

	color = vec4(color.rgb * cosTheta, color.a);

	// automated code for rendering on depth image after color calculation
}

// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT after:
//	=> main()
//	=> rendering on resolve image after fragment shader