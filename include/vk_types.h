#pragma once

#include <vulkan/vulkan.hpp> 
#include <vma/vk_mem_alloc.h>

struct AllocatedBuffer {

    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedImage {

    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};