#version 330 core
out vec4 FragColor;

in vec3 FragPos;

// Luz de la linterna (spotlight)
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 spotLightColor;
uniform float spotLightCutOff;
uniform float spotLightOuterCutOff;
uniform bool flashlightOn;

void main()
{
    vec3 baseColor = vec3(0.7, 0.8, 0.9);
    vec3 result = baseColor * 0.15; // Luz ambiente mínima (casi invisible)
    
    if (flashlightOn) {
        // Dirección de la luz hacia el fragmento
        vec3 lightDir = normalize(spotLightPos - FragPos);
        
        // Ángulo entre dirección de linterna y rayo de luz
        float theta = dot(lightDir, normalize(-spotLightDir));
        float epsilon = spotLightCutOff - spotLightOuterCutOff;
        float intensity = clamp((theta - spotLightOuterCutOff) / epsilon, 0.0, 1.0);
        
        if (theta > spotLightOuterCutOff) {
            // Atenuación por distancia
            float distance = length(spotLightPos - FragPos);
            float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            
            // Efecto especular (reflejo) - gotas brillan como cristal
            vec3 viewDir = normalize(spotLightPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, vec3(0.0, 1.0, 0.0)); // Normal hacia arriba
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0); // Alto brillo
            
            // Componente difusa
            vec3 diffuse = intensity * attenuation * spotLightColor * baseColor * 0.8;
            
            // Componente especular (reflejos brillantes)
            vec3 specular = intensity * attenuation * spec * vec3(1.2, 1.2, 1.0);
            
            result = diffuse + specular;
        }
    }
    
    FragColor = vec4(result, 0.5); // Semi-transparente
}