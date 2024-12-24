#include "../include/graphics_setup.h"
#include "../include/init.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h" 

void initGraphics(GraphicsSetup& graphics, VulkanSetup& setup) {

	initUBO(graphics, *setup.swapChainInfo, *setup.uniformData, setup.syncObjects->currentFrame);
    //loadModel(*graphics.vertexData);
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
        std::cout << "Shape: " << shape.name << std::endl;
        std::cout << "Material IDs: ";
        for (size_t i = 0; i < shape.mesh.material_ids.size(); ++i) {
            std::cout << shape.mesh.material_ids[i] << " ";
        }
        std::cout << std::endl;
    }

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
