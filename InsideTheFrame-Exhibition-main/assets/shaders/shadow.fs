// =============================================================================
//  shadow.fs — Depth-only fragment shader for shadow map generation
//  No color output needed — depth buffer is written automatically.
// =============================================================================
#version 330 core

// Explicitly no outputs — we only write to the depth buffer via FBO.
// On some drivers an explicit void discard helps avoid driver warnings.
void main()
{
    // gl_FragDepth = gl_FragCoord.z;  // written automatically
}
