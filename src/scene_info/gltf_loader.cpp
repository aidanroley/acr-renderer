#pragma once
#include "../../precompile/pch.h"
#include "../../include/scene_info/gltf_loader.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>
#include "../include/vk_descriptor.h"

#include "stb_image.h"

#include <cmath>

// This file will be a general GLTF loader once I get to the point where I don't have to hardcode anything for sun temple

std::shared_ptr<gltfData> gltfData::loadGltf(VkEngine* engine, std::string_view filePath) {

    /* Load file */
    std::shared_ptr<gltfData> scene = std::make_shared<gltfData>();
    gltfData& file = *scene;
    fastgltf::Parser parser {};

    constexpr auto gltfOptions = 
          fastgltf::Options::DontRequireValidAssetMember 
        | fastgltf::Options::AllowDouble // allows double floating point nums instead of float
        | fastgltf::Options::LoadGLBBuffers
        | fastgltf::Options::LoadExternalBuffers;

    fastgltf::GltfDataBuffer data;
    data.FromPath(filePath);

    fastgltf::Asset gltf;
    std::filesystem::path path = filePath;

    fastgltf::GltfType type = fastgltf::determineGltfFileType(data);

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
        vkCreateSampler(engine->device, &samplerInfo, nullptr, &newSampler);
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
        newMat = engine->
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

/*
const std::string SUN_TEMPLE_MODEL_PATH = "assets/SunTemple/SunTemple.glb";

void loadModel_SunTemple(VertexData& vertexData) {

    auto parser = fastgltf::Parser();
    auto data = fastgltf::GltfDataBuffer::FromPath(SUN_TEMPLE_MODEL_PATH);
    if (data.error() != fastgltf::Error::None) {

        std::cerr << "file load after fromPath failed ;_;";
    }

    // It's worth noting asset's type becomes fastgltf::Expected<class fastgltf::Asset> and is used in similar fashion to a smart pointer
    auto asset = parser.loadGltfBinary(data.get(), "assets/SunTemple", fastgltf::Options::None);

    if (asset.error() != fastgltf::Error::None) {

        std::ostringstream errorMessage;
        errorMessage << "Error: Failed to load glTF: " << fastgltf::getErrorMessage(asset.error());
        throw std::invalid_argument(errorMessage.str());
    }

    auto validationErrors = fastgltf::validate(asset.get());
    if (validationErrors != fastgltf::Error::None) {

        std::cout << "Validation Errors Found:" << std::endl;
    }
    else {

        std::cout << "The asset is valid!" << std::endl;
    }

        loadMeshData(asset.get(), vertexData);


        // SUN_TEMPLE SPECIFIC TRANSFORMS
        glm::mat3 flipMatrix = glm::mat3(1.0f);
        flipMatrix[1][1] = -1.0f;
        
        // Define the 90-degree rotation matrix for X-axis
        glm::mat3 rotationMatrix = glm::mat3(1.0f); 
        rotationMatrix[1][1] = 0.0f;  
        rotationMatrix[1][2] = -1.0f; 
        rotationMatrix[2][1] = 1.0f;  
        rotationMatrix[2][2] = 0.0f; 

        
        for (auto& vertex : vertexData.vertices) {
            vertex.pos = flipMatrix * rotationMatrix * vertex.pos;
        }

        // Next, remove duplicates from vertex buffer and change index buffer accordingly
        optimizeVertexBuffer(vertexData);
}

// This function uses a map to make sure every vertex in the buffer is unique while updating the index buffer accordingly
void optimizeVertexBuffer(VertexData& vertexData) {

    std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices;
    std::vector<Vertex> optimizedVertexBuffer;
    std::vector<uint32_t> updatedIndexBuffer;

    for (uint32_t index : vertexData.indices) {

        const Vertex& vertex = vertexData.vertices[index];
        if (uniqueVertices.find(vertex) == uniqueVertices.end()) {

            uniqueVertices[vertex] = static_cast<uint32_t>(optimizedVertexBuffer.size());
            optimizedVertexBuffer.push_back(vertex);
        }
        updatedIndexBuffer.push_back(uniqueVertices[vertex]);
    }
    vertexData.vertices = std::move(optimizedVertexBuffer);
    vertexData.indices = std::move(updatedIndexBuffer);

    // normalize texcoords
    for (auto& vertex : vertexData.vertices) {

        vertex.texCoord = glm::mod(vertex.texCoord + 1.0f, 1.0f);
    }

}

void loadMeshData(const fastgltf::Asset& asset, VertexData& vertexData) {

    const std::vector<fastgltf::Mesh>& meshes = asset.meshes;

    std::cout << "loading " + std::to_string(meshes.size()) + " meshes";
    for (std::size_t meshIdx = 0; meshIdx < meshes.size(); meshIdx++) {

        for (const fastgltf::Primitive& primitive : meshes[meshIdx].primitives) {

            if (!primitive.indicesAccessor.has_value()) {

                throw std::invalid_argument("error: gltf file needs indexed geometry");
            }
            loadMeshIndices(asset, primitive, vertexData);
            loadMeshVertices(asset, &primitive, vertexData);
        }
    }
}

void loadMeshVertices(const fastgltf::Asset& asset, const fastgltf::Primitive* primitive, VertexData& vertexData) {

    // fetch matIdx if it exists
    std::size_t materialIdx = -1;
    if (primitive->materialIndex.has_value()) materialIdx = primitive->materialIndex.value();

    // Puts the primitive data into this vector, then pushes each el in this vector into the main one once everything is done
    std::vector<Vertex> tempVertices{};

    auto* positionIterator = primitive->findAttribute("POSITION");
    if (!positionIterator) {

        throw std::invalid_argument("POSITION attribute not found.");
    }
    auto& posAccessor = asset.accessors[positionIterator->accessorIndex];

    constexpr float scaleFactor = 0.0006f;
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
        asset, posAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
            
            Vertex vert{};
            vert.pos = glm::vec3(pos.x(), pos.y(), pos.z()) * scaleFactor;
            tempVertices.push_back(vert);
        });
    
    auto* texIterator = primitive->findAttribute("TEXCOORD_0");

    if (!texIterator) {

        throw std::invalid_argument("COLOR attribute not found.");
    }
    auto& colorAccessor = asset.accessors[texIterator->accessorIndex];
    
    // get TEXCOORD data
    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
        asset, colorAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {

            glm::vec2 vertexTexCoord = glm::vec2(uv.x(), uv.y());
            tempVertices[idx].texCoord = vertexTexCoord;
        });

    for (Vertex& v : tempVertices) {

        if (materialIdx) {

            v.texIndex = materialIdx;
        }
        vertexData.vertices.push_back(v);
    }
}

void loadMeshIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, VertexData& vertexData) {

    // Indices must be offset by the size of vertex buffer at the beginning of this function since the indices in the
    // GLTF file are relative to the primitive, not all vertices
    int startingIdx = vertexData.vertices.size();

    std::vector<std::uint32_t> indices;
    if (primitive.indicesAccessor.has_value()) {

        auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
        indices.resize(accessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(asset, accessor, [&](std::uint32_t index) {

            vertexData.indices.push_back(index + startingIdx);
        });
    }

}

// Helper funcs for vertices
bool areVec3Equal(const glm::vec3& a, const glm::vec3& b, float epsilon) {

    return std::abs(a.x - b.x) < epsilon &&
        std::abs(a.y - b.y) < epsilon &&
        std::abs(a.z - b.z) < epsilon;
}
*/

