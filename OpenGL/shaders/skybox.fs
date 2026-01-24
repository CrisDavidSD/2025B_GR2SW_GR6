#version 330 core
out vec4 FragColor;

in vec3 TexCoords; // <--- OJO: DEBE SER vec3 (Igual que el .vs)

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
}