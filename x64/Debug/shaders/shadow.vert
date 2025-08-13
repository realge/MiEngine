#version 450

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 lightSpaceMatrix;
    mat4 model;
} push;

void main() {
    gl_Position = push.lightSpaceMatrix * push.model * vec4(inPosition, 1.0);
}
