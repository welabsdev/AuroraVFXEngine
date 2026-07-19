// Posição calculada com base na câmera para o quadrado ficar sempre de frente
const char* particleVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos; // Posição local do quadrado (-0.5 a 0.5)
    layout (location = 1) in vec3 offset; // Posição da partícula no espaço
    layout (location = 2) in vec4 color;  // Cor e opacidade (Alpha)
    layout (location = 3) in float size;  // Tamanho dinâmico

    out vec4 ParticleColor;
    out vec2 TexCoords;

    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        ParticleColor = color;
        TexCoords = aPos.xy + vec2(0.5); // Coordenadas de 0.0 a 1.0
        
        // Técnica de Billboarding na GPU: extrai os vetores Right e Up da matriz View
        vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 cameraUp    = vec3(view[0][1], view[1][1], view[2][1]);
        
        vec3 vertexPosition = offset 
            + cameraRight * aPos.x * size 
            + cameraUp    * aPos.y * size;

        gl_Position = projection * view * vec4(vertexPosition, 1.0);
    }
)";

const char* particleFragmentShaderSource = R"(
    #version 330 core
    in vec4 ParticleColor;
    in vec2 TexCoords;
    out vec4 FragColor;

    void main() {
        // Calcula a distância do centro do quadrado para criar uma partícula redonda e suave
        float distance = length(TexCoords - vec2(0.5));
        if (distance > 0.5) discard; // Corta os cantos do quadrado

        // Suaviza a borda (Efeito de gás/brilho)
        float intensity = smoothstep(0.5, 0.0, distance);
        
        // Multiplica a cor base pela intensidade do brilho esférico
        FragColor = vec4(ParticleColor.rgb, ParticleColor.a * intensity);
    }
)";