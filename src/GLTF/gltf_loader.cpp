#include "pch.h"
#include "GLTF/gltf_loader.h"
#include "Engine/vk_setup.h"


#include <cmath>
#include "stb_image.h"
// This file will be a general GLTF loader once I get to the point where I don't have to hardcode anything for sun temple

std::shared_ptr<gltfData> gltfData::loadGltf(VkEngine* engine, std::filesystem::path path) {

    //path = "assets/SunTemple/SunTemple.glb";
    path = "assets/Chess.glb";
    //path = "avocado/Avocado.gltf";
    /* Load file */
    std::shared_ptr<gltfData> scene = std::make_shared<gltfData>();
    gltfData& file = *scene;
    fastgltf::Parser parser {};

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember
        | fastgltf::Options::AllowDouble; // allows double floating point nums instead of float
        //| fastgltf::Options::LoadExternalBuffers;
/*
    auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
    if (!bool(gltfFile)) {

        std::cerr << "Failed to open glTF file: " << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
        //return false;
    }
    */

    auto data = fastgltf::GltfDataBuffer::FromPath(path);
    auto asset = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
    if (asset.error() != fastgltf::Error::None) {

        std::cerr << "Failed to load glTF: " << fastgltf::getErrorMessage(asset.error()) << '\n';
        //return false;
    }


    fastgltf::Asset gltf = std::move(asset.get());

    
    // figure out desceriptor stuff here

    /* Sampler creation */
    for (fastgltf::Sampler& sampler : gltf.samplers) {

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
        vkCreateSampler(engine->device, &samplerInfo, nullptr, &newSampler);
        file.samplers.push_back(newSampler);
    }

    std::vector<AllocatedImage> vecImages;

    // *potential bug make sure the line   materialResources.colorImage = vecImages[img];   the images are in the same order as this enhanced for loop
    /* Load images */
    for (fastgltf::Image& image : gltf.images) {

        std::optional<AllocatedImage> img = loadImage(engine, gltf, image);

        if (img.has_value()) {

            vecImages.push_back(*img);
            images[image.name.c_str()] = *img;
        }
        else {

            vecImages.push_back(engine->_errorImage);
            std::cout << "gltf failed image loading" << std::endl;
        }
    }

    /* Load materials */

    // Material data buffer loading 
    materialDataBuffer = createBufferVMA(sizeof(GLTFMetallicRoughness::MaterialConstants) * gltf.materials.size(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, engine->_allocator);
    int dataIdx = 0;
    // store the mapped pointer *if bugs arise check this line***
    GLTFMetallicRoughness::MaterialConstants* sceneMaterialConstants = reinterpret_cast<GLTFMetallicRoughness::MaterialConstants*>(materialDataBuffer.info.pMappedData); // info is of VmaAllocationInfo, vk_setup.h has this AllocatedBuffer struct

    // Load material from gltf
    std::vector<std::shared_ptr<gltfMaterial>> vecMaterials;
    for (fastgltf::Material& mat : gltf.materials) {

        std::shared_ptr<gltfMaterial> newMat = std::make_shared<gltfMaterial>();
        vecMaterials.push_back(newMat);
        materials[mat.name.c_str()] = newMat;

        GLTFMetallicRoughness::MaterialConstants constants;
        constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
        constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
        constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
        constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

        constants.metalRoughFactors.x = mat.pbrData.metallicFactor;
        constants.metalRoughFactors.y = mat.pbrData.roughnessFactor;

        sceneMaterialConstants[dataIdx] = constants;

        MaterialPass passType = MaterialPass::MainColor;
        if (mat.alphaMode == fastgltf::AlphaMode::Blend) {

            passType = MaterialPass::Transparent;
        }

        GLTFMetallicRoughness::MaterialResources materialResources;
        // defaults in case none available
        materialResources.colorImage = engine->_whiteImage;
        materialResources.colorSampler = engine->_defaultSamplerLinear;
        materialResources.metalRoughImage = engine->_whiteImage;
        materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

        materialResources.dataBuffer = materialDataBuffer.buffer;
        materialResources.dataBufferOffset = dataIdx * sizeof(GLTFMetallicRoughness::MaterialConstants);

        if (mat.pbrData.baseColorTexture.has_value()) {

            size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
            size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

            materialResources.colorImage = vecImages[img];
            materialResources.colorSampler = file.samplers[sampler];
        }
        
        std::cout << "ImageView: " << (uint64_t)(materialResources.colorImage.imageView)
            << ", Sampler: " << (uint64_t)(materialResources.colorSampler) << std::endl;
        constants.colorTexID = engine->texStorage.addTexture(materialResources.colorImage.imageView, materialResources.colorSampler);
        std::cout << "yte" << constants.colorTexID << std::endl;
        constants.metalRoughTexID = engine->texStorage.addTexture(materialResources.metalRoughImage.imageView, materialResources.metalRoughSampler);
        std::cout << "yt2222e" << constants.metalRoughTexID << std::endl;

        sceneMaterialConstants[dataIdx] = constants;
        
        newMat->data = engine->metalRoughMaterial.writeMaterial(passType, materialResources, engine->descriptorManager, engine->device);
        dataIdx++;
    }

    // load meshes
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    std::vector<std::shared_ptr<MeshAsset>> vecMeshes;

    for (fastgltf::Mesh& mesh : gltf.meshes) {

        std::shared_ptr<MeshAsset> newMesh = std::make_shared<MeshAsset>();
        vecMeshes.push_back(newMesh);
        meshes[mesh.name.c_str()] = newMesh;
        newMesh->name = mesh.name;

        indices.clear();
        vertices.clear();

        for (auto& p : mesh.primitives) {

            GeoSurface newSurface;
            newSurface.startIndex = (uint32_t)indices.size();
            newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

            size_t initial_vtx = vertices.size();

            // indices
            {
                fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
                indices.reserve(indices.size() + indexaccessor.count);

                fastgltf::iterateAccessor<std::uint32_t>(gltf, indexaccessor,
                    [&](std::uint32_t idx) {

                        indices.push_back(idx + initial_vtx); // this is called for each index in the primitive
                    });
            }

            // vertex positions
            {
                auto* positionIt = p.findAttribute("POSITION");
                auto& posAccessor = gltf.accessors[positionIt->accessorIndex]; // check make sure this is ok when debugging
                vertices.resize(vertices.size() + posAccessor.count);

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor,
                    [&](glm::vec3 v, size_t index) {

                        Vertex newVtx;
                        newVtx.pos = v;
                        newVtx.normal = { 1, 0, 0 };
                        newVtx.color = glm::vec4{ 1.f };
                        newVtx.texCoord = glm::vec2(0.0f, 0.0f);
                        vertices[initial_vtx + index] = newVtx;
                    });
            }

            // normals
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
                    [&](glm::vec3 v, size_t index) {

                        vertices[initial_vtx + index].normal = v;
                    });
            }

            // load texcoord/uv
            auto uv = p.findAttribute("TEXCOORD_0");
            if (uv != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex],
                    [&](glm::vec2 v, size_t index) {

                        vertices[initial_vtx + index].texCoord.x = v.x;
                        vertices[initial_vtx + index].texCoord.y = v.y;
                    });
            }

            // load colors
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end()) {

                fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex],
                    [&](glm::vec4 v, size_t index) {

                        vertices[initial_vtx + index].color = v;
                    });
            }

            if (p.materialIndex.has_value()) {

                newSurface.material = vecMaterials[p.materialIndex.value()];
            }
            else {

                newSurface.material = materials[0];
            }

            newMesh->surfaces.push_back(newSurface);
        }

        newMesh->meshBuffers = engine->uploadMesh(indices, vertices);
    }

    // load nodes :D
    std::vector<std::shared_ptr<Node>> nodes;

        size_t sceneIdx = gltf.defaultScene.value_or(0);
        fastgltf::iterateSceneNodes(gltf, sceneIdx, fastgltf::math::fmat4x4(),
            [&](fastgltf::Node& node, fastgltf::math::fmat4x4 matrix) {
                if (node.meshIndex.has_value()) {

                    std::shared_ptr<Node> newNode;

                    if (node.meshIndex.has_value()) {

                        newNode = std::make_shared<MeshNode>();
                        static_cast<MeshNode*>(newNode.get())->mesh = vecMeshes[*node.meshIndex];
                    }
                    else {

                        newNode = std::make_shared<Node>();
                    }
                    
                    file.nodes[node.name.c_str()] = newNode;

                    static_cast<MeshNode*>(newNode.get())->mesh->transform = *reinterpret_cast<const glm::mat4*>(&matrix); // ?
                    nodes.push_back(newNode);
                }
            });

    // graph loading
    for (int i = 0; i < gltf.nodes.size()  ; i++) { // IDK ABOUT THE -1 MAN

        fastgltf::Node& node = gltf.nodes[i];
        std::shared_ptr<Node>& sceneNode = nodes[i];

        for (auto& c : node.children) {

            if (c < nodes.size()) {
                sceneNode->children.push_back(nodes[c]);
                nodes[c]->parent = sceneNode;
            }
            else {
                std::cerr << "Invalid child index " << c << "\n";
            }
        }
    }

    for (auto& node : nodes) {

        if (node->parent.lock() == nullptr) {

            topNodes.push_back(node);
            // refresh transform here
        }
    }
    return scene;


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
VkFilter gltfData::extract_filter(fastgltf::Filter filter) {

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

VkSamplerMipmapMode gltfData::extract_mipmap_mode(fastgltf::Filter filter) {

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

void gltfData::setDevice(VkDevice device, VkSampler dsl, AllocatedImage wi) {

    passed.device = device;
    passed.whiteImage = wi;
    passed.defaultSamplerLinear = dsl;
}

