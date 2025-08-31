#include "pch.h"
#include "Engine/Descriptor/vk_descriptor.h"
#include "Engine/engine.h"
#include "Core/logger.h"

VkDescriptorSetLayoutBinding DescriptorManager::createLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags, int binding) {

    VkDescriptorSetLayoutBinding b{};
    b.binding = binding;
    b.descriptorCount = 1;
    b.descriptorType = type;
    b.pImmutableSamplers = nullptr;
    b.stageFlags = stageFlags;
    return b;
}

void DescriptorManager::createDescriptorLayout(std::vector<VkDescriptorSetLayoutBinding> bindings, VkDescriptorSetLayout& layout) {

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    Logger::vkCheck(vkCreateDescriptorSetLayout(_engine->device, &layoutInfo, nullptr, &layout), "failed to create descriptor set layout");
}


// How many of each type of descriptor?
// How many descriptor sets to allocate?
void DescriptorManager::initDescriptorPool() {

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); // do *3 for 3 textures for example

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 500;

    Logger::vkCheck(vkCreateDescriptorPool(_engine->device, &poolInfo, nullptr, &_descriptorPool), "failed to create descriptor pool");
}

void DescriptorManager::initDescriptorSets() {
    
    // allocate for the 2 descriptor sets for double buffering
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayoutCamera);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    Logger::vkCheck(vkAllocateDescriptorSets(_engine->device, &allocInfo, _descriptorSets.data()), "failed to allocate descriptor sets");
}

// this is called once, memcpy camera data when needed.
void DescriptorManager::initCameraDescriptor() {

    // we need each descriptor set to have a ubo (cpu writes to one while GPU reads from the other)
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _engine->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(FrameUBO);

        VkWriteDescriptorSet descriptorWrite{};

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = _descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(_engine->device, 1, &descriptorWrite, 0, nullptr);
    }
}

// this is called for every image change.
void DescriptorManager::writeSamplerDescriptor() {

    // still write it to both since the UBO needs it
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //imageInfo.imageView = _engine->textureImageView; //temporary, create one for each texture used
        imageInfo.imageView = _engine->swapChainImageViews[0];
        //imageInfo.sampler = _engine->textureSampler; 

        VkWriteDescriptorSet descriptorWrite{};

        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = _descriptorSets[i];
        descriptorWrite.dstBinding = 1;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(_engine->device, 1, &descriptorWrite, 0, nullptr);
    }
}
/*
// pass in descriptor type and binding num
// type is something like this: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
void DescriptorManager::addBinding(uint32_t binding, VkDescriptorType type) {

    VkDescriptorSetLayoutBinding newBinding{};
    newBinding.binding = binding;
    newBinding.descriptorType = type;
    newBinding.descriptorCount = 1;
    newBinding.pImmutableSamplers = nullptr;

    // what parts of the shader can access it?
    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {

        newBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {

        newBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    _bindings.push_back(newBinding);
}
*/

// sampler bound to 1
void DescriptorManager::writeImage(VkImageView image, VkImageLayout imageLayout, VkDescriptorType type) {

    VkDescriptorImageInfo info = {};
    //info.sampler = _engine->textureSampler;
    info.imageView = image;
    info.imageLayout = imageLayout;
    imageInfos.push_back(info); // this cannot be destroyed until VkWriteDescriptorSet is called.

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = 1;
    write.dstSet = VK_NULL_HANDLE;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

// ubo bound to 0
void DescriptorManager::writeBuffer(VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type) {


    VkDescriptorBufferInfo info = {};
    info.buffer = buffer;
    info.offset = offset;
    info.range = size;
    bufferInfos.push_back(info);

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void DescriptorManager::updateSet(VkDescriptorSet set) {

    for (VkWriteDescriptorSet& write : writes) {

        write.dstSet = set;
    }
    vkUpdateDescriptorSets(_engine->device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
}

void DescriptorManager::clear() {

    writes.clear();
    imageInfos.clear();
    bufferInfos.clear();
}

VkDescriptorSet DescriptorManager::allocateSet(VkDescriptorSetLayout layout) {

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = 1;
    //allocInfo.pSetLayouts = &_descriptorSetLayout;

    VkDescriptorSet set;
    VkResult result = vkAllocateDescriptorSets(_engine->device, &allocInfo, &set);

    return set;
}



