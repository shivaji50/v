cl.exe /c /EHsc /I C:\VulkanSDK\Vulkan\Include VK.c
rc.exe VK.rc
link.exe VK.obj VK.res /LIBPATH:C:\VulkanSDK\Vulkan\Lib user32.lib gdi32.lib /SUBSYSTEM:WINDOWS
