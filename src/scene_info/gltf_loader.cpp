#pragma once
#include "../../precompile/pch.h"
#include "../../include/scene_info/gltf_loader.h"
#include "../../include/vk_setup.h"



#include "stb_image.h"
#include <cmath>

// This file will be a general GLTF loader once I get to the point where I don't have to hardcode anything for sun temple

std::shared_ptr<gltfData> gltfData::loadGltf(VkEngine* engine, std::filesystem::path path) {

    path = "SunTempleGLTF/SunTemple.gltf";
    /* Load file */
    std::shared_ptr<gltfData> scene = std::make_shared<gltfData>();
    gltfData& file = *scene;
    fastgltf::Parser parser {};

    constexpr auto gltfOptions = 
          fastgltf::Options::DontRequireValidAssetMember 
        | fastgltf::Options::AllowDouble // allows double floating point nums instead of float
        //| fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers;

    auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
    if (!bool(gltfFile)) {

        std::cerr << "Failed to open glTF file: " << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
        //return false;
    }
    auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
    if (asset.error() != fastgltf::Error::None) {

        std::cerr << "Failed to load glTF: " << fastgltf::getErrorMessage(asset.error()) << '\n';
        //return false;
    }


    fastgltf::Asset gltf =  std::move(asset.get());

    //std::filesystem::path path = filePath;

    //fastgltf::GltfType type = fastgltf::determineGltfFileType(data);
    /*
    switch (type) {

    case fastgltf::GltfType::glTF: {

        auto loaded = parser.loadGltf(data, path.parent_path(), gltfOptions);
        if (loaded) {

            gltf = std::move(loaded.get());
        }
        else {

            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(loaded.error()) << std::endl;
            return {};
        }
        break;
    }

    case fastgltf::GltfType::GLB: {

        auto loaded = parser.loadGltfBinary(data, path.parent_path(), gltfOptions);
        if (loaded) {

            gltf = std::move(loaded.get());
        }
        else {

            std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(loaded.error()) << std::endl;
            return {};
        }
        break;
    }

        default: std::cerr << "Failed to determine glTF container" << std::endl; break;

    }
    */
   

    // figure out desceriptor stuff here

    /* Sampler creation */
    for (fastgltf::Sampler& sampler : gltf.samplers) {

        VkSamplerCreateInfo samplerInfo;
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.pNext = nullptr;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.minLod = 0;

        // value_or is used in case sampler.x doesnt exist
        samplerInfo.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
        samplerInfo.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        samplerInfo.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(passed.device, &samplerInfo, nullptr, &newSampler);
        samplers.push_back(newSampler);
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

        /*
        constants.colorTexID = engine->texCache.AddTexture(materialResources.colorImage.imageView, materialResources.colorSampler).Index;
		constants.metalRoughTexID = engine->texCache.AddTexture(materialResources.metalRoughImage.imageView, materialResources.metalRoughSampler).Index;
      
		// write material parameters to buffer
		sceneMaterialConstants[data_index] = constants;
        // build material
        newMat->data = engine->metalRoughMaterial.write_material(engine->_device, passType, materialResources, file.descriptorPool);
        */

        constants.colorTexID = engine->texStorage.addTexture(materialResources.colorImage.imageView, materialResources.colorSampler);
        constants.metalRoughTexID = engine->texStorage.addTexture(materialResources.metalRoughImage.imageView, materialResources.metalRoughSampler);

        sceneMaterialConstants[dataIdx] = constants;
        //newMat->data = engine->writeMaterial(passType, materialResources);
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

    for (fastgltf::Node& node : gltf.nodes) {

        std::shared_ptr<Node> newNode;

        if (node.meshIndex.has_value()) {

            newNode = std::make_shared<Node>();
            static_cast<Node*>(newNode.get())->mesh = vecMeshes[*node.meshIndex];
        }
        else {

            newNode = std::make_shared<Node>();
        }
        nodes.push_back(newNode);
        file.nodes[node.name.c_str()] = newNode; // check this lol

        // do transforms here after get it working.

        //
        for (int i = 0; i < gltf.nodes.size(); i++) {

            fastgltf::Node& node = gltf.nodes[i];
            std::shared_ptr<Node>& sceneNode = nodes[i];

            for (auto& c : node.children) {

                sceneNode->children.push_back(nodes[c]);
                nodes[c]->parent = sceneNode;
            }
        }

        for (auto& node : nodes) {

            if (node->parent.lock() == nullptr) {

                file.topNodes.push_back(node);
                // refresh transform here
            }
        }
        return scene;
    }
}

std::optional<AllocatedImage> loadImage(VkEngine* engine, fastgltf::Asset& asset, fastgltf::Image& image) {

    AllocatedImage newImage{};
    int width, height, numChannels;

    std::visit(

        fastgltf::visitor{

            [](auto& arg) {},
            // textures stored outside the gl.. file
            [&](fastgltf::sources::URI& filePath) {

                assert(filePath.fileByteOffset == 0);
                assert(filePath.uri.isLocalPath());

                const std::string path(filePath.uri.path().begin(), filePath.uri.path().end()); // lol
                unsigned char* data = stbi_load(path.c_str(), &width, &height, &numChannels, 4);
                if (data) {

                    VkExtent3D imageSize;
                    imageSize.width = width;
                    imageSize.height = height;
                    imageSize.depth = 1;

                    newImage = engine->createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
                    stbi_image_free(data);
                }
            },
        // for when fastgltf loads texture into vector
        [&](fastgltf::sources::Vector& vector) {

            unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()), static_cast<int>(vector.bytes.size()),
            &width, &height, &numChannels, 4);
            if (data) {

                VkExtent3D imageSize;
                imageSize.width = width;
                imageSize.height = height;
                imageSize.depth = 1;

                newImage = engine->createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
                stbi_image_free(data);
            }
        },
        // for binary GLB file
        [&](fastgltf::sources::BufferView& view) {

            auto& bufferView = asset.bufferViews[view.bufferViewIndex];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            std::visit(fastgltf::visitor{

                [](auto& arg) {},
                [&](fastgltf::sources::Vector& vector) {

                    unsigned char* data = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(vector.bytes.data()) + bufferView.byteOffset,
                        static_cast<int>(bufferView.byteLength),
                        &width, &height, &numChannels, 4);
                    if (data) {

                        VkExtent3D imageSize;
                        imageSize.width = width;
                        imageSize.height = height;
                        imageSize.depth = 1;

                        newImage = engine->createImage(data, imageSize, VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_USAGE_SAMPLED_BIT, false);
                        stbi_image_free(data);
                    }
                } },
                buffer.data);
        },
        },
        image.data);

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

