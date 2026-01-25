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

uniform sampler2D texture_diffuse1; 
uniform sampler2D texture_specular1;

// --- NUEVO: Uniform para el color de la niebla ---
uniform vec3 fogColor; 
// -------------------------------------------------

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
    
    // Intensidad del foco
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
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spotLight.specular * spec * vec3(texture(texture_specular1, TexCoords));
    
    // Aplicar factores
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    // Resultado de iluminación normal
    vec3 result = ambient + diffuse + specular + vec3(0.02); 
    
    // ========================================================
    // CÁLCULO DE NIEBLA (FOG)
    // ========================================================
    
    // 1. Calcular distancia desde la cámara hasta el objeto
    float dist = length(viewPos - FragPos);

    // 2. Configurar límites de la niebla
    // Puedes convertir estos en 'uniforms' si quieres controlarlos desde el main.cpp
    float fogStart = 2.0;   // La niebla empieza a los 2 metros
    float fogEnd = 15.0;    // A los 15 metros ya no se ve nada (Niebla espesa de terror)

    // 3. Calcular factor de mezcla (Lineal)
    float fogFactor = (dist - fogStart) / (fogEnd - fogStart);
    fogFactor = clamp(fogFactor, 0.0, 1.0); // Asegurar que esté entre 0 y 1

    // 4. Mezclar el color iluminado con el color de la niebla
    // mix(ColorA, ColorB, factor): si factor es 0 devuelve A, si es 1 devuelve B
    vec3 finalOutput = mix(result, fogColor, fogFactor);

    FragColor = vec4(finalOutput, 1.0);
}