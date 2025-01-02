#pragma once
#include "../../precompile/pch.h"
#include "../../include/scene_info/Sun_Temple.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <unordered_map>
#include <cmath>

const std::string SUN_TEMPLE_MODEL_PATH = "assets/SunTemple/SunTemple.glb";

// Helper funcs for vertices
bool areVec3Equal(const glm::vec3& a, const glm::vec3& b, float epsilon) {

    return std::abs(a.x - b.x) < epsilon &&
        std::abs(a.y - b.y) < epsilon &&
        std::abs(a.z - b.z) < epsilon;
}

void loadModel_SunTemple(VertexData& vertexData) {

    auto parser = fastgltf::Parser();
    auto data = fastgltf::GltfDataBuffer::FromPath(SUN_TEMPLE_MODEL_PATH);
    if (data.error() != fastgltf::Error::None) {

        std::cerr << "file load after fromPath failed ;_;";
    }

    // It's worth noting asset's type becomes fastgltf::Expected<class fastgltf::Asset> and is used in similar fashion to a smart pointer
    auto asset = parser.loadGltfBinary(data.get(), "assets/SunTemple", fastgltf::Options::None);

    if (asset.error() != fastgltf::Error::None) {

        std::ostringstream errorMessage;
        errorMessage << "Error: Failed to load glTF: " << fastgltf::getErrorMessage(asset.error());
        throw std::invalid_argument(errorMessage.str());
    }

    auto validationErrors = fastgltf::validate(asset.get());
    if (validationErrors != fastgltf::Error::None) {

        std::cout << "Validation Errors Found:" << std::endl;
    }
    else {

        std::cout << "The asset is valid!" << std::endl;
    }

    //auto [mesh, meshRenderer] = 
        loadMeshData(asset.get(), vertexData);

        glm::mat3 flipMatrix = glm::mat3(1.0f);
        flipMatrix[1][1] = -1.0f;
        
        // Define the 90-degree rotation matrix for X-axis
        glm::mat3 rotationMatrix = glm::mat3(1.0f); // Identity matrix as a base
        rotationMatrix[1][1] = 0.0f;  // Set Y-Y component
        rotationMatrix[1][2] = -1.0f; // Set Y-Z component
        rotationMatrix[2][1] = 1.0f;  // Set Z-Y component
        rotationMatrix[2][2] = 0.0f;  // Set Z-Z component

        // Apply the rotation to your vertices
        for (auto& vertex : vertexData.vertices) {
            vertex.pos = flipMatrix * rotationMatrix * vertex.pos;
        }
}

void loadMeshData(const fastgltf::Asset& asset, VertexData& vertexData) {

    const std::vector<fastgltf::Mesh>& meshes = asset.meshes;
    std::unordered_map<glm::vec3, uint32_t, Vec3Hash, Vec3Equal> uniqueVertices;

    std::cout << "loading " + std::to_string(meshes.size()) + " meshes";
    for (std::size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++) {

        for (const fastgltf::Primitive& primitive : meshes[meshIdx].primitives) {

            if (!primitive.indicesAccessor.has_value()) {

                throw std::invalid_argument("error: gltf file needs indexed geometry");
            }
            loadMeshIndices(asset, primitive, vertexData);
            loadMeshVertices(asset, &primitive, vertexData, uniqueVertices);
        }
    }
}

void loadMeshVertices(const fastgltf::Asset& asset, const fastgltf::Primitive* primitive, VertexData& vertexData, std::unordered_map<glm::vec3, uint32_t, Vec3Hash, Vec3Equal>& uniqueVertices) {

    auto* positionIterator = primitive->findAttribute("POSITION");
    if (!positionIterator) {

        throw std::invalid_argument("POSITION attribute not found.");
    }
    auto& posAccessor = asset.accessors[positionIterator->accessorIndex];
    if (!posAccessor.bufferViewIndex.has_value()) {

        //
    }

    constexpr float scaleFactor = 0.0006f;
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, posAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {

            glm::vec3 vertexPos = glm::vec3(pos.x(), pos.y(), pos.z());
            vertexPos *= scaleFactor;

            // Check if the vertex is already in the unordered_map
            //if (uniqueVertices.find(vertexPos) == uniqueVertices.end()) {

                // If not, add it to the vertex buffer and the map
                Vertex vertex{};
                vertex.pos = vertexPos;
                uniqueVertices[vertexPos] = static_cast<uint32_t>(vertexData.vertices.size());
                vertexData.vertices.push_back(vertex);
            //}

            // Add the index of this vertex to the indices buffer
        });
}

void loadMeshIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, VertexData& vertexData) {

    // Indices must be offset by the size of vertex buffer at the beginning of this function since the indices in the
    // GLTF file are relative to the primitive, not all vertices
    int startingIdx = vertexData.vertices.size();

    std::vector<std::uint32_t> indices;
    if (primitive.indicesAccessor.has_value()) {

        auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
        indices.resize(accessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](std::uint32_t index) {

            vertexData.indices.push_back(index + startingIdx);
        });
    }

}

