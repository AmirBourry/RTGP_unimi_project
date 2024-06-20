// Based on https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/5.advanced_lighting/7.bloom

#version 410 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, TexCoords);
}