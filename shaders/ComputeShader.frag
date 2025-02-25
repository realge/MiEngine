#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

// Texture samplers
layout(binding = 1) uniform sampler2D texSampler; // Diffuse/Albedo

// In future implementation, add more texture samplers:
// layout(binding = 2) uniform sampler2D normalMap;
// layout(binding = 3) uniform sampler2D metallicMap;
// layout(binding = 4) uniform sampler2D roughnessMap;
// layout(binding = 5) uniform sampler2D aoMap;
// layout(binding = 6) uniform sampler2D emissiveMap;

// Material uniform buffer (for future implementation)
// layout(binding = 7) uniform MaterialUBO {
//     vec3 diffuseColor;
//     float alpha;
//     float metallic;
//     float roughness;
//     vec3 emissiveColor;
//     float emissiveStrength;
//     int useNormalMap;
//     int useMetallicMap;
//     int useRoughnessMap;
//     int useAOMap;
//     int useEmissiveMap;
// } material;

// Output color
layout(location = 0) out vec4 outColor;

void main() {
    // Sample the diffuse texture
    vec4 texColor = texture(texSampler, fragTexCoord);
    
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
    
    // Diffuse lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);
    
    // Combine lighting with texture and vertex color
    vec3 lighting = ambient + diffuse;
    vec3 result = lighting * texColor.rgb * fragColor;
    
    outColor = vec4(result, 1.0);
    
    // Note: For a full PBR implementation, you would need:
    // 1. Transform normals using a normal map if available
    // 2. Calculate specular reflection using roughness and metallic parameters
    // 3. Apply ambient occlusion
    // 4. Add emissive contribution
    // This would require a more complex shader and additional uniforms
}