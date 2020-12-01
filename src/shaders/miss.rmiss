#version 460
#extension GL_EXT_ray_tracing : enable

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload hitValue;

void main()
{
    hitValue.color = vec3(0.0);
	hitValue.distance = -1;
}