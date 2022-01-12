#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>
#include <stdio.h>
#include <vector>

#include "Matrix4.h"
#include "TemporaryFormat.h"


#define FailMessage(error_condition, error_message)			\
{															\
	if (error_condition != 0)								\
	{														\
		MessageBox(NULL, error_message, L"ERROR", MB_OK);	\
		exit(-1);											\
	}														\
}																

struct MVP {
	Matrix4 projection_matrix;
	Matrix4 view_matrix;
	Matrix4 model_matrix;
} ;

struct vertices_info{
	VkBuffer buf;
	VkDeviceMemory mem;

	VkPipelineVertexInputStateCreateInfo vi;
	VkVertexInputBindingDescription vi_bindings[1];
	VkVertexInputAttributeDescription vi_attrs[1];
};
vertices_info vinfo;

struct SwapchainBuffer
{
	VkImage image;
	VkImageView view;
};

struct DepthBuffer
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
};

struct UniformBuffer
{
	VkDescriptorBufferInfo buffer_info;
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize alloc_size;
};

struct VertexBuffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct IndexBuffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
	uint32_t count;
};

const char *APP_NAME = "Vulkan Test";
LPCWSTR WINDOW_NAME = L"Vulkan Test";
uint32_t WINDOW_WIDTH = 1366;//640;
uint32_t WINDOW_HEIGHT = 768;//480;
VkDevice VK_DEVICE;
VkPhysicalDeviceMemoryProperties MEMORY_PROPERTIES;
VkFormat COLOR_FORMAT;
VkFormat DEPTH_FORMAT;
const VkSampleCountFlagBits SAMPLE_COUNT = VK_SAMPLE_COUNT_1_BIT;
uint32_t SWAPCHAIN_IMAGE_COUNT;
VkQueue QUEUE;


UniformBuffer uniform_buffer;
//UniformBuffer ub_test;   //새로운 버퍼 추가
VertexBuffer vertex_buffer;
IndexBuffer index_buffer;

VkCommandBuffer post_present_command_buffer;   // ?

//VkCommandBuffer tex_command_buffer;

MVP mvp;


char *readBinaryFile(const char *filename, size_t *psize)
{
	long int size;
	size_t retval;
	void *shader_code;

	FILE *fp = fopen(filename, "rb");
	if (!fp) return NULL;

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);

	shader_code = malloc(size);
	retval = fread(shader_code, size, 1, fp);

	*psize = size;
	return (char*)shader_code;
}

void InitializeVertexShader(VkShaderModule *vert_shader)
{
	size_t size;
	const char *shaderCode = readBinaryFile("Polygon.vert.spv", &size);

	VkShaderModuleCreateInfo shader_module_create_info;
	VkResult err;

	shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.pNext = NULL;
	shader_module_create_info.flags = 0;
	shader_module_create_info.codeSize = size;
	shader_module_create_info.pCode = (uint32_t *)shaderCode;
	err = vkCreateShaderModule(VK_DEVICE, &shader_module_create_info, NULL, vert_shader);

	if (err)
	{
		MessageBox(NULL, L"failed 'vkCreateShaderModule'", L"ERROR", MB_OK);
		exit(-1);
	}
}

void InitializeFragmentShader(VkShaderModule *frag_shader)
{
	size_t size;
	const char *shaderCode = readBinaryFile("Polygon.frag.spv", &size);

	VkShaderModuleCreateInfo shader_module_create_info;
	VkResult err;

	shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_module_create_info.pNext = NULL;
	shader_module_create_info.flags = 0;
	shader_module_create_info.codeSize = size;
	shader_module_create_info.pCode = (uint32_t *)shaderCode;
	err = vkCreateShaderModule(VK_DEVICE, &shader_module_create_info, NULL, frag_shader);

	if(err)
	{
		MessageBox(NULL, L"failed 'vkCreateShaderModule'", L"ERROR", MB_OK);
		exit(-1);
	}
}

void SetImageLayout(VkCommandBuffer setup_command_buffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout old_image_layout, VkImageLayout new_image_layout)
{
	VkImageMemoryBarrier image_memory_barrier;
	memset(&image_memory_barrier, 0, sizeof(VkImageMemoryBarrier));
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.pNext = NULL;
	image_memory_barrier.srcAccessMask = 0;
	image_memory_barrier.dstAccessMask = 0;
	image_memory_barrier.oldLayout = old_image_layout;
	image_memory_barrier.newLayout = new_image_layout;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = aspectMask;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	if (old_image_layout == VK_IMAGE_LAYOUT_UNDEFINED)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	// Old layout is color attachment
	// Make sure any writes to the color buffer have been finished
	if (old_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	// Old layout is transfer source
	// Make sure any reads from the image have been finished
	if (old_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// Old layout is shader read (sampler, input attachment)
	// Make sure any shader reads from the image have been finished
	if (old_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	// Target layouts (new)

	// New layout is transfer destination (copy, blit)
	// Make sure any copyies to the image have been finished
	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	// New layout is transfer source (copy, blit)
	// Make sure any reads from and writes to the image have been finished
	if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = image_memory_barrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// New layout is color attachment
	// Make sure any writes to the color buffer hav been finished
	if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	// New layout is depth attachment
	// Make sure any writes to depth/stencil buffer have been finished
	if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		image_memory_barrier.dstAccessMask = image_memory_barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	// New layout is shader read (sampler, input attachment)
	// Make sure any writes to the image have been finished
	if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

	vkCmdPipelineBarrier(setup_command_buffer, src_stages, dest_stages, 0, 0, NULL, 0, NULL, 1, &image_memory_barrier);
}

bool memory_type_from_properties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
	for (uint32_t i = 0; i < 32; i++) 
	{
		if ((typeBits & 1) == 1) 
		{
			if ((MEMORY_PROPERTIES.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}

void draw(VkSwapchainKHR *swapchain_khr, SwapchainBuffer *swapchain_buffers, VkCommandBuffer *draw_command_buffers)
{
	
	static float yrot = 0.0f;
	const float speed = 10.0f;
	const float aspeed = 15.0f;
	if (GetAsyncKeyState(VK_UP))
	{
		mvp.view_matrix = mvp.view_matrix * Matrix4Translation(0.0f, 0.0f, 0.1f*speed);
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		mvp.view_matrix = mvp.view_matrix * Matrix4Translation(0.0f, 0.0f, -0.1f*speed);
	}
	if (GetAsyncKeyState(VK_LEFT))
	{
		mvp.view_matrix = mvp.view_matrix * Matrix4Translation(0.1f*speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState(VK_RIGHT))
	{
		mvp.view_matrix = mvp.view_matrix * Matrix4Translation(-0.1f*speed, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState(0x52))
	{
		mvp.view_matrix = mvp.view_matrix * Matrix4Translation(0.0f, -0.1f*speed, 0.0f);
	}
	if (GetAsyncKeyState(0x46))
	{
		mvp.view_matrix = mvp.view_matrix * Matrix4Translation(0.0f, 0.1f*speed, 0.0f);
	}
	if (GetAsyncKeyState(0x53))
	{
		mvp.view_matrix = mvp.view_matrix *Matrix4AxisAngleRotation(0.01f, 1.0f, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState(0x57))
	{
		mvp.view_matrix = mvp.view_matrix *Matrix4AxisAngleRotation(0.01f, -1.0f, 0.0f, 0.0f);
	}
	if (GetAsyncKeyState(0x41))
	{
		yrot = 0.001f*aspeed;
		mvp.view_matrix = mvp.view_matrix *Matrix4AxisAngleRotation(0.01f, 0.0f, -1.0f, 0.0f);
	}
	if (GetAsyncKeyState(0x44))
	{
		yrot = 0.001f*aspeed;
		mvp.view_matrix = mvp.view_matrix *Matrix4AxisAngleRotation(0.01f, 0.0f, 1.0f, 0.0f);
	}
	
	void *data;
	Matrix4 im = mvp.model_matrix * mvp.view_matrix * mvp.projection_matrix * AdjustCoordinate();
	vkMapMemory(VK_DEVICE, uniform_buffer.memory, 0, uniform_buffer.alloc_size, 0, &data);
	memcpy(data, &im, uniform_buffer.alloc_size);
	vkUnmapMemory(VK_DEVICE, uniform_buffer.memory);


	VkResult err;

	vkDeviceWaitIdle(VK_DEVICE);
	static uint32_t buffer_index = 0;

	VkSemaphore presentCompleteSemaphore;
	VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
	presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	presentCompleteSemaphoreCreateInfo.pNext = NULL;
	presentCompleteSemaphoreCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	err = vkCreateSemaphore(VK_DEVICE, &presentCompleteSemaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
	FailMessage(err, L"failed 'vkCreateSemaphore'");

	err = vkAcquireNextImageKHR(VK_DEVICE, *swapchain_khr, 0xffffffff, presentCompleteSemaphore, 0, &buffer_index);
	FailMessage(err, L"failed 'vkAcquireNextImageKHR'");

	// The submit infor strcuture contains a list of
	// command buffers and semaphores to be submitted to a queue
	// If you want to submit multiple command buffers, pass an array
	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &draw_command_buffers[buffer_index];
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	submitInfo.pWaitDstStageMask = NULL;

	// Submit to the graphics queue
	err = vkQueueSubmit(QUEUE, 1, &submitInfo, VK_NULL_HANDLE);
	FailMessage(err, L"failed 'vkQueueSubmit'");

	// Present the current buffer to the swap chain
	// This will display the image

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchain_khr;
	presentInfo.pImageIndices = &buffer_index;
	presentInfo.pResults = NULL;
	presentInfo.waitSemaphoreCount = 0;
	presentInfo.pWaitSemaphores = NULL;

	err = vkQueuePresentKHR(QUEUE, &presentInfo);
	FailMessage(err, L"failed 'vkQueuePresentKHR'");

	vkDestroySemaphore(VK_DEVICE, presentCompleteSemaphore, NULL);

	// Add a post present image memory barrier
	// This will transform the frame buffer color attachment back
	// to it's initial layout after it has been presented to the
	// windowing system
	// See buildCommandBuffers for the pre present barrier that 
	// does the opposite transformation 
	VkImageMemoryBarrier postPresentBarrier;
	postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	postPresentBarrier.pNext = NULL;
	postPresentBarrier.srcAccessMask = 0;
	postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	postPresentBarrier.image = swapchain_buffers[buffer_index].image;

	// Use dedicated command buffer from example base class for submitting the post present barrier
	VkCommandBufferBeginInfo cmdBufInfo;
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = NULL;
	cmdBufInfo.flags = 0;
	cmdBufInfo.pInheritanceInfo = NULL;

	err = vkBeginCommandBuffer(post_present_command_buffer, &cmdBufInfo);
	FailMessage(err, L"failed 'vkBeginCommandBuffer'");

	// Put post present barrier into command buffer
	vkCmdPipelineBarrier(
		post_present_command_buffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &postPresentBarrier);

	err = vkEndCommandBuffer(post_present_command_buffer);
	FailMessage(err, L"failed 'vkEndCommandBuffer'");

	// Submit to the queue
	submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &post_present_command_buffer;



	err = vkQueueSubmit(QUEUE, 1, &submitInfo, VK_NULL_HANDLE);
	FailMessage(err, L"failed 'vkQueueSubmit'");

	err = vkQueueWaitIdle(QUEUE);
	FailMessage(err, L"failed 'vkQueueWaitIdle'");

	vkDeviceWaitIdle(VK_DEVICE);
}


void buildCommandBuffers(VkRenderPass render_pass, VkPipelineLayout pipeline_layout, VkPipeline pipeline, VkDescriptorSet *descriptor_set,SwapchainBuffer *swapchain_buffers, VkCommandBuffer *draw_command_buffers, VkFramebuffer *frame_buffers)
{
	VkResult err;

	VkCommandBufferBeginInfo command_buffer_begin_info;
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = NULL;
	command_buffer_begin_info.flags = 0;
	command_buffer_begin_info.pInheritanceInfo = NULL;

	VkClearValue clearValues[2];
	clearValues[0].color = { 0.025f, 0.025f, 0.025f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo render_pass_begin_info;
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext = NULL;
	render_pass_begin_info.renderPass = render_pass;
	render_pass_begin_info.renderArea.offset.x = 0;
	render_pass_begin_info.renderArea.offset.y = 0;
	render_pass_begin_info.renderArea.extent.width = WINDOW_WIDTH;
	render_pass_begin_info.renderArea.extent.height = WINDOW_HEIGHT;
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues = clearValues;


	for (uint32_t i = 0; i < SWAPCHAIN_IMAGE_COUNT; ++i)
	{
		render_pass_begin_info.framebuffer = frame_buffers[i];

		err = vkBeginCommandBuffer(draw_command_buffers[i], &command_buffer_begin_info);
		FailMessage(err, L"failed 'vkBeginCommandBuffer'");

		vkCmdBeginRenderPass(draw_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport;
		viewport.height = (float)WINDOW_HEIGHT;
		viewport.width = (float)WINDOW_WIDTH;
		viewport.minDepth = (float) 0.0f;
		viewport.maxDepth = (float) 1.0f;
		viewport.x = 0;
		viewport.y = 0;
		vkCmdSetViewport(draw_command_buffers[i], 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor;
		scissor.extent.width = WINDOW_WIDTH;
		scissor.extent.height = WINDOW_HEIGHT;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(draw_command_buffers[i], 0, 1, &scissor);

		// Bind descriptor sets describing shader binding points
		vkCmdBindDescriptorSets(draw_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, descriptor_set, 0, NULL);

		// Bind the rendering pipeline (including the shaders)
		vkCmdBindPipeline(draw_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		// Bind triangle vertices
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(draw_command_buffers[i], 0, 1, &vertex_buffer.buffer, offsets);

		// Bind triangle indices
		vkCmdBindIndexBuffer(draw_command_buffers[i], index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Draw indexed triangle
		vkCmdDrawIndexed(draw_command_buffers[i], index_buffer.count, 1, 0, 0, 1);

		vkCmdEndRenderPass(draw_command_buffers[i]);

		// Add a present memory barrier to the end of the command buffer
		// This will transform the frame buffer color attachment to a
		// new layout for presenting it to the windowing system integration 
		VkImageMemoryBarrier prePresentBarrier;
		prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		prePresentBarrier.pNext = NULL;
		prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		prePresentBarrier.dstAccessMask = 0;
		prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		prePresentBarrier.image = swapchain_buffers[i].image;

		VkImageMemoryBarrier *pMemoryBarrier = &prePresentBarrier;
		vkCmdPipelineBarrier(draw_command_buffers[i], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			0, 0, NULL, 0, NULL, 1, &prePresentBarrier);

		err = vkEndCommandBuffer(draw_command_buffers[i]);
		FailMessage(err, L"failed 'vkEndCommandBuffer'");
	}
}

void AllocateDescriptorSets(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout *descriptor_set_layout, VkDescriptorSet *descriptor_set)
{
	VkResult err;

	VkDescriptorSetAllocateInfo alloc_info[1];
	alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info[0].pNext = NULL;
	alloc_info[0].descriptorPool = descriptor_pool;
	alloc_info[0].descriptorSetCount = 1;
	alloc_info[0].pSetLayouts = descriptor_set_layout;

	err = vkAllocateDescriptorSets(VK_DEVICE, alloc_info, descriptor_set);
	FailMessage(err, L"failed 'vkAllocateDescriptorSets'");

	/*memset(&tex_descs, 0, sizeof(tex_descs));
	for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
	tex_descs[i].sampler = demo->textures[i].sampler;
	tex_descs[i].imageView = demo->textures[i].view;
	tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	}*/

	VkWriteDescriptorSet writes[1];
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = *descriptor_set;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;   //uniform type
	writes[0].pBufferInfo = &uniform_buffer.buffer_info;   //uniform 버퍼 정보
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0;   //바인드할 번호
	writes[0].pImageInfo = NULL;
	writes[0].pTexelBufferView = NULL;

	/*writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].pNext = NULL;
	writes[1].dstSet = *descriptor_set;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[1].pBufferInfo = &ub_test.buffer_info;
	writes[1].dstArrayElement = 0;
	writes[1].dstBinding = 1;
	writes[1].pImageInfo = NULL;
	writes[1].pTexelBufferView = NULL;*/

	vkUpdateDescriptorSets(VK_DEVICE, 1, writes, 0, NULL);

}

void CreateDescriptorPool(VkDescriptorPool *descriptor_pool)
{
	VkResult err;

	VkDescriptorPoolSize type_count[1];
	type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[0].descriptorCount = 1;   //uniform type

	/*type_count[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_count[1].descriptorCount = 1;*/

	VkDescriptorPoolCreateInfo descriptor_pool_create_info;
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.pNext = NULL;
	descriptor_pool_create_info.flags = 0;
	descriptor_pool_create_info.maxSets = 1;
	descriptor_pool_create_info.poolSizeCount = 1;
	descriptor_pool_create_info.pPoolSizes = type_count;

	err = vkCreateDescriptorPool(VK_DEVICE, &descriptor_pool_create_info, NULL, descriptor_pool);
	FailMessage(err, L"failed 'vkCreateDescriptorPool'");

}


void InitializePipeline(VkRenderPass render_pass, VkPipelineLayout pipeline_layout, VkPipeline *pipeline)
{
	VkResult err;

	VkPipelineCacheCreateInfo pipeline_cache_create_info;
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = NULL;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = NULL;
	pipeline_cache_create_info.flags = 0;

	VkPipelineCache pipeline_cache;
	err = vkCreatePipelineCache(VK_DEVICE, &pipeline_cache_create_info, NULL, &pipeline_cache);
	FailMessage(err, L"failed 'vkCreatePipelineCache'");


	
	VkPipelineVertexInputStateCreateInfo vi = vinfo.vi;
	/*vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.pNext = NULL;
	vi.flags = 0;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = vinfo.vi_bindings;
	vi.vertexAttributeDescriptionCount = 2;
	vi.pVertexAttributeDescriptions = vinfo.vi_attrs;*/

	VkPipelineInputAssemblyStateCreateInfo ia;
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.pNext = NULL;
	ia.flags = 0;
	ia.primitiveRestartEnable = VK_FALSE;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rs;
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.pNext = NULL;
	rs.flags = 0;
	rs.polygonMode = VK_POLYGON_MODE_LINE;   //폴리곤 모드
	rs.cullMode = VK_CULL_MODE_NONE;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.depthBiasConstantFactor = 0;
	rs.depthBiasClamp = 0;
	rs.depthBiasSlopeFactor = 0;
	rs.lineWidth = 1;   //선 폴리곤 모드의 선 두께

	VkPipelineColorBlendStateCreateInfo cb;
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.pNext = NULL;
	cb.flags = 0;

	VkPipelineColorBlendAttachmentState att_state[1];
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	att_state[0].alphaBlendOp = VK_BLEND_OP_ADD;
	att_state[0].colorBlendOp = VK_BLEND_OP_ADD;
	att_state[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	att_state[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;
	cb.logicOpEnable = VK_FALSE;
	cb.logicOp = VK_LOGIC_OP_CLEAR;
	cb.blendConstants[0] = 0.0f;
	cb.blendConstants[1] = 0.0f;
	cb.blendConstants[2] = 0.0f;
	cb.blendConstants[3] = 0.0f;

	VkPipelineViewportStateCreateInfo vp;
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.pNext = NULL;
	vp.flags = 0;
	vp.viewportCount = 1;
	vp.scissorCount = 1;
	vp.pScissors = NULL;
	vp.pViewports = NULL;

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
	std::vector<VkDynamicState> dynamicStateEnables;
	//VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];

	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	//dynamicStateEnables[dynamic_state_create_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
	//dynamicStateEnables[dynamic_state_create_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.pNext = NULL;
	dynamic_state_create_info.pDynamicStates = dynamicStateEnables.data();
	dynamic_state_create_info.dynamicStateCount = dynamicStateEnables.size();
	dynamic_state_create_info.flags = 0;

	VkPipelineDepthStencilStateCreateInfo ds;
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.pNext = NULL;
	ds.flags = 0;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.minDepthBounds = 0;
	ds.maxDepthBounds = 0;
	ds.stencilTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.back.compareMask = 0;
	ds.back.reference = 0;
	ds.back.depthFailOp = VK_STENCIL_OP_KEEP;
	ds.back.writeMask = 0;
	ds.front = ds.back;

	VkPipelineMultisampleStateCreateInfo ms;
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pNext = NULL;
	ms.flags = 0;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = SAMPLE_COUNT;
	ms.sampleShadingEnable = VK_FALSE;
	ms.alphaToCoverageEnable = VK_FALSE;
	ms.alphaToOneEnable = VK_FALSE;
	ms.minSampleShading = 0.0f;


	VkPipelineShaderStageCreateInfo shaderStages[2];
	memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

	VkShaderModule vert_shader;
	InitializeVertexShader(&vert_shader);
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vert_shader;
	shaderStages[0].pName = "main";

	VkShaderModule frag_shader;
	InitializeFragmentShader(&frag_shader);
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = frag_shader;
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info;
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.pNext = NULL;
	graphics_pipeline_create_info.layout = pipeline_layout;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = 0;
	graphics_pipeline_create_info.flags = 0;
	graphics_pipeline_create_info.pVertexInputState = &vi;
	graphics_pipeline_create_info.pInputAssemblyState = &ia;
	graphics_pipeline_create_info.pRasterizationState = &rs;
	graphics_pipeline_create_info.pColorBlendState = &cb;
	graphics_pipeline_create_info.pTessellationState = NULL;
	graphics_pipeline_create_info.pMultisampleState = &ms;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	graphics_pipeline_create_info.pViewportState = &vp;
	graphics_pipeline_create_info.pDepthStencilState = &ds;
	graphics_pipeline_create_info.pStages = shaderStages;
	graphics_pipeline_create_info.stageCount = 2;
	graphics_pipeline_create_info.renderPass = render_pass;
	graphics_pipeline_create_info.subpass = 0;

	err = vkCreateGraphicsPipelines(VK_DEVICE, pipeline_cache, 1, &graphics_pipeline_create_info, NULL, pipeline);
	FailMessage(err, L"failed 'vkCreateGraphicsPipelines'");

}


void InitializeVertexBuffer()
{
	VkResult err;

	Model model1;
	ReadModel("map.hmap", &model1);

	uint32_t vertex_data_size = model1.vertex_count * 3 * sizeof(float);
	uint32_t index_data_size = model1.index_count* sizeof(int);
	index_buffer.count = model1.index_count;


	VkBufferCreateInfo buffer_create_info;
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = NULL;
	buffer_create_info.flags = 0;
	buffer_create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_create_info.size = vertex_data_size;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = NULL;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	
	err = vkCreateBuffer(VK_DEVICE, &buffer_create_info, NULL, &vertex_buffer.buffer);
	FailMessage(err, L"failed 'vkCreateBuffer'");

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(VK_DEVICE, vertex_buffer.buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info;
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = NULL;
	memory_allocate_info.memoryTypeIndex = 0;
	memory_allocate_info.allocationSize = memory_requirements.size;

	bool pass = memory_type_from_properties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,&memory_allocate_info.memoryTypeIndex);
	FailMessage(!pass, L"failed 'memory_type_from_properties'");

	err = vkAllocateMemory(VK_DEVICE, &memory_allocate_info, NULL, &vertex_buffer.memory);
	FailMessage(err, L"failed 'vkAllocateMemory'");

	void *pData;
	err = vkMapMemory(VK_DEVICE, vertex_buffer.memory, 0, memory_requirements.size, 0, &pData);
	FailMessage(err, L"failed 'vkMapMemory'");

	memcpy(pData, model1.vertices, vertex_data_size);

	vkUnmapMemory(VK_DEVICE, vertex_buffer.memory);

	err = vkBindBufferMemory(VK_DEVICE, vertex_buffer.buffer, vertex_buffer.memory, 0);
	FailMessage(err, L"failed 'vkBindBufferMemory'");



	VkBufferCreateInfo indexbufferInfo = {};
	indexbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	indexbufferInfo.pNext = NULL;
	indexbufferInfo.flags = 0;
	indexbufferInfo.size = index_data_size;
	indexbufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	// Copy index data to VRAM

	err = vkCreateBuffer(VK_DEVICE, &indexbufferInfo, nullptr, &index_buffer.buffer);
	FailMessage(err, L"failed 'vkCreateBuffer'");

	vkGetBufferMemoryRequirements(VK_DEVICE, index_buffer.buffer, &memory_requirements);
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_type_from_properties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex);

	err = vkAllocateMemory(VK_DEVICE, &memory_allocate_info, nullptr, &index_buffer.memory);
	FailMessage(err, L"failed 'vkAllocateMemory'");

	err = vkMapMemory(VK_DEVICE, index_buffer.memory, 0, index_data_size, 0, &pData);
	FailMessage(err, L"failed 'vkMapMemory'");

	memcpy(pData, model1.indices, index_data_size);
	vkUnmapMemory(VK_DEVICE, index_buffer.memory);
	err = vkBindBufferMemory(VK_DEVICE, index_buffer.buffer, index_buffer.memory, 0);
	FailMessage(err, L"failed 'vkBindBufferMemory'");
	//indices.count = indexBuffer.size();


	vinfo.vi_bindings[0].binding = 0;
	vinfo.vi_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vinfo.vi_bindings[0].stride = 3*sizeof(float);

	vinfo.vi_attrs[0].binding = 0;
	vinfo.vi_attrs[0].location = 0;
	vinfo.vi_attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vinfo.vi_attrs[0].offset = 0;

	vinfo.vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vinfo.vi.pNext = NULL;
	vinfo.vi.flags = 0;
	vinfo.vi.vertexBindingDescriptionCount = 1;
	vinfo.vi.pVertexBindingDescriptions = vinfo.vi_bindings;
	vinfo.vi.vertexAttributeDescriptionCount = 1;
	vinfo.vi.pVertexAttributeDescriptions = vinfo.vi_attrs;

}

void InitializeFrameBuffer(VkRenderPass render_pass, SwapchainBuffer *swapchain_buffers, DepthBuffer depth_buffer, VkFramebuffer **rv_frame_buffers)
{
	VkImageView attachments[2];
	attachments[1] = depth_buffer.view;

	VkFramebufferCreateInfo frame_buffer_create_info;
	frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frame_buffer_create_info.flags = 0;
	frame_buffer_create_info.pNext = NULL;
	frame_buffer_create_info.renderPass = render_pass;
	frame_buffer_create_info.attachmentCount = 2;
	frame_buffer_create_info.pAttachments = attachments;
	frame_buffer_create_info.width = WINDOW_WIDTH;
	frame_buffer_create_info.height = WINDOW_HEIGHT;
	frame_buffer_create_info.layers = 1;

	VkFramebuffer *frame_buffers = (VkFramebuffer *)malloc(SWAPCHAIN_IMAGE_COUNT * sizeof(VkFramebuffer));
	FailMessage(!frame_buffers, L"failed to allocate 'frame_buffers'");

	for (uint32_t i = 0; i < SWAPCHAIN_IMAGE_COUNT; i++)
	{
		attachments[0] = swapchain_buffers[i].view;
		VkResult err = vkCreateFramebuffer(VK_DEVICE, &frame_buffer_create_info, NULL, &frame_buffers[i]);
		FailMessage(err, L"failed 'vkCreateFramebuffer'");
	}
	*rv_frame_buffers = frame_buffers;
}

void InitializeRenderPass(VkRenderPass *render_pass)
{
	VkResult err;

	VkAttachmentDescription attachment_description[2];
	attachment_description[0].format = COLOR_FORMAT;
	attachment_description[0].samples = SAMPLE_COUNT;
	attachment_description[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment_description[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment_description[0].flags = 0;

	attachment_description[1].format = DEPTH_FORMAT;
	attachment_description[1].samples = SAMPLE_COUNT;
	attachment_description[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_description[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment_description[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_description[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_description[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment_description[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment_description[1].flags = 0;

	VkAttachmentReference color_reference;
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference;
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.flags = 0;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &depth_reference;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

	VkRenderPassCreateInfo render_pass_create_info;
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.pNext = NULL;
	render_pass_create_info.flags = 0;
	render_pass_create_info.attachmentCount = 2;
	render_pass_create_info.pAttachments = attachment_description;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;
	render_pass_create_info.dependencyCount = 0;
	render_pass_create_info.pDependencies = NULL;

	err = vkCreateRenderPass(VK_DEVICE, &render_pass_create_info, NULL, render_pass);
	FailMessage(err, L"failed 'vkCreateRenderPass'");
	
}

/*void VulkanTextureLoader(VkCommandPool command_pool)
{
	//vkGetPhysicalDeviceMemoryProperties(physical_device, &deviceMemoryProperties);

	// Create command buffer for submitting image barriers
	// and converting tilings
	VkCommandBufferAllocateInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufInfo.commandPool = command_pool;
	cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufInfo.commandBufferCount = 1;
	cmdBufInfo.pNext = NULL;

	VkResult vkRes = vkAllocateCommandBuffers(VK_DEVICE, &cmdBufInfo, &tex_command_buffer);
	assert(vkRes == VK_SUCCESS);
}*/

void CreatePipelineLayout(VkDescriptorSetLayout *descriptor_set_layout, VkPipelineLayout *pipeline_layout)
{
	VkPipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.pNext = NULL;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = NULL;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = descriptor_set_layout;
	pipeline_layout_create_info.flags = 0;

	VkResult err = vkCreatePipelineLayout(VK_DEVICE, &pipeline_layout_create_info, NULL, pipeline_layout);
	FailMessage(err, L"failed 'vkCreatePipelineLayout'");

}

void CreateDescriptorSetLayout(VkDescriptorSetLayout *descriptor_set_layout)
{
	VkResult err;

	VkDescriptorSetLayoutBinding layout_binding[1];   //VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER도 있음
	layout_binding[0].binding = 0;   //바인딩 번호
	layout_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;   //uniform type
	layout_binding[0].descriptorCount = 1;
	layout_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   //셰이더 종류(vert와 frag 구분없이 사용가능)
	layout_binding[0].pImmutableSamplers = NULL;

	/*
	layout_binding[1].binding = 1;
	layout_binding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_binding[1].descriptorCount = 1;
	layout_binding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_binding[1].pImmutableSamplers = NULL;*/

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
	descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.pNext = NULL;
	descriptor_set_layout_create_info.bindingCount = 1;
	descriptor_set_layout_create_info.pBindings = layout_binding;
	descriptor_set_layout_create_info.flags = 0;

	err = vkCreateDescriptorSetLayout(VK_DEVICE, &descriptor_set_layout_create_info, NULL, descriptor_set_layout);
	FailMessage(err, L"failed 'vkCreateDescriptorSetLayout'");
}

void CreateUniformBuffer(const void *input_data, uint32_t data_size, UniformBuffer *ret_uniform_buffer)
{
	VkBufferCreateInfo buffer_create_info;   //버퍼 정보
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.pNext = NULL;
	buffer_create_info.flags = 0;
	buffer_create_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;   //용도는 uniform 버퍼로 사용
	buffer_create_info.size = data_size;   //버퍼의 크기
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = NULL;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(VK_DEVICE, &buffer_create_info, NULL, &ret_uniform_buffer->buffer);   //버퍼 생성

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(VK_DEVICE, ret_uniform_buffer->buffer, &memory_requirements);
	ret_uniform_buffer->alloc_size = memory_requirements.size;

	VkMemoryAllocateInfo memory_allocate_info;   //메모리 할당 정보
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = NULL;
	memory_allocate_info.memoryTypeIndex = 0;
	memory_allocate_info.allocationSize = memory_requirements.size;

	memory_type_from_properties(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memory_allocate_info.memoryTypeIndex);

	vkAllocateMemory(VK_DEVICE, &memory_allocate_info, NULL, &ret_uniform_buffer->memory);   //메모리 할당

	vkBindBufferMemory(VK_DEVICE, ret_uniform_buffer->buffer, ret_uniform_buffer->memory, 0);   //버퍼와 메모리를 결속

	//메모리 맵핑
	void *map_data;
	vkMapMemory(VK_DEVICE, ret_uniform_buffer->memory, 0, memory_requirements.size, 0, &map_data);
	memcpy(map_data, input_data, data_size);
	vkUnmapMemory(VK_DEVICE, ret_uniform_buffer->memory);

	//버퍼 정보 저장
	ret_uniform_buffer->buffer_info.buffer = ret_uniform_buffer->buffer;
	ret_uniform_buffer->buffer_info.offset = 0;
	ret_uniform_buffer->buffer_info.range = data_size;
}


void CreateDepthBuffer(VkCommandBuffer setup_command_buffer, DepthBuffer *depth_buffer)
{
	VkImageCreateInfo image_create_info;
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.pNext = NULL;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = DEPTH_FORMAT;
	image_create_info.extent.width = WINDOW_WIDTH;
	image_create_info.extent.height = WINDOW_HEIGHT;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = SAMPLE_COUNT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_create_info.queueFamilyIndexCount = 0;
	image_create_info.pQueueFamilyIndices = NULL;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.flags = 0;

	VkImageViewCreateInfo view_create_info;
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.pNext = NULL;
	view_create_info.image = VK_NULL_HANDLE;
	view_create_info.format = DEPTH_FORMAT;
	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.flags = 0;

	VkMemoryRequirements mem_reqs;
	VkResult err;
	//info.depth.format = depth_format;


	err = vkCreateImage(VK_DEVICE, &image_create_info, NULL, &depth_buffer->image);
	FailMessage(err, L"failed 'vkCreateImage'");

	vkGetImageMemoryRequirements(VK_DEVICE, depth_buffer->image, &mem_reqs);

	VkMemoryAllocateInfo memory_allocate_info;
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.pNext = NULL;
	memory_allocate_info.allocationSize = mem_reqs.size;
	memory_allocate_info.memoryTypeIndex = 0;


	bool pass = memory_type_from_properties(mem_reqs.memoryTypeBits, 0, &memory_allocate_info.memoryTypeIndex);
	FailMessage(!pass, L"failed 'memory_type_from_properties'");

	
	err = vkAllocateMemory(VK_DEVICE, &memory_allocate_info, NULL, &depth_buffer->memory);
	FailMessage(err, L"failed 'vkAllocateMemory'");

	err = vkBindImageMemory(VK_DEVICE, depth_buffer->image, depth_buffer->memory, 0);
	FailMessage(err, L"failed 'vkBindImageMemory'");

	SetImageLayout(setup_command_buffer, depth_buffer->image, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	view_create_info.image = depth_buffer->image;
	err = vkCreateImageView(VK_DEVICE, &view_create_info, NULL, &depth_buffer->view);
	FailMessage(err, L"failed 'vkCreateImageView'");

}

void CreateDrawCommandBuffers(VkCommandPool command_pool, VkCommandBuffer **rv_draw_command_buffers)
{
	// Create one command buffer per frame buffer 
	// in the swap chain
	// Command buffers store a reference to the 
	// frame buffer inside their render pass info
	// so for static usage withouth having to rebuild 
	// them each frame, we use one per frame buffer

	VkCommandBuffer *draw_command_buffers = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer)* SWAPCHAIN_IMAGE_COUNT);

	VkCommandBufferAllocateInfo cmd_buffer_allocate_info;
	cmd_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_allocate_info.pNext = NULL;
	cmd_buffer_allocate_info.commandPool = command_pool;
	cmd_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buffer_allocate_info.commandBufferCount = SWAPCHAIN_IMAGE_COUNT;

	VkResult err = vkAllocateCommandBuffers(VK_DEVICE, &cmd_buffer_allocate_info, draw_command_buffers);
	FailMessage(err, L"failed 'vkAllocateCommandBuffers'");

	// Create one command buffer for submitting the
	// post present image memory barrier
	cmd_buffer_allocate_info.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(VK_DEVICE, &cmd_buffer_allocate_info, &post_present_command_buffer);
	FailMessage(err, L"failed 'vkAllocateCommandBuffers'");

	*rv_draw_command_buffers = draw_command_buffers;
}

void CreateSwapchainBuffers(VkSwapchainKHR swapchain_khr, VkCommandBuffer setup_command_buffer, SwapchainBuffer **rv_swapchain_buffers)
{
	VkResult err;

	uint32_t swapchain_image_count;
	err = vkGetSwapchainImagesKHR(VK_DEVICE, swapchain_khr, &swapchain_image_count, NULL);
	FailMessage(err, L"failed 'vkGetSwapchainImagesKHR'");
	SWAPCHAIN_IMAGE_COUNT = swapchain_image_count;

	VkImage *swapchain_images = (VkImage *)malloc(swapchain_image_count * sizeof(VkImage));
	FailMessage(!swapchain_images, L"failed 'vkGetSwapchainImagesKHR'");
	err = vkGetSwapchainImagesKHR(VK_DEVICE, swapchain_khr, &swapchain_image_count, swapchain_images);
	FailMessage(err, L"failed 'vkGetSwapchainImagesKHR'");

	SwapchainBuffer *swapchain_buffers = (SwapchainBuffer *)malloc(sizeof(SwapchainBuffer)*swapchain_image_count);
	FailMessage(swapchain_buffers == NULL, L"failed 'vkAllocateCommandBuffers'");



	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		VkImageViewCreateInfo color_image_view;
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.format = COLOR_FORMAT;
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.flags = 0;

		swapchain_buffers[i].image = swapchain_images[i];

		SetImageLayout(setup_command_buffer, swapchain_buffers[i].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		color_image_view.image = swapchain_buffers[i].image;

		err = vkCreateImageView(VK_DEVICE, &color_image_view, NULL, &swapchain_buffers[i].view);
		FailMessage(err, L"failed 'vkCreateImageView'");

		*rv_swapchain_buffers = swapchain_buffers;
	}
	free(swapchain_images);
}

void InitializeSwapchainKHR(VkPhysicalDevice physical_device, VkSurfaceKHR surface_khr, VkSwapchainKHR *swapchain_khr)
{
	VkResult err;


	uint32_t format_count;
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface_khr, &format_count, NULL);
	FailMessage(err, L"failed 'vkGetPhysicalDeviceSurfaceFormatsKHR'");

	VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(format_count * sizeof(VkSurfaceFormatKHR));
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface_khr, &format_count, surface_formats);
	FailMessage(err, L"failed 'vkGetPhysicalDeviceSurfaceFormatsKHR'");


	if (format_count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		COLOR_FORMAT = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else
	{
		FailMessage(format_count < 1, L"error Surface Format Count");
		COLOR_FORMAT = surface_formats[0].format;
	}
	VkColorSpaceKHR color_space = surface_formats[0].colorSpace;
	free(surface_formats);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface_khr, &surface_capabilities);
	FailMessage(err, L"failed 'vkGetPhysicalDeviceSurfaceCapabilitiesKHR'");

	uint32_t present_mode_count;
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, NULL);
	FailMessage(err, L"failed 'vkGetPhysicalDeviceSurfacePresentModesKHR'");

	VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(present_mode_count * sizeof(VkPresentModeKHR));
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface_khr, &present_mode_count, present_modes);
	FailMessage(err, L"failed 'vkGetPhysicalDeviceSurfacePresentModesKHR'");

	VkExtent2D swapchain_extent;
	if (surface_capabilities.currentExtent.width == (uint32_t)-1)
	{
		swapchain_extent.width = WINDOW_WIDTH;
		swapchain_extent.height = WINDOW_HEIGHT;
	}
	else
	{
		swapchain_extent = surface_capabilities.currentExtent;
		WINDOW_WIDTH = surface_capabilities.currentExtent.width;
		WINDOW_HEIGHT = surface_capabilities.currentExtent.height;
	}

	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < present_mode_count; i++)
	{
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if ((swapchain_present_mode != VK_PRESENT_MODE_MAILBOX_KHR) && (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR))
		{
			swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
	free(present_modes);

	uint32_t desired_number_of_swapchain_images = surface_capabilities.minImageCount + 1;
	if ((surface_capabilities.maxImageCount > 0) && (desired_number_of_swapchain_images > surface_capabilities.maxImageCount))
	{
		desired_number_of_swapchain_images = surface_capabilities.maxImageCount;
	}

	VkSurfaceTransformFlagBitsKHR pre_transform;
	if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		pre_transform = surface_capabilities.currentTransform;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info;
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext = NULL;
	swapchain_create_info.flags = 0;
	swapchain_create_info.surface = surface_khr;
	swapchain_create_info.minImageCount = desired_number_of_swapchain_images;
	swapchain_create_info.imageFormat = COLOR_FORMAT;
	swapchain_create_info.imageExtent.width = swapchain_extent.width;
	swapchain_create_info.imageExtent.height = swapchain_extent.height;
	swapchain_create_info.preTransform = pre_transform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.presentMode = swapchain_present_mode;
	swapchain_create_info.oldSwapchain = NULL;
	swapchain_create_info.clipped = true;
	swapchain_create_info.imageColorSpace = color_space;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = NULL;

	err = vkCreateSwapchainKHR(VK_DEVICE, &swapchain_create_info, NULL, swapchain_khr);
	FailMessage(err, L"failed 'vkCreateSwapchainKHR'");

}

void FlushSetupCommandBuffer(VkCommandPool command_pool, VkCommandBuffer *setup_command_buffer)
{
	VkResult err;

	if (setup_command_buffer == VK_NULL_HANDLE)
		return;

	err = vkEndCommandBuffer(*setup_command_buffer);
	FailMessage(err, L"failed 'vkEndCommandBuffer'");

	VkSubmitInfo submitInfo = {};
	
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = setup_command_buffer;
	
	err = vkQueueSubmit(QUEUE, 1, &submitInfo, VK_NULL_HANDLE);
	FailMessage(err, L"failed 'vkQueueSubmit'");

	err = vkQueueWaitIdle(QUEUE);
	FailMessage(err, L"failed 'vkQueueWaitIdle'");

	vkFreeCommandBuffers(VK_DEVICE, command_pool, 1, setup_command_buffer);
	*setup_command_buffer = VK_NULL_HANDLE;
}

void CreateSetupCommandBuffer(VkCommandPool command_pool, VkCommandBuffer *setup_command_buffer)
{
	VkResult err;

	VkCommandBufferAllocateInfo command_buffer_allocate_info;
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.pNext = NULL;
	command_buffer_allocate_info.commandPool = command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = 1;

	err = vkAllocateCommandBuffers(VK_DEVICE, &command_buffer_allocate_info, setup_command_buffer);
	FailMessage(err, L"failed 'vkAllocateCommandBuffers'");


	VkCommandBufferBeginInfo command_buffer_begin_info;
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.pNext = NULL;
	command_buffer_begin_info.flags = 0;
	command_buffer_begin_info.pInheritanceInfo = NULL; //&cmd_buffer_inheritance_info;

	err = vkBeginCommandBuffer(*setup_command_buffer, &command_buffer_begin_info);
	FailMessage(err, L"failed 'vkBeginCommandBuffer'");

}

void CreateCommandPool(VkCommandPool *command_pool, uint32_t queue_family_index)
{
	vkGetDeviceQueue(VK_DEVICE, queue_family_index, 0, &QUEUE);

	VkCommandPoolCreateInfo command_pool_create_info;
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.pNext = NULL;
	command_pool_create_info.queueFamilyIndex = queue_family_index;
	command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult err = vkCreateCommandPool(VK_DEVICE, &command_pool_create_info, NULL, command_pool);
	FailMessage(err, L"failed 'vkCreateCommandPool'");

}

void CreateVulkanDevice(VkPhysicalDevice physical_device, uint32_t queue_family_index)
{	
	VkDeviceQueueCreateInfo device_queue_create_info;
	float queue_priorities[1] = { 0.0f };
	device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	device_queue_create_info.pNext = NULL;
	device_queue_create_info.flags = 0;
	device_queue_create_info.queueFamilyIndex = queue_family_index;
	device_queue_create_info.queueCount = 1;
	device_queue_create_info.pQueuePriorities = queue_priorities;

	VkDeviceCreateInfo device_create_info;
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext = NULL;
	device_create_info.flags = 0;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &device_queue_create_info;
	device_create_info.enabledExtensionCount = 0;
	device_create_info.ppEnabledExtensionNames = NULL;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = NULL;
	device_create_info.pEnabledFeatures = NULL;
	
	VkResult err = vkCreateDevice(physical_device, &device_create_info, NULL, &VK_DEVICE);
	FailMessage(err, L"failed 'vkCreateDevice'");

}

void FindQueueFamilyIndex(VkPhysicalDevice physical_device, VkSurfaceKHR surface_khr, uint32_t *queue_family_index)
{
	uint32_t queue_count;
	VkQueueFamilyProperties *queue_properties;

	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, NULL);
	FailMessage(queue_count < 1, L"failed 'vkGetPhysicalDeviceQueueFamilyProperties'");

	queue_properties = (VkQueueFamilyProperties *)malloc(queue_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_count, queue_properties);

	VkBool32 *supports_present = (VkBool32 *)malloc(queue_count * sizeof(VkBool32));
	for (uint32_t i = 0; i < queue_count; i++)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface_khr, &supports_present[i]);
	}

	*queue_family_index = UINT32_MAX;
	for (uint32_t i = 0; i < queue_count; i++)
	{
		if ((queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			if (supports_present[i] == VK_TRUE)
			{
				*queue_family_index = i;
				break;
			}
		}
	}
	free(supports_present);
	free(queue_properties);
	

	FailMessage(*queue_family_index == UINT32_MAX, L"Could not find a queue");

}

void CreateSurfaceKHR(HINSTANCE hinstance, HWND hwnd, VkInstance vk_instance, VkSurfaceKHR *surface_khr)
{
	VkWin32SurfaceCreateInfoKHR surface_create_Info;
	surface_create_Info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create_Info.pNext = NULL;
	surface_create_Info.flags = 0;
	surface_create_Info.hinstance = hinstance;
	surface_create_Info.hwnd = hwnd;

	VkResult err = vkCreateWin32SurfaceKHR(vk_instance, &surface_create_Info, NULL, surface_khr);
	FailMessage(err, L"failed 'vkCreateWin32SurfaceKHR'");
}

VkBool32 getSupportedDepthFormat(VkPhysicalDevice physical_device)
{
	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format
	std::vector<VkFormat> depthFormats = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	for (auto& format : depthFormats)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &formatProps);
		// Format must support depth stencil attachment for optimal tiling
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			DEPTH_FORMAT = format;
			return true;
		}
	}

	return false;
}

void EnumeratePhysicalDevices(VkInstance vk_instance, VkPhysicalDevice *physical_device)
{
	uint32_t gpu_count;

	vkEnumeratePhysicalDevices(vk_instance, &gpu_count, NULL);
	FailMessage(gpu_count < 1, L"failed 'vkEnumeratePhysicalDevices'");

	VkPhysicalDevice *physical_devices_list = (VkPhysicalDevice *)malloc(sizeof(VkPhysicalDevice) * gpu_count);
	VkResult err = vkEnumeratePhysicalDevices(vk_instance, &gpu_count, physical_devices_list);
	FailMessage(err, L"failed 'vkEnumeratePhysicalDevices'");

	*physical_device = physical_devices_list[0];
	free(physical_devices_list);


	vkGetPhysicalDeviceMemoryProperties(*physical_device, &MEMORY_PROPERTIES);   //
	getSupportedDepthFormat(*physical_device);
	
}

void CreateVulkanInstance(VkInstance *vk_instance)
{
	VkResult err;

	VkApplicationInfo application_info;
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pNext = NULL;
	application_info.pApplicationName = APP_NAME;
	application_info.applicationVersion = 1;
	application_info.pEngineName = APP_NAME;
	application_info.engineVersion = 1;
	application_info.apiVersion = VK_API_VERSION;

	VkInstanceCreateInfo instance_create_info;
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.flags = 0;
	instance_create_info.pApplicationInfo = &application_info;
	instance_create_info.enabledExtensionCount = 0;
	instance_create_info.ppEnabledExtensionNames = NULL;
	instance_create_info.enabledLayerCount = 0;
	instance_create_info.ppEnabledLayerNames = NULL;

	err = vkCreateInstance(&instance_create_info, NULL, vk_instance);
	FailMessage(err == VK_ERROR_INCOMPATIBLE_DRIVER, L"cannot find a compatible Vulkan ICD");
	FailMessage(err, L"failed 'vkCreateInstance'");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)   //윈도우 프로시저
{
	switch (uMsg)
	{
	case WM_CREATE:
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	default:
		break;
	}
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void CreateDisplayWindow(HINSTANCE hinstance, HWND *hwnd)   // 윈도우 생성
{
	WNDCLASSEX wnd_class;

	wnd_class.cbSize = sizeof(WNDCLASSEX);
	wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wnd_class.lpfnWndProc = WndProc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = hinstance;
	wnd_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd_class.hbrBackground = NULL;
	wnd_class.lpszMenuName = NULL;
	wnd_class.lpszClassName = WINDOW_NAME;
	wnd_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	FailMessage(!RegisterClassEx(&wnd_class), L"failed 'RegisterClassEx'");
	
	*hwnd = CreateWindowEx(
		0, WINDOW_NAME, WINDOW_NAME,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, hinstance, NULL);

	FailMessage(!(*hwnd), L"failed 'CreateWindowEx'");
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	bool quit;

	HINSTANCE hinstance = hInstance;
	HWND hwnd;
	VkInstance vk_instance;
	VkPhysicalDevice physical_device;
	uint32_t queue_family_index;
	VkSurfaceKHR surface_khr;
	VkCommandPool command_pool;
	VkCommandBuffer setup_command_buffer;
	VkSwapchainKHR swapchain_khr;
	SwapchainBuffer *swapchain_buffers;
	VkCommandBuffer *draw_command_buffers;
	DepthBuffer depth_buffer;
	VkRenderPass render_pass;
	VkFramebuffer *frame_buffers;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;

	CreateDisplayWindow(hinstance, &hwnd);
	CreateVulkanInstance(&vk_instance);
	EnumeratePhysicalDevices(vk_instance, &physical_device);
	CreateSurfaceKHR(hinstance, hwnd, vk_instance, &surface_khr);
	FindQueueFamilyIndex(physical_device, surface_khr, &queue_family_index);
	CreateVulkanDevice(physical_device, queue_family_index);
	CreateCommandPool(&command_pool, queue_family_index);
	CreateSetupCommandBuffer(command_pool, &setup_command_buffer);
	InitializeSwapchainKHR(physical_device, surface_khr, &swapchain_khr);
	CreateSwapchainBuffers(swapchain_khr, setup_command_buffer, &swapchain_buffers);
	CreateDrawCommandBuffers(command_pool, &draw_command_buffers);
	CreateDepthBuffer(setup_command_buffer, &depth_buffer);
	InitializeRenderPass(&render_pass);
	InitializeFrameBuffer(render_pass, swapchain_buffers, depth_buffer, &frame_buffers);

	InitializeVertexBuffer();

	float aspect_ratio = (float)WINDOW_WIDTH / WINDOW_HEIGHT;
	mvp.projection_matrix = PerspectiveProjection(aspect_ratio, 55.0f*3.14f/180.0f, 1.0f, 700.0f);
	//mvp.projection_matrix = OrthographicProjection(-10.0f, 10.0f, -10.0f, 10.0f, 0.5f, 100.0f);
	mvp.view_matrix = SimpleView(0.0f, 120.0f, 00.0f, 0.0f, 0.0f);
	mvp.model_matrix = Matrix4Scale(50.0f,50.0f,50.0f);
	Matrix4 transform = mvp.model_matrix * mvp.view_matrix * mvp.projection_matrix * AdjustCoordinate();
	CreateUniformBuffer(&transform, sizeof(Matrix4), &uniform_buffer);


	CreateDescriptorSetLayout(&descriptor_set_layout);
	CreatePipelineLayout(&descriptor_set_layout, &pipeline_layout);
	InitializePipeline(render_pass, pipeline_layout, &pipeline);

	CreateDescriptorPool(&descriptor_pool);
	AllocateDescriptorSets(descriptor_pool, &descriptor_set_layout, &descriptor_set);
	buildCommandBuffers(render_pass, pipeline_layout, pipeline, &descriptor_set, swapchain_buffers, draw_command_buffers, frame_buffers);

	quit = false;
	while (!quit)
	{
		PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		if (msg.message == WM_QUIT)
		{
			quit = true;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		draw(&swapchain_khr, swapchain_buffers, draw_command_buffers);
		Sleep(10);
	}

	vkFreeDescriptorSets(VK_DEVICE, descriptor_pool, 1, &descriptor_set);
	vkDestroyDescriptorPool(VK_DEVICE, descriptor_pool, NULL);
	vkDestroyPipeline(VK_DEVICE, pipeline, NULL);
	vkDestroyPipelineLayout(VK_DEVICE, pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(VK_DEVICE, descriptor_set_layout, NULL);

	for (uint32_t i = 0;i < SWAPCHAIN_IMAGE_COUNT;i++)
	{
		vkDestroyFramebuffer(VK_DEVICE, frame_buffers[i], NULL);
	}
	free(frame_buffers);

	vkDestroyRenderPass(VK_DEVICE, render_pass, NULL);
	vkDestroyImage(VK_DEVICE, depth_buffer.image, NULL);
	vkDestroyImageView(VK_DEVICE, depth_buffer.view, NULL);

	vkFreeCommandBuffers(VK_DEVICE, command_pool, SWAPCHAIN_IMAGE_COUNT, draw_command_buffers);
	free(draw_command_buffers);
	for (uint32_t i = 0;i < SWAPCHAIN_IMAGE_COUNT;i++)
	{
		vkDestroyImage(VK_DEVICE, swapchain_buffers[i].image, NULL);
		vkDestroyImageView(VK_DEVICE, swapchain_buffers[i].view,NULL);
	}
	free(swapchain_buffers);

	vkFreeCommandBuffers(VK_DEVICE, command_pool, 1, &setup_command_buffer);
	vkDestroyCommandPool(VK_DEVICE, command_pool, NULL);
	vkDestroyDevice(VK_DEVICE, NULL);
	//vkDestroySwapchainKHR(VK_DEVICE, swapchain_khr, NULL);

	vkDestroySurfaceKHR(vk_instance, surface_khr, NULL);   
	vkDestroyInstance(vk_instance, NULL);

	DestroyWindow(hwnd);
	UnregisterClass(WINDOW_NAME, hInstance);

	return (int)msg.wParam;
}