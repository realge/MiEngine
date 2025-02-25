#include "../include/loader/ModelLoader.h"
#include <fbxsdk.h>
#include <iostream>

ModelLoader::ModelLoader() {
    // Initialize the FBX Manager
    fbxManager = FbxManager::Create();
    if (!fbxManager) {
        throw std::runtime_error("Error: Unable to create FBX Manager!");
    }

    // Create an IOSettings object.
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(ios);

    // Create the FBX scene.
    fbxScene = FbxScene::Create(fbxManager, "MyScene");
    if (!fbxScene) {
        throw std::runtime_error("Error: Unable to create FBX Scene!");
    }
  
}

ModelLoader::~ModelLoader() {
    // Clean up the FBX objects
    if (fbxScene) {
        fbxScene->Destroy();
    }
    if (fbxManager) {
        fbxManager->Destroy();
    }
}

bool ModelLoader::LoadModel(const std::string& filename) {
    // Create an importer using the FBX SDK.
    FbxImporter* importer = FbxImporter::Create(fbxManager, "");
    if (!importer->Initialize(filename.c_str(), -1, fbxManager->GetIOSettings())) {
        std::cerr << "Failed to initialize FBX importer: " 
                  << importer->GetStatus().GetErrorString() << std::endl;
        importer->Destroy();
        return false;
    }

    // Import the scene from the file.
    if (!importer->Import(fbxScene)) {
        std::cerr << "Failed to import FBX file." << std::endl;
        importer->Destroy();
        return false;
    }
    FbxGeometryConverter geometryConverter(fbxManager);
    geometryConverter.Triangulate(fbxScene, true);
    importer->Destroy();

    // Process the scene starting from the root node.
    FbxNode* rootNode = fbxScene->GetRootNode();
    if (rootNode) {
        for (int i = 0; i < rootNode->GetChildCount(); i++) {
            ProcessNode(rootNode->GetChild(i), 0);
        }
    }
    std::cout << "Total meshes loaded: " << meshes.size() << std::endl;
    return true;
}

void ModelLoader::ProcessNode(FbxNode* node, int indentLevel) {
    if (!node) return;

    std::string indent(indentLevel * 2, ' ');  // 2 spaces per level
    std::cout << indent << "Processing node: " << node->GetName() 
              << " (Children: " << node->GetChildCount() << ")" << std::endl;

    // Get node attributes
    FbxNodeAttribute* attribute = node->GetNodeAttribute();
    if (attribute) {
        FbxNodeAttribute::EType attributeType = attribute->GetAttributeType();
        std::cout << indent << "Node has attribute type: ";
        switch (attributeType) {
        case FbxNodeAttribute::eMesh: 
            std::cout << "Mesh"; 
            break;
        case FbxNodeAttribute::eNull: 
            std::cout << "Null"; 
            break;
        case FbxNodeAttribute::eMarker: 
            std::cout << "Marker"; 
            break;
        default: 
            std::cout << "Other(" << attributeType << ")";
        }
        std::cout << std::endl;
    }

    // If the node contains a mesh, process it
    FbxMesh* fbxMesh = node->GetMesh();
    if (fbxMesh) {
        std::cout << indent << "Found mesh with " 
                  << fbxMesh->GetPolygonCount() << " polygons and "
                  << fbxMesh->GetControlPointsCount() << " vertices" << std::endl;
        ProcessMesh(fbxMesh);
    }

    // Recursively process all children
    for (int i = 0; i < node->GetChildCount(); i++) {
        ProcessNode(node->GetChild(i), indentLevel + 1);
    }
}
void ModelLoader::ProcessMesh(FbxMesh* mesh) {
    std::cout << "Starting mesh processing..." << std::endl;
    MeshData meshData;
    
    // Get vertex positions
    FbxVector4* controlPoints = mesh->GetControlPoints();
    int vertexCount = mesh->GetControlPointsCount();
    std::cout << "Processing " << vertexCount << " control points" << std::endl;
    // Prepare temporary storage for processed vertices
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Process each polygon
    int polygonCount = mesh->GetPolygonCount();
    for (int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++) {
        // FBX polygons should be triangulated, so each polygon should have 3 vertices
        if (mesh->GetPolygonSize(polygonIndex) != 3) {
            std::cerr << "Warning: Non-triangulated polygon found" << std::endl;
            continue;
        }
        
        // Process each vertex of the triangle
        for (int vertexIndex = 0; vertexIndex < 3; vertexIndex++) {
            Vertex vertex{};
            int controlPointIndex = mesh->GetPolygonVertex(polygonIndex, vertexIndex);
            
            // Get position
            FbxVector4 position = controlPoints[controlPointIndex];
            vertex.pos[0] = static_cast<float>(position[0]);
            vertex.pos[1] = static_cast<float>(position[1]);
            vertex.pos[2] = static_cast<float>(position[2]);
            
            // Get UV if available
            // In ModelLoader.cpp, in the ProcessMesh function, modify the UV handling:
            if (mesh->GetElementUV(0)) {
                FbxVector2 uv;
                bool unmapped;
                mesh->GetPolygonVertexUV(polygonIndex, vertexIndex, mesh->GetElementUV(0)->GetName(), uv, unmapped);
    
                // Store U coordinate as is
                vertex.uv[0] = static_cast<float>(uv[0]);
    
                // Flip the V coordinate for Vulkan
                vertex.uv[1] = 1.0f - static_cast<float>(uv[1]);
    
                // Debug
                if (vertices.size() < 10) {
                    std::cout << "Original UV: (" << uv[0] << ", " << uv[1] << "), "
                              << "Flipped UV: (" << vertex.uv[0] << ", " << vertex.uv[1] << ")" << std::endl;
                }
            }
            
            // Get normal
            if (mesh->GetElementNormal(0)) {
                FbxVector4 normal;
                mesh->GetPolygonVertexNormal(polygonIndex, vertexIndex, normal);
                vertex.normal[0] = static_cast<float>(normal[0]);
                vertex.normal[1] = static_cast<float>(normal[1]);
                vertex.normal[2] = static_cast<float>(normal[2]);
            }
            
            // Set default color
            vertex.color[0] = 1.0f;
            vertex.color[1] = 1.0f;
            vertex.color[2] = 1.0f;
            
            // Add vertex and index
            vertices.push_back(vertex);
            indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
        }
    }
    
    meshData.vertices = vertices;
    meshData.indices = indices;
    meshes.push_back(meshData);
    std::cout << "Finished processing mesh. Added " 
          << vertices.size() << " vertices and "
          << indices.size() << " indices" << std::endl;
}
