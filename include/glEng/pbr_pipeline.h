#pragma once
#include "glEng/gl_types.h"

class glEngine;

class PBRSystem {

public:

    enum class MaterialPass :uint8_t {

        Opaque,
        Transparent,
        Transmission,
        Other
    };

    struct alignas(16) MaterialPBRConstants {

        glm::vec4 colorFactors;        // 16 bytes
        glm::vec4 metalRoughFactors;   // 16 bytes

        glm::vec4 volume;              // usesVolume (x), thicknessFactor (y), attenuationDistance (z), unused (w)
        glm::vec4 attenuationColor;    // attenuationColor.rgb, .a unused

        glm::vec4 transmission;        // usesTransmission (x), transmissionFactor (y), unused (zw)

        glm::vec4 extra[11];           // pad to 256 bytes if you need
    };

    struct MaterialResources {

        GLTexture albedo;
        GLTexture metalRough;
        GLTexture occlusion;
        GLTexture normalMap;
        GLTexture transmission;
        GLTexture volumeThickness;
        GLuint dataBuffer;
        GLintptr dataBufferOffset;
    };

    struct MaterialInstance {

        MaterialPass type;
        GLuint pipelineProgram;
        GLuint ubo;
        GLuint uboOffset;
        MaterialResources resources;
    };

    MaterialInstance writeMaterial(MaterialPass pass, const PBRSystem::MaterialResources& resources, glEngine* engine);
};

