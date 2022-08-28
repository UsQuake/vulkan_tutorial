#pragma once
#include "BufferDef.h"
struct Renderable {

    unsigned int m_vertexOffset;
    unsigned int m_indexOffset;

	UniformBufferObject m_AnimMatrices;
	std::vector<Vertex> m_vertices;
	std::vector<uint16_t> m_indices;

    std::vector < VkBuffer> m_UniformBuffers;
    std::vector < VkDeviceMemory> m_UniformBuffersMemory;
    std::vector < VkDescriptorSet> m_DescSets;

    const char* texture_path = nullptr;
    VkImage m_textureImage;
    VkImageView m_textureImageView;
    VkDeviceMemory m_textureImageMemory;
    uint32_t m_mipLevels;
    VkSampler m_textureSampler;
    
};