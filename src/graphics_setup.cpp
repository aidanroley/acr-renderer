#include "../include/graphics_setup.h"
#include "../include/init.h"

void initGraphics(GraphicsSetup& graphics, VulkanSetup& setup) {

	initUBO(graphics, *setup.swapChainInfo, *setup.uniformData, setup.syncObjects->currentFrame);
}

void initUBO(GraphicsSetup& graphics, SwapChainInfo& swapChainInfo, UniformData& uniformData, uint32_t currentImage) {

	glm::vec3 cameraDirection = graphics.cameraHelper->camera.getCameraDirection();
	glm::vec3 cameraPosition = graphics.cameraHelper->camera.getCameraPosition();
	float fov = graphics.cameraHelper->camera.getCameraFov();

	graphics.ubo->model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // rotate along x-axis
	graphics.ubo->model = glm::rotate(graphics.ubo->model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	// view (camera position, target position, up)
	graphics.ubo->view = glm::lookAt(cameraPosition, cameraPosition + cameraDirection, glm::vec3(0.0f, 1.0f, 0.0f));

	// (fovy, aspect, near, far)
	graphics.ubo->proj = glm::perspective(glm::radians(fov), swapChainInfo.swapChainExtent.width / (float)swapChainInfo.swapChainExtent.height, 0.1f, 10.0f);
	graphics.ubo->proj[1][1] *= -1; // Y flipped in vulkan

	memcpy(uniformData.uniformBuffersMapped[currentImage], &graphics.ubo, sizeof(graphics.ubo));
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
