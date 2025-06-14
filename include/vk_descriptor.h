#pragma once

//#include "../include/vk_setup.h" //FIX THIS LATER


// pools - heap/memory pool for descriptors (not descriptor sets)
// set - allocated object from the pool
// setLayout - blueprint for each set 

class VkEngine;

// * remember after gltf loading works make the stuff in vkengine private and make functions to fetch*
class DescriptorManager {

public:

	//DescriptorManager() = default;
	DescriptorManager(VkEngine* engine) {

		_engine = engine;
		//_textureSampler = engine->textureSampler;
	}

	// setup
	void initDescriptorSetLayouts();
	void initDescriptorSets();
	void initDescriptorPool();

	// both descriptor sets have a binding to a uniform buffer
	void initCameraDescriptor(); // writeBuffer for now
	void writeSamplerDescriptor(); // writeImage for now

	void addBinding(uint32_t binding, VkDescriptorType type);

	// update set to uniform buffer/texture
	void writeBuffer(VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
	void writeImage(VkImageView image, VkImageLayout imageLayout, VkDescriptorType type);
	void writeSampler();
	void clear();

	void updateSet(VkDescriptorSet set);
	VkDescriptorSet allocateSet(VkDescriptorSetLayout layout);

	// sets/layout
	VkDescriptorSetLayout _descriptorSetLayoutCamera;
	VkDescriptorSetLayout _descriptorSetLayoutMat;
	std::vector<VkDescriptorSet> _descriptorSets;
	// pool stuff
	VkDescriptorPool _descriptorPool;
	
private:

	// main engine
	VkEngine* _engine;

	// first one is just for running, use second one later
	std::array<VkDescriptorSetLayoutBinding, 3> _defaultBindings = {};
	std::vector<VkDescriptorSetLayoutBinding> _bindings;

	

	

	// default texture sampler
	VkSampler _textureSampler;

	// temp storage for writes/infos before calling VkWriteDescriptorSet
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<VkDescriptorImageInfo> imageInfos;
};