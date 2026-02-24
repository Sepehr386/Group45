#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <string>
#include <cmath>
using namespace std;

const int W=1280,H=720,PAL_W=280,TAB_H=38,BLK_H=38,BLK_PAD=6,STAGE_W=360,STAGE_H=270,TOOLBAR_H=45,CAT_PW=110;

struct Cat{const char*name;Uint8 r,g,b;};
static Cat cats[]={
    {"Motion",66,133,244},{"Looks",147,83,211},{"Sound",207,99,207},{"Events",255,191,0},
    {"Control",255,171,25},{"Sensing",92,177,214},{"Operators",89,192,89},{"Variables",255,140,26}
};
const int NCAT=8;

struct Blk{int cat;const char*txt;int shape;};
static Blk palette[]={
    {0,"move 10 steps",0},{0,"turn right 15",0},{0,"turn left 15",0},
    {0,"go to x:0 y:0",0},{0,"glide 1s to x:0 y:0",0},{0,"set x to 0",0},
    {0,"set y to 0",0},{0,"change x by 10",0},{0,"change y by 10",0},
    {0,"point in direction 90",0},{0,"if on edge bounce",0},
    {0,"x position",2},{0,"y position",2},{0,"direction",2},

    {1,"say Hello! for 2s",0},{1,"say Hello!",0},{1,"think Hmm.. for 2s",0},
    {1,"show",0},{1,"hide",0},{1,"switch costume to",0},
    {1,"next costume",0},{1,"set size to 100%",0},{1,"change size by 10",0},
    {1,"set ghost effect to 0",0},{1,"costume #",2},{1,"size",2},

    {2,"play sound pop",0},{2,"play sound until done",0},{2,"stop all sounds",0},
    {2,"set volume to 100%",0},{2,"change volume by -10",0},{2,"volume",2},

    {3,"when flag clicked",1},{3,"when space key pressed",1},
    {3,"when this sprite clicked",1},{3,"when backdrop switches",1},
    {3,"broadcast message",0},{3,"when I receive message",1},

    {4,"wait 1 seconds",0},{4,"repeat 10",4},{4,"forever",4},
    {4,"if  then",4},{4,"if  else",4},{4,"wait until",0},
    {4,"repeat until",4},{4,"stop all",5},{4,"create clone of myself",0},
    {4,"delete this clone",5},

    {5,"touching mouse-pointer?",3},{5,"touching color?",3},
    {5,"color is touching?",3},{5,"distance to mouse",2},
    {5,"ask and wait",0},{5,"answer",2},{5,"key space pressed?",3},
    {5,"mouse down?",3},{5,"mouse x",2},{5,"mouse y",2},
    {5,"timer",2},{5,"reset timer",0},

    {6,"( ) + ( )",2},{6,"( ) - ( )",2},{6,"( ) * ( )",2},{6,"( ) / ( )",2},
    {6,"pick random 1 to 10",2},{6,"( ) > ( )",3},{6,"( ) < ( )",3},
    {6,"( ) = ( )",3},{6,"( ) and ( )",3},{6,"( ) or ( )",3},
    {6,"not ( )",3},{6,"join hello world",2},{6,"letter 1 of world",2},
    {6,"length of world",2},{6,"( ) mod ( )",2},{6,"round ( )",2},

    {7,"set myVar to 0",0},{7,"change myVar by 1",0},
    {7,"show variable myVar",0},{7,"hide variable myVar",0},{7,"myVar",2},
};
const int NBLK=sizeof(palette)/sizeof(palette[0]);

static TTF_Font*font=nullptr;

void box(SDL_Renderer*r,int x,int y,int w,int h,Uint8 R,Uint8 G,Uint8 B,Uint8 A=255){
    SDL_SetRenderDrawColor(r,R,G,B,A);
    SDL_Rect rc={x,y,w,h};
    SDL_RenderFillRect(r,&rc);
}

void roundBox(SDL_Renderer*r,int x,int y,int w,int h,int rad,Uint8 R,Uint8 G,Uint8 B){
    SDL_SetRenderDrawColor(r,R,G,B,255);
    SDL_Rect rr[]={{x+rad,y,w-2*rad,h},{x,y+rad,rad,h-2*rad},{x+w-rad,y+rad,rad,h-2*rad}};
    SDL_RenderFillRects(r,rr,3);
    auto circ=[&](int cx,int cy,int rd){
        for(int dy=-rd;dy<=rd;dy++){
            int dx=(int)sqrtf((float)(rd*rd-dy*dy));
            SDL_RenderDrawLine(r,cx-dx,cy+dy,cx+dx,cy+dy);
        }
    };
    circ(x+rad,y+rad,rad);circ(x+w-rad-1,y+rad,rad);
    circ(x+rad,y+h-rad-1,rad);circ(x+w-rad-1,y+h-rad-1,rad);
}

void txt(SDL_Renderer*r,int x,int y,const char*s,Uint8 R,Uint8 G,Uint8 B){
    if(!font||!s||!*s)return;
    SDL_Color c={R,G,B,255};
    SDL_Surface*sf=TTF_RenderUTF8_Blended(font,s,c);
    if(!sf)return;
    SDL_Texture*t=SDL_CreateTextureFromSurface(r,sf);
    SDL_Rect d={x,y,sf->w,sf->h};
    SDL_RenderCopy(r,t,NULL,&d);
    SDL_DestroyTexture(t);SDL_FreeSurface(sf);
}

int tw(const char*s){if(!font)return 0;int w,h;TTF_SizeUTF8(font,s,&w,&h);return w;}
int th(){return font?TTF_FontHeight(font):14;}

void drawBlock(SDL_Renderer*r,int x,int y,int w,int h,int shape,Uint8 cr,Uint8 cg,Uint8 cb,const char*label){
    if(shape==2){
        roundBox(r,x,y,w,h,h/2,cr,cg,cb);
    }else if(shape==3){
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
    }else if(shape==1){
        roundBox(r,x,y+12,w,h,7,cr,cg,cb);
        roundBox(r,x+10,y,60,16,8,cr,cg,cb);
    }else if(shape==4){
        roundBox(r,x,y,w,h,5,cr,cg,cb);
        box(r,x,y+h,18,28,cr,cg,cb);
        roundBox(r,x,y+h+28,w,h*2/3,5,cr,cg,cb);
        roundBox(r,x+18,y+h,w-22,28,3,245,245,248);
    }else if(shape==5){
        roundBox(r,x,y,w,h,7,cr,cg,cb);
        Uint8 dr=(Uint8)(cr>35?cr-35:0),dg=(Uint8)(cg>35?cg-35:0),db=(Uint8)(cb>35?cb-35:0);
        box(r,x+4,y+h-3,w-8,3,dr,dg,db);
    }else{
        roundBox(r,x,y,w,h,5,cr,cg,cb);
        box(r,x+14,y-2,30,3,cr,cg,cb);
        box(r,x+14,y+h-1,30,3,cr,cg,cb);
    }
    int tx=(shape==2||shape==3)?x+h/2+4:x+10;
    int ty=y+(h-th())/2;
    if(shape==1) ty+=6;
    txt(r,tx,ty,label,255,255,255);
}

void drawToolbar(SDL_Renderer*r){
    box(r,0,0,W,TOOLBAR_H,50,50,60);
    roundBox(r,10,7,32,32,6,34,180,80);
    txt(r,17,11,"▶",255,255,255);
    roundBox(r,52,7,32,32,6,200,60,60);
    txt(r,59,11,"■",255,255,255);
    txt(r,W/2-60,12,"Scratch Simulator",220,220,230);
}

void drawStage(SDL_Renderer*r,int sx,int sy,int sw,int sh){
    box(r,sx,sy,sw,sh,255,255,255);
    box(r,sx,sy,sw,2,200,200,210);
    box(r,sx,sy,2,sh,200,200,210);
    box(r,sx+sw-2,sy,2,sh,200,200,210);
    box(r,sx,sy+sh-2,sw,2,200,200,210);

    int cx=sx+sw/2, cy=sy+sh/2;
    SDL_SetRenderDrawColor(r,220,220,230,255);
    SDL_RenderDrawLine(r,cx,sy+4,cx,sy+sh-4);
    SDL_RenderDrawLine(r,sx+4,cy,sx+sw-4,cy);

    txt(r,sx+sw/2-20,sy+sh-20,"0 , 0",180,180,190);
    txt(r,sx+6,sy+6,"Stage",160,160,175);
}

void drawSpritePanel(SDL_Renderer*r,int sx,int sy,int sw,int sh){
    box(r,sx,sy,sw,sh,248,248,252);
    box(r,sx,sy,sw,2,210,210,220);
    txt(r,sx+10,sy+8,"Sprites",100,100,120);

    int thumbS=64,tx=sx+14,ty=sy+32;
    roundBox(r,tx,ty,thumbS,thumbS,8,66,133,244);
    txt(r,tx+10,ty+thumbS/2-7,"Cat",255,255,255);
    txt(r,tx+4,ty+thumbS+4,"Sprite1",80,80,100);

    roundBox(r,sx+sw-44,sy+sh-44,36,36,18,50,160,80);
    txt(r,sx+sw-32,sy+sh-38,"+",255,255,255);
}

void drawCategoryPanel(SDL_Renderer*r,int px,int py,int pw,int ph,int sel){
    box(r,px,py,pw,ph,245,245,250);
    box(r,px+pw-1,py,1,ph,215,215,225);
    int btnH=32,gap=3,top=py+6;
    for(int i=0;i<NCAT;i++){
        int by=top+i*(btnH+gap);
        Cat&c=cats[i];
        if(i==sel){
            roundBox(r,px+4,by,pw-8,btnH,6,c.r,c.g,c.b);
            txt(r,px+12,by+(btnH-th())/2,c.name,255,255,255);
        }else{
            Uint8 lr=(Uint8)(c.r*0.3+170),lg=(Uint8)(c.g*0.3+170),lb=(Uint8)(c.b*0.3+170);
            roundBox(r,px+4,by,pw-8,btnH,6,lr,lg,lb);
            txt(r,px+12,by+(btnH-th())/2,c.name,60,60,70);
        }
    }
}

int main(int,char**){
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window*win=SDL_CreateWindow("Scratch Blocks",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,SDL_WINDOW_SHOWN);
    SDL_Renderer*ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);

    const char*fp[]={
        "DejaVuSans.ttf","font.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf","C:\\Windows\\Fonts\\arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",NULL
    };
    for(int i=0;fp[i];i++){font=TTF_OpenFont(fp[i],13);if(font)break;}

    int sel=0,scrollY=0;
    bool run=true;

    int stageX=W-STAGE_W-10, stageY=TOOLBAR_H+10;
    int spPanelX=stageX, spPanelY=stageY+STAGE_H+8, spPanelW=STAGE_W, spPanelH=H-spPanelY-6;
    int catPX=PAL_W, catPY=TOOLBAR_H;
    int catPW=CAT_PW, catPH=H-TOOLBAR_H;
    int blkAreaX=0, blkAreaY=TOOLBAR_H;
    int blkAreaW=PAL_W, blkAreaH=H-TOOLBAR_H;

    while(run){
        SDL_Event e;
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) run=false;
            if(e.type==SDL_KEYDOWN&&e.key.keysym.sym==SDLK_ESCAPE) run=false;

            if(e.type==SDL_MOUSEBUTTONDOWN){
                int mx=e.button.x,my=e.button.y;
                if(mx>=catPX&&mx<catPX+catPW&&my>=catPY){
                    int idx=(my-catPY-6)/(32+3);
                    if(idx>=0&&idx<NCAT){sel=idx;scrollY=0;}
                }
            }
            if(e.type==SDL_MOUSEWHEEL){
                int mx,my;SDL_GetMouseState(&mx,&my);
                if(mx<PAL_W&&my>TOOLBAR_H){
                    scrollY-=e.wheel.y*20;
                    if(scrollY<0)scrollY=0;
                }
            }
        }

        SDL_SetRenderDrawColor(ren,255,255,255,255);
        SDL_RenderClear(ren);

        drawToolbar(ren);

        box(ren,blkAreaX,blkAreaY,blkAreaW,blkAreaH,250,250,255);

        Cat&cc=cats[sel];
        txt(ren,10,TOOLBAR_H+8,cc.name,cc.r,cc.g,cc.b);

        SDL_Rect clip={blkAreaX,blkAreaY+28,blkAreaW,blkAreaH-28};
        SDL_RenderSetClipRect(ren,&clip);

        int by=TOOLBAR_H+32-scrollY;
        for(int i=0;i<NBLK;i++){
            if(palette[i].cat!=sel)continue;
            int bw=tw(palette[i].txt)+34;
            if(palette[i].shape==2||palette[i].shape==3) bw+=BLK_H/2;
            if(bw<160)bw=160;
            if(bw>blkAreaW-20)bw=blkAreaW-20;
            int totalH=BLK_H;
            if(palette[i].shape==4) totalH=BLK_H+28+BLK_H*2/3;
            if(palette[i].shape==1) totalH=BLK_H+14;
            if(by+totalH>TOOLBAR_H&&by<H)
                drawBlock(ren,12,by,bw,BLK_H,palette[i].shape,cc.r,cc.g,cc.b,palette[i].txt);
            by+=totalH+BLK_PAD;
        }
        SDL_RenderSetClipRect(ren,NULL);

        drawCategoryPanel(ren,catPX,catPY,catPW,catPH,sel);

        int wsX=catPX+catPW, wsY=TOOLBAR_H, wsW=stageX-wsX, wsH=H-TOOLBAR_H;
        box(ren,wsX,wsY,wsW,wsH,255,255,255);
        box(ren,wsX,wsY,1,wsH,215,215,225);
        const char*wl="drag blocks here";
        txt(ren,wsX+wsW/2-tw(wl)/2,wsY+wsH/2-th()/2,wl,215,215,225);

        drawStage(ren,stageX,stageY,STAGE_W,STAGE_H);
        drawSpritePanel(ren,spPanelX,spPanelY,spPanelW,spPanelH);

        box(ren,stageX-2,TOOLBAR_H,2,H-TOOLBAR_H,210,210,220);

        SDL_RenderPresent(ren);
    }

    if(font)TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
