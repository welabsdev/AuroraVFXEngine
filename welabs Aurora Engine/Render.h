#pragma once
#include <glad/glad.h>
#include "Model.h" 
#include <SDL3/SDL.h>

extern unsigned int shaderProgram;
extern Model g_Model;
extern SDL_Window* window;

void Renderer_Init();
void Renderer_Draw();