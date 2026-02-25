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
static bool costumeEditMode = false;
static SDL_Texture* costumeCanvas = nullptr;
static int canvasW = 480, canvasH = 360;
static Uint8 penColorR = 0, penColorG = 0, penColorB = 0;
static int penSize = 3;
static bool isDrawing = false;
static int lastDrawX = -1, lastDrawY = -1;
static int selectedCostumeSpriteIdx = -1;
static TTF_Font* gFontSmall  = nullptr;
static TTF_Font* gFontNormal = nullptr;
static TTF_Font* gFontLarge  = nullptr;
static int gFontSizeNormal = 13;
static string gFontPath = "DejaVuSans.ttf";
static SDL_Texture* gBackdropTexture = nullptr;
static string gCurrentBackdropName = "default";
static bool gBackdropEditMode = false;
static bool gBackdropPanelOpen = false;
static int  gBackdropPanelTab  = 0;

static const char* gBackdropLibrary[] = {"default", "ocean", "sunset"};
static int gBackdropLibraryCount = 3;
struct BackdropItem {
    const char* name;
    const char* path;
};
static BackdropItem gBackdropItems[] = {
    {"default",  "backdrops/default.png"},
    {"ocean",    "backdrops/ocean.png"},
    {"sunset",   "backdrops/sunset.png"},
};




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
        float fontFactor = 1.0f + (gFontSizeNormal - 13) * 0.04f;
        BLOCK_WIDTH = BASE_BLOCK_WIDTH * s * fontFactor;
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




static bool loadBackdrop(SDL_Renderer* rnd, const char* path) {
    SDL_Surface* surf = IMG_Load(path);
    if (!surf) return false;
    
    int winW = L.winW, winH = L.winH - L.TOOLBAR_HEIGHT;
    float scaleX = (float)winW / surf->w;
    float scaleY = (float)winH / surf->h;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;
    int dstW = (int)(surf->w * scale);
    int dstH = (int)(surf->h * scale);
    SDL_Surface* scaled = SDL_CreateRGBSurface(0, dstW, dstH, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_BlitScaled(surf, nullptr, scaled, nullptr);
    if (gBackdropTexture) SDL_DestroyTexture(gBackdropTexture);
    gBackdropTexture = SDL_CreateTextureFromSurface(rnd, scaled);
    SDL_FreeSurface(scaled);
    SDL_FreeSurface(surf);
    return gBackdropTexture != nullptr;
}

static bool initFonts(const char* fontPath) {
    if (TTF_Init() < 0) return false;
    gFontPath = fontPath;
    gFontSmall  = TTF_OpenFont(fontPath, 10);
    gFontNormal = TTF_OpenFont(fontPath, gFontSizeNormal);
    gFontLarge  = TTF_OpenFont(fontPath, 18);
    if (!gFontNormal) { fprintf(stderr, "TTF Error: %s\n", TTF_GetError()); return false; }
    return true;
}

static void closeFonts() {
    if (gFontSmall)  { TTF_CloseFont(gFontSmall);  gFontSmall  = nullptr; }
    if (gFontNormal) { TTF_CloseFont(gFontNormal);  gFontNormal = nullptr; }
    if (gFontLarge)  { TTF_CloseFont(gFontLarge);   gFontLarge  = nullptr; }
    TTF_Quit();
}

static void setFontSize(int size) {
    if (size < 8) size = 8;
    if (size > 32) size = 32;
    gFontSizeNormal = size;
    if (gFontNormal) TTF_CloseFont(gFontNormal);
    gFontNormal = TTF_OpenFont(gFontPath.c_str(), gFontSizeNormal);
}

static void drawTextTTF(SDL_Renderer* rnd, int x, int y, const char* text,
                        Uint8 r, Uint8 g, Uint8 b, Uint8 a,
                        TTF_Font* font = nullptr, int maxWidth = 0)
{
    if (!text || text[0] == '\0') return;
    if (!font) font = gFontNormal;
    if (!font) return;
    SDL_Color col = {r, g, b, a};
    SDL_Surface* surf = (maxWidth > 0)
        ? TTF_RenderUTF8_Blended_Wrapped(font, text, col, (Uint32)maxWidth)
        : TTF_RenderUTF8_Blended(font, text, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(rnd, surf);
    if (tex) {
        SDL_SetTextureAlphaMod(tex, a);
        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(rnd, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

static int textWidthTTF(const char* text, TTF_Font* font = nullptr) {
    if (!text || text[0] == '\0') return 0;
    if (!font) font = gFontNormal;
    if (!font) return 0;
    int w = 0, h = 0;
    TTF_SizeUTF8(font, text, &w, &h);
    return w;
}

static int textHeightTTF(TTF_Font* font = nullptr) {
    if (!font) font = gFontNormal;
    if (!font) return 14;
    return TTF_FontHeight(font);
}




static void fillRoundedRect(SDL_Renderer* rnd, int x, int y, int w, int h,
                            int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    if (radius < 1) radius = 1;
    if (radius > w/2) radius = w/2;
    if (radius > h/2) radius = h/2;

    roundedBoxRGBA(rnd, (Sint16)x, (Sint16)y,
                   (Sint16)(x + w), (Sint16)(y + h),
                   (Sint16)radius, r, g, b, a);

    aacircleRGBA(rnd, (Sint16)(x + radius),     (Sint16)(y + radius),     (Sint16)radius, r, g, b, a);
    aacircleRGBA(rnd, (Sint16)(x + w - radius), (Sint16)(y + radius),     (Sint16)radius, r, g, b, a);
    aacircleRGBA(rnd, (Sint16)(x + radius),     (Sint16)(y + h - radius), (Sint16)radius, r, g, b, a);
    aacircleRGBA(rnd, (Sint16)(x + w - radius), (Sint16)(y + h - radius), (Sint16)radius, r, g, b, a);
}

static void fillEllipse(SDL_Renderer* rnd, int cx, int cy, int rx, int ry,
                        Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    filledEllipseRGBA(rnd, (Sint16)cx, (Sint16)cy, (Sint16)rx, (Sint16)ry, r, g, b, a);
    aaellipseRGBA(rnd, (Sint16)cx, (Sint16)cy, (Sint16)rx, (Sint16)ry, r, g, b, a);
}

static void drawRoundedRectOutline(SDL_Renderer* rnd, int x, int y, int w, int h,
                                   int radius, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    roundedRectangleRGBA(rnd, (Sint16)x, (Sint16)y, (Sint16)(x + w), (Sint16)(y + h),
                         (Sint16)radius, r, g, b, a);
}
static void renderBackdropPanel(SDL_Renderer* rnd) {
    if (!gBackdropPanelOpen) return;

    
    int panelW = (int)(320 * L.s);
    int panelH = (int)(420 * L.s);
    int panelX = (L.winW - panelW) / 2;
    int panelY = L.TOOLBAR_HEIGHT + (int)(20 * L.s);

    
    fillRoundedRect(rnd, panelX, panelY, panelW, panelH, 10, 240, 240, 245, 255);
    drawRoundedRectOutline(rnd, panelX, panelY, panelW, panelH, 10, 180, 180, 200, 255);

    
    drawTextTTF(rnd, panelX + 10, panelY + 8, "Background Settings", 50, 50, 80, 255, gFontLarge);

    
    string curName = "Current: " + gCurrentBackdropName;
    drawTextTTF(rnd, panelX + 10, panelY + 32, curName.c_str(), 100, 100, 130, 255);

    
    int closeBtnSz = (int)(24 * L.s);
    int closeBtnX = panelX + panelW - closeBtnSz - 8;
    int closeBtnY = panelY + 8;
    fillRoundedRect(rnd, closeBtnX, closeBtnY, closeBtnSz, closeBtnSz, 5, 220, 60, 60, 255);
    drawTextTTF(rnd, closeBtnX + 6, closeBtnY + 2, "X", 255, 255, 255, 255);

    
    const char* tabs[] = {"Library", "Upload", "Editor"};
    int tabW = panelW / 3;
    int tabH = (int)(32 * L.s);
    int tabY = panelY + (int)(55 * L.s);
    for (int i = 0; i < 3; i++) {
        int tabX = panelX + i * tabW;
        bool active = (gBackdropPanelTab == i);
        fillRoundedRect(rnd, tabX, tabY, tabW, tabH, 5,
            active ? 100 : 200,
            active ? 150 : 200,
            active ? 220 : 210,
            255);
        int tw = textWidthTTF(tabs[i]);
        drawTextTTF(rnd, tabX + (tabW - tw) / 2, tabY + (tabH - textHeightTTF()) / 2,
            tabs[i],
            active ? 255 : 80,
            active ? 255 : 80,
            active ? 255 : 80,
            255);
    }

    
    int contentY = tabY + tabH + (int)(10 * L.s);
    int contentX = panelX + (int)(10 * L.s);
    int contentW = panelW - (int)(20 * L.s);

if (gBackdropPanelTab == 0) {
    
    drawTextTTF(rnd, contentX, contentY, "Default Backgrounds:", 60, 60, 80, 255);
    int thumbW = (int)(60 * L.s);
    int thumbH = (int)(45 * L.s);
    int itemH = thumbH + (int)(10 * L.s);
    for (int i = 0; i < gBackdropLibraryCount; i++) {
        int itemY = contentY + (int)(20 * L.s) + i * (itemH + 8);
        bool isSelected = (gCurrentBackdropName == gBackdropItems[i].name);

        
        fillRoundedRect(rnd, contentX, itemY, contentW, itemH, 6,
            isSelected ? 80 : 230,
            isSelected ? 140 : 230,
            isSelected ? 200 : 235,
            255);

        
        SDL_Surface* prevSurf = IMG_Load(gBackdropItems[i].path);
        if (prevSurf) {
            SDL_Texture* prevTex = SDL_CreateTextureFromSurface(rnd, prevSurf);
            SDL_FreeSurface(prevSurf);
            if (prevTex) {
                SDL_Rect thumbRect = {
                    contentX + 4,
                    itemY + (itemH - thumbH) / 2,
                    thumbW,
                    thumbH
                };
                SDL_RenderCopy(rnd, prevTex, nullptr, &thumbRect);
                SDL_DestroyTexture(prevTex);
            }
        } else {
            
            fillRoundedRect(rnd, contentX + 4, itemY + (itemH - thumbH) / 2,
                thumbW, thumbH, 4, 180, 180, 180, 255);
            drawTextTTF(rnd, contentX + 8, itemY + itemH/2 - 6, "?", 100, 100, 100, 255);
        }

        
        drawTextTTF(rnd, contentX + thumbW + 12, itemY + (itemH - textHeightTTF()) / 2,
            gBackdropItems[i].name,
            isSelected ? 255 : 50,
            isSelected ? 255 : 50,
            isSelected ? 255 : 60,
            255);

        
        if (isSelected) {
            drawTextTTF(rnd, contentX + contentW - 30, itemY + (itemH - textHeightTTF()) / 2,
                "✓", 255, 255, 255, 255);
        }
    }
    } else if (gBackdropPanelTab == 1) {
        
        drawTextTTF(rnd, contentX, contentY, "Upload from system:", 60, 60, 80, 255);
        int btnW = (int)(140 * L.s), btnH = (int)(40 * L.s);
        int btnX = panelX + (panelW - btnW) / 2;
        int btnY = contentY + (int)(30 * L.s);
        fillRoundedRect(rnd, btnX, btnY, btnW, btnH, 8, 50, 130, 200, 255);
        int tw = textWidthTTF("Choose Image...");
        drawTextTTF(rnd, btnX + (btnW - tw) / 2, btnY + (btnH - textHeightTTF()) / 2,
            "Choose Image...", 255, 255, 255, 255);
    } else if (gBackdropPanelTab == 2) {
        
        drawTextTTF(rnd, contentX, contentY, "Built-in editor:", 60, 60, 80, 255);
        int btnW = (int)(140 * L.s), btnH = (int)(40 * L.s);
        int btnX = panelX + (panelW - btnW) / 2;
        int btnY = contentY + (int)(30 * L.s);
        fillRoundedRect(rnd, btnX, btnY, btnW, btnH, 8, 130, 50, 170, 255);
        int tw = textWidthTTF("Open Editor");
        drawTextTTF(rnd, btnX + (btnW - tw) / 2, btnY + (btnH - textHeightTTF()) / 2,
            "Open Editor", 255, 255, 255, 255);
    }
}





enum Category { MOTION, LOOKS, SOUND, EVENTS, CONTROL, SENSING, OPERATORS, VARIABLES };
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



struct ScriptThread {
    int currentBlockId;      
    int spriteIdx;           
    float waitTimer;         
    bool isWaiting;          
    int repeatCounter;       
    vector<pair<int,int>> loopStack;  

    ScriptThread(int blockId, int sprite)
        : currentBlockId(blockId), spriteIdx(sprite),
          waitTimer(0), isWaiting(false), repeatCounter(0) {}
};

static vector<ScriptThread> gActiveThreads;  


static int gNextBlockId = 1000;




static bool gIsRunning = false;
static float gTimer = 0;
static int gBgColor = 0;
static float gToolbarAnimOffset = 0;

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

    
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "play sound", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "stop sounds", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "set vol to %", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"100")},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::COMMAND, "change vol by ", 0,0,true, {makeInput(bw*0.6f,bh*0.15f,fieldW,fieldH,"-10")},{}));
    blocks.push_back(makeBlock(id++, Category::SOUND, BlockShape::REPORTER, "volume", 0,0,true,{},{}));

    
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when flag clicked", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when space pressed", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when sprite clicked", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::COMMAND, "broadcast ", 0,0,true, {makeInput(bw*0.5f,bh*0.15f,fieldW*1.2f,fieldH,"msg1")},{}));
    blocks.push_back(makeBlock(id++, Category::EVENTS, BlockShape::HAT, "when I receive ", 0,0,true, {makeInput(bw*0.65f,bh*0.15f,fieldW*1.0f,fieldH,"msg1")},{}));

    
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::COMMAND, "wait  secs", 0,0,true, {makeInput(bw*0.27f,bh*0.15f,fieldW,fieldH,"1")}, {makeOpSlot(bw*0.27f,bh*0.15f,slotW,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "repeat ", 0,0,true, {makeInput(bw*0.4f,bh*0.05f,fieldW,fieldH*0.8f,"10")}, {makeOpSlot(bw*0.4f,bh*0.05f,slotW,fieldH*0.8f)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "forever", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "if  then", 0,0,true, {}, {makeOpSlot(bw*0.2f,bh*0.05f,slotW*1.5f,fieldH*0.8f)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "if  else", 0,0,true, {}, {makeOpSlot(bw*0.2f,bh*0.05f,slotW*1.5f,fieldH*0.8f)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::CAP, "stop all", 0,0,true,{},{}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::COMMAND, "wait until ", 0,0,true, {}, {makeOpSlot(bw*0.5f,bh*0.15f,slotW*1.3f,fieldH)}));
    blocks.push_back(makeBlock(id++, Category::CONTROL, BlockShape::C_BLOCK, "repeat until ", 0,0,true, {}, {makeOpSlot(bw*0.55f,bh*0.05f,slotW*1.3f,fieldH*0.8f)}));

    
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

    { int tx=bx+(int)(8*L.s), ty; if(b.shape==BlockShape::C_BLOCK) ty=by+(int)(L.CBLOCK_BAR_H*0.25f); else if(b.shape==BlockShape::HAT) ty=by+(int)(14*L.s); else ty=by+(int)(bh/2)-textHeightTTF()/2; drawTextTTF(rnd, tx,ty,b.text.c_str(),255,255,255,255); }

    for (int fi=0;fi<(int)b.inputs.size();fi++) {
        auto& inp=b.inputs[fi];
        int fy=by+(int)inp.relY, fw=(int)inp.width, fh=(int)inp.height;

        int textStartX = bx + (int)(8*L.s);
        int textEndX = textStartX + textWidthTTF(b.text.c_str()) + (int)(6*L.s);

        int fx = bx + (int)inp.relX;
        if (fx < textEndX && b.inputs.size() == 1) {
            fx = textEndX;
        }
        if (fi > 0 && b.inputs.size() > 1) {
            auto& prev = b.inputs[fi-1];
            int prevFx = bx + (int)prev.relX;
            if (prevFx < textEndX) prevFx = textEndX;
            int minFx = prevFx + (int)prev.width + (int)(8*L.s);
            if (fx < minFx) fx = minFx;
        }
        fillRoundedRect(rnd,fx,fy,fw,fh,4,255,255,255,255);
        if (inp.editing) { drawRoundedRectOutline(rnd,fx-1,fy-1,fw+2,fh+2,4,50,150,255,255); int tw=textWidthTTF(inp.value.c_str()); int cursorX=fx+3+tw; SDL_SetRenderDrawColor(rnd,0,0,0,255); SDL_RenderDrawLine(rnd,cursorX,fy+2,cursorX,fy+fh-2); }
        drawTextTTF(rnd, fx+3,fy+(fh-textHeightTTF())/2,inp.value.c_str(),0,0,0,255);
    }

    for (int si=0;si<(int)b.opSlots.size();si++) {
        auto& sl=b.opSlots[si];
        if (sl.embeddedBlockId<0) {
            bool hasInput=false;
            for (auto& inp:b.inputs) if (abs(inp.relX-sl.relX)<5&&abs(inp.relY-sl.relY)<5){hasInput=true;break;}
            if (!hasInput) { int sx2=bx+(int)sl.relX,sy2=by+(int)sl.relY,sw2=(int)sl.width,sh2=(int)sl.height; fillRoundedRect(rnd,sx2,sy2,sw2,sh2,sh2/2,255,255,255,120); }
        }
    }
}




static void drawCatSprite(SDL_Renderer* rnd, int cx, int cy, int sz, SDL_Color col) {
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
    int half=sz/2;
    fillEllipse(rnd,cx,cy,half,half,col.r,col.g,col.b,col.a);
    int earH=half*2/3, earW=half/3;
    for (int row=0;row<earH;row++) { float t=(float)row/earH; int w=(int)(earW*(1.0f-t)); SDL_SetRenderDrawColor(rnd,col.r,col.g,col.b,col.a); SDL_RenderDrawLine(rnd,cx-half/2-w,cy-half-row,cx-half/2+w,cy-half-row); SDL_RenderDrawLine(rnd,cx+half/2-w,cy-half-row,cx+half/2+w,cy-half-row); }
    fillEllipse(rnd,cx-half/3,cy-half/4,sz/10,sz/8,255,255,255,col.a);
    fillEllipse(rnd,cx+half/3,cy-half/4,sz/10,sz/8,255,255,255,col.a);
    fillEllipse(rnd,cx-half/3,cy-half/4,sz/20,sz/14,0,0,0,col.a);
    fillEllipse(rnd,cx+half/3,cy-half/4,sz/20,sz/14,0,0,0,col.a);
    SDL_SetRenderDrawColor(rnd,0,0,0,col.a);
    aalineRGBA(rnd,(Sint16)(cx-half/5),(Sint16)(cy+half/4),(Sint16)cx,(Sint16)(cy+half/3),0,0,0,col.a);
    aalineRGBA(rnd,(Sint16)(cx+half/5),(Sint16)(cy+half/4),(Sint16)cx,(Sint16)(cy+half/3),0,0,0,col.a);
}

static void drawSpeechBubble(SDL_Renderer* rnd, int cx, int cy, const char* text, bool isThink) {
    if (!text||text[0]=='\0') return;
    int tw=textWidthTTF(text)+(int)(20*L.s), th=textHeightTTF()+(int)(16*L.s);
    int bx2=cx-tw/2, by2=cy-th-(int)(20*L.s);
    fillRoundedRect(rnd,bx2,by2,tw,th,10,255,255,255,255);
    SDL_SetRenderDrawColor(rnd,180,180,180,255);
    SDL_Rect bdr={bx2,by2,tw,th}; SDL_RenderDrawRect(rnd,&bdr);
    if (isThink) { fillEllipse(rnd,cx,by2+th+5,5,5,255,255,255,255); fillEllipse(rnd,cx+3,by2+th+12,3,3,255,255,255,255); }
    else { SDL_SetRenderDrawColor(rnd,255,255,255,255); for(int row=0;row<12;row++){int y=by2+th+row;float t=row/12.0f;int lx=(int)((1-t)*(cx-5)+t*cx),rx=(int)((1-t)*(cx+5)+t*cx);SDL_RenderDrawLine(rnd,lx,y,rx,y);} }
    drawTextTTF(rnd, bx2+(int)(10*L.s),by2+(int)(8*L.s),text,0,0,0,255);
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





static float getInputValue(Block& block, int inputIdx) {
    if (inputIdx >= 0 && inputIdx < (int)block.inputs.size()) {
        try {
            return stof(block.inputs[inputIdx].value);
        } catch (...) {
            return 0;
        }
    }
    return 0;
}


static string getInputString(Block& block, int inputIdx) {
    if (inputIdx >= 0 && inputIdx < (int)block.inputs.size()) {
        return block.inputs[inputIdx].value;
    }
    return "";
}


static void startGreenFlag(vector<Block>& blocks, vector<Sprite>& sprites) {
    gActiveThreads.clear();
    gIsRunning = true;
    gTimer = 0;

    
    for (auto& block : blocks) {
        if (block.inPalette) continue;  

        
        if (block.text.find("when") != string::npos &&
            block.text.find("flag") != string::npos) {

            
            for (int i = 0; i < (int)sprites.size(); i++) {
                
                if (block.nextBlockId != -1) {
                    gActiveThreads.push_back(ScriptThread(block.nextBlockId, i));
                }
            }
        }
    }

    cout << "Green flag clicked! Started " << gActiveThreads.size() << " threads." << endl;
}


static void executeStep(ScriptThread& thread, vector<Block>& blocks,
                        vector<Sprite>& sprites, float dt) {

    
    if (thread.currentBlockId == -1) return;

    
    if (thread.isWaiting) {
        thread.waitTimer -= dt;
        if (thread.waitTimer > 0) return;  
        thread.isWaiting = false;
        
        Block* b = findBlock(blocks, thread.currentBlockId);
        if (b) thread.currentBlockId = b->nextBlockId;
        return;
    }

    
    Block* block = findBlock(blocks, thread.currentBlockId);
    if (!block) {
        thread.currentBlockId = -1;
        return;
    }

    
    if (thread.spriteIdx < 0 || thread.spriteIdx >= (int)sprites.size()) {
        thread.currentBlockId = -1;
        return;
    }

    Sprite& sp = sprites[thread.spriteIdx];
    string txt = block->text;


    if (txt.find("move") != string::npos && txt.find("steps") != string::npos) {
        float steps = getInputValue(*block, 0);
        float rad = (sp.direction - 90.0f) * 3.14159f / 180.0f;
        sp.x += cos(rad) * steps;
        sp.y += sin(rad) * steps;
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("turn") != string::npos && txt.find("R") != string::npos) {
        sp.direction += getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("turn") != string::npos && txt.find("L") != string::npos) {
        sp.direction -= getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("go to x") != string::npos) {
        sp.x = getInputValue(*block, 0);
        sp.y = getInputValue(*block, 1);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("set x to") != string::npos) {
        sp.x = getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("set y to") != string::npos) {
        sp.y = getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("change x by") != string::npos) {
        sp.x += getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("change y by") != string::npos) {
        sp.y += getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("point dir") != string::npos) {
        sp.direction = getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("glide") != string::npos) {
        
        sp.x = getInputValue(*block, 1);
        sp.y = getInputValue(*block, 2);
        thread.isWaiting = true;
        thread.waitTimer = getInputValue(*block, 0);
    }


    else if (txt.find("say") != string::npos && txt.find("sec") != string::npos) {
        sp.sayText = getInputString(*block, 0);
        sp.sayTimer = getInputValue(*block, 1);
        thread.isWaiting = true;
        thread.waitTimer = sp.sayTimer;
    }
    else if (txt.find("say") != string::npos) {
        sp.sayText = getInputString(*block, 0);
        sp.sayTimer = -1;  
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("think") != string::npos && txt.find("sec") != string::npos) {
        sp.thinkText = getInputString(*block, 0);
        sp.thinkTimer = getInputValue(*block, 1);
        thread.isWaiting = true;
        thread.waitTimer = sp.thinkTimer;
    }
    else if (txt.find("think") != string::npos) {
        sp.thinkText = getInputString(*block, 0);
        sp.thinkTimer = -1;
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt == "show") {
        sp.visible = true;
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt == "hide") {
        sp.visible = false;
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("set size") != string::npos) {
        sp.size = getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }
    else if (txt.find("change size") != string::npos) {
        sp.size += getInputValue(*block, 0);
        thread.currentBlockId = block->nextBlockId;
    }

    
    
    
    else if (txt.find("wait") != string::npos && txt.find("sec") != string::npos) {
        thread.isWaiting = true;
        thread.waitTimer = getInputValue(*block, 0);
    }
    else if (txt == "forever") {
        
        if (block->childHeadId != -1) {
            thread.loopStack.push_back({block->id, -1});  
            thread.currentBlockId = block->childHeadId;
        } else {
            
            
            thread.isWaiting = true;
            thread.waitTimer = 0.016f;  
        }
    }
    else if (txt.find("repeat") != string::npos && txt.find("until") == string::npos) {
        int count = (int)getInputValue(*block, 0);

        
        bool foundInStack = false;
        for (auto& lp : thread.loopStack) {
            if (lp.first == block->id) {
                foundInStack = true;
                lp.second++;
                if (lp.second >= count) {
                    
                    thread.loopStack.pop_back();
                    thread.currentBlockId = block->nextBlockId;
                } else {
                    
                    if (block->childHeadId != -1) {
                        thread.currentBlockId = block->childHeadId;
                    }
                }
                break;
            }
        }

        if (!foundInStack) {
            
            thread.loopStack.push_back({block->id, 0});
            if (block->childHeadId != -1) {
                thread.currentBlockId = block->childHeadId;
            } else {
                thread.currentBlockId = block->nextBlockId;
            }
        }
    }
    else if (txt.find("stop") != string::npos && txt.find("all") != string::npos) {
        gIsRunning = false;
        gActiveThreads.clear();
        return;
    }

    
    
    
    else {
        thread.currentBlockId = block->nextBlockId;
    }

    
    
    
    if (thread.currentBlockId == -1 && !thread.loopStack.empty()) {
        
        int loopBlockId = thread.loopStack.back().first;
        thread.currentBlockId = loopBlockId;
    }
}


static void executeAllThreads(vector<Block>& blocks, vector<Sprite>& sprites, float dt) {
    if (!gIsRunning) return;

    
    for (int i = (int)gActiveThreads.size() - 1; i >= 0; i--) {
        executeStep(gActiveThreads[i], blocks, sprites, dt);

        
        if (gActiveThreads[i].currentBlockId == -1 &&
            gActiveThreads[i].loopStack.empty()) {
            gActiveThreads.erase(gActiveThreads.begin() + i);
        }
    }
}




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
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    const char* fontPaths[] = {
        "DejaVuSans.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "C:/Windows/Fonts/verdana.ttf",
        "C:/msys64/mingw64/share/fonts/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        nullptr
    };
    bool fontLoaded = false;
    for (int fi = 0; fontPaths[fi]; fi++) {
        if (initFonts(fontPaths[fi])) { fontLoaded = true; break; }
    }
    if (!fontLoaded) {
        fprintf(stderr, "WARNING: Could not load any TTF font. Place DejaVuSans.ttf next to the exe.\n");
    }

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

            
            if (e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT) {
                int mx=e.button.x, my=e.button.y;
                bool clickedOnField=false;
                
                if (gBackdropPanelOpen) {
    int panelW = (int)(320 * L.s);
    int panelH = (int)(420 * L.s);
    int panelX = (L.winW - panelW) / 2;
    int panelY = L.TOOLBAR_HEIGHT + (int)(20 * L.s);

    int closeSz = (int)(24 * L.s);
    int closeBtnX = panelX + panelW - closeSz - 8;
    int closeBtnY = panelY + 8;
    if (mx >= closeBtnX && mx <= closeBtnX + closeSz &&
        my >= closeBtnY && my <= closeBtnY + closeSz) {
        gBackdropPanelOpen = false;
        continue;
    }

    int tabW = panelW / 3;
    int tabH = (int)(32 * L.s);
    int tabY = panelY + (int)(55 * L.s);
    for (int i = 0; i < 3; i++) {
        int tabX = panelX + i * tabW;
        if (mx >= tabX && mx <= tabX + tabW &&
            my >= tabY && my <= tabY + tabH) {
            gBackdropPanelTab = i;
            continue;
        }
    }

    int contentY = tabY + tabH + (int)(10 * L.s);
    int contentX = panelX + (int)(10 * L.s);
    int contentW = panelW - (int)(20 * L.s);

    if (gBackdropPanelTab == 0) {
        int itemH = (int)(36 * L.s);
        for (int i = 0; i < gBackdropLibraryCount; i++) {
            int itemY = contentY + (int)(20 * L.s) + i * (itemH + 6);
            if (mx >= contentX && mx <= contentX + contentW &&
                my >= itemY && my <= itemY + itemH) {
                gCurrentBackdropName = gBackdropItems[i].name;
                loadBackdrop(rnd, gBackdropItems[i].path);
                break;
            }
        }
    } else if (gBackdropPanelTab == 1) {
        int btnW = (int)(140 * L.s), btnH = (int)(40 * L.s);
        int btnX = panelX + (panelW - btnW) / 2;
        int btnY = contentY + (int)(30 * L.s);
        if (mx >= btnX && mx <= btnX + btnW && my >= btnY && my <= btnY + btnH) {
            const char* filters[4] = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
            const char* fileName = tinyfd_openFileDialog(
                "Select Background Image", "", 4, filters, "Image files", 0);
            if (fileName) {
                string fullPath = fileName;
                size_t sep = fullPath.find_last_of("/\\");
                gCurrentBackdropName = (sep != string::npos)
                    ? fullPath.substr(sep + 1) : fullPath;
                loadBackdrop(rnd, fileName);
                gBackdropPanelOpen = false;
            }
        }
    } else if (gBackdropPanelTab == 2) {
        int btnW = (int)(140 * L.s), btnH = (int)(40 * L.s);
        int btnX = panelX + (panelW - btnW) / 2;
        int btnY = contentY + (int)(30 * L.s);
        if (mx >= btnX && mx <= btnX + btnW && my >= btnY && my <= btnY + btnH) {
            gBackdropEditMode = true;
            gBackdropPanelOpen = false;
            costumeEditMode = true;
            selectedCostumeSpriteIdx = -1;
            if (costumeCanvas) SDL_DestroyTexture(costumeCanvas);
            costumeCanvas = SDL_CreateTexture(rnd, SDL_PIXELFORMAT_RGBA8888,
                SDL_TEXTUREACCESS_TARGET, canvasW, canvasH);
            SDL_SetRenderTarget(rnd, costumeCanvas);
            SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
            SDL_RenderClear(rnd);
            
            if (gBackdropTexture) {
                SDL_RenderCopy(rnd, gBackdropTexture, nullptr, nullptr);
            }
            SDL_SetRenderTarget(rnd, nullptr);

        }
    }
    continue;
}

                
                {
                    int spriteAreaY = L.TOOLBAR_HEIGHT + L.STAGE_HEIGHT + 5;
                    int thumbSz = L.SPRITE_THUMB;
                    int infoY = spriteAreaY + thumbSz + 10;
                    int infoX = L.PALETTE_WIDTH + 5;
                    int fieldW2 = (int)(L.STAGE_WIDTH * 0.35f);
                    int fieldH2 = (int)(22 * L.s);

                    int layerBtnW = (int)(40 * L.s), layerBtnH = fieldH2;
                    int row3Y = infoY + 2 * (fieldH2 + 8);
                    int layerBtnY = row3Y;

                    int frontBtnX = infoX + fieldW2 * 2 - layerBtnW - 5;
                    if (mx >= frontBtnX && mx <= frontBtnX + layerBtnW &&
                        my >= layerBtnY && my <= layerBtnY + layerBtnH) {
                        if (selectedSpriteIdx < (int)sprites.size() && sprites.size() > 1) {
                            std::swap(sprites[selectedSpriteIdx], sprites.back());
                            selectedSpriteIdx = (int)sprites.size() - 1;
                        }
                        continue;
                    }
                    if(costumeEditMode && costumeCanvas) {
                        int editorX = L.PALETTE_WIDTH + L.STAGE_WIDTH + 20;
                        int editorY = L.TOOLBAR_HEIGHT + 20;

                        int drawX = mx - editorX - 20;
                        int drawY = my - editorY - 20;

                        if(drawX >= 0 && drawX < canvasW && drawY >= 0 && drawY < canvasH) {
                            isDrawing = true;
                            lastDrawX = drawX;
                            lastDrawY = drawY;
                        }
                    }

                    int backBtnX = frontBtnX - layerBtnW - 5;
                    if (mx >= backBtnX && mx <= backBtnX + layerBtnW &&
                        my >= layerBtnY && my <= layerBtnY + layerBtnH) {
                        if (selectedSpriteIdx < (int)sprites.size() && sprites.size() > 1) {
                            std::swap(sprites[selectedSpriteIdx], sprites.front());
                            selectedSpriteIdx = 0;
                        }
                        continue;
                    }
                }
                if(costumeEditMode && costumeCanvas) {
                    int editorX = L.PALETTE_WIDTH + L.STAGE_WIDTH + 20;
                    int editorY = L.TOOLBAR_HEIGHT + 20;
                    int toolbarY = editorY + canvasH + 30;

                    if(mx >= editorX+20 && mx <= editorX+80 && my >= toolbarY && my <= toolbarY+30) {
                        penColorR = 0; penColorG = 0; penColorB = 0;
                    }
                    if(mx >= editorX+90 && mx <= editorX+150 && my >= toolbarY && my <= toolbarY+30) {
                        penColorR = 255; penColorG = 255; penColorB = 255;
                    }
                    int colorBtnY = toolbarY + 40;
                    int colorBtnW = 25, colorBtnH = 25;

                    if(mx >= editorX+20 && mx <= editorX+20+colorBtnW &&
                       my >= colorBtnY && my <= colorBtnY+colorBtnH) {
                        penColorR = 255; penColorG = 0; penColorB = 0;
                    }
                    if(mx >= editorX+50 && mx <= editorX+50+colorBtnW &&
                       my >= colorBtnY && my <= colorBtnY+colorBtnH) {
                        penColorR = 0; penColorG = 255; penColorB = 0;
                    }
                    if(mx >= editorX+80 && mx <= editorX+80+colorBtnW &&
                       my >= colorBtnY && my <= colorBtnY+colorBtnH) {
                        penColorR = 0; penColorG = 0; penColorB = 255;
                    }
                    if(mx >= editorX+110 && mx <= editorX+110+colorBtnW &&
                       my >= colorBtnY && my <= colorBtnY+colorBtnH) {
                        penColorR = 0; penColorG = 0; penColorB = 0;
                    }
                    if(mx >= editorX+140 && mx <= editorX+140+colorBtnW &&
                       my >= colorBtnY && my <= colorBtnY+colorBtnH) {
                        penColorR = 255; penColorG = 255; penColorB = 255;
                    }
                    if(mx >= editorX+160 && mx <= editorX+220 && my >= toolbarY && my <= toolbarY+30) {
                        if (gBackdropEditMode) {
                            
                            if (gBackdropTexture) SDL_DestroyTexture(gBackdropTexture);
                            gBackdropTexture = costumeCanvas;
                            costumeCanvas = nullptr;
                            gCurrentBackdropName = "custom";
                            gBackdropEditMode = false;
                        } else if(selectedCostumeSpriteIdx >= 0 && selectedCostumeSpriteIdx < (int)sprites.size()) {
                            if(sprites[selectedCostumeSpriteIdx].uploadedTexture) SDL_DestroyTexture(sprites[selectedCostumeSpriteIdx].uploadedTexture);
                            sprites[selectedCostumeSpriteIdx].uploadedTexture = costumeCanvas;
                            costumeCanvas = nullptr;
                        }
                        costumeEditMode = false;
                    }

                    if(mx >= editorX+230 && mx <= editorX+290 && my >= toolbarY && my <= toolbarY+30) {
                        if(costumeCanvas) SDL_DestroyTexture(costumeCanvas);
                        costumeCanvas = nullptr;
                        costumeEditMode = false;
                    }
                }
                int panelX=L.PALETTE_WIDTH, stageW=L.STAGE_WIDTH;
                int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
                int thumbSize=L.SPRITE_THUMB;
                int infoY=spriteAreaY+thumbSize+10, infoX=panelX+5;
                int fieldW2=(int)(stageW*0.35f), fieldH2=(int)(22*L.s);
                infoY=spriteAreaY+thumbSize+10, infoX=panelX+5;

                
                {
                    int spriteAreaY = L.TOOLBAR_HEIGHT + L.STAGE_HEIGHT + 5;
                    int thumbSz = L.SPRITE_THUMB;
                    int infoY = spriteAreaY + thumbSz + 10;
                    int infoX = L.PALETTE_WIDTH + 5;
                    int fieldW2 = (int)(L.STAGE_WIDTH * 0.35f);
                    int fieldH2 = (int)(22 * L.s);

                    int uploadBtnW = (int)(60 * L.s), uploadBtnH = fieldH2;
                    int uploadBtnX = infoX + fieldW2 * 2 - uploadBtnW - 10;
                    int uploadBtnY = infoY;

                    if(mx >= uploadBtnX && mx <= uploadBtnX + uploadBtnW &&
                       my >= uploadBtnY && my <= uploadBtnY + uploadBtnH) {

                        if(selectedSpriteIdx < (int)sprites.size()) {
                            const char* filters[3] = { "*.png", "*.jpg", "*.jpeg" };
                            const char* fileName = tinyfd_openFileDialog(
                                "Select Image", "",
                                3, filters,
                                "Image files",
                                0
                            );

                            if(fileName != NULL) {
                                SDL_Surface* surf = IMG_Load(fileName);
                                if(surf) {
                                    if(sprites[selectedSpriteIdx].uploadedTexture)
                                        SDL_DestroyTexture(sprites[selectedSpriteIdx].uploadedTexture);
                                    sprites[selectedSpriteIdx].uploadedTexture = SDL_CreateTextureFromSurface(rnd, surf);
                                    SDL_FreeSurface(surf);
                                    cout << "Image loaded: " << fileName << endl;
                                } else {
                                    cout << "Failed to load: " << IMG_GetError() << endl;
                                }
                            }
                        }
                    }
                }

                
                {
                    int spriteAreaY = L.TOOLBAR_HEIGHT + L.STAGE_HEIGHT + 5;
                    int thumbSz = L.SPRITE_THUMB;
                    int infoY = spriteAreaY + thumbSz + 10;
                    int infoX = L.PALETTE_WIDTH + 5;
                    int fieldW2 = (int)(L.STAGE_WIDTH * 0.35f);
                    int fieldH2 = (int)(22 * L.s);

                    int editBtnW = (int)(60*L.s), editBtnH = fieldH2;
                    int uploadBtnW = (int)(60*L.s);
                    int editBtnX = infoX + fieldW2*2 - uploadBtnW - editBtnW - 15;
                    int editBtnY = infoY;

                    if(mx >= editBtnX && mx <= editBtnX + editBtnW &&
                    my >= editBtnY && my <= editBtnY + editBtnH) {
                        costumeEditMode = !costumeEditMode;
                        if(costumeEditMode) {
                            selectedCostumeSpriteIdx = selectedSpriteIdx;
                            if(costumeCanvas) SDL_DestroyTexture(costumeCanvas);
                            costumeCanvas = SDL_CreateTexture(rnd, SDL_PIXELFORMAT_RGBA8888,
                            SDL_TEXTUREACCESS_TARGET, canvasW, canvasH);
                            SDL_SetRenderTarget(rnd, costumeCanvas);
                            SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
                            SDL_RenderClear(rnd);

                            if(selectedSpriteIdx < (int)sprites.size() &&
                               sprites[selectedSpriteIdx].uploadedTexture) {
                                SDL_RenderCopy(rnd, sprites[selectedSpriteIdx].uploadedTexture, nullptr, nullptr);
                            }

                            SDL_SetRenderTarget(rnd, nullptr);
                        }
                    }
                }
                
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
                                switch(fr.idx){case 0:sprInfoEdit.buffer=sp.name;break;case 1:sprInfoEdit.buffer=floatToString(sp.x);break;case 2:sprInfoEdit.buffer=floatToString(sp.y);break;case 3:sprInfoEdit.buffer=floatToString(sp.size);break;case 4:sprInfoEdit.buffer=floatToString(sp.direction);break;case 5: sprInfoEdit.buffer=floatToString(sp.ghostEffect); break;}
                                clickedOnField=true;
                                if(gEdit.active&&gEdit.blockId>=0){Block* eb=findBlock(blocks,gEdit.blockId);if(eb&&gEdit.fieldIndex>=0&&gEdit.fieldIndex<(int)eb->inputs.size())eb->inputs[gEdit.fieldIndex].editing=false;gEdit.active=false;}
                            }
                            break;
                        }
                    }
                }

                
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
                    if(sprInfoEdit.field>=0&&selectedSpriteIdx<(int)sprites.size()){
                        Sprite& sp=sprites[selectedSpriteIdx];
                        switch(sprInfoEdit.field){case 0:sp.name=sprInfoEdit.buffer;break;case 1:sp.x=atof(sprInfoEdit.buffer.c_str());break;case 2:sp.y=atof(sprInfoEdit.buffer.c_str());break;case 3:sp.size=atof(sprInfoEdit.buffer.c_str());break;case 4:sp.direction=atof(sprInfoEdit.buffer.c_str());break;case 5: sp.ghostEffect = atof(sprInfoEdit.buffer.c_str()); break;}
                    }
                    sprInfoEdit.field=-1; sprInfoEdit.buffer.clear();
                }

                
                {
                    int bgBtnX = L.PALETTE_WIDTH + L.STAGE_WIDTH - (int)(35*L.s);
                    int bgBtnY = L.TOOLBAR_HEIGHT + 3;
                    int bgBtnW = (int)(32*L.s);
                    int bgBtnH = (int)(18*L.s);
                    if(mx>=bgBtnX && mx<=bgBtnX+bgBtnW && my>=bgBtnY && my<=bgBtnY+bgBtnH){
                        gBgColor = (gBgColor + 1) % NUM_BG_COLORS;
                        continue;
                    }
                }

                if (my<L.TOOLBAR_HEIGHT) {
                    int flagX=(int)(winW*0.4f),flagY=5,flagSz=L.TOOLBAR_HEIGHT-10;
                    if(mx>=flagX&&mx<=flagX+flagSz&&my>=flagY&&my<=flagY+flagSz) {
                        gIsRunning=true;
                    }
                    int stopX=flagX+flagSz+10;
                    if(mx>=stopX&&mx<=stopX+flagSz&&my>=flagY&&my<=flagY+flagSz) gIsRunning=false;
                    int resetX=stopX+flagSz+10;
                    int resetW2=(int)(60*L.s);
                    if(mx>=resetX&&mx<=resetX+resetW2&&my>=flagY&&my<=flagY+flagSz){resetProject(blocks,sprites);selectedSpriteIdx=0;}
                    int timerX = resetX + resetW2 + 30;
                    int backdropBtnX = timerX + 200;
                    int backdropBtnW = (int)(80*L.s);
if(mx >= backdropBtnX && mx <= backdropBtnX + backdropBtnW && my >= flagY && my <= flagY + flagSz) {
    gBackdropPanelOpen = !gBackdropPanelOpen;
    continue;
}





                    int fontBtnX=resetX+resetW2+80;
                    int fontBtnW=(int)(28*L.s);
                    if(mx>=fontBtnX&&mx<=fontBtnX+fontBtnW&&my>=flagY&&my<=flagY+flagSz){
                        setFontSize(gFontSizeNormal-1);
                        L.update(winW, winH);
                        vector<Block> kept;
                        for(auto& b:blocks) if(!b.inPalette) kept.push_back(b);
                        blocks = buildPaletteBlocks();
                        for(auto& b:kept) blocks.push_back(b);
                    }
                    if(mx>=fontBtnX+fontBtnW+4&&mx<=fontBtnX+fontBtnW*2+4&&my>=flagY&&my<=flagY+flagSz){
                        setFontSize(gFontSizeNormal+1);
                        L.update(winW, winH);
                        vector<Block> kept;
                        for(auto& b:blocks) if(!b.inPalette) kept.push_back(b);
                        blocks = buildPaletteBlocks();
                        for(auto& b:kept) blocks.push_back(b);
                    }
                    continue;
                }

                
                if (mx<L.CAT_PANEL_WIDTH&&my>L.TOOLBAR_HEIGHT) {
                    int catY=L.TOOLBAR_HEIGHT+5;
                    for(int i=0;i<NUM_CATEGORIES;i++){int btnY=catY+i*(L.CAT_BTN_HEIGHT+3);if(my>=btnY&&my<=btnY+L.CAT_BTN_HEIGHT){selectedCategory=(Category)i;paletteScrollY=0;break;}}
                    continue;
                }

                
                if (gBackdropEditMode) {
                    int panelX = L.PALETTE_WIDTH + L.STAGE_WIDTH + 20;
                    int panelY = L.TOOLBAR_HEIGHT + 20;
                    int panelW = 250, panelH = 300;
                    if (mx >= panelX && mx <= panelX + panelW && my >= panelY && my <= panelY + panelH) {
                        bool handled = false;
                        int btnY = panelY + 50;
                        for (int i = 0; i < gBackdropLibraryCount; i++) {
                            if (mx >= panelX + 10 && mx <= panelX + panelW - 10 && my >= btnY && my <= btnY + 35) {
                                gCurrentBackdropName = gBackdropItems[i].name;
                                loadBackdrop(rnd, gBackdropItems[i].path);
                                gBackdropTexture = nullptr; 
                                handled = true;
                                break;
                            }
                            btnY += 45;
                        }
                        if (!handled) {
                            int uploadBtnY = btnY + 20;
                            if (mx >= panelX + 10 && mx <= panelX + panelW - 10 && my >= uploadBtnY && my <= uploadBtnY + 35) {
                                const char* filters[3] = { "*.png", "*.jpg", "*.jpeg" };
                                const char* fileName = tinyfd_openFileDialog("Select Backdrop", "", 3, filters, "Image files", 0);
                                if (fileName) {
                                    if (gBackdropTexture) SDL_DestroyTexture(gBackdropTexture);
                                    SDL_Surface* surf = IMG_Load(fileName);
                                    if (surf) {
                                        gBackdropTexture = SDL_CreateTextureFromSurface(rnd, surf);
                                        SDL_FreeSurface(surf);
                                        gCurrentBackdropName = fileName;
                                    }
                                }
                                handled = true;
                            }
                            if (!handled) {
                                int closeBtnY = uploadBtnY + 50;
                                if (mx >= panelX + 10 && mx <= panelX + panelW - 10 && my >= closeBtnY && my <= closeBtnY + 35) {
                                    gBackdropEditMode = false;
                                    handled = true;
                                }
                            }
                        }
                        if (handled) {
                            continue;
                        }
                    }
                }

                
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

                
                {
                    int spriteAreaY=L.TOOLBAR_HEIGHT+L.STAGE_HEIGHT+5;
                    int thumbSz=L.SPRITE_THUMB, startX=L.PALETTE_WIDTH+5;
                    int delBtnSize=(int)(16*L.s);
                    int eyeBtnSize=(int)(16*L.s);
                    bool handledSprite=false;

                    for(int si=0;si<(int)sprites.size();si++){
                        int tx=startX+si*(thumbSz+8), ty=spriteAreaY;

                        int eyeBtnX=tx+thumbSz-delBtnSize-eyeBtnSize-6, eyeBtnY=ty+2;
                        if(mx>=eyeBtnX&&mx<=eyeBtnX+eyeBtnSize&&my>=eyeBtnY&&my<=eyeBtnY+eyeBtnSize){
                            sprites[si].visible=!sprites[si].visible;
                            handledSprite=true; break;
                        }

                        int delBtnX=tx+thumbSz-delBtnSize-2, delBtnY=ty+2;
                        if(mx>=delBtnX&&mx<=delBtnX+delBtnSize&&my>=delBtnY&&my<=delBtnY+delBtnSize){
                            if(sprites.size()>1){
                                sprites.erase(sprites.begin()+si);
                                if(selectedSpriteIdx>=(int)sprites.size()) selectedSpriteIdx=(int)sprites.size()-1;
                                for(int j=0;j<(int)sprites.size();j++) sprites[j].selected=(j==selectedSpriteIdx);
                            }
                            handledSprite=true; break;
                        }

                        if(mx>=tx&&mx<=tx+thumbSz&&my>=ty&&my<=ty+thumbSz){
                            selectedSpriteIdx=si;
                            for(int j=0;j<(int)sprites.size();j++) sprites[j].selected=(j==si);
                            handledSprite=true; break;
                        }
                    }
                    if (handledSprite) continue;
                }

                
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

            } 


            
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
                if(costumeEditMode && costumeCanvas && isDrawing) {
                    int editorX = L.PALETTE_WIDTH + L.STAGE_WIDTH + 20;
                    int editorY = L.TOOLBAR_HEIGHT + 20;

                    int drawX = mx - editorX - 20;
                    int drawY = my - editorY - 20;

                    if(drawX >= 0 && drawX < canvasW && drawY >= 0 && drawY < canvasH) {
                        SDL_SetRenderTarget(rnd, costumeCanvas);
                        SDL_SetRenderDrawColor(rnd, penColorR, penColorG, penColorB, 255);

                        if(lastDrawX >= 0 && lastDrawY >= 0) {
                            SDL_RenderDrawLine(rnd, lastDrawX, lastDrawY, drawX, drawY);
                        }

                        SDL_SetRenderTarget(rnd, nullptr);
                        lastDrawX = drawX;
                        lastDrawY = drawY;
                    }
                }
            }

            
            if (e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT) {
                if(costumeEditMode) {
                    isDrawing = false;
                    lastDrawX = -1;
                    lastDrawY = -1;
                }
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
        } 

        
        
        
        SDL_SetRenderDrawColor(rnd,240,240,240,255);
        SDL_RenderClear(rnd);

        
        {
            for (int i = 0; i < winW; i += 20) {
                Uint8 r = (Uint8)(128 + 127 * sin((i + gToolbarAnimOffset) * 0.05));
                Uint8 g = (Uint8)(128 + 127 * sin((i + gToolbarAnimOffset) * 0.05 + 2));
                Uint8 b = (Uint8)(128 + 127 * sin((i + gToolbarAnimOffset) * 0.05 + 4));
                SDL_SetRenderDrawColor(rnd, r, g, b, 255);
                SDL_Rect bar = {i, 0, 20, 3};
                SDL_RenderFillRect(rnd, &bar);
            }
            gToolbarAnimOffset += 0.5f;
            SDL_SetRenderDrawColor(rnd,55,55,70,255);
            SDL_Rect toolbar={0,0,winW,L.TOOLBAR_HEIGHT}; SDL_RenderFillRect(rnd,&toolbar);
            drawTextTTF(rnd, 10,(L.TOOLBAR_HEIGHT-textHeightTTF())/2,"Scratch IDE",255,255,255,255);
            int flagX=(int)(winW*0.4f),flagY=5,flagSz=L.TOOLBAR_HEIGHT-10;
            fillRoundedRect(rnd,flagX,flagY,flagSz,flagSz,6,gIsRunning?0:30,gIsRunning?180:150,gIsRunning?0:30,255);
            drawTextTTF(rnd, flagX+flagSz/4,flagY+flagSz/4,">",255,255,255,255);
            int stopX=flagX+flagSz+10;
            fillRoundedRect(rnd,stopX,flagY,flagSz,flagSz,6,200,50,50,255);
            drawTextTTF(rnd, stopX+flagSz/4,flagY+flagSz/4,"#",255,255,255,255);
            int resetX=stopX+flagSz+10, resetW=(int)(60*L.s);
            fillRoundedRect(rnd,resetX,flagY,resetW,flagSz,6,resetHovered?100:80,resetHovered?100:80,resetHovered?120:100,255);
            drawTextTTF(rnd, resetX+5,flagY+flagSz/4,"Reset",255,255,255,255);
            int timerX = resetX + resetW + 30;
            int backdropBtnX = timerX + 200;
            int backdropBtnW = (int)(80*L.s);
            fillRoundedRect(rnd, backdropBtnX, flagY, backdropBtnW, flagSz, 6, 50,100,150,255);
            drawTextTTF(rnd, backdropBtnX+10, flagY+flagSz/4, "Backdrop", 255,255,255,255);

            char timerBuf[32]; snprintf(timerBuf,sizeof(timerBuf),"T:%.1f",gTimer);
            drawTextTTF(rnd, timerX, flagY+flagSz/4, timerBuf, 200,200,200,255);
            int fontBtnX = resetX+resetW+80;
            int fontBtnW = (int)(28*L.s), fontBtnH = flagSz;
            fillRoundedRect(rnd,fontBtnX,flagY,fontBtnW,fontBtnH,5,80,80,120,255);
            drawTextTTF(rnd,fontBtnX+5,flagY+fontBtnH/4,"A-",220,220,220,255);
            fillRoundedRect(rnd,fontBtnX+fontBtnW+4,flagY,fontBtnW,fontBtnH,5,80,80,120,255);
            drawTextTTF(rnd,fontBtnX+fontBtnW+8,flagY+fontBtnH/4,"A+",220,220,220,255);
            char szBuf[8]; snprintf(szBuf,sizeof(szBuf),"%d",gFontSizeNormal);
            drawTextTTF(rnd,fontBtnX+fontBtnW*2+10,flagY+fontBtnH/4,szBuf,180,220,255,255);
        }

        
        {
            SDL_SetRenderDrawColor(rnd,45,45,60,255);
            SDL_Rect catPanel={0,L.TOOLBAR_HEIGHT,L.CAT_PANEL_WIDTH,winH-L.TOOLBAR_HEIGHT}; SDL_RenderFillRect(rnd,&catPanel);
            int catY=L.TOOLBAR_HEIGHT+5;
            for(int i=0;i<NUM_CATEGORIES;i++){
                Category c=(Category)i; SDL_Color cc=catColor(c);
                int btnY=catY+i*(L.CAT_BTN_HEIGHT+3); bool sel=(c==selectedCategory);
                if(sel){fillRoundedRect(rnd,3,btnY,L.CAT_PANEL_WIDTH-6,L.CAT_BTN_HEIGHT,6,cc.r,cc.g,cc.b,255);drawTextTTF(rnd, 10,btnY+(L.CAT_BTN_HEIGHT-textHeightTTF())/2,catName(c),255,255,255,255);}
                else{fillRoundedRect(rnd,3,btnY,L.CAT_PANEL_WIDTH-6,L.CAT_BTN_HEIGHT,6,60,60,75,255);drawTextTTF(rnd, 10,btnY+(L.CAT_BTN_HEIGHT-textHeightTTF())/2,catName(c),cc.r,cc.g,cc.b,255);}
            }
        }

        
        {
            SDL_SetRenderDrawColor(rnd,50,50,65,255);
            SDL_Rect palBg={L.CAT_PANEL_WIDTH,L.TOOLBAR_HEIGHT,L.PALETTE_WIDTH-L.CAT_PANEL_WIDTH,winH-L.TOOLBAR_HEIGHT}; SDL_RenderFillRect(rnd,&palBg);
            SDL_Rect clip={L.CAT_PANEL_WIDTH,L.TOOLBAR_HEIGHT,L.PALETTE_WIDTH-L.CAT_PANEL_WIDTH,winH-L.TOOLBAR_HEIGHT}; SDL_RenderSetClipRect(rnd,&clip);
            float yy=(float)(L.TOOLBAR_HEIGHT+5+paletteScrollY);
            for(auto& b:blocks){if(!b.inPalette||b.cat!=selectedCategory)continue;b.x=(float)(L.CAT_PANEL_WIDTH+5);b.y=yy;drawBlock(rnd,b,blocks);yy+=b.h+8*L.s;}
            SDL_RenderSetClipRect(rnd,nullptr);
        }

        
        {
            int stageX=L.PALETTE_WIDTH,stageY=L.TOOLBAR_HEIGHT,stageW=L.STAGE_WIDTH,stageH=L.STAGE_HEIGHT;SDL_SetRenderDrawColor(rnd,255,255,255,255);
            SDL_Rect stageRect={stageX,stageY,stageW,stageH};
            
            if (gBackdropTexture) {
                int tw, th;
                SDL_QueryTexture(gBackdropTexture, nullptr, nullptr, &tw, &th);
                SDL_Rect dst;
                dst.x = stageX + (stageW - tw) / 2;
                dst.y = stageY + (stageH - th) / 2;
                dst.w = tw;
                dst.h = th;
                SDL_RenderCopy(rnd, gBackdropTexture, nullptr, &dst);
            }

            if (!gBackdropTexture) {
                SDL_Color bgCol = BG_COLORS[gBgColor];
                SDL_SetRenderDrawColor(rnd, bgCol.r, bgCol.g, bgCol.b, 255);
                SDL_RenderFillRect(rnd, &stageRect);
            }

            int bgBtnX = stageX + stageW - (int)(35*L.s);
            int bgBtnY = stageY + 3;
            int bgBtnW = (int)(32*L.s);
            int bgBtnH = (int)(18*L.s);
            static float bgPulse = 0;
            bgPulse += 0.05f;
            Uint8 pulseVal = (Uint8)(100 + 30 * sin(bgPulse));
            fillRoundedRect(rnd, bgBtnX, bgBtnY, bgBtnW, bgBtnH, 4, pulseVal, pulseVal, pulseVal + 20, 255);
            drawTextTTF(rnd, bgBtnX+3, bgBtnY+3, "BG", 255,255,255,255);
            int stageCX=stageX+stageW/2, stageCY=stageY+stageH/2;
            aalineRGBA(rnd,(Sint16)stageCX,(Sint16)stageY,(Sint16)stageCX,(Sint16)(stageY+stageH),235,235,235,255);
            aalineRGBA(rnd,(Sint16)stageX,(Sint16)stageCY,(Sint16)(stageX+stageW),(Sint16)stageCY,235,235,235,255);

            SDL_Rect stageClip={stageX,stageY,stageW,stageH}; SDL_RenderSetClipRect(rnd,&stageClip);
            for(int si=0;si<(int)sprites.size();si++){
                Sprite& sp=sprites[si];
                if(!sp.visible) continue;
                int sx=stageCX+(int)sp.x, sy=stageCY-(int)sp.y;
                int sz=(int)(30*L.s*sp.size/100.0f);

                SDL_Color drawCol = sp.color;
                if (sp.colorEffect == 1) { drawCol = {255,100,100,255}; }
                else if (sp.colorEffect == 2) { drawCol = {100,255,100,255}; }
                else if (sp.colorEffect == 3) { drawCol = {100,100,255,255}; }
                else if (sp.colorEffect == 4) { drawCol = {255,255,100,255}; }
                else if (sp.colorEffect == 5) { drawCol = {200,100,255,255}; }

                drawCol.a = (Uint8)(255 * (1.0f - sp.ghostEffect / 100.0f));

                if (sp.uploadedTexture) {
                    SDL_Rect srcRect = {0,0,0,0};
                    SDL_QueryTexture(sp.uploadedTexture, NULL, NULL, &srcRect.w, &srcRect.h);
                    SDL_Rect dstRect = {sx-sz/2, sy-sz/2, sz, sz};
                    SDL_RenderCopy(rnd, sp.uploadedTexture, &srcRect, &dstRect);
                } else {
                    drawCatSprite(rnd,sx,sy,sz,drawCol);
                }
                float angle=(sp.direction-90)*M_PI/180.0f;
                int arrowX=sx+(int)((sz*0.8f)*cos(angle));
                int arrowY=sy+(int)((sz*0.8f)*sin(angle));
                aalineRGBA(rnd,(Sint16)sx,(Sint16)sy,(Sint16)arrowX,(Sint16)arrowY,0,0,0,180);
                filledEllipseRGBA(rnd,(Sint16)arrowX,(Sint16)arrowY,(Sint16)(3*L.s),(Sint16)(3*L.s),0,0,0,255);

                if(si==selectedSpriteIdx){SDL_SetRenderDrawColor(rnd,50,150,255,255);SDL_Rect selR={sx-sz-3,sy-sz-3,2*sz+6,2*sz+6};SDL_RenderDrawRect(rnd,&selR);}

                if(!sp.sayText.empty()) drawSpeechBubble(rnd,sx,sy-sz-5,sp.sayText.c_str(),false);
                if(!sp.thinkText.empty()) drawSpeechBubble(rnd,sx,sy-sz-5,sp.thinkText.c_str(),true);
            }
            SDL_RenderSetClipRect(rnd,nullptr);
        }

        
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

                SDL_Color thumbCol = sprites[si].color;
                if(!sprites[si].visible){ thumbCol.r=(Uint8)(thumbCol.r*0.4f); thumbCol.g=(Uint8)(thumbCol.g*0.4f); thumbCol.b=(Uint8)(thumbCol.b*0.4f); }
                drawCatSprite(rnd,tx+thumbSz/2,ty+thumbSz/2,thumbSz/2-4,thumbCol);

                drawTextTTF(rnd, tx+2,ty+thumbSz-textHeightTTF()-2,sprites[si].name.c_str(),0,0,0,255);

                int eyeBtnX=tx+thumbSz-delBtnSize-eyeBtnSize-6, eyeBtnY=ty+2;
                Uint8 eyeR=sprites[si].visible?50:150, eyeG=sprites[si].visible?150:150, eyeB2=sprites[si].visible?50:150;
                fillRoundedRect(rnd,eyeBtnX,eyeBtnY,eyeBtnSize,eyeBtnSize,4,eyeR,eyeG,eyeB2,255);
                drawTextTTF(rnd, eyeBtnX+eyeBtnSize/4,eyeBtnY+1,"O",255,255,255,255);

                int delBtnX=tx+thumbSz-delBtnSize-2, delBtnY=ty+2;
                fillRoundedRect(rnd,delBtnX,delBtnY,delBtnSize,delBtnSize,4,220,50,50,255);
                drawTextTTF(rnd, delBtnX+delBtnSize/4,delBtnY+1,"X",255,255,255,255);
            }

            {
                int addBtnX=stageX+stageW-(int)(40*L.s), addBtnY=spriteAreaY, addBtnSz=(int)(30*L.s);
                fillRoundedRect(rnd,addBtnX,addBtnY,addBtnSz,addBtnSz,8,50,150,50,255);
                drawTextTTF(rnd, addBtnX+addBtnSz/3,addBtnY+addBtnSz/4,"+",255,255,255,255);
            }

            if(selectedSpriteIdx<(int)sprites.size()){
                Sprite& sp=sprites[selectedSpriteIdx];
                int infoY=spriteAreaY+thumbSz+10, infoX=stageX+5;
                int fieldW2=(int)(stageW*0.35f), fieldH2=(int)(22*L.s);

                
                {
                    drawTextTTF(rnd, infoX,infoY+(fieldH2-textHeightTTF())/2,"Name:",80,80,80,255);
                    int fx=infoX+(int)(40*L.s); bool editing=(sprInfoEdit.field==0);
                    fillRoundedRect(rnd,fx,infoY,fieldW2*2,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,infoY-1,fieldW2*2+2,fieldH2+2,4,50,150,255,255);
                    const char* dispText=editing?sprInfoEdit.buffer.c_str():sp.name.c_str();
                    drawTextTTF(rnd, fx+4,infoY+(fieldH2-textHeightTTF())/2,dispText,0,0,0,255);
                    if(editing){int cw=textWidthTTF(dispText);SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,infoY+2,fx+4+cw,infoY+fieldH2-2);}
                }

                int row2Y=infoY+fieldH2+8;
                
                {
                    drawTextTTF(rnd, infoX,row2Y+(fieldH2-textHeightTTF())/2,"X:",80,80,80,255);
                    int fx=infoX+(int)(15*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==1);
                    fillRoundedRect(rnd,fx,row2Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row2Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.x);
                    drawTextTTF(rnd, fx+4,row2Y+(fieldH2-textHeightTTF())/2,val.c_str(),0,0,0,255);
                    if(editing){int cw=textWidthTTF(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row2Y+2,fx+4+cw,row2Y+fieldH2-2);}
                }
                
                {
                    int fx=infoX+fieldW2+(int)(15*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==2);
                    drawTextTTF(rnd, infoX+fieldW2,row2Y+(fieldH2-textHeightTTF())/2,"Y:",80,80,80,255);
                    fillRoundedRect(rnd,fx,row2Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row2Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.y);
                    drawTextTTF(rnd, fx+4,row2Y+(fieldH2-textHeightTTF())/2,val.c_str(),0,0,0,255);
                    if(editing){int cw=textWidthTTF(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row2Y+2,fx+4+cw,row2Y+fieldH2-2);}
                }

                int row3Y=row2Y+fieldH2+8;
                
                {
                    drawTextTTF(rnd, infoX,row3Y+(fieldH2-textHeightTTF())/2,"Sz:",80,80,80,255);
                    int fx=infoX+(int)(30*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==3);
                    fillRoundedRect(rnd,fx,row3Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row3Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.size);
                    drawTextTTF(rnd, fx+4,row3Y+(fieldH2-textHeightTTF())/2,val.c_str(),0,0,0,255);

                    if(editing){int cw=textWidthTTF(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row3Y+2,fx+4+cw,row3Y+fieldH2-2);}
                    
                    int uploadBtnW = (int)(60*L.s), uploadBtnH = fieldH2;
                    int uploadBtnX = infoX + fieldW2*2 - uploadBtnW - 10;
                    int uploadBtnY = infoY;
                    fillRoundedRect(rnd, uploadBtnX, uploadBtnY, uploadBtnW, uploadBtnH, 4, 50,150,50,255);
                    drawTextTTF(rnd, uploadBtnX+10, uploadBtnY+(uploadBtnH-textHeightTTF())/2, "Upload", 255,255,255,255);

                    int editBtnW = (int)(60*L.s), editBtnH = fieldH2;
                    int editBtnX = uploadBtnX - editBtnW - 10;
                    int editBtnY = uploadBtnY;
                    fillRoundedRect(rnd, editBtnX, editBtnY, editBtnW, editBtnH, 4, 150,50,150,255);
                    drawTextTTF(rnd, editBtnX+10, editBtnY+(editBtnH-textHeightTTF())/2, "Edit", 255,255,255,255);

                    int row4Y = row3Y + fieldH2 + 8;
                    {
                        drawTextTTF(rnd, infoX,row4Y+(fieldH2-textHeightTTF())/2,"Gh:",80,80,80,255);
                        int fx=infoX+(int)(30*L.s),fw=fieldW2-(int)(20*L.s);
                        bool editing=(sprInfoEdit.field==5);
                        fillRoundedRect(rnd,fx,row4Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                        if(editing) drawRoundedRectOutline(rnd,fx-1,row4Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                        string val=editing?sprInfoEdit.buffer:floatToString(sp.ghostEffect);
                        drawTextTTF(rnd, fx+4,row4Y+(fieldH2-textHeightTTF())/2,val.c_str(),0,0,0,255);
                        if(editing){int cw=textWidthTTF(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row4Y+2,fx+4+cw,row4Y+fieldH2-2);}
                    }
                }
                int layerBtnW = (int)(40 * L.s), layerBtnH = fieldH2;
                int layerBtnY = row3Y;

                int frontBtnX = infoX + fieldW2 * 2 - layerBtnW - 5;
                fillRoundedRect(rnd, frontBtnX, layerBtnY, layerBtnW, layerBtnH, 4, 50, 150, 50, 255);
                drawTextTTF(rnd, frontBtnX + 10, layerBtnY + (layerBtnH - textHeightTTF()) / 2, ">>", 255, 255, 255, 255);

                int backBtnX = frontBtnX - layerBtnW - 5;
                fillRoundedRect(rnd, backBtnX, layerBtnY, layerBtnW, layerBtnH, 4, 200, 50, 50, 255);
                drawTextTTF(rnd, backBtnX + 10, layerBtnY + (layerBtnH - textHeightTTF()) / 2, "<<", 255, 255, 255, 255);
                
                {
                    int fx=infoX+fieldW2+(int)(25*L.s),fw=fieldW2-(int)(20*L.s); bool editing=(sprInfoEdit.field==4);
                    drawTextTTF(rnd, infoX+fieldW2,row3Y+(fieldH2-textHeightTTF())/2,"Dir:",80,80,80,255);
                    fillRoundedRect(rnd,fx,row3Y,fw,fieldH2,4,editing?255:245,255,editing?220:245,255);
                    if(editing) drawRoundedRectOutline(rnd,fx-1,row3Y-1,fw+2,fieldH2+2,4,50,150,255,255);
                    string val=editing?sprInfoEdit.buffer:floatToString(sp.direction);
                    drawTextTTF(rnd, fx+4,row3Y+(fieldH2-textHeightTTF())/2,val.c_str(),0,0,0,255);
                    if(editing){int cw=textWidthTTF(val.c_str());SDL_SetRenderDrawColor(rnd,0,0,0,255);SDL_RenderDrawLine(rnd,fx+4+cw,row3Y+2,fx+4+cw,row3Y+fieldH2-2);}
                }
            }
        }
        
        if(gBackdropEditMode) {
            int panelX = L.PALETTE_WIDTH + L.STAGE_WIDTH + 20;
            int panelY = L.TOOLBAR_HEIGHT + 20;
            int panelW = 250, panelH = 300;

            SDL_SetRenderDrawColor(rnd, 60, 60, 80, 255);
            SDL_Rect panelBg = {panelX, panelY, panelW, panelH};
            SDL_RenderFillRect(rnd, &panelBg);

            drawTextTTF(rnd, panelX+10, panelY+10, "Backdrop Settings", 255,255,255,255);

            int btnY = panelY + 50;
            for(int i=0; i<gBackdropLibraryCount; i++) {
                fillRoundedRect(rnd, panelX+10, btnY, panelW-20, 35, 4, 80,150,80,255);
                drawTextTTF(rnd, panelX+20, btnY+10, gBackdropItems[i].name, 255,255,255,255);
                btnY += 45;
            }

            int uploadBtnY = btnY + 20;
            fillRoundedRect(rnd, panelX+10, uploadBtnY, panelW-20, 35, 4, 50,100,150,255);
            drawTextTTF(rnd, panelX+30, uploadBtnY+10, "Upload Image", 255,255,255,255);

            int closeBtnY = uploadBtnY + 50;
            fillRoundedRect(rnd, panelX+10, closeBtnY, panelW-20, 35, 4, 150,50,50,255);
            drawTextTTF(rnd, panelX+80, closeBtnY+10, "Close", 255,255,255,255);
        }

        
        {
            int wsX=L.PALETTE_WIDTH+L.STAGE_WIDTH, wsY=L.TOOLBAR_HEIGHT;
            int wsW=winW-wsX, wsH=winH-wsY;
            SDL_SetRenderDrawColor(rnd,245,245,250,255);
            SDL_Rect wsRect={wsX,wsY,wsW,wsH}; SDL_RenderFillRect(rnd,&wsRect);
            SDL_SetRenderDrawColor(rnd,230,230,235,255);
            for(int gx=wsX;gx<wsX+wsW;gx+=40) SDL_RenderDrawLine(rnd,gx,wsY,gx,wsY+wsH);
            for(int gy=wsY;gy<wsY+wsH;gy+=40) SDL_RenderDrawLine(rnd,wsX,gy,wsX+wsW,gy);
            drawTextTTF(rnd, wsX+10,wsY+5,"Code Workspace",150,150,160,255);
        }
        if(costumeEditMode && costumeCanvas) {
            int editorX = L.PALETTE_WIDTH + L.STAGE_WIDTH + 20;
            int editorY = L.TOOLBAR_HEIGHT + 20;
            int editorW = canvasW + 40;
            int editorH = canvasH + 100;

            SDL_SetRenderDrawColor(rnd, 60, 60, 80, 255);
            SDL_Rect editorBg = {editorX, editorY, editorW, editorH};
            SDL_RenderFillRect(rnd, &editorBg);

            SDL_Rect canvasRect = {editorX+20, editorY+20, canvasW, canvasH};
            SDL_RenderCopy(rnd, costumeCanvas, nullptr, &canvasRect);

            int toolbarY = editorY + canvasH + 30;

            fillRoundedRect(rnd, editorX+20, toolbarY, 60, 30, 4, 50,150,50,255);
            drawTextTTF(rnd, editorX+30, toolbarY+5, "Pen", 255,255,255,255);
            int colorBtnY = toolbarY + 40;
            int colorBtnW = 25, colorBtnH = 25;

            fillRoundedRect(rnd, editorX+20, colorBtnY, colorBtnW, colorBtnH, 4, 255,0,0,255);
            fillRoundedRect(rnd, editorX+50, colorBtnY, colorBtnW, colorBtnH, 4, 0,255,0,255);
            fillRoundedRect(rnd, editorX+80, colorBtnY, colorBtnW, colorBtnH, 4, 0,0,255,255);
            fillRoundedRect(rnd, editorX+110, colorBtnY, colorBtnW, colorBtnH, 4, 0,0,0,255);
            fillRoundedRect(rnd, editorX+140, colorBtnY, colorBtnW, colorBtnH, 4, 255,255,255,255);

            fillRoundedRect(rnd, editorX+90, toolbarY, 60, 30, 4, 150,50,50,255);
            drawTextTTF(rnd, editorX+100, toolbarY+5, "Erase", 255,255,255,255);

            fillRoundedRect(rnd, editorX+160, toolbarY, 60, 30, 4, 50,50,150,255);
            drawTextTTF(rnd, editorX+170, toolbarY+5, "Save", 255,255,255,255);

            fillRoundedRect(rnd, editorX+230, toolbarY, 60, 30, 4, 100,100,100,255);
            drawTextTTF(rnd, editorX+240, toolbarY+5, "Exit", 255,255,255,255);
        }

        
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
        renderBackdropPanel(rnd);

        SDL_RenderPresent(rnd);
        SDL_Delay(16);
    } 

    SDL_StopTextInput();
    SDL_DestroyRenderer(rnd);
    SDL_DestroyWindow(window);
    IMG_Quit();
    closeFonts();
    for(auto& sp : sprites){
        if(sp.uploadedTexture) SDL_DestroyTexture(sp.uploadedTexture);
    }
    if(gBackdropTexture) SDL_DestroyTexture(gBackdropTexture);
    if(costumeCanvas) SDL_DestroyTexture(costumeCanvas);

    SDL_Quit();
    return 0;
}