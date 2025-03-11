#version 450
#extension GL_ARB_separate_shader_objects : enable

//------------------------------------------------------------------------------
// Uniform buffers
//------------------------------------------------------------------------------
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;           // Model matrix
    mat4 view;            // View matrix
    mat4 proj;            // Projection matrix
    vec3 cameraPos;       // Camera position in world space
    float time;           // Time for animations (if needed)
} ubo;
const int MAX_LIGHTS = 16; 
// Push constant for model matrix - per-instance data
layout(push_constant) uniform PushConstants {
   layout(offset = 0) mat4 model;           // Model matrix from push constant
   layout(offset = 64) vec4 baseColorFactor;
   layout(offset = 80) float metallicFactor;
   layout(offset = 84) float roughnessFactor;
   layout(offset = 88) float ambientOcclusion;
   layout(offset = 92) float emissiveFactor;
   layout(offset = 96) int hasAlbedoMap;
   layout(offset = 100) int hasNormalMap;
   layout(offset = 104) int hasMetallicRoughnessMap;
   layout(offset = 108) int hasEmissiveMap;
} pushConstants;

//------------------------------------------------------------------------------
// Input Vertex Attributes
//------------------------------------------------------------------------------
layout(location = 0) in vec3 inPosition;    // Vertex position
layout(location = 1) in vec3 inColor;       // Vertex color
layout(location = 2) in vec3 inNormal;      // Vertex normal
layout(location = 3) in vec2 inTexCoord;    // Texture coordinates
layout(location = 4) in vec4 inTangent;     // Tangent vector (xyz) + handedness (w)

//------------------------------------------------------------------------------
// Output to Fragment Shader
//------------------------------------------------------------------------------
layout(location = 0) out vec3 fragColor;        // Vertex color
layout(location = 1) out vec2 fragTexCoord;     // Texture coordinates
layout(location = 2) out vec3 fragNormal;       // World-space normal
layout(location = 3) out vec3 fragPosition;     // World-space position
layout(location = 4) out mat3 TBN;              // Tangent-Bitangent-Normal matrix
layout(location = 7) out vec3 fragViewDir;      // View direction in tangent space

void main() {
    //--------------------------------------------------------------------------
    // Position Transformation
    //--------------------------------------------------------------------------
    // Transform vertex to world space using the instance's model matrix
    vec4 worldPosition = pushConstants.model * vec4(inPosition, 1.0);
    
    // Save world position for fragment shader
    fragPosition = worldPosition.xyz;
    
    // Output clip space position
    gl_Position = ubo.proj * ubo.view * worldPosition;
    
    //--------------------------------------------------------------------------
    // Normal and Tangent Space Calculation
    //--------------------------------------------------------------------------
    // Calculate normal matrix (inverse transpose of model matrix's 3x3 part)
    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.model)));
    
    // Transform normal and tangent to world space
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 T = normalize(normalMatrix * inTangent.xyz);
    
    // Ensure T is perpendicular to N using Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    
    // Calculate bitangent using the handedness stored in tangent.w
    vec3 B = normalize(cross(N, T) * inTangent.w);
    
    // Create TBN matrix for normal mapping
    TBN = mat3(T, B, N);
    
    //--------------------------------------------------------------------------
    // View Direction Calculation
    //--------------------------------------------------------------------------
    // Calculate view direction in world space
    vec3 worldViewDir = ubo.cameraPos - fragPosition;
    
    // Transform view direction to tangent space
    fragViewDir = normalize(transpose(TBN) * worldViewDir);
    
    //--------------------------------------------------------------------------
    // Pass-through Attributes
    //--------------------------------------------------------------------------
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragNormal = N;
}