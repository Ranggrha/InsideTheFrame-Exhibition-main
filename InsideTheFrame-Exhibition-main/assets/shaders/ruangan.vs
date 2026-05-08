// =============================================================================
//  ruangan.vs — Room vertex shader (Advanced Lighting version)
//  Adds: FragPosLightSpace for PCF shadow mapping, time passthrough
// =============================================================================
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2  TexCoord;
out vec3  FragPos;
out vec3  Normal;
out vec4  FragPosLightSpace;   // position in light-clip space for shadow test

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix; // orthographic light VP matrix
uniform int  surfaceType;      // 0=lantai, 1=tembok, 2=atap

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);

    // -----------------------------------------------------------------------
    //  Normal derivation per surface type (no normal attribute in VBO)
    // -----------------------------------------------------------------------
    vec3 localNormal;

    if (surfaceType == 0)
    {
        // Lantai — normal menghadap ke atas
        localNormal = vec3(0.0, 1.0, 0.0);
    }
    else if (surfaceType == 2)
    {
        // Atap — normal menghadap ke bawah
        localNormal = vec3(0.0, -1.0, 0.0);
    }
    else
    {
        // Tembok — deteksi dominansi sumbu X vs Z
        float ax = abs(aPos.x);
        float az = abs(aPos.z);
        if (az >= ax)
            localNormal = vec3(0.0, 0.0, -sign(aPos.z));
        else
            localNormal = vec3(-sign(aPos.x), 0.0, 0.0);
    }

    Normal           = mat3(transpose(inverse(model))) * localNormal;
    TexCoord         = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;
    gl_Position      = projection * view * worldPos;
}