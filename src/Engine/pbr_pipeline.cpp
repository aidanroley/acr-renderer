#include "pch.h"
#include "Engine/vk_setup.h"
#include "Engine/pbr_pipeline.h"

// uses descriptor manager to create bindings and layouts.
void PBRMaterialSystem::initDescriptorSetLayouts() {

   std::vector<VkDescriptorSetLayoutBinding> bindings;

   // create layout for material (0 = color I/S, 1 = metal I/S, 2 = UBO material)
   VkDescriptorSetLayoutBinding colorSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
   VkDescriptorSetLayoutBinding metalSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
   VkDescriptorSetLayoutBinding occSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
   VkDescriptorSetLayoutBinding normalSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
   VkDescriptorSetLayoutBinding materialBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 4);

   bindings.push_back(colorSamplerBinding);
   bindings.push_back(metalSamplerBinding);
   bindings.push_back(occSamplerBinding);
   bindings.push_back(normalSamplerBinding);
   bindings.push_back(materialBinding);

   _descriptorManager->createDescriptorLayout(bindings, _descriptorSetLayoutMat);
}

MaterialInstance PBRMaterialSystem::writeMaterial(MaterialPass pass, const PBRMaterialSystem::MaterialResources& resources, VkDevice& device) {

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
        allocInfo.descriptorPool = _descriptorManager->_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &allocInfo, &materialData.materialSet[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to allocate descriptor sets");
        }


        VkDescriptorBufferInfo info = {};
        info.buffer = resources.dataBuffer; // this data buffer points to the PBR constants. the gpu reads directly from it.
        info.offset = resources.dataBufferOffset;
        info.range = sizeof(MaterialPBRConstants);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = UBO_INDEX;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &info;
        write.dstSet = materialData.materialSet[i];
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        std::cout << "success" << std::endl;

        // descriptorManager.clear();
         //materialData.imageSamplerSet = descriptorManager.allocateSet(materialLayout);

        VkDescriptorImageInfo colorImageInfo = {};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = resources.colorImage.imageView;
        colorImageInfo.sampler = resources.colorSampler;

        VkWriteDescriptorSet writeColorImage{};
        writeColorImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeColorImage.dstBinding = MATERIAL_TEX_ALBEDO;
        writeColorImage.dstSet = materialData.materialSet[i];
        writeColorImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeColorImage.descriptorCount = 1;
        writeColorImage.pImageInfo = &colorImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeColorImage, 0, nullptr);

        VkDescriptorImageInfo metalImageInfo = {};
        metalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        metalImageInfo.imageView = resources.metalRoughImage.imageView;
        metalImageInfo.sampler = resources.metalRoughSampler;

        VkWriteDescriptorSet writeMetalImage{};
        writeMetalImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeMetalImage.dstBinding = MATERIAL_TEX_METAL_ROUGH;
        writeMetalImage.dstSet = materialData.materialSet[i];
        writeMetalImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeMetalImage.descriptorCount = 1;
        writeMetalImage.pImageInfo = &metalImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeMetalImage, 0, nullptr);

        VkDescriptorImageInfo occImageInfo = {};
        occImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        occImageInfo.imageView = resources.occImage.imageView;
        occImageInfo.sampler = resources.occSampler;

        VkWriteDescriptorSet writeOccImage{};
        writeOccImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeOccImage.dstBinding = MATERIAL_TEX_OCCULUSION;
        writeOccImage.dstSet = materialData.materialSet[i];
        writeOccImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeOccImage.descriptorCount = 1;
        writeOccImage.pImageInfo = &occImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeOccImage, 0, nullptr);

        VkDescriptorImageInfo normalImageInfo = {};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = resources.normalImage.imageView;
        normalImageInfo.sampler = resources.normalSampler;

        VkWriteDescriptorSet writeNormalImage{};
        writeNormalImage.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeNormalImage.dstBinding = MATERIAL_TEX_NORMAL;
        writeNormalImage.dstSet = materialData.materialSet[i];
        writeNormalImage.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeNormalImage.descriptorCount = 1;
        writeNormalImage.pImageInfo = &normalImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeNormalImage, 0, nullptr);
    }

    return materialData;
}