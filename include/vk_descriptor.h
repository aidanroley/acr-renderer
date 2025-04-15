// pools - heap/memory pool for descriptors
// set - allocated object from the pool
// setLayout - blueprint for each set 


// * remember after gltf loading works make the stuff in vkengine private and make functions to fetch*
class DescriptorManager {

public:

	DescriptorManager(VkEngine* engine) {

		_engine = engine;
		_textureSampler = engine->textureSampler;
	}

	// setup
	void initDescriptorSetLayout();
	void initDescriptorSets();
	void initDescriptorPool();

	// both descriptor sets have a binding to a uniform buffer
	void writeUboDescriptor(); // writeBuffer for now
	void writeSamplerDescriptor(); // writeImage for now

	void addBinding(uint32_t binding, VkDescriptorType type);

	// update set to uniform buffer/texture
	void writeBuffer();
	void writeImage();
	void writeSampler();
	
private:

	// main engine
	VkEngine* _engine;

	// first one is just for running, use second one later
	std::array<VkDescriptorSetLayoutBinding, 2> _defaultBindings = {};
	std::vector<VkDescriptorSetLayoutBinding> _bindings;

	// sets/layout
	VkDescriptorSetLayout _descriptorSetLayout;
	std::vector<VkDescriptorSet> _descriptorSets;

	// pool stuff
	VkDescriptorPool _descriptorPool;

	// default texture sampler
	VkSampler _textureSampler;
};