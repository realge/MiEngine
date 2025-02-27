#version 450
#extension GL_ARB_separate_shader_objects : enable

// Constants
const float PI = 3.14159265359;
const float EPSILON = 0.00001;
const int MAX_LIGHTS = 16;

// Material properties uniform buffer
layout(binding = 1) uniform MaterialUniformBuffer {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float aoStrength;
    float emissiveStrength;
    
    int hasBaseColorMap;
    int hasNormalMap;
    int hasMetallicRoughnessMap;
    int hasOcclusionMap;
    int hasEmissiveMap;
    
    float alphaCutoff;
    int alphaMode;
    vec2 padding;
} material;

// Texture samplers
layout(binding = 2) uniform sampler2D albedoMap;
layout(binding = 3) uniform sampler2D normalMap;
layout(binding = 4) uniform sampler2D metallicRoughnessMap;
layout(binding = 5) uniform sampler2D occlusionMap;
layout(binding = 6) uniform sampler2D emissiveMap;

// Light data
struct Light {
    vec4 position;   // w=1 for point, w=0 for directional
    vec4 color;      // rgb + intensity
    float radius;
    float falloff;
    vec2 padding;
};

layout(binding = 7) uniform LightUniformBuffer {
    int lightCount;
    int padding[3];
    Light lights[MAX_LIGHTS];
    vec4 ambientColor;
} lightData;

// IBL resources
layout(binding = 0, set = 1) uniform samplerCube irradianceMap;
layout(binding = 1, set = 1) uniform samplerCube prefilterMap;
layout(binding = 2, set = 1) uniform sampler2D brdfLUT;

// Render settings
layout(push_constant) uniform PushConstants {
    int useIBL;      // 0 = no IBL, 1 = use IBL
    int debugView;   // 0 = normal, 1 = albedo, 2 = normal, 3 = metallic, 4 = roughness, 5 = ao
} settings;

// Input from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in mat3 TBN; not being used
layout(location = 7) in vec3 fragCameraPos;

// Output
layout(location = 0) out vec4 outColor;

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, EPSILON);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, EPSILON);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Sample material textures
    vec4 albedo = material.baseColorFactor;
    if (material.hasBaseColorMap > 0) {
        albedo *= texture(albedoMap, fragTexCoord);
    }
    
    // Handle alpha mode
    if (material.alphaMode == 1) { // Mask mode
        if (albedo.a < material.alphaCutoff) {
            discard;
        }
    }
    
    // Get metallic and roughness
    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    if (material.hasMetallicRoughnessMap > 0) {
        vec4 mrSample = texture(metallicRoughnessMap, fragTexCoord);
        // In glTF, G channel is roughness and B channel is metallic
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    
    // Clamp roughness to avoid visual artifacts
    roughness = clamp(roughness, 0.04, 1.0);
    
    // Get normal from normal map or use geometric normal
    vec3 N = normalize(fragNormal);
    if (material.hasNormalMap > 0) {
        // Sample normal map and transform from [0,1] to [-1,1] range
        vec3 normalSample = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        N = normalize(TBN * normalSample);
    }
    
    // Get ambient occlusion
    float ao = 1.0;
    if (material.hasOcclusionMap > 0) {
        ao = texture(occlusionMap, fragTexCoord).r;
        ao = mix(1.0, ao, material.aoStrength);
    }
    
    // Get emissive color
    vec3 emissive = vec3(0.0);
    if (material.hasEmissiveMap > 0) {
        emissive = texture(emissiveMap, fragTexCoord).rgb * material.emissiveStrength;
    }
    
    // Debug view modes
    if (settings.debugView == 1) {
        outColor = vec4(albedo.rgb, 1.0);
        return;
    } else if (settings.debugView == 2) {
        outColor = vec4(N * 0.5 + 0.5, 1.0);
        return;
    } else if (settings.debugView == 3) {
        outColor = vec4(vec3(metallic), 1.0);
        return;
    } else if (settings.debugView == 4) {
        outColor = vec4(vec3(roughness), 1.0);
        return;
    } else if (settings.debugView == 5) {
        outColor = vec4(vec3(ao), 1.0);
        return;
    }
    
    // Calculate basic vectors needed for lighting
    vec3 V = normalize(fragCameraPos - fragPos);
    vec3 R = reflect(-V, N);
    
    // Calculate reflectance at normal incidence (F0)
    // Dielectrics have F0 of 0.04, metals use albedo as F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    
    // Initialize lighting accumulators
    vec3 Lo = vec3(0.0); // Direct lighting
    
    // Calculate direct lighting contribution from all lights
    for (int i = 0; i < min(lightData.lightCount, MAX_LIGHTS); i++) {
        Light light = lightData.lights[i];
        
        vec3 L;
        float attenuation = 1.0;
        
        if (light.position.w < 0.5) {
            // Directional light
            L = normalize(-light.position.xyz);
        } else {
            // Point light
            vec3 lightDir = light.position.xyz - fragPos;
            float distance = length(lightDir);
            L = normalize(lightDir);
            
            // Calculate attenuation based on distance
            if (distance > light.radius) {
                // Light is out of range
                continue;
            }
            
            // Inverse-square attenuation with smooth falloff at range boundary
            float normalized = distance / light.radius;
            attenuation = pow(clamp(1.0 - pow(normalized, light.falloff), 0.0, 1.0), 2.0);
        }
        
        // Calculate half vector between view and light directions
        vec3 H = normalize(V + L);
        
        // Calculate all dot products needed for BRDF
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        
        // Skip calculation if light is on the wrong side of the surface
        if (NdotL <= 0.0) continue;
        
        // Calculate Cook-Torrance BRDF terms
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(VdotH, F0);
        
        // Calculate specular and diffuse components
        vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, EPSILON);
        
        // Energy conservation: subtract specular from diffuse
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        
        // Combine components and accumulate light contribution
        vec3 radiance = light.color.rgb * light.color.a * attenuation;
        Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
    }
    
    // IBL (Image-Based Lighting)
    vec3 ambient = vec3(0.0);
    
    // Check for IBL setting at runtime
    if (settings.useIBL > 0) {
        // Calculate IBL diffuse contribution
        vec3 kS = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuse = irradiance * albedo.rgb;
        
        // Calculate IBL specular contribution
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);
        
        // Combine ambient components
        ambient = (kD * diffuse + specular) * ao;
    } else {
        // Fallback to simple ambient lighting when IBL is not available
        ambient = lightData.ambientColor.rgb * albedo.rgb * ao;
    }
    
    // Combine direct and indirect lighting
    vec3 color = Lo + ambient + emissive;
    
    // Tone mapping: ACES filmic (better than Reinhard)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
    
    // Gamma correction (assuming sRGB output)
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, albedo.a);
}