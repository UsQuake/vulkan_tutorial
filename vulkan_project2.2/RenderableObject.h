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
    
    UniformBufferObject m_AnimMatrices;
    unsigned int parentIndex = 0;
    std::vector<unsigned int> childrenBoneOffsets;
    std::vector<Renderable>& ref_Meshes;

    static void UpdateAnimTree(Renderable& object)
    {
        glm::mat4x4 animTreeMatrix = glm::mat4(1.0f);
        for (auto childIdx : object.childrenBoneOffsets) {
            object.ref_Meshes[childIdx].m_AnimMatrices.model *= object.m_AnimMatrices.model;
        }
    }
};