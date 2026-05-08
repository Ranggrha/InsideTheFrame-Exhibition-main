// =============================================================================
//  papan.vs — Exhibition board vertex shader (Lit version)
//  Computes per-vertex normal from face direction + passes light-space coord.
// =============================================================================
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;   // vertex color (white for board)

out vec3 VertColor;
out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    vec4 worldPos  = model * vec4(aPos, 1.0);
    FragPos        = vec3(worldPos);
    VertColor      = aColor;

    // Derive face normal from dominant position component
    // The board is a thin box scaled along X/Z — faces are axis-aligned.
    vec3 absPos = abs(aPos);
    vec3 localNormal;
    if (absPos.y >= absPos.x && absPos.y >= absPos.z)
        localNormal = vec3(0.0, sign(aPos.y), 0.0);
    else if (absPos.x >= absPos.z)
        localNormal = vec3(sign(aPos.x), 0.0, 0.0);
    else
        localNormal = vec3(0.0, 0.0, sign(aPos.z));

    Normal            = mat3(transpose(inverse(model))) * localNormal;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position       = projection * view * worldPos;
}
