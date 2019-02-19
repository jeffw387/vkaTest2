#include <memory>
#include "logger.hpp"
#include "platform_glfw.hpp"
#include "vka.hpp"

using namespace platform;
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
  auto surfacePtr =
    vka::surface_builder{}
      .width(900)
      .height(900)
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
  auto swapchainPtr =
    vka::swapchain_builder{}
      .queue_family_index(queueFamily.familyIndex)
      .present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .image_format(VK_FORMAT_B8G8R8A8_UNORM)
      .image_color_space(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      .image_count(3)
      .build(physicalDevice, *surfacePtr, *devicePtr)
      .map_error([](auto error) {
        multi_logger::get()->critical("Failed to create vulkan swapchain!");
        exit(error);
      })
      .value();
  
  return 0;
}