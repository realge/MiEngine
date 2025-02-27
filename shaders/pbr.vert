#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input vertex attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec4 inTangent;

// Uniform buffer for MVP matrices
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
} ubo;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out mat3 TBN;
layout(location = 7) out vec3 fragCameraPos;

void main() {
    // Calculate world position
    vec4 worldPos = ubo.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    // Calculate fragment normal in world space
    mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
    fragNormal = normalize(normalMatrix * inNormal);
    
    // Calculate tangent-bitangent-normal matrix for normal mapping
    vec3 T = normalize(normalMatrix * inTangent.xyz);
    vec3 N = fragNormal;
    
    // Re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    
    // Calculate bitangent and apply handedness from tangent.w
    vec3 B = cross(N, T) * inTangent.w;
    
    // Create TBN matrix for transforming from tangent to world space
    TBN = mat3(T, B, N);
    
    // Pass color and texture coordinates to fragment shader
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    
    // Pass camera position to fragment shader
    fragCameraPos = ubo.cameraPos;
    
    // Calculate final clip-space position
    gl_Position = ubo.proj * ubo.view * worldPos;
}
