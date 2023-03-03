This is a basic renderer created from the Vulkan tutorial, by Peter Holmes.

The renderer requires:
- glfw-3.3.6.bin.WIN64			(For creating the window)
https://www.glfw.org/download.html

- glm-master				(A maths library)
https://github.com/g-truc/glm

- stb-master				(An image loader)
https://github.com/nothings/stb

- tinyobjloader				(A model loader)
https://github.com/tinyobjloader/tinyobjloader

- VulkanSDK				(And vulkan itself)
https://vulkan.lunarg.com/sdk/home#windows

All files should be stored externally to the solution file. For example:
../../stb-master
This should then work and allow the vulkan renderer to run.

Some files may be replaced with code of my own or included in the solution file 
in later versions to make it easier to clone and run.