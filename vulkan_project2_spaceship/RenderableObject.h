#pragma once
#include "BufferDef.h"
#include "MemoryPools.h"
#include "VKUtil.h"

extern VkPhysicalDevice g_physicalDevice;
extern VkDevice g_device;
class Renderable {
public:
    string name;
    unsigned int m_vertexOffset;
    unsigned int m_indexOffset;

    glm::vec3 pos;
    glm::vec3 scale;
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

    void createUniformBuffers(size_t swapChainCount)
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
       m_UniformBuffers.resize(swapChainCount);
        m_UniformBuffersMemory.resize(swapChainCount);

        for (size_t i = 0; i < swapChainCount; i++) {
            createBuffer(g_physicalDevice, g_device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_UniformBuffers[i], m_UniformBuffersMemory[i]);
        }
    }

    void createTexture(VkCommandPool& targetCommandPool, VkQueue targetQueue)
    {
        
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(this->texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        this->m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        VkDeviceSize imageSize = texWidth * texHeight * 4;
        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(g_physicalDevice, g_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* pData;
        vkMapMemory(g_device, stagingBufferMemory, 0, imageSize, 0, &pData);
        memcpy(pData, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(g_device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, m_mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);

        transitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_mipLevels, targetCommandPool, targetQueue);
        copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), targetCommandPool, targetQueue);

        vkDestroyBuffer(g_device, stagingBuffer, nullptr);
        vkFreeMemory(g_device, stagingBufferMemory, nullptr);

        generateMipmaps(targetCommandPool, targetQueue, m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_mipLevels);
    }
    void createTextureImageViews()
    {
        m_textureImageView = createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);
    }
    
    void createTextureSamplers()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;


        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(g_physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);
        samplerInfo.minLod = 0.0f;
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(g_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void createDescriptorSets(size_t swapChainSize, VkDescriptorPool& descPool,  VkDescriptorSetLayout& descLayout)
    {
        std::vector<VkDescriptorSetLayout> layouts(swapChainSize, descLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainSize);

        allocInfo.pSetLayouts = layouts.data();

        m_DescSets.resize(swapChainSize);

        if (vkAllocateDescriptorSets(g_device, &allocInfo, m_DescSets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");

        }

        for (size_t i = 0; i < swapChainSize; i++)
        {
            VkDescriptorBufferInfo bufferInfo{};

            bufferInfo.buffer = m_UniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_textureImageView;
            imageInfo.sampler = m_textureSampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_DescSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;

            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_DescSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;

            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            descriptorWrites[1].pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(g_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

        }
    }
    void InsertVertsToBuffer(VertexPool& targetPool, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory,VkCommandPool& targetCommandPool, VkQueue& targetQ)
    {
        for (int i = 0; i < m_vertices.GetCount(); i++)
            targetPool.Add(*m_vertices[i]);
        void* pData;
        VkDeviceSize bufferSize = sizeof(Vertex) * targetPool.GetCount();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(g_physicalDevice, g_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


        vkMapMemory(g_device, stagingBufferMemory, 0, bufferSize, 0, &pData);
        memcpy(pData, targetPool.GetData(), (size_t)bufferSize);

        vkUnmapMemory(g_device, stagingBufferMemory);

        createBuffer(g_physicalDevice, g_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
        copyBuffer(stagingBuffer, vertexBuffer, bufferSize, targetCommandPool, targetQ);

        vkDestroyBuffer(g_device, stagingBuffer, nullptr);
        vkFreeMemory(g_device, stagingBufferMemory, nullptr);
    }
    void InsertIndicesToBuffer(UintPool& indexPool, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory, VkCommandPool& targetCommandPool, VkQueue& targetQ)
    {
        for (int i = 0; i < m_indices.GetCount(); i++)
            indexPool.Add(*m_indices[i]);
        VkDeviceSize bufferSize = sizeof(uint16_t) * indexPool.GetCount();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(g_physicalDevice, g_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(g_device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indexPool.GetData(), (size_t)bufferSize);

        vkUnmapMemory(g_device, stagingBufferMemory);

        createBuffer(g_physicalDevice, g_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize, targetCommandPool, targetQ);

        vkDestroyBuffer(g_device, stagingBuffer, nullptr);
        vkFreeMemory(g_device, stagingBufferMemory, nullptr);
    }
};