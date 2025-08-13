#version 450
#extension GL_ARB_separate_shader_objects : enable

const float PI = 3.14159265359;
const int MAX_LIGHTS = 16;

struct Light {
    vec4 position;
    vec4 color;
    float radius;
    float falloff;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) in mat3 TBN;
layout(location = 7) in vec3 fragViewDir;

layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float time;
	 float maxReflectionLod;
    vec3 padding;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 1, binding = 3) uniform sampler2D emissiveMap;
layout(set = 1, binding = 4) uniform sampler2D occlusionMap;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) vec4 baseColorFactor;
    layout(offset = 80) float metallicFactor;
    layout(offset = 84) float roughnessFactor;
    layout(offset = 88) float ambientOcclusion;
    layout(offset = 92) float emissiveFactor;
    layout(offset = 96) int hasAlbedoMap;
    layout(offset = 100) int hasNormalMap;
    layout(offset = 104) int hasMetallicRoughnessMap;
    layout(offset = 108) int hasEmissiveMap;
    layout(offset = 112) int hasOcclusionMap;
} pushConstants;

layout(set = 2, binding = 0) uniform LightBuffer {
    Light lights[MAX_LIGHTS];
    vec4 ambientColor;
    int lightCount;
} lightBuffer;

layout(set = 3, binding = 0) uniform samplerCube environmentMap;
layout(set = 3, binding = 1) uniform samplerCube irradianceMap;
layout(set = 3, binding = 2) uniform samplerCube prefilterMap;
layout(set = 3, binding = 3) uniform sampler2D brdfLUT;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

void main() {
    // Get material properties
    vec4 albedo = pushConstants.baseColorFactor;
    if (pushConstants.hasAlbedoMap > 0) {
        albedo = texture(albedoMap, fragTexCoord) * pushConstants.baseColorFactor;
    }
    albedo.rgb *= fragColor;
    
    float metallic = clamp(pushConstants.metallicFactor, 0.0, 1.0);
    float roughness = clamp(pushConstants.roughnessFactor, 0.0, 1.0);
    float ao = pushConstants.ambientOcclusion;
    
    if (pushConstants.hasMetallicRoughnessMap > 0) {
        vec4 metallicRoughness = texture(metallicRoughnessMap, fragTexCoord);
        roughness = clamp(metallicRoughness.g * roughness, 0.00, 1.0);
        metallic = clamp(metallicRoughness.b * metallic, 0.0, 1.0);
    }
    
    if (pushConstants.hasOcclusionMap > 0) {
        ao = texture(occlusionMap, fragTexCoord).r;
    }
    
    // Calculate vectors
    vec3 N = normalize(fragNormal);
    if (pushConstants.hasNormalMap > 0) {
        vec3 tangentNormal = texture(normalMap, fragTexCoord).rgb * 2.0 - 1.0;
        N = normalize(TBN * tangentNormal);
    }
    
    vec3 V = normalize(ubo.cameraPos - fragPosition);
    vec3 R = reflect(-V, N);
    
    // Calculate F0
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    
    // Direct lighting
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < lightBuffer.lightCount; i++) {
        Light light = lightBuffer.lights[i];
        vec3 L;
        float attenuation = 1.0;
        
        if (light.position.w < 0.5) {
            L = normalize(-light.position.xyz);
        } else {
            vec3 lightVec = light.position.xyz - fragPosition;
            float distance = length(lightVec);
            L = normalize(lightVec);
            
            if (light.radius > 0.0) {
                float falloff = clamp(1.0 - (distance / light.radius), 0.0, 1.0);
                attenuation = falloff * falloff;
            }
        }
        
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        
        if (NdotL > 0.0) {
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
            vec3 specular = numerator / denominator;
            
            vec3 radiance = light.color.rgb * light.color.a * attenuation;
            Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL;
        }
    }
    
    // IBL
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    // Diffuse IBL from irradiance map
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo.rgb;
	
		float mipLevel = roughness * ubo.maxReflectionLod;

    
    
	
    vec3 prefilteredColor = textureLod(prefilterMap, R, mipLevel).rgb;
    
    // Sample BRDF LUT with correct coordinates
    float NdotV = max(dot(N, V), 0.0);
    vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg; // R=scale, G=bias
    
    // Apply split-sum approximation
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    
    vec3 ambient = (kD * diffuse + specular) * ao;
    
    // Add emissive
    vec3 emissive = vec3(0.0);
    if (pushConstants.hasEmissiveMap > 0) {
        emissive = texture(emissiveMap, fragTexCoord).rgb * pushConstants.emissiveFactor;
    }
    
    vec3 color = ambient + Lo + emissive;
    
    // Tone mapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    outColor = vec4(color, albedo.a);
	

}