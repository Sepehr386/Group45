// Scratch-like IDE — Pure SDL2 with Bitmap Font (NO SDL_ttf)
// FIXED & ENHANCED VERSION
// Compile:
//   g++ -std=c++17 -o scratch scratch.cpp $(sdl2-config --cflags --libs) -lm

#include <SDL2/SDL.h>
#include <bits/stdc++.h>
#include <SDL_image.h>

using namespace std;

// ════════════════════════════════════════════
//  Constants (base design size)
// ════════════════════════════════════════════
static const int   BASE_WIDTH            = 1280;
static const int   BASE_HEIGHT           = 720;

static const float BASE_BLOCK_WIDTH      = 200.0f;
static const float BASE_BLOCK_HEIGHT     = 50.0f;
static const float BASE_CBLOCK_MIN_H     = 90.0f;
static const float BASE_CBLOCK_MOUTH_H   = 40.0f;
static const float BASE_CBLOCK_BAR_H     = 20.0f;
static const float BASE_BLOCK_CORNER_R   = 8.0f;
static const float BASE_SNAP_DISTANCE    = 55.0f;
static const float BASE_SNAP_VERT_OVERLAP= 6.0f;

static const int   BASE_TOOLBAR_HEIGHT   = 45;
static const int   BASE_PALETTE_WIDTH    = 360;
static const int   BASE_CAT_BTN_HEIGHT   = 40;
static const int   BASE_CAT_BTN_WIDTH    = 120;
static const int   BASE_CAT_PANEL_WIDTH  = 130;
static const int   BASE_STAGE_WIDTH      = 360;
static const int   BASE_STAGE_HEIGHT     = 270;
static const int   BASE_SPRITE_THUMB     = 70;

// ════════════════════════════════════════════
//  Scaling system
// ════════════════════════════════════════════
struct LayoutScale {
    float sx, sy, s;
    int winW, winH;

    int   TOOLBAR_HEIGHT;
    int   PALETTE_WIDTH;
    int   CAT_PANEL_WIDTH;
    int   CAT_BTN_HEIGHT;
    int   CAT_BTN_WIDTH;
    int   STAGE_WIDTH;
    int   STAGE_HEIGHT;
    int   SPRITE_THUMB;
    float BLOCK_WIDTH;
    float BLOCK_HEIGHT;
    float CBLOCK_MIN_H;
    float CBLOCK_MOUTH_H;
    float CBLOCK_BAR_H;
    float BLOCK_CORNER_R;
    float SNAP_DISTANCE;
    float SNAP_VERT_OVERLAP;
    int   fontScale;

    void update(int w, int h) {
        winW = w;  winH = h;
        sx = (float)w / BASE_WIDTH;
        sy = (float)h / BASE_HEIGHT;
        s  = min(sx, sy);
        if (s < 1.0f) s = 1.0f;

        TOOLBAR_HEIGHT   = (int)(BASE_TOOLBAR_HEIGHT   * sy);
        PALETTE_WIDTH    = (int)(BASE_PALETTE_WIDTH    * sx);
        CAT_PANEL_WIDTH  = (int)(BASE_CAT_PANEL_WIDTH  * sx);
        CAT_BTN_HEIGHT   = (int)(BASE_CAT_BTN_HEIGHT   * sy);
        CAT_BTN_WIDTH    = (int)(BASE_CAT_BTN_WIDTH    * sx);
        STAGE_WIDTH      = (int)(BASE_STAGE_WIDTH      * sx);
        STAGE_HEIGHT     = (int)(BASE_STAGE_HEIGHT     * sy);
        SPRITE_THUMB     = (int)(BASE_SPRITE_THUMB     * s);
        BLOCK_WIDTH      = BASE_BLOCK_WIDTH  * s;
        BLOCK_HEIGHT     = BASE_BLOCK_HEIGHT * s;
        CBLOCK_MIN_H     = BASE_CBLOCK_MIN_H * s;
        CBLOCK_MOUTH_H   = BASE_CBLOCK_MOUTH_H * s;
        CBLOCK_BAR_H     = BASE_CBLOCK_BAR_H * s;
        BLOCK_CORNER_R   = BASE_BLOCK_CORNER_R * s;
        SNAP_DISTANCE    = BASE_SNAP_DISTANCE * s;
        SNAP_VERT_OVERLAP= BASE_SNAP_VERT_OVERLAP * s;

        fontScale = max(1, (int)(s + 0.5f));
        if (fontScale < 1) fontScale = 1;
    }
};
static LayoutScale L;

// ════════════════════════════════════════════
//  Bitmap Font 5x7
// ════════════════════════════════════════════
static const int GLYPH_W = 5;
static const int GLYPH_H = 7;
static const int FONT_FIRST_CHAR = 32;
static const int FONT_LAST_CHAR  = 126;

static const unsigned char FONT_5x7[][7] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x04,0x04,0x04,0x04,0x00,0x04,0x00}, // 33 '!'
    {0x0A,0x0A,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
    {0x0A,0x1F,0x0A,0x1F,0x0A,0x00,0x00}, // 35 '#'
    {0x04,0x0F,0x14,0x0E,0x05,0x1E,0x04}, // 36 '$'
    {0x18,0x19,0x02,0x04,0x08,0x13,0x03}, // 37 '%'
    {0x08,0x14,0x14,0x08,0x15,0x12,0x0D}, // 38 '&'
    {0x04,0x04,0x00,0x00,0x00,0x00,0x00}, // 39 '\''
    {0x02,0x04,0x08,0x08,0x08,0x04,0x02}, // 40 '('
    {0x08,0x04,0x02,0x02,0x02,0x04,0x08}, // 41 ')'
    {0x00,0x04,0x15,0x0E,0x15,0x04,0x00}, // 42 '*'
    {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}, // 43 '+'
    {0x00,0x00,0x00,0x00,0x04,0x04,0x08}, // 44 ','
    {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}, // 45 '-'
    {0x00,0x00,0x00,0x00,0x00,0x04,0x00}, // 46 '.'
    {0x01,0x01,0x02,0x04,0x08,0x10,0x10}, // 47 '/'
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 48 '0'
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 49 '1'
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 50 '2'
    {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}, // 51 '3'
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 52 '4'
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 53 '5'
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 54 '6'
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 55 '7'
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 56 '8'
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 57 '9'
    {0x00,0x04,0x00,0x00,0x04,0x00,0x00}, // 58 ':'
    {0x00,0x04,0x00,0x00,0x04,0x04,0x08}, // 59 ';'
    {0x02,0x04,0x08,0x10,0x08,0x04,0x02}, // 60 '<'
    {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}, // 61 '='
    {0x08,0x04,0x02,0x01,0x02,0x04,0x08}, // 62 '>'
    {0x0E,0x11,0x01,0x02,0x04,0x00,0x04}, // 63 '?'
    {0x0E,0x11,0x17,0x15,0x17,0x10,0x0E}, // 64 '@'
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // 65 'A'
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // 66 'B'
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // 67 'C'
    {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}, // 68 'D'
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // 69 'E'
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // 70 'F'
    {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, // 71 'G'
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // 72 'H'
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // 73 'I'
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // 74 'J'
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // 75 'K'
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // 76 'L'
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // 77 'M'
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // 78 'N'
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // 79 'O'
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // 80 'P'
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // 81 'Q'
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // 82 'R'
    {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E}, // 83 'S'
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // 84 'T'
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // 85 'U'
    {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}, // 86 'V'
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // 87 'W'
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // 88 'X'
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // 89 'Y'
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // 90 'Z'
    {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E}, // 91 '['
    {0x10,0x10,0x08,0x04,0x02,0x01,0x01}, // 92 '\'
    {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E}, // 93 ']'
    {0x04,0x0A,0x11,0x00,0x00,0x00,0x00}, // 94 '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x1F}, // 95 '_'
    {0x08,0x04,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
    {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F}, // 97 'a'
    {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E}, // 98 'b'
    {0x00,0x00,0x0E,0x11,0x10,0x11,0x0E}, // 99 'c'
    {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F}, // 100 'd'
    {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E}, // 101 'e'
    {0x02,0x05,0x04,0x0E,0x04,0x04,0x04}, // 102 'f'
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x0E}, // 103 'g'
    {0x10,0x10,0x16,0x19,0x11,0x11,0x11}, // 104 'h'
    {0x04,0x00,0x0C,0x04,0x04,0x04,0x0E}, // 105 'i'
    {0x02,0x00,0x06,0x02,0x02,0x12,0x0C}, // 106 'j'
    {0x10,0x10,0x12,0x14,0x18,0x14,0x12}, // 107 'k'
    {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E}, // 108 'l'
    {0x00,0x00,0x1A,0x15,0x15,0x11,0x11}, // 109 'm'
    {0x00,0x00,0x16,0x19,0x11,0x11,0x11}, // 110 'n'
    {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E}, // 111 'o'
    {0x00,0x00,0x1E,0x11,0x1E,0x10,0x10}, // 112 'p'
    {0x00,0x00,0x0F,0x11,0x0F,0x01,0x01}, // 113 'q'
    {0x00,0x00,0x16,0x19,0x10,0x10,0x10}, // 114 'r'
    {0x00,0x00,0x0E,0x10,0x0E,0x01,0x1E}, // 115 's'
    {0x04,0x04,0x0E,0x04,0x04,0x05,0x02}, // 116 't'
    {0x00,0x00,0x11,0x11,0x11,0x13,0x0D}, // 117 'u'
    {0x00,0x00,0x11,0x11,0x11,0x0A,0x04}, // 118 'v'
    {0x00,0x00,0x11,0x11,0x15,0x15,0x0A}, // 119 'w'
    {0x00,0x00,0x11,0x0A,0x04,0x0A,0x11}, // 120 'x'
    {0x00,0x00,0x11,0x11,0x0F,0x01,0x0E}, // 121 'y'
    {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F}, // 122 'z'
    {0x02,0x04,0x04,0x08,0x04,0x04,0x02}, // 123 '{'
    {0x04,0x04,0x04,0x04,0x04,0x04,0x04}, // 124 '|'
    {0x08,0x04,0x04,0x02,0x04,0x04,0x08}, // 125 '}'
    {0x00,0x00,0x08,0x15,0x02,0x00,0x00}, // 126 '~'
};

static void drawChar(SDL_Renderer* rnd, int x, int y, char ch, int scale,
                     Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    int idx = (int)ch - FONT_FIRST_CHAR;
    if (idx < 0 || idx > (FONT_LAST_CHAR - FONT_FIRST_CHAR)) return;
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    const unsigned char* glyph = FONT_5x7[idx];
    for (int row = 0; row < GLYPH_H; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < GLYPH_W; col++) {
            if (bits & (1 << (GLYPH_W - 1 - col))) {
                SDL_Rect px = { x + col * scale, y + row * scale, scale, scale };
                SDL_RenderFillRect(rnd, &px);
            }
        }
    }
}

static void drawText(SDL_Renderer* rnd, int x, int y, const char* text,
                     Uint8 r, Uint8 g, Uint8 b, Uint8 a, int scale = -1)
{
    if (!text) return;
    if (scale < 0) scale = L.fontScale;
    int cx = x;
    int spacing = (GLYPH_W + 1) * scale;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n') {
            cx = x;
            y += (GLYPH_H + 2) * scale;
            continue;
        }
        drawChar(rnd, cx, y, text[i], scale, r, g, b, a);
        cx += spacing;
    }
}

static int textWidth(const char* text, int scale = -1) {
    if (!text) return 0;
    if (scale < 0) scale = L.fontScale;
    int maxW = 0, curW = 0;
    int spacing = (GLYPH_W + 1) * scale;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            if (curW > maxW) maxW = curW;
            curW = 0;
        } else {
            curW += spacing;
        }
    }
    if (curW > maxW) maxW = curW;
    return maxW;
}

static int textHeight(const char* text, int scale = -1) {
    if (!text) return 0;
    if (scale < 0) scale = L.fontScale;
    int lines = 1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') lines++;
    }
    return lines * (GLYPH_H + 2) * scale;
}

// ════════════════════════════════════════════
//  Shape helpers
// ════════════════════════════════════════════
static void fillRoundedRect(SDL_Renderer* rnd, int x, int y, int w, int h,
                            int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    SDL_Rect rc = {x + radius, y, w - 2*radius, h};
    SDL_RenderFillRect(rnd, &rc);
    SDL_Rect rl = {x, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(rnd, &rl);
    SDL_Rect rr2 = {x + w - radius, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(rnd, &rr2);
    for (int cy2 = -radius; cy2 <= radius; cy2++) {
        for (int cx2 = -radius; cx2 <= radius; cx2++) {
            if (cx2*cx2 + cy2*cy2 <= radius*radius) {
                SDL_RenderDrawPoint(rnd, x + radius + cx2, y + radius + cy2);
                SDL_RenderDrawPoint(rnd, x + w - radius + cx2, y + radius + cy2);
                SDL_RenderDrawPoint(rnd, x + radius + cx2, y + h - radius + cy2);
                SDL_RenderDrawPoint(rnd, x + w - radius + cx2, y + h - radius + cy2);
            }
        }
    }
}

static void fillEllipse(SDL_Renderer* rnd, int cx, int cy, int rx, int ry,
                        Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    for (int dy = -ry; dy <= ry; dy++) {
        int halfW = (int)(rx * sqrt(1.0 - (double)(dy*dy)/(double)(ry*ry)));
        SDL_RenderDrawLine(rnd, cx - halfW, cy + dy, cx + halfW, cy + dy);
    }
}

static void drawRoundedRectOutline(SDL_Renderer* rnd, int x, int y, int w, int h,
                                   int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_SetRenderDrawColor(rnd, r, g, b, a);
    SDL_RenderDrawLine(rnd, x + radius, y, x + w - radius, y);
    SDL_RenderDrawLine(rnd, x + radius, y + h, x + w - radius, y + h);
    SDL_RenderDrawLine(rnd, x, y + radius, x, y + h - radius);
    SDL_RenderDrawLine(rnd, x + w, y + radius, x + w, y + h - radius);
}

// ════════════════════════════════════════════
//  Category
// ════════════════════════════════════════════
enum class Category { MOTION, LOOKS, SOUND, EVENTS, CONTROL, SENSING, OPERATORS, VARIABLES };
static const int NUM_CATEGORIES = 8;

static SDL_Color catColor(Category c) {
    switch(c){
        case Category::MOTION:    return {66,133,244,255};
        case Category::LOOKS:     return {147,83,211,255};
        case Category::SOUND:     return {207,99,207,255};
        case Category::EVENTS:    return {255,191,0,255};
        case Category::CONTROL:   return {255,171,25,255};
        case Category::SENSING:   return {92,177,214,255};
        case Category::OPERATORS: return {89,192,89,255};
        case Category::VARIABLES: return {255,140,26,255};
    }
    return {128,128,128,255};
}

static const char* catName(Category c) {
    switch(c){
        case Category::MOTION:    return "Motion";
        case Category::LOOKS:     return "Looks";
        case Category::SOUND:     return "Sound";
        case Category::EVENTS:    return "Events";
        case Category::CONTROL:   return "Control";
        case Category::SENSING:   return "Sensing";
        case Category::OPERATORS: return "Operators";
        case Category::VARIABLES: return "Variables";
    }
    return "?";
}

// ════════════════════════════════════════════
//  Costume System
// ════════════════════════════════════════════
enum class CostumeType { CAT, CIRCLE, SQUARE, TRIANGLE, STAR, ARROW };

struct Costume {
    string name;
    CostumeType type;
    SDL_Color primaryColor;
    SDL_Color secondaryColor;
};
struct UploadedImage {
    string filename;
    SDL_Texture* texture;
    int width, height;
    float scale;
    float rotation;
    bool flippedH, flippedV;

    UploadedImage() : texture(nullptr), width(0), height(0), scale(1.0f), rotation(0), flippedH(false), flippedV(false) {}
    ~UploadedImage() { if(texture) SDL_DestroyTexture(texture); }
};
// ════════════════════════════════════════════
//  Sprite
// ════════════════════════════════════════════
struct Sprite {
    string name;
    float x, y;
    float direction;
    float size;
    bool visible;
    bool selected;
    SDL_Color color;

    string sayText;
    float sayTimer;
    string thinkText;
    float thinkTimer;
    float ghostEffect;
    float colorEffect;
    int currentCostume;
    SDL_Texture* uploadedTexture;
    vector<Costume> costumes;
};

static int gNextSpriteNum = 2;

static Sprite createDefaultSprite(const char* name, float x, float y, SDL_Color col) {
    Sprite sp;
    sp.name = name;
    sp.x = x; sp.y = y;
    sp.direction = 90;
    sp.size = 100;
    sp.visible = true;
    sp.selected = false;
    sp.color = col;
    sp.sayTimer = 0;
    sp.thinkTimer = 0;
    sp.uploadedTexture = nullptr;
    sp.ghostEffect = 0;
    sp.colorEffect = 0;
    sp.currentCostume = 0;
    sp.costumes.push_back({"costume1", CostumeType::CAT, col, {255, 200, 150, 255}});
    sp.costumes.push_back({"costume2", CostumeType::CIRCLE, {100, 150, 255, 255}, {200, 200, 255, 255}});
    return sp;
}

// ════════════════════════════════════════════
//  Block types
// ════════════════════════════════════════════
enum class BlockShape { COMMAND, C_BLOCK, HAT, CAP, REPORTER, BOOLEAN };

struct InputField {
    string value;
    float relX, relY;
    float width, height;
    bool editing;
    string defaultVal;
};

struct OperatorSlot {
    float relX, relY;
    float width, height;
    int embeddedBlockId;
};

struct Block {
    int id;
    Category cat;
    BlockShape shape;
    string text;
    float x, y;
    float w, h;
    bool inPalette;
    int nextBlockId;
    int parentBlockId;
    int childHeadId;
    vector<InputField> inputs;
    vector<OperatorSlot> opSlots;
};

static int gNextBlockId = 1000;

// ════════════════════════════════════════════
//  Global state
// ════════════════════════════════════════════
static bool gIsRunning = false;
static float gTimer = 0;
static int gBgColor = 0;
static int gHighlightBlockId = -1; // بلاکی که الان هایلایته
static const SDL_Color BG_COLORS[] = {
    {255,255,255,255},
    {200,230,255,255},
    {200,255,200,255},
    {255,220,200,255},
    {230,200,255,255},
    {50,50,80,255},
};
static const int NUM_BG_COLORS = 6;
struct ActiveEdit {
    int blockId;
    int fieldIndex;
    int spriteField;
    bool active;
    string buffer;
    int cursorPos;
};
static ActiveEdit gEdit = {-1, -1, -1, false, "", 0};

static string intToString(int n) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", n);
    return string(buf);
}

static string floatToString(float f) {
    char buf[32];
    if (f == (int)f) snprintf(buf, sizeof(buf), "%d", (int)f);
    else snprintf(buf, sizeof(buf), "%.1f", f);
    return string(buf);
}

// ════════════════════════════════════════════
//  Build blocks
// ════════════════════════════════════════════
static Block makeBlock(int id, Category cat, BlockShape shape, const string& text,
                       float x, float y, bool inPalette,
                       vector<InputField> fields = {},
                       vector<OperatorSlot> slots = {})
{
    Block b;
    b.id = id; b.cat = cat; b.shape = shape; b.text = text;
    b.x = x; b.y = y;
    b.w = L.BLOCK_WIDTH;
    b.h = (shape == BlockShape::C_BLOCK) ? L.CBLOCK_MIN_H : L.BLOCK_HEIGHT;
    b.inPalette = inPalette;
    b.nextBlockId = -1; b.parentBlockId = -1; b.childHeadId = -1;
    b.inputs = fields; b.opSlots = slots;
    return b;
}

static InputField makeInput(float rx, float ry, float w, float h, const string& def) {
    InputField f;
    f.relX = rx; f.relY = ry; f.width = w; f.height = h;
    f.value = def; f.defaultVal = def; f.editing = false;
    return f;
}

static OperatorSlot makeOpSlot(float rx, float ry, float w, float h) {
    OperatorSlot s;
    s.relX = rx; s.relY = ry; s.width = w; s.height = h;
    s.embeddedBlockId = -1;
    return s;
}

static vector<Block> buildPaletteBlocks() {
    vector<Block> blocks;
    int id = 0;
    float bw = L.BLOCK_WIDTH, bh = L.BLOCK_HEIGHT;
    float fieldH = bh * 0.55f, fieldW = bw * 0.2f, slotW = bw * 0.25f;

    // MOTION
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "move  steps", 0,0,true, {makeInput(bw*0.25f,bh*0.15f,fieldW,fieldH,"10")}, {makeOpSlot(bw*0.25f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "turn R  deg", 0,0,true, {makeInput(bw*0.35f,bh*0.15f,fieldW,fieldH,"15")}, {makeOpSlot(bw*0.35f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "turn L  deg", 0,0,true, {makeInput(bw*0.35f,bh*0.15f,fieldW,fieldH,"15")}, {makeOpSlot(bw*0.35f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "go to x:  y: ", 0,0,true, {makeInput(bw*0.35f,bh*0.15f,fieldW,fieldH,"0"),makeInput(bw*0.65f,bh*0.15f,fieldW,fieldH,"0")}, {makeOpSlot(bw*0.35f,bh*0.15f,slotW,fieldH),makeOpSlot(bw*0.65f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "glide  s x:  y: ", 0,0,true, {makeInput(bw*0.28f,bh*0.15f,fieldW*0.7f,fieldH,"1"),makeInput(bw*0.5f,bh*0.15f,fieldW*0.7f,fieldH,"0"),makeInput(bw*0.72f,bh*0.15f,fieldW*0.7f,fieldH,"0")}, {}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "set x to ", 0,0,true, {makeInput(bw*0.5f,bh*0.15f,fieldW,fieldH,"0")}, {makeOpSlot(bw*0.5f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "set y to ", 0,0,true, {makeInput(bw*0.5f,bh*0.15f,fieldW,fieldH,"0")}, {makeOpSlot(bw*0.5f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "change x by ", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"10")}, {makeOpSlot(bw*0.6f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "change y by ", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"10")}, {makeOpSlot(bw*0.6f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::COMMAND, "point dir ", 0,0,true, {makeInput(bw*0.55f,bh*0.15f,fieldW,fieldH,"90")}, {makeOpSlot(bw*0.55f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::REPORTER, "x position", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::REPORTER, "y position", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::MOTION, BlockShape::REPORTER, "direction", 0,0,true,{},{}));

    // LOOKS
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "say  for  s", 0,0,true, {makeInput(bw*0.2f,bh*0.15f,fieldW*1.2f,fieldH,"Hello!"),makeInput(bw*0.6f,bh*0.15f,fieldW*0.7f,fieldH,"2")}, {}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "say ", 0,0,true, {makeInput(bw*0.25f,bh*0.15f,fieldW*1.5f,fieldH,"Hello!")}, {}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "think  for  s", 0,0,true, {makeInput(bw*0.25f,bh*0.15f,fieldW*1.2f,fieldH,"Hmm..."),makeInput(bw*0.65f,bh*0.15f,fieldW*0.7f,fieldH,"2")}, {}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "show", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "hide", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "set size to %", 0,0,true, {makeInput(bw*0.55f,bh*0.15f,fieldW,fieldH,"100")}, {makeOpSlot(bw*0.55f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "change size by ", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"10")}, {makeOpSlot(bw*0.6f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::COMMAND, "next costume", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::REPORTER, "costume #", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::LOOKS, BlockShape::REPORTER, "size", 0,0,true,{},{}));

    // SOUND
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "play sound", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "stop sounds", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "set vol to %", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"100")},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "change vol by ", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"-10")},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::REPORTER, "volume", 0,0,true,{},{}));

    // EVENTS
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when flag clicked", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when space pressed", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when sprite clicked", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::COMMAND, "broadcast ", 0,0,true, {makeInput(bw*0.5f,bh*0.15f,fieldW*1.2f,fieldH,"msg1")},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when I receive ", 0,0,true, {makeInput(bw*0.65f,bh*0.15f,fieldW*1.0f,fieldH,"msg1")},{}));

    // CONTROL
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::COMMAND, "wait  secs", 0,0,true, {makeInput(bw*0.27f,bh*0.15f,fieldW,fieldH,"1")}, {makeOpSlot(bw*0.27f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "repeat ", 0,0,true, {makeInput(bw*0.4f,bh*0.05f,fieldW,fieldH*0.8f,"10")}, {makeOpSlot(bw*0.4f,bh*0.05f,slotW,fieldH*0.8f)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "forever", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "if  then", 0,0,true, {}, {makeOpSlot(bw*0.2f,bh*0.05f,slotW*1.5f,fieldH*0.8f)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "if  else", 0,0,true, {}, {makeOpSlot(bw*0.2f,bh*0.05f,slotW*1.5f,fieldH*0.8f)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::CAP, "stop all", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::COMMAND, "wait until ", 0,0,true, {}, {makeOpSlot(bw*0.5f,bh*0.15f,slotW*1.3f,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "repeat until ", 0,0,true, {}, {makeOpSlot(bw*0.55f,bh*0.05f,slotW*1.3f,fieldH*0.8f)}));

    // SENSING
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::BOOLEAN, "touching edge?", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::BOOLEAN, "touching mouse?", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::REPORTER, "mouse x", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::REPORTER, "mouse y", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::BOOLEAN, "mouse down?", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::BOOLEAN, "key  pressed?", 0,0,true, {makeInput(bw*0.25f,bh*0.15f,fieldW,fieldH,"space")},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::REPORTER, "timer", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::COMMAND, "reset timer", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::COMMAND, "ask  and wait", 0,0,true, {makeInput(bw*0.2f,bh*0.15f,fieldW*1.5f,fieldH,"What?")},{}));
    blocks.push_back(makeBlock(id++, Category::SENSING, BlockShape::REPORTER, "answer", 0,0,true,{},{}));

    // OPERATORS
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "  +  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "  -  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "  *  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "  /  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "pick rand  to ", 0,0,true, {makeInput(bw*0.42f,bh*0.15f,fieldW*0.7f,fieldH,"1"),makeInput(bw*0.72f,bh*0.15f,fieldW*0.7f,fieldH,"10")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::BOOLEAN, "  <  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::BOOLEAN, "  =  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::BOOLEAN, "  >  ", 0,0,true, {makeInput(bw*0.05f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.55f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::BOOLEAN, " and ", 0,0,true, {}, {makeOpSlot(bw*0.0f,bh*0.15f,slotW,fieldH),makeOpSlot(bw*0.55f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::BOOLEAN, " or ", 0,0,true, {}, {makeOpSlot(bw*0.0f,bh*0.15f,slotW,fieldH),makeOpSlot(bw*0.55f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::BOOLEAN, "not ", 0,0,true, {}, {makeOpSlot(bw*0.25f,bh*0.15f,slotW*1.3f,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "mod  / ", 0,0,true, {makeInput(bw*0.2f,bh*0.15f,fieldW*0.8f,fieldH,""),makeInput(bw*0.6f,bh*0.15f,fieldW*0.8f,fieldH,"")},{}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "round ", 0,0,true, {makeInput(bw*0.35f,bh*0.15f,fieldW,fieldH,"")}, {makeOpSlot(bw*0.35f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::OPERATORS, BlockShape::REPORTER, "abs of ", 0,0,true, {makeInput(bw*0.4f,bh*0.15f,fieldW,fieldH,"")}, {makeOpSlot(bw*0.4f,bh*0.15f,slotW,fieldH)}));

    // VARIABLES
    blocks.push_back(makeBlock(id++, Category::VARIABLES, BlockShape::REPORTER, "my variable", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::VARIABLES, BlockShape::COMMAND, "set var to ", 0,0,true, {makeInput(bw*0.55f,bh*0.15f,fieldW,fieldH,"0")}, {makeOpSlot(bw*0.55f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::VARIABLES, BlockShape::COMMAND, "change var by ", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"1")}, {makeOpSlot(bw*0.6f,bh*0.15f,slotW,fieldH)}));

    gNextBlockId = id + 100;
    return blocks;
}

static Block* findBlock(vector<Block>& blocks, int id) {
    for (auto& b : blocks) if (b.id == id) return &b;
    return nullptr;
}

static float calcCBlockHeight(vector<Block>& blocks, Block& cb) {
    float barH = L.CBLOCK_BAR_H, mouthH = L.CBLOCK_MOUTH_H;
    float childrenH = 0;
    int cid = cb.childHeadId;
    while (cid >= 0) {
        Block* child = findBlock(blocks, cid);
        if (!child) break;
        childrenH += (child->shape == BlockShape::C_BLOCK) ? calcCBlockHeight(blocks, *child) : child->h;
        cid = child->nextBlockId;
    }
    if (childrenH < mouthH) childrenH = mouthH;
    return barH + childrenH + barH;
}

static void updateCBlockChildren(vector<Block>& blocks, Block& cb) {
    float barH = L.CBLOCK_BAR_H, indent = 20 * L.s;
    float cy = cb.y + barH;
    int cid = cb.childHeadId;
    while (cid >= 0) {
        Block* child = findBlock(blocks, cid);
        if (!child) break;
        child->x = cb.x + indent;
        child->y = cy;
        if (child->shape == BlockShape::C_BLOCK) {
            child->h = calcCBlockHeight(blocks, *child);
            updateCBlockChildren(blocks, *child);
        }
        cy += child->h;
        cid = child->nextBlockId;
    }
    cb.h = calcCBlockHeight(blocks, cb);
}

static Block cloneBlock(const Block& src, float x, float y) {
    Block b = src;
    b.id = gNextBlockId++;
    b.x = x; b.y = y;
    b.inPalette = false;
    b.nextBlockId = -1; b.parentBlockId = -1; b.childHeadId = -1;
    for (auto& inp : b.inputs) inp.editing = false;
    for (auto& sl : b.opSlots) sl.embeddedBlockId = -1;
    return b;
}

static void drawBlock(SDL_Renderer* rnd, Block& b, vector<Block>& allBlocks, bool highlight = false) {
    SDL_Color col = catColor(b.cat);
    Uint8 cr = col.r, cg = col.g, cb2 = col.b;
    if (highlight) { cr=min(255,cr+40); cg=min(255,cg+40); cb2=min(255,cb2+40); }
    bool isHighlighted = (b.id == gHighlightBlockId);
    int bx=(int)b.x, by=(int)b.y, bw=(int)b.w, bh=(int)b.h, r=(int)L.BLOCK_CORNER_R;

    switch (b.shape) {
    case BlockShape::COMMAND:
    case BlockShape::CAP:
        fillRoundedRect(rnd, bx, by, bw, bh, r, cr, cg, cb2, 255);
        { SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_Rect notch={bx+(int)(20*L.s),by-(int)(4*L.s),(int)(30*L.s),(int)(4*L.s)}; SDL_RenderFillRect(rnd,&notch); }
        if (b.shape != BlockShape::CAP) { SDL_Rect notchB={bx+(int)(20*L.s),by+bh,(int)(30*L.s),(int)(4*L.s)}; SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_RenderFillRect(rnd,&notchB); }
        break;
    case BlockShape::HAT:
        fillRoundedRect(rnd, bx, by+(int)(10*L.s), bw, bh-(int)(10*L.s), r, cr, cg, cb2, 255);
        fillEllipse(rnd, bx+bw/2, by+(int)(10*L.s), bw/2, (int)(12*L.s), cr, cg, cb2, 255);
        { SDL_Rect notchB={bx+(int)(20*L.s),by+bh,(int)(30*L.s),(int)(4*L.s)}; SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_RenderFillRect(rnd,&notchB); }
        break;
    case BlockShape::C_BLOCK: {
        float barH=L.CBLOCK_BAR_H, indent=20*L.s;
        fillRoundedRect(rnd,bx,by,bw,(int)barH,r,cr,cg,cb2,255);
        { SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_Rect notch={bx+(int)(20*L.s),by-(int)(4*L.s),(int)(30*L.s),(int)(4*L.s)}; SDL_RenderFillRect(rnd,&notch); }
        { SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_Rect notchIn={bx+(int)indent+(int)(20*L.s),by+(int)barH,(int)(30*L.s),(int)(4*L.s)}; SDL_RenderFillRect(rnd,&notchIn); }
        float mouthTop=by+barH, mouthBot=by+bh-barH;
        SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255);
        SDL_Rect leftBar={bx,(int)mouthTop,(int)indent,(int)(mouthBot-mouthTop)}; SDL_RenderFillRect(rnd,&leftBar);
        { Uint8 mr=(Uint8)max(0,(int)cr-30),mg=(Uint8)max(0,(int)cg-30),mb=(Uint8)max(0,(int)cb2-30); SDL_SetRenderDrawColor(rnd,mr,mg,mb,80); SDL_Rect mouth={bx+(int)indent,(int)mouthTop,bw-(int)indent,(int)(mouthBot-mouthTop)}; SDL_RenderFillRect(rnd,&mouth); }
        fillRoundedRect(rnd,bx,(int)mouthBot,bw,(int)barH,r,cr,cg,cb2,255);
        { SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_Rect notchB={bx+(int)(20*L.s),by+(int)b.h,(int)(30*L.s),(int)(4*L.s)}; SDL_RenderFillRect(rnd,&notchB); }
        break; }
    case BlockShape::REPORTER: { int rr=bh/2; fillRoundedRect(rnd,bx,by,bw,bh,rr,cr,cg,cb2,255); break; }
    case BlockShape::BOOLEAN: {
        int pointW=bh/2;
        for (int row=0;row<bh;row++) { int y=by+row; float t=(float)row/bh; int ind2=(t<0.5f)?(int)(pointW*(1.0f-2.0f*t)):(int)(pointW*(2.0f*t-1.0f)); SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255); SDL_RenderDrawLine(rnd,bx+ind2,y,bx+bw-ind2,y); }
        break; }
    }

    { int tx=bx+(int)(8*L.s), ty; if(b.shape==BlockShape::C_BLOCK) ty=by+(int)(L.CBLOCK_BAR_H*0.25f); else if(b.shape==BlockShape::HAT) ty=by+(int)(14*L.s); else ty=by+(int)(bh/2)-textHeight("A")/2; drawText(rnd,tx,ty,b.text.c_str(),255,255,255,255); }

    for (int fi=0;fi<(int)b.inputs.size();fi++) {
        auto& inp=b.inputs[fi];
        int fx=bx+(int)inp.relX, fy=by+(int)inp.relY, fw=(int)inp.width, fh=(int)inp.height;
        fillRoundedRect(rnd,fx,fy,fw,fh,4,255,255,255,255);
        if (inp.editing) { drawRoundedRectOutline(rnd,fx-1,fy-1,fw+2,fh+2,4,50,150,255,255); int tw=textWidth(inp.value.c_str()); int cursorX=fx+3+tw; SDL_SetRenderDrawColor(rnd,0,0,0,255); SDL_RenderDrawLine(rnd,cursorX,fy+2,cursorX,fy+fh-2); }
        drawText(rnd,fx+3,fy+(fh-textHeight("0"))/2,inp.value.c_str(),0,0,0,255);
    }

    for (int si=0;si<(int)b.opSlots.size();si++) {
        auto& sl=b.opSlots[si];
        if (sl.embeddedBlockId<0) {
            bool hasInput=false;
            for (auto& inp:b.inputs) if (abs(inp.relX-sl.relX)<5&&abs(inp.relY-sl.relY)<5){hasInput=true;break;}
            if (!hasInput) { int sx2=bx+(int)sl.relX,sy2=by+(int)sl.relY,sw2=(int)sl.width,sh2=(int)sl.height; fillRoundedRect(rnd,sx2,sy2,sw2,sh2,sh2/2,255,255,255,120); }
            // حاشیه زرد برای بلاک در حال اجرا
            if (isHighlighted) {
                SDL_SetRenderDrawColor(rnd, 255, 220, 0, 255);
                SDL_Rect hlRect = {bx-3, by-3, bw+6, bh+6};
                SDL_RenderDrawRect(rnd, &hlRect);
                SDL_Rect hlRect2 = {bx-2, by-2, bw+4, bh+4};
                SDL_RenderDrawRect(rnd, &hlRect2);
            }
        }
    }
}

// ════════════════════════════════════════════
//  Draw cat sprite
// ════════════════════════════════════════════
static void drawCatSprite(SDL_Renderer* rnd, int cx, int cy, int sz, SDL_Color col) {
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND); // مطمئن شویم فعال است
    int half=sz/2;
    fillEllipse(rnd,cx,cy,half,half,col.r,col.g,col.b,col.a);
    int earH=half*2/3, earW=half/3;
    for (int row=0;row<earH;row++) { float t=(float)row/earH; int w=(int)(earW*(1.0f-t)); SDL_SetRenderDrawColor(rnd,col.r,col.g,col.b,col.a); SDL_RenderDrawLine(rnd,cx-half/2-w,cy-half-row,cx-half/2+w,cy-half-row); SDL_RenderDrawLine(rnd,cx+half/2-w,cy-half-row,cx+half/2+w,cy-half-row); }
    fillEllipse(rnd,cx-half/3,cy-half/4,sz/10,sz/8,255,255,255,col.a);
    fillEllipse(rnd,cx+half/3,cy-half/4,sz/10,sz/8,255,255,255,col.a);
    fillEllipse(rnd,cx-half/3,cy-half/4,sz/20,sz/14,0,0,0,col.a);
    fillEllipse(rnd,cx+half/3,cy-half/4,sz/20,sz/14,0,0,0,col.a);
SDL_SetRenderDrawColor(rnd,0,0,0,col.a);    SDL_RenderDrawLine(rnd,cx-half/5,cy+half/4,cx,cy+half/3);
    SDL_RenderDrawLine(rnd,cx+half/5,cy+half/4,cx,cy+half/3);
}

static void drawSpeechBubble(SDL_Renderer* rnd, int cx, int cy, const char* text, bool isThink) {
    if (!text||text[0]=='\0') return;
    int tw=textWidth(text)+(int)(20*L.s), th=textHeight(text)+(int)(16*L.s);
    int bx2=cx-tw/2, by2=cy-th-(int)(20*L.s);
    fillRoundedRect(rnd,bx2,by2,tw,th,10,255,255,255,255);
    SDL_SetRenderDrawColor(rnd,180,180,180,255);
    SDL_Rect bdr={bx2,by2,tw,th}; SDL_RenderDrawRect(rnd,&bdr);
    if (isThink) { fillEllipse(rnd,cx,by2+th+5,5,5,255,255,255,255); fillEllipse(rnd,cx+3,by2+th+12,3,3,255,255,255,255); }
    else { SDL_SetRenderDrawColor(rnd,255,255,255,255); for(int row=0;row<12;row++){int y=by2+th+row;float t=row/12.0f;int lx=(int)((1-t)*(cx-5)+t*cx),rx=(int)((1-t)*(cx+5)+t*cx);SDL_RenderDrawLine(rnd,lx,y,rx,y);} }
    drawText(rnd,bx2+(int)(10*L.s),by2+(int)(8*L.s),text,0,0,0,255);
}

static void detachBlock(vector<Block>& blocks, int blockId) {
    Block* b=findBlock(blocks,blockId);
    if (!b) return;
    int parentId=b->parentBlockId;
    if (parentId>=0) {
        Block* parent=findBlock(blocks,parentId);
        if (parent) {
            if (parent->nextBlockId==blockId) parent->nextBlockId=-1;
            if (parent->childHeadId==blockId) { parent->childHeadId=b->nextBlockId; if(b->nextBlockId>=0){Block* next=findBlock(blocks,b->nextBlockId);if(next)next->parentBlockId=parentId;} }
            else if (parent->childHeadId>=0) { int prevId=parent->childHeadId; while(prevId>=0){Block* prev=findBlock(blocks,prevId);if(!prev)break;if(prev->nextBlockId==blockId){prev->nextBlockId=b->nextBlockId;if(b->nextBlockId>=0){Block* nxt=findBlock(blocks,b->nextBlockId);if(nxt)nxt->parentBlockId=prevId;}break;}prevId=prev->nextBlockId;} }
            for (auto& sl:parent->opSlots) if(sl.embeddedBlockId==blockId) sl.embeddedBlockId=-1;
        }
    }
    b->parentBlockId=-1; b->nextBlockId=-1;
}

static void moveBlockChain(vector<Block>& blocks, int blockId, float dx, float dy) {
    Block* b=findBlock(blocks,blockId);
    if (!b) return;
    b->x+=dx; b->y+=dy;
    if (b->shape==BlockShape::C_BLOCK&&b->childHeadId>=0) { int cid=b->childHeadId; while(cid>=0){Block* c=findBlock(blocks,cid);if(!c)break;moveBlockChain(blocks,cid,dx,dy);cid=c->nextBlockId;} }
    for (auto& sl:b->opSlots) if(sl.embeddedBlockId>=0){Block* emb=findBlock(blocks,sl.embeddedBlockId);if(emb){emb->x+=dx;emb->y+=dy;}}
    if (b->nextBlockId>=0) moveBlockChain(blocks,b->nextBlockId,dx,dy);
}

static void trySnapBlocks(vector<Block>& blocks, int dragId) {
    Block* drag=findBlock(blocks,dragId);
    if (!drag||drag->inPalette) return;
    float snapDist=L.SNAP_DISTANCE;

    if (drag->shape==BlockShape::COMMAND||drag->shape==BlockShape::C_BLOCK||drag->shape==BlockShape::CAP) {
        for (auto& other:blocks) {
            if (other.id==dragId||other.inPalette) continue;
            if (other.nextBlockId>=0||other.shape==BlockShape::CAP||other.shape==BlockShape::REPORTER||other.shape==BlockShape::BOOLEAN) continue;
            float ox=other.x, oy=other.y+other.h;
            if (abs(drag->x-ox)<snapDist&&abs(drag->y-oy)<snapDist) { float ddx=ox-drag->x,ddy=oy-drag->y; moveBlockChain(blocks,dragId,ddx,ddy); other.nextBlockId=dragId; drag->parentBlockId=other.id; return; }
        }
    }

    if (drag->shape==BlockShape::COMMAND||drag->shape==BlockShape::C_BLOCK) {
        for (auto& other:blocks) {
            if (other.id==dragId||other.inPalette||other.shape!=BlockShape::C_BLOCK) continue;
            float indent=20*L.s, barH=L.CBLOCK_BAR_H, mouthX=other.x+indent, mouthY=other.y+barH;
            if (abs(drag->x-mouthX)<snapDist&&abs(drag->y-mouthY)<snapDist) {
                float ddx=mouthX-drag->x,ddy=mouthY-drag->y; moveBlockChain(blocks,dragId,ddx,ddy);
                if (other.childHeadId>=0) { int lastInDrag=dragId; while(true){Block* lb=findBlock(blocks,lastInDrag);if(!lb||lb->nextBlockId<0)break;lastInDrag=lb->nextBlockId;} Block* lastB=findBlock(blocks,lastInDrag); if(lastB){lastB->nextBlockId=other.childHeadId;Block* oldHead=findBlock(blocks,other.childHeadId);if(oldHead)oldHead->parentBlockId=lastInDrag;} }
                other.childHeadId=dragId; drag->parentBlockId=other.id;
                updateCBlockChildren(blocks,other); return;
            }
        }
    }

    if (drag->shape==BlockShape::REPORTER||drag->shape==BlockShape::BOOLEAN) {
        for (auto& other:blocks) {
            if (other.id==dragId||other.inPalette) continue;
            for (auto& sl:other.opSlots) {
                if (sl.embeddedBlockId>=0) continue;
                float sx2=other.x+sl.relX,sy2=other.y+sl.relY;
                if (abs(drag->x-sx2)<snapDist*0.7f&&abs(drag->y-sy2)<snapDist*0.7f) { drag->x=sx2;drag->y=sy2;sl.embeddedBlockId=dragId;drag->parentBlockId=other.id;return; }
            }
        }
    }
}

static void resetProject(vector<Block>& blocks, vector<Sprite>& sprites) {
    blocks.erase(remove_if(blocks.begin(),blocks.end(),[](const Block& b){return !b.inPalette;}),blocks.end());
    sprites.clear();
    sprites.push_back(createDefaultSprite("Sprite1",0,0,{255,140,0,255}));
    gIsRunning=false; gTimer=0; gNextBlockId=1000; gNextSpriteNum=2;
    gEdit={-1,-1,-1,false,"",0};
}

// ════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════
int main(int argc, char* argv[]) {
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        cout << "SDL_image Error: " << IMG_GetError() << endl;
    }

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    SDL_Window* window=SDL_CreateWindow("Scratch IDE - SDL2 (Enhanced)",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,BASE_WIDTH,BASE_HEIGHT,SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
    SDL_Renderer* rnd=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
    int winW=BASE_WIDTH, winH=BASE_HEIGHT;
    L.update(winW,winH);
    SDL_StartTextInput();

    vector<Sprite> sprites;
    sprites.push_back(createDefaultSprite("Sprite1",0,0,{255,140,0,255}));
    vector<Block> blocks=buildPaletteBlocks();

    Category selectedCategory=Category::MOTION;
    int dragBlockId=-1;
    float dragOffX=0, dragOffY=0;
    int paletteScrollY=0;
    bool draggingSprite=false;
    int dragSpriteIdx=-1;
    float spDragOffX=0, spDragOffY=0;
    bool resetHovered=false;
    int selectedSpriteIdx=0;

    struct SpriteInfoEdit { int field; string buffer; int cursorPos; };
    SpriteInfoEdit sprInfoEdit={-1,"",0};

    Uint32 lastTick=SDL_GetTicks();
    bool running=true;

    while (running) {
        Uint32 now=SDL_GetTicks();
        float dt=(now-lastTick)/1000.0f;
        lastTick=now;
        if (gIsRunning) gTimer+=dt;

        for (auto& sp:sprites) {
            if(sp.sayTimer>0){sp.sayTimer-=dt;if(sp.sayTimer<=0){sp.sayTimer=0;sp.sayText.clear();}}
            if(sp.thinkTimer>0){sp.thinkTimer-=dt;if(sp.thinkTimer<=0){sp.thinkTimer=0;sp.thinkText.clear();}}
        }

        // ════════════════════════════════════════════
        //  EVENT LOOP
        // ════════════════════════════════════════════
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_QUIT) running=false;

            if (e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_RESIZED) {
                winW=e.window.data1; winH=e.window.data2; L.update(winW,winH);
                vector<Block> kept; for(auto& b:blocks) if(!b.inPalette) kept.push_back(b);
                blocks=buildPaletteBlocks(); for(auto& b:kept) blocks.push_back(b);
            }

            if (e.type==SDL_MOUSEWHEEL) {
                int mx,my; SDL_GetMouseState(&mx,&my);
                if(mx<L.PALETTE_WIDTH&&my>L.TOOLBAR_HEIGHT){paletteScrollY+=e.wheel.y*20;if(paletteScrollY>0)paletteScrollY=0;}
            }

            // TEXT INPUT
            if (e.type==SDL_TEXTINPUT) {
                if(gEdit.active&&gEdit.blockId>=0&&gEdit.fieldIndex>=0){Block* eb=findBlock(blocks,gEdit.blockId);if(eb&&gEdit.fieldIndex<(int)eb->inputs.size())eb->inputs[gEdit.fieldIndex].value+=e.text.text;}
                if(sprInfoEdit.field>=0&&selectedSpriteIdx<(int)sprites.size())sprInfoEdit.buffer+=e.text.text;
            }

            if (e.type==SDL_KEYDOWN) {
                if(gEdit.active&&gEdit.blockId>=0&&gEdit.fieldIndex>=0){
                    Block* eb=findBlock(blocks,gEdit.blockId);
                    if(eb&&gEdit.fieldIndex<(int)eb->inputs.size()){
                        auto& inp=eb->inputs[gEdit.fieldIndex];
                        if(e.key.keysym.sym==SDLK_BACKSPACE&&!inp.value.empty()) inp.value.pop_back();
                        else if(e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_ESCAPE){inp.editing=false;gEdit.active=false;gEdit.blockId=-1;gEdit.fieldIndex=-1;}
                    }
                } else if(sprInfoEdit.field>=0&&selectedSpriteIdx<(int)sprites.size()){
                    Sprite& sp=sprites[selectedSpriteIdx];
                    if(e.key.keysym.sym==SDLK_BACKSPACE&&!sprInfoEdit.buffer.empty()) sprInfoEdit.buffer.pop_back();
                    else if(e.key.keysym.sym==SDLK_RETURN||e.key.keysym.sym==SDLK_ESCAPE){
                        switch(sprInfoEdit.field){case 0:sp.name=sprInfoEdit.buffer;break;case 1:sp.x=atof(sprInfoEdit.buffer.c_str());break;case 2:sp.y=atof(sprInfoEdit.buffer.c_str());break;case 3:sp.size=atof(sprInfoEdit.buffer.c_str());break;case 4:sp.direction=atof(sprInfoEdit.buffer.c_str());break;case 5: sp.ghostEffect = atof(sprInfoEdit.buffer.c_str()); break;}
                        sprInfoEdit.field=-1; sprInfoEdit.buffer.clear();
                    }
                }
            }

            // MOUSE DOWN
            if (e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT) {
                int mx=e.button.x, my=e.button.y;
                bool clickedOnField=false;

                46545;
                int panelX=L.PALETTE_WIDTH, stageW=L.STAGE_WIDTH;
                int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
                int thumbSize=L.SPRITE_THUMB;
                int infoY=spriteAreaY+thumbSize+10, infoX=panelX+5;
                int fieldW2=(int)(stageW*0.35f), fieldH2=(int)(22*L.s);
                // کلیک روی دکمه Upload
                                    infoY=spriteAreaY+thumbSize+10, infoX=panelX+5;

                {
                    int uploadBtnW = (int)(60*L.s), uploadBtnH = fieldH2;
                    int uploadBtnX = infoX + fieldW2*2 - uploadBtnW - 10;
                    int uploadBtnY = infoY;
                    if(mx>=uploadBtnX && mx<=uploadBtnX+uploadBtnW && my>=uploadBtnY && my<=uploadBtnY+uploadBtnH){
                        // اینجا بعداً کد باز کردن فایل را می‌نویسیم
                        // فعلاً برای تست یک فایل فرضی لود می‌کنیم
                        if(selectedSpriteIdx < (int)sprites.size()){
                            SDL_Surface* surf = IMG_Load("cat.png"); // نام فایل عکس خود را بگذارید
                            if(surf){
                                if(sprites[selectedSpriteIdx].uploadedTexture) SDL_DestroyTexture(sprites[selectedSpriteIdx].uploadedTexture);
                                sprites[selectedSpriteIdx].uploadedTexture = SDL_CreateTextureFromSurface(rnd, surf);
                                SDL_FreeSurface(surf);
                            }
                        }
                    }
                }
                // ── Check sprite info panel fields ──
                {
                    int panelX=L.PALETTE_WIDTH, stageW=L.STAGE_WIDTH;
                    int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
                    int thumbSize=L.SPRITE_THUMB;
                    int infoY=spriteAreaY+thumbSize+10, infoX=panelX+5;
                    int fieldW2=(int)(stageW*0.35f), fieldH2=(int)(22*L.s);
                    struct FieldRect{int x,y,w,h,idx;};
                    vector<FieldRect> fields;
                    fields.push_back({infoX+(int)(40*L.s),infoY,fieldW2*2,fieldH2,0});
                    fields.push_back({infoX+(int)(15*L.s),infoY+fieldH2+8,fieldW2-(int)(20*L.s),fieldH2,1});
                    fields.push_back({infoX+fieldW2+(int)(15*L.s),infoY+fieldH2+8,fieldW2-(int)(20*L.s),fieldH2,2});
                    fields.push_back({infoX+(int)(30*L.s),infoY+2*(fieldH2+8),fieldW2-(int)(20*L.s),fieldH2,3});

                    fields.push_back({infoX+fieldW2+(int)(25*L.s),infoY+2*(fieldH2+8),fieldW2-(int)(20*L.s),fieldH2,4});
                    fields.push_back({infoX+(int)(30*L.s), infoY+3*(fieldH2+8), fieldW2-(int)(20*L.s), fieldH2, 5});
                    for (auto& fr:fields) {
                        if(mx>=fr.x&&mx<=fr.x+fr.w&&my>=fr.y&&my<=fr.y+fr.h){
                            if(selectedSpriteIdx<(int)sprites.size()){
                                Sprite& sp=sprites[selectedSpriteIdx];
                                sprInfoEdit.field=fr.idx;
                                switch(fr.idx){case 0:sprInfoEdit.buffer=sp.name;break;case 1:sprInfoEdit.buffer=floatToString(sp.x);break;case 2:sprInfoEdit.buffer=floatToString(sp.y);break;case 3:sprInfoEdit.buffer=floatToString(sp.size);break;case 4:sprInfoEdit.buffer=floatToString(sp.direction);break;}
                                clickedOnField=true;
                                if(gEdit.active&&gEdit.blockId>=0){Block* eb=findBlock(blocks,gEdit.blockId);if(eb&&gEdit.fieldIndex>=0&&gEdit.fieldIndex<(int)eb->inputs.size())eb->inputs[gEdit.fieldIndex].editing=false;gEdit.active=false;}
                            }
                            break;
                        }
                    }
                }

                // ── Check block input fields ──
                if (!clickedOnField) {
                    for (auto& b:blocks) {
                        if(b.inPalette) continue;
                        for(int fi=0;fi<(int)b.inputs.size();fi++){
                            auto& inp=b.inputs[fi];
                            int fx=(int)b.x+(int)inp.relX,fy=(int)b.y+(int)inp.relY,fw=(int)inp.width,fh=(int)inp.height;
                            if(mx>=fx&&mx<=fx+fw&&my>=fy&&my<=fy+fh){
                                if(gEdit.active&&gEdit.blockId>=0){Block* prevB=findBlock(blocks,gEdit.blockId);if(prevB&&gEdit.fieldIndex>=0&&gEdit.fieldIndex<(int)prevB->inputs.size())prevB->inputs[gEdit.fieldIndex].editing=false;}
                                sprInfoEdit.field=-1;
                                inp.editing=true;gEdit.active=true;gEdit.blockId=b.id;gEdit.fieldIndex=fi;clickedOnField=true;break;
                            }
                        }
                        if(clickedOnField) break;
                    }
                }

                if (!clickedOnField) {
                    if(gEdit.active&&gEdit.blockId>=0){Block* eb=findBlock(blocks,gEdit.blockId);if(eb&&gEdit.fieldIndex>=0&&gEdit.fieldIndex<(int)eb->inputs.size())eb->inputs[gEdit.fieldIndex].editing=false;}
                    gEdit={-1,-1,-1,false,"",0};
                    // ذخیره sprite info قبل از بستن
                    if(sprInfoEdit.field>=0&&selectedSpriteIdx<(int)sprites.size()){
                        Sprite& sp=sprites[selectedSpriteIdx];
                        switch(sprInfoEdit.field){case 0:sp.name=sprInfoEdit.buffer;break;case 1:sp.x=atof(sprInfoEdit.buffer.c_str());break;case 2:sp.y=atof(sprInfoEdit.buffer.c_str());break;case 3:sp.size=atof(sprInfoEdit.buffer.c_str());break;case 4:sp.direction=atof(sprInfoEdit.buffer.c_str());break;case 5: sp.ghostEffect = atof(sprInfoEdit.buffer.c_str()); break;}
                    }
                    sprInfoEdit.field=-1; sprInfoEdit.buffer.clear();
                }

                // ── Toolbar buttons ──
                // کلیک روی دکمه BG
                {
                    int bgBtnX = L.PALETTE_WIDTH + L.STAGE_WIDTH - (int)(35*L.s);
                    int bgBtnY = L.TOOLBAR_HEIGHT + 3;
                    int bgBtnW = (int)(32*L.s);
                    int bgBtnH = (int)(18*L.s);
                    if(mx>=bgBtnX && mx<=bgBtnX+bgBtnW && my>=bgBtnY && my<=bgBtnY+bgBtnH){
                        gBgColor = (gBgColor + 1) % NUM_BG_COLORS;
                        continue;
                    }
                    //////////////
                    ///
                    ///
                }
               ////////////////
               ///
                if (my<L.TOOLBAR_HEIGHT) {
                    int flagX=(int)(winW*0.4f),flagY=5,flagSz=L.TOOLBAR_HEIGHT-10;
                    if(mx>=flagX&&mx<=flagX+flagSz&&my>=flagY&&my<=flagY+flagSz) {
                        gIsRunning=true;
                        gHighlightBlockId=-1;
                    }                    int stopX=flagX+flagSz+10;
                    if(mx>=stopX&&mx<=stopX+flagSz&&my>=flagY&&my<=flagY+flagSz) gIsRunning=false;
                    int resetX=stopX+flagSz+10;
                    if(mx>=resetX&&mx<=resetX+(int)(60*L.s)&&my>=flagY&&my<=flagY+flagSz){resetProject(blocks,sprites);selectedSpriteIdx=0;}
                    continue;
                }

                // ── Category buttons ──
                if (mx<L.CAT_PANEL_WIDTH&&my>L.TOOLBAR_HEIGHT) {
                    int catY=L.TOOLBAR_HEIGHT+5;
                    for(int i=0;i<NUM_CATEGORIES;i++){int btnY=catY+i*(L.CAT_BTN_HEIGHT+3);if(my>=btnY&&my<=btnY+L.CAT_BTN_HEIGHT){selectedCategory=(Category)i;paletteScrollY=0;break;}}
                    continue;
                }

                // ── Add sprite button ──
                {
                    int stageRight=L.PALETTE_WIDTH+L.STAGE_WIDTH;
                    int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
                    int addBtnX=stageRight-(int)(40*L.s),addBtnY=spriteAreaY,addBtnSz=(int)(30*L.s);
                    if(mx>=addBtnX&&mx<=addBtnX+addBtnSz&&my>=addBtnY&&my<=addBtnY+addBtnSz){
                        string newName="Sprite"+intToString(gNextSpriteNum++);
                        SDL_Color colors[]={{66,133,244,255},{255,100,100,255},{100,200,100,255},{200,100,200,255},{100,200,200,255},{255,200,0,255}};
                        SDL_Color newCol=colors[(sprites.size())%6];
                        float nx=(float)((rand()%200)-100),ny=(float)((rand()%200)-100);
                        sprites.push_back(createDefaultSprite(newName.c_str(),nx,ny,newCol));
                        selectedSpriteIdx=(int)sprites.size()-1;
                        for(int si=0;si<(int)sprites.size();si++) sprites[si].selected=(si==selectedSpriteIdx);
                        continue;
                    }
                }

                // ── Sprite thumbnails click ──
                {
                    int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
                    int thumbSz=L.SPRITE_THUMB, startX=L.PALETTE_WIDTH+5;
                    int delBtnSize=(int)(16*L.s);
                    int eyeBtnSize=(int)(16*L.s);
                    bool handledSprite=false;

                    for(int si=0;si<(int)sprites.size();si++){
                        int tx=startX+si*(thumbSz+8), ty=spriteAreaY;

                        // چک کلیک روی دکمه چشم (show/hide)
                        int eyeBtnX=tx+thumbSz-delBtnSize-eyeBtnSize-6, eyeBtnY=ty+2;
                        if(mx>=eyeBtnX&&mx<=eyeBtnX+eyeBtnSize&&my>=eyeBtnY&&my<=eyeBtnY+eyeBtnSize){
                            sprites[si].visible=!sprites[si].visible;
                            handledSprite=true; break;
                        }

                        // چک کلیک روی دکمه X (حذف)
                        int delBtnX=tx+thumbSz-delBtnSize-2, delBtnY=ty+2;
                        if(mx>=delBtnX&&mx<=delBtnX+delBtnSize&&my>=delBtnY&&my<=delBtnY+delBtnSize){
                            if(sprites.size()>1){
                                sprites.erase(sprites.begin()+si);
                                if(selectedSpriteIdx>=(int)sprites.size()) selectedSpriteIdx=(int)sprites.size()-1;
                                for(int j=0;j<(int)sprites.size();j++) sprites[j].selected=(j==selectedSpriteIdx);
                            }
                            handledSprite=true; break;
                        }

                        // کلیک عادی برای انتخاب
                        if(mx>=tx&&mx<=tx+thumbSz&&my>=ty&&my<=ty+thumbSz){
                            selectedSpriteIdx=si;
                            for(int j=0;j<(int)sprites.size();j++) sprites[j].selected=(j==si);
                            handledSprite=true; break;
                        }
                    }
                }

                // ── Stage sprite drag ──
                if (!clickedOnField) {
                    int stageX=L.PALETTE_WIDTH,stageY=L.TOOLBAR_HEIGHT,stageW=L.STAGE_WIDTH,stageH=L.STAGE_HEIGHT;
                    int stageCX=stageX+stageW/2, stageCY=stageY+stageH/2;
                    if(mx>=stageX&&mx<=stageX+stageW&&my>=stageY&&my<=stageY+stageH){
                        for(int si=(int)sprites.size()-1;si>=0;si--){
                            Sprite& sp=sprites[si];
                            if(!sp.visible) continue;
                            int sx=stageCX+(int)sp.x, sy=stageCY-(int)sp.y;
                            int spSz=(int)(30*L.s*sp.size/100.0f);
                            if(abs(mx-sx)<spSz&&abs(my-sy)<spSz){
                                draggingSprite=true; dragSpriteIdx=si;
                                spDragOffX=mx-sx; spDragOffY=my-sy;
                                selectedSpriteIdx=si;
                                for(int j=0;j<(int)sprites.size();j++) sprites[j].selected=(j==si);
                                break;
                            }
                        }
                    }
                }

                // ── Block drag ──
                if (!clickedOnField&&!draggingSprite) {
                    for(int i=(int)blocks.size()-1;i>=0;i--){
                        Block& b=blocks[i];
                        if(b.inPalette) continue;
                        int bx=(int)b.x,by=(int)b.y,bw2=(int)b.w,bh2=(int)b.h;
                        if(mx>=bx&&mx<=bx+bw2&&my>=by&&my<=by+bh2){detachBlock(blocks,b.id);dragBlockId=b.id;dragOffX=mx-b.x;dragOffY=my-b.y;break;}
                    }
                    if(dragBlockId<0){
                        int palX=L.CAT_PANEL_WIDTH;
                        float yy=(float)(L.TOOLBAR_HEIGHT+5+paletteScrollY);
                        for(auto& b:blocks){
                            if(!b.inPalette||b.cat!=selectedCategory) continue;
                            float drawX=(float)palX+5,drawY=yy;
                            if(mx>=drawX&&mx<=drawX+b.w&&my>=drawY&&my<=drawY+b.h&&my>L.TOOLBAR_HEIGHT){
                                Block nb=cloneBlock(b,(float)mx-b.w/2,(float)my-b.h/2);
                                blocks.push_back(nb);dragBlockId=nb.id;dragOffX=b.w/2;dragOffY=b.h/2;break;
                            }
                            yy+=b.h+8*L.s;
                        }
                    }
                }
            } // end MOUSE DOWN

            // MOUSE MOTION
            if (e.type==SDL_MOUSEMOTION) {
                int mx=e.motion.x, my=e.motion.y;
                if(dragBlockId>=0){Block* db=findBlock(blocks,dragBlockId);if(db){float newX=mx-dragOffX,newY=my-dragOffY;moveBlockChain(blocks,dragBlockId,newX-db->x,newY-db->y);}}
                if(draggingSprite&&dragSpriteIdx>=0&&dragSpriteIdx<(int)sprites.size()){
                    int stageX=L.PALETTE_WIDTH,stageY=L.TOOLBAR_HEIGHT,stageW=L.STAGE_WIDTH,stageH=L.STAGE_HEIGHT;
                    int stageCX=stageX+stageW/2, stageCY=stageY+stageH/2;
                    sprites[dragSpriteIdx].x=(float)(mx-spDragOffX-stageCX);
                    sprites[dragSpriteIdx].y=(float)(stageCY-(my-spDragOffY));
                }
                { int flagX=(int)(winW*0.4f),flagSz=L.TOOLBAR_HEIGHT-10,resetX=flagX+2*(flagSz+10); resetHovered=(mx>=resetX&&mx<=resetX+(int)(60*L.s)&&my>=5&&my<=5+flagSz); }
            }

            // MOUSE UP
            if (e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT) {
                if(dragBlockId>=0){
                    Block* db=findBlock(blocks,dragBlockId);
                    if(db){
                        if(db->x<L.PALETTE_WIDTH) blocks.erase(remove_if(blocks.begin(),blocks.end(),[&](const Block& b){return b.id==dragBlockId;}),blocks.end());
                        else{trySnapBlocks(blocks,dragBlockId);for(auto& b:blocks){if(!b.inPalette&&b.shape==BlockShape::C_BLOCK){b.h=calcCBlockHeight(blocks,b);updateCBlockChildren(blocks,b);}}}
                    }
                    dragBlockId=-1;
                }
                draggingSprite=false; dragSpriteIdx=-1;
            }
        } // end event loop

        // ════════════════════════════════════════════
        //  RENDER
        // ════════════════════════════════════════════
        SDL_SetRenderDrawColor(rnd,240,240,240,255);
        SDL_RenderClear(rnd);

        // ── Toolbar ──
        {
            SDL_SetRenderDrawColor(rnd,55,55,70,255);
            SDL_Rect toolbar={0,0,winW,L.TOOLBAR_HEIGHT}; SDL_RenderFillRect(rnd,&toolbar);
            drawText(rnd,10,(L.TOOLBAR_HEIGHT-textHeight("Scratch IDE"))/2,"Scratch IDE",255,255,255,255);
            int flagX=(int)(winW*0.4f),flagY=5,flagSz=L.TOOLBAR_HEIGHT-10;
            fillRoundedRect(rnd,flagX,flagY,flagSz,flagSz,6,gIsRunning?0:30,gIsRunning?180:150,gIsRunning?0:30,255);
            drawText(rnd,flagX+flagSz/4,flagY+flagSz/4,">",255,255,255,255);
            int stopX=flagX+flagSz+10;
            fillRoundedRect(rnd,stopX,flagY,flagSz,flagSz,6,200,50,50,255);
            drawText(rnd,stopX+flagSz/4,flagY+flagSz/4,"#",255,255,255,255);
            int resetX=stopX+flagSz+10, resetW=(int)(60*L.s);
            fillRoundedRect(rnd,resetX,flagY,resetW,flagSz,6,resetHovered?100:80,resetHovered?100:80,resetHovered?120:100,255);
            drawText(rnd,resetX+5,flagY+flagSz/4,"Reset",255,255,255,255);
            char timerBuf[32]; snprintf(timerBuf,sizeof(timerBuf),"T:%.1f",gTimer);
            drawText(rnd,resetX+resetW+15,flagY+flagSz/4,timerBuf,200,200,200,255);
        }

        // ── Category panel ──
        {
            SDL_SetRenderDrawColor(rnd,45,45,60,255);
            SDL_Rect catPanel={0,L.TOOLBAR_HEIGHT,L.CAT_PANEL_WIDTH,winH-L.TOOLBAR_HEIGHT}; SDL_RenderFillRect(rnd,&catPanel);
            int catY=L.TOOLBAR_HEIGHT+5;
            for(int i=0;i<NUM_CATEGORIES;i++){
                Category c=(Category)i; SDL_Color cc=catColor(c);
                int btnY=catY+i*(L.CAT_BTN_HEIGHT+3); bool sel=(c==selectedCategory);
                if(sel){fillRoundedRect(rnd,3,btnY,L.CAT_PANEL_WIDTH-6,L.CAT_BTN_HEIGHT,6,cc.r,cc.g,cc.b,255);drawText(rnd,10,btnY+(L.CAT_BTN_HEIGHT-textHeight("A"))/2,catName(c),255,255,255,255);}
                else{fillRoundedRect(rnd,3,btnY,L.CAT_PANEL_WIDTH-6,L.CAT_BTN_HEIGHT,6,60,60,75,255);drawText(rnd,10,btnY+(L.CAT_BTN_HEIGHT-textHeight("A"))/2,catName(c),cc.r,cc.g,cc.b,255);}
            }
        }

        // ── Palette ──
        {
            SDL_SetRenderDrawColor(rnd,50,50,65,255);
            SDL_Rect palBg={L.CAT_PANEL_WIDTH,L.TOOLBAR_HEIGHT,L.PALETTE_WIDTH-L.CAT_PANEL_WIDTH,winH-L.TOOLBAR_HEIGHT}; SDL_RenderFillRect(rnd,&palBg);
            SDL_Rect clip={L.CAT_PANEL_WIDTH,L.TOOLBAR_HEIGHT,L.PALETTE_WIDTH-L.CAT_PANEL_WIDTH,winH-L.TOOLBAR_HEIGHT}; SDL_RenderSetClipRect(rnd,&clip);
            float yy=(float)(L.TOOLBAR_HEIGHT+5+paletteScrollY);
            for(auto& b:blocks){if(!b.inPalette||b.cat!=selectedCategory)continue;b.x=(float)(L.CAT_PANEL_WIDTH+5);b.y=yy;drawBlock(rnd,b,blocks);yy+=b.h+8*L.s;}
            SDL_RenderSetClipRect(rnd,nullptr);
        }

        // ── Stage ──
        {
            int stageX=L.PALETTE_WIDTH,stageY=L.TOOLBAR_HEIGHT,stageW=L.STAGE_WIDTH,stageH=L.STAGE_HEIGHT;SDL_SetRenderDrawColor(rnd,255,255,255,255);
            SDL_Color bgCol = BG_COLORS[gBgColor];
            SDL_SetRenderDrawColor(rnd, bgCol.r, bgCol.g, bgCol.b, 255);
            SDL_Rect stageRect={stageX,stageY,stageW,stageH}; SDL_RenderFillRect(rnd,&stageRect);SDL_SetRenderDrawColor(rnd,200,200,200,255); SDL_RenderDrawRect(rnd,&stageRect);
            int bgBtnX = stageX + stageW - (int)(35*L.s);
            int bgBtnY = stageY + 3;
            int bgBtnW = (int)(32*L.s);
            int bgBtnH = (int)(18*L.s);
            fillRoundedRect(rnd, bgBtnX, bgBtnY, bgBtnW, bgBtnH, 4, 100,100,120, 255);
            drawText(rnd, bgBtnX+3, bgBtnY+3, "BG", 255,255,255,255);
            int stageCX=stageX+stageW/2, stageCY=stageY+stageH/2;
            SDL_SetRenderDrawColor(rnd,235,235,235,255);
            SDL_RenderDrawLine(rnd,stageCX,stageY,stageCX,stageY+stageH);
            SDL_RenderDrawLine(rnd,stageX,stageCY,stageX+stageW,stageCY);

            SDL_Rect stageClip={stageX,stageY,stageW,stageH}; SDL_RenderSetClipRect(rnd,&stageClip);
            for(int si=0;si<(int)sprites.size();si++){
                Sprite& sp=sprites[si];
                if(!sp.visible) continue;
                int sx=stageCX+(int)sp.x, sy=stageCY-(int)sp.y;
                int sz=(int)(30*L.s*sp.size/100.0f);

                // اعمال colorEffect
                SDL_Color drawCol = sp.color;
                if (sp.colorEffect == 1) { drawCol = {255,100,100,255}; }
                else if (sp.colorEffect == 2) { drawCol = {100,255,100,255}; }
                else if (sp.colorEffect == 3) { drawCol = {100,100,255,255}; }
                else if (sp.colorEffect == 4) { drawCol = {255,255,100,255}; }
                else if (sp.colorEffect == 5) { drawCol = {200,100,255,255}; }

                // اعمال ghostEffect (شفافیت)
                drawCol.a = (Uint8)(255 * (1.0f - sp.ghostEffect / 100.0f));

                if (sp.uploadedTexture) {
                    // رسم تصویر آپلود شده
                    SDL_Rect srcRect = {0,0,0,0};
                    SDL_QueryTexture(sp.uploadedTexture, NULL, NULL, &srcRect.w, &srcRect.h);
                    SDL_Rect dstRect = {sx-sz/2, sy-sz/2, sz, sz};
                    SDL_RenderCopy(rnd, sp.uploadedTexture, &srcRect, &dstRect);
                } else {
                    // رسم گربه پیش‌فرض
                    drawCatSprite(rnd,sx,sy,sz,drawCol);
                }                // نشانگر جهت
                float angle=(sp.direction-90)*M_PI/180.0f;
                int arrowX=sx+(int)((sz*0.8f)*cos(angle));
                int arrowY=sy+(int)((sz*0.8f)*sin(angle));
                SDL_SetRenderDrawColor(rnd,0,0,0,180);
                SDL_RenderDrawLine(rnd,sx,sy,arrowX,arrowY);
                fillEllipse(rnd,arrowX,arrowY,(int)(3*L.s),(int)(3*L.s),0,0,0,255);

                // کادر انتخاب
                if(si==selectedSpriteIdx){SDL_SetRenderDrawColor(rnd,50,150,255,255);SDL_Rect selR={sx-sz-3,sy-sz-3,2*sz+6,2*sz+6};SDL_RenderDrawRect(rnd,&selR);}

                if(!sp.sayText.empty()) drawSpeechBubble(rnd,sx,sy-sz-5,sp.sayText.c_str(),false);
                if(!sp.thinkText.empty()) drawSpeechBubble(rnd,sx,sy-sz-5,sp.thinkText.c_str(),true);
            }
            SDL_RenderSetClipRect(rnd,nullptr);
        }

        // ── Sprite panel (below stage) ──
        {
            int stageX=L.PALETTE_WIDTH, stageW=L.STAGE_WIDTH;
            int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
            int spriteAreaH=winH-spriteAreaY;
            int thumbSz=L.SPRITE_THUMB;
            int delBtnSize=(int)(16*L.s);
            int eyeBtnSize=(int)(16*L.s);

            SDL_SetRenderDrawColor(rnd,230,230,240,255);
            SDL_Rect spArea={stageX,spriteAreaY,stageW,spriteAreaH}; SDL_RenderFillRect(rnd,&spArea);

            int startX=stageX+5;
            for(int si=0;si<(int)sprites.size();si++){
                int tx=startX+si*(thumbSz+8), ty=spriteAreaY;
                bool isSel=(si==selectedSpriteIdx);
                fillRoundedRect(rnd,tx,ty,thumbSz,thumbSz,6,isSel?200:240,isSel?220:240,isSel?255:240,255);
                if(isSel){SDL_SetRenderDrawColor(rnd,50,150,255,255);SDL_Rect selBdr={tx,ty,thumbSz,thumbSz};SDL_RenderDrawRect(rnd,&selBdr);}

                // اگه مخفیه، کمرنگ‌تر نشون بده
                SDL_Color thumbCol = sprites[si].color;
                if(!sprites[si].visible){ thumbCol.r=(Uint8)(thumbCol.r*0.4f); thumbCol.g=(Uint8)(thumbCol.g*0.4f); thumbCol.b=(Uint8)(thumbCol.b*0.4f); }
                drawCatSprite(rnd,tx+thumbSz/2,ty+thumbSz/2,thumbSz/2-4,thumbCol);

                drawText(rnd,tx+2,ty+thumbSz-textHeight("A")-2,sprites[si].name.c_str(),0,0,0,255);

                // دکمه چشم (show/hide) - سبز=نمایان، خاکستری=مخفی
                int eyeBtnX=tx+thumbSz-delBtnSize-eyeBtnSize-6, eyeBtnY=ty+2;
                Uint8 eyeR=sprites[si].visible?50:150, eyeG=sprites[si].visible?150:150, eyeB2=sprites[si].visible?50:150;
                fillRoundedRect(rnd,eyeBtnX,eyeBtnY,eyeBtnSize,eyeBtnSize,4,eyeR,eyeG,eyeB2,255);
                drawText(rnd,eyeBtnX+eyeBtnSize/4,eyeBtnY+1,"O",255,255,255,255);

                // دکمه X (حذف) - قرمز
                int delBtnX=tx+thumbSz-delBtnSize-2, delBtnY=ty+2;
                fillRoundedRect(rnd,delBtnX,delBtnY,delBtnSize,delBtnSize,4,220,50,50,255);
                drawText(rnd,delBtnX+delBtnSize/4,delBtnY+1,"X",255,255,255,255);
            }

            // دکمه + اضافه کردن sprite
            {
                int addBtnX=stageX+stageW-(int)(40*L.s), addBtnY=spriteAreaY, addBtnSz=(int)(30*L.s);
                fillRoundedRect(rnd,addBtnX,addBtnY,addBtnSz,addBtnSz,8,50,150,50,255);
                drawText(rnd,addBtnX+addBtnSz/3,addBtnY+addBtnSz/4,"+",255,255,255,255);
            }

            // ── Sprite info fields ──
            if(selectedSpriteIdx<(int)sprites.size()){
                Sprite& sp=sprites[selectedSpriteIdx];
                int infoY=spriteAreaY+thumbSz+10, infoX=stageX+5;
                int fieldW2=(int)(stageW*0.35f), fieldH2=(int)(22*L.s);

                // Name
                {
                    drawText(rnd,infoX,infoY+(fieldH2-textHeight("A"))/2,"Name:",80,80,80,255);
                    int fx=infoX+(int)(40*L.s); bool editing=(sprInfoEdit.field==0);
                    fillRoundedRect(rnd,fx,infoY,fieldW2*2,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,infoY-1,fieldW2*2+2,fieldH2+2,4,50,150,255,255);
                    const char* dispText=editing?sprInfoEdit.buffer.c_str():sp.name.c_str();
                    drawText(rnd,fx+4,infoY+(fieldH2-textHeight("A"))/2,dispText,0,0,0,255);
                    if(editing){int cw=textWidth(dispText);SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,infoY+2,fx+4+cw,infoY+fieldH2-2);}
                }

                int row2Y=infoY+fieldH2+8;
                // X
                {
                    drawText(rnd,infoX,row2Y+(fieldH2-textHeight("A"))/2,"X:",80,80,80,255);
                    int fx=infoX+(int)(15*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==1);
                    fillRoundedRect(rnd,fx,row2Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row2Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.x);
                    drawText(rnd,fx+4,row2Y+(fieldH2-textHeight("A"))/2,val.c_str(),0,0,0,255);
                    if(editing){int cw=textWidth(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row2Y+2,fx+4+cw,row2Y+fieldH2-2);}
                }
                // Y
                {
                    int fx=infoX+fieldW2+(int)(15*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==2);
                    drawText(rnd,infoX+fieldW2,row2Y+(fieldH2-textHeight("A"))/2,"Y:",80,80,80,255);
                    fillRoundedRect(rnd,fx,row2Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row2Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.y);
                    drawText(rnd,fx+4,row2Y+(fieldH2-textHeight("A"))/2,val.c_str(),0,0,0,255);
                    if(editing){int cw=textWidth(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row2Y+2,fx+4+cw,row2Y+fieldH2-2);}
                }

                int row3Y=row2Y+fieldH2+8;
                // Size
                {
                    drawText(rnd,infoX,row3Y+(fieldH2-textHeight("A"))/2,"Sz:",80,80,80,255);
                    int fx=infoX+(int)(30*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==3);
                    fillRoundedRect(rnd,fx,row3Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row3Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.size);
                    drawText(rnd,fx+4,row3Y+(fieldH2-textHeight("A"))/2,val.c_str(),0,0,0,255);


                    if(editing){int cw=textWidth(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row3Y+2,fx+4+cw,row3Y+fieldH2-2);}
                    // دکمه Upload Costume
                    int uploadBtnW = (int)(60*L.s), uploadBtnH = fieldH2;
                    int uploadBtnX = infoX + fieldW2*2 - uploadBtnW - 10;
                    int uploadBtnY = infoY;
                    fillRoundedRect(rnd, uploadBtnX, uploadBtnY, uploadBtnW, uploadBtnH, 4, 50,150,50,255);

                    drawText(rnd, uploadBtnX+10, uploadBtnY+(uploadBtnH-textHeight("A"))/2, "Upload", 255,255,255,255);
                    // دکمه Upload Costume
                    uploadBtnW = (int)(60*L.s), uploadBtnH = fieldH2;
                    uploadBtnX = infoX + fieldW2*2 - uploadBtnW - 10;
                    uploadBtnY = infoY;
                    fillRoundedRect(rnd, uploadBtnX, uploadBtnY, uploadBtnW, uploadBtnH, 4, 50,150,50,255);
                    drawText(rnd, uploadBtnX+10, uploadBtnY+(uploadBtnH-textHeight("A"))/2, "Upload", 255,255,255,255);
                    int row4Y = row3Y + fieldH2 + 8;

                    {
                        drawText(rnd,infoX,row4Y+(fieldH2-textHeight("A"))/2,"Gh:",80,80,80,255);
                        int fx=infoX+(int)(30*L.s),fw=fieldW2-(int)(20*L.s);
                        bool editing=(sprInfoEdit.field==5);
                        fillRoundedRect(rnd,fx,row4Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                        if(editing) drawRoundedRectOutline(rnd,fx-1,row4Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                        string val=editing?sprInfoEdit.buffer:floatToString(sp.ghostEffect);
                        drawText(rnd,fx+4,row4Y+(fieldH2-textHeight("A"))/2,val.c_str(),0,0,0,255);
                        if(editing){int cw=textWidth(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row4Y+2,fx+4+cw,row4Y+fieldH2-2);}
                    }
                }
                // Direction
                {
                    int fx=infoX+fieldW2+(int)(25*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==4);
                    drawText(rnd,infoX+fieldW2,row3Y+(fieldH2-textHeight("A"))/2,"Dir:",80,80,80,255);
                    fillRoundedRect(rnd,fx,row3Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row3Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.direction);
                    drawText(rnd,fx+4,row3Y+(fieldH2-textHeight("A"))/2,val.c_str(),0,0,0,255);
                    if(editing){int cw=textWidth(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row3Y+2,fx+4+cw,row3Y+fieldH2-2);}
                }
            }
        }

        // ── Workspace ──
        {
            int wsX=L.PALETTE_WIDTH+L.STAGE_WIDTH, wsY=L.TOOLBAR_HEIGHT;
            int wsW=winW-wsX, wsH=winH-wsY;
            SDL_SetRenderDrawColor(rnd,245,245,250,255);
            SDL_Rect wsRect={wsX,wsY,wsW,wsH}; SDL_RenderFillRect(rnd,&wsRect);
            SDL_SetRenderDrawColor(rnd,230,230,235,255);
            for(int gx=wsX;gx<wsX+wsW;gx+=40) SDL_RenderDrawLine(rnd,gx,wsY,gx,wsY+wsH);
            for(int gy=wsY;gy<wsY+wsH;gy+=40) SDL_RenderDrawLine(rnd,wsX,gy,wsX+wsW,gy);
            drawText(rnd,wsX+10,wsY+5,"Code Workspace",150,150,160,255);
        }

        // ── Draw workspace blocks ──
        {
            for(auto& b:blocks){if(b.inPalette||b.id==dragBlockId) continue; drawBlock(rnd,b,blocks);}
            for(auto& b:blocks){if(b.inPalette) continue; for(auto& sl:b.opSlots){if(sl.embeddedBlockId>=0){Block* emb=findBlock(blocks,sl.embeddedBlockId);if(emb)drawBlock(rnd,*emb,blocks);}}}
            if(dragBlockId>=0){
                Block* db=findBlock(blocks,dragBlockId);
                if(db){
                    drawBlock(rnd,*db,blocks,true);
                    for(auto& other:blocks){
                        if(other.id==dragBlockId||other.inPalette||other.nextBlockId>=0) continue;
                        if(other.shape==BlockShape::CAP||other.shape==BlockShape::REPORTER||other.shape==BlockShape::BOOLEAN) continue;
                        float ox=other.x,oy=other.y+other.h;
                        if(abs(db->x-ox)<L.SNAP_DISTANCE&&abs(db->y-oy)<L.SNAP_DISTANCE){SDL_SetRenderDrawColor(rnd,50,150,255,150);SDL_Rect prev={(int)ox,(int)oy-2,(int)db->w,4};SDL_RenderFillRect(rnd,&prev);}
                        if(other.shape==BlockShape::C_BLOCK){float indent=20*L.s,barH=L.CBLOCK_BAR_H,mouthX=other.x+indent,mouthY=other.y+barH;if(abs(db->x-mouthX)<L.SNAP_DISTANCE&&abs(db->y-mouthY)<L.SNAP_DISTANCE){SDL_SetRenderDrawColor(rnd,255,200,50,150);SDL_Rect prev={(int)mouthX,(int)mouthY-2,(int)(other.w-indent),4};SDL_RenderFillRect(rnd,&prev);}}
                    }
                }
            }
        }

        SDL_RenderPresent(rnd);
        SDL_Delay(16);
    } // end main loop

    SDL_StopTextInput();
    SDL_DestroyRenderer(rnd);
    SDL_DestroyWindow(window);
    IMG_Quit();
    for(auto& sp : sprites){
        if(sp.uploadedTexture) SDL_DestroyTexture(sp.uploadedTexture);
    }
    SDL_Quit();
    return 0;
}