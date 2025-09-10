#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp> //"..\Vulkan-Hpp\Vulkan-Hpp-1.3.295\vulkan\vulkan.hpp"

#include <vma/vk_mem_alloc.h>
#include "VkBootstrap.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/util.hpp>

#include <iostream>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip> 
#include <filesystem>
#include <utility>

#include "Core/Debug/debug_timer.h"
#include "Core/Debug/logger.h"