#version 450
#extension GL_ARB_separate_shader_objects : enable

// ----------------- Constants -----------------
const float PI = 3.14159265359;
const float EPSILON = 0.00001;
const int MAX_LIGHTS = 16;

// ----------------- Structures -----------------
struct Light {
    vec4 position;   // xyz = position/direction, w = 1 for point, 0 for directional
    vec4 color;      // rgb = color, a = intensity
    float radius;
    float falloff;
};

// ----------------- Input/Output -----------------
// Input from vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;  
layout(location = 4) in mat3 TBN;           
layout(location = 7) in vec3 fragViewDir; 

// Output color
layout(location = 0) out vec4 outColor;

// ----------------- Uniform Buffers -----------------
// Camera and time data
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
} ubo;

// Material properties
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(push_constant) uniform MaterialConstants {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float ambientOcclusion;
    float emissiveFactor;
    int hasAlbedoMap;
    int hasNormalMap;
    int hasMetallicRoughnessMap;
    int hasEmissiveMap;
} material;

// Light data
layout(set = 2, binding = 0) uniform LightBuffer {
    Light lights[MAX_LIGHTS];
    vec4 ambientColor;
    int lightCount;
} lightBuffer;

// ----------------- PBR Functions -----------------

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, EPSILON);
}

// Geometry Function (Smith's method with GGX for both)
float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, EPSILON);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel Equation (Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// Fresnel Equation with roughness (for IBL)
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------- Main Function -----------------
void main() {
    // Sample albedo texture or use base color
    vec4 albedo;
    if (material.hasAlbedoMap > 0) {
        albedo = texture(albedoMap, fragTexCoord);
    } else {
        albedo = material.baseColorFactor;
    }
    
    // Mix vertex color if needed
    albedo.rgb *= fragColor;
    
    // Early discard for alpha cutoff (if implementing alpha cutoff)
    if (albedo.a < 0.1) {
        discard;
    }
    
    // Get material properties
    float metallic = material.metallicFactor;
    float roughness = material.roughnessFactor;
    float ao = material.ambientOcclusion;
    
    // Normalize vectors
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(ubo.cameraPos - fragPosition);
    
    // Surface reflection at zero incidence angle (F0)
    // Dielectrics have F0 of 0.04, metals use their albedo color
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    
    // Initialize lighting components
    vec3 Lo = vec3(0.0);
    
    // Calculate lighting contribution from each light
    for (int i = 0; i < lightBuffer.lightCount; i++) {
        Light light = lightBuffer.lights[i];
        vec3 lightPos = light.position.xyz;
        float lightIntensity = light.color.a;
        vec3 lightColor = light.color.rgb * lightIntensity;
        
        vec3 L;
        float attenuation = 1.0;
        
        // Directional light
        if (light.position.w < 0.5) {
            L = normalize(-lightPos); // Direction from light
        } 
        // Point light
        else {
            // Calculate light direction and attenuation
            vec3 lightVec = lightPos - fragPosition;
            float distance = length(lightVec);
            L = normalize(lightVec);
            
            // Inverse square falloff with smooth falloff near radius edge
            float radiusSq = light.radius * light.radius;
            float distanceSq = distance * distance;
            float falloff = pow(max(1.0 - pow(distanceSq / radiusSq, light.falloff), 0.0), 2.0);
            attenuation = falloff / (1.0 + distanceSq);
        }
        
        // Calculate half-vector
        vec3 H = normalize(V + L);
        
        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // Calculate specular component
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
        vec3 specular = numerator / denominator;
        
        // Energy conservation - no diffuse for pure metals
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        // Calculate final light contribution
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo.rgb / PI + specular) * lightColor * NdotL * attenuation;
    }
    
    // Add ambient lighting (very simple for now)
    vec3 ambient = lightBuffer.ambientColor.rgb * albedo.rgb * ao;
    
    // Combine lighting
    vec3 color = ambient + Lo;
    
    // Add emissive if present
    if (material.hasEmissiveMap > 0) {
        // Would sample emissive map here if we had it
        //vec3 emissive = texture(emissiveMap, fragTexCoord).rgb * material.emissiveFactor;
        //color += emissive;
    }
    
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, albedo.a);
}