#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool isFlame;
uniform float time;

void main()
{
    vec3 pos = aPos;
    if (isFlame) {
        // Alt kısmı (y = -0.5) sabit tutup, üst kısmı (y = 0.5) yukarı doğru uzatıp kısaltıyoruz (dikey salınım)
        float heightFactor = clamp(aPos.y + 0.5, 0.0, 1.0);
        pos.y += (sin(time * 12.0) * 0.06 + cos(time * 6.0) * 0.03) * heightFactor;
    }

    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
