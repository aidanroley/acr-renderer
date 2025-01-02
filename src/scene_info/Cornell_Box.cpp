#include "../../precompile/pch.h"
#include"../../include/scene_info/Cornell_Box.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <iostream>

const glm::vec3 CORNELL_BOX_LIGHT_POSITION = glm::vec3(0.0f, 1.98f, -0.03f);
const glm::vec3 CORNELL_BOX_LIGHT_COLOR = glm::vec3(17.0f, 12.0f, 4.0f);
const float CORNELL_BOX_SPECULAR_STRENGTH = 0.5;

const std::string CORNELL_BOX_MODEL_PATH = "assets/CornellBox-Original.obj";

void loadModel_CornellBox(VertexData& vertexData) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, CORNELL_BOX_MODEL_PATH.c_str(), MATERIAL_PATH.c_str())) {

        throw std::runtime_error(warn + err);
    }

    // The key of the map is the vertex data
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {

        // Processing one face at time which is 4 verties specified in obj
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {

            const auto& index0 = shape.mesh.indices[i + 0];
            const auto& index1 = shape.mesh.indices[i + 1];
            const auto& index2 = shape.mesh.indices[i + 2];

            Vertex vertex0{}, vertex1{}, vertex2{};

            // Get vertex position for each vertex (to pass into buffer)
            auto getVertexPosition = [&](int vertexIndex) -> glm::vec3 {

                if (vertexIndex < 0) {

                    vertexIndex = attrib.vertices.size() / 3 + vertexIndex; // in negative vertices, -1 refers to last one
                }
                return glm::vec3{

                    attrib.vertices[3 * vertexIndex + 0],
                    attrib.vertices[3 * vertexIndex + 1],
                    attrib.vertices[3 * vertexIndex + 2]
                };
                };
            glm::vec3 pos0 = getVertexPosition(index0.vertex_index);
            glm::vec3 pos1 = getVertexPosition(index1.vertex_index);
            glm::vec3 pos2 = getVertexPosition(index2.vertex_index);

            vertex0.pos = pos0;
            vertex1.pos = pos1;
            vertex2.pos = pos2;

            // get texcoord for each vertex
            auto setTexCoord = [&](Vertex& vertex, const tinyobj::index_t& index) {

                if (index.texcoord_index >= 0) {

                    vertex.texCoord = {

                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                }
                else {

                    vertex.texCoord = { 0.0f, 0.0f };
                }
                };
            setTexCoord(vertex0, index0);
            setTexCoord(vertex1, index1);
            setTexCoord(vertex2, index2);

            // get normals for each vertex by first calculating face normal then assigning it to the vertices (if not specified by obj)
            glm::vec3 edge1 = pos1 - pos0;
            glm::vec3 edge2 = pos2 - pos0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            auto setVertexNormal = [&](Vertex& vertex, const tinyobj::index_t& index) {

                if (index.normal_index >= 0) {

                    int normalIndex = index.normal_index;
                    vertex.normal = glm::normalize(glm::vec3(

                        attrib.normals[3 * normalIndex + 0],
                        attrib.normals[3 * normalIndex + 1],
                        attrib.normals[3 * normalIndex + 2]
                    ));
                }
                else {

                    vertex.normal = faceNormal;
                }
                };

            setVertexNormal(vertex0, index0);
            setVertexNormal(vertex1, index1);
            setVertexNormal(vertex2, index2);

            // get color from mtl
            int materialIndex = shape.mesh.material_ids[i / 3];
            auto setVertexColor = [&](Vertex& vertex) {

                if (materialIndex >= 0 && materialIndex < materials.size()) {

                    const auto& material = materials[materialIndex];

                    if (material.emission[0] != 0 || material.emission[1] != 0 || material.emission[2] != 0) {

                        vertex.isEmissive = true;
                    }

                    vertex.color = {

                        material.diffuse[0],
                        material.diffuse[1],
                        material.diffuse[2]
                    };
                    if (vertex.color.x > 0.724f && vertex.color.x < 0.726f) {
                        vertex.color.x == 0.1f;
                    }
                }
                else {

                    vertex.color = { 1.0f, 1.0f, 1.0f };
                }
                };
            setVertexColor(vertex0);
            setVertexColor(vertex1);
            setVertexColor(vertex2);

            for (const Vertex& vertex : { vertex0, vertex1, vertex2 }) {

                if (uniqueVertices.count(vertex) == 0) {

                    uniqueVertices[vertex] = static_cast<uint32_t>(vertexData.vertices.size());
                    vertexData.vertices.push_back(vertex);
                }
                vertexData.indices.push_back(uniqueVertices[vertex]);
            }

            for (const Vertex& vertex : { vertex0, vertex1, vertex2 }) {

                std::cout << "color" << vertex.color.x << " " << vertex.color.y << " " << vertex.color.z << std::endl;
            }
        }
    }
}

void setLightData_CornellBox(UniformBufferObject& ubo, CameraHelper& cameraHelper) {

    ubo.lightPos = CORNELL_BOX_LIGHT_POSITION;
    ubo.lightColor = CORNELL_BOX_LIGHT_COLOR;
    ubo.viewPos = cameraHelper.camera.getCameraPosition();

}

void updateInfo_CornellBox(GraphicsSetup& graphics) {

    graphics.ubo->viewPos = graphics.cameraHelper->camera.getCameraPosition();
}