#pragma once

#include <string>
#include <vector>
#include <fbxsdk.h>
#include <../include/Utils/CommonVertex.h>
// A simple structure to hold vertex data (position only for now)


// Structure to hold a mesh's data
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

class ModelLoader {
public:
   
    ModelLoader();
    ~ModelLoader();

    // Loads an FBX file and populates internal mesh data
    bool LoadModel(const std::string& filename);

    // Returns the loaded meshes
    const std::vector<MeshData>& GetMeshData() const { return meshes; }

private:
    // Recursively process each node in the FBX scene
    void ProcessNode(FbxNode* node, int indentLevel);

    // Extract mesh data from an FBX mesh node
    void ProcessMesh(FbxMesh* mesh);

    // Storage for the meshes loaded from the FBX file
    std::vector<MeshData> meshes;

    // FBX SDK objects for managing the scene
    FbxManager* fbxManager;
    FbxScene* fbxScene;
};
