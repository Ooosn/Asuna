#pragma once

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <nvvk/appbase_vk.hpp>
#include <nvvk/context_vk.hpp>
#include <nvvk/debug_util_vk.hpp>
#include <nvvk/gizmos_vk.hpp>
#include <nvvk/resourceallocator_vk.hpp>
#include <nvvk/structs_vk.hpp>

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

using std::array;
using std::string;
using std::vector;

struct ContextInitSetting {
  bool offline{false};
};

class ContextAware : public nvvk::AppBaseVk {
public:
  void init(ContextInitSetting cis);
  void deinit();
  void setViewport(const VkCommandBuffer& cmdBuf);

  // Online mode: resize glfw window according to ContextAware::getSize()
  void resizeGlfwWindow();

  // Online mode: whether glfw has been closed
  bool shouldGlfwCloseWindow();

public:
  // Set window size
  void setSize(VkExtent2D& size);

  // Get window size
  VkExtent2D& getSize();

  // Get vulkan resource allocator
  nvvk::ResourceAllocatorDedicated& getAlloc();

  // Get vulkan debugger
  nvvk::DebugUtil& getDebug();

  // If in offline mode, return true
  bool getOfflineMode();

  // Path of exectuable program
  string& getRoot();

  // Offline rgba32f buffer(ldr)
  nvvk::Texture getOfflineColor();

  // Offline depth buffer(only used for renderpass)
  nvvk::Texture getOfflineDepth();

  // Offline framebuffer(attachment: offlineColor and offlineDepth)
  VkFramebuffer getFramebuffer(int curFrame = 0);

  // Offline render pass
  VkRenderPass getRenderPass();

private:
  void createGlfwWindow();
  void initializeVulkan();
  void createAppContext();
  void createOfflineResources();

private:
  VkRenderPass m_offlineRenderPass{VK_NULL_HANDLE};
  VkFramebuffer m_offlineFramebuffer{VK_NULL_HANDLE};
  nvvk::Texture m_offlineColor;
  nvvk::Texture m_offlineDepth;

private:
  ContextInitSetting m_cis;
  nvvk::ResourceAllocatorDedicated m_alloc;
  nvvk::DebugUtil m_debug;
  nvvk::Context m_vkcontext{};
  std::string m_root{};
};