#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;
//layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec3 attribs;

layout(binding = 0, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	int vertexSize;
} ubo;

layout(binding = 1, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) uniform Material 
{ 
	vec4 baseColorFactor;
	int baseColorTextureIndex;
} materials[];

layout(binding = 6, set = 0) uniform Light 
{ 
    vec3 pos;
    vec4 color;
    float intensity;
} lights[];


layout( push_constant ) uniform ColorBlock {
  int nbLights;
} PushConstant;

layout(binding = 0, set = 1) uniform sampler2D texSamplers[];

struct Vertex
{
    vec3 pos;
    vec4 color;
    vec3 normal;
    vec2 texCoord;
	int materialId;
 };

Vertex unpack(uint index)
{
	// Unpack the vertices from the SSBO using the glTF vertex structure
	// The multiplier is the size of the vertex divided by four float components (=16 bytes)
	const int m = ubo.vertexSize / 16;

	vec4 d0 = vertices.v[m * index + 0];
	vec4 d1 = vertices.v[m * index + 1];
	vec4 d2 = vertices.v[m * index + 2];
	vec4 d3 = vertices.v[m * index + 3];

	Vertex v;
	v.pos = d0.xyz;
	v.color = vec4(d0.w, d1.x, d1.y, d1.z);
	v.normal  = vec3(d1.w, d2.x, d2.y);
	v.texCoord = vec2(d2.z, d2.w);
	v.materialId = int(d3.x);

	return v;
}

void main()
{
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 = unpack(index.x);
	Vertex v1 = unpack(index.y);
	Vertex v2 = unpack(index.z);

	// Interpolate normal
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	const vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
	const vec4 color = (v0.color * barycentricCoords.x + v1.color * barycentricCoords.y + v2.color * barycentricCoords.z) * materials[v0.materialId].baseColorFactor ;
	const vec3 pos = (v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z);

	// Interpolate for texture
	const vec2 textCoords = (v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z);
//	vec2 texCoords = 

	// Basic lighting
	vec4 lightColor = vec4(0.,0.,0.,1.);
	for (int i = 0; i < PushConstant.nbLights; i ++)
	{
		vec3 lightVector = normalize(lights[i].pos - pos);
		float dot_product = max(dot(lightVector, normal), 0);
		lightColor += lights[i].color * dot_product;
	}

	lightColor = vec4(min(1., lightColor.x), min(1., lightColor.y), min(1., lightColor.z), 1.);
 
	// Shadow casting
	//	float tmin = 0.001;
	//	float tmax = 10000.0;
	//	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	//	shadowed = true;  
	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	//	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, origin, tmin, lightVector, tmax, 2);
	//	if (shadowed) {
	//		hitValue *= 0.3;
	//	}

//	hitValue = vec3(materials[v0.materialId].baseColorTextureIndex/10., 0., 0.);

//	hitValue = vec3(PushConstant.nbLights/1,0., 0.);
//	hitValue = normal * 0.5 + 0.5;
	hitValue = (lightColor * color).xyz;
//	hitValue = texture(texSamplers[materials[v0.materialId].baseColorTextureIndex], textCoords).xyz;
//	hitValue = texture(texSampler, v0.texCoord).xyz;
//	hitValue = color.xyz;
}