// =============================================================================
//  papan.vs — Exhibition board vertex shader
//  Updated: UV passthrough + per-vertex normal (from geometry attrib)
//
//  Vertex layout (ExhibitionBoard): pos(3) + normal(3) + uv(2)
// =============================================================================
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    vec4 worldPos     = model * vec4(aPos, 1.0);
    FragPos           = vec3(worldPos);
    Normal            = mat3(transpose(inverse(model))) * aNormal;
    TexCoord          = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position       = projection * view * worldPos;
}
