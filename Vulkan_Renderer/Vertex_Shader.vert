// Specify the version of glsl
#version 450

#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// Vertex positions and colour
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColour;
layout(location = 2) in vec2 inTexCoord;

// Note - 64bit numbers take two slots

// "layout(location = 0)" specifies the index of the framebuffer
// The color is written to this outColor variable
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;


// The main function is invoked for every vertex
void main() 
{
    // Set the output position to the x y z position from the input
    gl_Position = ubo.proj * ubo.view* ubo.model * vec4(inPosition, 1.0);    // The w value is filled in

    // And set the output colour the same way
    fragColor = inColour;

    // UVs are the same too
    fragTexCoord = inTexCoord;
}