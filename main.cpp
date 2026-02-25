#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <bits/stdc++.h>
#include <SDL_image.h>
#include "tinyfiledialogs.h"
#include <SDL2/SDL2_gfxPrimitives.h>
using namespace std;

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

static TTF_Font* gFontSmall  = nullptr;
static TTF_Font* gFontNormal = nullptr;
static TTF_Font* gFontLarge  = nullptr;
static int gFontSizeNormal = 13;
static string gFontPath = "DejaVuSans.ttf";

static SDL_Renderer* gRenderer = nullptr;
static SDL_Renderer* rnd = nullptr;
static int gHighlightBlockId = -1;
static int activeSpriteTab = 0;

struct ScriptThread {
    int currentBlockId;
    int spriteIdx;
    float waitTimer;
    bool isWaiting;
    bool finished;
    vector<pair<int,int>> loopStack;
    ScriptThread(int blockId, int sprite)
        : currentBlockId(blockId), spriteIdx(sprite),
          waitTimer(0), isWaiting(false), finished(false) {}
};

static vector<ScriptThread> gActiveThreads;

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
        float fontFactor = max(s, 0.85f);
        BLOCK_WIDTH      = BASE_BLOCK_WIDTH  * fontFactor;
        BLOCK_HEIGHT     = BASE_BLOCK_HEIGHT * sy;
        CBLOCK_MIN_H     = BASE_CBLOCK_MIN_H * sy;
        CBLOCK_MOUTH_H   = BASE_CBLOCK_MOUTH_H * sy;
        CBLOCK_BAR_H     = BASE_CBLOCK_BAR_H * sy;
        BLOCK_CORNER_R   = BASE_BLOCK_CORNER_R * s;
        SNAP_DISTANCE    = BASE_SNAP_DISTANCE * s;
        SNAP_VERT_OVERLAP= BASE_SNAP_VERT_OVERLAP * sy;
        fontScale        = max(1, (int)(gFontSizeNormal * s));
    }
};
static LayoutScale L;

static void initFonts(const char* path, int baseSize) {
    gFontPath = path;
    gFontSizeNormal = baseSize;
    if (TTF_Init() < 0) return;
    gFontSmall  = TTF_OpenFont(path, baseSize - 2);
    gFontNormal = TTF_OpenFont(path, baseSize);
    gFontLarge  = TTF_OpenFont(path, baseSize + 6);
}

static void closeFonts() {
    if (gFontSmall)  TTF_CloseFont(gFontSmall);
    if (gFontNormal) TTF_CloseFont(gFontNormal);
    if (gFontLarge)  TTF_CloseFont(gFontLarge);
    gFontSmall = gFontNormal = gFontLarge = nullptr;
    TTF_Quit();
}

static void setFontSize(int sz) {
    gFontSizeNormal = sz;
    if (gFontNormal) TTF_CloseFont(gFontNormal);
    gFontNormal = TTF_OpenFont(gFontPath.c_str(), sz);
}

static void drawTextTTF(SDL_Renderer* r, const char* text, int x, int y,
                         SDL_Color col, TTF_Font* font = nullptr) {
    TTF_Font* f = font ? font : gFontNormal;
    if (!f || !text || !text[0]) return;
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, text, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static int textWidthTTF(const char* text, TTF_Font* font = nullptr) {
    TTF_Font* f = font ? font : gFontNormal;
    if (!f || !text) return 0;
    int w = 0, h = 0;
    TTF_SizeUTF8(f, text, &w, &h);
    return w;
}

static int textHeightTTF(TTF_Font* font = nullptr) {
    TTF_Font* f = font ? font : gFontNormal;
    if (!f) return 14;
    return TTF_FontHeight(f);
}

static void fillRoundedRect(SDL_Renderer* r, int x, int y, int w, int h, int rad,
                             Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    roundedBoxRGBA(r, x, y, x+w, y+h, rad, cr, cg, cb, ca);
    if (rad > 0) {
        aacircleRGBA(r, x+rad, y+rad, rad, cr, cg, cb, ca);
        aacircleRGBA(r, x+w-rad, y+rad, rad, cr, cg, cb, ca);
        aacircleRGBA(r, x+rad, y+h-rad, rad, cr, cg, cb, ca);
        aacircleRGBA(r, x+w-rad, y+h-rad, rad, cr, cg, cb, ca);
    }
}

static void fillEllipse(SDL_Renderer* r, int cx, int cy, int rx, int ry,
                          Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    filledEllipseRGBA(r, cx, cy, rx, ry, cr, cg, cb, ca);
    aaellipseRGBA(r, cx, cy, rx, ry, cr, cg, cb, ca);
}

static void drawRoundedRectOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad,
                                     Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    roundedRectangleRGBA(r, x, y, x+w, y+h, rad, cr, cg, cb, ca);
}

enum Category { MOTION, LOOKS, EVENTS, CONTROL, OPERATORS, VARIABLES };
static const int NUM_CATEGORIES = 6;

static SDL_Color catColor(Category c) {
    switch(c){
        case MOTION:    return {100,160,240,255};
        case LOOKS:     return {180,100,220,255};
        case EVENTS:    return {230,180,0,255};
        case CONTROL:   return {230,160,0,255};
        case OPERATORS: return {80,200,80,255};
        case VARIABLES: return {230,120,0,255};
    }
    return {128,128,128,255};
}

static const char* catName(Category c) {
    switch(c){
        case MOTION:    return "Motion";
        case LOOKS:     return "Looks";
        case EVENTS:    return "Events";
        case CONTROL:   return "Control";
        case OPERATORS: return "Operators";
        case VARIABLES: return "Variables";
    }
    return "?";
}

struct Sprite {
    string name;
    float x, y;
    float direction;
    float size;
    bool visible;
    bool selected;
    SDL_Color color;
    int currentCostume;
    SDL_Texture* uploadedTexture;
    int uploadedW, uploadedH;
    Sprite() : x(0), y(0), direction(90), size(100),
               visible(true), selected(false),
               color({100,160,240,255}), currentCostume(0),
               uploadedTexture(nullptr), uploadedW(0), uploadedH(0) {}
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
    sp.currentCostume = 0;
    sp.uploadedTexture = nullptr;
    sp.uploadedW = 0;
    sp.uploadedH = 0;
    return sp;
}

enum BlockShape { COMMAND, C_BLOCK, HAT, CAP, REPORTER, BOOLEAN };

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

static bool gIsRunning = true;
static float gTimer = 0;
static SDL_Color gBgColor = {255, 255, 255, 255};
static float gToolbarAnimOffset = 0;

struct ActiveEdit {
    int blockId = -1;
    int inputIdx = -1;
};
static ActiveEdit gActiveEdit;

static string intToString(int v) {
    char buf[64]; snprintf(buf, sizeof(buf), "%d", v);
    return string(buf);
}
static string floatToString(float v) {
    char buf[64]; snprintf(buf, sizeof(buf), "%.1f", v);
    return string(buf);
}

static Block makeBlock(int id, Category cat, BlockShape shape, const char* text,
                        float x, float y, float w, float h, bool inPalette) {
    Block b;
    b.id = id; b.cat = cat; b.shape = shape; b.text = text;
    b.x = x; b.y = y; b.w = w; b.h = h;
    b.inPalette = inPalette;
    b.nextBlockId = -1; b.parentBlockId = -1; b.childHeadId = -1;
    return b;
}

static InputField makeInput(const char* def, float rx, float ry, float w, float h) {
    InputField f;
    f.value = def; f.defaultVal = def;
    f.relX = rx; f.relY = ry; f.width = w; f.height = h;
    f.editing = false;
    return f;
}

static OperatorSlot makeOpSlot(float rx, float ry, float w, float h) {
    OperatorSlot s;
    s.relX = rx; s.relY = ry; s.width = w; s.height = h;
    s.embeddedBlockId = -1;
    return s;
}

static vector<Block> gBlocks;
static vector<Sprite> gSprites;
static vector<vector<int>> gSpriteBlockIds;
static Category gSelectedCat = MOTION;

static Block* findBlock(int id) {
    for (auto& b : gBlocks) if (b.id == id) return &b;
    return nullptr;
}

static int findBlockIndex(int id) {
    for (int i = 0; i < (int)gBlocks.size(); i++)
        if (gBlocks[i].id == id) return i;
    return -1;
}

static float getInputValue(Block& b, int idx) {
    if (idx < 0 || idx >= (int)b.inputs.size()) return 0;
    try { return stof(b.inputs[idx].value); }
    catch (...) { return 0; }
}

static void buildPaletteBlocks() {
    gBlocks.clear();
    float bw = L.BLOCK_WIDTH, bh = L.BLOCK_HEIGHT;
    float px = 10, py = 10;
    float gap = bh + 6;
    int id = 100;

    {
        float y = py;
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "move %1 steps", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("10", bw*0.30f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "turn right %1 deg", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("15", bw*0.50f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "turn left %1 deg", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("15", bw*0.48f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "go to x:%1 y:%2", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("0", bw*0.35f, bh*0.15f, bw*0.18f, bh*0.7f));
            b.inputs.push_back(makeInput("0", bw*0.68f, bh*0.15f, bw*0.18f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "glide %1s to x:%2 y:%3", px, y, bw*1.2f, bh, true);
            b.inputs.push_back(makeInput("1", bw*0.22f, bh*0.15f, bw*0.12f, bh*0.7f));
            b.inputs.push_back(makeInput("0", bw*0.55f, bh*0.15f, bw*0.15f, bh*0.7f));
            b.inputs.push_back(makeInput("0", bw*0.85f, bh*0.15f, bw*0.15f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "point in dir %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("90", bw*0.58f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "change x by %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("10", bw*0.55f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "set x to %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("0", bw*0.45f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "change y by %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("10", bw*0.55f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, COMMAND, "set y to %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("0", bw*0.45f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, REPORTER, "x position", px, y, bw*0.6f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, REPORTER, "y position", px, y, bw*0.6f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, MOTION, REPORTER, "direction", px, y, bw*0.6f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, LOOKS, COMMAND, "show", px, y, bw*0.55f, bh, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, LOOKS, COMMAND, "hide", px, y, bw*0.55f, bh, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, LOOKS, COMMAND, "set size to %1 %", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("100", bw*0.50f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, LOOKS, COMMAND, "change size by %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("10", bw*0.60f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, LOOKS, REPORTER, "size", px, y, bw*0.45f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, EVENTS, HAT, "when green flag clicked", px, y, bw*1.1f, bh*1.2f, true);
            gBlocks.push_back(b); y += bh*1.2f + 6;
        }
        {
            Block b = makeBlock(id++, EVENTS, HAT, "when space key pressed", px, y, bw*1.1f, bh*1.2f, true);
            gBlocks.push_back(b); y += bh*1.2f + 6;
        }
        {
            Block b = makeBlock(id++, EVENTS, HAT, "when this sprite clicked", px, y, bw*1.1f, bh*1.2f, true);
            gBlocks.push_back(b); y += bh*1.2f + 6;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, CONTROL, COMMAND, "wait %1 secs", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("1", bw*0.35f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, CONTROL, C_BLOCK, "repeat %1", px, y, bw, L.CBLOCK_MIN_H, true);
            b.inputs.push_back(makeInput("10", bw*0.42f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += L.CBLOCK_MIN_H + 6;
        }
        {
            Block b = makeBlock(id++, CONTROL, C_BLOCK, "forever", px, y, bw, L.CBLOCK_MIN_H, true);
            gBlocks.push_back(b); y += L.CBLOCK_MIN_H + 6;
        }
        {
            Block b = makeBlock(id++, CONTROL, C_BLOCK, "if %1 then", px, y, bw, L.CBLOCK_MIN_H, true);
            b.opSlots.push_back(makeOpSlot(bw*0.22f, bh*0.15f, bw*0.35f, bh*0.7f));
            gBlocks.push_back(b); y += L.CBLOCK_MIN_H + 6;
        }
        {
            Block b = makeBlock(id++, CONTROL, CAP, "stop all", px, y, bw*0.7f, bh, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 + %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 - %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 * %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 / %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "%1 > %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "%1 < %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "%1 = %2", px, y, bw*0.65f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.15f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.38f, bh*0.15f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "pick random %1 to %2", px, y, bw*0.95f, bh*0.75f, true);
            b.inputs.push_back(makeInput("1", bw*0.52f, bh*0.15f, bw*0.15f, bh*0.55f));
            b.inputs.push_back(makeInput("10", bw*0.75f, bh*0.15f, bw*0.15f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, VARIABLES, COMMAND, "set myVar to %1", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("0", bw*0.62f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, VARIABLES, COMMAND, "change myVar by %1", px, y, bw*1.05f, bh, true);
            b.inputs.push_back(makeInput("1", bw*0.70f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, VARIABLES, REPORTER, "myVar", px, y, bw*0.5f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
    }
}

static float computeChainHeight(int blockId) {
    float total = 0;
    int cur = blockId;
    while (cur != -1) {
        Block* b = findBlock(cur);
        if (!b) break;
        total += b->h;
        if (b->shape == C_BLOCK) {
            float mouthH = L.CBLOCK_MOUTH_H;
            if (b->childHeadId != -1)
                mouthH = computeChainHeight(b->childHeadId);
            b->h = L.BLOCK_HEIGHT + mouthH + L.CBLOCK_BAR_H;
            if (b->h < L.CBLOCK_MIN_H) b->h = L.CBLOCK_MIN_H;
            total = total - b->h + b->h;
        }
        cur = b->nextBlockId;
    }
    return total;
}

static void repositionChain(int headId, float startX, float startY) {
    float cy = startY;
    int cur = headId;
    while (cur != -1) {
        Block* b = findBlock(cur);
        if (!b) break;
        b->x = startX;
        b->y = cy;
        if (b->shape == C_BLOCK) {
            float mouthH = L.CBLOCK_MOUTH_H;
            if (b->childHeadId != -1) {
                mouthH = computeChainHeight(b->childHeadId);
                repositionChain(b->childHeadId, startX + 20, cy + L.BLOCK_HEIGHT);
            }
            b->h = L.BLOCK_HEIGHT + mouthH + L.CBLOCK_BAR_H;
            if (b->h < L.CBLOCK_MIN_H) b->h = L.CBLOCK_MIN_H;
        }
        cy += b->h - L.SNAP_VERT_OVERLAP;
        cur = b->nextBlockId;
    }
}

static void detachBlock(int blockId) {
    Block* b = findBlock(blockId);
    if (!b) return;
    int parentId = b->parentBlockId;
    if (parentId == -1) return;
    Block* parent = findBlock(parentId);
    if (!parent) return;
    if (parent->nextBlockId == blockId)
        parent->nextBlockId = -1;
    if (parent->childHeadId == blockId)
        parent->childHeadId = -1;
    b->parentBlockId = -1;
}

static void attachAfter(int targetId, int movingId) {
    Block* target = findBlock(targetId);
    Block* moving = findBlock(movingId);
    if (!target || !moving) return;
    int oldNext = target->nextBlockId;
    target->nextBlockId = movingId;
    moving->parentBlockId = targetId;
    int last = movingId;
    while (true) {
        Block* lb = findBlock(last);
        if (!lb || lb->nextBlockId == -1) break;
        last = lb->nextBlockId;
    }
    if (oldNext != -1) {
        Block* on = findBlock(oldNext);
        if (on) {
            Block* lb = findBlock(last);
            if (lb) { lb->nextBlockId = oldNext; on->parentBlockId = last; }
        }
    }
}

static void attachAsChild(int cBlockId, int childId) {
    Block* cBlock = findBlock(cBlockId);
    Block* child = findBlock(childId);
    if (!cBlock || !child) return;
    cBlock->childHeadId = childId;
    child->parentBlockId = cBlockId;
}

static int findTopOfChain(int blockId) {
    int cur = blockId;
    while (true) {
        Block* b = findBlock(cur);
        if (!b || b->parentBlockId == -1) return cur;
        cur = b->parentBlockId;
    }
}

static void drawBlock(SDL_Renderer* r, Block& b, bool highlight = false) {
    SDL_Color col = catColor(b.cat);
    int bx = (int)b.x, by = (int)b.y, bw = (int)b.w, bh = (int)b.h;
    int rad = (int)L.BLOCK_CORNER_R;

    if (b.shape == REPORTER) {
        int rx = bw/2, ry = bh/2;
        fillEllipse(r, bx + bw/2, by + bh/2, rx, ry, col.r, col.g, col.b, col.a);
        if (highlight)
            aaellipseRGBA(r, bx+bw/2, by+bh/2, rx, ry, 255, 255, 0, 255);
    } else if (b.shape == BOOLEAN) {
        int cx = bx + bw/2, cy = by + bh/2;
        int hw = bw/2, hh = bh/2;
        Sint16 vx[] = {(Sint16)(cx-hw), (Sint16)(cx-hw+hh), (Sint16)(cx+hw-hh), (Sint16)(cx+hw), (Sint16)(cx+hw-hh), (Sint16)(cx-hw+hh)};
        Sint16 vy[] = {(Sint16)cy, (Sint16)(cy-hh), (Sint16)(cy-hh), (Sint16)cy, (Sint16)(cy+hh), (Sint16)(cy+hh)};
        filledPolygonRGBA(r, vx, vy, 6, col.r, col.g, col.b, col.a);
        if (highlight)
            aapolygonRGBA(r, vx, vy, 6, 255, 255, 0, 255);
    } else if (b.shape == HAT) {
        fillRoundedRect(r, bx, by, bw, bh, rad, col.r, col.g, col.b, col.a);
        filledPieRGBA(r, bx + bw/2, by, (int)(bw*0.25f), 180, 360, col.r, col.g, col.b, col.a);
        if (highlight)
            drawRoundedRectOutline(r, bx, by, bw, bh, rad, 255, 255, 0, 255);
    } else if (b.shape == C_BLOCK) {
        float topH = L.BLOCK_HEIGHT;
        float mouthH = b.h - topH - L.CBLOCK_BAR_H;
        if (mouthH < L.CBLOCK_MOUTH_H) mouthH = L.CBLOCK_MOUTH_H;
        fillRoundedRect(r, bx, by, bw, (int)topH, rad, col.r, col.g, col.b, col.a);
        fillRoundedRect(r, bx, by + (int)(topH + mouthH), bw, (int)L.CBLOCK_BAR_H, rad, col.r, col.g, col.b, col.a);
        SDL_Rect side = {bx, by + (int)topH, 20, (int)mouthH};
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_RenderFillRect(r, &side);
        if (highlight) {
            drawRoundedRectOutline(r, bx, by, bw, (int)topH, rad, 255, 255, 0, 255);
            drawRoundedRectOutline(r, bx, by+(int)(topH+mouthH), bw, (int)L.CBLOCK_BAR_H, rad, 255,255,0,255);
        }
    } else {
        fillRoundedRect(r, bx, by, bw, bh, rad, col.r, col.g, col.b, col.a);
        if (b.shape == CAP) {
            Sint16 tx[] = {(Sint16)bx, (Sint16)(bx+bw), (Sint16)(bx+bw-10), (Sint16)(bx+10)};
            Sint16 ty[] = {(Sint16)(by+bh-8), (Sint16)(by+bh-8), (Sint16)(by+bh), (Sint16)(by+bh)};
            filledPolygonRGBA(r, tx, ty, 4, col.r, col.g, col.b, col.a);
        }
        if (highlight)
            drawRoundedRectOutline(r, bx, by, bw, bh, rad, 255, 255, 0, 255);
    }

    for (auto& inp : b.inputs) {
        int ix = bx + (int)inp.relX, iy = by + (int)inp.relY;
        int iw = (int)inp.width, ih = (int)inp.height;
        fillRoundedRect(r, ix, iy, iw, ih, 4, 255, 255, 255, 220);
        if (inp.editing)
            drawRoundedRectOutline(r, ix, iy, iw, ih, 4, 0, 120, 255, 255);
        if (!inp.value.empty()) {
            int tw = textWidthTTF(inp.value.c_str(), gFontSmall);
            int tx = ix + (iw - tw)/2;
            int ty2 = iy + (ih - textHeightTTF(gFontSmall))/2;
            drawTextTTF(r, inp.value.c_str(), tx, ty2, {0,0,0,255}, gFontSmall);
        }
    }

    for (auto& slot : b.opSlots) {
        int sx2 = bx + (int)slot.relX, sy2 = by + (int)slot.relY;
        int sw = (int)slot.width, sh = (int)slot.height;
        int cx = sx2 + sw/2, cy = sy2 + sh/2;
        Sint16 dx[] = {(Sint16)(cx-sw/2), (Sint16)(cx-sw/2+sh/2), (Sint16)(cx+sw/2-sh/2), (Sint16)(cx+sw/2), (Sint16)(cx+sw/2-sh/2), (Sint16)(cx-sw/2+sh/2)};
        Sint16 dy[] = {(Sint16)cy, (Sint16)(cy-sh/2), (Sint16)(cy-sh/2), (Sint16)cy, (Sint16)(cy+sh/2), (Sint16)(cy+sh/2)};
        filledPolygonRGBA(r, dx, dy, 6, 255, 255, 255, 180);
    }

    string label = b.text;
    {
        size_t pos;
        int idx = 1;
        while ((pos = label.find("%" + to_string(idx))) != string::npos)
            label.replace(pos, 2, "    ");
        idx++;
    }
    int textY = by + ((int)(b.shape == C_BLOCK ? L.BLOCK_HEIGHT : b.h) - textHeightTTF(gFontSmall)) / 2;
    drawTextTTF(r, label.c_str(), bx + 8, textY, {255,255,255,255}, gFontSmall);
}

static void drawSprite(SDL_Renderer* r, Sprite& sp, int stageX, int stageY, int stageW, int stageH) {
    if (!sp.visible) return;
    int cx = stageX + stageW/2 + (int)sp.x;
    int cy = stageY + stageH/2 - (int)sp.y;
    float sc = sp.size / 100.0f;

    if (sp.uploadedTexture) {
        int dw = (int)(sp.uploadedW * sc);
        int dh = (int)(sp.uploadedH * sc);
        SDL_Rect dst = {cx - dw/2, cy - dh/2, dw, dh};
        SDL_RenderCopyEx(r, sp.uploadedTexture, nullptr, &dst, sp.direction - 90, nullptr, SDL_FLIP_NONE);
    } else {
        int sz = (int)(30 * sc);
        float rad = sp.direction * M_PI / 180.0f;
        int tx = cx + (int)(sz * cos(rad - M_PI/2));
        int ty = cy + (int)(sz * sin(rad - M_PI/2));
        int lx = cx + (int)(sz*0.6f * cos(rad + 2.5f));
        int ly = cy + (int)(sz*0.6f * sin(rad + 2.5f));
        int rx2 = cx + (int)(sz*0.6f * cos(rad - 2.5f));
        int ry2 = cy + (int)(sz*0.6f * sin(rad - 2.5f));
        filledTrigonRGBA(r, tx, ty, lx, ly, rx2, ry2, sp.color.r, sp.color.g, sp.color.b, sp.color.a);
        aatrigonRGBA(r, tx, ty, lx, ly, rx2, ry2, 255, 255, 255, 200);
    }
}

// ═══════════════════════════════════════════
//  EXECUTION ENGINE
// ═══════════════════════════════════════════

static map<string, float> gVariables;

static float evaluateReporter(Block& b, int sprIdx) {
    Sprite& sp = gSprites[sprIdx];
    if (b.text == "x position") return sp.x;
    if (b.text == "y position") return sp.y;
    if (b.text == "direction")  return sp.direction;
    if (b.text == "size")       return sp.size;
    if (b.text == "myVar") {
        if (gVariables.count("myVar")) return gVariables["myVar"];
        return 0;
    }
    if (b.text == "%1 + %2") return getInputValue(b, 0) + getInputValue(b, 1);
    if (b.text == "%1 - %2") return getInputValue(b, 0) - getInputValue(b, 1);
    if (b.text == "%1 * %2") return getInputValue(b, 0) * getInputValue(b, 1);
    if (b.text == "%1 / %2") {
        float d = getInputValue(b, 1);
        return (d != 0) ? getInputValue(b, 0) / d : 0;
    }
    if (b.text == "pick random %1 to %2") {
        int lo = (int)getInputValue(b, 0);
        int hi = (int)getInputValue(b, 1);
        if (lo > hi) swap(lo, hi);
        return (float)(lo + rand() % (hi - lo + 1));
    }
    return 0;
}

static bool evaluateBoolean(Block& b, int sprIdx) {
    if (b.text == "%1 > %2") return getInputValue(b, 0) > getInputValue(b, 1);
    if (b.text == "%1 < %2") return getInputValue(b, 0) < getInputValue(b, 1);
    if (b.text == "%1 = %2") return fabs(getInputValue(b, 0) - getInputValue(b, 1)) < 0.001f;
    return false;
}

static void executeBlock(ScriptThread& thread, Block& b, float dt) {
    int si = thread.spriteIdx;
    if (si < 0 || si >= (int)gSprites.size()) return;
    Sprite& sp = gSprites[si];
    string txt = b.text;

    if (txt == "move %1 steps") {
        float steps = getInputValue(b, 0);
        float rad = sp.direction * M_PI / 180.0f;
        sp.x += steps * cos(rad - M_PI/2);
        sp.y += steps * sin(M_PI/2 - rad);
    }
    else if (txt == "turn right %1 deg") {
        sp.direction += getInputValue(b, 0);
    }
    else if (txt == "turn left %1 deg") {
        sp.direction -= getInputValue(b, 0);
    }
    else if (txt == "go to x:%1 y:%2") {
        sp.x = getInputValue(b, 0);
        sp.y = getInputValue(b, 1);
    }
    else if (txt == "glide %1s to x:%2 y:%3") {
        float secs = getInputValue(b, 0);
        if (secs <= 0) secs = 0.001f;
        if (!thread.isWaiting) {
            thread.isWaiting = true;
            thread.waitTimer = secs;
        }
        float tx = getInputValue(b, 1);
        float ty = getInputValue(b, 2);
        float frac = dt / thread.waitTimer;
        if (frac > 1) frac = 1;
        sp.x += (tx - sp.x) * frac;
        sp.y += (ty - sp.y) * frac;
        thread.waitTimer -= dt;
        if (thread.waitTimer <= 0) {
            sp.x = tx; sp.y = ty;
            thread.isWaiting = false;
        } else {
            return;
        }
    }
    else if (txt == "point in dir %1") {
        sp.direction = getInputValue(b, 0);
    }
    else if (txt == "change x by %1") {
        sp.x += getInputValue(b, 0);
    }
    else if (txt == "set x to %1") {
        sp.x = getInputValue(b, 0);
    }
    else if (txt == "change y by %1") {
        sp.y += getInputValue(b, 0);
    }
    else if (txt == "set y to %1") {
        sp.y = getInputValue(b, 0);
    }
    else if (txt == "show") {
        sp.visible = true;
    }
    else if (txt == "hide") {
        sp.visible = false;
    }
    else if (txt == "set size to %1 %") {
        sp.size = getInputValue(b, 0);
        if (sp.size < 1) sp.size = 1;
    }
    else if (txt == "change size by %1") {
        sp.size += getInputValue(b, 0);
        if (sp.size < 1) sp.size = 1;
    }
    else if (txt == "wait %1 secs") {
        if (!thread.isWaiting) {
            thread.isWaiting = true;
            thread.waitTimer = getInputValue(b, 0);
        }
        thread.waitTimer -= dt;
        if (thread.waitTimer <= 0) {
            thread.isWaiting = false;
        } else {
            return;
        }
    }
    else if (txt == "stop all") {
        gActiveThreads.clear();
        return;
    }
    else if (txt == "set myVar to %1") {
        gVariables["myVar"] = getInputValue(b, 0);
    }
    else if (txt == "change myVar by %1") {
        gVariables["myVar"] += getInputValue(b, 0);
    }

    if (b.shape == C_BLOCK) {
        if (txt == "repeat %1") {
            int count = (int)getInputValue(b, 0);
            bool found = false;
            for (auto& ls : thread.loopStack) {
                if (ls.first == b.id) { found = true; ls.second--; break; }
            }
            if (!found) {
                thread.loopStack.push_back({b.id, count});
            }
            int remaining = 0;
            for (auto& ls : thread.loopStack)
                if (ls.first == b.id) { remaining = ls.second; break; }

            if (remaining > 0 && b.childHeadId != -1) {
                thread.currentBlockId = b.childHeadId;
                return;
            } else {
                for (auto it = thread.loopStack.begin(); it != thread.loopStack.end(); ++it) {
                    if (it->first == b.id) { thread.loopStack.erase(it); break; }
                }
            }
        }
        else if (txt == "forever") {
            if (b.childHeadId != -1) {
                thread.currentBlockId = b.childHeadId;
                return;
            }
        }
        else if (txt == "if %1 then") {
            bool cond = false;
            if (!b.opSlots.empty() && b.opSlots[0].embeddedBlockId != -1) {
                Block* eb = findBlock(b.opSlots[0].embeddedBlockId);
                if (eb) cond = evaluateBoolean(*eb, si);
            }
            if (cond && b.childHeadId != -1) {
                thread.currentBlockId = b.childHeadId;
                return;
            }
        }
    }

    if (b.nextBlockId != -1) {
        thread.currentBlockId = b.nextBlockId;
    } else {
        if (b.parentBlockId != -1) {
            Block* parent = findBlock(b.parentBlockId);
            if (parent && parent->shape == C_BLOCK && parent->childHeadId == b.id) {
            } else if (parent && parent->shape == C_BLOCK) {
                int cur = parent->childHeadId;
                bool isLastInMouth = false;
                while (cur != -1) {
                    Block* cb = findBlock(cur);
                    if (!cb) break;
                    if (cb->nextBlockId == -1 && cur == b.id) { isLastInMouth = true; break; }
                    cur = cb->nextBlockId;
                }
                if (isLastInMouth) {
                    if (parent->text == "forever") {
                        thread.currentBlockId = parent->childHeadId;
                        return;
                    }
                    if (parent->text == "repeat %1") {
                        thread.currentBlockId = parent->id;
                        return;
                    }
                    if (parent->nextBlockId != -1)
                        thread.currentBlockId = parent->nextBlockId;
                    else
                        thread.finished = true;
                    return;
                }
            }
        }
        thread.finished = true;
    }
}

static void runOneStep(float dt) {
    for (int i = (int)gActiveThreads.size() - 1; i >= 0; i--) {
        ScriptThread& t = gActiveThreads[i];
        if (t.finished) {
            gActiveThreads.erase(gActiveThreads.begin() + i);
            continue;
        }
        if (t.currentBlockId == -1) {
            gActiveThreads.erase(gActiveThreads.begin() + i);
            continue;
        }
        Block* b = findBlock(t.currentBlockId);
        if (!b) {
            gActiveThreads.erase(gActiveThreads.begin() + i);
            continue;
        }
        if (b->shape == HAT) {
            if (b->nextBlockId != -1)
                t.currentBlockId = b->nextBlockId;
            else {
                gActiveThreads.erase(gActiveThreads.begin() + i);
                continue;
            }
            b = findBlock(t.currentBlockId);
            if (!b) { gActiveThreads.erase(gActiveThreads.begin() + i); continue; }
        }
        executeBlock(t, *b, dt);
    }
}

static void startGreenFlag() {
    gActiveThreads.clear();
    for (int si = 0; si < (int)gSprites.size(); si++) {
        if (si >= (int)gSpriteBlockIds.size()) continue;
        for (int bid : gSpriteBlockIds[si]) {
            Block* b = findBlock(bid);
            if (b && b->shape == HAT && b->text == "when green flag clicked" && !b->inPalette) {
                gActiveThreads.push_back(ScriptThread(b->id, si));
            }
        }
    }
}

static void startKeyPressed(const char* keyName) {
    for (int si = 0; si < (int)gSprites.size(); si++) {
        if (si >= (int)gSpriteBlockIds.size()) continue;
        for (int bid : gSpriteBlockIds[si]) {
            Block* b = findBlock(bid);
            if (b && b->shape == HAT && b->text == "when space key pressed" && !b->inPalette) {
                bool already = false;
                for (auto& t : gActiveThreads)
                    if (t.currentBlockId == b->id && t.spriteIdx == si) { already = true; break; }
                if (!already)
                    gActiveThreads.push_back(ScriptThread(b->id, si));
            }
        }
    }
}

static void startSpriteClicked(int sprIdx) {
    if (sprIdx < 0 || sprIdx >= (int)gSpriteBlockIds.size()) return;
    for (int bid : gSpriteBlockIds[sprIdx]) {
        Block* b = findBlock(bid);
        if (b && b->shape == HAT && b->text == "when this sprite clicked" && !b->inPalette) {
            gActiveThreads.push_back(ScriptThread(b->id, sprIdx));
        }
    }
}

// ═══════════════════════════════════════════
//  DRAG & DROP STATE
// ═══════════════════════════════════════════
static bool gDragging = false;
static int gDragBlockId = -1;
static float gDragOffX = 0, gDragOffY = 0;
static bool gDragFromPalette = false;

static Block cloneBlockDeep(Block& src, bool asWorkspace) {
    Block nb = src;
    nb.id = gNextBlockId++;
    nb.inPalette = false;
    nb.nextBlockId = -1;
    nb.parentBlockId = -1;
    nb.childHeadId = -1;
    for (auto& inp : nb.inputs) inp.editing = false;
    return nb;
}

// ═══════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════
int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    TTF_Init();
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    SDL_Window* window = SDL_CreateWindow("Scratch Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        BASE_WIDTH, BASE_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    gRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    rnd = gRenderer;

    int winW = BASE_WIDTH, winH = BASE_HEIGHT;
    L.update(winW, winH);
    initFonts("DejaVuSans.ttf", 13);
    setFontSize(L.fontScale);

    gSprites.push_back(createDefaultSprite("Sprite1", 0, 0, {100,160,240,255}));
    gSpriteBlockIds.push_back(vector<int>());

    buildPaletteBlocks();

    if (!gVariables.count("myVar")) gVariables["myVar"] = 0;

    bool running = true;
    Uint32 lastTick = SDL_GetTicks();
    float paletteScrollY = 0;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTick) / 1000.0f;
        lastTick = now;
        gTimer += dt;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;

            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                winW = e.window.data1; winH = e.window.data2;
                L.update(winW, winH);
                setFontSize(L.fontScale);
                buildPaletteBlocks();
            }

            if (e.type == SDL_MOUSEWHEEL) {
                int mx, my; SDL_GetMouseState(&mx, &my);
                if (mx < L.PALETTE_WIDTH && my > L.TOOLBAR_HEIGHT) {
                    paletteScrollY += e.wheel.y * 20;
                    if (paletteScrollY > 0) paletteScrollY = 0;
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;

                int stageX = winW - L.STAGE_WIDTH;
                int stageY = L.TOOLBAR_HEIGHT;
                int flagBtnX = stageX + L.STAGE_WIDTH - 80;
                int flagBtnY = stageY + L.STAGE_HEIGHT + 5;
                if (mx >= flagBtnX && mx <= flagBtnX + 35 && my >= flagBtnY && my <= flagBtnY + 30) {
                    startGreenFlag();
                }

                int stopBtnX = flagBtnX + 40;
                if (mx >= stopBtnX && mx <= stopBtnX + 35 && my >= flagBtnY && my <= flagBtnY + 30) {
                    gActiveThreads.clear();
                }

                int sprListY = stageY + L.STAGE_HEIGHT + 45;
                int addBtnX = stageX + 5;
                int addBtnY = sprListY;
                int addBtnW = 60, addBtnH = 30;
                if (mx >= addBtnX && mx <= addBtnX+addBtnW && my >= addBtnY && my <= addBtnY+addBtnH) {
                    string nm = "Sprite" + to_string(gNextSpriteNum++);
                    SDL_Color cols[] = {{240,100,100,255},{100,240,100,255},{240,240,100,255},{200,100,240,255}};
                    gSprites.push_back(createDefaultSprite(nm.c_str(),
                        (float)(rand()%200-100), (float)(rand()%150-75),
                        cols[gSprites.size() % 4]));
                    gSpriteBlockIds.push_back(vector<int>());
                    activeSpriteTab = (int)gSprites.size() - 1;
                }

                int uploadBtnX = addBtnX + addBtnW + 10;
                if (mx >= uploadBtnX && mx <= uploadBtnX+80 && my >= addBtnY && my <= addBtnY+addBtnH) {
                    const char* filters[] = {"*.png","*.jpg","*.bmp"};
                    const char* file = tinyfd_openFileDialog("Upload Costume", "", 3, filters, "Images", 0);
                    if (file && activeSpriteTab >= 0 && activeSpriteTab < (int)gSprites.size()) {
                        SDL_Surface* surf = IMG_Load(file);
                        if (surf) {
                            if (gSprites[activeSpriteTab].uploadedTexture)
                                SDL_DestroyTexture(gSprites[activeSpriteTab].uploadedTexture);
                            gSprites[activeSpriteTab].uploadedTexture = SDL_CreateTextureFromSurface(gRenderer, surf);
                            gSprites[activeSpriteTab].uploadedW = surf->w;
                            gSprites[activeSpriteTab].uploadedH = surf->h;
                            SDL_FreeSurface(surf);
                        }
                    }
                }

                for (int i = 0; i < (int)gSprites.size(); i++) {
                    int tx = stageX + 5 + i * (L.SPRITE_THUMB + 10);
                    int ty = sprListY + 40;
                    if (mx >= tx && mx <= tx + L.SPRITE_THUMB && my >= ty && my <= ty + L.SPRITE_THUMB) {
                        activeSpriteTab = i;
                    }
                }

                int cx = stageX + L.STAGE_WIDTH/2;
                int cy2 = stageY + L.STAGE_HEIGHT/2;
                for (int si = 0; si < (int)gSprites.size(); si++) {
                    Sprite& sp = gSprites[si];
                    if (!sp.visible) continue;
                    int sx2 = cx + (int)sp.x;
                    int sy2 = cy2 - (int)sp.y;
                    int sz = (int)(30 * sp.size / 100.0f);
                    if (abs(mx - sx2) < sz && abs(my - sy2) < sz) {
                        startSpriteClicked(si);
                    }
                }

                if (gActiveEdit.blockId != -1) {
                    Block* eb = findBlock(gActiveEdit.blockId);
                    if (eb && gActiveEdit.inputIdx < (int)eb->inputs.size())
                        eb->inputs[gActiveEdit.inputIdx].editing = false;
                    gActiveEdit.blockId = -1;
                    gActiveEdit.inputIdx = -1;
                }

                for (int i = (int)gBlocks.size()-1; i >= 0; i--) {
                    Block& b = gBlocks[i];
                    bool inPaletteArea = (mx < L.PALETTE_WIDTH && my > L.TOOLBAR_HEIGHT);
                    if (b.inPalette && !inPaletteArea) continue;
                    if (b.inPalette && b.cat != gSelectedCat) continue;

                    float bx = b.x, by = b.y;
                    if (b.inPalette) by += paletteScrollY;

                    if (mx >= bx && mx <= bx + b.w && my >= by && my <= by + b.h) {
                        bool clickedInput = false;
                        for (int j = 0; j < (int)b.inputs.size(); j++) {
                            auto& inp = b.inputs[j];
                            int ix = (int)(bx + inp.relX);
                            int iy = (int)(by + inp.relY);
                            if (mx >= ix && mx <= ix + (int)inp.width && my >= iy && my <= iy + (int)inp.height) {
                                if (!b.inPalette) {
                                    inp.editing = true;
                                    gActiveEdit.blockId = b.id;
                                    gActiveEdit.inputIdx = j;
                                    clickedInput = true;
                                }
                                break;
                            }
                        }
                        if (clickedInput) break;

                        if (b.inPalette) {
                            Block nb = cloneBlockDeep(b, true);
                            nb.x = mx - nb.w/2;
                            nb.y = my - nb.h/2;
                            gBlocks.push_back(nb);
                            gDragBlockId = nb.id;
                            gDragging = true;
                            gDragFromPalette = true;
                            gDragOffX = nb.w/2;
                            gDragOffY = nb.h/2;
                        } else {
                            detachBlock(b.id);
                            gDragBlockId = b.id;
                            gDragging = true;
                            gDragFromPalette = false;
                            gDragOffX = mx - b.x;
                            gDragOffY = my - b.y;
                        }
                        break;
                    }
                }

                int catPanelX = 0;
                int catPanelY = L.TOOLBAR_HEIGHT;
                for (int c = 0; c < NUM_CATEGORIES; c++) {
                    int btnY = catPanelY + c * L.CAT_BTN_HEIGHT;
                    if (mx >= catPanelX && mx <= catPanelX + L.CAT_PANEL_WIDTH &&
                        my >= btnY && my <= btnY + L.CAT_BTN_HEIGHT) {
                        gSelectedCat = (Category)c;
                    }
                }
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
                int mx = e.button.x, my = e.button.y;
                for (int i = (int)gBlocks.size()-1; i >= 0; i--) {
                    Block& b = gBlocks[i];
                    if (b.inPalette) continue;
                    if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h) {
                        detachBlock(b.id);
                        if (activeSpriteTab >= 0 && activeSpriteTab < (int)gSpriteBlockIds.size()) {
                            auto& ids = gSpriteBlockIds[activeSpriteTab];
                            ids.erase(remove(ids.begin(), ids.end(), b.id), ids.end());
                        }
                        gBlocks.erase(gBlocks.begin() + i);
                        break;
                    }
                }
            }

            if (e.type == SDL_MOUSEMOTION && gDragging) {
                Block* b = findBlock(gDragBlockId);
                if (b) {
                    b->x = e.motion.x - gDragOffX;
                    b->y = e.motion.y - gDragOffY;
                }
            }

            if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT && gDragging) {
                gDragging = false;
                Block* moving = findBlock(gDragBlockId);
                if (moving) {
                    if (moving->x < L.PALETTE_WIDTH) {
                        if (activeSpriteTab >= 0 && activeSpriteTab < (int)gSpriteBlockIds.size()) {
                            auto& ids = gSpriteBlockIds[activeSpriteTab];
                            ids.erase(remove(ids.begin(), ids.end(), moving->id), ids.end());
                        }
                        int idx = findBlockIndex(gDragBlockId);
                        if (idx >= 0) gBlocks.erase(gBlocks.begin() + idx);
                    } else {
                        bool snapped = false;
                        for (auto& target : gBlocks) {
                            if (target.id == moving->id || target.inPalette) continue;
                            if (target.shape == C_BLOCK) {
                                float mouthX = target.x + 20;
                                float mouthY = target.y + L.BLOCK_HEIGHT;
                                if (target.childHeadId == -1 &&
                                    fabs(moving->x - mouthX) < L.SNAP_DISTANCE &&
                                    fabs(moving->y - mouthY) < L.SNAP_DISTANCE) {
                                    attachAsChild(target.id, moving->id);
                                    int top = findTopOfChain(target.id);
                                    Block* tb = findBlock(top);
                                    if (tb) repositionChain(top, tb->x, tb->y);
                                    snapped = true;
                                    break;
                                }
                            }
                            float snapY = target.y + target.h - L.SNAP_VERT_OVERLAP;
                            if (fabs(moving->x - target.x) < L.SNAP_DISTANCE &&
                                fabs(moving->y - snapY) < L.SNAP_DISTANCE) {
                                if (target.shape != REPORTER && target.shape != BOOLEAN) {
                                    attachAfter(target.id, moving->id);
                                    int top = findTopOfChain(target.id);
                                    Block* tb = findBlock(top);
                                    if (tb) repositionChain(top, tb->x, tb->y);
                                    snapped = true;
                                    break;
                                }
                            }
                        }
                        if (activeSpriteTab >= 0 && activeSpriteTab < (int)gSpriteBlockIds.size()) {
                            auto& ids = gSpriteBlockIds[activeSpriteTab];
                            if (find(ids.begin(), ids.end(), moving->id) == ids.end())
                                ids.push_back(moving->id);
                        }
                    }
                }
                gDragBlockId = -1;
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_SPACE)
                    startKeyPressed("space");

                if (gActiveEdit.blockId != -1) {
                    Block* eb = findBlock(gActiveEdit.blockId);
                    if (eb && gActiveEdit.inputIdx < (int)eb->inputs.size()) {
                        auto& inp = eb->inputs[gActiveEdit.inputIdx];
                        if (e.key.keysym.sym == SDLK_BACKSPACE && !inp.value.empty())
                            inp.value.pop_back();
                        else if (e.key.keysym.sym == SDLK_RETURN) {
                            inp.editing = false;
                            gActiveEdit.blockId = -1;
                            gActiveEdit.inputIdx = -1;
                        }
                    }
                }
            }

            if (e.type == SDL_TEXTINPUT) {
                if (gActiveEdit.blockId != -1) {
                    Block* eb = findBlock(gActiveEdit.blockId);
                    if (eb && gActiveEdit.inputIdx < (int)eb->inputs.size()) {
                        eb->inputs[gActiveEdit.inputIdx].value += e.text.text;
                    }
                }
            }
        }

        runOneStep(dt);

        SDL_SetRenderDrawColor(rnd, gBgColor.r, gBgColor.g, gBgColor.b, 255);
        SDL_RenderClear(rnd);

        {
            SDL_Rect toolbar = {0, 0, winW, L.TOOLBAR_HEIGHT};
            SDL_SetRenderDrawColor(rnd, 60, 60, 100, 255);
            SDL_RenderFillRect(rnd, &toolbar);
            drawTextTTF(rnd, "Scratch Simulator", 15, (L.TOOLBAR_HEIGHT - textHeightTTF(gFontLarge))/2,
                        {255,255,255,255}, gFontLarge);
        }

        {
            int catPanelX = 0;
            int catPanelY = L.TOOLBAR_HEIGHT;
            SDL_Rect catBg = {catPanelX, catPanelY, L.CAT_PANEL_WIDTH, winH - catPanelY};
            SDL_SetRenderDrawColor(rnd, 45, 45, 75, 255);
            SDL_RenderFillRect(rnd, &catBg);
            for (int c = 0; c < NUM_CATEGORIES; c++) {
                int btnY = catPanelY + c * L.CAT_BTN_HEIGHT;
                SDL_Color cc = catColor((Category)c);
                if ((Category)c == gSelectedCat) {
                    fillRoundedRect(rnd, catPanelX+2, btnY+2, L.CAT_PANEL_WIDTH-4, L.CAT_BTN_HEIGHT-4,
                                    6, cc.r, cc.g, cc.b, 255);
                } else {
                    fillRoundedRect(rnd, catPanelX+4, btnY+4, L.CAT_PANEL_WIDTH-8, L.CAT_BTN_HEIGHT-8,
                                    4, cc.r, cc.g, cc.b, 180);
                }
                drawTextTTF(rnd, catName((Category)c),
                            catPanelX + 10, btnY + (L.CAT_BTN_HEIGHT - textHeightTTF(gFontSmall))/2,
                            {255,255,255,255}, gFontSmall);
            }
        }

        {
            int palX = L.CAT_PANEL_WIDTH;
            int palY = L.TOOLBAR_HEIGHT;
            int palW = L.PALETTE_WIDTH - L.CAT_PANEL_WIDTH;
            int palH = winH - palY;
            SDL_Rect palBg = {palX, palY, palW, palH};
            SDL_SetRenderDrawColor(rnd, 235, 235, 240, 255);
            SDL_RenderFillRect(rnd, &palBg);

            SDL_Rect clip = {palX, palY, palW, palH};
            SDL_RenderSetClipRect(rnd, &clip);
            for (auto& b : gBlocks) {
                if (!b.inPalette || b.cat != gSelectedCat) continue;
                float drawX = b.x + palX - 10;
                float drawY = b.y + palY + paletteScrollY;
                float ox = b.x, oy = b.y;
                b.x = drawX; b.y = drawY;
                drawBlock(rnd, b);
                b.x = ox; b.y = oy;
            }
            SDL_RenderSetClipRect(rnd, nullptr);
        }

        {
            int wsX = L.PALETTE_WIDTH;
            int wsY = L.TOOLBAR_HEIGHT;
            int wsW = winW - L.PALETTE_WIDTH - L.STAGE_WIDTH;
            int wsH = winH - wsY;
            SDL_Rect wsBg = {wsX, wsY, wsW, wsH};
            SDL_SetRenderDrawColor(rnd, 250, 250, 252, 255);
            SDL_RenderFillRect(rnd, &wsBg);
            drawTextTTF(rnd, "Workspace", wsX + 10, wsY + 5, {180,180,190,255}, gFontSmall);

            if (activeSpriteTab >= 0 && activeSpriteTab < (int)gSprites.size()) {
                string info = gSprites[activeSpriteTab].name + " blocks:";
                drawTextTTF(rnd, info.c_str(), wsX + 10, wsY + 22, {120,120,140,255}, gFontSmall);
            }

            for (auto& b : gBlocks) {
                if (b.inPalette) continue;
                if (activeSpriteTab >= 0 && activeSpriteTab < (int)gSpriteBlockIds.size()) {
                    auto& ids = gSpriteBlockIds[activeSpriteTab];
                    if (find(ids.begin(), ids.end(), b.id) == ids.end()) continue;
                }
                bool hl = (gDragging && b.id == gDragBlockId);
                drawBlock(rnd, b, hl);
            }
        }

        {
            int stageX = winW - L.STAGE_WIDTH;
            int stageY = L.TOOLBAR_HEIGHT;
            SDL_Rect stageBg = {stageX, stageY, L.STAGE_WIDTH, L.STAGE_HEIGHT};
            SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
            SDL_RenderFillRect(rnd, &stageBg);
            drawRoundedRectOutline(rnd, stageX, stageY, L.STAGE_WIDTH, L.STAGE_HEIGHT, 4, 200, 200, 210, 255);

            for (auto& sp : gSprites) {
                drawSprite(rnd, sp, stageX, stageY, L.STAGE_WIDTH, L.STAGE_HEIGHT);
            }

            int flagX = stageX + L.STAGE_WIDTH - 80;
            int flagY = stageY + L.STAGE_HEIGHT + 5;
            fillRoundedRect(rnd, flagX, flagY, 35, 30, 5, 50, 180, 50, 255);
            drawTextTTF(rnd, "▶", flagX + 8, flagY + 5, {255,255,255,255}, gFontSmall);
            fillRoundedRect(rnd, flagX + 40, flagY, 35, 30, 5, 200, 50, 50, 255);
            drawTextTTF(rnd, "■", flagX + 48, flagY + 5, {255,255,255,255}, gFontSmall);

            if (!gVariables.empty()) {
                int vy = stageY + L.STAGE_HEIGHT - 25;
                for (auto& kv : gVariables) {
                    string vs = kv.first + ": " + floatToString(kv.second);
                    fillRoundedRect(rnd, stageX + 5, vy, 120, 20, 4, 230, 120, 0, 200);
                    drawTextTTF(rnd, vs.c_str(), stageX + 10, vy + 2, {255,255,255,255}, gFontSmall);
                    vy -= 25;
                }
            }

            int sprListY = stageY + L.STAGE_HEIGHT + 45;

            fillRoundedRect(rnd, stageX + 5, sprListY, 60, 30, 5, 80, 80, 180, 255);
            drawTextTTF(rnd, "+ Sprite", stageX + 10, sprListY + 6, {255,255,255,255}, gFontSmall);

            fillRoundedRect(rnd, stageX + 75, sprListY, 80, 30, 5, 80, 160, 80, 255);
            drawTextTTF(rnd, "Upload Img", stageX + 80, sprListY + 6, {255,255,255,255}, gFontSmall);

            for (int i = 0; i < (int)gSprites.size(); i++) {
                int tx = stageX + 5 + i * (L.SPRITE_THUMB + 10);
                int ty = sprListY + 40;
                SDL_Color sc = gSprites[i].color;
                if (i == activeSpriteTab) {
                    fillRoundedRect(rnd, tx-2, ty-2, L.SPRITE_THUMB+4, L.SPRITE_THUMB+4, 6, 60, 60, 200, 255);
                }
                fillRoundedRect(rnd, tx, ty, L.SPRITE_THUMB, L.SPRITE_THUMB, 5, sc.r, sc.g, sc.b, 200);
                drawTextTTF(rnd, gSprites[i].name.c_str(), tx+3, ty + L.SPRITE_THUMB/2 - 6,
                            {255,255,255,255}, gFontSmall);
            }
        }

        SDL_RenderPresent(rnd);
        SDL_Delay(16);
    }

    for (auto& sp : gSprites) {
        if (sp.uploadedTexture) SDL_DestroyTexture(sp.uploadedTexture);
    }
    closeFonts();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
