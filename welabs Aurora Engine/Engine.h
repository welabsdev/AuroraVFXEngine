#pragma once
#include <SDL3/SDL.h> 
#include <iostream>

extern SDL_Window* window;

void Renderer_Init();
void Renderer_Draw();

class Engine {
public:
    Engine() = default;
    ~Engine() = default;

    bool Init(int width, int height, const char* title);
    void Run();
    void Cleanup();
};