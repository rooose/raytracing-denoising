#version 450
#extension GL_ARB_separate_shader_objects : enable
#define PI 3.1415926538

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {

	mat4 invvp = inverse(ubo.proj * ubo.view);
	vec4 reflection = invvp * vec4(reflect(fragPos, fragNormal), 1);
	vec3 R = normalize(reflection.xyz);


	float theta = atan(R.z / R.x);
	float xz = sqrt(R.x * R.x + R.z * R.z);
	float phi = acos(R.y);

	float h = phi / PI;
	float l = theta / (2 * PI );

	vec2 coords = vec2(l, h);

	outColor = texture(texSampler, coords);
//	outColor = vec4(R, 1.);
}