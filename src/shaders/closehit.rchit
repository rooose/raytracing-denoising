#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};


layout(location = 0) rayPayloadInEXT RayPayload hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
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
	float ambientCoeff;
    float diffuseCoeff;
    float specularCoeff;
    float shininessCoeff;
    float reflexionCoeff;
    float refractionCoeff;
    float refractionIndice;
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
	vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
	const vec4 color = (v0.color * barycentricCoords.x + v1.color * barycentricCoords.y + v2.color * barycentricCoords.z) * materials[v0.materialId].baseColorFactor ;
	const vec3 pos = (v0.pos * barycentricCoords.x + v1.pos * barycentricCoords.y + v2.pos * barycentricCoords.z);

	// Interpolate for texture
	const vec2 textCoords = (v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y + v2.texCoord * barycentricCoords.z);

	// Basic lighting
	vec4 lightColor = vec4(0.,0.,0.,1.);
	for (int i = 0; i < PushConstant.nbLights; i ++) //  PushConstant.nbLights
	{
		const vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
		const vec3 lightVector = normalize(lights[i].pos - origin);
		const float dot_product = dot(lightVector, normal);

		if (dot_product > 0.) {
			const float tmin = 0.001;
			const float tmax = 10000.0;

			shadowed = true;
			float shadow_factor = 1.;
			//	 Trace shadow ray and offset indices to match shadow hit/miss shader group indices
			traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, origin, tmin, lightVector, tmax, 2);
			const float gouraudFactor = materials[v0.materialId].diffuseCoeff * dot_product;

			if (shadowed) {
				shadow_factor = 0.3;
			} else {
				const float alignement = dot(normalize(reflect(lightVector, normal)), gl_WorldRayDirectionEXT);
				if (alignement > 0.)
				{
					const float phongfactor = materials[v0.materialId].specularCoeff * pow(alignement, materials[v0.materialId].shininessCoeff);
					lightColor += phongfactor * lights[i].color;

				}
			}
			lightColor += lights[i].color  * gouraudFactor * shadow_factor;
		}
	}

	lightColor = vec4(min(1., lightColor.x), min(1., lightColor.y), min(1., lightColor.z), 1.);

	hitValue.color = (lightColor * color).xyz;
//	hitValue.color = normal;
	hitValue.distance = gl_HitTEXT;
	hitValue.normal = normal;
	hitValue.reflector = materials[v0.materialId].reflexionCoeff;

}