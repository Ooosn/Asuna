#include "tracer.h"

#include <backends/imgui_impl_glfw.h>
#include <nvvk/context_vk.hpp>
#include <nvvk/images_vk.hpp>
#include <nvvk/structs_vk.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <iostream>

using std::filesystem::path;

// **************************************************************************
// functions exposed to main.cpp
// **************************************************************************
void Tracer::init(TracerInitState tis)
{
	m_tis = tis;

	m_context.init({m_tis.m_offline});

	m_scene.init(&m_context);
	m_scene.create(m_tis.m_scenefile);

	m_context.m_size = m_scene.getSensorSize();
	if (!m_tis.m_offline)
		m_context.resizeGlfwWindow();
	else
		m_context.createOfflineResources();

	PipelineCorrelated *pPipCorrGraphic = new PipelineCorrelatedRaytrace;
	pPipCorrGraphic->m_pContext         = &m_context;
	pPipCorrGraphic->m_pScene           = &m_scene;
	m_pipelineGraphics.init(pPipCorrGraphic);

	PipelineCorrelatedRaytrace *pPipCorrRaytrace =
	    (PipelineCorrelatedRaytrace *) pPipCorrGraphic;
	pPipCorrRaytrace->m_pPipGraphics = &m_pipelineGraphics;
	m_pipelineRaytrace.init(pPipCorrRaytrace);

	PipelineCorrelatedPost *pPipCorrPost = (PipelineCorrelatedPost *) pPipCorrRaytrace;
	m_pipelinePost.init(pPipCorrPost);

	delete pPipCorrGraphic;
}

void Tracer::run()
{
	if (m_tis.m_offline)
		runOffline();
	else
		runOnline();
}

void Tracer::deinit()
{
	m_pipelineGraphics.deinit();
	m_pipelineRaytrace.deinit();
	m_pipelinePost.deinit();
	m_scene.deinit();
	m_context.deinit();
}

void Tracer::runOnline()
{
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color        = {0.0f, 0.0f, 0.0f, 0.0f};
	clearValues[1].depthStencil = {1.0f, 0};
	// Main loop
	while (!glfwWindowShouldClose(m_context.m_glfw))
	{
		glfwPollEvents();
		if (m_context.isMinimized())
			continue;
		// Start the Dear ImGui frame
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		// Start rendering the scene
		m_context.prepareFrame();
		// Start command buffer of this frame
		uint32_t                 curFrame  = m_context.getCurFrame();
		const VkCommandBuffer   &cmdBuf    = m_context.getCommandBuffers()[curFrame];
		VkCommandBufferBeginInfo beginInfo = nvvk::make<VkCommandBufferBeginInfo>();
		beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		{
			vkBeginCommandBuffer(cmdBuf, &beginInfo);
			{
				m_pipelineGraphics.run(cmdBuf);
			}
			{
				m_pipelineRaytrace.run(cmdBuf);
			}
			{
				VkRenderPassBeginInfo postRenderPassBeginInfo{
				    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
				postRenderPassBeginInfo.clearValueCount = 2;
				postRenderPassBeginInfo.pClearValues    = clearValues.data();
				postRenderPassBeginInfo.renderPass      = m_context.getRenderPass();
				postRenderPassBeginInfo.framebuffer     = m_context.getFramebuffer(curFrame);
				postRenderPassBeginInfo.renderArea      = {{0, 0}, m_context.getSize()};

				// Rendering to the swapchain framebuffer the rendered image and apply a
				// tonemapper
				vkCmdBeginRenderPass(cmdBuf, &postRenderPassBeginInfo,
				                     VK_SUBPASS_CONTENTS_INLINE);

				m_pipelinePost.run(cmdBuf);

				// Rendering UI
				ImGui::Render();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);

				// Display axis in the lower left corner.
				// vkAxis.display(cmdBuf, CameraManip.getMatrix(), vkSample.getSize());

				vkCmdEndRenderPass(cmdBuf);
			}
		}
		vkEndCommandBuffer(cmdBuf);
		m_context.submitFrame();
	}
	vkDeviceWaitIdle(m_context.getDevice());
}

void Tracer::runOffline()
{
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color        = {0.0f, 0.0f, 0.0f, 0.0f};
	clearValues[1].depthStencil = {1.0f, 0};

	nvvk::CommandPool genCmdBuf(m_context.getDevice(), m_context.getQueueFamily());

	// Main loop
	int spp = 100;
	while (spp-- > 0)
	{
		const VkCommandBuffer &cmdBuf = genCmdBuf.createCommandBuffer();
		{
			m_pipelineGraphics.run(cmdBuf);
		}
		{
			m_pipelineRaytrace.run(cmdBuf);
		}
		{
			VkRenderPassBeginInfo postRenderPassBeginInfo{
			    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			postRenderPassBeginInfo.clearValueCount = 2;
			postRenderPassBeginInfo.pClearValues    = clearValues.data();
			postRenderPassBeginInfo.renderPass      = m_context.getRenderPass();
			postRenderPassBeginInfo.framebuffer     = m_context.getFramebuffer();
			postRenderPassBeginInfo.renderArea      = {{0, 0}, m_context.getSize()};

			// Rendering to the swapchain framebuffer the rendered image and apply a tonemapper
			vkCmdBeginRenderPass(cmdBuf, &postRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			m_pipelinePost.run(cmdBuf);

			vkCmdEndRenderPass(cmdBuf);
		}
		genCmdBuf.submitAndWait(cmdBuf);
	}
	vkDeviceWaitIdle(m_context.getDevice());
	saveImage(m_tis.m_outputname);
}

void Tracer::imageToBuffer(const nvvk::Texture &imgIn, const VkBuffer &pixelBufferOut)
{
	nvvk::CommandPool genCmdBuf(m_context.getDevice(), m_context.getQueueFamily());
	VkCommandBuffer   cmdBuf = genCmdBuf.createCommandBuffer();

	// Make the image layout eTransferSrcOptimal to copy to buffer
	nvvk::cmdBarrierImageLayout(cmdBuf, imgIn.image, VK_IMAGE_LAYOUT_GENERAL,
	                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

	// Copy the image to the buffer
	VkBufferImageCopy copyRegion;
	copyRegion.imageSubresource  = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
	copyRegion.imageExtent       = {m_context.getSize().width, m_context.getSize().height, 1};
	copyRegion.imageOffset       = {0};
	copyRegion.bufferOffset      = 0;
	copyRegion.bufferImageHeight = m_context.getSize().height;
	copyRegion.bufferRowLength   = m_context.getSize().width;
	vkCmdCopyImageToBuffer(cmdBuf, imgIn.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                       pixelBufferOut, 1, &copyRegion);

	// Put back the image as it was
	nvvk::cmdBarrierImageLayout(cmdBuf, imgIn.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	                            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
	genCmdBuf.submitAndWait(cmdBuf);
}

void Tracer::saveImage(std::string outputpath)
{
	bool isRelativePath = path(outputpath).is_relative();
	if (isRelativePath)
		outputpath = NVPSystem::exePath() + outputpath;

	auto &m_alloc = m_context.m_alloc;
	auto  m_size  = m_context.getSize();

	// Create a temporary buffer to hold the pixels of the image
	VkBufferUsageFlags usage{VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
	                         VK_BUFFER_USAGE_TRANSFER_DST_BIT};
	VkDeviceSize       bufferSize = 4 * sizeof(float) * m_size.width * m_size.height;
	nvvk::Buffer       pixelBuffer =
	    m_context.m_alloc.createBuffer(bufferSize, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	imageToBuffer(m_context.getOfflineFramebufferTexture(), pixelBuffer.buffer);

	// Write the buffer to disk
	void *data = m_alloc.map(pixelBuffer);
	stbi_write_hdr(outputpath.c_str(), m_size.width, m_size.height, 4,
	               reinterpret_cast<float *>(data));
	m_alloc.unmap(pixelBuffer);

	// Destroy temporary buffer
	m_alloc.destroy(pixelBuffer);
}
