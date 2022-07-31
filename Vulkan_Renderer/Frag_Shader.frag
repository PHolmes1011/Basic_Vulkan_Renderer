#version 450

layout(binding = 1) uniform sampler2D texSampler;

// Matching input sent from the vertex shader
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // Just set the output to this colour 
    outColor = texture(texSampler, fragTexCoord);
}