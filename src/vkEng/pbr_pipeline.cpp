#include "pch.h"
#include "vkEng/vk_engine_setup.h"
#include "vkEng/pbr_pipeline.h"

// uses descriptor manager to create bindings and layouts.
void PBRMaterialSystem::initDescriptorSetLayouts() {

   std::vector<VkDescriptorSetLayoutBinding> bindings;

   // create layout for material (0 = color I/S, 1 = metal I/S, 2 = UBO material)
   VkDescriptorSetLayoutBinding colorSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
   VkDescriptorSetLayoutBinding metalSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1);
   VkDescriptorSetLayoutBinding occSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2);
   VkDescriptorSetLayoutBinding normalSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3);
   VkDescriptorSetLayoutBinding transmissionSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4);
   VkDescriptorSetLayoutBinding thicknessSamplerBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5);
   VkDescriptorSetLayoutBinding materialBinding = _descriptorManager->createLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 6);
   

   bindings.push_back(colorSamplerBinding);
   bindings.push_back(metalSamplerBinding);
   bindings.push_back(occSamplerBinding);
   bindings.push_back(normalSamplerBinding);
   bindings.push_back(transmissionSamplerBinding);
   bindings.push_back(thicknessSamplerBinding);
   bindings.push_back(materialBinding);

   _descriptorManager->createDescriptorLayout(bindings, _descriptorSetLayoutMat);
}

VkWriteDescriptorSet PBRMaterialSystem::makeImageWrite(VkDescriptorSet dstSet, uint32_t dstBinding, const TextureBinding& tb, VkDescriptorImageInfo& outInfo) {

    outInfo = {};
    outInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    outInfo.imageView = tb.image.imageView;
    outInfo.sampler = tb.sampler;

    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = dstSet;
    w.dstBinding = dstBinding;
    w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    w.descriptorCount = 1;
    w.pImageInfo = &outInfo;
    return w;
}

MaterialInstance PBRMaterialSystem::writeMaterial(MaterialPass pass, const PBRMaterialSystem::MaterialResources& resources, VkEngine* engine) {

    MaterialInstance materialData;
    materialData.type = pass;

    materialData.matPipeline = getPipeline(pass, engine);

    materialData.materialSet.resize(MAX_FRAMES_IN_FLIGHT);

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayoutMat);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorManager->_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

   
    Logger::vkCheck(vkAllocateDescriptorSets(engine->device, &allocInfo, materialData.materialSet.data()), "failed to allocate descriptor sets pbr pipeline");

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = resources.dataBuffer;
        bufInfo.offset = resources.dataBufferOffset;
        bufInfo.range = sizeof(MaterialPBRConstants);

        VkWriteDescriptorSet uboWrite{};
        uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboWrite.dstSet = materialData.materialSet[i];
        uboWrite.dstBinding = UBO_INDEX;                           
        uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboWrite.descriptorCount = 1;
        uboWrite.pBufferInfo = &bufInfo;

        std::array<VkDescriptorImageInfo, 6> imgInfos;
        std::array<VkWriteDescriptorSet, 7>  writes; // 1 UBO + 4 images

        writes[0] = uboWrite;
        writes[1] = makeImageWrite(materialData.materialSet[i], MATERIAL_TEX_ALBEDO, resources.albedo, imgInfos[0]);
        writes[2] = makeImageWrite(materialData.materialSet[i], MATERIAL_TEX_METAL_ROUGH, resources.metalRough, imgInfos[1]);
        writes[3] = makeImageWrite(materialData.materialSet[i], MATERIAL_TEX_OCCLUSION, resources.occlusion, imgInfos[2]);
        writes[4] = makeImageWrite(materialData.materialSet[i], MATERIAL_TEX_NORMAL, resources.normalMap, imgInfos[3]);
        writes[5] = makeImageWrite(materialData.materialSet[i], MATERIAL_TEX_TRANSMISSION, resources.transmission, imgInfos[4]);
        writes[6] = makeImageWrite(materialData.materialSet[i], MATERIAL_TEX_THICKNESS, resources.volumeThickness, imgInfos[5]);

        vkUpdateDescriptorSets(engine->device,
            static_cast<uint32_t>(writes.size()), writes.data(),
            0, nullptr);
    }

    return materialData;
}
MaterialPipeline PBRMaterialSystem::getPipeline(MaterialPass pass, VkEngine* engine) {

    MaterialPipeline p;
    if (pass == MaterialPass::Opaque) p.pipeline = engine->pipelines.opaque;
    else if (pass == MaterialPass::Transparent) p.pipeline = engine->pipelines.transparent;
    else if (pass == MaterialPass::Transmission) p.pipeline = engine->pipelines.transparent;
    p.layout = engine->pipelines.layout;
    return p;
}

void PBRMaterialSystem::initTransmissionPass(int swapW, int swapH, VkDevice device, VmaAllocator allocator) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT; // good HDR choice
    imageInfo.extent.width = swapW;
    imageInfo.extent.height = swapH;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // optional, if you want to debug-dump

    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(allocator, &imageInfo, &allocinfo, &trPass.sceneColorImage.image, &trPass.sceneColorImage.allocation, nullptr);
    
    // image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = trPass.sceneColorImage.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkCreateImageView(device, &viewInfo, nullptr, &trPass.sceneColorImage.imageView);
    
    // sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    vkCreateSampler(device, &samplerInfo, nullptr, &trPass.sampler);
}