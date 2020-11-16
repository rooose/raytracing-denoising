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

vec4 fullRelflexion() {
	vec3 I = normalize(fragPos);
	mat4 invvp = inverse(ubo.view);
	vec4 reflection = invvp * vec4(reflect(I, fragNormal), 0.);
	vec3 R = normalize(reflection.xyz);

	float theta = atan(R.y / R.x) + ((R.x < 0) ? sign(R.y) * PI : 0.);
	float xz = sqrt(R.x * R.x + R.z * R.z);
	float phi = asin(R.z);

	float h = phi / PI + 0.5;
	float l = theta / (2 * PI ) + 0.5;

	vec2 coords = vec2(l, -h);
	return texture(texSampler, coords);
}

void main() {
    outColor = texture(texSampler, fragTexCoord);
}