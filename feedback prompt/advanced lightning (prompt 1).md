# 🕵️ GPU Auto-Detection — at launch, prints [Lighting] Detected quality: HIGH/MEDIUM/… based on GL_MAX_TEXTURE_SIZE and extension checks
# 🌑 PCF Shadow Mapping — 512×512 depth FBO, 4-tap kernel, 0.005 bias (no acne), 60% max darkness
# 🌅 Time-of-Day Sun — 60-second demo cycle, warm amber sunrise → cool white noon → red-orange sunset
# 💡 8 Point Lights in Uniform Array — 7 ceiling strips + 2 amber walls + 1 cool accent, capped per quality tier
# 🕯️ 3 Falloff Presets — press L to cycle: Candlelight (tight pools) → Fluorescent (even wash) → Daylight (wide coverage)
# 🔲 Baked AO — analytic corner darkening at floor-wall-ceiling junctions, zero texture cost
# ✨ Emissive Ceiling — Gaussian glow halo around each strip light position
# 📉 Quality Step-Down — press Q to lower quality (debug); shadows auto-disable on MEDIUM/LOW/MINIMAL
# 📊 FPS Diagnostic — console prints FPS every 5 seconds with current quality and preset