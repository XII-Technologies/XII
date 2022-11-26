#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>

vk::Device                       xiiSemaphorePoolVulkan::s_device;
xiiHybridArray<vk::Semaphore, 4> xiiSemaphorePoolVulkan::s_semaphores;

void xiiSemaphorePoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void xiiSemaphorePoolVulkan::DeInitialize()
{
  for (vk::Semaphore& semaphore : s_semaphores)
  {
    s_device.destroySemaphore(semaphore, nullptr);
  }
  s_semaphores.Clear();
  s_semaphores.Compact();

  s_device = nullptr;
}

vk::Semaphore xiiSemaphorePoolVulkan::RequestSemaphore()
{
  XII_ASSERT_DEBUG(s_device, "xiiSemaphorePoolVulkan::Initialize not called");
  if (!s_semaphores.IsEmpty())
  {
    vk::Semaphore semaphore = s_semaphores.PeekBack();
    s_semaphores.PopBack();
    return semaphore;
  }
  else
  {
    vk::Semaphore           semaphore;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_ASSERT_DEV(s_device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
  }
}

void xiiSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore& semaphore)
{
  XII_ASSERT_DEBUG(s_device, "xiiSemaphorePoolVulkan::Initialize not called");
  s_semaphores.PushBack(semaphore);
}
