#pragma once
#include <glad/glad.h>
#include <glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <vector>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void SetupMesh();
    void Draw();
};

class Model {
public:
    bool LoadModel(const std::string& path);
    void Draw();

    bool IsLoaded() const { return isLoaded; }
    size_t GetTotalVertices() const {
        size_t total = 0;
        for (const auto& m : meshes) total += m.vertices.size();
        return total;
    }
    size_t GetTotalFaces() const {
        size_t total = 0;
        for (const auto& m : meshes) total += m.indices.size() / 3;
        return total;
    }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    bool isLoaded = false;

    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};