#pragma once

// =========================================================================
// --- 1. MODEL SHADER (Iluminação Espacial Direcional & Hemisférica) ---
// =========================================================================

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    // CORREÇÃO: Normalizamos as normais no Vertex Shader para evitar bugs de luz ao mudar a escala!
    Normal = normalize(normalMatrix * aNormal);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos;     // Usado como VETOR DE DIREÇÃO GLOBAL DO ESPAÇO
uniform vec3 viewPos;
uniform vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform vec3 objectColor = vec3(0.9f, 0.5f, 0.1f);

void main()
{
    vec3 norm = normalize(Normal);
    
    // ---------------------------------------------------------------------
    // 1. ILUMINAÇÃO HEMISFÉRICA (Luz Ambiente do Espaço 3D)
    // ---------------------------------------------------------------------
    float hemiWeight = dot(norm, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3 skyLight = lightColor * 0.45f;    // Luz clara vinda de cima
    vec3 groundLight = lightColor * 0.15f; // Luz suave refletida de baixo
    vec3 ambient = mix(groundLight, skyLight, hemiWeight);

    // ---------------------------------------------------------------------
    // 2. LUZ DIRECIONAL INFINITA (Sol / Luz de Estúdio Global)
    // ---------------------------------------------------------------------
    vec3 lightDir = normalize(lightPos); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * 0.7f;

    // ---------------------------------------------------------------------
    // 3. BRILHO ESPECULAR (Reflexo da Luz no Espaço)
    // ---------------------------------------------------------------------
    float specularStrength = 0.35f;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";


// =========================================================================
// --- 2. GRID SHADER (Grade com Horizon Fade / Neblina Espacial) ---
// =========================================================================

const char* gridVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* gridFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;

void main()
{
    float dist = length(FragPos.xz);
    float fogFactor = smoothstep(4.0, 10.0, dist);
    
    vec3 gridColor = vec3(0.45f, 0.47f, 0.5f);
    vec3 bgColor = vec3(0.11f, 0.11f, 0.13f);
    
    vec3 finalColor = mix(gridColor, bgColor, fogFactor);
    FragColor = vec4(finalColor, 1.0);
}
)";


// =========================================================================
// --- 3. GIZMO SHADER (Eixos XYZ no Canto do Ecrã) ---
// =========================================================================

const char* gizmoVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 VertexColor;

uniform mat4 gizmoModel;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    VertexColor = aColor;
    gl_Position = projection * view * gizmoModel * vec4(aPos, 1.0);
}
)";

const char* gizmoFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 VertexColor;

void main()
{
    FragColor = vec4(VertexColor, 1.0);
}
)";