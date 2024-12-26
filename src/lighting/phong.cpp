#include "../../include/init.h"

const glm::vec3 CORNELL_BOX_LIGHT_POSITION = glm::vec3(0.0f, 1.98f, -0.03f);

// Universal ambient for all vertices
void applyAmbientLighting(VertexData& vertexData) {

    glm::vec3 lightGray = glm::vec3(0.8f, 0.8f, 0.8f); // Bright gray
    float ambientStrength = 0.1;
    glm::vec3 ambient = lightGray * ambientStrength;

    for (Vertex& vertex : vertexData.vertices) {

        glm::vec3 result = ambient * vertex.color;
        vertex.color = result;
    }
}