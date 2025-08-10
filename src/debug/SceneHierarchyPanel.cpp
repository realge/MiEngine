#include "../include/debug/SceneHierarchyPanel.h"
#include "../VulkanRenderer.h"
SceneHierarchyPanel::SceneHierarchyPanel(VulkanRenderer* renderer)
    : DebugPanel("Scene Hierarchy", renderer) {
}

void SceneHierarchyPanel::draw() {
    ImGui::SetNextWindowPos(ImVec2(780, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin(panelName.c_str(), &isOpen)) {
        if (ImGui::CollapsingHeader("Meshes", ImGuiTreeNodeFlags_DefaultOpen)) {
            drawMeshList();
        }
        
        if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            drawLightList();
        }
        
        if (selectedMeshIndex >= 0) {
            ImGui::Separator();
            drawMeshProperties();
        }
    }
    ImGui::End();
}

void SceneHierarchyPanel::drawMeshList() {
    Scene* scene = renderer->getScene();
    if (!scene) return;
    
    const auto& meshInstances = scene->getMeshInstances();
    
    for (size_t i = 0; i < meshInstances.size(); i++) {
        bool isSelected = (selectedMeshIndex == i);
        
        std::string label = "Mesh " + std::to_string(i);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedMeshIndex = static_cast<int>(i);
        }
    }
}

void SceneHierarchyPanel::drawMeshProperties() {
    Scene* scene = renderer->getScene();
    if (!scene) return;
    
    const auto& meshInstances = scene->getMeshInstances();
    if (selectedMeshIndex >= meshInstances.size()) return;
    
    const auto& instance = meshInstances[selectedMeshIndex];
    
    ImGui::Text("Mesh Properties:");
    ImGui::Separator();
    
    // Transform
    glm::vec3 pos = instance.transform.position;
    glm::vec3 rot = instance.transform.rotation;
    glm::vec3 scale = instance.transform.scale;
    
    ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
    ImGui::Text("Rotation: (%.2f, %.2f, %.2f)", rot.x, rot.y, rot.z);
    ImGui::Text("Scale: (%.2f, %.2f, %.2f)", scale.x, scale.y, scale.z);
    
    // Material info
    if (instance.mesh && instance.mesh->getMaterial()) {
        const auto& material = instance.mesh->getMaterial();
        ImGui::Separator();
        ImGui::Text("Material:");
        ImGui::Text("  Metallic: %.2f", material->metallic);
        ImGui::Text("  Roughness: %.2f", material->roughness);
        ImGui::Text("  Alpha: %.2f", material->alpha);
    }
}

void SceneHierarchyPanel::drawLightList() {
    Scene* scene = renderer->getScene();
    if (!scene) return;
    
    const auto& lights = scene->getLights();
    
    for (size_t i = 0; i < lights.size(); i++) {
        const auto& light = lights[i];
        
        std::string label = light.isDirectional ? "Directional Light " : "Point Light ";
        label += std::to_string(i);
        
        if (ImGui::TreeNode(label.c_str())) {
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", 
                       light.position.x, light.position.y, light.position.z);
            ImGui::Text("Color: (%.2f, %.2f, %.2f)", 
                       light.color.r, light.color.g, light.color.b);
            ImGui::Text("Intensity: %.2f", light.intensity);
            
            if (!light.isDirectional) {
                ImGui::Text("Radius: %.2f", light.radius);
                ImGui::Text("Falloff: %.2f", light.falloff);
            }
            
            ImGui::TreePop();
        }
    }
}