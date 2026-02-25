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
        float fontFactor = max(0.85f, s);
        BLOCK_WIDTH      = BASE_BLOCK_WIDTH  * fontFactor;
        BLOCK_HEIGHT     = BASE_BLOCK_HEIGHT * s;
        CBLOCK_MIN_H     = BASE_CBLOCK_MIN_H * s;
        CBLOCK_MOUTH_H   = BASE_CBLOCK_MOUTH_H * s;
        CBLOCK_BAR_H     = BASE_CBLOCK_BAR_H * s;
        BLOCK_CORNER_R   = BASE_BLOCK_CORNER_R * s;
        SNAP_DISTANCE    = BASE_SNAP_DISTANCE * s;
        SNAP_VERT_OVERLAP= BASE_SNAP_VERT_OVERLAP * s;
        fontScale        = max(10, (int)(13 * s));
    }
};
static LayoutScale L;

static void initFonts(const char* path, int baseSize) {
    gFontPath = path;
    gFontSizeNormal = baseSize;
    if (TTF_Init() == -1) return;
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

static void drawTextTTF(SDL_Renderer* r, int x, int y, const char* text,
                         Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca,
                         TTF_Font* font = nullptr, int maxWidth = 0)
{
    TTF_Font* f = font ? font : gFontNormal;
    if (!f || !text || !text[0]) return;
    SDL_Color col = {cr, cg, cb, ca};
    SDL_Surface* surf = nullptr;
    if (maxWidth > 0)
        surf = TTF_RenderUTF8_Blended_Wrapped(f, text, col, maxWidth);
    else
        surf = TTF_RenderUTF8_Blended(f, text, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_FreeSurface(surf);
    if (tex) {
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
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
                             Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    roundedBoxRGBA(r, x, y, x+w, y+h, rad, cr, cg, cb, ca);
    if (rad > 0) {
        aacircleRGBA(r, x+rad, y+rad, rad, cr, cg, cb, ca);
        aacircleRGBA(r, x+w-rad, y+rad, rad, cr, cg, cb, ca);
        aacircleRGBA(r, x+rad, y+h-rad, rad, cr, cg, cb, ca);
        aacircleRGBA(r, x+w-rad, y+h-rad, rad, cr, cg, cb, ca);
    }
}

static void fillEllipse(SDL_Renderer* r, int cx, int cy, int rx, int ry,
                          Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    filledEllipseRGBA(r, cx, cy, rx, ry, cr, cg, cb, ca);
    aaellipseRGBA(r, cx, cy, rx, ry, cr, cg, cb, ca);
}

static void drawRoundedRectOutline(SDL_Renderer* r, int x, int y, int w, int h, int rad,
                                     Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    roundedRectangleRGBA(r, x, y, x+w, y+h, rad, cr, cg, cb, ca);
}

enum Category { MOTION, LOOKS, EVENTS, CONTROL, OPERATORS, VARIABLES };
static const int NUM_CATEGORIES = 6;

static SDL_Color catColor(Category c) {
    switch(c){
        case Category::MOTION:    return {100,160,240,255};
        case Category::LOOKS:     return {180,100,220,255};
        case Category::EVENTS:    return {230,180,0,255};
        case Category::CONTROL:   return {230,160,0,255};
        case Category::OPERATORS: return {80,200,80,255};
        case Category::VARIABLES: return {230,120,0,255};
    }
    return {128,128,128,255};
}

static const char* catName(Category c) {
    switch(c){
        case Category::MOTION:    return "Motion";
        case Category::LOOKS:     return "Looks";
        case Category::EVENTS:    return "Events";
        case Category::CONTROL:   return "Control";
        case Category::OPERATORS: return "Operators";
        case Category::VARIABLES: return "Variables";
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

static const SDL_Color BG_COLORS[] = {
    {255,255,255,255},{230,240,255,255},{240,255,240,255},
    {255,240,230,255},{245,245,245,255}
};

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
                        float x, float y, float w, float h, bool inPalette)
{
    Block b;
    b.id = id;
    b.cat = cat;
    b.shape = shape;
    b.text = text;
    b.x = x; b.y = y;
    b.w = w; b.h = h;
    b.inPalette = inPalette;
    b.nextBlockId = -1;
    b.parentBlockId = -1;
    b.childHeadId = -1;
    return b;
}

static InputField makeInput(const char* def, float rx, float ry, float w, float h) {
    InputField f;
    f.value = def;
    f.defaultVal = def;
    f.relX = rx; f.relY = ry;
    f.width = w; f.height = h;
    f.editing = false;
    return f;
}

static OperatorSlot makeOpSlot(float rx, float ry, float w, float h) {
    OperatorSlot s;
    s.relX = rx; s.relY = ry;
    s.width = w; s.height = h;
    s.embeddedBlockId = -1;
    return s;
}

static vector<Block> gBlocks;
static vector<Sprite> gSprites;
static vector<vector<int>> gSpriteBlockIds;
static Category gSelectedCat = MOTION;

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
            Block b = makeBlock(id++, LOOKS, COMMAND, "show", px, y, bw*0.5f, bh, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, LOOKS, COMMAND, "hide", px, y, bw*0.5f, bh, true);
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
            Block b = makeBlock(id++, LOOKS, REPORTER, "size", px, y, bw*0.5f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, EVENTS, HAT, "when green flag clicked", px, y, bw*1.1f, bh*1.1f, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, EVENTS, HAT, "when space key pressed", px, y, bw*1.1f, bh*1.1f, true);
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, EVENTS, HAT, "when this sprite clicked", px, y, bw*1.1f, bh*1.1f, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, CONTROL, COMMAND, "wait %1 secs", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("1", bw*0.32f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, CONTROL, C_BLOCK, "repeat %1", px, y, bw, L.CBLOCK_MIN_H, true);
            b.inputs.push_back(makeInput("10", bw*0.42f, bh*0.15f, bw*0.20f, bh*0.7f));
            gBlocks.push_back(b); y += L.CBLOCK_MIN_H + 6;
        }
        {
            Block b = makeBlock(id++, CONTROL, C_BLOCK, "forever", px, y, bw, L.CBLOCK_MIN_H, true);
            gBlocks.push_back(b); y += L.CBLOCK_MIN_H + 6;
        }
        {
            Block b = makeBlock(id++, CONTROL, C_BLOCK, "if %1 then", px, y, bw, L.CBLOCK_MIN_H, true);
            b.opSlots.push_back(makeOpSlot(bw*0.18f, bh*0.15f, bw*0.40f, bh*0.7f));
            gBlocks.push_back(b); y += L.CBLOCK_MIN_H + 6;
        }
        {
            Block b = makeBlock(id++, CONTROL, CAP, "stop all", px, y, bw*0.6f, bh, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 + %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 - %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 * %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "%1 / %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, REPORTER, "pick random %1 to %2", px, y, bw*1.0f, bh*0.75f, true);
            b.inputs.push_back(makeInput("1", bw*0.52f, bh*0.1f, bw*0.15f, bh*0.55f));
            b.inputs.push_back(makeInput("10", bw*0.78f, bh*0.1f, bw*0.15f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "%1 > %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "%1 < %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "%1 = %2", px, y, bw*0.7f, bh*0.75f, true);
            b.inputs.push_back(makeInput("", bw*0.05f, bh*0.1f, bw*0.18f, bh*0.55f));
            b.inputs.push_back(makeInput("", bw*0.42f, bh*0.1f, bw*0.18f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, OPERATORS, BOOLEAN, "not %1", px, y, bw*0.6f, bh*0.75f, true);
            b.opSlots.push_back(makeOpSlot(bw*0.22f, bh*0.1f, bw*0.30f, bh*0.55f));
            gBlocks.push_back(b); y += gap;
        }
    }

    {
        float y = py;
        {
            Block b = makeBlock(id++, VARIABLES, COMMAND, "set %1 to %2", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("var", bw*0.22f, bh*0.15f, bw*0.22f, bh*0.7f));
            b.inputs.push_back(makeInput("0", bw*0.58f, bh*0.15f, bw*0.22f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, VARIABLES, COMMAND, "change %1 by %2", px, y, bw, bh, true);
            b.inputs.push_back(makeInput("var", bw*0.30f, bh*0.15f, bw*0.22f, bh*0.7f));
            b.inputs.push_back(makeInput("1", bw*0.66f, bh*0.15f, bw*0.18f, bh*0.7f));
            gBlocks.push_back(b); y += gap;
        }
        {
            Block b = makeBlock(id++, VARIABLES, REPORTER, "var", px, y, bw*0.5f, bh*0.75f, true);
            gBlocks.push_back(b); y += gap;
        }
    }

    gNextBlockId = id + 500;
}

static Block* findBlock(int id) {
    for (auto& b : gBlocks) if (b.id == id) return &b;
    return nullptr;
}

static Block cloneBlock(const Block& src) {
    Block b = src;
    b.id = gNextBlockId++;
    b.inPalette = false;
    b.nextBlockId = -1;
    b.parentBlockId = -1;
    b.childHeadId = -1;
    for (auto& inp : b.inputs) inp.editing = false;
    for (auto& op : b.opSlots) op.embeddedBlockId = -1;
    return b;
}

static void detachBlock(int blockId) {
    Block* blk = findBlock(blockId);
    if (!blk) return;
    int parentId = blk->parentBlockId;
    if (parentId < 0) return;
    Block* parent = findBlock(parentId);
    if (!parent) { blk->parentBlockId = -1; return; }
    if (parent->nextBlockId == blockId) {
        parent->nextBlockId = -1;
    }
    if (parent->childHeadId == blockId) {
        parent->childHeadId = -1;
    }
    blk->parentBlockId = -1;
}

static float chainHeight(int blockId) {
    float total = 0;
    int cur = blockId;
    while (cur >= 0) {
        Block* b = findBlock(cur);
        if (!b) break;
        total += b->h;
        cur = b->nextBlockId;
    }
    return total;
}

static void recalcCBlockHeight(Block* b) {
    if (!b || b->shape != C_BLOCK) return;
    float inner = 0;
    if (b->childHeadId >= 0) {
        inner = chainHeight(b->childHeadId);
    }
    float mouth = max(L.CBLOCK_MOUTH_H, inner);
    b->h = L.CBLOCK_BAR_H + mouth + L.CBLOCK_BAR_H;
    if (b->h < L.CBLOCK_MIN_H) b->h = L.CBLOCK_MIN_H;
}

static void repositionChain(int headId) {
    int cur = headId;
    while (cur >= 0) {
        Block* b = findBlock(cur);
        if (!b) break;
        if (b->nextBlockId >= 0) {
            Block* nxt = findBlock(b->nextBlockId);
            if (nxt) {
                nxt->x = b->x;
                nxt->y = b->y + b->h - L.SNAP_VERT_OVERLAP;
            }
        }
        if (b->shape == C_BLOCK && b->childHeadId >= 0) {
            Block* child = findBlock(b->childHeadId);
            if (child) {
                child->x = b->x + 20 * L.s;
                child->y = b->y + L.CBLOCK_BAR_H;
            }
            int cc = b->childHeadId;
            while (cc >= 0) {
                Block* cb = findBlock(cc);
                if (!cb) break;
                if (cb->nextBlockId >= 0) {
                    Block* cn = findBlock(cb->nextBlockId);
                    if (cn) {
                        cn->x = cb->x;
                        cn->y = cb->y + cb->h - L.SNAP_VERT_OVERLAP;
                    }
                }
                cc = cb->nextBlockId;
            }
            recalcCBlockHeight(b);
        }
        cur = b->nextBlockId;
    }
}

static void deleteBlock(int blockId) {
    detachBlock(blockId);
    Block* blk = findBlock(blockId);
    if (blk) {
        if (blk->nextBlockId >= 0) {
            Block* nxt = findBlock(blk->nextBlockId);
            if (nxt) nxt->parentBlockId = -1;
        }
        if (blk->childHeadId >= 0) {
            Block* ch = findBlock(blk->childHeadId);
            if (ch) ch->parentBlockId = -1;
        }
        for (auto& op : blk->opSlots) {
            if (op.embeddedBlockId >= 0) {
                Block* emb = findBlock(op.embeddedBlockId);
                if (emb) emb->parentBlockId = -1;
                op.embeddedBlockId = -1;
            }
        }
    }
    gBlocks.erase(
        remove_if(gBlocks.begin(), gBlocks.end(),
                   [blockId](const Block& b){ return b.id == blockId; }),
        gBlocks.end()
    );
}

static void deleteChain(int blockId) {
    Block* blk = findBlock(blockId);
    if (!blk) return;
    if (blk->nextBlockId >= 0) deleteChain(blk->nextBlockId);
    if (blk->childHeadId >= 0) deleteChain(blk->childHeadId);
    for (auto& op : blk->opSlots) {
        if (op.embeddedBlockId >= 0) deleteChain(op.embeddedBlockId);
    }
    deleteBlock(blockId);
}

static void trySnap(int dragId) {
    Block* drag = findBlock(dragId);
    if (!drag || drag->inPalette) return;
    float snapDist = L.SNAP_DISTANCE;
    for (auto& target : gBlocks) {
        if (target.id == dragId || target.inPalette) continue;
        if (target.nextBlockId < 0 && target.shape != CAP &&
            drag->shape != HAT && drag->shape != REPORTER && drag->shape != BOOLEAN)
        {
            float tx = target.x;
            float ty = target.y + target.h - L.SNAP_VERT_OVERLAP;
            float dx = drag->x - tx;
            float dy = drag->y - ty;
            if (abs(dx) < snapDist && abs(dy) < snapDist) {
                drag->x = tx;
                drag->y = ty;
                target.nextBlockId = dragId;
                drag->parentBlockId = target.id;
                repositionChain(target.id);
                return;
            }
        }
        if (target.parentBlockId < 0 &&
            drag->shape != REPORTER && drag->shape != BOOLEAN)
        {
            float tx = target.x;
            float ty = target.y - drag->h + L.SNAP_VERT_OVERLAP;
            float dx = drag->x - tx;
            float dy = drag->y - ty;
            if (abs(dx) < snapDist && abs(dy) < snapDist) {
                drag->x = tx;
                drag->y = ty;
                int tail = dragId;
                while (true) {
                    Block* t = findBlock(tail);
                    if (!t || t->nextBlockId < 0) break;
                    tail = t->nextBlockId;
                }
                Block* tailBlk = findBlock(tail);
                if (tailBlk) {
                    tailBlk->nextBlockId = target.id;
                    target.parentBlockId = tail;
                }
                repositionChain(dragId);
                return;
            }
        }
        if (target.shape == C_BLOCK && target.childHeadId < 0) {
            float mx = target.x + 20 * L.s;
            float my = target.y + L.CBLOCK_BAR_H;
            float dx = drag->x - mx;
            float dy = drag->y - my;
            if (abs(dx) < snapDist && abs(dy) < snapDist) {
                drag->x = mx;
                drag->y = my;
                target.childHeadId = dragId;
                drag->parentBlockId = target.id;
                recalcCBlockHeight(&target);
                repositionChain(target.id);
                return;
            }
        }
        if (drag->shape == REPORTER || drag->shape == BOOLEAN) {
            for (auto& op : target.opSlots) {
                if (op.embeddedBlockId >= 0) continue;
                float sx = target.x + op.relX;
                float sy = target.y + op.relY;
                float dx = drag->x - sx;
                float dy = drag->y - sy;
                if (abs(dx) < snapDist && abs(dy) < snapDist) {
                    drag->x = sx;
                    drag->y = sy;
                    op.embeddedBlockId = dragId;
                    drag->parentBlockId = target.id;
                    return;
                }
            }
        }
    }
}

static void renderBlock(SDL_Renderer* r, Block& b) {
    SDL_Color col = catColor(b.cat);
    int bx = (int)b.x, by = (int)b.y;
    int bw = (int)b.w, bh = (int)b.h;
    int rad = (int)L.BLOCK_CORNER_R;
    bool highlighted = (b.id == gHighlightBlockId);
    if (b.shape == REPORTER) {
        fillEllipse(r, bx + bw/2, by + bh/2, bw/2, bh/2, col.r, col.g, col.b, col.a);
        if (highlighted)
            drawRoundedRectOutline(r, bx-2, by-2, bw+4, bh+4, bh/2, 255, 255, 0, 255);
    } else if (b.shape == BOOLEAN) {
        Sint16 vx[6] = {(Sint16)(bx+bh/2), (Sint16)(bx+bw-bh/2), (Sint16)(bx+bw),
                         (Sint16)(bx+bw-bh/2), (Sint16)(bx+bh/2), (Sint16)bx};
        Sint16 vy[6] = {(Sint16)by, (Sint16)by, (Sint16)(by+bh/2),
                         (Sint16)(by+bh), (Sint16)(by+bh), (Sint16)(by+bh/2)};
        filledPolygonRGBA(r, vx, vy, 6, col.r, col.g, col.b, col.a);
        aapolygonRGBA(r, vx, vy, 6, col.r*0.7f, col.g*0.7f, col.b*0.7f, 255);
        if (highlighted)
            aapolygonRGBA(r, vx, vy, 6, 255, 255, 0, 255);
    } else if (b.shape == HAT) {
        fillRoundedRect(r, bx, by, bw, bh, rad+4, col.r, col.g, col.b, col.a);
        fillRoundedRect(r, bx+10, by-8, bw-20, 16, 8, col.r, col.g, col.b, col.a);
        if (highlighted)
            drawRoundedRectOutline(r, bx-2, by-10, bw+4, bh+14, rad+4, 255, 255, 0, 255);
    } else if (b.shape == C_BLOCK) {
        float barH = L.CBLOCK_BAR_H;
        fillRoundedRect(r, bx, by, bw, (int)barH, rad, col.r, col.g, col.b, col.a);
        SDL_Rect leftBar = {bx, by + (int)barH, (int)(20*L.s), bh - (int)(2*barH)};
        SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
        SDL_RenderFillRect(r, &leftBar);
        fillRoundedRect(r, bx, by + bh - (int)barH, bw, (int)barH, rad, col.r, col.g, col.b, col.a);
        SDL_Rect mouth = {bx + (int)(20*L.s), by + (int)barH,
                          bw - (int)(20*L.s), bh - (int)(2*barH)};
        SDL_SetRenderDrawColor(r, 40, 40, 50, 200);
        SDL_RenderFillRect(r, &mouth);
        if (highlighted)
            drawRoundedRectOutline(r, bx-2, by-2, bw+4, bh+4, rad, 255, 255, 0, 255);
    } else if (b.shape == CAP) {
        fillRoundedRect(r, bx, by, bw, bh, rad, col.r, col.g, col.b, col.a);
        SDL_Rect btm = {bx, by + bh - rad, bw, rad};
        SDL_SetRenderDrawColor(r, col.r*0.8f, col.g*0.8f, col.b*0.8f, col.a);
        SDL_RenderFillRect(r, &btm);
        if (highlighted)
            drawRoundedRectOutline(r, bx-2, by-2, bw+4, bh+4, rad, 255, 255, 0, 255);
    } else {
        fillRoundedRect(r, bx, by, bw, bh, rad, col.r, col.g, col.b, col.a);
        if (highlighted)
            drawRoundedRectOutline(r, bx-2, by-2, bw+4, bh+4, rad, 255, 255, 0, 255);
    }
    {
        string txt = b.text;
        for (int i = 1; i <= 9; i++) {
            string ph = "%" + to_string(i);
            size_t pos = txt.find(ph);
            if (pos != string::npos) {
                txt.replace(pos, ph.size(), "     ");
            }
        }
        int tx = bx + 8;
        int ty = by + (bh - textHeightTTF()) / 2;
        if (b.shape == C_BLOCK) ty = by + ((int)L.CBLOCK_BAR_H - textHeightTTF()) / 2;
        drawTextTTF(r, tx, ty, txt.c_str(), 255, 255, 255, 255);
    }
    for (int i = 0; i < (int)b.inputs.size(); i++) {
        auto& inp = b.inputs[i];
        int ix = bx + (int)inp.relX;
        int iy = by + (int)inp.relY;
        int iw = (int)inp.width;
        int ih = (int)inp.height;
        fillRoundedRect(r, ix, iy, iw, ih, 4, 255, 255, 255, 230);
        if (inp.editing) {
            drawRoundedRectOutline(r, ix-1, iy-1, iw+2, ih+2, 4, 50, 150, 255, 255);
        }
        int tw = textWidthTTF(inp.value.c_str());
        int ttx = ix + (iw - tw) / 2;
        int tty = iy + (ih - textHeightTTF()) / 2;
        drawTextTTF(r, ttx, tty, inp.value.c_str(), 40, 40, 40, 255);
    }
    for (auto& op : b.opSlots) {
        if (op.embeddedBlockId < 0) {
            int ox = bx + (int)op.relX;
            int oy = by + (int)op.relY;
            int ow = (int)op.width;
            int oh = (int)op.height;
            fillRoundedRect(r, ox, oy, ow, oh, oh/2, 60, 60, 70, 200);
        }
    }
}

static void renderSpriteOnStage(SDL_Renderer* r, Sprite& sp, int stageX, int stageY,
                                  int stageW, int stageH)
{
    if (!sp.visible) return;
    float scaleF = sp.size / 100.0f;
    int baseSize = (int)(40 * L.s);
    int sw = (int)(baseSize * scaleF);
    int sh = (int)(baseSize * scaleF);
    int px = stageX + stageW/2 + (int)(sp.x * stageW / 480.0f) - sw/2;
    int py = stageY + stageH/2 - (int)(sp.y * stageH / 360.0f) - sh/2;
    if (sp.uploadedTexture) {
        float aspect = (float)sp.uploadedW / (float)sp.uploadedH;
        int dw = sw;
        int dh = (int)(sw / aspect);
        if (dh > sh) { dh = sh; dw = (int)(sh * aspect); }
        SDL_Rect dst = {px + (sw-dw)/2, py + (sh-dh)/2, dw, dh};
        SDL_RenderCopy(r, sp.uploadedTexture, nullptr, &dst);
    } else {
        fillEllipse(r, px + sw/2, py + sh/2, sw/2, sh/2,
                    sp.color.r, sp.color.g, sp.color.b, 255);
        char letter[2] = {sp.name[0], 0};
        int tw = textWidthTTF(letter, gFontLarge);
        drawTextTTF(r, px + (sw - tw)/2, py + (sh - textHeightTTF(gFontLarge))/2,
                    letter, 255, 255, 255, 255, gFontLarge);
    }
    if (sp.selected) {
        drawRoundedRectOutline(r, px-3, py-3, sw+6, sh+6, 6, 50, 150, 255, 200);
    }
}

static void uploadSpriteImage(Sprite& sp) {
    const char* filters[] = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
    const char* path = tinyfd_openFileDialog("Choose Sprite Image", "", 4, filters, "Image Files", 0);
    if (!path) return;
    SDL_Surface* surf = IMG_Load(path);
    if (!surf) return;
    if (sp.uploadedTexture) SDL_DestroyTexture(sp.uploadedTexture);
    sp.uploadedTexture = SDL_CreateTextureFromSurface(rnd, surf);
    sp.uploadedW = surf->w;
    sp.uploadedH = surf->h;
    SDL_FreeSurface(surf);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    SDL_Window* window = SDL_CreateWindow("Scratch Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        BASE_WIDTH, BASE_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    rnd = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gRenderer = rnd;
    L.update(BASE_WIDTH, BASE_HEIGHT);
    initFonts(gFontPath.c_str(), L.fontScale);
    buildPaletteBlocks();
    gSprites.push_back(createDefaultSprite("Cat", 0, 0, {100,160,240,255}));
    gSpriteBlockIds.push_back({});
    int selectedSpriteIdx = 0;
    gSprites[0].selected = true;
    int dragBlockId = -1;
    float dragOffX = 0, dragOffY = 0;
    bool draggingSpriteOnStage = false;
    int draggingSpriteIdx = -1;
    float spriteDragOffX = 0, spriteDragOffY = 0;
    float paletteScrollY = 0;
    float paletteScrollTarget = 0;
    bool showDeleteMenu = false;
    int deleteMenuBlockId = -1;
    int deleteMenuX = 0, deleteMenuY = 0;
    Uint32 lastTime = SDL_GetTicks();

    while (gIsRunning) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTime) / 1000.0f;
        lastTime = now;
        gTimer += dt;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                gIsRunning = false;
            }
            else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                L.update(e.window.data1, e.window.data2);
                setFontSize(L.fontScale);
                buildPaletteBlocks();
            }
            else if (e.type == SDL_MOUSEWHEEL) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                int palX = L.CAT_PANEL_WIDTH;
                int palW = L.PALETTE_WIDTH - L.CAT_PANEL_WIDTH;
                int palY = L.TOOLBAR_HEIGHT;
                int palH = L.winH - L.TOOLBAR_HEIGHT;
                if (mx >= palX && mx < palX + palW && my >= palY && my < palY + palH) {
                    paletteScrollTarget -= e.wheel.y * 30;
                    if (paletteScrollTarget < 0) paletteScrollTarget = 0;
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                showDeleteMenu = false;
                int catY = L.TOOLBAR_HEIGHT;
                for (int i = 0; i < NUM_CATEGORIES; i++) {
                    int btnY = catY + i * L.CAT_BTN_HEIGHT;
                    if (mx < L.CAT_PANEL_WIDTH && my >= btnY && my < btnY + L.CAT_BTN_HEIGHT) {
                        gSelectedCat = (Category)i;
                        paletteScrollY = 0;
                        paletteScrollTarget = 0;
                    }
                }
                if (my < L.TOOLBAR_HEIGHT) {
                    int addBtnX = L.winW - L.STAGE_WIDTH + 5;
                    int addBtnW = (int)(100 * L.s);
                    int addBtnH = L.TOOLBAR_HEIGHT - 8;
                    if (mx >= addBtnX && mx < addBtnX + addBtnW && my >= 4 && my < 4 + addBtnH) {
                        string nm = "Sprite" + to_string(gNextSpriteNum++);
                        SDL_Color colors[] = {{240,100,100,255},{100,220,100,255},
                                              {220,180,60,255},{180,100,220,255}};
                        SDL_Color c = colors[gSprites.size() % 4];
                        gSprites.push_back(createDefaultSprite(nm.c_str(),
                            (gSprites.size() * 40) % 200 - 100,
                            (gSprites.size() * 30) % 150 - 75, c));
                        gSpriteBlockIds.push_back({});
                        for (auto& s : gSprites) s.selected = false;
                        gSprites.back().selected = true;
                        selectedSpriteIdx = gSprites.size() - 1;
                    }
                }
                {
                    int stageX = L.winW - L.STAGE_WIDTH;
                    int thumbY = L.TOOLBAR_HEIGHT + L.STAGE_HEIGHT + 5;
                    int thumbSize = L.SPRITE_THUMB;
                    for (int i = 0; i < (int)gSprites.size(); i++) {
                        int tx = stageX + 10 + i * (thumbSize + 8);
                        if (mx >= tx && mx < tx + thumbSize && my >= thumbY && my < thumbY + thumbSize) {
                            for (auto& s : gSprites) s.selected = false;
                            gSprites[i].selected = true;
                            selectedSpriteIdx = i;
                        }
                    }
                    int uploadBtnY = thumbY + thumbSize + 8;
                    int uploadBtnW = (int)(100 * L.s);
                    int uploadBtnH = (int)(28 * L.s);
                    int uploadBtnX = stageX + 10;
                    if (mx >= uploadBtnX && mx < uploadBtnX + uploadBtnW &&
                        my >= uploadBtnY && my < uploadBtnY + uploadBtnH) {
                        if (selectedSpriteIdx >= 0 && selectedSpriteIdx < (int)gSprites.size()) {
                            uploadSpriteImage(gSprites[selectedSpriteIdx]);
                        }
                    }
                }
                {
                    int stageX = L.winW - L.STAGE_WIDTH;
                    int stageY = L.TOOLBAR_HEIGHT;
                    int stageW = L.STAGE_WIDTH;
                    int stageH = L.STAGE_HEIGHT;
                    if (mx >= stageX && mx < stageX + stageW && my >= stageY && my < stageY + stageH) {
                        for (int i = (int)gSprites.size()-1; i >= 0; i--) {
                            Sprite& sp = gSprites[i];
                            if (!sp.visible) continue;
                            float scaleF = sp.size / 100.0f;
                            int baseSize = (int)(40 * L.s);
                            int sw = (int)(baseSize * scaleF);
                            int sh = (int)(baseSize * scaleF);
                            int px = stageX + stageW/2 + (int)(sp.x * stageW / 480.0f) - sw/2;
                            int py = stageY + stageH/2 - (int)(sp.y * stageH / 360.0f) - sh/2;
                            if (mx >= px && mx < px+sw && my >= py && my < py+sh) {
                                draggingSpriteOnStage = true;
                                draggingSpriteIdx = i;
                                spriteDragOffX = sp.x - (mx - stageX - stageW/2) * 480.0f / stageW;
                                spriteDragOffY = sp.y + (my - stageY - stageH/2) * 360.0f / stageH;
                                for (auto& s : gSprites) s.selected = false;
                                sp.selected = true;
                                selectedSpriteIdx = i;
                                break;
                            }
                        }
                    }
                }
                {
                    int palX = L.CAT_PANEL_WIDTH;
                    int palW = L.PALETTE_WIDTH - L.CAT_PANEL_WIDTH;
                    int palYtop = L.TOOLBAR_HEIGHT;
                    int workX = L.PALETTE_WIDTH;
                    int workW = L.winW - L.PALETTE_WIDTH - L.STAGE_WIDTH;
                    bool clickedInput = false;
                    for (auto& b : gBlocks) {
                        if (b.inPalette) continue;
                        for (int ii = 0; ii < (int)b.inputs.size(); ii++) {
                            auto& inp = b.inputs[ii];
                            int ix = (int)(b.x + inp.relX);
                            int iy = (int)(b.y + inp.relY);
                            if (mx >= ix && mx < ix + (int)inp.width &&
                                my >= iy && my < iy + (int)inp.height)
                            {
                                if (gActiveEdit.blockId >= 0) {
                                    Block* prev = findBlock(gActiveEdit.blockId);
                                    if (prev && gActiveEdit.inputIdx < (int)prev->inputs.size())
                                        prev->inputs[gActiveEdit.inputIdx].editing = false;
                                }
                                gActiveEdit.blockId = b.id;
                                gActiveEdit.inputIdx = ii;
                                inp.editing = true;
                                SDL_StartTextInput();
                                clickedInput = true;
                                break;
                            }
                        }
                        if (clickedInput) break;
                    }
                    if (!clickedInput) {
                        if (gActiveEdit.blockId >= 0) {
                            Block* prev = findBlock(gActiveEdit.blockId);
                            if (prev && gActiveEdit.inputIdx < (int)prev->inputs.size())
                                prev->inputs[gActiveEdit.inputIdx].editing = false;
                            gActiveEdit.blockId = -1;
                            gActiveEdit.inputIdx = -1;
                            SDL_StopTextInput();
                        }
                        for (int i = (int)gBlocks.size()-1; i >= 0; i--) {
                            Block& b = gBlocks[i];
                            int bx = (int)b.x, by = (int)b.y;
                            int checkY = by;
                            if (b.inPalette) checkY = by - (int)paletteScrollY + palYtop;
                            if (b.inPalette && b.cat != gSelectedCat) continue;
                            int bw = (int)b.w, bh = (int)b.h;
                            int drawX = b.inPalette ? palX + (int)b.x : bx;
                            int drawY = b.inPalette ? checkY : by;
                            if (mx >= drawX && mx < drawX + bw && my >= drawY && my < drawY + bh) {
                                if (b.inPalette) {
                                    Block nb = cloneBlock(b);
                                    nb.x = mx - bw/2;
                                    nb.y = my - bh/2;
                                    gBlocks.push_back(nb);
                                    dragBlockId = nb.id;
                                    dragOffX = bw/2;
                                    dragOffY = bh/2;
                                    if (selectedSpriteIdx >= 0 && selectedSpriteIdx < (int)gSpriteBlockIds.size())
                                        gSpriteBlockIds[selectedSpriteIdx].push_back(nb.id);
                                } else {
                                    detachBlock(b.id);
                                    dragBlockId = b.id;
                                    dragOffX = mx - bx;
                                    dragOffY = my - by;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
                int mx = e.button.x, my = e.button.y;
                for (int i = (int)gBlocks.size()-1; i >= 0; i--) {
                    Block& b = gBlocks[i];
                    if (b.inPalette) continue;
                    if (mx >= (int)b.x && mx < (int)(b.x+b.w) &&
                        my >= (int)b.y && my < (int)(b.y+b.h))
                    {
                        showDeleteMenu = true;
                        deleteMenuBlockId = b.id;
                        deleteMenuX = mx;
                        deleteMenuY = my;
                        break;
                    }
                }
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                if (dragBlockId >= 0) {
                    Block* db = findBlock(dragBlockId);
                    if (db) {
                        if (db->x < L.PALETTE_WIDTH) {
                            for (auto& sbids : gSpriteBlockIds) {
                                sbids.erase(remove(sbids.begin(), sbids.end(), dragBlockId), sbids.end());
                            }
                            deleteChain(dragBlockId);
                        } else {
                            trySnap(dragBlockId);
                        }
                    }
                    dragBlockId = -1;
                }
                draggingSpriteOnStage = false;
                draggingSpriteIdx = -1;
            }
            else if (e.type == SDL_MOUSEMOTION) {
                int mx = e.motion.x, my = e.motion.y;
                if (dragBlockId >= 0) {
                    Block* db = findBlock(dragBlockId);
                    if (db) {
                        db->x = mx - dragOffX;
                        db->y = my - dragOffY;
                        repositionChain(dragBlockId);
                    }
                }
                if (draggingSpriteOnStage && draggingSpriteIdx >= 0) {
                    int stageX = L.winW - L.STAGE_WIDTH;
                    int stageW = L.STAGE_WIDTH;
                    int stageY = L.TOOLBAR_HEIGHT;
                    int stageH = L.STAGE_HEIGHT;
                    gSprites[draggingSpriteIdx].x = (mx - stageX - stageW/2) * 480.0f / stageW + spriteDragOffX;
                    gSprites[draggingSpriteIdx].y = -(my - stageY - stageH/2) * 360.0f / stageH + spriteDragOffY;
                }
                gHighlightBlockId = -1;
                for (int i = (int)gBlocks.size()-1; i >= 0; i--) {
                    Block& b = gBlocks[i];
                    if (b.inPalette) continue;
                    if (mx >= (int)b.x && mx < (int)(b.x+b.w) &&
                        my >= (int)b.y && my < (int)(b.y+b.h)) {
                        gHighlightBlockId = b.id;
                        break;
                    }
                }
            }
            else if (e.type == SDL_TEXTINPUT) {
                if (gActiveEdit.blockId >= 0) {
                    Block* b = findBlock(gActiveEdit.blockId);
                    if (b && gActiveEdit.inputIdx < (int)b->inputs.size()) {
                        b->inputs[gActiveEdit.inputIdx].value += e.text.text;
                    }
                }
            }
            else if (e.type == SDL_KEYDOWN) {
                if (gActiveEdit.blockId >= 0) {
                    Block* b = findBlock(gActiveEdit.blockId);
                    if (b && gActiveEdit.inputIdx < (int)b->inputs.size()) {
                        auto& val = b->inputs[gActiveEdit.inputIdx].value;
                        if (e.key.keysym.sym == SDLK_BACKSPACE && !val.empty()) {
                            val.pop_back();
                        } else if (e.key.keysym.sym == SDLK_RETURN) {
                            b->inputs[gActiveEdit.inputIdx].editing = false;
                            gActiveEdit.blockId = -1;
                            gActiveEdit.inputIdx = -1;
                            SDL_StopTextInput();
                        }
                    }
                }
                if (e.key.keysym.sym == SDLK_DELETE && gHighlightBlockId >= 0) {
                    for (auto& sbids : gSpriteBlockIds)
                        sbids.erase(remove(sbids.begin(), sbids.end(), gHighlightBlockId), sbids.end());
                    deleteChain(gHighlightBlockId);
                    gHighlightBlockId = -1;
                }
            }
            if (showDeleteMenu && e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int mx = e.button.x, my = e.button.y;
                int menuW = (int)(120 * L.s), menuH = (int)(30 * L.s);
                if (mx >= deleteMenuX && mx < deleteMenuX + menuW &&
                    my >= deleteMenuY && my < deleteMenuY + menuH)
                {
                    for (auto& sbids : gSpriteBlockIds)
                        sbids.erase(remove(sbids.begin(), sbids.end(), deleteMenuBlockId), sbids.end());
                    deleteChain(deleteMenuBlockId);
                }
                showDeleteMenu = false;
            }
        }
        paletteScrollY += (paletteScrollTarget - paletteScrollY) * min(1.0f, dt * 12.0f);
        SDL_SetRenderDrawColor(rnd, gBgColor.r, gBgColor.g, gBgColor.b, 255);
        SDL_RenderClear(rnd);
        fillRoundedRect(rnd, 0, 0, L.winW, L.TOOLBAR_HEIGHT, 0, 50, 50, 60, 255);
        drawTextTTF(rnd, 10, (L.TOOLBAR_HEIGHT - textHeightTTF(gFontLarge))/2,
                    "Scratch Clone", 255, 255, 255, 255, gFontLarge);
        {
            int addBtnX = L.winW - L.STAGE_WIDTH + 5;
            int addBtnW = (int)(100 * L.s);
            int addBtnH = L.TOOLBAR_HEIGHT - 8;
            fillRoundedRect(rnd, addBtnX, 4, addBtnW, addBtnH, 6, 80, 180, 80, 255);
            int tw = textWidthTTF("+ Sprite");
            drawTextTTF(rnd, addBtnX + (addBtnW-tw)/2, 4 + (addBtnH-textHeightTTF())/2,
                        "+ Sprite", 255, 255, 255, 255);
        }
        {
            SDL_Rect catPanel = {0, L.TOOLBAR_HEIGHT, L.CAT_PANEL_WIDTH, L.winH - L.TOOLBAR_HEIGHT};
            SDL_SetRenderDrawColor(rnd, 55, 55, 65, 255);
            SDL_RenderFillRect(rnd, &catPanel);
            for (int i = 0; i < NUM_CATEGORIES; i++) {
                Category c = (Category)i;
                SDL_Color cc = catColor(c);
                int btnY = L.TOOLBAR_HEIGHT + i * L.CAT_BTN_HEIGHT;
                bool active = (c == gSelectedCat);
                fillRoundedRect(rnd, 2, btnY+1, L.CAT_PANEL_WIDTH-4, L.CAT_BTN_HEIGHT-2, 4,
                    active ? cc.r : 70, active ? cc.g : 70, active ? cc.b : 75, 255);
                int tw = textWidthTTF(catName(c));
                drawTextTTF(rnd, (L.CAT_PANEL_WIDTH-tw)/2,
                            btnY + (L.CAT_BTN_HEIGHT-textHeightTTF())/2,
                            catName(c), 255, 255, 255, 255);
            }
        }
        {
            int palX = L.CAT_PANEL_WIDTH;
            int palW = L.PALETTE_WIDTH - L.CAT_PANEL_WIDTH;
            int palY = L.TOOLBAR_HEIGHT;
            int palH = L.winH - L.TOOLBAR_HEIGHT;
            SDL_Rect palBg = {palX, palY, palW, palH};
            SDL_SetRenderDrawColor(rnd, 42, 42, 52, 255);
            SDL_RenderFillRect(rnd, &palBg);
            SDL_Rect clipRect = {palX, palY, palW, palH};
            SDL_RenderSetClipRect(rnd, &clipRect);
            for (auto& b : gBlocks) {
                if (!b.inPalette || b.cat != gSelectedCat) continue;
                float origX = b.x, origY = b.y;
                b.x = palX + origX;
                b.y = palY + origY - paletteScrollY;
                renderBlock(rnd, b);
                b.x = origX;
                b.y = origY;
            }
            SDL_RenderSetClipRect(rnd, nullptr);
        }
        {
            int workX = L.PALETTE_WIDTH;
            int workW = L.winW - L.PALETTE_WIDTH - L.STAGE_WIDTH;
            int workY = L.TOOLBAR_HEIGHT;
            int workH = L.winH - L.TOOLBAR_HEIGHT;
            SDL_Rect workBg = {workX, workY, workW, workH};
            SDL_SetRenderDrawColor(rnd, 240, 240, 245, 255);
            SDL_RenderFillRect(rnd, &workBg);
            for (int gx = workX + 20; gx < workX + workW; gx += 30) {
                for (int gy = workY + 20; gy < workY + workH; gy += 30) {
                    SDL_SetRenderDrawColor(rnd, 210, 210, 215, 255);
                    SDL_RenderDrawPoint(rnd, gx, gy);
                }
            }
            SDL_Rect workClip = {workX, workY, workW, workH};
            SDL_RenderSetClipRect(rnd, &workClip);
            for (auto& b : gBlocks) {
                if (b.inPalette) continue;
                bool belongs = false;
                if (selectedSpriteIdx >= 0 && selectedSpriteIdx < (int)gSpriteBlockIds.size()) {
                    auto& ids = gSpriteBlockIds[selectedSpriteIdx];
                    belongs = find(ids.begin(), ids.end(), b.id) != ids.end();
                }
                if (b.id == dragBlockId) belongs = true;
                if (!belongs) continue;
                renderBlock(rnd, b);
            }
            SDL_RenderSetClipRect(rnd, nullptr);
        }
        {
            int stageX = L.winW - L.STAGE_WIDTH;
            int stageY = L.TOOLBAR_HEIGHT;
            int stageW = L.STAGE_WIDTH;
            int stageH = L.STAGE_HEIGHT;
            fillRoundedRect(rnd, stageX, stageY, stageW, stageH, 4, 255, 255, 255, 255);
            drawRoundedRectOutline(rnd, stageX, stageY, stageW, stageH, 4, 180, 180, 190, 255);
            for (auto& sp : gSprites) {
                renderSpriteOnStage(rnd, sp, stageX, stageY, stageW, stageH);
            }
        }
        {
            int stageX = L.winW - L.STAGE_WIDTH;
            int thumbY = L.TOOLBAR_HEIGHT + L.STAGE_HEIGHT + 5;
            int thumbSize = L.SPRITE_THUMB;
            SDL_Rect thumbBg = {stageX, thumbY - 2, L.STAGE_WIDTH, thumbSize + 50};
            SDL_SetRenderDrawColor(rnd, 230, 230, 235, 255);
            SDL_RenderFillRect(rnd, &thumbBg);
            drawTextTTF(rnd, stageX + 5, thumbY - (int)(2), "Sprites:", 80, 80, 90, 255, gFontSmall);
            for (int i = 0; i < (int)gSprites.size(); i++) {
                Sprite& sp = gSprites[i];
                int tx = stageX + 10 + i * (thumbSize + 8);
                int ty = thumbY + 14;
                bool sel = (i == selectedSpriteIdx);
                fillRoundedRect(rnd, tx, ty, thumbSize, thumbSize, 6,
                    sel ? 200 : 240, sel ? 220 : 240, sel ? 255 : 245, 255);
                if (sel)
                    drawRoundedRectOutline(rnd, tx-1, ty-1, thumbSize+2, thumbSize+2, 6, 50,150,255,255);
                if (sp.uploadedTexture) {
                    SDL_Rect dst = {tx+4, ty+4, thumbSize-8, thumbSize-8};
                    SDL_RenderCopy(rnd, sp.uploadedTexture, nullptr, &dst);
                } else {
                    fillEllipse(rnd, tx + thumbSize/2, ty + thumbSize/2,
                                thumbSize/3, thumbSize/3, sp.color.r, sp.color.g, sp.color.b, 255);
                    char l[2] = {sp.name[0], 0};
                    int tw = textWidthTTF(l, gFontSmall);
                    drawTextTTF(rnd, tx + (thumbSize-tw)/2, ty + thumbSize/2 - 6,
                                l, 255, 255, 255, 255, gFontSmall);
                }
                int nw = textWidthTTF(sp.name.c_str(), gFontSmall);
                drawTextTTF(rnd, tx + (thumbSize-nw)/2, ty + thumbSize + 1,
                            sp.name.c_str(), 60, 60, 70, 255, gFontSmall);
            }
            {
                int uploadBtnY = thumbY + thumbSize + 22;
                int uploadBtnW = (int)(100 * L.s);
                int uploadBtnH = (int)(28 * L.s);
                int uploadBtnX = stageX + 10;
                fillRoundedRect(rnd, uploadBtnX, uploadBtnY, uploadBtnW, uploadBtnH, 5,
                                60, 130, 200, 255);
                int tw = textWidthTTF("Upload Image");
                drawTextTTF(rnd, uploadBtnX + (uploadBtnW-tw)/2,
                            uploadBtnY + (uploadBtnH - textHeightTTF())/2,
                            "Upload Image", 255, 255, 255, 255);
            }
        }
        if (selectedSpriteIdx >= 0 && selectedSpriteIdx < (int)gSprites.size()) {
            Sprite& sp = gSprites[selectedSpriteIdx];
            int infoX = L.winW - L.STAGE_WIDTH;
            int infoY = L.TOOLBAR_HEIGHT + L.STAGE_HEIGHT + L.SPRITE_THUMB + 60;
            int infoW = L.STAGE_WIDTH;
            int infoH = L.winH - infoY;
            if (infoH > 30) {
                SDL_Rect infoBg = {infoX, infoY, infoW, infoH};
                SDL_SetRenderDrawColor(rnd, 245, 245, 250, 255);
                SDL_RenderFillRect(rnd, &infoBg);
                int ty = infoY + 5;
                char buf[128];
                snprintf(buf, sizeof(buf), "Name: %s", sp.name.c_str());
                drawTextTTF(rnd, infoX+10, ty, buf, 50, 50, 60, 255, gFontSmall); ty += 18;
                snprintf(buf, sizeof(buf), "x: %.0f  y: %.0f", sp.x, sp.y);
                drawTextTTF(rnd, infoX+10, ty, buf, 50, 50, 60, 255, gFontSmall); ty += 18;
                snprintf(buf, sizeof(buf), "Size: %.0f%%  Dir: %.0f", sp.size, sp.direction);
                drawTextTTF(rnd, infoX+10, ty, buf, 50, 50, 60, 255, gFontSmall);
            }
        }
        if (showDeleteMenu) {
            int menuW = (int)(120 * L.s);
            int menuH = (int)(30 * L.s);
            fillRoundedRect(rnd, deleteMenuX, deleteMenuY, menuW, menuH, 5, 200, 60, 60, 240);
            int tw = textWidthTTF("Delete Block");
            drawTextTTF(rnd, deleteMenuX + (menuW-tw)/2, deleteMenuY + (menuH-textHeightTTF())/2,
                        "Delete Block", 255, 255, 255, 255);
        }
        SDL_RenderPresent(rnd);
        SDL_Delay(16);
    }
    for (auto& sp : gSprites) {
        if (sp.uploadedTexture) SDL_DestroyTexture(sp.uploadedTexture);
    }
    closeFonts();
    SDL_DestroyRenderer(rnd);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
