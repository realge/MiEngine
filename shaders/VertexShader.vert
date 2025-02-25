#version 450
#extension GL_ARB_separate_shader_objects : enable

// Uniform buffer with MVP matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

// Outputs to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;

void main() {
    // Transform the vertex position
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    
    // Pass color, texture coordinates and normals to fragment shader
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    
    // Transform the normal using the model matrix
    // Note: for a proper normal transformation, you should use the inverse transpose
    // of the model matrix, but this is simplified for now
   fragNormal = transpose(inverse(mat3(ubo.model))) * inNormal;
}