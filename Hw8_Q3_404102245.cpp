#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
using namespace std;
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 800;
bool is_3d = false;
int n_depth = 0;
SDL_Color current_color = {255, 255, 255, 255};
struct Point3D {
    float x, y, z;
};
SDL_Point project(Point3D p) {
    float angle = M_PI / 6.0;
    int screen_x = (int)((p.y - p.x) * cos(angle)) + SCREEN_WIDTH / 2;
    int screen_y = (int)(-(p.z) + (p.x + p.y) * sin(angle)) + SCREEN_HEIGHT / 2;
    return { screen_x, screen_y };
}
void drawLine3D(SDL_Renderer* renderer, Point3D p1, Point3D p2) {
    SDL_Point s1 = project(p1);
    SDL_Point s2 = project(p2);
    SDL_RenderDrawLine(renderer, s1.x, s1.y, s2.x, s2.y);
}
void drawH2D(SDL_Renderer* renderer, float x, float y, float len, int depth) {
    if (depth <= 0) return;
    float h = len / 2;
    SDL_RenderDrawLine(renderer, x - h + SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - y, x + h + SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - y);
    SDL_RenderDrawLine(renderer, x - h + SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - (y - h), x - h + SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - (y + h));
    SDL_RenderDrawLine(renderer, x + h + SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - (y - h), x + h + SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - (y + h));
    drawH2D(renderer, x - h, y + h, len / 2, depth - 1);
    drawH2D(renderer, x - h, y - h, len / 2, depth - 1);
    drawH2D(renderer, x + h, y + h, len / 2, depth - 1);
    drawH2D(renderer, x + h, y - h, len / 2, depth - 1);
}
void drawH3D(SDL_Renderer* renderer, Point3D p, float len, int depth, int axis) {
    if (depth <= 0) return;
    float h = len / 2;
    Point3D p1, p2;
    if (axis == 0) {
        p1 = {p.x, p.y, p.z - h}; p2 = {p.x, p.y, p.z + h};
        drawLine3D(renderer, p1, p2);
        drawH3D(renderer, p1, len * 0.7, depth - 1, 1);
        drawH3D(renderer, p2, len * 0.7, depth - 1, 1);
    }
    else if (axis == 1) {
        p1 = {p.x, p.y - h, p.z}; p2 = {p.x, p.y + h, p.z};
        drawLine3D(renderer, p1, p2);
        drawH3D(renderer, p1, len * 0.7, depth - 1, 2);
        drawH3D(renderer, p2, len * 0.7, depth - 1, 2);
    }
    else {
        p1 = {p.x - h, p.y, p.z}; p2 = {p.x + h, p.y, p.z};
        drawLine3D(renderer, p1, p2);
        drawH3D(renderer, p1, len * 0.7, depth - 1, 0);
        drawH3D(renderer, p2, len * 0.7, depth - 1, 0);
    }
}
void runInputWindow() {
    SDL_Window* win = SDL_CreateWindow("Settings", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 400, 300, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("arial.ttf", 20);
    bool done = false;
    string input_n = "";
    SDL_StartTextInput();
    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) exit(0);
            if (e.type == SDL_TEXTINPUT) {
                if (isdigit(e.text.text[0])) input_n += e.text.text;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && !input_n.empty()) input_n.pop_back();
                if (e.key.keysym.sym == SDLK_RETURN && !input_n.empty()) {
                    n_depth = stoi(input_n);
                    done = true;
                }
                if (e.key.keysym.sym == SDLK_2) is_3d = false;
                if (e.key.keysym.sym == SDLK_3) is_3d = true;
            }
        }
        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);
        SDL_Color white = {255, 255, 255};
        auto renderT = [&](string t, int y) {
            SDL_Surface* s = TTF_RenderText_Solid(font, t.c_str(), white);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, s);
            SDL_Rect r = {50, y, s->w, s->h};
            SDL_RenderCopy(ren, tex, NULL, &r);
            SDL_FreeSurface(s); SDL_DestroyTexture(tex);
        };
        renderT("Enter Depth (n): " + input_n, 50);
        renderT("Press '2' for 2D, '3' for 3D", 100);
        renderT("Mode: " + string(is_3d ? "3D" : "2D"), 150);
        renderT("Press ENTER to start", 220);
        SDL_RenderPresent(ren);
    }
    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
}
int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) return -1;
    runInputWindow();
    SDL_Window* window = SDL_CreateWindow("H-Fractal Viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_x: quit = true; break;
                    case SDLK_r: current_color = {255, 0, 0, 255}; break;
                    case SDLK_g: current_color = {0, 255, 0, 255}; break;
                    case SDLK_b: current_color = {0, 0, 255, 255}; break;
                    case SDLK_y: current_color = {255, 255, 0, 255}; break;
                    case SDLK_w: current_color = {255, 255, 255, 255}; break;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, current_color.r, current_color.g, current_color.b, 255);
        if (is_3d) {
            drawH3D(renderer, {0, 0, 0}, 250, n_depth, 0);
        } else {
            drawH2D(renderer, 0, 0, 300, n_depth);
        }
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
