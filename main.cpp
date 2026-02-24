// ============================================
//  Minimal Scratch Simulator - SDL2
//  Features: Block Palette, Workspace, Stage, Drag & Drop
// ============================================

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>
using namespace std;

// ============================================
//  Constants
// ============================================
static const int WIN_W = 1100;
static const int WIN_H = 700;
static const int PALETTE_H = 130;
static const int STAGE_W = 320;
static const int STAGE_H = 240;
static const int BLOCK_W = 160;
static const int BLOCK_H = 36;
static const int BLOCK_R = 6;
static const int SNAP_DIST = 25;

static SDL_Window*   gWindow   = nullptr;
static SDL_Renderer* gRenderer = nullptr;
static TTF_Font*     gFont     = nullptr;
static bool          gRunning  = true;

// ============================================
//  Category
// ============================================
enum class Category { MOTION, LOOKS, SOUND, EVENTS, CONTROL, SENSING, OPERATORS, VARIABLES, PEN };

static SDL_Color catColor(Category c) {
    switch (c) {
        case Category::MOTION:    return {66,133,244,255};
        case Category::LOOKS:     return {147,70,211,255};
        case Category::SOUND:     return {207,99,207,255};
        case Category::EVENTS:    return {230,168,34,255};
        case Category::CONTROL:   return {230,168,34,255};
        case Category::SENSING:   return {92,177,214,255};
        case Category::OPERATORS: return {89,192,89,255};
        case Category::VARIABLES: return {255,140,26,255};
        case Category::PEN:       return {14,154,108,255};
        default:                  return {128,128,128,255};
    }
}

static const char* catName(Category c) {
    switch (c) {
        case Category::MOTION:    return "Motion";
        case Category::LOOKS:     return "Looks";
        case Category::SOUND:     return "Sound";
        case Category::EVENTS:    return "Events";
        case Category::CONTROL:   return "Control";
        case Category::SENSING:   return "Sensing";
        case Category::OPERATORS: return "Operators";
        case Category::VARIABLES: return "Variables";
        case Category::PEN:       return "Pen";
        default:                  return "?";
    }
}

// ============================================
//  Block
// ============================================
enum class BlockShape { COMMAND, HAT, REPORTER, BOOLEAN, CAP };

struct Block {
    int       id;
    Category  cat;
    BlockShape shape;
    string    text;
    float     x, y;
    float     w, h;
    bool      inPalette;
    int       nextBlockId;   // بلوک بعدی (اسنپ شده)
    int       parentBlockId; // بلوک والد
};

static int gNextId = 5000;
static vector<Block> gBlocks;
static int gDragId = -1;
static float gDragOffX = 0, gDragOffY = 0;

// ============================================
//  Shape Drawing Helpers
// ============================================
static void fillRoundedRect(SDL_Renderer* r, int x, int y, int w, int h,
                            int rad, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    if (rad > h/2) rad = h/2;
    if (rad > w/2) rad = w/2;
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    // center
    SDL_Rect rc = {x+rad, y, w-2*rad, h};
    SDL_RenderFillRect(r, &rc);
    // left
    SDL_Rect rl = {x, y+rad, rad, h-2*rad};
    SDL_RenderFillRect(r, &rl);
    // right
    SDL_Rect rr2 = {x+w-rad, y+rad, rad, h-2*rad};
    SDL_RenderFillRect(r, &rr2);
    // corners (filled circles)
    int cx[4] = {x+rad, x+w-rad-1, x+rad, x+w-rad-1};
    int cy[4] = {y+rad, y+rad, y+h-rad-1, y+h-rad-1};
    for (int c = 0; c < 4; c++) {
        for (int dy = -rad; dy <= rad; dy++) {
            int dx = (int)sqrt((float)(rad*rad - dy*dy));
            SDL_RenderDrawLine(r, cx[c]-dx, cy[c]+dy, cx[c]+dx, cy[c]+dy);
        }
    }
}

static void fillEllipse(SDL_Renderer* r, int cx, int cy, int rx, int ry,
                         Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
    for (int dy = -ry; dy <= ry; dy++) {
        int dx = (int)(rx * sqrt(1.0 - (double)(dy*dy)/(double)(ry*ry)));
        SDL_RenderDrawLine(r, cx-dx, cy+dy, cx+dx, cy+dy);
    }
}

// ============================================
//  Text Drawing
// ============================================
static void drawText(SDL_Renderer* rnd, int x, int y, const char* text,
                     Uint8 r, Uint8 g, Uint8 b)
{
    if (!text || !text[0] || !gFont) return;
    SDL_Color col = {r, g, b, 255};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(gFont, text, col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(rnd, surf);
    SDL_Rect dst = {x, y, surf->w, surf->h};
    SDL_RenderCopy(rnd, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

static int textWidth(const char* text) {
    if (!text || !gFont) return 0;
    int w = 0, h = 0;
    TTF_SizeUTF8(gFont, text, &w, &h);
    return w;
}

// ============================================
//  Build Palette Blocks
// ============================================
static void buildPalette() {
    gBlocks.clear();
    struct Def { Category cat; BlockShape shape; const char* text; };
    Def defs[] = {
        {Category::MOTION,    BlockShape::COMMAND,  "move 10 steps"},
        {Category::MOTION,    BlockShape::COMMAND,  "turn R 15 deg"},
        {Category::MOTION,    BlockShape::COMMAND,  "turn L 15 deg"},
        {Category::MOTION,    BlockShape::COMMAND,  "go to x:0 y:0"},
        {Category::MOTION,    BlockShape::COMMAND,  "set x to 0"},
        {Category::MOTION,    BlockShape::COMMAND,  "set y to 0"},
        {Category::MOTION,    BlockShape::REPORTER, "x position"},
        {Category::MOTION,    BlockShape::REPORTER, "y position"},
        {Category::LOOKS,     BlockShape::COMMAND,  "say Hello!"},
        {Category::LOOKS,     BlockShape::COMMAND,  "think Hmm..."},
        {Category::LOOKS,     BlockShape::COMMAND,  "show"},
        {Category::LOOKS,     BlockShape::COMMAND,  "hide"},
        {Category::LOOKS,     BlockShape::COMMAND,  "set size 100%"},
        {Category::LOOKS,     BlockShape::REPORTER, "size"},
        {Category::SOUND,     BlockShape::COMMAND,  "play sound"},
        {Category::SOUND,     BlockShape::COMMAND,  "stop sounds"},
        {Category::EVENTS,    BlockShape::HAT,      "when flag clicked"},
        {Category::EVENTS,    BlockShape::HAT,      "when key pressed"},
        {Category::CONTROL,   BlockShape::COMMAND,  "wait 1 secs"},
        {Category::CONTROL,   BlockShape::COMMAND,  "repeat 10"},
        {Category::CONTROL,   BlockShape::CAP,      "stop all"},
        {Category::SENSING,   BlockShape::BOOLEAN,  "touching edge?"},
        {Category::SENSING,   BlockShape::REPORTER, "mouse x"},
        {Category::SENSING,   BlockShape::REPORTER, "mouse y"},
        {Category::OPERATORS, BlockShape::REPORTER, "  +  "},
        {Category::OPERATORS, BlockShape::REPORTER, "  -  "},
        {Category::OPERATORS, BlockShape::REPORTER, "  *  "},
        {Category::OPERATORS, BlockShape::REPORTER, "  /  "},
        {Category::OPERATORS, BlockShape::BOOLEAN,  "  <  "},
        {Category::OPERATORS, BlockShape::BOOLEAN,  "  >  "},
        {Category::VARIABLES, BlockShape::COMMAND,  "set var to 0"},
        {Category::VARIABLES, BlockShape::COMMAND,  "change var by 1"},
        {Category::VARIABLES, BlockShape::REPORTER, "my variable"},
        {Category::PEN,       BlockShape::COMMAND,  "pen down"},
        {Category::PEN,       BlockShape::COMMAND,  "pen up"},
        {Category::PEN,       BlockShape::COMMAND,  "erase all"},
    };
    int count = sizeof(defs)/sizeof(defs[0]);

    // چینش افقی در پالت - بر اساس کتگوری گروه‌بندی
    float px = 10, py = 8;
    Category lastCat = (Category)-1;
    for (int i = 0; i < count; i++) {
        if (defs[i].cat != lastCat) {
            if (lastCat != (Category)-1) { px += 18; } // فاصله بین کتگوری‌ها
            lastCat = defs[i].cat;
        }
        int tw = max((int)(textWidth(defs[i].text) + 24), BLOCK_W);
        // اگه از عرض پالت رد شد، برو خط بعد
        if (px + tw > WIN_W - STAGE_W - 20) {
            px = 10;
            py += BLOCK_H + 6;
        }
        Block b;
        b.id = i;
        b.cat = defs[i].cat;
        b.shape = defs[i].shape;
        b.text = defs[i].text;
        b.x = px; b.y = py;
        b.w = tw; b.h = BLOCK_H;
        b.inPalette = true;
        b.nextBlockId = -1;
        b.parentBlockId = -1;
        gBlocks.push_back(b);
        px += tw + 8;
    }
    gNextId = count + 100;
}

// ============================================
//  Find Block
// ============================================
static Block* findBlock(int id) {
    for (auto& b : gBlocks) if (b.id == id) return &b;
    return nullptr;
}

// ============================================
//  Clone block from palette
// ============================================
static Block cloneBlock(const Block& src, float x, float y) {
    Block b = src;
    b.id = gNextId++;
    b.x = x; b.y = y;
    b.inPalette = false;
    b.nextBlockId = -1;
    b.parentBlockId = -1;
    return b;
}

// ============================================
//  Snap: connect blocks vertically
// ============================================
static void trySnap(Block& dropped) {
    if (dropped.shape == BlockShape::REPORTER || dropped.shape == BlockShape::BOOLEAN)
        return;
    for (auto& other : gBlocks) {
        if (other.id == dropped.id || other.inPalette) continue;
        if (other.shape == BlockShape::REPORTER || other.shape == BlockShape::BOOLEAN) continue;
        // اسنپ به زیر بلوک دیگه
        float dx = fabs(dropped.x - other.x);
        float dy = fabs(dropped.y - (other.y + other.h));
        if (dx < SNAP_DIST && dy < SNAP_DIST) {
            dropped.x = other.x;
            dropped.y = other.y + other.h - 2;
            // اتصال زنجیره
            if (other.nextBlockId < 0) {
                other.nextBlockId = dropped.id;
                dropped.parentBlockId = other.id;
            }
            return;
        }
        // اسنپ بالای بلوک دیگه
        float dy2 = fabs((dropped.y + dropped.h) - other.y);
        if (dx < SNAP_DIST && dy2 < SNAP_DIST) {
            dropped.x = other.x;
            dropped.y = other.y - dropped.h + 2;
            if (dropped.nextBlockId < 0) {
                dropped.nextBlockId = other.id;
                other.parentBlockId = dropped.id;
            }
            return;
        }
    }
}

// ============================================
//  Detach block from chain
// ============================================
static void detachBlock(Block& b) {
    if (b.parentBlockId >= 0) {
        Block* parent = findBlock(b.parentBlockId);
        if (parent && parent->nextBlockId == b.id) {
            parent->nextBlockId = -1;
        }
        b.parentBlockId = -1;
    }
    // همچنین بلوک بعدی رو جدا کن
    if (b.nextBlockId >= 0) {
        Block* child = findBlock(b.nextBlockId);
        if (child) child->parentBlockId = -1;
    }
}

// ============================================
//  Draw single block
// ============================================
static void drawBlock(SDL_Renderer* rnd, Block& b, bool highlight) {
    SDL_Color col = catColor(b.cat);
    Uint8 cr = col.r, cg = col.g, cb2 = col.b;
    if (highlight) {
        cr = min(255, cr+50);
        cg = min(255, cg+50);
        cb2 = min(255, cb2+50);
    }
    int bx=(int)b.x, by=(int)b.y, bw=(int)b.w, bh=(int)b.h;

    switch (b.shape) {
    case BlockShape::COMMAND:
    case BlockShape::CAP:
        fillRoundedRect(rnd, bx, by, bw, bh, BLOCK_R, cr, cg, cb2, 255);
        // notch بالا
        { SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255);
          SDL_Rect n={bx+15, by-3, 24, 3}; SDL_RenderFillRect(rnd,&n); }
        // notch پایین (فقط COMMAND)
        if (b.shape != BlockShape::CAP) {
            SDL_Rect n2={bx+15, by+bh, 24, 3};
            SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255);
            SDL_RenderFillRect(rnd,&n2);
        }
        break;
    case BlockShape::HAT:
        fillRoundedRect(rnd, bx, by+8, bw, bh-8, BLOCK_R, cr, cg, cb2, 255);
        fillEllipse(rnd, bx+bw/2, by+8, bw/2, 10, cr, cg, cb2, 255);
        { SDL_Rect n2={bx+15, by+bh, 24, 3};
          SDL_SetRenderDrawColor(rnd,cr,cg,cb2,255);
          SDL_RenderFillRect(rnd,&n2); }
        break;
    case BlockShape::REPORTER:
        fillRoundedRect(rnd, bx, by, bw, bh, bh/2, cr, cg, cb2, 255);
        break;
    case BlockShape::BOOLEAN: {
        // شکل لوزی‌شکل
        int mx = bx + bw/2, my = by + bh/2;
        int hw = bw/2, hh = bh/2;
        SDL_SetRenderDrawColor(rnd, cr, cg, cb2, 255);
        for (int dy = -hh; dy <= hh; dy++) {
            float ratio = 1.0f - fabs((float)dy / hh);
            int dxr = (int)(hw * ratio);
            SDL_RenderDrawLine(rnd, mx-dxr, my+dy, mx+dxr, my+dy);
        }
        break;
    }
    }
    // متن بلوک
    int tx = bx + 10, ty = by + (bh - 14) / 2;
    if (b.shape == BlockShape::BOOLEAN) tx = bx + bw/4;
    drawText(rnd, tx, ty, b.text.c_str(), 255, 255, 255);
}

// ============================================
//  Draw Stage
// ============================================
static void drawStage(SDL_Renderer* rnd) {
    int sx = WIN_W - STAGE_W - 10;
    int sy = PALETTE_H + 10;
    // border
    SDL_SetRenderDrawColor(rnd, 180, 180, 180, 255);
    SDL_Rect border = {sx-2, sy-2, STAGE_W+4, STAGE_H+4};
    SDL_RenderFillRect(rnd, &border);
    // white stage
    SDL_SetRenderDrawColor(rnd, 255, 255, 255, 255);
    SDL_Rect stage = {sx, sy, STAGE_W, STAGE_H};
    SDL_RenderFillRect(rnd, &stage);
    // label
    drawText(rnd, sx+4, sy+4, "Stage", 100, 100, 100);
    // simple cat icon
    int catX = sx + STAGE_W/2, catY = sy + STAGE_H/2;
    SDL_SetRenderDrawColor(rnd, 230, 150, 50, 255);
    fillEllipse(rnd, catX, catY, 20, 25, 230, 150, 50, 255);
    fillEllipse(rnd, catX, catY-30, 14, 14, 230, 150, 50, 255);
    // ears
    SDL_SetRenderDrawColor(rnd, 230, 150, 50, 255);
    fillEllipse(rnd, catX-10, catY-42, 5, 8, 230, 150, 50, 255);
    fillEllipse(rnd, catX+10, catY-42, 5, 8, 230, 150, 50, 255);
    // eyes
    fillEllipse(rnd, catX-5, catY-32, 3, 3, 255, 255, 255, 255);
    fillEllipse(rnd, catX+5, catY-32, 3, 3, 255, 255, 255, 255);
    fillEllipse(rnd, catX-5, catY-32, 1, 1, 0, 0, 0, 255);
    fillEllipse(rnd, catX+5, catY-32, 1, 1, 0, 0, 0, 255);
    // green flag + stop
    int flagX = sx, flagY = sy - 28;
    fillRoundedRect(rnd, flagX, flagY, 30, 24, 4, 40, 180, 40, 255);
    drawText(rnd, flagX+6, flagY+3, "▶", 255, 255, 255);
    fillRoundedRect(rnd, flagX+36, flagY, 30, 24, 4, 200, 50, 50, 255);
    drawText(rnd, flagX+44, flagY+3, "■", 255, 255, 255);
}

// ============================================
//  Draw Palette background
// ============================================
static void drawPaletteBackground(SDL_Renderer* rnd) {
    SDL_SetRenderDrawColor(rnd, 235, 235, 235, 255);
    SDL_Rect r = {0, 0, WIN_W - STAGE_W - 20, PALETTE_H};
    SDL_RenderFillRect(rnd, &r);
    // separator line
    SDL_SetRenderDrawColor(rnd, 200, 200, 200, 255);
    SDL_RenderDrawLine(rnd, 0, PALETTE_H, WIN_W - STAGE_W - 20, PALETTE_H);
}

// ============================================
//  Draw Workspace background
// ============================================
static void drawWorkspaceBackground(SDL_Renderer* rnd) {
    SDL_SetRenderDrawColor(rnd, 245, 245, 245, 255);
    SDL_Rect r = {0, PALETTE_H+1, WIN_W - STAGE_W - 20, WIN_H - PALETTE_H - 1};
    SDL_RenderFillRect(rnd, &r);
    // grid dots for visual
    SDL_SetRenderDrawColor(rnd, 220, 220, 220, 255);
    for (int gx = 20; gx < WIN_W - STAGE_W - 20; gx += 30) {
        for (int gy = PALETTE_H + 20; gy < WIN_H; gy += 30) {
            SDL_RenderDrawPoint(rnd, gx, gy);
        }
    }
    drawText(rnd, 10, PALETTE_H + 6, "Workspace", 180, 180, 180);
}

// ============================================
//  Main
// ============================================
int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF Init failed: %s\n", TTF_GetError());
        return 1;
    }

    gWindow = SDL_CreateWindow("Scratch Simulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    gRenderer = SDL_CreateRenderer(gWindow, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // فونت - مسیر فونت رو تنظیم کنید
    gFont = TTF_OpenFont("font.ttf", 13);
    if (!gFont) gFont = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 13);
    if (!gFont) gFont = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 13);
    if (!gFont) {
        fprintf(stderr, "Warning: Could not load font. Text will not render.\n");
    }

    buildPalette();

    while (gRunning) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                gRunning = false;
                break;

            case SDL_MOUSEBUTTONDOWN: {
                if (ev.button.button != SDL_BUTTON_LEFT) break;
                int mx = ev.button.x, my = ev.button.y;
                // بلوک‌ها رو از آخر بررسی کن (بالاترین لایه)
                for (int i = (int)gBlocks.size()-1; i >= 0; i--) {
                    Block& b = gBlocks[i];
                    if (mx >= b.x && mx <= b.x+b.w && my >= b.y && my <= b.y+b.h) {
                        if (b.inPalette) {
                            // کلون از پالت
                            Block nb = cloneBlock(b, (float)mx - b.w/2, (float)my - b.h/2);
                            gBlocks.push_back(nb);
                            gDragId = nb.id;
                            gDragOffX = b.w/2;
                            gDragOffY = b.h/2;
                        } else {
                            // برداشتن بلوک از ورک‌اسپیس
                            detachBlock(b);
                            gDragId = b.id;
                            gDragOffX = mx - b.x;
                            gDragOffY = my - b.y;
                        }
                        break;
                    }
                }
                break;
            }

            case SDL_MOUSEMOTION: {
                if (gDragId < 0) break;
                Block* db = findBlock(gDragId);
                if (db) {
                    db->x = ev.motion.x - gDragOffX;
                    db->y = ev.motion.y - gDragOffY;
                    // بلوک‌های متصل رو هم بکش
                    float cy = db->y + db->h - 2;
                    int nid = db->nextBlockId;
                    while (nid >= 0) {
                        Block* nb = findBlock(nid);
                        if (!nb) break;
                        nb->x = db->x;
                        nb->y = cy;
                        cy += nb->h - 2;
                        nid = nb->nextBlockId;
                    }
                }
                break;
            }

            case SDL_MOUSEBUTTONUP: {
                if (ev.button.button != SDL_BUTTON_LEFT) break;
                if (gDragId < 0) break;
                Block* db = findBlock(gDragId);
                if (db) {
                    // اگه داخل پالت ول شد، حذفش کن
                    if (db->y < PALETTE_H && !db->inPalette) {
                        // حذف بلوک
                        for (auto it = gBlocks.begin(); it != gBlocks.end(); ++it) {
                            if (it->id == gDragId) {
                                gBlocks.erase(it);
                                break;
                            }
                        }
                    } else if (!db->inPalette) {
                        // اسنپ
                        trySnap(*db);
                    }
                }
                gDragId = -1;
                break;
            }

            case SDL_KEYDOWN:
                if (ev.key.keysym.sym == SDLK_ESCAPE) gRunning = false;
                break;
            }
        }

        // ============ Render ============
        SDL_SetRenderDrawColor(gRenderer, 250, 250, 250, 255);
        SDL_RenderClear(gRenderer);

        drawPaletteBackground(gRenderer);
        drawWorkspaceBackground(gRenderer);
        drawStage(gRenderer);

        // بلوک‌های پالت رو اول بکش
        for (auto& b : gBlocks) {
            if (b.inPalette && b.id != gDragId) {
                drawBlock(gRenderer, b, false);
            }
        }
        // بلوک‌های ورک‌اسپیس
        for (auto& b : gBlocks) {
            if (!b.inPalette && b.id != gDragId) {
                drawBlock(gRenderer, b, false);
            }
        }
        // بلوک درحال درگ (روی همه)
        if (gDragId >= 0) {
            Block* db = findBlock(gDragId);
            if (db) {
                drawBlock(gRenderer, *db, true);
                // بلوک‌های متصل
                int nid = db->nextBlockId;
                while (nid >= 0) {
                    Block* nb = findBlock(nid);
                    if (!nb) break;
                    drawBlock(gRenderer, *nb, false);
                    nid = nb->nextBlockId;
                }
            }
        }

        SDL_RenderPresent(gRenderer);
        SDL_Delay(16);
    }

    // Cleanup
    if (gFont) TTF_CloseFont(gFont);
    TTF_Quit();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
    return 0;
}
