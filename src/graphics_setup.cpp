#include "../include/graphics_setup.h"
#include "../include/init.h"
#include "../include/lighting/phong.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h" 
#include <iostream>

void initGraphics(GraphicsSetup& graphics, VulkanSetup& setup) {

    setLightData_CornellBox(*graphics.ubo);
    initUBO(graphics, *setup.swapChainInfo, *setup.uniformData, setup.syncObjects->currentFrame);
}

void initUBO(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage) {

    glm::vec3 cameraDirection = graphics.cameraHelper->camera.getCameraDirection();
    glm::vec3 cameraPosition = graphics.cameraHelper->camera.getCameraPosition();
    float fov = graphics.cameraHelper->camera.getCameraFov();

    graphics.ubo->model = glm::mat4(1.0f); // Identity matrix, no rotation

    // view (camera position, target position, up)
    graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

    // (fovy, aspect, near, far)
    graphics.ubo->proj = glm::perspective(glm::radians(fov), swapChainInfo.swapChainExtent.width / (float)swapChainInfo.swapChainExtent.height, 0.1f, 10.0f);
    graphics.ubo->proj[1][1] *= -1; // Y flipped in vulkan

    memcpy(uniformData.uniformBuffersMapped[currentImage], &graphics.ubo, sizeof(graphics.ubo));
}

void populateVertexBuffer(VertexData& vertexData) {

    loadModel(vertexData);
    applyAmbientLighting_CornellBox(vertexData);
}


void loadModel(VertexData& vertexData) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str(), MATERIAL_PATH.c_str())) {

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
                    vertex.color = {

                        material.diffuse[0],
                        material.diffuse[1],
                        material.diffuse[2]
                    };
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
        }
    }

    /*
    
    for (const auto& shape : shapes) {

        for (size_t i = 0; i < shape.mesh.indices.size(); i++) {

            const auto& index = shape.mesh.indices[i];

            Vertex vertex{};

            int vertexIndex = index.vertex_index;
            
            if (vertexIndex < 0) {

                vertexIndex = attrib.vertices.size() / 3 + vertexIndex; // in negative vertices, -1 refers to the last one
            }
            vertex.pos = {

                attrib.vertices[3 * vertexIndex + 0],
                attrib.vertices[3 * vertexIndex + 1],
                attrib.vertices[3 * vertexIndex + 2]
            };
            if (index.texcoord_index >= 0) {

                vertex.texCoord = {

                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
            else {

                vertex.texCoord = { 0.0f, 0.0f };
            }
            
            if (index.normal_index >= 0) {

                int normalIndex = index.normal_index;
                vertex.normal = {

                    attrib.normals[3 * normalIndex + 0],
                    attrib.normals[3 * normalIndex + 1],
                    attrib.normals[3 * normalIndex + 2]
                };
            }
            else {

                vertex.normal = { 0.0f, 0.0f, 0.0f };
            }
            int materialIndex = shape.mesh.material_ids[i / 3];
            if (materialIndex >= 0 && materialIndex < materials.size()) {

                const auto& material = materials[materialIndex];
                vertex.color = {

                    material.diffuse[0],
                    material.diffuse[1],
                    material.diffuse[2]
                };
            }
            else {

                vertex.color = { 1.0f, 1.0f, 1.0f };
            }
            if (uniqueVertices.count(vertex) == 0) {

                uniqueVertices[vertex] = static_cast<uint32_t>(vertexData.vertices.size());
                vertexData.vertices.push_back(vertex);
            }
            vertexData.indices.push_back(uniqueVertices[vertex]);
        }
    }
    */
}

// look into push constants at some point
void updateUniformBuffers(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage) {

    bool viewNeedsUpdate = graphics.cameraHelper->camera.directionChanged || graphics.cameraHelper->camera.posChanged;
    bool projNeedsUpdate = graphics.cameraHelper->camera.zoomChanged;

    if (viewNeedsUpdate) {

        glm::vec3 cameraDirection = graphics.cameraHelper->camera.getCameraDirection();
        glm::vec3 cameraPosition = graphics.cameraHelper->camera.getCameraPosition();
        graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

        graphics.cameraHelper->camera.directionChanged = false;
        graphics.cameraHelper->camera.posChanged = false;
    }

    if (projNeedsUpdate) {

        float fov = graphics.cameraHelper->camera.getCameraFov();
        graphics.ubo->proj = glm::perspective(glm::radians(fov), swapChainInfo.swapChainExtent.width / (float)swapChainInfo.swapChainExtent.height, 0.1f, 10.0f);
        graphics.ubo->proj[1][1] *= -1;
        
        graphics.cameraHelper->camera.zoomChanged = false;
    }

    if (viewNeedsUpdate || projNeedsUpdate) {
        
        memcpy(uniformData.uniformBuffersMapped[currentImage], graphics.ubo, sizeof(UniformBufferObject));
    }
}
