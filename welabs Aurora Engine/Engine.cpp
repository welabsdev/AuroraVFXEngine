
#include "macroredef.h"

#include "Engine.h"
#include "Render.h"
#include "Shaders.h" 
#include "Model.h" 
#include <glad/glad.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <chrono>
#include <mutex> 
#include <algorithm> 
#include <fstream>   
#include <sstream>   
#include <cstdlib>
#include <ctime>

// --- INCLUDES DO DEAR IMGUI ---
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

// --- Includes GLM ---
#include <glm.hpp>             
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

using glm::clamp;
using glm::smoothstep;

// --- FIX DO CONFLITO COM WINDOWS.H ---
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifdef APIENTRY
#undef APIENTRY 
#endif
#include <windows.h>
#include <commdlg.h>
#endif

#define U8(str) ((const char*)u8##str)

// Constantes para Extensões de Leitura de Memória VRAM (NVIDIA / AMD)
#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_VIDMEM_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif

// ============================================================================
// --- SHADERS ASTROFÍSICOS (TEXTURA DE PLASMA, LENTE DE EINSTEIN & DOPPLER) ---
// ============================================================================
const char* particleVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 offset;
    layout (location = 2) in vec3 velocity;
    layout (location = 3) in vec4 color;
    layout (location = 4) in float size;

    out vec4 ParticleColor;
    out vec2 TexCoords;
    out vec3 WorldPos;
    out vec3 ParticleVel;

    uniform mat4 view;
    uniform mat4 projection;
    uniform vec3 u_CameraPos;
    uniform bool u_IsBlackHole;
    uniform float u_EventHorizonRadius;
    uniform bool u_EnableGravitationalLensing;

    void main() {
        ParticleColor = color;
        TexCoords = aPos.xy + vec2(0.5);
        WorldPos = offset;
        ParticleVel = velocity;
        
        vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 cameraUp    = vec3(view[0][1], view[1][1], view[2][1]);
        
        vec3 vertexPosition = offset 
            + cameraRight * aPos.x * size 
            + cameraUp    * aPos.y * size;

        // --- LENTE GRAVITACIONAL DE EINSTEIN ---
        if (u_IsBlackHole && u_EnableGravitationalLensing) {
            vec3 toCam = u_CameraPos - vec3(0.0); 
            vec3 toPart = offset - vec3(0.0);     
            float distToCenter = length(offset);
            
            if (dot(toCam, toPart) < 0.0 && distToCenter > u_EventHorizonRadius * 1.05) {
                float lensingFactor = (u_EventHorizonRadius * 2.0) / (distToCenter * distToCenter + 0.05);
                vec3 warpDir = normalize(offset - (dot(offset, normalize(u_CameraPos)) * normalize(u_CameraPos)));
                vertexPosition += warpDir * lensingFactor * 3.0; // Curva a luz em volta da esfera negra
            }
        }

        gl_Position = projection * view * vec4(vertexPosition, 1.0);
    }
)";

const char* particleFragmentShaderSource = R"(
    #version 330 core
    in vec4 ParticleColor;
    in vec2 TexCoords;
    in vec3 WorldPos;
    in vec3 ParticleVel;
    out vec4 FragColor;

    uniform float u_EventHorizonRadius;
    uniform bool u_IsBlackHole;
    uniform bool u_EnableDopplerBeaming;
    uniform vec3 u_CameraPos;
    uniform bool u_IsKerrBlackHole;
    uniform float u_ErgosphereRadius;
    uniform float u_Time;

    // Ruído Procedural para simular Plasma no Disco de Acreção
    float hash(vec2 p) {
        return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
    }
    float noise(vec2 p) {
        vec2 i = floor(p);
        vec2 f = fract(p);
        f = f * f * (3.0 - 2.0 * f);
        return mix(mix(hash(i + vec2(0.0, 0.0)), hash(i + vec2(1.0, 0.0)), f.x),
                   mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
    }

    void main() {
        float dist = length(TexCoords - vec2(0.5)) * 2.0;
        if (dist > 1.0) discard;

        vec2 polarCoords = vec2(atan(TexCoords.y - 0.5, TexCoords.x - 0.5), dist);
        float plasmaTexture = noise(polarCoords * 8.0 + vec2(u_Time * 5.0, 0.0)) * 0.5 + 0.5;

        float core = exp(-dist * 8.0) * 2.8;
        float corona = exp(-dist * 3.5) * plasmaTexture;
        float haze = smoothstep(1.0, 0.0, dist);

        float totalIntensity = core + corona + (haze * 0.4);
        vec3 finalRGB = ParticleColor.rgb * totalIntensity;
        float finalAlpha = ParticleColor.a * haze * clamp(totalIntensity, 0.0, 1.0);

        if (u_IsBlackHole) {
            float distToSingularity = length(WorldPos);
            
            if (distToSingularity < u_EventHorizonRadius * 1.01) {
                discard; // Absorção total no horizonte de eventos
            }
            else if (distToSingularity < u_EventHorizonRadius * 1.55) {
                float photonSphereGlow = smoothstep(u_EventHorizonRadius * 1.01, u_EventHorizonRadius * 1.55, distToSingularity);
                finalRGB = mix(vec3(1.0, 0.65, 0.15) * 6.0, finalRGB, photonSphereGlow);
            }
            else if (u_IsKerrBlackHole && distToSingularity < u_ErgosphereRadius) {
                float ergoGlow = smoothstep(u_EventHorizonRadius * 1.55, u_ErgosphereRadius, distToSingularity);
                finalRGB = mix(vec3(0.7, 0.1, 1.0) * 3.0, finalRGB, ergoGlow); 
            }

            if (u_EnableDopplerBeaming) {
                vec3 viewDir = normalize(u_CameraPos - WorldPos);
                vec3 velDir = normalize(ParticleVel + vec3(0.0001));
                float dopplerFactor = dot(viewDir, velDir); 
                
                if (dopplerFactor > 0.0) {
                    finalRGB *= (1.0 + dopplerFactor * 2.5);
                    finalRGB = mix(finalRGB, vec3(0.85, 0.95, 1.0) * 3.5, dopplerFactor * 0.65);
                } else {
                    finalRGB *= (1.0 + dopplerFactor * 0.9);
                    finalRGB = mix(finalRGB, vec3(0.35, 0.01, 0.0), -dopplerFactor * 0.75);
                }
            }
        }

        FragColor = vec4(finalRGB, finalAlpha);
    }
)";

// ============================================================================
// --- ESTRUTURA E PARÂMETROS DO MOTOR (.AURA v7.0) ---
// ============================================================================
struct AuraEffectConfig {
    std::string name = "Nenhum Efeito Carregado (Cena Limpa)";
    int count = 0;

    int emissionShape = 0; // 0=Esfera, 1=Disco de Acreção, 2=VFX Portal/Caixa, 3=Jatos Polares
    float radiusMin = 0.0f;
    float radiusMax = 0.0f;

    float speedMin = 0.0f;
    float speedMax = 0.0f;
    float sizeStart = 0.0f;
    float sizeEnd = 0.0f;
    float lifeMin = 0.0f;
    float lifeMax = 0.0f;

    glm::vec4 colorStart = glm::vec4(1.0f);
    glm::vec4 colorEnd = glm::vec4(1.0f);

    glm::vec3 gravity = glm::vec3(0.0f);
    float damping = 1.0f;
    float attractorStrength = 0.0f;
    float vortexSpeed = 0.0f;
    float turbulencePower = 0.0f;
    float turbulenceFreq = 1.0f;

    // --- PARÂMETROS DE RELATIVIDADE GERAL ---
    bool isBlackHole = false;
    float eventHorizonRadius = 1.5f;
    float lenseThirringFrameDragging = 20.0f;
    bool enableDopplerBeaming = true;
    bool enableGravitationalLensing = true;
    bool enableRelativisticTimeDilation = true;

    // --- BURACO NEGRO DE KERR & RADIAÇÃO DE HAWKING ---
    bool isKerrBlackHole = false;
    float kerrSpinParam = 0.88f;
    float ergosphereRadius = 2.6f;
    bool enableHawkingRadiation = false;

    // --- PULSAR / QUASAR ---
    bool isPulsar = false;
    float jetSpeed = 38.0f;
    float jetCollimation = 0.10f;

    bool isLoaded = false;
    std::string filepath = "";
};

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;
    float life;
    float maxLife;
    float size;
};

const int MAX_PARTICLES = 45000;
std::vector<Particle> g_Particles(MAX_PARTICLES);
unsigned int particleShader;
unsigned int particleVAO, particleVBO_Quad, particleVBO_Instance;

std::vector<float> g_ParticleInstanceData;
int g_ActiveParticles = 0;
AuraEffectConfig g_CurrentAuraConfig;
bool g_AuraLoopContinuous = true;

// --- BUFFER DA IDE DE TEXTO AO VIVO ---
static char g_AuraTextEditorBuffer[8192] = "";

// --- INTERAÇÃO COM CURSOR 3D (RAYCASTING) ---
bool g_EnableMouseInteraction = false;
int g_MouseInteractionMode = 0; // 0=Atrator (Puxar), 1=Repulsor (Empurrar), 2=Vórtice (Girar)
float g_MouseForceStrength = 40.0f;
float g_MouseInfluenceRadius = 7.0f;
glm::vec3 g_MousePos3D(0.0f, 0.0f, 0.0f);

// --- GIZMO 3D & CONTROLE DO MODELO (.OBJ) ---
bool g_ShowBlackHoleModel = false; // Inicia desativado (Cena Limpa)
glm::vec3 g_ModelPosition(0.0f, 0.0f, 0.0f);
glm::vec3 g_ModelRotation(0.0f, 0.0f, 0.0f);
glm::vec3 g_ModelScaleVec(0.5f, 0.5f, 0.5f);

// --- TREMOR DE CÂMERA CINEMÁTICO ---
bool g_EnableScreenShake = true;
float g_ScreenShakeIntensity = 0.0f;

float particleQuad[] = {
    -0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f, -0.5f,  0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f,  0.5f, -0.5f, 0.0f,  0.5f,  0.5f, 0.0f
};

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

float stringToFloat(std::string str) {
    str = trim(str);
    if (str.empty()) return 0.0f;
    for (char& c : str) {
        if (c == ',') c = '.';
    }
    try {
        return std::stof(str);
    }
    catch (...) {
        return 0.0f;
    }
}

std::vector<float> parseFloats(const std::string& val) {
    std::vector<float> result;
    std::string temp;
    for (char c : val) {
        if (c == ',' || c == ' ' || c == ';' || c == '\t') {
            if (!temp.empty()) {
                result.push_back(stringToFloat(temp));
                temp.clear();
            }
        }
        else if (c != '\r' && c != '\n') {
            temp += c;
        }
    }
    if (!temp.empty()) {
        result.push_back(stringToFloat(temp));
    }
    return result;
}

std::string OpenFileDialog(const char* filter, const char* title) {
    char filename[MAX_PATH] = "";
#ifdef _WIN32
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

// ============================================================================
// --- GERADOR PROCEDURAL GEOMÉTRICO (FLAMM & ERGOSFERA DE KERR) ---
// ============================================================================
extern Model g_Model;

void GenerateSpacetimeDistortionModel(AuraEffectConfig& config) {
    std::string filename = "./spacetime_singularity.obj";
    std::ofstream file(filename);
    if (!file.is_open()) return;

    file << "# Aurora Render Engine - Parabola de Flamm & Ergosfera de Kerr (.OBJ)\n";
    file << "# Modelo Gerado Cientificamente: " << config.name << "\n\n";

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<glm::ivec3> faces;

    float r_s = (config.eventHorizonRadius > 0.1f) ? config.eventHorizonRadius : 1.5f;
    int rings = 40;
    int sectors = 50;

    for (int i = 0; i <= rings; ++i) {
        float phi = glm::pi<float>() * (float)i / (float)rings;
        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * glm::pi<float>() * (float)j / (float)sectors;
            float x = r_s * sin(phi) * cos(theta);
            float y = r_s * cos(phi);
            float z = r_s * sin(phi) * sin(theta);

            vertices.push_back(glm::vec3(x, y, z));
            uvs.push_back(glm::vec2((float)j / sectors, (float)i / rings));
            normals.push_back(glm::normalize(glm::vec3(x, y, z)));
        }
    }

    int sphereVertexCount = (int)vertices.size();
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = (i * (sectors + 1)) + j + 1;
            int second = first + sectors + 1;
            faces.push_back(glm::ivec3(first, second, first + 1));
            faces.push_back(glm::ivec3(second, second + 1, first + 1));
        }
    }

    int gridRings = 50;
    int gridSectors = 60;
    float maxRadius = config.radiusMax * 1.5f;
    if (maxRadius < r_s * 3.0f) maxRadius = r_s * 8.0f;

    int funnelStartIndex = (int)vertices.size();

    for (int i = 0; i <= gridRings; ++i) {
        float t = (float)i / (float)gridRings;
        float r = r_s * 1.001f + pow(t, 1.4f) * (maxRadius - r_s);
        float depth = -2.0f * sqrt(r_s * (r - r_s));

        for (int j = 0; j <= gridSectors; ++j) {
            float theta = 2.0f * glm::pi<float>() * (float)j / (float)gridSectors;
            float x = r * cos(theta);
            float z = r * sin(theta);
            float y = depth;

            if (config.isKerrBlackHole && r < config.ergosphereRadius * 1.5f) {
                float twist = (config.ergosphereRadius / (r + 0.1f)) * config.kerrSpinParam * 0.6f;
                float newX = x * cos(twist) - z * sin(twist);
                float newZ = x * sin(twist) + z * cos(twist);
                x = newX; z = newZ;
            }

            vertices.push_back(glm::vec3(x, y, z));
            uvs.push_back(glm::vec2((float)j / gridSectors, t));

            glm::vec3 tangentR = glm::normalize(glm::vec3(cos(theta), -sqrt(r_s / (r - r_s)), sin(theta)));
            glm::vec3 tangentTheta = glm::normalize(glm::vec3(-sin(theta), 0.0f, cos(theta)));
            normals.push_back(glm::normalize(glm::cross(tangentTheta, tangentR)));
        }
    }

    for (int i = 0; i < gridRings; ++i) {
        for (int j = 0; j < gridSectors; ++j) {
            int first = funnelStartIndex + (i * (gridSectors + 1)) + j + 1;
            int second = first + gridSectors + 1;
            faces.push_back(glm::ivec3(first, second, first + 1));
            faces.push_back(glm::ivec3(second, second + 1, first + 1));
        }
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
        file << "v " << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";
    }
    for (size_t i = 0; i < normals.size(); ++i) {
        file << "vn " << normals[i].x << " " << normals[i].y << " " << normals[i].z << "\n";
    }
    for (size_t i = 0; i < uvs.size(); ++i) {
        file << "vt " << uvs[i].x << " " << uvs[i].y << "\n";
    }
    file << "s 1\n";
    for (size_t i = 0; i < faces.size(); ++i) {
        file << "f " << faces[i].x << "/" << faces[i].x << "/" << faces[i].x << " "
            << faces[i].y << "/" << faces[i].y << "/" << faces[i].y << " "
            << faces[i].z << "/" << faces[i].z << "/" << faces[i].z << "\n";
    }

    file.close();
    g_Model.LoadModel(filename);
}

void SpawnSingleParticle(Particle& p, glm::vec3 center) {
    if (!g_CurrentAuraConfig.isLoaded || g_CurrentAuraConfig.count <= 0) return;

    float radiusRange = g_CurrentAuraConfig.radiusMax - g_CurrentAuraConfig.radiusMin;
    float dist = g_CurrentAuraConfig.radiusMin + ((rand() % 1000) / 1000.0f) * radiusRange;

    if (g_CurrentAuraConfig.isBlackHole && dist < g_CurrentAuraConfig.eventHorizonRadius * 1.4f) {
        dist = g_CurrentAuraConfig.eventHorizonRadius * 1.5f + ((rand() % 1000) / 1000.0f) * radiusRange;
    }

    float theta = (rand() % 360) * 3.14159f / 180.0f;
    float phi = (rand() % 180) * 3.14159f / 180.0f;
    float speedRange = g_CurrentAuraConfig.speedMax - g_CurrentAuraConfig.speedMin;
    float speed = g_CurrentAuraConfig.speedMin + ((rand() % 1000) / 1000.0f) * speedRange;

    if (g_CurrentAuraConfig.isBlackHole && g_CurrentAuraConfig.enableHawkingRadiation && (rand() % 100 < 8)) {
        glm::vec3 escapeDir = glm::normalize(glm::vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));
        p.position = center + escapeDir * (g_CurrentAuraConfig.eventHorizonRadius * 1.05f);
        p.velocity = escapeDir * (speed * 0.45f);
        p.color = glm::vec4(0.8f, 0.95f, 1.0f, 1.0f);
        p.size = g_CurrentAuraConfig.sizeStart * 0.45f;
        p.maxLife = 1.3f;
        p.life = p.maxLife;
        return;
    }

    if (g_CurrentAuraConfig.isPulsar && (rand() % 100 < 35)) {
        float polarDir = (rand() % 2 == 0) ? 1.0f : -1.0f;
        float spreadX = ((rand() % 1000) / 500.0f - 1.0f) * g_CurrentAuraConfig.jetCollimation;
        float spreadZ = ((rand() % 1000) / 500.0f - 1.0f) * g_CurrentAuraConfig.jetCollimation;

        p.position = center + glm::vec3(spreadX, polarDir * g_CurrentAuraConfig.eventHorizonRadius * 1.2f, spreadZ);
        p.velocity = glm::normalize(glm::vec3(spreadX, polarDir, spreadZ)) * g_CurrentAuraConfig.jetSpeed;
    }
    else if (g_CurrentAuraConfig.emissionShape == 1 || g_CurrentAuraConfig.isBlackHole) {
        float thickness = g_CurrentAuraConfig.isBlackHole ? 0.03f : 0.2f;
        p.position = center + glm::vec3(cos(theta) * dist, ((rand() % 100) / 500.0f - 0.1f) * dist * thickness, sin(theta) * dist);

        glm::vec3 toCenter = glm::normalize(-p.position);
        glm::vec3 tangent = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), toCenter));
        float keplerSpeed = speed * sqrt(12.0f / (dist + 0.1f));
        p.velocity = tangent * keplerSpeed;
    }
    else if (g_CurrentAuraConfig.emissionShape == 0) {
        p.position = center + glm::vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)) * dist;
        p.velocity = glm::vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)) * speed;
    }
    else {
        float rx = ((rand() % 1000) / 500.0f - 1.0f) * dist;
        float ry = ((rand() % 1000) / 500.0f - 1.0f) * dist;
        float rz = ((rand() % 1000) / 500.0f - 1.0f) * dist;
        p.position = center + glm::vec3(rx, ry, rz);
        p.velocity = glm::vec3(0.0f, speed, 0.0f);
    }

    float lifeRange = g_CurrentAuraConfig.lifeMax - g_CurrentAuraConfig.lifeMin;
    p.maxLife = g_CurrentAuraConfig.lifeMin + ((rand() % 1000) / 1000.0f) * lifeRange;
    p.life = p.maxLife;
    p.color = g_CurrentAuraConfig.colorStart;
    p.size = g_CurrentAuraConfig.sizeStart;
}

void TriggerAuraEffect(glm::vec3 center) {
    if (!g_CurrentAuraConfig.isLoaded || g_CurrentAuraConfig.count <= 0) return;

    int particlesToEmit = clamp(g_CurrentAuraConfig.count, 0, MAX_PARTICLES);
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (i < particlesToEmit) {
            SpawnSingleParticle(g_Particles[i], center);
        }
        else {
            g_Particles[i].life = 0.0f;
        }
    }

    if (g_CurrentAuraConfig.attractorStrength > 30.0f || g_CurrentAuraConfig.isPulsar) {
        g_ScreenShakeIntensity = 0.35f;
    }
}

// ============================================================================
// --- PARSER UNIFICADO E SINCRONIZAÇÃO BIDIRECIONAL (GUI <-> IDE) ---
// ============================================================================
bool ParseAuraFromStream(std::istream& stream, const std::string& sourceName) {
    AuraEffectConfig newConfig;
    newConfig.name = "Efeito Personalizado (.AURA)";

    std::string line;
    while (std::getline(stream, line)) {
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        line = trim(line);
        if (line.empty()) continue;

        size_t delimiter = line.find_first_of("=:");
        if (delimiter == std::string::npos) continue;

        std::string key = trim(line.substr(0, delimiter));
        std::string val = trim(line.substr(delimiter + 1));
        std::transform(key.begin(), key.end(), key.begin(), ::toupper);

        if (key == "NAME") newConfig.name = val;
        else if (key == "COUNT") newConfig.count = clamp((int)stringToFloat(val), 0, MAX_PARTICLES);
        else if (key == "EMISSION_SHAPE") newConfig.emissionShape = clamp((int)stringToFloat(val), 0, 3);
        else if (key == "RADIUS_MIN") newConfig.radiusMin = stringToFloat(val);
        else if (key == "RADIUS_MAX") newConfig.radiusMax = stringToFloat(val);
        else if (key == "SPEED_MIN") newConfig.speedMin = stringToFloat(val);
        else if (key == "SPEED_MAX") newConfig.speedMax = stringToFloat(val);
        else if (key == "SIZE_START") newConfig.sizeStart = stringToFloat(val);
        else if (key == "SIZE_END") newConfig.sizeEnd = stringToFloat(val);
        else if (key == "LIFE_MIN") newConfig.lifeMin = stringToFloat(val);
        else if (key == "LIFE_MAX") newConfig.lifeMax = stringToFloat(val);
        else if (key == "DAMPING") newConfig.damping = stringToFloat(val);
        else if (key == "ATTRACTOR_STRENGTH") newConfig.attractorStrength = stringToFloat(val);
        else if (key == "VORTEX_SPEED") newConfig.vortexSpeed = stringToFloat(val);
        else if (key == "TURBULENCE_POWER") newConfig.turbulencePower = stringToFloat(val);
        else if (key == "IS_BLACK_HOLE") newConfig.isBlackHole = (stringToFloat(val) > 0.0f);
        else if (key == "EVENT_HORIZON_RADIUS") newConfig.eventHorizonRadius = stringToFloat(val);
        else if (key == "LENSE_THIRRING_DRAG") newConfig.lenseThirringFrameDragging = stringToFloat(val);
        else if (key == "DOPPLER_BEAMING") newConfig.enableDopplerBeaming = (stringToFloat(val) > 0.0f);
        else if (key == "GRAVITATIONAL_LENSING") newConfig.enableGravitationalLensing = (stringToFloat(val) > 0.0f);
        else if (key == "TIME_DILATION") newConfig.enableRelativisticTimeDilation = (stringToFloat(val) > 0.0f);
        else if (key == "IS_KERR_BLACK_HOLE") newConfig.isKerrBlackHole = (stringToFloat(val) > 0.0f);
        else if (key == "KERR_SPIN") newConfig.kerrSpinParam = stringToFloat(val);
        else if (key == "ERGOSPHERE_RADIUS") newConfig.ergosphereRadius = stringToFloat(val);
        else if (key == "HAWKING_RADIATION") newConfig.enableHawkingRadiation = (stringToFloat(val) > 0.0f);
        else if (key == "IS_PULSAR") newConfig.isPulsar = (stringToFloat(val) > 0.0f);
        else if (key == "JET_SPEED") newConfig.jetSpeed = stringToFloat(val);
        else if (key == "COLOR_START") {
            auto nums = parseFloats(val);
            if (nums.size() >= 4) newConfig.colorStart = glm::vec4(nums[0], nums[1], nums[2], nums[3]);
        }
        else if (key == "COLOR_END") {
            auto nums = parseFloats(val);
            if (nums.size() >= 4) newConfig.colorEnd = glm::vec4(nums[0], nums[1], nums[2], nums[3]);
        }
    }

    newConfig.isLoaded = true;
    newConfig.filepath = sourceName;
    g_CurrentAuraConfig = newConfig;

    // Se ativamos um Buraco Negro, exibe o modelo 3D por padrão!
    if (g_CurrentAuraConfig.isBlackHole) {
        g_ShowBlackHoleModel = true;
        GenerateSpacetimeDistortionModel(g_CurrentAuraConfig);
    }

    TriggerAuraEffect(glm::vec3(0.0f, 0.0f, 0.0f));
    return true;
}

void SyncConfigToTextEditorBuffer() {
    if (!g_CurrentAuraConfig.isLoaded) {
        snprintf(g_AuraTextEditorBuffer, sizeof(g_AuraTextEditorBuffer),
            "# Aurora VFX & Astro-Sim Engine (.aura v7.0 Definitive)\n"
            "# NENHUM EFEITO CARREGADO EM MEMÓRIA (CENA LIMPA)\n"
            "# Selecione um Preset na primeira aba ou digite seus parâmetros abaixo:\n\n"
            "NAME=Novo Efeito\nCOUNT=0\nEMISSION_SHAPE=0\nRADIUS_MIN=1.0\nRADIUS_MAX=5.0\n"
            "SPEED_MIN=1.0\nSPEED_MAX=3.0\nSIZE_START=0.5\nSIZE_END=0.0\nLIFE_MIN=1.0\nLIFE_MAX=2.0\n"
            "COLOR_START=1.0,1.0,1.0,1.0\nCOLOR_END=0.0,0.0,0.0,0.0\nATTRACTOR_STRENGTH=0.0\n"
            "VORTEX_SPEED=0.0\nTURBULENCE_POWER=0.0\nIS_BLACK_HOLE=0\nEVENT_HORIZON_RADIUS=1.5\n");
        return;
    }

    std::stringstream ss;
    ss << "# Aurora VFX & Astro-Sim Engine (.aura v7.0 Definitive)\n";
    ss << "NAME=" << g_CurrentAuraConfig.name << "\n";
    ss << "COUNT=" << g_CurrentAuraConfig.count << "\n";
    ss << "EMISSION_SHAPE=" << g_CurrentAuraConfig.emissionShape << "\n";
    ss << "RADIUS_MIN=" << g_CurrentAuraConfig.radiusMin << "\n";
    ss << "RADIUS_MAX=" << g_CurrentAuraConfig.radiusMax << "\n";
    ss << "SPEED_MIN=" << g_CurrentAuraConfig.speedMin << "\n";
    ss << "SPEED_MAX=" << g_CurrentAuraConfig.speedMax << "\n";
    ss << "SIZE_START=" << g_CurrentAuraConfig.sizeStart << "\n";
    ss << "SIZE_END=" << g_CurrentAuraConfig.sizeEnd << "\n";
    ss << "LIFE_MIN=" << g_CurrentAuraConfig.lifeMin << "\n";
    ss << "LIFE_MAX=" << g_CurrentAuraConfig.lifeMax << "\n";
    ss << "COLOR_START=" << g_CurrentAuraConfig.colorStart.r << "," << g_CurrentAuraConfig.colorStart.g << "," << g_CurrentAuraConfig.colorStart.b << "," << g_CurrentAuraConfig.colorStart.a << "\n";
    ss << "COLOR_END=" << g_CurrentAuraConfig.colorEnd.r << "," << g_CurrentAuraConfig.colorEnd.g << "," << g_CurrentAuraConfig.colorEnd.b << "," << g_CurrentAuraConfig.colorEnd.a << "\n";
    ss << "ATTRACTOR_STRENGTH=" << g_CurrentAuraConfig.attractorStrength << "\n";
    ss << "VORTEX_SPEED=" << g_CurrentAuraConfig.vortexSpeed << "\n";
    ss << "TURBULENCE_POWER=" << g_CurrentAuraConfig.turbulencePower << "\n";
    ss << "IS_BLACK_HOLE=" << (g_CurrentAuraConfig.isBlackHole ? "1" : "0") << "\n";
    ss << "EVENT_HORIZON_RADIUS=" << g_CurrentAuraConfig.eventHorizonRadius << "\n";
    ss << "LENSE_THIRRING_DRAG=" << g_CurrentAuraConfig.lenseThirringFrameDragging << "\n";
    ss << "DOPPLER_BEAMING=" << (g_CurrentAuraConfig.enableDopplerBeaming ? "1" : "0") << "\n";
    ss << "GRAVITATIONAL_LENSING=" << (g_CurrentAuraConfig.enableGravitationalLensing ? "1" : "0") << "\n";
    ss << "TIME_DILATION=" << (g_CurrentAuraConfig.enableRelativisticTimeDilation ? "1" : "0") << "\n";
    ss << "IS_KERR_BLACK_HOLE=" << (g_CurrentAuraConfig.isKerrBlackHole ? "1" : "0") << "\n";
    ss << "KERR_SPIN=" << g_CurrentAuraConfig.kerrSpinParam << "\n";
    ss << "ERGOSPHERE_RADIUS=" << g_CurrentAuraConfig.ergosphereRadius << "\n";
    ss << "HAWKING_RADIATION=" << (g_CurrentAuraConfig.enableHawkingRadiation ? "1" : "0") << "\n";
    ss << "IS_PULSAR=" << (g_CurrentAuraConfig.isPulsar ? "1" : "0") << "\n";
    ss << "JET_SPEED=" << g_CurrentAuraConfig.jetSpeed << "\n";

    std::string str = ss.str();
    snprintf(g_AuraTextEditorBuffer, sizeof(g_AuraTextEditorBuffer), "%s", str.c_str());
}

void LoadPresetEffect(int presetIndex) {
    AuraEffectConfig p;
    p.isLoaded = true;
    p.filepath = "PRESET_INTERNO";

    if (presetIndex == 0) {
        p.name = "Buraco Negro de Kerr (Spinning Singularity)";
        p.count = 40000;
        p.emissionShape = 1;
        p.radiusMin = 2.2f;
        p.radiusMax = 16.0f;
        p.speedMin = 5.0f;
        p.speedMax = 12.0f;
        p.sizeStart = 0.55f;
        p.sizeEnd = 0.05f;
        p.lifeMin = 2.5f;
        p.lifeMax = 6.0f;
        p.colorStart = glm::vec4(1.0f, 0.85f, 0.35f, 0.95f);
        p.colorEnd = glm::vec4(0.85f, 0.05f, 0.01f, 0.0f);
        p.attractorStrength = 45.0f;
        p.vortexSpeed = 24.0f;
        p.turbulencePower = 0.3f;
        p.isBlackHole = true;
        p.eventHorizonRadius = 1.6f;
        p.lenseThirringFrameDragging = 32.0f;
        p.enableDopplerBeaming = true;
        p.enableGravitationalLensing = true;
        p.enableRelativisticTimeDilation = true;
        p.isKerrBlackHole = true;
        p.kerrSpinParam = 0.95f;
        p.ergosphereRadius = 2.8f;
        p.enableHawkingRadiation = true;
    }
    else if (presetIndex == 1) {
        p.name = "Quasar Polar / Pulsar Magnetar";
        p.count = 35000;
        p.emissionShape = 1;
        p.radiusMin = 1.0f;
        p.radiusMax = 9.0f;
        p.speedMin = 7.0f;
        p.speedMax = 15.0f;
        p.sizeStart = 0.45f;
        p.sizeEnd = 0.04f;
        p.lifeMin = 1.0f;
        p.lifeMax = 3.5f;
        p.colorStart = glm::vec4(0.2f, 0.85f, 1.0f, 1.0f);
        p.colorEnd = glm::vec4(0.7f, 0.1f, 0.95f, 0.0f);
        p.attractorStrength = 28.0f;
        p.vortexSpeed = 35.0f;
        p.isBlackHole = true;
        p.eventHorizonRadius = 0.9f;
        p.lenseThirringFrameDragging = 45.0f;
        p.isPulsar = true;
        p.jetSpeed = 45.0f;
        p.jetCollimation = 0.08f;
    }
    else if (presetIndex == 2) {
        p.name = "Remanescente de Supernova (Astro-Sim)";
        p.count = 45000;
        p.emissionShape = 0;
        p.radiusMin = 0.5f;
        p.radiusMax = 20.0f;
        p.speedMin = 0.5f;
        p.speedMax = 4.5f;
        p.sizeStart = 0.9f;
        p.sizeEnd = 1.8f;
        p.lifeMin = 4.0f;
        p.lifeMax = 10.0f;
        p.colorStart = glm::vec4(1.0f, 0.4f, 0.1f, 0.85f);
        p.colorEnd = glm::vec4(0.1f, 0.6f, 0.9f, 0.0f);
        p.turbulencePower = 3.5f;
        p.turbulenceFreq = 1.5f;
        p.isBlackHole = false;
    }
    else if (presetIndex == 3) {
        p.name = "Vórtice Arcano / Portal de Jogo (VFX)";
        p.count = 28000;
        p.emissionShape = 1;
        p.radiusMin = 3.0f;
        p.radiusMax = 3.8f;
        p.speedMin = 2.0f;
        p.speedMax = 6.0f;
        p.sizeStart = 0.5f;
        p.sizeEnd = 0.02f;
        p.lifeMin = 1.0f;
        p.lifeMax = 2.2f;
        p.colorStart = glm::vec4(0.1f, 1.0f, 0.4f, 1.0f);
        p.colorEnd = glm::vec4(0.0f, 0.3f, 0.9f, 0.0f);
        p.attractorStrength = -12.0f;
        p.vortexSpeed = 28.0f;
        p.isBlackHole = false;
    }

    g_CurrentAuraConfig = p;
    if (g_CurrentAuraConfig.isBlackHole) {
        g_ShowBlackHoleModel = true;
        GenerateSpacetimeDistortionModel(g_CurrentAuraConfig);
    }
    else {
        g_ShowBlackHoleModel = false;
    }

    TriggerAuraEffect(glm::vec3(0.0f, 0.0f, 0.0f));
    SyncConfigToTextEditorBuffer();
}

bool LoadAuraEffectFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    snprintf(g_AuraTextEditorBuffer, sizeof(g_AuraTextEditorBuffer), "%s", buffer.str().c_str());

    std::istringstream stream(buffer.str());
    return ParseAuraFromStream(stream, filepath);
}

void SaveAuraEffectFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    file << g_AuraTextEditorBuffer;
    file.close();
    g_CurrentAuraConfig.filepath = filepath;
}

class ImGuiConsoleBuffer : public std::streambuf {
public:
    std::string buffer;
    std::mutex mutex;
    bool scrollToBottom = false;

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex);
        buffer.clear();
    }

protected:
    virtual int_type overflow(int_type ch) override {
        if (ch != traits_type::eof()) {
            std::lock_guard<std::mutex> lock(mutex);
            buffer.push_back(static_cast<char>(ch));
            scrollToBottom = true;
        }
        return ch;
    }

    virtual std::streamsize xsputn(const char* s, std::streamsize count) override {
        std::lock_guard<std::mutex> lock(mutex);
        buffer.append(s, count);
        scrollToBottom = true;
        return count;
    }
};

namespace AuroraConsole {
    ImGuiConsoleBuffer g_ConsoleBuffer;
    std::streambuf* g_OldCoutBuffer = nullptr;
    std::streambuf* g_OldCerrBuffer = nullptr;

    void Init() {
        g_OldCoutBuffer = std::cout.rdbuf(&g_ConsoleBuffer);
        g_OldCerrBuffer = std::cerr.rdbuf(&g_ConsoleBuffer);
    }
    void Cleanup() {
        if (g_OldCoutBuffer) std::cout.rdbuf(g_OldCoutBuffer);
        if (g_OldCerrBuffer) std::cerr.rdbuf(g_OldCerrBuffer);
    }
}

unsigned int shaderProgram;
Model g_Model;
SDL_Window* window = nullptr;

std::string g_GpuVendor = "Desconhecido";
std::string g_GpuRenderer = "Desconhecido";
std::string g_GlVersion = "Desconhecido";
std::string g_GlslVersion = "Desconhecido";

glm::vec3 cameraPos = glm::vec3(0.0f, 6.0f, 18.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 8.0f;
float mouseSensitivity = 0.15f;
float yaw = -90.0f;
float pitch = -20.0f;
float g_Fov = 45.0f;

bool firstMouse = true;
bool isLMBDown = false;
static bool isFullscreen = false;
bool g_CinematicOrbitCamera = false;
float g_OrbitAngle = 0.0f;

unsigned int gridShaderProgram;
unsigned int gridVAO;
unsigned int gizmoShaderProgram;
unsigned int gizmoVAO;

glm::vec2 g_GizmoScreenPos_X(0.0f, 0.0f);
glm::vec2 g_GizmoScreenPos_Y(0.0f, 0.0f);
glm::vec2 g_GizmoScreenPos_Z(0.0f, 0.0f);

glm::vec3 g_BgColor(0.005f, 0.005f, 0.008f);
glm::vec3 g_LightPos(10.0f, 15.0f, 10.0f);
glm::vec3 g_LightColor(1.0f, 1.0f, 1.0f);
glm::vec3 g_ObjectColor(0.35f, 0.35f, 0.4f);
bool g_ShowGrid = true;
bool g_Wireframe = false;

glm::vec3 RaycastMouseTo3DSpace(int mouseX, int mouseY, int screenW, int screenH, glm::mat4 view, glm::mat4 proj) {
    float x = (2.0f * mouseX) / screenW - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenH;

    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    if (fabs(rayWorld.y) > 0.0001f) {
        float t = -cameraPos.y / rayWorld.y;
        if (t > 0.0f) {
            return cameraPos + rayWorld * t;
        }
    }
    return cameraPos + rayWorld * 15.0f;
}

void Renderer_Init() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int gvs_grid = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gvs_grid, 1, &gridVertexShaderSource, NULL);
    glCompileShader(gvs_grid);
    unsigned int gfs_grid = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gfs_grid, 1, &gridFragmentShaderSource, NULL);
    glCompileShader(gfs_grid);
    gridShaderProgram = glCreateProgram();
    glAttachShader(gridShaderProgram, gvs_grid);
    glAttachShader(gridShaderProgram, gfs_grid);
    glLinkProgram(gridShaderProgram);
    glDeleteShader(gvs_grid);
    glDeleteShader(gfs_grid);

    std::vector<float> gridVertices;
    int gridSize = 40;
    float step = 1.0f;
    float halfSize = (float)gridSize / 2.0f;
    for (int i = 0; i <= gridSize; ++i) {
        float z = (float)i * step - halfSize;
        gridVertices.push_back(-halfSize); gridVertices.push_back(0.0f); gridVertices.push_back(z);
        gridVertices.push_back(halfSize); gridVertices.push_back(0.0f); gridVertices.push_back(z);
        float x = (float)i * step - halfSize;
        gridVertices.push_back(x); gridVertices.push_back(0.0f); gridVertices.push_back(-halfSize);
        gridVertices.push_back(x); gridVertices.push_back(0.0f); gridVertices.push_back(halfSize);
    }

    glGenVertexArrays(1, &gridVAO);
    unsigned int gridVBO;
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), &gridVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    unsigned int gvs_gizmo = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(gvs_gizmo, 1, &gizmoVertexShaderSource, NULL);
    glCompileShader(gvs_gizmo);
    unsigned int gfs_gizmo = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(gfs_gizmo, 1, &gizmoFragmentShaderSource, NULL);
    glCompileShader(gfs_gizmo);

    gizmoShaderProgram = glCreateProgram();
    glAttachShader(gizmoShaderProgram, gvs_gizmo);
    glAttachShader(gizmoShaderProgram, gfs_gizmo);
    glLinkProgram(gizmoShaderProgram);
    glDeleteShader(gvs_gizmo);
    glDeleteShader(gfs_gizmo);

    float gizmoVertices[] = {
         0.0f, 0.0f, 0.0f,     1.0f, 0.0f, 0.0f,
         1.0f, 0.0f, 0.0f,     1.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.0f,     0.0f, 1.0f, 0.0f,
         0.0f, 1.0f, 0.0f,     0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 0.0f,     0.0f, 0.5f, 1.0f,
         0.0f, 0.0f, 1.0f,     0.0f, 0.5f, 1.0f
    };

    glGenVertexArrays(1, &gizmoVAO);
    unsigned int gizmoVBO;
    glGenBuffers(1, &gizmoVBO);
    glBindVertexArray(gizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gizmoVertices), gizmoVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    unsigned int pVS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(pVS, 1, &particleVertexShaderSource, NULL);
    glCompileShader(pVS);
    unsigned int pFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(pFS, 1, &particleFragmentShaderSource, NULL);
    glCompileShader(pFS);

    particleShader = glCreateProgram();
    glAttachShader(particleShader, pVS);
    glAttachShader(particleShader, pFS);
    glLinkProgram(particleShader);
    glDeleteShader(pVS);
    glDeleteShader(pFS);

    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO_Quad);
    glGenBuffers(1, &particleVBO_Instance);

    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO_Quad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particleQuad), particleQuad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, particleVBO_Instance);
    glBufferData(GL_ARRAY_BUFFER, MAX_PARTICLES * 11 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(10 * sizeof(float)));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);

    for (auto& p : g_Particles) {
        p.life = 0.0f;
        p.size = 0.0f;
    }
    g_ActiveParticles = 0;

    // --- INICIA O MOTOR 100% LIMPO E VAZIO (SEM EFEITOS OU MODELOS)! ---
    g_CurrentAuraConfig = AuraEffectConfig();
    g_CurrentAuraConfig.isLoaded = false;
    g_ShowBlackHoleModel = false;
    SyncConfigToTextEditorBuffer();
}

void Renderer_Draw() {
    glm::vec3 actualCamPos = cameraPos;

    if (g_EnableScreenShake && g_ScreenShakeIntensity > 0.001f) {
        float rx = ((rand() % 100) / 100.0f - 0.5f) * g_ScreenShakeIntensity;
        float ry = ((rand() % 100) / 100.0f - 0.5f) * g_ScreenShakeIntensity;
        float rz = ((rand() % 100) / 100.0f - 0.5f) * g_ScreenShakeIntensity;
        actualCamPos += glm::vec3(rx, ry, rz);
        g_ScreenShakeIntensity *= 0.92f;
    }

    if (g_CinematicOrbitCamera) {
        g_OrbitAngle += 0.004f;
        float radius = 18.0f;
        actualCamPos.x = sin(g_OrbitAngle) * radius;
        actualCamPos.z = cos(g_OrbitAngle) * radius;
        actualCamPos.y = 5.0f + sin(g_OrbitAngle * 0.5f) * 3.5f;
        cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - actualCamPos);
    }
    else {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(direction);
    }

    glm::mat4 view = glm::lookAt(actualCamPos, actualCamPos + cameraFront, cameraUp);

    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    if (height == 0) height = 1;

    glViewport(0, 0, width, height);
    glm::mat4 projection = glm::perspective(glm::radians(g_Fov), (float)width / (float)height, 0.1f, 500.0f);

    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    g_MousePos3D = RaycastMouseTo3DSpace((int)mouseX, (int)mouseY, width, height, view, projection);

    if (g_ShowGrid) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUseProgram(gridShaderProgram);
        glm::mat4 gridModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "model"), 1, GL_FALSE, &gridModel[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, (40 + 1) * 4);
        glBindVertexArray(0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, g_Wireframe ? GL_LINE : GL_FILL);
    glUseProgram(shaderProgram);

    if (g_ShowBlackHoleModel && g_Model.IsLoaded()) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, g_ModelPosition);
        model = glm::rotate(model, glm::radians(g_ModelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(g_ModelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(g_ModelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, g_ModelScaleVec);

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        glUniformMatrix3fv(glGetUniformLocation(shaderProgram, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(g_LightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(g_LightColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(g_ObjectColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(actualCamPos));

        g_Model.Draw();
    }

    if (g_CurrentAuraConfig.isLoaded && g_ActiveParticles > 0) {
        glUseProgram(particleShader);
        glUniformMatrix4fv(glGetUniformLocation(particleShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(particleShader, "projection"), 1, GL_FALSE, &projection[0][0]);

        glUniform1f(glGetUniformLocation(particleShader, "u_EventHorizonRadius"), g_CurrentAuraConfig.eventHorizonRadius);
        glUniform1i(glGetUniformLocation(particleShader, "u_IsBlackHole"), g_CurrentAuraConfig.isBlackHole ? 1 : 0);
        glUniform1i(glGetUniformLocation(particleShader, "u_EnableDopplerBeaming"), g_CurrentAuraConfig.enableDopplerBeaming ? 1 : 0);
        glUniform1i(glGetUniformLocation(particleShader, "u_EnableGravitationalLensing"), g_CurrentAuraConfig.enableGravitationalLensing ? 1 : 0);
        glUniform1i(glGetUniformLocation(particleShader, "u_IsKerrBlackHole"), g_CurrentAuraConfig.isKerrBlackHole ? 1 : 0);
        glUniform1f(glGetUniformLocation(particleShader, "u_ErgosphereRadius"), g_CurrentAuraConfig.ergosphereRadius);
        glUniform3fv(glGetUniformLocation(particleShader, "u_CameraPos"), 1, glm::value_ptr(actualCamPos));
        glUniform1f(glGetUniformLocation(particleShader, "u_Time"), (float)SDL_GetTicks() * 0.001f);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        glBindBuffer(GL_ARRAY_BUFFER, particleVBO_Instance);
        glBufferSubData(GL_ARRAY_BUFFER, 0, g_ParticleInstanceData.size() * sizeof(float), g_ParticleInstanceData.data());

        glBindVertexArray(particleVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, g_ActiveParticles);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    glUseProgram(gizmoShaderProgram);
    glDisable(GL_DEPTH_TEST);

    float aspect = (float)width / (float)height;
    glm::mat4 orthoProjection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -10.0f, 10.0f);

    glm::mat4 cameraRotation = glm::mat4(glm::mat3(view));
    glm::mat4 gizmoModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.08f));
    glm::mat4 gizmoView = glm::translate(glm::mat4(1.0f), glm::vec3(aspect * 0.85f, -0.85f, 0.0f)) * cameraRotation;

    glUniformMatrix4fv(glGetUniformLocation(gizmoShaderProgram, "gizmoModel"), 1, GL_FALSE, &gizmoModel[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(gizmoShaderProgram, "view"), 1, GL_FALSE, &gizmoView[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(gizmoShaderProgram, "projection"), 1, GL_FALSE, &orthoProjection[0][0]);

    glLineWidth(3.0f);
    glBindVertexArray(gizmoVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);

    glm::mat4 MVP = orthoProjection * gizmoView * gizmoModel;
    auto ProjectToScreen = [&](glm::vec3 localPos) -> glm::vec2 {
        glm::vec4 clipPos = MVP * glm::vec4(localPos, 1.0f);
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        float screenX = (ndc.x + 1.0f) * 0.5f * (float)width;
        float screenY = (1.0f - ndc.y) * 0.5f * (float)height;
        return glm::vec2(screenX, screenY);
        };

    g_GizmoScreenPos_X = ProjectToScreen(glm::vec3(1.3f, 0.0f, 0.0f));
    g_GizmoScreenPos_Y = ProjectToScreen(glm::vec3(0.0f, 1.3f, 0.0f));
    g_GizmoScreenPos_Z = ProjectToScreen(glm::vec3(0.0f, 0.0f, 1.3f));

    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}

bool Engine::Init(int width, int height, const char* title) {
    srand(static_cast<unsigned int>(time(0)));
    AuroraConsole::Init();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "Falha ao inicializar SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) return false;

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) return false;

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) return false;

    if (const GLubyte* v = glGetString(GL_VENDOR))                   g_GpuVendor = reinterpret_cast<const char*>(v);
    if (const GLubyte* r = glGetString(GL_RENDERER))                 g_GpuRenderer = reinterpret_cast<const char*>(r);
    if (const GLubyte* ver = glGetString(GL_VERSION))                g_GlVersion = reinterpret_cast<const char*>(ver);
    if (const GLubyte* glsl = glGetString(GL_SHADING_LANGUAGE_VERSION)) g_GlslVersion = reinterpret_cast<const char*>(glsl);

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    Renderer_Init();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (!font) {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    return true;
}

void Engine::Run() {
    bool running = true;
    SDL_Event e;

    Uint64 LAST = SDL_GetPerformanceCounter();
    Uint64 NOW = LAST;
    double deltaTime = 0;

    while (running) {
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency()) * 0.001;
        LAST = NOW;

        g_ParticleInstanceData.clear();
        g_ActiveParticles = 0;

        if (g_CurrentAuraConfig.isLoaded && g_CurrentAuraConfig.count > 0) {
            int maxEmit = clamp(g_CurrentAuraConfig.count, 0, MAX_PARTICLES);
            float timeSec = (float)SDL_GetTicks() * 0.001f;

            for (int i = 0; i < maxEmit; ++i) {
                Particle& p = g_Particles[i];
                if (p.life > 0.0f) {
                    float dist = glm::length(p.position);

                    float localDeltaTime = (float)deltaTime;
                    if (g_CurrentAuraConfig.isBlackHole && g_CurrentAuraConfig.enableRelativisticTimeDilation) {
                        float r_s = g_CurrentAuraConfig.eventHorizonRadius;
                        if (dist > r_s) {
                            float timeFactor = sqrt(clamp(1.0f - (r_s / dist), 0.001f, 1.0f));
                            localDeltaTime *= timeFactor;
                        }
                        else {
                            localDeltaTime = 0.0f; // Tempo congela no horizonte de eventos
                        }
                    }

                    if (g_EnableMouseInteraction) {
                        float distToMouse = glm::length(p.position - g_MousePos3D);
                        if (distToMouse < g_MouseInfluenceRadius && distToMouse > 0.1f) {
                            glm::vec3 dirToMouse = glm::normalize(g_MousePos3D - p.position);
                            float forceFactor = (1.0f - distToMouse / g_MouseInfluenceRadius) * g_MouseForceStrength;

                            if (g_MouseInteractionMode == 0) p.velocity += dirToMouse * forceFactor * localDeltaTime;
                            else if (g_MouseInteractionMode == 1) p.velocity -= dirToMouse * forceFactor * localDeltaTime;
                            else if (g_MouseInteractionMode == 2) {
                                glm::vec3 swirlDir = glm::normalize(glm::cross(dirToMouse, glm::vec3(0.0f, 1.0f, 0.0f)));
                                p.velocity += swirlDir * forceFactor * 1.8f * localDeltaTime;
                            }
                        }
                    }

                    if (g_CurrentAuraConfig.isBlackHole && dist > 0.001f) {
                        float r_s = g_CurrentAuraConfig.eventHorizonRadius;

                        if (dist <= r_s * 1.01f) {
                            p.life = 0.0f;
                        }
                        else {
                            float effectiveDist = clamp(dist - r_s, 0.05f, 100.0f);
                            glm::vec3 pullDir = glm::normalize(-p.position);
                            float pwGravity = g_CurrentAuraConfig.attractorStrength / (effectiveDist * effectiveDist);
                            p.velocity += pullDir * pwGravity * localDeltaTime;

                            float kerrMultiplier = g_CurrentAuraConfig.isKerrBlackHole ? (g_CurrentAuraConfig.ergosphereRadius / (dist + 0.1f)) : 1.0f;
                            glm::vec3 spinDir = glm::normalize(glm::cross(-p.position, glm::vec3(0.0f, 1.0f, 0.0f)));
                            float frameDragging = (g_CurrentAuraConfig.lenseThirringFrameDragging * kerrMultiplier) / (dist * dist + 0.1f);
                            p.velocity += spinDir * frameDragging * localDeltaTime;
                        }
                    }
                    else {
                        if (g_CurrentAuraConfig.attractorStrength != 0.0f && dist > 0.1f) {
                            glm::vec3 pullDir = glm::normalize(-p.position);
                            p.velocity += pullDir * (g_CurrentAuraConfig.attractorStrength / (dist * dist + 1.0f)) * localDeltaTime;
                        }

                        if (g_CurrentAuraConfig.vortexSpeed != 0.0f && dist > 0.01f) {
                            glm::vec3 spinDir = glm::normalize(glm::cross(-p.position, glm::vec3(0.0f, 1.0f, 0.0f)));
                            p.velocity += spinDir * g_CurrentAuraConfig.vortexSpeed * localDeltaTime;
                        }
                    }

                    if (g_CurrentAuraConfig.turbulencePower > 0.0f) {
                        float freq = g_CurrentAuraConfig.turbulenceFreq;
                        glm::vec3 noise(
                            sin(p.position.y * freq + timeSec),
                            cos(p.position.z * freq + timeSec),
                            sin(p.position.x * freq + timeSec)
                        );
                        p.velocity += noise * g_CurrentAuraConfig.turbulencePower * localDeltaTime;
                    }

                    p.position += p.velocity * localDeltaTime;
                    p.velocity = p.velocity * g_CurrentAuraConfig.damping + g_CurrentAuraConfig.gravity * localDeltaTime;

                    p.life -= localDeltaTime;
                    float t = 1.0f - clamp(p.life / p.maxLife, 0.0f, 1.0f);

                    p.color = glm::mix(g_CurrentAuraConfig.colorStart, g_CurrentAuraConfig.colorEnd, smoothstep(0.0f, 1.0f, t));
                    p.size = glm::mix(g_CurrentAuraConfig.sizeStart, g_CurrentAuraConfig.sizeEnd, t);

                    g_ParticleInstanceData.push_back(p.position.x);
                    g_ParticleInstanceData.push_back(p.position.y);
                    g_ParticleInstanceData.push_back(p.position.z);
                    g_ParticleInstanceData.push_back(p.velocity.x);
                    g_ParticleInstanceData.push_back(p.velocity.y);
                    g_ParticleInstanceData.push_back(p.velocity.z);
                    g_ParticleInstanceData.push_back(p.color.r);
                    g_ParticleInstanceData.push_back(p.color.g);
                    g_ParticleInstanceData.push_back(p.color.b);
                    g_ParticleInstanceData.push_back(p.color.a);
                    g_ParticleInstanceData.push_back(p.size);

                    g_ActiveParticles++;
                }
                else if (g_AuraLoopContinuous) {
                    SpawnSingleParticle(p, glm::vec3(0.0f, 0.0f, 0.0f));
                }
            }
        }

        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) running = false;
            if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) continue;

            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
                else if (e.key.scancode == SDL_SCANCODE_F11) {
                    isFullscreen = !isFullscreen;
                    SDL_SetWindowFullscreen(window, isFullscreen);
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (e.button.button == SDL_BUTTON_LEFT) isLMBDown = true;
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (e.button.button == SDL_BUTTON_LEFT) {
                    isLMBDown = false;
                    firstMouse = true;
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                if (isLMBDown && !g_CinematicOrbitCamera) {
                    float xoffset = (float)e.motion.xrel;
                    float yoffset = (float)e.motion.yrel * -1;

                    yaw += xoffset * mouseSensitivity;
                    pitch += yoffset * mouseSensitivity;

                    if (pitch > 89.0f) pitch = 89.0f;
                    if (pitch < -89.0f) pitch = -89.0f;
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
                float scrollSpeed = 30.0f;
                cameraPos += cameraFront * e.wheel.y * scrollSpeed * (float)deltaTime;
            }
        }

        if (!ImGui::GetIO().WantCaptureKeyboard && !g_CinematicOrbitCamera) {
            const bool* state = SDL_GetKeyboardState(NULL);
            float velocity = cameraSpeed * (float)deltaTime;

            if (state[SDL_SCANCODE_W]) cameraPos += velocity * cameraFront;
            if (state[SDL_SCANCODE_S]) cameraPos -= velocity * cameraFront;
            if (state[SDL_SCANCODE_A]) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
            if (state[SDL_SCANCODE_D]) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
        }

        glClearColor(g_BgColor.r, g_BgColor.g, g_BgColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Renderer_Draw();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        drawList->AddText(ImVec2(g_GizmoScreenPos_X.x - 5, g_GizmoScreenPos_X.y - 8), IM_COL32(255, 60, 60, 255), "X");
        drawList->AddText(ImVec2(g_GizmoScreenPos_Y.x - 5, g_GizmoScreenPos_Y.y - 8), IM_COL32(60, 255, 60, 255), "Y");
        drawList->AddText(ImVec2(g_GizmoScreenPos_Z.x - 5, g_GizmoScreenPos_Z.y - 8), IM_COL32(60, 160, 255, 255), "Z");

        ImGui::SetNextWindowSize(ImVec2(800, 720), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(25, 25), ImGuiCond_FirstUseEver);

        ImGui::Begin(U8("Aurora VFX & Astro-Sim Engine v0.7 | Por Wenderson Dias"), nullptr, ImGuiWindowFlags_MenuBar);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu(U8("Ficheiro"))) {
                if (ImGui::MenuItem(U8("Carregar Malha 3D (.obj)..."))) {
                    std::string file = OpenFileDialog("Modelos Wavefront 3D (*.obj)\0*.obj\0Todos os Ficheiros (*.*)\0*.*\0", "Selecione um Modelo 3D (.obj)");
                    if (!file.empty()) g_Model.LoadModel(file);
                }
                if (ImGui::MenuItem(U8("Carregar Efeito (.AURA)..."))) {
                    std::string file = OpenFileDialog("Arquivos Aurora Effect (*.aura)\0*.aura\0Todos os Ficheiros (*.*)\0*.*\0", "Selecione um Arquivo (.AURA)");
                    if (!file.empty()) LoadAuraEffectFile(file);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(isFullscreen ? U8("Modo Janela") : U8("Tela Cheia"), "F11")) {
                    isFullscreen = !isFullscreen;
                    SDL_SetWindowFullscreen(window, isFullscreen);
                }
                ImGui::Separator();
                if (ImGui::MenuItem(U8("Sair do Motor"), "Alt+F4")) running = false;
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::BeginTabBar("AuroraTabs")) {

            // --- ABA 1: ASTRO-SIM & PRESETS ---
            if (ImGui::BeginTabItem(U8("Astro-Sim & Presets"))) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), U8("Biblioteca de Fenômenos Astrofísicos e Efeitos de Jogos"));
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.2f, 1.0f));
                if (ImGui::Button(U8("BURACO NEGRO DE KERR (ROTACIONAL / ERGOSFERA)"), ImVec2(-FLT_MIN, 45))) LoadPresetEffect(0);
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.4f, 0.8f, 1.0f));
                if (ImGui::Button(U8("PULSAR DE NEUTRÕES / QUASAR POLAR"), ImVec2(-FLT_MIN, 45))) LoadPresetEffect(1);
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
                if (ImGui::Button(U8("NEBULOSA / REMANESCENTE DE SUPERNOVA"), ImVec2(-FLT_MIN, 45))) LoadPresetEffect(2);
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.3f, 1.0f));
                if (ImGui::Button(U8("VÓRTICE ARCANO / PORTAL INTERDIMENSIONAL (VFX)"), ImVec2(-FLT_MIN, 45))) LoadPresetEffect(3);
                ImGui::PopStyleColor();

                ImGui::Separator();
                ImGui::Checkbox(U8("Ativar Modo de Câmara Cinemática Orbital (360°)"), &g_CinematicOrbitCamera);
                ImGui::Checkbox(U8("Ativar Tremor de Câmara Cinemático (Screen Shake)"), &g_EnableScreenShake);
                ImGui::EndTabItem();
            }

            // --- ABA 2: IDE DE TEXTO AO VIVO ---
            if (ImGui::BeginTabItem(U8("IDE / AURA Live Editor"))) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 1.0f, 1.0f), U8("Ambiente de Programação Ao Vivo (.AURA)"));
                ImGui::Separator();

                if (ImGui::Button(U8("Carregar Ficheiro (.aura)"), ImVec2(180, 30))) {
                    std::string file = OpenFileDialog("Arquivos Aurora (*.aura)\0*.aura\0Todos os Ficheiros (*.*)\0*.*\0", "Selecione um Arquivo");
                    if (!file.empty()) LoadAuraEffectFile(file);
                }
                ImGui::SameLine();
                if (ImGui::Button(U8("Salvar Editor no Disco"), ImVec2(180, 30))) {
                    SaveAuraEffectFile("./live_edit_ultimate.aura");
                }
                ImGui::SameLine();
                if (ImGui::Button(U8("Sincronizar da GUI"), ImVec2(160, 30))) {
                    SyncConfigToTextEditorBuffer();
                }

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.9f, 0.4f, 1.0f));
                if (ImGui::Button(U8("⚡ COMPILAR E APLICAR CÓDIGO AO VIVO (IN-MEMORY)"), ImVec2(-FLT_MIN, 40))) {
                    std::string codeStr(g_AuraTextEditorBuffer);
                    std::istringstream stream(codeStr);
                    ParseAuraFromStream(stream, "MEMÓRIA_IDE_AO_VIVO");
                    SyncConfigToTextEditorBuffer();
                }
                ImGui::PopStyleColor(2);

                ImGui::Spacing();
                ImGui::Text(U8("Editor de Sintaxe AURA (Edite os parâmetros abaixo livremente):"));
                ImGui::InputTextMultiline("##AuraCodeEditor", g_AuraTextEditorBuffer, IM_ARRAYSIZE(g_AuraTextEditorBuffer), ImVec2(-FLT_MIN, 380), ImGuiInputTextFlags_AllowTabInput);
                ImGui::EndTabItem();
            }

            // --- ABA 3: GUI / PARÂMETROS DE EFEITOS AO VIVO ---
            if (ImGui::BeginTabItem(U8("GUI / Efeitos (.AURA)"))) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 1.0f, 1.0f), U8("Controle Visual Bidirecional (Qualquer mudança atualiza a IDE)"));
                ImGui::Separator();

                if (ImGui::Button(U8("Carregar (.aura)"), ImVec2(180, 30))) {
                    std::string file = OpenFileDialog("Arquivos Aurora (*.aura)\0*.aura\0Todos os Ficheiros (*.*)\0*.*\0", "Selecione um Arquivo");
                    if (!file.empty()) LoadAuraEffectFile(file);
                }
                ImGui::SameLine();
                if (ImGui::Button(U8("Salvar (.aura)"), ImVec2(180, 30))) {
                    SaveAuraEffectFile("./meu_efeito_gui.aura");
                }
                ImGui::SameLine();
                ImGui::Checkbox(U8("Loop Contínuo"), &g_AuraLoopContinuous);

                if (ImGui::CollapsingHeader(U8("Física Relativística (Schwarzschild & Kerr)"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Checkbox(U8("Ativar Singularidade"), &g_CurrentAuraConfig.isBlackHole)) SyncConfigToTextEditorBuffer();
                    if (g_CurrentAuraConfig.isBlackHole) {
                        if (ImGui::SliderFloat(U8("Raio de Horizonte (r_s)"), &g_CurrentAuraConfig.eventHorizonRadius, 0.2f, 4.0f, "%.2f")) SyncConfigToTextEditorBuffer();
                        if (ImGui::Checkbox(U8("Buraco Negro em Rotação (Métrica de Kerr)"), &g_CurrentAuraConfig.isKerrBlackHole)) SyncConfigToTextEditorBuffer();
                        if (g_CurrentAuraConfig.isKerrBlackHole) {
                            if (ImGui::SliderFloat(U8("Parâmetro de Spin (a)"), &g_CurrentAuraConfig.kerrSpinParam, 0.0f, 1.0f, "%.2f")) SyncConfigToTextEditorBuffer();
                            if (ImGui::SliderFloat(U8("Raio da Ergosfera"), &g_CurrentAuraConfig.ergosphereRadius, 1.5f, 6.0f, "%.2f")) SyncConfigToTextEditorBuffer();
                        }
                        if (ImGui::SliderFloat(U8("Arrasto Lense-Thirring"), &g_CurrentAuraConfig.lenseThirringFrameDragging, 0.0f, 60.0f, "%.1f")) SyncConfigToTextEditorBuffer();
                        if (ImGui::Checkbox(U8("Lente Gravitacional de Einstein"), &g_CurrentAuraConfig.enableGravitationalLensing)) SyncConfigToTextEditorBuffer();
                        if (ImGui::Checkbox(U8("Efeito Doppler Beaming (Blue/Redshift)"), &g_CurrentAuraConfig.enableDopplerBeaming)) SyncConfigToTextEditorBuffer();
                        if (ImGui::Checkbox(U8("Dilatação Temporal (Tempo Congela no Horizonte)"), &g_CurrentAuraConfig.enableRelativisticTimeDilation)) SyncConfigToTextEditorBuffer();
                        if (ImGui::Checkbox(U8("Radiação de Hawking (Evaporação Quântica)"), &g_CurrentAuraConfig.enableHawkingRadiation)) SyncConfigToTextEditorBuffer();

                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.7f, 1.0f));
                        if (ImGui::Button(U8("REGERAR MALHA 3D COM ERGOSFERA DE KERR AGORA"), ImVec2(-FLT_MIN, 32))) {
                            GenerateSpacetimeDistortionModel(g_CurrentAuraConfig);
                        }
                        ImGui::PopStyleColor();
                    }
                }

                if (ImGui::CollapsingHeader(U8("Emissor & Forças"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::Combo(U8("Formato do Emissor"), &g_CurrentAuraConfig.emissionShape, "Esférico 3D\0Disco de Acreção\0VFX Portal/Caixa\0\0")) SyncConfigToTextEditorBuffer();
                    if (ImGui::DragFloatRange2(U8("Raio Mín / Máx"), &g_CurrentAuraConfig.radiusMin, &g_CurrentAuraConfig.radiusMax, 0.1f, 0.0f, 50.0f)) SyncConfigToTextEditorBuffer();
                    if (ImGui::SliderInt(U8("Contagem de Partículas"), &g_CurrentAuraConfig.count, 100, MAX_PARTICLES)) SyncConfigToTextEditorBuffer();
                    if (ImGui::SliderFloat(U8("Força Atratora"), &g_CurrentAuraConfig.attractorStrength, -50.0f, 70.0f, "%.1f")) SyncConfigToTextEditorBuffer();
                    if (ImGui::SliderFloat(U8("Vórtice Angular"), &g_CurrentAuraConfig.vortexSpeed, -50.0f, 50.0f, "%.1f")) SyncConfigToTextEditorBuffer();
                    if (ImGui::SliderFloat(U8("Caos Quântico (Turbulência)"), &g_CurrentAuraConfig.turbulencePower, 0.0f, 20.0f, "%.2f")) SyncConfigToTextEditorBuffer();
                }

                if (ImGui::CollapsingHeader(U8("Cores e Interpolação Visual"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::DragFloatRange2(U8("Velocidade Mín / Máx"), &g_CurrentAuraConfig.speedMin, &g_CurrentAuraConfig.speedMax, 0.2f, 0.0f, 100.0f)) SyncConfigToTextEditorBuffer();
                    if (ImGui::DragFloatRange2(U8("Tamanho Início / Fim"), &g_CurrentAuraConfig.sizeStart, &g_CurrentAuraConfig.sizeEnd, 0.01f, 0.0f, 5.0f)) SyncConfigToTextEditorBuffer();
                    if (ImGui::ColorEdit4(U8("Cor Inicial (Nascimento)"), glm::value_ptr(g_CurrentAuraConfig.colorStart))) SyncConfigToTextEditorBuffer();
                    if (ImGui::ColorEdit4(U8("Cor Final (Dissipação)"), glm::value_ptr(g_CurrentAuraConfig.colorEnd))) SyncConfigToTextEditorBuffer();
                }

                ImGui::Spacing();
                if (ImGui::Button(U8("APLICAR MODIFICAÇÕES AGORA"), ImVec2(-FLT_MIN, 38))) {
                    TriggerAuraEffect(glm::vec3(0.0f, 0.0f, 0.0f));
                }
                ImGui::Text(U8("Partículas na GPU: %d / %d"), g_ActiveParticles, MAX_PARTICLES);
                ImGui::EndTabItem();
            }

            // --- ABA 4: INTERAÇÃO E GEOMETRIA 3D ---
            if (ImGui::BeginTabItem(U8("Interação & Geometria 3D"))) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), U8("Controle Geométrico e Visibilidade do Modelo 3D (.obj)"));
                ImGui::Separator();

                ImGui::Checkbox(U8("Exibir Modelo 3D do Buraco Negro na Cena"), &g_ShowBlackHoleModel);
                ImGui::DragFloat3(U8("Posição XYZ (Gizmo)"), glm::value_ptr(g_ModelPosition), 0.1f, -50.0f, 50.0f);
                ImGui::DragFloat3(U8("Rotação (Graus XYZ)"), glm::value_ptr(g_ModelRotation), 0.5f, -360.0f, 360.0f);
                ImGui::DragFloat3(U8("Escala XYZ (Tamanho)"), glm::value_ptr(g_ModelScaleVec), 0.05f, 0.01f, 10.0f);
                if (ImGui::Button(U8("Repor Geometria do Modelo para Padrão"))) {
                    g_ModelPosition = glm::vec3(0.0f); g_ModelRotation = glm::vec3(0.0f); g_ModelScaleVec = glm::vec3(0.5f);
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), U8("Interação em Espaço 3D com Cursor (Raycasting)"));
                ImGui::Separator();
                ImGui::Checkbox(U8("Ativar Campo de Força do Cursor do Rato"), &g_EnableMouseInteraction);
                if (g_EnableMouseInteraction) {
                    ImGui::Combo(U8("Modo de Força"), &g_MouseInteractionMode, "Atrator (Puxar)\0Repulsor (Empurrar)\0Vórtice (Girar / Furacão)\0\0");
                    ImGui::SliderFloat(U8("Força Gravitacional do Rato"), &g_MouseForceStrength, 5.0f, 100.0f, "%.1f");
                    ImGui::SliderFloat(U8("Raio de Influência 3D"), &g_MouseInfluenceRadius, 1.0f, 20.0f, "%.1f");
                }
                ImGui::EndTabItem();
            }

            // --- ABA 5: DIAGNÓSTICO DE HARDWARE E VRAM ---
            if (ImGui::BeginTabItem(U8("Diagnóstico HW & VRAM"))) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), U8("Leitura Real de Driver & Placa Gráfica (GPU)"));
                ImGui::Separator();
                ImGui::Text(U8("Placa Gráfica: %s"), g_GpuRenderer.c_str());
                ImGui::Text(U8("Fabricante do Chip: %s"), g_GpuVendor.c_str());
                ImGui::Text(U8("Versão do Driver OpenGL: %s"), g_GlVersion.c_str());
                ImGui::Text(U8("Linguagem Shading (GLSL): %s"), g_GlslVersion.c_str());
                ImGui::Spacing();

                int maxTexSize = 0, maxUniforms = 0, maxAttribs = 0;
                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
                glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniforms);
                glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);

                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), U8("Limites Reais de Hardware OpenGL:"));
                ImGui::BulletText(U8("Resolução Máxima de Textura: %d x %d px"), maxTexSize, maxTexSize);
                ImGui::BulletText(U8("Máximo de Atributos por Vértice: %d canais"), maxAttribs);
                ImGui::BulletText(U8("Máximo de Uniform Buffers: %d bindings"), maxUniforms);
                ImGui::Spacing();

                int totalVRAM = 0, curVRAM = 0;
                glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_VIDMEM_NVX, &totalVRAM);
                glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &curVRAM);

                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), U8("Estatísticas de Memória VRAM (NVIDIA/AMD Direct Query):"));
                if (totalVRAM > 0) {
                    float totalMB = (float)totalVRAM / 1024.0f;
                    float curMB = (float)curVRAM / 1024.0f;
                    float usedMB = totalMB - curMB;
                    ImGui::Text(U8("VRAM Total da GPU: %.1f MB"), totalMB);
                    ImGui::Text(U8("VRAM Em Uso Agora: %.1f MB (%.1f%%)"), usedMB, (usedMB / totalMB) * 100.0f);
                    ImGui::Text(U8("VRAM Livre Disponível: %.1f MB"), curMB);
                    ImGui::ProgressBar(usedMB / totalMB, ImVec2(-FLT_MIN, 20), "Uso de Memória de Vídeo");
                }
                else {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), U8("Nota: A extensão de leitura direta de VRAM não é suportada por este driver gráfico específico."));
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), U8("Desempenho Em Tempo Real:"));
                ImGui::Separator();
                ImGui::Text(U8("Taxa de Quadros: %.1f FPS"), ImGui::GetIO().Framerate);
                ImGui::Text(U8("Tempo por Quadro: %.3f milissegundos"), 1000.0f / ImGui::GetIO().Framerate);
                ImGui::Text(U8("Total de Partículas Ativas na Memória: %d / %d"), g_ActiveParticles, MAX_PARTICLES);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(U8("Console"))) {
                ImGui::Spacing();
                if (ImGui::Button(U8("Limpar Registos"), ImVec2(140, 28))) AuroraConsole::g_ConsoleBuffer.Clear();
                ImGui::Separator();
                ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                {
                    std::lock_guard<std::mutex> lock(AuroraConsole::g_ConsoleBuffer.mutex);
                    ImGui::TextUnformatted(AuroraConsole::g_ConsoleBuffer.buffer.c_str());
                }
                if (AuroraConsole::g_ConsoleBuffer.scrollToBottom) {
                    ImGui::SetScrollHereY(1.0f);
                    AuroraConsole::g_ConsoleBuffer.scrollToBottom = false;
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(U8("Cena & Configs"))) {
                ImGui::Spacing();
                if (ImGui::CollapsingHeader(U8("Ambiente & Viewport"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::ColorEdit3(U8("Cor de Fundo"), glm::value_ptr(g_BgColor));
                    ImGui::Checkbox(U8("Exibir Grade Espacial"), &g_ShowGrid);
                    ImGui::Checkbox(U8("Modo Wireframe"), &g_Wireframe);
                    if (ImGui::Checkbox(U8("Ecrã Cheio (F11)"), &isFullscreen)) SDL_SetWindowFullscreen(window, isFullscreen);
                }

                if (ImGui::CollapsingHeader(U8("Câmara Livre 3D"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::SliderFloat(U8("Campo de Visão (FoV)"), &g_Fov, 30.0f, 120.0f, "%.1f°");
                    ImGui::SliderFloat(U8("Velocidade"), &cameraSpeed, 1.0f, 30.0f, "%.1f");
                    ImGui::SliderFloat(U8("Sensibilidade"), &mouseSensitivity, 0.05f, 0.5f, "%.2f");
                    if (ImGui::Button(U8("Repor Posição Inicial"))) {
                        cameraPos = glm::vec3(0.0f, 6.0f, 18.0f); yaw = -90.0f; pitch = -20.0f;
                        g_CinematicOrbitCamera = false;
                    }
                }
                ImGui::EndTabItem();
            }

            // --- ABA 7: CRÉDITOS & ARQUITETURA DO SISTEMA ---
            if (ImGui::BeginTabItem(U8("Créditos & Info"))) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), U8("Aurora VFX & Astro-Sim Engine v0.7.0 Ultimate Pro"));
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Destaque de Crédito para o Desenvolvedor
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f), U8("DESENVOLVIDO POR:"));
                ImGui::SetWindowFontScale(1.3f);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), U8("★ Wenderson Dias ★"));
                ImGui::SetWindowFontScale(1.0f);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.7f, 1.0f), U8("Siliconarch Softwares | Arquitetura Gráfica & Simulação Científica"));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), U8("Especificações Arquiteturais & Dependências:"));
                ImGui::BulletText(U8("API Gráfica: OpenGL Core Profile v3.3+ (Instanced Rendering / GLSL Core)"));
                ImGui::BulletText(U8("Gestão de Contexto e Janela: SDL3 (Simple DirectMedia Layer 3)"));
                ImGui::BulletText(U8("Carregador de Extensões OpenGL: GLAD"));
                ImGui::BulletText(U8("Importador Geométrico 3D: Assimp (Open Asset Import Library)"));
                ImGui::BulletText(U8("Álgebra Linear e Matemática Matricial: GLM (OpenGL Mathematics)"));
                ImGui::BulletText(U8("GUI Dinâmica Em Tempo Real: Dear ImGui (Docking & Multiline IDE)"));
                ImGui::BulletText(U8("Física Astrofísica: Métrica de Schwarzschild, Ergosfera de Kerr e Lente de Einstein"));
                ImGui::BulletText(U8("Mecânica Quântica: Texturização Procedural por Ruído de Plasma & Evaporação de Hawking"));

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextWrapped(U8("O Aurora VFX foi projetado como um motor unificado que mescla simulações astrofísicas reais com ferramentas de VFX de alto desempenho para jogos eletrônicos."));
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }
}

void Engine::Cleanup() {
    AuroraConsole::Cleanup();

    glDeleteProgram(gizmoShaderProgram);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(gridShaderProgram);
    glDeleteProgram(particleShader);
    glDeleteVertexArrays(1, &particleVAO);
    glDeleteBuffers(1, &particleVBO_Quad);
    glDeleteBuffers(1, &particleVBO_Instance);

    if (window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
    std::cout << "[AURORA WENDERSON DIAS] Motor finalizado com absoluto sucesso. Todos os recursos limpos!" << std::endl;
}