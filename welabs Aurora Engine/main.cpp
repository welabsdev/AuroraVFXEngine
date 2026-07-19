#include "Engine.h"
#include <SDL3/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {
    Engine app;

    // 1. Inicializamos o vídeo primeiro para o SDL conseguir ler o hardware do monitor
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "[AURORA ERRO] Falha ao inicializar SDL Video: " << SDL_GetError() << std::endl;
        return -1;
    }

    // 2. Descobrimos o Monitor Principal (Primary Display) e a sua resolução nativa
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* displayMode = SDL_GetCurrentDisplayMode(displayID);

    // Resoluções de segurança padrão (caso o sistema operativo falhe a leitura)
    int windowWidth = 1440;
    int windowHeight = 900;


    if (displayMode) {
        // 3. Calculamos 85% da resolução nativa para abrir centralizado sem cortar na barra de tarefas
        windowWidth = static_cast<int>(displayMode->w * 0.85f);
        windowHeight = static_cast<int>(displayMode->h * 0.85f);

        std::cout << "[AURORA HARDWARE] Monitor Detetado: " << displayMode->w << "x" << displayMode->h << "px" << std::endl;
        std::cout << "[AURORA ENGINE] Viewport inicial configurada para: " << windowWidth << "x" << windowHeight << "px (" << displayMode->refresh_rate << " Hz)" << std::endl;
    }
    else {
        std::cout << "[AURORA AVISO] Não foi possível ler a resolução do monitor. A usar padrão 1440x900." << std::endl;
    }

    // 4. Inicia a Engine com as dimensões adaptadas ao monitor do utilizador
    if (app.Init(windowWidth, windowHeight, "Aurora VFX Engine | Apha 0.7.0 ")) {
        std::cout << "[AURORA] Engine inicializada com sucesso!" << std::endl;
        app.Run();
    }

    app.Cleanup();
    return 0;
}