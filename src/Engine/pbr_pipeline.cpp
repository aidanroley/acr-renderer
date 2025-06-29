#include "pch.h"
#include "Engine/vk_setup.h"
#include "Engine/pbr_pipeline.h"

// uses descriptor manager to create bindings and layouts.
void PBRMaterialSystem::initDescriptorSetLayouts() {

   std::vector<VkDescriptorSetLayoutBinding> bindings;
   /*
   // create layout for camera
   VkDescriptorSetLayoutBinding cameraBinding = _descriptorManager->createLayoutBinding(
       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
   bindings.push_back(cameraBinding);
   _descriptorManager->createDescriptorLayout(bindings, _descriptorSetLayoutCamera);
   bindings.clear();
   */

   // create layout for material
   VkDescriptorSetLayoutBinding samplerBinding = _descriptorManager->createLayoutBinding(
       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
   VkDescriptorSetLayoutBinding materialBinding = _descriptorManager->createLayoutBinding(
       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
   bindings.push_back(samplerBinding);
   bindings.push_back(materialBinding);
   _descriptorManager->createDescriptorLayout(bindings, _descriptorSetLayoutMat);
}

MaterialInstance PBRMaterialSystem::writeMaterial(MaterialPass pass, const PBRMaterialSystem::MaterialResources& resources, DescriptorManager& descriptorManager, VkDevice& device) {

    MaterialInstance materialData;
    MaterialPipeline matPipeline;

    if (pass == MaterialPass::Transparent) {

        materialData.pipeline = &matPipeline; // make this trnapsnare/opaque later
    }

    materialData.materialSet.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        // allocate for the 2 descriptor sets for double buffering
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayoutMat);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorManager._descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &allocInfo, &materialData.materialSet[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to allocate descriptor sets");
        }


        VkDescriptorBufferInfo info = {};
        info.buffer = resources.dataBuffer;
        info.offset = resources.dataBufferOffset;
        info.range = sizeof(MaterialConstants);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = 1;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &info;
        write.dstSet = materialData.materialSet[i];
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        std::cout << "success" << std::endl;

        // descriptorManager.clear();
         //materialData.imageSamplerSet = descriptorManager.allocateSet(materialLayout);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = resources.colorImage.imageView;
        imageInfo.sampler = resources.colorSampler;

        VkWriteDescriptorSet writeImage{};
        writeImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeImage.dstBinding = 0;
        writeImage.dstSet = materialData.materialSet[i];
        writeImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeImage.descriptorCount = 1;
        writeImage.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(device, 1, &writeImage, 0, nullptr);
    }

    return materialData;
}