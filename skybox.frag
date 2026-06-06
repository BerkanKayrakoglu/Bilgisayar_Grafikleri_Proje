#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform vec3 sunDir;       // Normalized direction to the sun
uniform vec3 moonDir;      // Normalized direction to the moon
uniform vec3 zenithColor;  // Color of the sky directly overhead
uniform vec3 horizonColor; // Color of the sky at the horizon
uniform vec3 sunColor;     // Base color of the sun glow
uniform vec3 moonColor;    // Base color of the moon glow
uniform float cycleValue;  // 1.0 for day, 0.0 for night

void main()
{
    vec3 dir = normalize(TexCoords);
    
    // Zenith to Horizon gradient based on vertical height
    float h = clamp(dir.y, 0.0, 1.0);
    vec3 sky = mix(horizonColor, zenithColor, pow(h, 0.75));
    
    // Soft blend below the horizon to ground haze (darker at night)
    if (dir.y < 0.0) {
        vec3 nightHaze = vec3(0.04, 0.06, 0.1);
        vec3 dayHaze = vec3(0.35, 0.45, 0.55);
        sky = mix(horizonColor, mix(nightHaze, dayHaze, cycleValue), clamp(-dir.y * 3.0, 0.0, 1.0));
    }
    
    // 1. Procedural Sun Glow (Daytime)
    float sunFactor = dot(dir, sunDir);
    if (sunFactor > 0.0 && cycleValue > 0.01) {
        // Broad atmospheric scatter glow (haze)
        float wideGlow = pow(sunFactor, 24.0);
        sky += sunColor * wideGlow * 0.45 * cycleValue;
        
        // Mid-sized corona glow
        float midGlow = pow(sunFactor, 128.0);
        sky += sunColor * midGlow * 0.75 * cycleValue;
        
        // Direct intense core glare
        float coreGlare = pow(sunFactor, 512.0);
        sky += vec3(1.0, 1.0, 0.95) * coreGlare * 2.0 * cycleValue;
    }
    
    // 2. Procedural Moon Glow (Nighttime)
    float moonFactor = dot(dir, moonDir);
    float moonAlpha = 1.0 - cycleValue;
    if (moonFactor > 0.0 && moonAlpha > 0.01) {
        // Broad atmospheric scatter glow
        float wideGlow = pow(moonFactor, 16.0);
        sky += moonColor * wideGlow * 0.25 * moonAlpha;
        
        // Mid-sized corona glow
        float midGlow = pow(moonFactor, 96.0);
        sky += moonColor * midGlow * 0.45 * moonAlpha;
        
        // Direct intense core glare
        float coreGlare = pow(moonFactor, 384.0);
        sky += vec3(0.9, 0.95, 1.0) * coreGlare * 1.2 * moonAlpha;
    }
    
    FragColor = vec4(sky, 1.0);
}
