#pragma once

enum class GraphicsAPI {
	Vulkan,
	OpenGL
};
#ifdef USE_VULKAN
inline GraphicsAPI gGraphicsAPI = GraphicsAPI::Vulkan;
#elif USE_OPENGL
inline GraphicsAPI gGraphicsAPI = GraphicsAPI::OpenGL;
#endif