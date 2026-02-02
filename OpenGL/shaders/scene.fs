#version 330 core
out vec4 FragColor;

uniform sampler2D texture_diffuse1; 
uniform sampler2D texture_specular1;
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
uniform int numPointLights;
uniform PointLight pointLights[MAX_LAMPS];

// --- Uniform para el color de la niebla ---
uniform vec3 fogColor;

// --- FUNCIÓN DE CÁLCULO DE PUNTO LUZ COMO FOCO CUADRADO ---
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // ====== LUCES CUADRADAS EN LUGAR DE CONOS ======
    // Vector desde la lámpara hacia el fragmento
    vec3 toFragment = fragPos - light.position;
    
    // Definir el tamaño de la caja cuadrada (puedes ajustar estos valores)
    float boxWidth = 8.0;   // Ancho X de la caja
    float boxHeight = 6.0;  // Alto Y de la caja (hacia abajo desde la lámpara)
    float boxDepth = 8.0;   // Profundidad Z de la caja
    
    // Verificar si el fragmento está dentro de la caja
    // La lámpara apunta hacia abajo (eje Y negativo)
    // Coordenadas locales respecto a la lámpara
    float localX = abs(toFragment.x);
    float localY = -toFragment.y;  // Negativo porque baja
    float localZ = abs(toFragment.z);
    
    // Si está fuera de los límites de la caja, sin iluminación
    if (localX > boxWidth || localY > boxHeight || localZ > boxDepth) {
        return vec3(0.0); // Zona oscura fuera de la caja
    }
    
    // ====== CÁLCULOS DE ILUMINACIÓN NORMALES ======
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
                               light.linear * distance +
                               light.quadratic * distance * distance);

    // ====== SUAVIZADO EN LOS BORDES DE LA CAJA ======
    // Hace que los bordes no sean tan duros
    float edgeSmooth = 1.0;
    
    // Suavizar en los bordes X
    if (localX > boxWidth * 0.8) {
        edgeSmooth *= (1.0 - (localX - boxWidth * 0.8) / (boxWidth * 0.2));
    }
    
    // Suavizar en los bordes Y
    if (localY > boxHeight * 0.8) {
        edgeSmooth *= (1.0 - (localY - boxHeight * 0.8) / (boxHeight * 0.2));
    }
    
    // Suavizar en los bordes Z
    if (localZ > boxDepth * 0.8) {
        edgeSmooth *= (1.0 - (localZ - boxDepth * 0.8) / (boxDepth * 0.2));
    }

    vec3 ambient  = light.ambient  * vec3(texture(texture_diffuse1, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(texture_specular1, TexCoords));

    // Aplicar atenuación y suavizado
    ambient  *= attenuation * edgeSmooth;
    diffuse  *= attenuation * edgeSmooth;
    specular *= attenuation * edgeSmooth;

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
    
    // Ambiente (siempre hay un poco de luz)
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
    
    // Color base sin lámparas
    vec3 result = ambient + diffuse + specular + vec3(0.02);

    // ====== AGREGAR ILUMINACIÓN DE LÁMPARAS (CON RESTRICCIÓN DE CONO) ======
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
    
    float dist = length(viewPos - FragPos);

    float fogStart = 2.0;
    float fogEnd = 15.0;

    float fogFactor = (dist - fogStart) / (fogEnd - fogStart);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalOutput = mix(result, fogColor, fogFactor);

    FragColor = vec4(finalOutput, 1.0);
}