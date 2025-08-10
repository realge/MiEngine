#pragma once
#include "DebugPanel.h"

class SceneHierarchyPanel : public DebugPanel {
public:
    SceneHierarchyPanel(VulkanRenderer* renderer);
    void draw() override;
    
private:
    int selectedMeshIndex = -1;
    
    void drawMeshList();
    void drawMeshProperties();
    void drawLightList();
};
