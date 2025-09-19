#include "pch.h"
#include "vkEng/gltf_loader.h"
#include "vkEng/engine_setup.h"
#include "vkEng/mesh_utils.h"
#include <cmath>
#include "stb_image.h"

std::shared_ptr<gltfData> gltfData::Load(VkEngine* engine, std::filesystem::path path) {

    std::shared_ptr<gltfData> scene = std::make_shared<gltfData>();
    gltfData& file = *scene;
    fastgltf::Asset gltf = file.getGltfAsset(path);
    GltfLoadContext ctx = { scene, &gltf, engine };

    // each main portion of loading a gltf (into the class; no drawing) here
    std::vector<VkSampler> samplers = file.createSamplers(ctx);
    std::vector<AllocatedImage> images = file.createImages(ctx);
    std::vector<std::shared_ptr<gltfMaterial>> materials = file.loadMaterials(ctx, samplers, images);
    std::vector<std::shared_ptr<MeshAsset>> vecMeshes = file.loadMeshes(ctx, materials);
    std::vector<std::shared_ptr<Node>> nodes = file.loadNodes(ctx, vecMeshes);
    std::cout << typeid(materials[0]->data.materialSet).name() << std::endl;
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
    auto asset = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
    if (asset.error() != fastgltf::Error::None) {

        std::cerr << "Failed to load glTF: " << fastgltf::getErrorMessage(asset.error()) << '\n';
    }
    fastgltf::Asset gltf = std::move(asset.get());
    return gltf;
}

std::vector<VkSampler> gltfData::createSamplers(GltfLoadContext ctx) {

    std::vector<VkSampler> samplers;

    /* Sampler creation */
    for (fastgltf::Sampler& sampler : ctx.gltf->samplers) {

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.pNext = nullptr;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.minLod = 0;
        samplerInfo.flags = 0;


        // value_or is used in case sampler.x doesnt exist
        samplerInfo.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        samplerInfo.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        samplerInfo.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(ctx.engine->device, &samplerInfo, nullptr, &newSampler);
        samplers.push_back(newSampler);
    }
    return samplers;
}

std::vector<AllocatedImage> gltfData::createImages(GltfLoadContext ctx) {

    std::vector<AllocatedImage> images;

    // *potential bug make sure the line   materialResources.colorImage = vecImages[img];   the images are in the same order as this enhanced for loop
    /* Load images */
    for (fastgltf::Image& image : ctx.gltf->images) {

        std::optional<AllocatedImage> img = loadImage(ctx.engine, *ctx.gltf, image);

        if (img.has_value()) {

            images.push_back(*img);
        }
        else {

            images.push_back(ctx.engine->_errorImage);
            std::cout << "gltf failed image loading" << std::endl;
        }
    }
    return images;
}

void gltfData::fetchPBRTextures(fastgltf::Material& mat, GltfLoadContext ctx, PBRMaterialSystem::MaterialResources& materialResources, std::vector<VkSampler>& samplers, std::vector<AllocatedImage>& images) {

    auto assignTex = [&](auto& dst, auto& texOpt) {

        if (!texOpt.has_value()) return;

        auto& tex = ctx.gltf->textures[texOpt.value().textureIndex];
        if (!tex.imageIndex.has_value() || !tex.samplerIndex.has_value()) return;

        dst.image = images[tex.imageIndex.value()];
        dst.sampler = samplers[tex.samplerIndex.value()];
        };

    assignTex(materialResources.albedo, mat.pbrData.baseColorTexture);
    assignTex(materialResources.metalRough, mat.pbrData.metallicRoughnessTexture);
    assignTex(materialResources.occlusion, mat.occlusionTexture);
    assignTex(materialResources.normalMap, mat.normalTexture);

    if (mat.transmission) assignTex(materialResources.transmission, mat.transmission->transmissionTexture);
    if (mat.volume) assignTex(materialResources.volumeThickness, mat.volume->thicknessTexture);
}

static PBRMaterialSystem::MaterialPBRConstants populatePBRConstants(fastgltf::Material& mat, MaterialPass* passType) {

    PBRMaterialSystem::MaterialPBRConstants pbrConstants;
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
    if (pbrConstants.transmission.x || pbrConstants.volume.x) *passType = MaterialPass::Transmission;
    return pbrConstants;
}

std::vector<std::shared_ptr<gltfMaterial>> gltfData::loadMaterials(GltfLoadContext ctx, std::vector<VkSampler>& samplers, std::vector<AllocatedImage>& images) {

    /* Load materials */

    // putting this in scene variable so it doesnt die out of scope...
    ctx.scene->materialDataBuffer = createBufferVMA(sizeof(PBRMaterialSystem::MaterialPBRConstants) * ctx.gltf->materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, ctx.engine->_allocator);
    int dataIdx = 0;

    // store the mapped pointer *if bugs arise check this line***
    PBRMaterialSystem::MaterialPBRConstants* sceneMaterialConstants = reinterpret_cast<PBRMaterialSystem::MaterialPBRConstants*>(ctx.scene->materialDataBuffer.info.pMappedData); // info is of VmaAllocationInfo, engine_setup.h has this AllocatedBuffer struct

    // Load material from gltf
    std::vector<std::shared_ptr<gltfMaterial>> materials;
    for (fastgltf::Material& mat : ctx.gltf->materials) {

        std::shared_ptr<gltfMaterial> newMat = std::make_shared<gltfMaterial>();
        materials.push_back(newMat);

        // set pass type to opaque by default
        MaterialPass passType = MaterialPass::Opaque;

        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {

            passType = MaterialPass::Transparent;
        }

        // store pbr data
        // pbr can change whether it needs transparent pipeline (volume/transmission)
        PBRMaterialSystem::MaterialPBRConstants pbrConstants = populatePBRConstants(mat, &passType);

        
        
        if (passType == MaterialPass::Opaque) std::cout << "YAY!!!!" << std::endl;
        if (passType == MaterialPass::Transparent) std::cout << "YAY 2222!!!!" << std::endl;
        if (passType == MaterialPass::Transmission) std::cout << "YAY 33333!!!!" << std::endl;
        std::cout << mat.name << std::endl;

        PBRMaterialSystem::MaterialResources materialResources;

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
        materialResources.dataBuffer = ctx.scene->materialDataBuffer.buffer;
        materialResources.dataBufferOffset = dataIdx * sizeof(PBRMaterialSystem::MaterialPBRConstants);

        

        fetchPBRTextures(mat, ctx, materialResources, samplers, images);

        sceneMaterialConstants[dataIdx] = pbrConstants;

        newMat->data = ctx.engine->pbrSystem.writeMaterial(passType, materialResources, ctx.engine);
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
            newVtx.color = glm::vec4{ 1.f };
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
                vertices[initial_vtx + index].color = v;
            });
    }
}

static GeoSurface buildSurface(fastgltf::Primitive& p, fastgltf::Asset* gltf, std::vector<uint32_t>& indices,
    std::vector<Vertex>& vertices,
    std::vector<std::shared_ptr<gltfMaterial>>& materials) {

    GeoSurface newSurface;
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

static void loadTangents(const std::vector<uint32_t> &indices, std::vector<Vertex> &vertices) {

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

// Main mesh loading function

std::vector<std::shared_ptr<MeshAsset>> gltfData::loadMeshes(
    GltfLoadContext ctx, std::vector<std::shared_ptr<gltfMaterial>> materials) {

    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<std::shared_ptr<MeshAsset>> vecMeshes;

    for (fastgltf::Mesh& mesh : ctx.gltf->meshes) {

        std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
        vecMeshes.push_back(newMesh);
        newMesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto& p : mesh.primitives) {

            GeoSurface newSurface = buildSurface(p, ctx.gltf, indices, vertices, materials);
            newMesh->surfaces.push_back(newSurface);
        }
        loadTangents(indices, vertices);
        
        std::cout << "mesh name thing idk" << newMesh->name << std::endl;
        newMesh->meshBuffers = ctx.engine->uploadMesh(indices, vertices);
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

void gltfData::drawNodes(DrawContext& ctx) {

    for (auto& node : topNodes) {

        node->Draw(ctx);
    }
}
std::optional<AllocatedImage> loadImage(VkEngine* engine,
    fastgltf::Asset& asset,
    fastgltf::Image& image)
{
    AllocatedImage newImage{};
    int width, height, nrChannels;

    // handle external URI
    if (std::holds_alternative<fastgltf::sources::URI>(image.data)) {
        auto& filePath = std::get<fastgltf::sources::URI>(image.data);
        assert(filePath.fileByteOffset == 0);
        assert(filePath.uri.isLocalPath());

        const std::string path(filePath.uri.path().begin(),
            filePath.uri.path().end());
        unsigned char* data = stbi_load(path.c_str(),
            &width, &height, &nrChannels, 4);
        if (data) {
            VkExtent3D imagesize{ uint32_t(width),
                                  uint32_t(height),
                                  1u };
            newImage = engine->createImage(data,
                imagesize,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                false);
            stbi_image_free(data);
        }

        // handle embedded vector
    }
    else if (std::holds_alternative<fastgltf::sources::Vector>(image.data)) {
        auto& vector = std::get<fastgltf::sources::Vector>(image.data);
        const stbi_uc* rawBytes =
            reinterpret_cast<const stbi_uc*>(vector.bytes.data());
        unsigned char* data = stbi_load_from_memory(rawBytes,
            int(vector.bytes.size()),
            &width, &height, &nrChannels, 4);
        if (data) {
            VkExtent3D imagesize{ uint32_t(width),
                                  uint32_t(height),
                                  1u };
            newImage = engine->createImage(data,
                imagesize,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT,
                false);
            stbi_image_free(data);
        }

        // handle binary-GLB bufferView
    }
    else if (std::holds_alternative<fastgltf::sources::BufferView>(image.data)) {
        auto& view = std::get<fastgltf::sources::BufferView>(image.data);
        auto& bufView = asset.bufferViews[view.bufferViewIndex];
        auto& buffer = asset.buffers[bufView.bufferIndex];

        // debug: enumerate all possible buffer.data types
        if (std::holds_alternative<fastgltf::sources::URI>(buffer.data)) {
            std::cout << "buffer.data is URI\n";
        }
        else if (std::holds_alternative<fastgltf::sources::Vector>(buffer.data)) {
            std::cout << "buffer.data is Vector\n";
        }
        else if (std::holds_alternative<fastgltf::sources::BufferView>(buffer.data)) {
            std::cout << "buffer.data is BufferView\n";
        }
        else if (std::holds_alternative<fastgltf::sources::Array>(buffer.data)) {
            std::cout << "buffer.data is Array\n";
        }
        else {
            std::cout << "buffer.data is UNKNOWN TYPE (index="
                << buffer.data.index() << ")\n";
        }


        if (std::holds_alternative<fastgltf::sources::Vector>(buffer.data)) {
            auto& vector2 = std::get<fastgltf::sources::Vector>(buffer.data);
            const stbi_uc* rawBytes =
                reinterpret_cast<const stbi_uc*>(vector2.bytes.data());
            unsigned char* data = stbi_load_from_memory(
                rawBytes + bufView.byteOffset,
                int(bufView.byteLength),
                &width, &height, &nrChannels, 4);
            if (data) {
                VkExtent3D imagesize{ uint32_t(width),
                                      uint32_t(height),
                                      1u };
                newImage = engine->createImage(data,
                    imagesize,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                    false);
                stbi_image_free(data);
            }
        }
        else if (std::holds_alternative<fastgltf::sources::Array>(buffer.data)) {
            auto& array2 = std::get<fastgltf::sources::Array>(buffer.data);
            const stbi_uc* rawBytes =
                reinterpret_cast<const stbi_uc*>(array2.bytes.data());
            unsigned char* data = stbi_load_from_memory(
                rawBytes + bufView.byteOffset,
                int(bufView.byteLength),
                &width, &height, &nrChannels, 4);
            if (data) {
                VkExtent3D imagesize{ uint32_t(width),
                                      uint32_t(height),
                                      1u };
                newImage = engine->createImage(data,
                    imagesize,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_USAGE_SAMPLED_BIT,
                    false);
                stbi_image_free(data);
            }
        }
    }

    // if nothing loaded, return empty
    if (newImage.image == VK_NULL_HANDLE) {
        return {};
    }
    else {
        return newImage;
    }
}
VkFilter extract_filter(fastgltf::Filter filter) {

    switch (filter) {

    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear:
        return VK_FILTER_NEAREST;

    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:

    default:
        return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter) {

    switch (filter) {

    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:

    default:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}