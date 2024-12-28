#include "../../include/init.h"
#include"../../include/scene_info/Cornell_Box.h"

const glm::vec3 CORNELL_BOX_LIGHT_POSITION = glm::vec3(0.0f, 1.98f, -0.03f);
const glm::vec3 CORNELL_BOX_LIGHT_COLOR = glm::vec3(17.0f, 12.0f, 4.0f);
const float CORNELL_BOX_SPECULAR_STRENGTH = 0.5;
//const glm::vec3 CORNELL_BOX_LIGHT_COLOR = glm::vec3(1.0f, 1.0f, 1.0f);

void setLightData_CornellBox(UniformBufferObject& ubo, CameraHelper& cameraHelper) {

    ubo.lightPos = CORNELL_BOX_LIGHT_POSITION;
    ubo.lightColor = CORNELL_BOX_LIGHT_COLOR;
    ubo.viewPos = cameraHelper.camera.getCameraPosition();

}

void updateInfo_CornellBox(GraphicsSetup& graphics) {

    graphics.ubo->viewPos = graphics.cameraHelper->camera.getCameraPosition();
}