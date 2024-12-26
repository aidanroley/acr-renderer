#include "../../include/init.h"

const glm::vec3 CORNELL_BOX_LIGHT_POSITION = glm::vec3(0.0f, 1.98f, -0.03f);
const glm::vec3 CORNELL_BOX_LIGHT_COLOR = glm::vec3(17.0f, 12.0f, 4.0f);

// Universal ambient for all vertices
void applyAmbientLighting_CornellBox(VertexData& vertexData) {

    glm::vec3 lightGray = glm::vec3(0.8f, 0.8f, 0.8f); // Bright gray
    float ambientStrength = 0.1;
    glm::vec3 ambient = lightGray * ambientStrength;

    for (Vertex& vertex : vertexData.vertices) {

        glm::vec3 result = ambient * vertex.color;
        vertex.color = result;
    }
}

void setLightData_CornellBox(UniformBufferObject& ubo) {

    ubo.lightPos = CORNELL_BOX_LIGHT_POSITION;
    ubo.lightColor = CORNELL_BOX_LIGHT_COLOR;
}