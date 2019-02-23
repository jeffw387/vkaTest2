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
      .add_extensions(
        glfw::get_required_instance_extensions())
      .add_layer(vka::standard_validation)
      .build()
      .map_error(
        err::crit{"Cannot create vulkan instance!"})
      .value();
  auto physicalDevice =
    vka::physical_device_selector{}
      .select(*instancePtr)
      .map_error(
        err::crit{"Can't find a suitable vulkan device!"})
      .value();
  constexpr auto surfaceWidth = 900;
  constexpr auto surfaceHeight = 900;
  auto surfacePtr = vka::surface_builder{}
      .width(surfaceWidth)
      .height(surfaceHeight)
      .title("vkaTest2")
      .build(*instancePtr)
                      .map_error(err::crit{
                        "Unable to create vulkan surface!"})
      .value();
  auto queueFamily =
    vka::queue_family_builder{}
                       .graphics_support()
                       .present_support(*surfacePtr)
                       .queue(1.f)
                       .build(physicalDevice)
      .map_error(err::crit{
        "Unable to find a suitable queue family!"})
                       .value();
  auto devicePtr = vka::device_builder{}
      .extension(vka::swapchain_extension)
      .physical_device(physicalDevice)
      .add_queue_family(queueFamily)
      .build(*instancePtr)
                     .map_error(err::crit{
                       "Failed to create vulkan device!"})
      .value();
  auto queue = vka::queue_builder{}
      .queue_info(queueFamily, 0)
      .build(*devicePtr)
                 .map_error(err::crit{
                   "Unable to retrieve device queue!"})
      .value();
  constexpr auto swapFormat = VK_FORMAT_B8G8R8A8_UNORM;
  constexpr auto swapColorSpace =
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  uint32_t swapImageCount{3};
  auto swapchainPtr =
    vka::swapchain_builder{}
      .queue_family_index(queueFamily.familyIndex)
      .present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .image_format(swapFormat)
      .image_color_space(swapColorSpace)
      .image_count(swapImageCount)
      .build(physicalDevice, *surfacePtr, *devicePtr)
      .map_error(
        err::crit{"Failed to create vulkan swapchain!"})
      .value();

  auto vertexShaderData =
    vka::make_shader<jshd::vertex_shader_data>(
      *devicePtr, "shader.vert")
      .map_error(err::crit{
        "Unable to create shader module shader.vert!"})
      .value();
  auto fragmentShaderData =
    vka::make_shader<jshd::fragment_shader_data>(
      *devicePtr, "shader.frag")
      .map_error(err::crit{
        "Unable to create shader module shader.frag!"})
      .value();
  auto cmdPoolPtr =
    vka::command_pool_builder{}
      .queue_family_index(queueFamily.familyIndex)
      .build(*devicePtr)
      .map_error(
        err::crit{"Unable to create command pool!"})
      .value();
  std::array<std::unique_ptr<vka::command_buffer>, 3>
    cmdPtrs;
  std::for_each(
    std::begin(cmdPtrs),
    std::end(cmdPtrs),
    [&devicePtr, &cmdPoolPtr](auto& cmdPtr) {
      cmdPtr = vka::command_buffer_allocator{}
      .set_command_pool(cmdPoolPtr.get())
      .level(VK_COMMAND_BUFFER_LEVEL_PRIMARY)
      .allocate(*devicePtr)
                 .map_error(err::crit{
                   "Unable to allocate command buffer!"})
      .value();
    });
  auto pipelineLayoutPtr = vka::make_pipeline_layout(
    *devicePtr, vertexShaderData, fragmentShaderData, {});
  auto renderPassPtr =
    vka::render_pass_builder{}
      .add_attachment(
        vka::attachment_builder{}
          .initial_layout(
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .final_layout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
          .format(swapFormat)
          .loadOp(VK_ATTACHMENT_LOAD_OP_CLEAR)
          .storeOp(VK_ATTACHMENT_STORE_OP_STORE)
          .samples(VK_SAMPLE_COUNT_1_BIT)
          .build())
      .add_subpass(
        vka::subpass_builder{}
          .color_attachment(
            0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
          .build())
      .build(*devicePtr)
      .map_error(err::crit{"Error creating render pass!"})
      .value();
  auto blendState =
    vka::make_blend_state({vka::make_blend_attachment(
      vka::no_blend_attachment{})});
  auto depthStencilState =
    vka::make_depth_stencil_state(false, false);
  auto dynamicState = vka::make_dynamic_state();
  auto inputAssemblyState = vka::make_input_assembly(
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  VkViewport viewport{};
  viewport.width = static_cast<float>(surfaceWidth);
  viewport.height = static_cast<float>(surfaceHeight);
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;

  VkRect2D scissor{};
  scissor.extent.width =
    static_cast<uint32_t>(surfaceWidth);
  scissor.extent.height =
    static_cast<uint32_t>(surfaceHeight);
  auto viewportState =
    vka::make_viewport_state({viewport}, {scissor});
  auto rasterizationState = vka::make_rasterization_state();
  auto multisampleState = vka::make_multisample_state();
  auto vertexState =
    vka::make_vertex_state(vertexShaderData.shaderData);
  auto vertexStageState =
    vka::make_shader_stage(vertexShaderData, "main", {});
  auto fragmentStageState =
    vka::make_shader_stage(fragmentShaderData, "main", {});
  auto pipelinePtr = vka::make_pipeline(
    *devicePtr,
    *renderPassPtr,
    0,
    *pipelineLayoutPtr,
    VK_NULL_HANDLE,
    blendState,
    depthStencilState,
    dynamicState,
    inputAssemblyState,
    viewportState,
    rasterizationState,
    multisampleState,
    vertexState,
    vertexStageState,
    fragmentStageState);
  auto allocator =
    vka::allocator_builder{}
      .physical_device(physicalDevice)
      .device(*devicePtr)
      .preferred_block_size(1 << 5)
      .build()
      .map_error(
        err::crit{"Unable to create vulkan allocator!"})
      .value();
  std::array<glm::vec2, 3> positions = {
    glm::vec2{0.f, -0.5f},
    glm::vec2{-0.5, 0.5f},
    glm::vec2{0.5f, 0.5f}};
  auto posBuffer =
    vka::buffer_builder{}
      .cpu_to_gpu()
      .vertex_buffer()
      .queue_family_index(queueFamily.familyIndex)
      .size(sizeof(positions))
      .build(*allocator)
      .map_error(err::crit{
        "Unable to create vertex position buffer!"})
      .value();
  auto posBufferPtr =
    posBuffer->map()
      .map_error(
        err::crit{"Unable to map vertex position buffer!"})
      .value();
  std::memcpy(
    posBufferPtr, positions.data(), sizeof(positions));

  std::array<glm::vec4, 3> colors = {
    glm::vec4{1.f, 0.f, 0.f, 1.f},
    glm::vec4{0.f, 1.f, 0.f, 1.f},
    glm::vec4{0.f, 0.f, 1.f, 1.f}};
  auto colorBuffer =
    vka::buffer_builder{}
      .cpu_to_gpu()
      .vertex_buffer()
      .queue_family_index(queueFamily.familyIndex)
      .size(sizeof(colors))
      .build(*allocator)
      .map_error(
        err::crit{"Unable to create vertex color buffer!"})
      .value();
  auto colorBufferPtr =
    colorBuffer->map()
      .map_error(
        err::crit{"Unable to map vertex color buffer!"})
      .value();
  std::memcpy(colorBufferPtr, colors.data(), sizeof(colors));

  vmaFlushAllocation(*allocator, *posBuffer, 0, VK_WHOLE_SIZE);
  vmaFlushAllocation(*allocator, *colorBuffer, 0, VK_WHOLE_SIZE);

  auto flushCmdPtr = vka::command_buffer_allocator{}
    .set_command_pool(cmdPoolPtr.get())
    .allocate(*devicePtr)
    .map_error(err::crit{"Unable to allocate command buffer for vertex flush!"})
    .value();
  
  {
    VkCommandBufferBeginInfo flushBeginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    flushBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(*flushCmdPtr, &flushBeginInfo);
    std::array flushPrev{ThsvsAccessType::THSVS_ACCESS_HOST_WRITE};
    std::array flushNext{ThsvsAccessType::THSVS_ACCESS_VERTEX_BUFFER};
    ThsvsGlobalBarrier vertexBarrier{};
    vertexBarrier.prevAccessCount = vka::size32(flushPrev);
    vertexBarrier.pPrevAccesses = flushPrev.data();
    vertexBarrier.nextAccessCount = vka::size32(flushNext);
    vertexBarrier.pNextAccesses = flushNext.data();
    thsvsCmdPipelineBarrier(*flushCmdPtr, &vertexBarrier, {}, {}, {}, {});
    vkEndCommandBuffer(*flushCmdPtr);

    auto flushFencePtr = vka::fence_builder{}
      .build(*devicePtr)
      .map_error(err::crit{"Unable to create vertex flush fence!"})
      .value();
    VkFence flushFence = *flushFencePtr;
    
    VkSubmitInfo flushSubmit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkCommandBuffer flushCmd = *flushCmdPtr;
    flushSubmit.commandBufferCount = 1;
    flushSubmit.pCommandBuffers = &flushCmd;
    
    vkQueueSubmit(queue, 1, &flushSubmit, flushFence);
    vkWaitForFences(*devicePtr, 1, &flushFence, true, ~uint64_t{});
  }

  return 0;
}