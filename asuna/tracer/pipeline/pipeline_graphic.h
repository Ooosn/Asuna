#pragma once

#include "pipeline.h"
#include "../../assets/hostdevice/pushconstant.h"

class PipelineGraphic : public PipelineAware
{
public:
    virtual void init(PipelineCorrelated* pPipCorr);
    virtual void run(const VkCommandBuffer& cmdBuf) {};
    virtual void deinit();
public:
    // Canvas we draw things on
    nvvk::Texture m_tColor;
    // Depth buffer
    nvvk::Texture m_tDepth;
private:
    VkRenderPass  m_offscreenRenderPass{ VK_NULL_HANDLE };
    VkFramebuffer m_offscreenFramebuffer{ VK_NULL_HANDLE };
    // Record unprocessed command when interruption happen
    // -- used by raster to replay rendering commands
    VkCommandBuffer m_recordedCmdBuffer{ VK_NULL_HANDLE };
    // Device-Host of the camera matrices
    nvvk::Buffer m_bCamera;
    // Push constant
    PushConstantGraphic m_pcGraphic{ 0 };
private:
    // Creating an offscreen frame buffer and the associated render pass
    void createOffscreenResources();
    // Describing the layout pushed when rendering
    void createGraphicDescriptorSetLayout();
    // Create graphic pipeline with vertex and fragment shader
    void createGraphicPipeline();
    // Creating the uniform buffer holding the camera matrices
    void createCameraBuffer();
    // Setting up the buffers in the descriptor set
    void updateGraphicDescriptorSet();
};