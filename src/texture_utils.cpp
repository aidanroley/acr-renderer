#pragma once
#include "../precompile/pch.h"
#include "../include/vk_setup.h"

VkImageCreateInfo VkEngine::createImageInfo(ImageCreateInfoProperties& properties) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext = nullptr;

    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = properties.extent.width;
    imageInfo.extent.height = properties.extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = properties.mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = properties.format;
    imageInfo.tiling = properties.tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = properties.usage;
    imageInfo.samples = properties.numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    /*
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &newImage.image, &newImage.allocation, nullptr) != VK_SUCCESS) {

        throw std::runtime_error("failed to create image VMA");
    }
    */

    return imageInfo;
}

VkImageViewCreateInfo VkEngine::createImageViewInfo(ImageViewCreateInfoProperties& properties) {

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;

    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = properties.image;
    viewInfo.format = properties.format;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = properties.aspectFlags;

    return viewInfo;
}

AllocatedImage VkEngine::createRawImage(ImageCreateInfoProperties& imageProperties, bool mipmapped) {

	AllocatedImage newImage;
	newImage.imageFormat = imageProperties.format;
	newImage.imageExtent = imageProperties.extent;

    VkImageCreateInfo imageInfo = createImageInfo(imageProperties);
    if (mipmapped) {

        imageInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(imageProperties.extent.width, imageProperties.extent.height)))) + 1;
    }

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vmaCreateImage(_allocator, &imageInfo, &allocInfo, &newImage.image, &newImage.allocation, nullptr) != VK_SUCCESS) {

        throw std::runtime_error("failed to create window surface");
    }

    // which aspect of the image am i working with? this below answers that
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (imageProperties.format == VK_FORMAT_D32_SFLOAT) {

        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // init imageViewProperties struct containing info for createinfo
    ImageViewCreateInfoProperties imageViewProperties;
    imageViewProperties.format = imageProperties.format;
    imageViewProperties.image = imageProperties.image;
    imageViewProperties.aspectFlags = aspectFlag;

    VkImageViewCreateInfo viewInfo = createImageViewInfo(imageViewProperties);
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;

    // create image view and store it in the AllocatedImage
    if (vkCreateImageView(device, &viewInfo, nullptr, &newImage.imageView) != VK_SUCCESS) {

        throw std::runtime_error("failed to create window surface");
    }

    return newImage;

}

AllocatedImage VkEngine::createImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {

    ImageCreateInfoProperties imageProperties;
    imageProperties.format = format;
    imageProperties.extent = extent;
    imageProperties.usage = usage;

    AllocatedImage newa;
    return newa;

}
/*
// fix this for error thing
AllocatedImage VkEngine::createImage(void* data, VkExtent3D extent, VkFormat format, VkImageUsageFlags usage, bool mipmapped) {

    AllocatedImage newImage;
    newImage.imageFormat = format;
    newImage.imageExtent;

    // inf
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = extent;

    info.mipLevels = 1;
    info.arrayLayers = 1;

    info.samples = VK_SAMPLE_COUNT_1_BIT;

    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;

    if (mipmapped) {

        info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
    }
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vmaCreateImage(_allocator, &info, &allocInfo, &newImage.image, &newImage.allocation, nullptr) != VK_SUCCESS) {

        throw std::runtime_error("failed image creation VMA");
    }

    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {

        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // img view
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;

    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = newImage.image;
    viewInfo.format = format;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = aspectFlag;
    viewInfo.subresourceRange.levelCount = info.mipLevels;

    if (vkCreateImageView(device, &viewInfo, nullptr, &newImage.imageView) != VK_SUCCESS) {

        throw std::runtime_error("error creating image view for createImage");
    }
    return newImage;

}
*/