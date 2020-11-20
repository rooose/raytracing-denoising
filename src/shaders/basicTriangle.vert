#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout( push_constant ) uniform ColorBlock {
  mat4 current_model;
} PushConstant;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;

void main() {
    vec4 fragPositionv4 =  ubo.view * PushConstant.current_model * vec4(inPosition, 1.0);
    vec4 fragNomalv4 =  ubo.view *  ubo.model * vec4(inNormal, 0.);
    gl_Position = ubo.proj * fragPositionv4;
    fragColor = inColor;
    fragNormal = fragNomalv4.xyz;
    fragTexCoord = inTexCoord;
    fragPos = fragPositionv4.xyz / fragPositionv4.w ;
}