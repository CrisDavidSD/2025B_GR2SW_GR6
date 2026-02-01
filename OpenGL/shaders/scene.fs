#version 330 core
out vec4 FragColor;

uniform sampler2D texture_emissive1;
uniform bool hasEmissive;
uniform float emissiveStrength;


// --- ESTRUCTURAS DE LUCES ---
#define MAX_LAMPS 32


struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform int numPointLights;
uniform PointLight pointLights[MAX_LAMPS];

// ----------------------------
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

// --- FUNCIONES DE CÁLCULO DE LUCES ---
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
                               light.linear * distance +
                               light.quadratic * distance * distance);

    vec3 ambient  = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords));

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}


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
    vec3 result = ambient + diffuse + specular + vec3(0.02);

    for (int i = 0; i < numPointLights; i++)
    {
        result += CalcPointLight(
            pointLights[i],
            norm,
            FragPos,
            viewDir
        );
    }

    // ================= EMISIÓN =================
    if (hasEmissive)
    {
        vec3 emissive = texture(texture_emissive1, TexCoords).rgb;
        result += emissive * emissiveStrength;
    }


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