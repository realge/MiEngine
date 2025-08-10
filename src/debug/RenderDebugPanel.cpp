#include "../include/debug/RenderDebugPanel.h"
#include "../VulkanRenderer.h"

RenderDebugPanel::RenderDebugPanel(VulkanRenderer* renderer)
    : DebugPanel("Render Debug", renderer) {
}

void RenderDebugPanel::draw() {
    ImGui::SetNextWindowPos(ImVec2(10, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin(panelName.c_str(), &isOpen)) {
        drawRenderModeSection();
        ImGui::Separator();
        drawSceneStatistics();
        ImGui::Separator();
        drawViewportInfo();
        ImGui::Separator();
        drawPipelineInfo();
    }
    ImGui::End();
}

void RenderDebugPanel::drawRenderModeSection() {
    const char* renderModeStr = "Unknown";
    RenderMode mode = renderer->getRenderMode();
    
    switch(mode) {
        case RenderMode::Standard: renderModeStr = "Standard"; break;
        case RenderMode::PBR: renderModeStr = "PBR"; break;
        case RenderMode::PBR_IBL: renderModeStr = "PBR with IBL"; break;
    }
    
    ImGui::Text("Render Mode: %s", renderModeStr);
    
    // Render mode selector
    if (ImGui::BeginCombo("Change Mode", renderModeStr)) {
        if (ImGui::Selectable("Standard", mode == RenderMode::Standard)) {
            renderer->setRenderMode(RenderMode::Standard);
        }
        if (ImGui::Selectable("PBR", mode == RenderMode::PBR)) {
            renderer->setRenderMode(RenderMode::PBR);
        }
        if (renderer->isIBLReady() && ImGui::Selectable("PBR with IBL", mode == RenderMode::PBR_IBL)) {
            renderer->setRenderMode(RenderMode::PBR_IBL);
        }
        ImGui::EndCombo();
    }
}

void RenderDebugPanel::drawSceneStatistics() {
    Scene* scene = renderer->getScene();
    if (scene) {
        ImGui::Text("Scene Statistics:");
        ImGui::Indent();
        ImGui::Text("Lights: %zu", scene->getLights().size());
        ImGui::Text("Mesh Instances: %zu", scene->getMeshInstances().size());
        ImGui::Unindent();
    }
}

void RenderDebugPanel::drawViewportInfo() {
    VkExtent2D extent = renderer->getSwapChainExtent();
    ImGui::Text("Viewport:");
    ImGui::Indent();
    ImGui::Text("Resolution: %dx%d", extent.width, extent.height);
    ImGui::Text("Aspect Ratio: %.2f", extent.width / (float)extent.height);
    ImGui::Text("Near Plane: %.3f", renderer->getNearPlane());
    ImGui::Text("Far Plane: %.1f", renderer->getFarPlane());
    ImGui::Unindent();
}

void RenderDebugPanel::drawPipelineInfo() {
    ImGui::Text("Pipeline Status:");
    ImGui::Indent();
    
    bool pbrEnabled = renderer->isPBRPipelineReady();
    bool iblEnabled = renderer->isIBLReady();
    bool skyboxEnabled = renderer->isSkyboxReady();
    
    ImGui::TextColored(pbrEnabled ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                      "PBR: %s", pbrEnabled ? "Ready" : "Not Available");
    
    ImGui::TextColored(iblEnabled ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                      "IBL: %s", iblEnabled ? "Ready" : "Not Available");
    
    ImGui::TextColored(skyboxEnabled ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                      "Skybox: %s", skyboxEnabled ? "Ready" : "Not Available");
    
    ImGui::Unindent();
}
