#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <bits/stdc++.h>
using namespace std;

static const int BASE_WIDTH = 1280;
static const int BASE_HEIGHT = 720;
static const float BASE_BLOCK_WIDTH = 200.0f;
static const float BASE_BLOCK_HEIGHT = 50.0f;
static const float BASE_CBLOCK_MIN_H = 90.0f;
static const float BASE_CBLOCK_MOUTH_H = 40.0f;
static const float BASE_CBLOCK_BAR_H = 20.0f;
static const float BASE_BLOCK_CORNER_R = 8.0f;
static const int BASE_TOOLBAR_HEIGHT = 45;
static const int BASE_PALETTE_WIDTH = 360;
static const int BASE_CAT_BTN_HEIGHT = 40;
static const int BASE_CAT_BTN_WIDTH = 120;
static const int BASE_CAT_PANEL_WIDTH = 130;
static const int BASE_STAGE_WIDTH = 360;
static const int BASE_STAGE_HEIGHT = 270;
static const int BASE_SPRITE_THUMB = 70;

static TTF_Font* gFontSmall = nullptr;
static TTF_Font* gFontNormal = nullptr;
static TTF_Font* gFontLarge = nullptr;
static int gFontSizeNormal = 13;
static string gFontPath = "DejaVuSans.ttf";
static SDL_Renderer* rnd = nullptr;

struct LayoutScale {
    float sx, sy, s;
    int winW, winH;
    int TOOLBAR_HEIGHT;
    int PALETTE_WIDTH;
    int CAT_PANEL_WIDTH;
    int CAT_BTN_HEIGHT;
    int CAT_BTN_WIDTH;
    int STAGE_WIDTH;
    int STAGE_HEIGHT;
    int SPRITE_THUMB;
    float BLOCK_WIDTH;
    float BLOCK_HEIGHT;
    float CBLOCK_MIN_H;
    float CBLOCK_MOUTH_H;
    float CBLOCK_BAR_H;
    float BLOCK_CORNER_R;
    int fontScale;

    void update(int w, int h) {
        winW = w; winH = h;
        sx = (float)w / BASE_WIDTH;
        sy = (float)h / BASE_HEIGHT;
        s = min(sx, sy);
        if (s < 1.0f) s = 1.0f;
        TOOLBAR_HEIGHT = (int)(BASE_TOOLBAR_HEIGHT * sy);
        PALETTE_WIDTH = (int)(BASE_PALETTE_WIDTH * sx);
        CAT_PANEL_WIDTH = (int)(BASE_CAT_PANEL_WIDTH * sx);
        CAT_BTN_HEIGHT = (int)(BASE_CAT_BTN_HEIGHT * sy);
        CAT_BTN_WIDTH = (int)(BASE_CAT_BTN_WIDTH * sx);
        STAGE_WIDTH = (int)(BASE_STAGE_WIDTH * sx);
        STAGE_HEIGHT = (int)(BASE_STAGE_HEIGHT * sy);
        SPRITE_THUMB = (int)(BASE_SPRITE_THUMB * s);
        float fontFactor = gFontSizeNormal / 13.0f;
        BLOCK_WIDTH = BASE_BLOCK_WIDTH * s * fontFactor;
        BLOCK_HEIGHT = BASE_BLOCK_HEIGHT * s * fontFactor;
        CBLOCK_MIN_H = BASE_CBLOCK_MIN_H * s * fontFactor;
        CBLOCK_MOUTH_H = BASE_CBLOCK_MOUTH_H * s * fontFactor;
        CBLOCK_BAR_H = BASE_CBLOCK_BAR_H * s * fontFactor;
        BLOCK_CORNER_R = BASE_BLOCK_CORNER_R * s;
        fontScale = max(1, (int)(s * 1.0f));
    }
} L;

enum Category { MOTION, LOOKS, SOUND, EVENTS, CONTROL, SENSING, OPERATORS, VARIABLES, PEN };
static const int NUM_CATEGORIES = 9;

static SDL_Color catColor(Category c) {
    switch (c) {
        case MOTION:    return {100,160,240,255};
        case LOOKS:     return {180,100,220,255};
        case SOUND:     return {220,100,170,255};
        case EVENTS:    return {230,180,0,255};
        case CONTROL:   return {230,160,0,255};
        case SENSING:   return {80,180,220,255};
        case OPERATORS: return {80,200,80,255};
        case VARIABLES: return {230,120,0,255};
        case PEN:       return {0,180,120,255};
    }
    return {128,128,128,255};
}

static const char* catName(Category c) {
    switch (c) {
        case MOTION:    return "Motion";
        case LOOKS:     return "Looks";
        case SOUND:     return "Sound";
        case EVENTS:    return "Events";
        case CONTROL:   return "Control";
        case SENSING:   return "Sensing";
        case OPERATORS: return "Operators";
        case VARIABLES: return "Variables";
        case PEN:       return "Pen";
    }
    return "?";
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
    int spriteOwner;
    vector<InputField> inputs;
    vector<OperatorSlot> opSlots;
};

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
};

static vector<Sprite> gSprites;
static int gSelectedSprite = 0;
static int gNextSpriteNum = 2;
static vector<Block> gBlocks;
static int gNextBlockId = 1000;
static Category gSelectedCategory = MOTION;
static bool gIsRunning = false;
static float gTimer = 0;

static int gDragBlockId = -1;
static float gDragOffX = 0, gDragOffY = 0;
static bool gDragging = false;

struct ActiveEdit {
    int blockId;
    int fieldIndex;
    bool active;
    string buffer;
    int cursorPos;
} gEdit = {-1, -1, false, "", 0};

static Sprite createDefaultSprite(const char* name, float x, float y, SDL_Color col) {
    Sprite sp;
    sp.name = name;
    sp.x = x; sp.y = y;
    sp.direction = 90;
    sp.size = 100;
    sp.visible = true;
    sp.selected = false;
    sp.color = col;
    sp.sayText = "";
    sp.sayTimer = 0;
    return sp;
}

static void fillRoundedRect(SDL_Renderer* r, int x, int y, int w, int h, int rad,
                             Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    SDL_Rect center = {x + rad, y, w - 2 * rad, h};
    SDL_RenderFillRect(r, &center);
    SDL_Rect left = {x, y + rad, rad, h - 2 * rad};
    SDL_RenderFillRect(r, &left);
    SDL_Rect right = {x + w - rad, y + rad, rad, h - 2 * rad};
    SDL_RenderFillRect(r, &right);
    for (int cy2 = -rad; cy2 <= rad; cy2++) {
        int cx2 = (int)sqrt((double)(rad * rad - cy2 * cy2));
        SDL_RenderDrawLine(r, x + rad - cx2, y + rad + cy2, x + rad, y + rad + cy2);
        SDL_RenderDrawLine(r, x + w - rad, y + rad + cy2, x + w - rad + cx2, y + rad + cy2);
        SDL_RenderDrawLine(r, x + rad - cx2, y + h - rad + cy2, x + rad, y + h - rad + cy2);
        SDL_RenderDrawLine(r, x + w - rad, y + h - rad + cy2, x + w - rad + cx2, y + h - rad + cy2);
    }
}

static void fillEllipse(SDL_Renderer* r, int cx, int cy, int rx, int ry,
                          Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca) {
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    for (int y2 = -ry; y2 <= ry; y2++) {
        int x2 = (int)(rx * sqrt(1.0 - (double)(y2 * y2) / (double)(ry * ry)));
        SDL_RenderDrawLine(r, cx - x2, cy + y2, cx + x2, cy + y2);
    }
}

static int textWidthTTF(const char* txt) {
    if (!gFontNormal || !txt) return 0;
    int w = 0, h = 0;
    TTF_SizeText(gFontNormal, txt, &w, &h);
    return w;
}

static int textHeightTTF() {
    if (!gFontNormal) return 14;
    return TTF_FontHeight(gFontNormal);
}

static void drawTextTTF(SDL_Renderer* r, int x, int y, const char* txt,
                          Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca, TTF_Font* font = nullptr) {
    if (!txt || strlen(txt) == 0) return;
    TTF_Font* f = font ? font : gFontNormal;
    if (!f) return;
    SDL_Color col = {cr, cg, cb, ca};
    SDL_Surface* surf = TTF_RenderText_Blended(f, txt, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(r, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static string intToString(int v) {
    char buf[32]; snprintf(buf, 32, "%d", v); return string(buf);
}

static string floatToString(float v) {
    char buf[32]; snprintf(buf, 32, "%.1f", v); return string(buf);
}

static InputField makeInput(float rx, float ry, float w, float h, const char* def) {
    InputField inp;
    inp.relX = rx; inp.relY = ry;
    inp.width = w; inp.height = h;
    inp.value = def; inp.defaultVal = def;
    inp.editing = false;
    return inp;
}

static OperatorSlot makeOpSlot(float rx, float ry, float w, float h) {
    OperatorSlot sl;
    sl.relX = rx; sl.relY = ry;
    sl.width = w; sl.height = h;
    sl.embeddedBlockId = -1;
    return sl;
}

static Block makeBlock(int id, Category cat, BlockShape shape, const char* text,
                        float x, float y, bool inPalette,
                        vector<InputField> inputs, vector<OperatorSlot> ops) {
    Block b;
    b.id = id;
    b.cat = cat;
    b.shape = shape;
    b.text = text;
    b.x = x; b.y = y;
    b.w = L.BLOCK_WIDTH;
    b.h = (shape == C_BLOCK) ? L.CBLOCK_MIN_H : L.BLOCK_HEIGHT;
    b.inPalette = inPalette;
    b.nextBlockId = -1;
    b.parentBlockId = -1;
    b.childHeadId = -1;
    b.spriteOwner = -1;
    b.inputs = inputs;
    b.opSlots = ops;
    return b;
}

static vector<Block> buildPaletteBlocks() {
    vector<Block> blocks;
    int id = 0;
    float bw = L.BLOCK_WIDTH, bh = L.BLOCK_HEIGHT;
    float fieldW = bw * 0.18f, fieldH = bh * 0.6f;

    blocks.push_back(makeBlock(id++, MOTION, HAT, "when green flag clicked", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "move  steps", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "10")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "turn right  deg", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "15")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "turn left  deg", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "15")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "go to x:  y:", 0,0,true,
        {makeInput(bw*0.35f, bh*0.15f, fieldW, fieldH, "0"),
         makeInput(bw*0.65f, bh*0.15f, fieldW, fieldH, "0")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "glide  secs to x:  y:", 0,0,true,
        {makeInput(bw*0.25f, bh*0.15f, fieldW, fieldH, "1"),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "0"),
         makeInput(bw*0.78f, bh*0.15f, fieldW, fieldH, "0")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "set x to", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "0")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "set y to", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "0")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "change x by", 0,0,true,
        {makeInput(bw*0.6f, bh*0.15f, fieldW, fieldH, "10")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "change y by", 0,0,true,
        {makeInput(bw*0.6f, bh*0.15f, fieldW, fieldH, "10")}, {}));
    blocks.push_back(makeBlock(id++, MOTION, COMMAND, "point in direction", 0,0,true,
        {makeInput(bw*0.7f, bh*0.15f, fieldW, fieldH, "90")}, {}));

    blocks.push_back(makeBlock(id++, LOOKS, COMMAND, "say  for  secs", 0,0,true,
        {makeInput(bw*0.2f, bh*0.15f, fieldW*1.5f, fieldH, "Hello!"),
         makeInput(bw*0.65f, bh*0.15f, fieldW, fieldH, "2")}, {}));
    blocks.push_back(makeBlock(id++, LOOKS, COMMAND, "say", 0,0,true,
        {makeInput(bw*0.3f, bh*0.15f, fieldW*1.5f, fieldH, "Hello!")}, {}));
    blocks.push_back(makeBlock(id++, LOOKS, COMMAND, "show", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, LOOKS, COMMAND, "hide", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, LOOKS, COMMAND, "set size to  %", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "100")}, {}));
    blocks.push_back(makeBlock(id++, LOOKS, COMMAND, "change size by", 0,0,true,
        {makeInput(bw*0.65f, bh*0.15f, fieldW, fieldH, "10")}, {}));

    blocks.push_back(makeBlock(id++, SOUND, COMMAND, "play sound", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, SOUND, COMMAND, "stop all sounds", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, SOUND, COMMAND, "set volume to  %", 0,0,true,
        {makeInput(bw*0.6f, bh*0.15f, fieldW, fieldH, "100")}, {}));

    blocks.push_back(makeBlock(id++, EVENTS, HAT, "when green flag clicked", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, EVENTS, HAT, "when space key pressed", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, EVENTS, HAT, "when this sprite clicked", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, EVENTS, COMMAND, "broadcast", 0,0,true,
        {makeInput(bw*0.5f, bh*0.15f, fieldW*1.2f, fieldH, "msg1")}, {}));

    blocks.push_back(makeBlock(id++, CONTROL, COMMAND, "wait  secs", 0,0,true,
        {makeInput(bw*0.5f, bh*0.15f, fieldW, fieldH, "1")}, {}));
    blocks.push_back(makeBlock(id++, CONTROL, C_BLOCK, "repeat", 0,0,true,
        {makeInput(bw*0.4f, bh*0.05f, fieldW, fieldH, "10")}, {}));
    blocks.push_back(makeBlock(id++, CONTROL, C_BLOCK, "forever", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, CONTROL, C_BLOCK, "if  then", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, CONTROL, CAP, "stop all", 0,0,true, {}, {}));

    blocks.push_back(makeBlock(id++, SENSING, REPORTER, "mouse x", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, SENSING, REPORTER, "mouse y", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, SENSING, BOOLEAN, "mouse down?", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, SENSING, BOOLEAN, "key  pressed?", 0,0,true,
        {makeInput(bw*0.3f, bh*0.15f, fieldW, fieldH, "space")}, {}));
    blocks.push_back(makeBlock(id++, SENSING, REPORTER, "timer", 0,0,true, {}, {}));

    blocks.push_back(makeBlock(id++, OPERATORS, REPORTER, "  +  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, REPORTER, "  -  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, REPORTER, "  *  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, REPORTER, "  /  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, REPORTER, "pick rand  to ", 0,0,true,
        {makeInput(bw*0.4f, bh*0.15f, fieldW, fieldH, "1"),
         makeInput(bw*0.7f, bh*0.15f, fieldW, fieldH, "10")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, BOOLEAN, "  <  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, BOOLEAN, "  =  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, BOOLEAN, "  >  ", 0,0,true,
        {makeInput(bw*0.1f, bh*0.15f, fieldW, fieldH, ""),
         makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "")}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, BOOLEAN, " and ", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, BOOLEAN, " or ", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, OPERATORS, BOOLEAN, "not ", 0,0,true, {}, {}));

    blocks.push_back(makeBlock(id++, VARIABLES, REPORTER, "my variable", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, VARIABLES, COMMAND, "set var to ", 0,0,true,
        {makeInput(bw*0.55f, bh*0.15f, fieldW, fieldH, "0")}, {}));
    blocks.push_back(makeBlock(id++, VARIABLES, COMMAND, "change var by ", 0,0,true,
        {makeInput(bw*0.6f, bh*0.15f, fieldW, fieldH, "1")}, {}));

    blocks.push_back(makeBlock(id++, PEN, COMMAND, "erase all", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, PEN, COMMAND, "stamp", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, PEN, COMMAND, "pen down", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, PEN, COMMAND, "pen up", 0,0,true, {}, {}));
    blocks.push_back(makeBlock(id++, PEN, COMMAND, "set pen color to", 0,0,true,
        {makeInput(bw*0.7f, bh*0.15f, fieldW, fieldH, "#0000FF")}, {}));
    blocks.push_back(makeBlock(id++, PEN, COMMAND, "set pen size to", 0,0,true,
        {makeInput(bw*0.65f, bh*0.15f, fieldW, fieldH, "1")}, {}));
    blocks.push_back(makeBlock(id++, PEN, COMMAND, "change pen size by", 0,0,true,
        {makeInput(bw*0.7f, bh*0.15f, fieldW, fieldH, "1")}, {}));

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
        childrenH += (child->shape == C_BLOCK) ? calcCBlockHeight(blocks, *child) : child->h;
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
        if (child->shape == C_BLOCK) {
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
    b.nextBlockId = -1;
    b.parentBlockId = -1;
    b.childHeadId = -1;
    b.spriteOwner = gSelectedSprite;
    for (auto& inp : b.inputs) inp.editing = false;
    for (auto& sl : b.opSlots) sl.embeddedBlockId = -1;
    return b;
}

static void drawBlock(SDL_Renderer* r, Block& b, vector<Block>& allBlocks, bool highlight = false) {
    SDL_Color col = catColor(b.cat);
    Uint8 cr = col.r, cg = col.g, cb2 = col.b;
    if (highlight) { cr = min(255, cr + 40); cg = min(255, cg + 40); cb2 = min(255, cb2 + 40); }
    int bx = (int)b.x, by = (int)b.y, bw = (int)b.w, bh = (int)b.h, rd = (int)L.BLOCK_CORNER_R;

    switch (b.shape) {
    case COMMAND:
    case CAP:
        fillRoundedRect(r, bx, by, bw, bh, rd, cr, cg, cb2, 255);
        { SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
          SDL_Rect notch = {bx + (int)(20*L.s), by - (int)(4*L.s), (int)(30*L.s), (int)(4*L.s)};
          SDL_RenderFillRect(r, &notch); }
        if (b.shape != CAP) {
            SDL_Rect notchB = {bx + (int)(20*L.s), by + bh, (int)(30*L.s), (int)(4*L.s)};
            SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
            SDL_RenderFillRect(r, &notchB);
        }
        break;
    case HAT:
        fillRoundedRect(r, bx, by + (int)(10*L.s), bw, bh - (int)(10*L.s), rd, cr, cg, cb2, 255);
        fillEllipse(r, bx + bw/2, by + (int)(10*L.s), bw/2, (int)(12*L.s), cr, cg, cb2, 255);
        { SDL_Rect notchB = {bx + (int)(20*L.s), by + bh, (int)(30*L.s), (int)(4*L.s)};
          SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
          SDL_RenderFillRect(r, &notchB); }
        break;
    case C_BLOCK: {
        float barH = L.CBLOCK_BAR_H, indent = 20 * L.s;
        fillRoundedRect(r, bx, by, bw, (int)barH, rd, cr, cg, cb2, 255);
        { SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
          SDL_Rect notch = {bx + (int)(20*L.s), by - (int)(4*L.s), (int)(30*L.s), (int)(4*L.s)};
          SDL_RenderFillRect(r, &notch); }
        float mouthTop = by + barH;
        float mouthBot = by + b.h - barH;
        SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
        SDL_Rect leftBar = {bx, (int)mouthTop, (int)indent, (int)(mouthBot - mouthTop)};
        SDL_RenderFillRect(r, &leftBar);
        { SDL_Rect innerNotch = {bx + (int)indent + (int)(20*L.s), (int)mouthTop, (int)(30*L.s), (int)(4*L.s)};
          SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
          SDL_RenderFillRect(r, &innerNotch); }
        fillRoundedRect(r, bx, (int)mouthBot, bw, (int)barH, rd, cr, cg, cb2, 255);
        { SDL_Rect notchB = {bx + (int)(20*L.s), by + (int)b.h, (int)(30*L.s), (int)(4*L.s)};
          SDL_SetRenderDrawColor(r, cr, cg, cb2, 255);
          SDL_RenderFillRect(r, &notchB); }
        int cid = b.childHeadId;
        while (cid >= 0) {
            Block* child = findBlock(allBlocks, cid);
            if (!child) break;
            drawBlock(r, *child, allBlocks, false);
            cid = child->nextBlockId;
        }
    } break;
    case REPORTER:
        fillRoundedRect(r, bx, by, bw, bh, bh/2, cr, cg, cb2, 255);
        break;
    case BOOLEAN:
        fillRoundedRect(r, bx, by, bw, bh, bh/2, cr, cg, cb2, 255);
        { SDL_SetRenderDrawColor(r, 255, 255, 255, 60);
          int cx = bx + bh/4, cy2 = by + bh/4, sz = bh/2;
          for (int i = 0; i < sz/2; i++) {
              SDL_RenderDrawLine(r, cx + i, cy2 + sz/2 - i, cx + i, cy2 + sz/2 + i);
          }
        }
        break;
    }

    int textX = bx + (int)(8 * L.s);
    int textY = by + (bh - textHeightTTF()) / 2;
    if (b.shape == C_BLOCK) textY = by + ((int)L.CBLOCK_BAR_H - textHeightTTF()) / 2;
    drawTextTTF(r, textX, textY, b.text.c_str(), 255, 255, 255, 255);

    for (auto& inp : b.inputs) {
        int fx = bx + (int)inp.relX, fy = by + (int)inp.relY;
        int fw = (int)inp.width, fh = (int)inp.height;
        fillRoundedRect(r, fx, fy, fw, fh, 4, 255, 255, 255, 220);
        if (inp.editing) {
            SDL_SetRenderDrawColor(r, 80, 80, 255, 255);
            SDL_Rect border = {fx-1, fy-1, fw+2, fh+2};
            SDL_RenderDrawRect(r, &border);
        }
        string display = inp.value.empty() ? inp.defaultVal : inp.value;
        drawTextTTF(r, fx + 3, fy + (fh - textHeightTTF()) / 2, display.c_str(), 40, 40, 40, 255);
    }
}

static void drawSpriteOnStage(SDL_Renderer* r, Sprite& sp, int stageX, int stageY,
                                int stageW, int stageH) {
    if (!sp.visible) return;
    float scale = sp.size / 100.0f;
    int sz = (int)(30 * scale);
    int sx = stageX + stageW/2 + (int)sp.x - sz/2;
    int sy = stageY + stageH/2 - (int)sp.y - sz/2;

    fillRoundedRect(r, sx, sy, sz, sz, sz/4, sp.color.r, sp.color.g, sp.color.b, 255);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
    int eyeOff = sz / 4;
    int eyeSz = max(2, sz / 8);
    SDL_Rect eye1 = {sx + eyeOff, sy + eyeOff, eyeSz, eyeSz};
    SDL_Rect eye2 = {sx + sz - eyeOff - eyeSz, sy + eyeOff, eyeSz, eyeSz};
    SDL_RenderFillRect(r, &eye1);
    SDL_RenderFillRect(r, &eye2);

    if (!sp.sayText.empty() && sp.sayTimer > 0) {
        int tw = textWidthTTF(sp.sayText.c_str()) + 12;
        int th = textHeightTTF() + 8;
        int bx = sx + sz/2 - tw/2;
        int by2 = sy - th - 8;
        fillRoundedRect(r, bx, by2, tw, th, 6, 255, 255, 255, 240);
        SDL_SetRenderDrawColor(r, 100, 100, 100, 255);
        SDL_Rect border = {bx, by2, tw, th};
        SDL_RenderDrawRect(r, &border);
        drawTextTTF(r, bx + 6, by2 + 4, sp.sayText.c_str(), 30, 30, 30, 255);
    }

    if (sp.selected) {
        SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
        SDL_Rect sel = {sx - 2, sy - 2, sz + 4, sz + 4};
        SDL_RenderDrawRect(r, &sel);
    }
}

static void drawGrid(SDL_Renderer* r, int x, int y, int w, int h) {
    SDL_SetRenderDrawColor(r, 220, 220, 230, 255);
    int gap = (int)(20 * L.s);
    if (gap < 8) gap = 8;
    for (int gx = x; gx < x + w; gx += gap) {
        for (int gy = y; gy < y + h; gy += gap) {
            SDL_RenderDrawPoint(r, gx, gy);
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("Scratch Clone",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        BASE_WIDTH, BASE_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    rnd = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);

    int winW = BASE_WIDTH, winH = BASE_HEIGHT;
    L.update(winW, winH);

    gFontSmall = TTF_OpenFont(gFontPath.c_str(), (int)(11 * L.s));
    gFontNormal = TTF_OpenFont(gFontPath.c_str(), (int)(gFontSizeNormal * L.s));
    gFontLarge = TTF_OpenFont(gFontPath.c_str(), (int)(18 * L.s));

    if (!gFontNormal) {
        const char* fallbacks[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                    "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
                                    "C:\\Windows\\Fonts\\arial.ttf"};
        for (auto& fb : fallbacks) {
            gFontNormal = TTF_OpenFont(fb, (int)(gFontSizeNormal * L.s));
            if (gFontNormal) {
                gFontSmall = TTF_OpenFont(fb, (int)(11 * L.s));
                gFontLarge = TTF_OpenFont(fb, (int)(18 * L.s));
                gFontPath = fb;
                break;
            }
        }
    }

    gSprites.push_back(createDefaultSprite("Sprite1", 0, 0, {100, 160, 240, 255}));
    gBlocks = buildPaletteBlocks();

    bool running = true;
    SDL_Event ev;
    Uint32 lastTick = SDL_GetTicks();

    while (running) {
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTick) / 1000.0f;
        lastTick = now;
        gTimer += dt;

        for (auto& sp : gSprites) {
            if (sp.sayTimer > 0) {
                sp.sayTimer -= dt;
                if (sp.sayTimer <= 0) { sp.sayTimer = 0; sp.sayText = ""; }
            }
        }

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;

            if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                winW = ev.window.data1;
                winH = ev.window.data2;
                L.update(winW, winH);
            }

            if (ev.type == SDL_KEYDOWN) {
                if (gEdit.active) {
                    if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_ESCAPE) {
                        Block* eb = findBlock(gBlocks, gEdit.blockId);
                        if (eb && gEdit.fieldIndex >= 0 && gEdit.fieldIndex < (int)eb->inputs.size()) {
                            eb->inputs[gEdit.fieldIndex].value = gEdit.buffer;
                            eb->inputs[gEdit.fieldIndex].editing = false;
                        }
                        gEdit.active = false;
                    } else if (ev.key.keysym.sym == SDLK_BACKSPACE && !gEdit.buffer.empty()) {
                        gEdit.buffer.pop_back();
                        gEdit.cursorPos = max(0, gEdit.cursorPos - 1);
                    }
                } else {
                    if (ev.key.keysym.sym == SDLK_LEFT && gSelectedSprite < (int)gSprites.size()) {
                        gSprites[gSelectedSprite].x -= 10;
                    }
                    if (ev.key.keysym.sym == SDLK_RIGHT && gSelectedSprite < (int)gSprites.size()) {
                        gSprites[gSelectedSprite].x += 10;
                    }
                    if (ev.key.keysym.sym == SDLK_UP && gSelectedSprite < (int)gSprites.size()) {
                        gSprites[gSelectedSprite].y += 10;
                    }
                    if (ev.key.keysym.sym == SDLK_DOWN && gSelectedSprite < (int)gSprites.size()) {
                        gSprites[gSelectedSprite].y -= 10;
                    }
                }
            }

            if (ev.type == SDL_TEXTINPUT && gEdit.active) {
                gEdit.buffer += ev.text.text;
                gEdit.cursorPos += (int)strlen(ev.text.text);
            }

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;

                int stageX = winW - L.STAGE_WIDTH;
                int stageY = L.TOOLBAR_HEIGHT;
                int sprPanelY = stageY + L.STAGE_HEIGHT + 10;
                int thumbSz = L.SPRITE_THUMB;

                int addBtnX = stageX + 10 + (int)gSprites.size() * (thumbSz + 8);
                int addBtnY = sprPanelY + 5;
                if (mx >= addBtnX && mx <= addBtnX + thumbSz && my >= addBtnY && my <= addBtnY + thumbSz) {
                    SDL_Color colors[] = {{240,80,80,255},{80,200,80,255},{200,80,200,255},
                                           {240,180,0,255},{80,180,220,255}};
                    SDL_Color c = colors[gSprites.size() % 5];
                    string nm = "Sprite" + intToString(gNextSpriteNum++);
                    gSprites.push_back(createDefaultSprite(nm.c_str(), (float)(rand()%200-100),
                                                            (float)(rand()%150-75), c));
                    gSelectedSprite = (int)gSprites.size() - 1;
                    continue;
                }

                for (int i = 0; i < (int)gSprites.size(); i++) {
                    int tx = stageX + 10 + i * (thumbSz + 8);
                    int ty = sprPanelY + 5;
                    if (mx >= tx && mx <= tx + thumbSz && my >= ty && my <= ty + thumbSz) {
                        int delX = tx + thumbSz - 14, delY = ty + 2;
                        if (mx >= delX && mx <= delX + 12 && my >= delY && my <= delY + 12 && gSprites.size() > 1) {
                            gBlocks.erase(remove_if(gBlocks.begin(), gBlocks.end(),
                                [i](const Block& b) { return !b.inPalette && b.spriteOwner == i; }), gBlocks.end());
                            gSprites.erase(gSprites.begin() + i);
                            for (auto& b : gBlocks) {
                                if (!b.inPalette && b.spriteOwner > i) b.spriteOwner--;
                            }
                            if (gSelectedSprite >= (int)gSprites.size()) gSelectedSprite = (int)gSprites.size() - 1;
                        } else {
                            gSelectedSprite = i;
                        }
                        continue;
                    }
                }

                int catPanelX = 0, catPanelY = L.TOOLBAR_HEIGHT;
                for (int i = 0; i < NUM_CATEGORIES; i++) {
                    int cy = catPanelY + i * L.CAT_BTN_HEIGHT;
                    if (mx >= catPanelX && mx < catPanelX + L.CAT_PANEL_WIDTH &&
                        my >= cy && my < cy + L.CAT_BTN_HEIGHT) {
                        gSelectedCategory = (Category)i;
                    }
                }

                gDragBlockId = -1;
                for (int i = (int)gBlocks.size() - 1; i >= 0; i--) {
                    Block& b = gBlocks[i];
                    if (mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h) {
                        bool clickedInput = false;
                        for (int fi = 0; fi < (int)b.inputs.size(); fi++) {
                            auto& inp = b.inputs[fi];
                            int fx = (int)(b.x + inp.relX), fy = (int)(b.y + inp.relY);
                            if (mx >= fx && mx <= fx + (int)inp.width &&
                                my >= fy && my <= fy + (int)inp.height && !b.inPalette) {
                                if (gEdit.active) {
                                    Block* eb = findBlock(gBlocks, gEdit.blockId);
                                    if (eb && gEdit.fieldIndex >= 0 && gEdit.fieldIndex < (int)eb->inputs.size()) {
                                        eb->inputs[gEdit.fieldIndex].value = gEdit.buffer;
                                        eb->inputs[gEdit.fieldIndex].editing = false;
                                    }
                                }
                                gEdit.blockId = b.id;
                                gEdit.fieldIndex = fi;
                                gEdit.active = true;
                                gEdit.buffer = inp.value;
                                gEdit.cursorPos = (int)inp.value.size();
                                inp.editing = true;
                                clickedInput = true;
                                break;
                            }
                        }
                        if (clickedInput) break;

                        if (gEdit.active) {
                            Block* eb = findBlock(gBlocks, gEdit.blockId);
                            if (eb && gEdit.fieldIndex >= 0 && gEdit.fieldIndex < (int)eb->inputs.size()) {
                                eb->inputs[gEdit.fieldIndex].value = gEdit.buffer;
                                eb->inputs[gEdit.fieldIndex].editing = false;
                            }
                            gEdit.active = false;
                        }

                        if (b.inPalette) {
                            Block nb = cloneBlock(b, (float)mx - b.w/2, (float)my - b.h/2);
                            gBlocks.push_back(nb);
                            gDragBlockId = nb.id;
                            gDragOffX = b.w / 2;
                            gDragOffY = b.h / 2;
                        } else {
                            gDragBlockId = b.id;
                            gDragOffX = mx - b.x;
                            gDragOffY = my - b.y;
                        }
                        gDragging = true;
                        break;
                    }
                }
            }

            if (ev.type == SDL_MOUSEMOTION && gDragging && gDragBlockId >= 0) {
                Block* db = findBlock(gBlocks, gDragBlockId);
                if (db) {
                    float nx = ev.motion.x - gDragOffX;
                    float ny = ev.motion.y - gDragOffY;
                    db->x = nx; db->y = ny;
                }
            }

            if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT && gDragging) {
                gDragging = false;
                gDragBlockId = -1;
            }
        }

        SDL_SetRenderDrawColor(rnd, 250, 250, 250, 255);
        SDL_RenderClear(rnd);

        SDL_SetRenderDrawColor(rnd, 55, 55, 70, 255);
        SDL_Rect toolbar = {0, 0, winW, L.TOOLBAR_HEIGHT};
        SDL_RenderFillRect(rnd, &toolbar);
        drawTextTTF(rnd, 15, (L.TOOLBAR_HEIGHT - textHeightTTF()) / 2,
                     "Scratch Clone", 255, 255, 255, 255, gFontLarge);

        int flagX = winW - L.STAGE_WIDTH - 80;
        fillRoundedRect(rnd, flagX, 5, 32, 32, 6, 50, 180, 50, 255);
        drawTextTTF(rnd, flagX + 8, 8, ">", 255, 255, 255, 255);
        fillRoundedRect(rnd, flagX + 40, 5, 32, 32, 6, 200, 60, 60, 255);
        drawTextTTF(rnd, flagX + 48, 8, "X", 255, 255, 255, 255);

        int catPanelX = 0, catPanelY = L.TOOLBAR_HEIGHT;
        int catPanelH = winH - L.TOOLBAR_HEIGHT;
        SDL_SetRenderDrawColor(rnd, 42, 42, 56, 255);
        SDL_Rect catBg = {catPanelX, catPanelY, L.CAT_PANEL_WIDTH, catPanelH};
        SDL_RenderFillRect(rnd, &catBg);

        for (int i = 0; i < NUM_CATEGORIES; i++) {
            Category c = (Category)i;
            SDL_Color cc = catColor(c);
            int cy = catPanelY + i * L.CAT_BTN_HEIGHT;
            if (c == gSelectedCategory) {
                fillRoundedRect(rnd, catPanelX + 4, cy + 2, L.CAT_PANEL_WIDTH - 8,
                                L.CAT_BTN_HEIGHT - 4, 6, cc.r, cc.g, cc.b, 255);
                drawTextTTF(rnd, catPanelX + 10, cy + (L.CAT_BTN_HEIGHT - textHeightTTF()) / 2,
                             catName(c), 255, 255, 255, 255);
            } else {
                drawTextTTF(rnd, catPanelX + 10, cy + (L.CAT_BTN_HEIGHT - textHeightTTF()) / 2,
                             catName(c), 180, 180, 200, 255);
            }
        }

        int palX = L.CAT_PANEL_WIDTH;
        int palY = L.TOOLBAR_HEIGHT;
        int palW = L.PALETTE_WIDTH - L.CAT_PANEL_WIDTH;
        int palH = winH - L.TOOLBAR_HEIGHT;
        SDL_SetRenderDrawColor(rnd, 235, 235, 240, 255);
        SDL_Rect palBg = {palX, palY, palW, palH};
        SDL_RenderFillRect(rnd, &palBg);

        float py = (float)(palY + 10);
        for (auto& b : gBlocks) {
            if (!b.inPalette) continue;
            if (b.cat != gSelectedCategory) continue;
            b.x = (float)(palX + 10);
            b.y = py;
            drawBlock(rnd, b, gBlocks, false);
            py += b.h + 8;
        }

        int wsX = L.PALETTE_WIDTH;
        int wsY = L.TOOLBAR_HEIGHT;
        int wsW = winW - L.PALETTE_WIDTH - L.STAGE_WIDTH;
        int wsH = winH - L.TOOLBAR_HEIGHT;
        SDL_SetRenderDrawColor(rnd, 245, 245, 248, 255);
        SDL_Rect wsBg = {wsX, wsY, wsW, wsH};
        SDL_RenderFillRect(rnd, &wsBg);
        drawGrid(rnd, wsX, wsY, wsW, wsH);

        for (auto& b : gBlocks) {
            if (b.inPalette) continue;
            if (b.spriteOwner != gSelectedSprite) continue;
            if (b.id == gDragBlockId) continue;
            drawBlock(rnd, b, gBlocks, false);
        }

        if (gDragging && gDragBlockId >= 0) {
            Block* db = findBlock(gBlocks, gDragBlockId);
            if (db) {
                drawBlock(rnd, *db, gBlocks, true);
            }
        }

        int stageX = winW - L.STAGE_WIDTH;
        int stageY = L.TOOLBAR_HEIGHT;
        SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
        SDL_Rect stageBg = {stageX, stageY, L.STAGE_WIDTH, L.STAGE_HEIGHT};
        SDL_RenderFillRect(rnd, &stageBg);
        SDL_SetRenderDrawColor(rnd, 180, 180, 190, 255);
        SDL_RenderDrawRect(rnd, &stageBg);

        for (auto& sp : gSprites) {
            drawSpriteOnStage(rnd, sp, stageX, stageY, L.STAGE_WIDTH, L.STAGE_HEIGHT);
        }

        int sprPanelY = stageY + L.STAGE_HEIGHT + 10;
        SDL_SetRenderDrawColor(rnd, 230, 230, 235, 255);
        SDL_Rect sprPanel = {stageX, sprPanelY, L.STAGE_WIDTH, winH - sprPanelY};
        SDL_RenderFillRect(rnd, &sprPanel);

        drawTextTTF(rnd, stageX + 5, sprPanelY - (int)(2 * L.s),
                     "Sprites:", 60, 60, 70, 255, gFontSmall);

        int thumbSz = L.SPRITE_THUMB;
        for (int i = 0; i < (int)gSprites.size(); i++) {
            int tx = stageX + 10 + i * (thumbSz + 8);
            int ty = sprPanelY + 5;

            if (i == gSelectedSprite) {
                SDL_SetRenderDrawColor(rnd, 70, 130, 240, 255);
                SDL_Rect sel = {tx - 3, ty - 3, thumbSz + 6, thumbSz + 6};
                SDL_RenderDrawRect(rnd, &sel);
                SDL_Rect sel2 = {tx - 2, ty - 2, thumbSz + 4, thumbSz + 4};
                SDL_RenderDrawRect(rnd, &sel2);
            }

            fillRoundedRect(rnd, tx, ty, thumbSz, thumbSz, 6,
                             gSprites[i].color.r, gSprites[i].color.g, gSprites[i].color.b, 200);

            int nameW = textWidthTTF(gSprites[i].name.c_str());
            drawTextTTF(rnd, tx + (thumbSz - nameW) / 2, ty + thumbSz + 2,
                         gSprites[i].name.c_str(), 50, 50, 60, 255, gFontSmall);

            if (gSprites.size() > 1) {
                int delX = tx + thumbSz - 14, delY = ty + 2;
                fillRoundedRect(rnd, delX, delY, 12, 12, 3, 200, 60, 60, 255);
                drawTextTTF(rnd, delX + 2, delY - 1, "x", 255, 255, 255, 255, gFontSmall);
            }
        }

        int addBtnX = stageX + 10 + (int)gSprites.size() * (thumbSz + 8);
        int addBtnY = sprPanelY + 5;
        fillRoundedRect(rnd, addBtnX, addBtnY, thumbSz, thumbSz, 6, 100, 180, 100, 220);
        int plusW = textWidthTTF("+");
        drawTextTTF(rnd, addBtnX + (thumbSz - plusW) / 2,
                     addBtnY + (thumbSz - textHeightTTF()) / 2, "+", 255, 255, 255, 255, gFontLarge);

        if (gSelectedSprite < (int)gSprites.size()) {
            Sprite& sel = gSprites[gSelectedSprite];
            int infoY = sprPanelY + thumbSz + 22;
            string info = sel.name + "  x:" + intToString((int)sel.x)
                          + " y:" + intToString((int)sel.y)
                          + " dir:" + intToString((int)sel.direction)
                          + " size:" + intToString((int)sel.size);
            drawTextTTF(rnd, stageX + 10, infoY, info.c_str(), 50, 50, 60, 255, gFontSmall);
        }

        if (gEdit.active) {
            Block* eb = findBlock(gBlocks, gEdit.blockId);
            if (eb && gEdit.fieldIndex >= 0 && gEdit.fieldIndex < (int)eb->inputs.size()) {
                eb->inputs[gEdit.fieldIndex].value = gEdit.buffer;
            }
        }

        SDL_RenderPresent(rnd);
        SDL_Delay(16);
    }

    if (gFontSmall) TTF_CloseFont(gFontSmall);
    if (gFontNormal) TTF_CloseFont(gFontNormal);
    if (gFontLarge) TTF_CloseFont(gFontLarge);
    TTF_Quit();
    SDL_DestroyRenderer(rnd);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
