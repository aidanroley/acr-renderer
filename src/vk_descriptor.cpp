#include "../precompile/pch.h"
#include "../include/vk_descriptor.h"

void DescriptorManager::initDescriptorSetLayout() {

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;


    // so shaders can access image through sampler object
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data(); // it copies the stuff right after, so bindings can be safely destryoed

    if (vkCreateDescriptorSetLayout(_engine->device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {

        throw std::runtime_error("failed to create descriptor set layout");
    }

    // not extensible, but test to make sure it works before making it extensible.
    _defaultBindings = bindings;
}

// How many of each type of descriptor?
// How many descriptor sets to allocate?
void DescriptorManager::initDescriptorPool() {

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(_engine->device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {

        throw std::runtime_error("failed to create descriptor pool");
    }
}

void DescriptorManager::initDescriptorSets() {
    
    // allocate for the 2 descriptor sets for double buffering
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(_engine->device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {

        throw std::runtime_error("failed to allocate descriptor sets");
    }
}

void DescriptorManager::writeUboDescriptor() {

    // we need each descriptor set to have a ubo (cpu writes to one while GPU reads from the other)
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _engine->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

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

void DescriptorManager::writeSamplerDescriptor() {

    // still write it to both since the UBO needs it
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _engine->textureImageView; //temporary
        imageInfo.sampler = _textureSampler;

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



