#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
using namespace std;

const int W = 900, H = 600, PAL_W = 260, TAB_H = 40, BLK_H = 36, BLK_PAD = 5;

struct Cat { const char* name; SDL_Color col; };
static Cat cats[] = {
    {"Motion",   {66,133,244,255}},  {"Looks",     {153,102,255,255}},
    {"Sound",    {207,99,207,255}},  {"Events",    {255,191,0,255}},
    {"Control",  {255,171,25,255}},  {"Sensing",   {92,177,214,255}},
    {"Operators",{89,192,89,255}},   {"Variables", {255,140,26,255}},
};
const int NCAT = 8;

struct Blk { int cat; const char* txt; int shape; };

static Blk palette[] = {
    {0,"move 10 steps",0},{0,"turn right 15",0},{0,"turn left 15",0},
    {0,"go to x:0 y:0",0},{0,"glide 1s to x:0 y:0",0},{0,"set x to 0",0},
    {0,"set y to 0",0},{0,"change x by 10",0},{0,"change y by 10",0},
    {0,"point in dir 90",0},{0,"if on edge bounce",0},
    {0,"x position",2},{0,"y position",2},{0,"direction",2},

    {1,"say Hello! for 2s",0},{1,"say Hello!",0},{1,"think Hmm.. for 2s",0},
    {1,"show",0},{1,"hide",0},{1,"set size to 100%",0},
    {1,"change size by 10",0},{1,"next costume",0},
    {1,"costume #",2},{1,"size",2},

    {2,"play sound",0},{2,"stop sounds",0},{2,"set vol to 100%",0},
    {2,"change vol by -10",0},{2,"volume",2},

    {3,"when flag clicked",1},{3,"when space pressed",1},
    {3,"when sprite clicked",1},{3,"broadcast msg",0},
    {3,"when I receive msg",1},

    {4,"wait 1 secs",0},{4,"repeat 10",4},{4,"forever",4},
    {4,"if  then",4},{4,"if  else",4},{4,"stop all",5},
    {4,"wait until",0},{4,"repeat until",4},

    {5,"touching mouse?",3},{5,"touching edge?",3},
    {5,"mouse x",2},{5,"mouse y",2},{5,"mouse down?",3},
    {5,"key pressed?",3},{5,"timer",2},{5,"reset timer",0},
    {5,"answer",2},{5,"ask and wait",0},

    {6,"( )+( )",2},{6,"( )-( )",2},{6,"( )*( )",2},{6,"( )/( )",2},
    {6,"random 1 to 10",2},{6,"( )>( )",3},{6,"( )<( )",3},
    {6,"( )=( )",3},{6,"( )and( )",3},{6,"( )or( )",3},
    {6,"not( )",3},{6,"join ab",2},{6,"length of",2},
    {6,"( )mod( )",2},{6,"round( )",2},

    {7,"set myVar to 0",0},{7,"change myVar by 1",0},
    {7,"show variable",0},{7,"hide variable",0},{7,"myVar",2},
};
const int NBLK = sizeof(palette)/sizeof(palette[0]);

static TTF_Font* font = nullptr;

void box(SDL_Renderer* r, int x, int y, int w, int h, Uint8 R, Uint8 G, Uint8 B) {
    SDL_SetRenderDrawColor(r,R,G,B,255);
    SDL_Rect rc={x,y,w,h};
    SDL_RenderFillRect(r,&rc);
}

void roundBox(SDL_Renderer* r, int x, int y, int w, int h, int rad, Uint8 R, Uint8 G, Uint8 B) {
    SDL_SetRenderDrawColor(r,R,G,B,255);
    SDL_Rect rr[]={{x+rad,y,w-2*rad,h},{x,y+rad,rad,h-2*rad},{x+w-rad,y+rad,rad,h-2*rad}};
    SDL_RenderFillRects(r,rr,3);
    auto circ=[&](int cx,int cy,int rd){
        for(int dy=-rd;dy<=rd;dy++){
            int dx=(int)SDL_sqrtf((float)(rd*rd-dy*dy));
            SDL_RenderDrawLine(r,cx-dx,cy+dy,cx+dx,cy+dy);
        }
    };
    circ(x+rad,y+rad,rad); circ(x+w-rad,y+rad,rad);
    circ(x+rad,y+h-rad,rad); circ(x+w-rad,y+h-rad,rad);
}

void txt(SDL_Renderer* r, int x, int y, const char* s, Uint8 R, Uint8 G, Uint8 B) {
    if(!font||!s||!*s) return;
    SDL_Color c={R,G,B,255};
    SDL_Surface* sf=TTF_RenderUTF8_Blended(font,s,c);
    if(!sf) return;
    SDL_Texture* t=SDL_CreateTextureFromSurface(r,sf);
    SDL_Rect d={x,y,sf->w,sf->h};
    SDL_RenderCopy(r,t,NULL,&d);
    SDL_DestroyTexture(t); SDL_FreeSurface(sf);
}

int tw(const char* s){ if(!font) return 0; int w,h; TTF_SizeUTF8(font,s,&w,&h); return w; }
int th(){ return font?TTF_FontHeight(font):14; }

void drawBlock(SDL_Renderer* r, int x, int y, int w, int h, int shape, SDL_Color c, const char* label) {
    Uint8 cr=c.r, cg=c.g, cb=c.b;
    if(shape==2) {
        roundBox(r,x,y,w,h,h/2,cr,cg,cb);
    } else if(shape==3) {
        int m=h/2;
        box(r,x+m,y,w-2*m,h,cr,cg,cb);
        for(int i=0;i<m;i++){
            int o=m-i;
            SDL_SetRenderDrawColor(r,cr,cg,cb,255);
            SDL_RenderDrawLine(r,x+o,y+i,x+m,y+i);
            SDL_RenderDrawLine(r,x+o,y+h-1-i,x+m,y+h-1-i);
            SDL_RenderDrawLine(r,x+w-m,y+i,x+w-o,y+i);
            SDL_RenderDrawLine(r,x+w-m,y+h-1-i,x+w-o,y+h-1-i);
        }
    } else if(shape==1) {
        roundBox(r,x,y,w,h+6,7,cr,cg,cb);
        roundBox(r,x+12,y-10,50,14,7,cr,cg,cb);
    } else if(shape==4) {
        roundBox(r,x,y,w,h,5,cr,cg,cb);
        box(r,x,y+h,16,24,cr,cg,cb);
        roundBox(r,x,y+h+24,w,h*2/3,5,cr,cg,cb);
        roundBox(r,x+16,y+h,w-20,24,3,240,240,245);
    } else if(shape==5) {
        roundBox(r,x,y,w,h,7,cr,cg,cb);
        box(r,x+3,y+h-3,w-6,3,(Uint8)(cr>30?cr-30:0),(Uint8)(cg>30?cg-30:0),(Uint8)(cb>30?cb-30:0));
    } else {
        roundBox(r,x,y,w,h,5,cr,cg,cb);
        box(r,x+14,y-2,28,2,cr,cg,cb);
        box(r,x+14,y+h,28,2,cr,cg,cb);
    }
    int tx= (shape==2||shape==3) ? x+h/2+3 : x+8;
    int ty= y+(h-th())/2;
    txt(r,tx,ty,label,255,255,255);
}

int main(int,char**) {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window* win=SDL_CreateWindow("Scratch Blocks",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
    SDL_Renderer* ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    const char* fp[]={
        "font.ttf","/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\arial.ttf","C:\\Windows\\Fonts\\segoeui.ttf",
        "/System/Library/Fonts/Helvetica.ttc",NULL
    };
    for(int i=0;fp[i];i++){ font=TTF_OpenFont(fp[i],13); if(font) break; }

    int sel=0, scrollY=0;
    bool run=true;

    while(run) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) run=false;
            if(e.type==SDL_KEYDOWN && e.key.keysym.sym==SDLK_ESCAPE) run=false;
            if(e.type==SDL_MOUSEBUTTONDOWN && e.button.y<TAB_H && e.button.x<PAL_W) {
                int cw=PAL_W/NCAT;
                int c=e.button.x/cw;
                if(c>=0&&c<NCAT){ sel=c; scrollY=0; }
            }
            if(e.type==SDL_MOUSEWHEEL) {
                int mx,my; SDL_GetMouseState(&mx,&my);
                if(mx<PAL_W && my>TAB_H) {
                    scrollY-=e.wheel.y*18;
                    if(scrollY<0) scrollY=0;
                }
            }
        }

        SDL_SetRenderDrawColor(ren,255,255,255,255);
        SDL_RenderClear(ren);

        box(ren,0,0,PAL_W,H,240,240,245);

        int cw=PAL_W/NCAT;
        for(int i=0;i<NCAT;i++){
            SDL_Color c=cats[i].col;
            if(i==sel) box(ren,i*cw,0,cw,TAB_H,c.r,c.g,c.b);
            else box(ren,i*cw,0,cw,TAB_H,(Uint8)(c.r*.5+120),(Uint8)(c.g*.5+120),(Uint8)(c.b*.5+120));
            char ini[2]={cats[i].name[0],0};
            int tx=i*cw+(cw-tw(ini))/2, ty=(TAB_H-th())/2;
            txt(ren,tx,ty,ini, i==sel?255:50, i==sel?255:50, i==sel?255:60);
        }
        box(ren,sel*cw,TAB_H-3,cw,3,cats[sel].col.r,cats[sel].col.g,cats[sel].col.b);

        txt(ren,10,TAB_H+6,cats[sel].name,cats[sel].col.r,cats[sel].col.g,cats[sel].col.b);

        SDL_Rect clip={0,TAB_H+26,PAL_W,H-TAB_H-26};
        SDL_RenderSetClipRect(ren,&clip);

        int by=TAB_H+30-scrollY;
        for(int i=0;i<NBLK;i++){
            if(palette[i].cat!=sel) continue;
            int bw=tw(palette[i].txt)+30;
            if(palette[i].shape==2||palette[i].shape==3) bw+=BLK_H/2;
            if(bw<140) bw=140;
            if(bw>PAL_W-20) bw=PAL_W-20;
            int totalH=BLK_H;
            if(palette[i].shape==4) totalH=BLK_H+24+BLK_H*2/3;
            if(by+totalH>TAB_H && by<H)
                drawBlock(ren,10,by,bw,BLK_H,palette[i].shape,cats[sel].col,palette[i].txt);
            by+=totalH+BLK_PAD;
        }
        SDL_RenderSetClipRect(ren,NULL);

        box(ren,PAL_W,0,2,H,200,200,210);

        const char* lb="Workspace";
        txt(ren, PAL_W+(W-PAL_W-tw(lb))/2, H/2-th()/2, lb, 210,210,220);

        SDL_RenderPresent(ren);
    }

    if(font) TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
