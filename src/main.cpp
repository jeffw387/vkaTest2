#include <fmt/format.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include "logger.hpp"
#include "memory_allocator.hpp"
#include "platform_glfw.hpp"
#include "sync_helper.hpp"
#include "vka.hpp"

using namespace platform;

namespace err {
struct crit {
  std::string_view errMessage;
  crit(std::string_view errMessage = "Critical error!")
      : errMessage(errMessage) {}

  template <typename T>
  void operator()(T error) {
    multi_logger::get()->critical("{}", errMessage);
    exit(1);
  }

  void operator()(int error) {
    multi_logger::get()->critical("{}", errMessage);
    exit(error);
  }
};
}  // namespace err

int main() {
  auto instancePtr =
    vka::instance_builder{}
      .add_extensions(glfw::get_required_instance_extensions())
      .add_layer(vka::standard_validation)
      .build()
      .map_error([](auto error) {
        multi_logger::get()->critical("Cannot create vulkan instance!");
        exit(error);
      })
      .value();
  auto physicalDevice =
    vka::physical_device_selector{}
      .select(*instancePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Can't find a suitable vulkan device!");
        exit(1);
      })
      .value();
  constexpr auto surfaceWidth = 900;
  constexpr auto surfaceHeight = 900;
  auto surfacePtr =
    vka::surface_builder{}
      .width(surfaceWidth)
      .height(surfaceHeight)
      .title("vkaTest2")
      .build(*instancePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Unable to create vulkan surface!");
        exit(1);
      })
      .value();
  auto queueFamily = vka::queue_family_builder{}
                       .graphics_support()
                       .present_support(*surfacePtr)
                       .queue(1.f)
                       .build(physicalDevice)
                       .map_error([](auto error) {
                         multi_logger::get()->critical(
                           "Unable to find a suitable queue family!");
                       })
                       .value();
  auto devicePtr =
    vka::device_builder{}
      .extension(vka::swapchain_extension)
      .physical_device(physicalDevice)
      .add_queue_family(queueFamily)
      .build(*instancePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Failed to create vulkan device!");
        exit(error);
      })
      .value();
  auto queue =
    vka::queue_builder{}
      .queue_info(queueFamily, 0)
      .build(*devicePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Unable to retrieve device queue!");
        exit(1);
      })
      .value();
  constexpr auto swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
  constexpr auto swapColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  auto swapchainPtr =
    vka::swapchain_builder{}
      .queue_family_index(queueFamily.familyIndex)
      .present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .image_format(swapFormat)
      .image_color_space(swapColorSpace)
      .image_count(3)
      .build(physicalDevice, *surfacePtr, *devicePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Failed to create vulkan swapchain!");
        exit(error);
      })
      .value();

  auto vertexShaderData =
    vka::make_shader<jshd::vertex_shader_data>(*devicePtr, "shader.vert")
      .map_error([](auto error) {
        multi_logger::get()->critical(
          "Unable to create shader module shader.vert!");
        exit(1);
      })
      .value();
  auto fragmentShaderData =
    vka::make_shader<jshd::fragment_shader_data>(*devicePtr, "shader.frag")
      .map_error([](auto error) {
        multi_logger::get()->critical(
          "Unable to create shader module shader.frag!");
        exit(1);
      })
      .value();
  auto cmdPoolPtr =
    vka::command_pool_builder{}
      .queue_family_index(queueFamily.familyIndex)
      .build(*devicePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Unable to create command pool!");
        exit(error);
      })
      .value();
  auto cmdPtr =
    vka::command_buffer_allocator{}
      .set_command_pool(cmdPoolPtr.get())
      .level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
      .allocate(*devicePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Unable to allocate command buffer!");
        exit(error);
      })
      .value();
  auto pipelineLayoutPtr = vka::make_pipeline_layout(
    *devicePtr, vertexShaderData, fragmentShaderData, {});
  auto renderPassPtr =
    vka::render_pass_builder{}
      .add_attachment(
        vka::attachment_builder{}
          .initial_layout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .final_layout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
          .format(swapFormat)
          .loadOp(VK_ATTACHMENT_LOAD_OP_CLEAR)
          .storeOp(VK_ATTACHMENT_STORE_OP_STORE)
          .samples(VK_SAMPLE_COUNT_1_BIT)
          .build())
      .add_subpass(
        vka::subpass_builder{}
          .color_attachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .build())
      .build(*devicePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Error creating render pass!");
        exit(error);
      })
      .value();
  auto blendState = vka::make_blend_state(
    {vka::make_blend_attachment(vka::no_blend_attachment{})});
  auto depthStencilState = vka::make_depth_stencil_state(false, false);
  auto dynamicState = vka::make_dynamic_state();
  auto inputAssemblyState =
    vka::make_input_assembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  VkViewport viewport{};
  viewport.width = static_cast<float>(surfaceWidth);
  viewport.height = static_cast<float>(surfaceHeight);
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  VkRect2D scissor{};
  scissor.extent.width = static_cast<uint32_t>(surfaceWidth);
  scissor.extent.height = static_cast<uint32_t>(surfaceHeight);
  auto viewportState = vka::make_viewport_state({viewport}, {scissor});
  auto rasterizationState = vka::make_rasterization_state();
  auto vertexStageState = vka::make_shader_stage(vertexShaderData, "main", {});
  auto fragmentStageState =
    vka::make_shader_stage(fragmentShaderData, "main", {});
  auto vertexState = vka::make_vertex_state(vertexShaderData.shaderData);
  auto pipelinePtr = vka::make_pipeline(
    *devicePtr,
    *renderPassPtr,
    0,
    VK_NULL_HANDLE,
    blendState,
    depthStencilState,
    dynamicState,
    inputAssemblyState,
    viewportState,
    rasterizationState,
    vertexStageState,
    fragmentStageState,
    vertexState);
  return 0;
}