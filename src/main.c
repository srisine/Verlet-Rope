#include<stdio.h>
#include<math.h>

#include<SDL3/SDL.h>

#define WIN_W 1080
#define WIN_H 720

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Event event;

typedef struct {
    float x;
    float y;
} vec2;

typedef struct{
    vec2 pos;
    vec2 oldPos;
} VerletNode;

void renderCircle(vec2 coord, int r){
    int x = 0;
    int y = -r;

    while(x < -y){
        int yMid = y + 0.5;

        if(x*x + yMid*yMid > r*r)
            y += 1;

        SDL_RenderPoint(renderer, coord.x+x, coord.y+y);
        SDL_RenderPoint(renderer, coord.x-x, coord.y+y);
        SDL_RenderPoint(renderer, coord.x+x, coord.y-y);
        SDL_RenderPoint(renderer, coord.x-x, coord.y-y);
        SDL_RenderPoint(renderer, coord.x+y, coord.y+x);
        SDL_RenderPoint(renderer, coord.x-y, coord.y+x);
        SDL_RenderPoint(renderer, coord.x+y, coord.y-x);
        SDL_RenderPoint(renderer, coord.x-y, coord.y-x);
        x+=1;
    }
}

float damp = 0.9999f;
void StepVerlet(VerletNode* node){
    vec2 temp = node->pos;
    float vx = (node->pos.x-node->oldPos.x);
    float vy = (node->pos.y-node->oldPos.y)+9.8f*0.1f*0.1f;

    float maxSpeed = 20.0f;
    float speed = sqrt(vx*vx+vy*vy);
    if(speed>maxSpeed){
        float s = maxSpeed/speed;
        vx*=s;
        vy*=s;
    }else{
        vx *= damp;
        vy *= damp;
    }
    node->pos.x += vx;
    node->pos.y += vy;
    node->oldPos = temp;
}
void CollideCircle(VerletNode* node, vec2 center, float r){
    float dx = node->pos.x-center.x;
    float dy = node->pos.y-center.y;

    float dist2 = dx*dx+dy*dy;
    float mDist = r+1.0f;

    if(dist2<mDist*mDist){
        float dist = sqrt(dist2);
        if(dist==0) dist=0.00001f;

        float diff = (mDist-dist)/dist;

        node->pos.x += dx*diff;
        node->pos.y += dy*diff;
    }
}

float nodeDist = 0.0f;
void ConstrainPoints(VerletNode* node, int segments, float mX, float mY){
   
    for(int i=1; i<segments; ++i){
       vec2* prev = &node[i-1].pos;
       vec2* curr = &node[i].pos;
        
       float dx = curr->x - prev->x;
       float dy = curr->y - prev->y;

        float diff = 0;

       float mag = sqrt((dx*dx+dy*dy));
       if(mag != 0){
            diff = (nodeDist - mag)/mag;
       }


       float nx = dx*(diff * 0.5f);
       float ny = dy*(diff * 0.5f);
        
       curr->x += nx;
       curr->y += ny;
       prev->x -= nx;
       prev->y -= ny;
    }
    node[0].pos = (vec2){mX, mY};
    node[0].oldPos = node[0].pos;
}

void addObs(vec2** obs, vec2 mouse, int* count){
    (*obs)[*count] = mouse;
    *count += 1;
}

bool isRunning = true;

int main(){
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Verlet Rope", WIN_W, WIN_H, SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, NULL);
    
    const int ropeLength = 400;
    const int segments = 100;
    const int radius = 1;
    nodeDist = ropeLength/(segments-1);
    
    VerletNode* verletArr = malloc(sizeof(VerletNode)*segments);
    
    for(int i=0; i<segments; ++i){
        vec2 pos = {100, 100*(2*i)};
        VerletNode node = {pos, pos};
        verletArr[i] = node;
    }

    vec2* obs = malloc(100*sizeof(vec2));
    int obsCount = 0;

    vec2 mouse = {0,0};
    vec2 anchor = {0,0};
    bool isAnchoredToMouse = true;

    while(isRunning){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
                isRunning = false;
            }
            if(event.type == SDL_EVENT_MOUSE_MOTION){
                mouse.x = event.motion.x;
                mouse.y = event.motion.y;
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
                if (event.button.button == SDL_BUTTON_LEFT){
                    isAnchoredToMouse = true;
                }
                if (event.button.button == SDL_BUTTON_RIGHT && isAnchoredToMouse==true){
                    isAnchoredToMouse = false;
                    anchor = mouse;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT && isAnchoredToMouse==false){
                    addObs(&obs, mouse, &obsCount);
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        
        for (int i = 0; i < segments; ++i) {
            StepVerlet(&verletArr[i]);
        }
        for (int iter = 0; iter < 64; ++iter) {
            if(isAnchoredToMouse) ConstrainPoints(verletArr, segments, mouse.x, mouse.y);
            else ConstrainPoints(verletArr, segments, anchor.x, anchor.y);
            for(int i=0; i<segments; ++i){
                for(int j=0; j<obsCount; j++)
                    CollideCircle(&verletArr[i], obs[j], 10);
                }
        }
        for(int i=0; i<obsCount; ++i)
            renderCircle(obs[i], 10);
        for (int i=0; i<segments-1; ++i){
            SDL_RenderLine(renderer, verletArr[i].pos.x, verletArr[i].pos.y, verletArr[i+1].pos.x, verletArr[i+1].pos.y);
        }
        for (int i = 0; i < segments; ++i) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            renderCircle(verletArr[i].pos, 1);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}
