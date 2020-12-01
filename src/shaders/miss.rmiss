#version 460
#extension GL_EXT_ray_tracing : enable
#define PI 3.1415926538

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload hitValue;
layout(binding = 0, set = 1) uniform sampler2D texSamplers[];

void main()
{
	vec3 R = normalize(gl_WorldRayDirectionEXT.xyz);

	float theta = atan(R.y / R.x) + ((R.x < 0) ? sign(R.y) * PI : 0.);
	float xz = sqrt(R.x * R.x + R.z * R.z);
	float phi = asin(R.z);

	float h = phi / PI + 0.5;
	float l = theta / (2 * PI ) + 0.5;

	vec2 coords = vec2(l, -h);
	hitValue.color = texture(texSamplers[0], coords).xyz;

//    hitValue.color = vec3(0.0);
	hitValue.distance = -1;
}