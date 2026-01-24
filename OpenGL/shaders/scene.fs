#version 330 core
out vec4 FragColor;

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform SpotLight spotLight;

// Assimp/LearnOpenGL usa estos nombres por defecto
uniform sampler2D texture_diffuse1; 
uniform sampler2D texture_specular1;

void main()
{
    // 1. Configuración básica
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 2. Cálculos de la Linterna (Spotlight)
    vec3 lightDir = normalize(spotLight.position - FragPos);
    
    // Atenuación
    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));    
    
    // Intensidad del foco (Flashlight edges)
    float theta = dot(lightDir, normalize(-spotLight.direction)); 
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Ambiente
    vec3 ambient = spotLight.ambient * vec3(texture(texture_diffuse1, TexCoords));
    
    // Difusa
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.diffuse * diff * vec3(texture(texture_diffuse1, TexCoords));
    
    // Especular
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Shininess 32
    vec3 specular = spotLight.specular * spec * vec3(texture(texture_specular1, TexCoords));
    
    // Aplicar factores
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    // 3. Resultado Final
    // Sumamos un pequeño valor base (0.05) para que no sea negro absoluto si la linterna no apunta
    vec3 result = ambient + diffuse + specular + vec3(0.02); 
    
    FragColor = vec4(result, 1.0);
}