struct ImageCreateInfoProperties {

	VkFormat format;
	VkExtent3D extent;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;
	VkImage image;
	uint32_t mipLevels = 1;
};

struct ImageViewCreateInfoProperties {

	VkFormat format;
	VkImage image;
	VkImageAspectFlags aspectFlags;
};
