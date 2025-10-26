#include "pch.h"
#include "glEng/pbr_pipeline.h"
#include "glEng/gl_engine.h"

PBRSystem::MaterialInstance PBRSystem::writeMaterial(MaterialPass pass, const PBRSystem::MaterialResources& resources, glEngine* engine) {

    MaterialInstance mat;
    mat.type = pass;
    mat.pipelineProgram = engine->_gltfData.prog.getID();
    mat.ubo = resources.dataBuffer;
    mat.uboOffset = resources.dataBufferOffset;
    mat.resources = resources;
    return mat;
}