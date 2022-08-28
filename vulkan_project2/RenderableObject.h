#pragma once
#include "BufferDef.h"
#include "MemoryPools.h"
struct Renderable {
    unsigned int m_vertexOffset;
    unsigned int m_indexOffset;

	UniformBufferObject m_AnimMatrices;
    VertexPool m_vertices{};
    UintPool m_indices{};

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