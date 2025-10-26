#include "pch.h"
#include "glEng/gl_engine.h"
#include "glEng/gltf_loader.h"
#include "glEng/mesh_utils.h"
#include <cmath>
#include "stb_image.h"

std::shared_ptr<gltfData> gltfData::Load(glEngine* engine, std::filesystem::path path) {

    std::shared_ptr<gltfData> scene = std::make_shared<gltfData>();
    gltfData& file = *scene;
    fastgltf::Asset gltf = file.getGltfAsset(path);
    GltfLoadContext ctx = { scene, &gltf, engine };

    // each main portion of loading a gltf (into the class; no drawing) here
    std::vector<GLSampler> samplers = file.createSamplers(ctx);
    std::vector<GLImage> images = file.createImages(ctx);
    std::vector<std::shared_ptr<gltfMaterial>> materials = file.loadMaterials(ctx, samplers, images);
    std::vector<std::shared_ptr<MeshAsset>> vecMeshes = file.loadMeshes(ctx, materials);
    std::vector<std::shared_ptr<Node>> nodes = file.loadNodes(ctx, vecMeshes);
    return scene;
}

fastgltf::Asset gltfData::getGltfAsset(std::filesystem::path path) {

    /* Load file */
    fastgltf::Parser parser{ 

        fastgltf::Extensions::KHR_materials_volume
      | fastgltf::Extensions::KHR_materials_transmission 
    };

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember
        | fastgltf::Options::AllowDouble; // allows double floating point nums instead of float
        
    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    std::cout << std::filesystem::absolute(path) << std::endl;
    auto asset = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
    if (asset.error() != fastgltf::Error::None) {

        std::cerr << "Failed to load glTF: " << fastgltf::getErrorMessage(asset.error()) << '\n';
    }
    fastgltf::Asset gltf = std::move(asset.get());
    return gltf;
}

std::vector<GLSampler> gltfData::createSamplers(GltfLoadContext ctx) {

    std::vector<GLSampler> samplers;

    for (fastgltf::Sampler& sampler : ctx.gltf->samplers) {
        GLSampler s{};
        glGenSamplers(1, &s.id);

        s.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        s.minFilter = extract_mipmap_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        s.wrapS = extract_wrap(sampler.wrapS);
        s.wrapT = extract_wrap(sampler.wrapT);

        // Apply to GL
        glSamplerParameteri(s.id, GL_TEXTURE_MAG_FILTER, s.magFilter);
        glSamplerParameteri(s.id, GL_TEXTURE_MIN_FILTER, s.minFilter);
        glSamplerParameteri(s.id, GL_TEXTURE_WRAP_S, s.wrapS);
        glSamplerParameteri(s.id, GL_TEXTURE_WRAP_T, s.wrapT);

        samplers.push_back(s);
    }
    return samplers;
}

std::optional<GLImage> loadImage(const fastgltf::Asset& asset, const fastgltf::Image& image) {

    int width, height, nrChannels;
    unsigned char* data = nullptr;


    if (std::holds_alternative<fastgltf::sources::URI>(image.data)) {

        auto& filePath = std::get<fastgltf::sources::URI>(image.data);
        std::string path(filePath.uri.path().begin(), filePath.uri.path().end());
        data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    }
    else if (std::holds_alternative<fastgltf::sources::Vector>(image.data)) {

        auto& vector = std::get<fastgltf::sources::Vector>(image.data);
        data = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
            int(vector.bytes.size()), &width, &height, &nrChannels, 4);
    }
    else if (std::holds_alternative<fastgltf::sources::BufferView>(image.data)) {

        auto& view = std::get<fastgltf::sources::BufferView>(image.data);
        auto& bufView = asset.bufferViews[view.bufferViewIndex];
        auto& buffer = asset.buffers[bufView.bufferIndex];

        const stbi_uc* rawBytes = nullptr;

        if (std::holds_alternative<fastgltf::sources::Vector>(buffer.data)) {

            auto& vector2 = std::get<fastgltf::sources::Vector>(buffer.data);
            rawBytes = reinterpret_cast<const stbi_uc*>(vector2.bytes.data());
        }
        else if (std::holds_alternative<fastgltf::sources::Array>(buffer.data)) {

            auto& array2 = std::get<fastgltf::sources::Array>(buffer.data);
            rawBytes = reinterpret_cast<const stbi_uc*>(array2.bytes.data());
        }

        if (rawBytes) {

            data = stbi_load_from_memory(rawBytes + bufView.byteOffset,
                int(bufView.byteLength),
                &width, &height, &nrChannels, 4);
        }
    }

    if (!data) return {};

    GLenum external = GL_RGBA;
    GLuint linearId;
    GLuint sRGBID;

    {
        GLint internal = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;

        glGenTextures(1, &linearId);
        glBindTexture(GL_TEXTURE_2D, linearId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, external, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    // --- sRGB variant ---
    {
        GLint internal = (nrChannels == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8;
        glGenTextures(1, &sRGBID);
        glBindTexture(GL_TEXTURE_2D, sRGBID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0, external, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

 

    /*
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    */
    
    stbi_image_free(data);

    return GLImage{ 0, width, height, external, linearId, sRGBID };
}

std::vector<GLImage> gltfData::createImages(GltfLoadContext ctx) {

    std::vector<GLImage> images;

    // *potential bug make sure the line   materialResources.colorImage = vecImages[img];   the images are in the same order as this enhanced for loop
    /* Load images */
    for (fastgltf::Image& image : ctx.gltf->images) {

        auto img = loadImage(*ctx.gltf, image);

        if (img.has_value()) {

            images.push_back(*img);
        }
        else {

            std::cout << "glTF failed image loading\n";

            // Push fallback 1x1 magenta
            GLuint texID;
            glGenTextures(1, &texID);
            glBindTexture(GL_TEXTURE_2D, texID);
            uint32_t magenta = 0xFF00FFFF;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &magenta);

            images.push_back({ texID, 1, 1, GL_RGBA });
        }
    }
    return images;
}

void gltfData::fetchPBRTextures(fastgltf::Material& mat, GltfLoadContext ctx, PBRSystem::MaterialResources& materialResources, std::vector<GLSampler>& samplers, std::vector<GLImage>& images) {

    auto assignTex = [&](auto& dst, auto& texOpt, bool isSRGB) {

        if (!texOpt.has_value()) return;

        auto& tex = ctx.gltf->textures[texOpt.value().textureIndex];
        if (!tex.imageIndex.has_value() || !tex.samplerIndex.has_value()) return;

        dst.image = images[tex.imageIndex.value()];
        dst.sampler = samplers[tex.samplerIndex.value()];

        if (isSRGB) { dst.image.id = dst.image.sRGBID; }
        else { dst.image.id = dst.image.linearID; }
        };

    // emissive is true :)

    assignTex(materialResources.albedo, mat.pbrData.baseColorTexture, true);
    assignTex(materialResources.metalRough, mat.pbrData.metallicRoughnessTexture, false);
    assignTex(materialResources.occlusion, mat.occlusionTexture, false);
    assignTex(materialResources.normalMap, mat.normalTexture, false);

    if (mat.transmission) {

        assignTex(materialResources.transmission, mat.transmission->transmissionTexture, false);
    }
    if (mat.volume) {

        assignTex(materialResources.volumeThickness, mat.volume->thicknessTexture, false);
    }
}

static PBRSystem::MaterialPBRConstants populatePBRConstants(fastgltf::Material& mat, PBRSystem::MaterialPass* passType) {

    PBRSystem::MaterialPBRConstants pbrConstants; 
    
    pbrConstants.transmission.x = 0;
    pbrConstants.volume.x = 0;

    pbrConstants.colorFactors.x = mat.pbrData.baseColorFactor[0];
    pbrConstants.colorFactors.y = mat.pbrData.baseColorFactor[1];
    pbrConstants.colorFactors.z = mat.pbrData.baseColorFactor[2];
    pbrConstants.colorFactors.w = mat.pbrData.baseColorFactor[3];

    // x = metallic, y = roughness.
    pbrConstants.metalRoughFactors.x = mat.pbrData.metallicFactor;
    pbrConstants.metalRoughFactors.y = mat.pbrData.roughnessFactor;

    if (mat.transmission) {
        // MAKE PASS TYPE TRANSPARENT
        const auto& tr = *mat.transmission;

        pbrConstants.transmission.x = ((tr.transmissionFactor > 0.0f) || tr.transmissionTexture.has_value());

        if (tr.transmissionFactor > 0.0f) {

            pbrConstants.transmission.y = tr.transmissionFactor;
        }

    }
    if (mat.volume) {

        const auto& vol = *mat.volume;

        bool hasThickness = (vol.thicknessFactor > 0.0f) || vol.thicknessTexture.has_value();

        bool hasFiniteAttenuation = std::isfinite(vol.attenuationDistance) && (vol.attenuationDistance > 0.0f);

        bool hasAttenuationColor =
            (vol.attenuationColor[0] > 0.0f) ||
            (vol.attenuationColor[1] > 0.0f) ||
            (vol.attenuationColor[2] > 0.0f);

        if (hasThickness) pbrConstants.volume.y = vol.thicknessFactor;
        if (hasFiniteAttenuation) pbrConstants.volume.z = vol.attenuationDistance;
        if (hasAttenuationColor) pbrConstants.attenuationColor = glm::vec4(glm::vec3(vol.attenuationColor[0], vol.attenuationColor[1], vol.attenuationColor[2]), 1.0f);

        pbrConstants.volume.x = hasThickness || hasFiniteAttenuation || hasAttenuationColor;
    }
    if (pbrConstants.transmission.x || pbrConstants.volume.x) {

        *passType = PBRSystem::MaterialPass::Transmission;
    }

    return pbrConstants;
}

std::vector<std::shared_ptr<gltfMaterial>> gltfData::loadMaterials(GltfLoadContext ctx, std::vector<GLSampler>& samplers, std::vector<GLImage>& images) {

    /* Load materials */

    GLuint materialUBO;
    glGenBuffers(1, &materialUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, materialUBO);

    GLsizeiptr totalSize = sizeof(PBRSystem::MaterialPBRConstants) * ctx.gltf->materials.size();

    glBufferData(GL_UNIFORM_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);

    ctx.scene->materialDataBuffer = materialUBO;

    std::vector<std::shared_ptr<gltfMaterial>> materials;
    int dataIdx = 0;
    for (fastgltf::Material& mat : ctx.gltf->materials) {

        std::shared_ptr<gltfMaterial> newMat = std::make_shared<gltfMaterial>();
        materials.push_back(newMat);

        // set pass type to opaque by default
        PBRSystem::MaterialPass passType = PBRSystem::MaterialPass::Opaque;

        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {

            passType = PBRSystem::MaterialPass::Transparent;
        }

        // store pbr data
        // pbr can change whether it needs transparent pipeline (volume/transmission)
        PBRSystem::MaterialPBRConstants pbrConstants = populatePBRConstants(mat, &passType);



        if (passType == PBRSystem::MaterialPass::Opaque) std::cout << "YAY!!!!" << std::endl;
        if (passType == PBRSystem::MaterialPass::Transparent) std::cout << "YAY 2222!!!!" << std::endl;
        if (passType == PBRSystem::MaterialPass::Transmission) std::cout << "YAY 33333!!!!" << std::endl;
        std::cout << mat.name << std::endl;

        PBRSystem::MaterialResources materialResources;

        // defaults in case none available
        materialResources.albedo.image = ctx.engine->_whiteImage;
        materialResources.albedo.sampler = ctx.engine->_defaultSamplerLinear;
        materialResources.metalRough.image = ctx.engine->_whiteImage;
        materialResources.metalRough.sampler = ctx.engine->_defaultSamplerLinear;
        materialResources.occlusion.image = ctx.engine->_whiteImage;
        materialResources.occlusion.sampler = ctx.engine->_defaultSamplerLinear;
        materialResources.normalMap.image = ctx.engine->_whiteImage;
        materialResources.normalMap.sampler = ctx.engine->_defaultSamplerLinear;
        materialResources.transmission.image = ctx.engine->_whiteImage;
        materialResources.transmission.sampler = ctx.engine->_defaultSamplerLinear;
        materialResources.volumeThickness.image = ctx.engine->_whiteImage;
        materialResources.volumeThickness.sampler = ctx.engine->_defaultSamplerLinear;

        // this below data buffer is gonna be pointer to by the GPU (.dataBuffer) (UBO)
        materialResources.dataBuffer = ctx.scene->materialDataBuffer;
        materialResources.dataBufferOffset = dataIdx * sizeof(PBRSystem::MaterialPBRConstants);

        fetchPBRTextures(mat, ctx, materialResources, samplers, images);

        glBindBuffer(GL_UNIFORM_BUFFER, ctx.scene->materialDataBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, dataIdx * sizeof(PBRSystem::MaterialPBRConstants), sizeof(PBRSystem::MaterialPBRConstants), &pbrConstants);

        newMat->data = pbrSystem.writeMaterial(passType, materialResources, ctx.engine);
        dataIdx++;
    }
    return materials;
}

// Helper functions for mesh loading

static void loadIndices(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<uint32_t>& indices, size_t initial_vtx) {

    fastgltf::Accessor& indexaccessor = gltf->accessors[p.indicesAccessor.value()];
    indices.reserve(indices.size() + indexaccessor.count);

    fastgltf::iterateAccessor<std::uint32_t>(*gltf, indexaccessor,
        [&](std::uint32_t idx) {
            indices.push_back(idx + initial_vtx);
        });
}

static void loadPositions(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<Vertex>& vertices, size_t initial_vtx) {

    auto* positionIt = p.findAttribute("POSITION");
    auto& posAccessor = gltf->accessors[positionIt->accessorIndex];
    vertices.resize(vertices.size() + posAccessor.count);

    fastgltf::iterateAccessorWithIndex<glm::vec3>(*gltf, posAccessor,
        [&](glm::vec3 v, size_t index) {
            Vertex newVtx;
            newVtx.pos = v;
            newVtx.normal = { 1, 0, 0 };
            //newVtx.color = glm::vec4{ 1.f };
            newVtx.texCoord = glm::vec2(0.0f, 0.0f);
            newVtx.tangent = { 1, 0, 0, 1 };
            vertices[initial_vtx + index] = newVtx;
        });
}

static void loadNormals(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<Vertex>& vertices, size_t initial_vtx) {

    auto normals = p.findAttribute("NORMAL");
    if (normals != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(*gltf, gltf->accessors[(*normals).accessorIndex],
            [&](glm::vec3 v, size_t index) {
                vertices[initial_vtx + index].normal = v;
            });
    }
}

static void loadTexCoords(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<Vertex>& vertices, size_t initial_vtx) {

    auto uv = p.findAttribute("TEXCOORD_0");
    if (uv != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(*gltf, gltf->accessors[(*uv).accessorIndex],
            [&](glm::vec2 v, size_t index) {
                vertices[initial_vtx + index].texCoord.x = v.x;
                vertices[initial_vtx + index].texCoord.y = v.y;
            });
    }
}

static void loadColors(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<Vertex>& vertices, size_t initial_vtx) {

    auto colors = p.findAttribute("COLOR_0");
    if (colors != p.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(*gltf, gltf->accessors[(*colors).accessorIndex],
            [&](glm::vec4 v, size_t index) {
                //vertices[initial_vtx + index].color = v;
            });
    }
}

static SubMesh buildSurface(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<uint32_t>& indices,
    std::vector<Vertex>& vertices,
    std::vector<std::shared_ptr<gltfMaterial>>& materials) {

    SubMesh newSurface;
    newSurface.startIndex = (uint32_t)indices.size();
    newSurface.count = (uint32_t)gltf->accessors[p.indicesAccessor.value()].count;

    size_t initial_vtx = vertices.size();

    loadIndices(p, gltf, indices, initial_vtx);
    loadPositions(p, gltf, vertices, initial_vtx);
    loadNormals(p, gltf, vertices, initial_vtx);
    loadTexCoords(p, gltf, vertices, initial_vtx);
    loadColors(p, gltf, vertices, initial_vtx);

    if (p.materialIndex.has_value()) {
        newSurface.material = materials[p.materialIndex.value()];
    }
    else {
        newSurface.material = materials[0];
    }

    return newSurface;
}

static void loadTangents(const std::vector<uint32_t>& indices, std::vector<Vertex>& vertices) {

    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    positions.reserve(vertices.size());

    for (auto& v : vertices) {

        positions.push_back(v.pos);
        texCoords.push_back(v.texCoord);
        normals.push_back(v.normal);
    }

    std::vector<glm::vec4> tangents = MeshUtils::calculateTangents(positions, indices, texCoords, normals);

    for (int i = 0; i < tangents.size(); i++) {

        vertices[i].tangent = tangents[i];
    }
}

GPUMeshBuffers uploadMesh(const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices) {

    GPUMeshBuffers gpu;

    glGenVertexArrays(1, &gpu.vao);
    glGenBuffers(1, &gpu.vbo);
    glGenBuffers(1, &gpu.ebo);

    glBindVertexArray(gpu.vao);

    // vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(Vertex),
        vertices.data(),
        GL_STATIC_DRAW);

    // index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(uint32_t),
        indices.data(),
        GL_STATIC_DRAW);

    // (pos, normal, uv)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2); 
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glBindVertexArray(0);

    gpu.indexCount = static_cast<GLsizei>(indices.size());
    return gpu;
}


// Main mesh loading function
std::vector<std::shared_ptr<MeshAsset>> gltfData::loadMeshes(
    GltfLoadContext ctx, std::vector<std::shared_ptr<gltfMaterial>> materials) {

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<std::shared_ptr<MeshAsset>> vecMeshes;

    for (fastgltf::Mesh& mesh : ctx.gltf->meshes) {

        std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
        vecMeshes.push_back(newMesh);
        //newMesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto& p : mesh.primitives) {

            SubMesh newSurface = buildSurface(p, ctx.gltf, indices, vertices, materials);
            newMesh->submeshes.push_back(newSurface);
        }
        loadTangents(indices, vertices);

        newMesh->meshBuffers = uploadMesh(indices, vertices);
    }

    return vecMeshes;
}

std::vector<std::shared_ptr<Node>> gltfData::loadNodes(GltfLoadContext ctx, std::vector<std::shared_ptr<MeshAsset>> vecMeshes) {

    // load nodes :D
    std::vector<std::shared_ptr<Node>> nodes;

    size_t sceneIdx = ctx.gltf->defaultScene.value_or(0);

    fastgltf::iterateSceneNodes(*ctx.gltf, sceneIdx, fastgltf::math::fmat4x4(),
        [&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix) {

            if (node.meshIndex.has_value()) {
                std::cout << "iteration node thing first meshIndex: " << *node.meshIndex << std::endl;
            }
            else {
                std::cout << "iteration node thing first meshIndex: none" << std::endl;
            }
            if (!node.meshIndex) {

                auto emptyNode = std::make_shared<Node>();
                nodes.push_back(emptyNode);
                return;
            }
            auto meshCopy = vecMeshes[*node.meshIndex]->clone();
            meshCopy->transform = *reinterpret_cast<const glm::mat4*>(&matrix);
            auto meshNode = std::make_shared<MeshNode>();
            meshNode->mesh = meshCopy;

            nodes.push_back(meshNode);
        });

    // Before the loop, verify vectors line up:
    std::cout
        << "gltf.nodes.size() = " << ctx.gltf->nodes.size()
        << ", sceneNodes.size() = " << nodes.size()
        << std::endl;

    for (size_t i = 0; i < ctx.gltf->nodes.size(); ++i) {
        auto& gltfNode = ctx.gltf->nodes[i];
        auto& sceneNode = nodes[i];

        // Check for a missing sceneNode:
        if (!sceneNode) {

            continue;
        }

        for (auto childIdx : gltfNode.children) {
            // Check that the child index is in bounds
            if (childIdx >= nodes.size()) {

                continue;
            }
            // Check that the target sceneNode exists
            if (!nodes[childIdx]) {

                continue;
            }
            // actual linking
            sceneNode->children.push_back(nodes[childIdx]);
            nodes[childIdx]->parent = sceneNode;
        }
    }
    for (auto& node : nodes) {

        if (node->parent.lock() == nullptr) {

            ctx.scene->topNodes.push_back(node);
        }
    }
    return nodes;
}

RenderObject MeshNode::createRenderObject(const SubMesh& surface) {

    RenderObject obj;
    obj.idxStart = surface.startIndex;
    obj.numIndices = surface.count;
    obj.meshBuffers = mesh->meshBuffers;
    obj.transform = mesh->transform;
    obj.material = surface.material;
    return obj;
}

void MeshNode::Draw(GltfDrawContext& ctx) {

    for (auto& submesh : mesh->submeshes) {

        RenderObject obj = createRenderObject(submesh);
        if (obj.material->data.type == PBRSystem::MaterialPass::Opaque) ctx.opaqueSubmeshes.push_back(obj);
        if (obj.material->data.type == PBRSystem::MaterialPass::Transparent) ctx.transparentSubmeshes.push_back(obj);
        if (obj.material->data.type == PBRSystem::MaterialPass::Transmission) {

            ctx.transmissionSubmeshes.push_back(obj);
            ctx.isTransmissionEnabled = true;
        }
    }
    Node::Draw(ctx);
}

void gltfData::drawNodes(GltfDrawContext& ctx) {

    for (auto& node : topNodes) {

        node->Draw(ctx);
    }
}

GLenum extract_filter(fastgltf::Filter f) {
    switch (f) {
    case fastgltf::Filter::Nearest: return GL_NEAREST;
    case fastgltf::Filter::Linear:  return GL_LINEAR;
    default:                        return GL_LINEAR;
    }
}

GLenum extract_mipmap_filter(fastgltf::Filter f) {
    using F = fastgltf::Filter;
    switch (f) {
    case F::Nearest:               return GL_NEAREST;
    case F::Linear:                return GL_LINEAR;
    case F::NearestMipMapNearest:  return GL_NEAREST_MIPMAP_NEAREST;
    case F::LinearMipMapNearest:   return GL_LINEAR_MIPMAP_NEAREST;
    case F::NearestMipMapLinear:   return GL_NEAREST_MIPMAP_LINEAR;
    case F::LinearMipMapLinear:    return GL_LINEAR_MIPMAP_LINEAR;
    default:                       return GL_LINEAR_MIPMAP_LINEAR;
    }
}

GLenum extract_wrap(fastgltf::Wrap w) {
    switch (w) {
    case fastgltf::Wrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
    case fastgltf::Wrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
    case fastgltf::Wrap::Repeat:         return GL_REPEAT;
    default:                             return GL_REPEAT;
    }
}