#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// Func decs
void mainLoop(VulkanSetup& setup);
void drawFrame(VulkanSetup& setup);
void recreateSwapChain(VulkanSetup& setup);