#include "/home/codeleaded/System/Static/Library/WindowEngine1.0.h"
#include "/home/codeleaded/System/Static/Library/RLCamera.h"
#include "/home/codeleaded/System/Static/Library/ImageFilter.h"


#define OUTPUT_WIDTH    (RLCAMERA_WIDTH / 2)
#define OUTPUT_HEIGHT   (RLCAMERA_HEIGHT / 2)


RLCamera rlc;
Sprite captured;
Sprite captured_old;

float* ln_before;
float* ln_after;

Vec2* vectorfield;
Vec2* flow;
Rect rect;
Vec2 rect_v;

#define WALK_X  2
#define WALK_Y  2

Vec2 Flow_CalculateVector(int index){
    Vec2 dir = { 0.0f,0.0f };
    for(int i = -WALK_Y;i<=WALK_Y;i++){
        for(int j = -WALK_X;j<=WALK_X;j++){
            if(i==0 && j==0) continue;
            
            const int access = i * OUTPUT_WIDTH + index + j;
            const Vec2 vdir = Vec2_Norm((Vec2){ j,i });
            const float diff = 100.0f * F32_Abs(ln_after[access] - ln_before[access]);
            if(access>=0 && access < OUTPUT_WIDTH * OUTPUT_HEIGHT){
                dir = Vec2_Add(dir,Vec2_Mulf(vdir,diff));
            }
        }
    }
    return dir;
}
void Flow_Calculate(){
    for(int i = 0;i<OUTPUT_WIDTH * OUTPUT_HEIGHT;i++){
        flow[i] = Vec2_Neg(Flow_CalculateVector(i));
    }
}

Vec2 CalculateVectorGes(int x,int y,int w,int h){
    Vec2 dir = { 0.0f,0.0f };
    for(int i = 0;i<h;i++){
        for(int j = 0;j<w;j++){
            const int access = (y + i) * OUTPUT_WIDTH + (x + j);
            if(access<0 || access>=OUTPUT_WIDTH) continue;

            dir = Vec2_Add(dir,flow[access]);
        }
    }
    return dir;
}
void BW_Render(Pixel* target,unsigned int width,unsigned int height,float* bw_buffer,int w,int h){
    for(int i = 0;i<h;i++){
        for(int j = 0;j<w;j++){
            target[i * width + j] = Pixel_toRGBA(bw_buffer[i * w + j],bw_buffer[i * w + j],bw_buffer[i * w + j],1.0f);
        }
    }
}

void Setup(AlxWindow* w){
    rlc = RLCamera_New("/dev/video0",RLCAMERA_WIDTH,RLCAMERA_HEIGHT);

    captured = Sprite_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    captured_old = Sprite_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    
    ln_before = (float*)malloc(sizeof(float) * OUTPUT_WIDTH * OUTPUT_HEIGHT);
    ln_after = (float*)malloc(sizeof(float) * OUTPUT_WIDTH * OUTPUT_HEIGHT);

    vectorfield = (Vec2*)malloc(sizeof(Vec2) * OUTPUT_WIDTH * OUTPUT_HEIGHT);
    flow = (Vec2*)malloc(sizeof(Vec2) * OUTPUT_WIDTH * OUTPUT_HEIGHT);
    
    rect = Rect_New((Vec2){ OUTPUT_WIDTH * 0.5f,OUTPUT_HEIGHT * 0.5f },(Vec2){ 10.0f,10.0f });
    rect_v = (Vec2){ 0.0f,0.0f };
}
void Update(AlxWindow* w){
    if(RLCamera_Ready(&rlc)){
        Sprite_Free(&captured_old);
        captured_old = captured;

        RLCamera_Update(&rlc);
        captured = Sprite_Cpy(&rlc.lastimg);
        Sprite_Resize(&captured,OUTPUT_WIDTH,OUTPUT_HEIGHT);
    }

    Clear(BLACK);

    for(int i = 0;i<captured.w * captured.h;i++){
        float filter = Pixel_Lightness_N(captured_old.img[i]) + (Pixel_Lightness_N(captured.img[i]) - Pixel_Lightness_N(captured_old.img[i])) * w->ElapsedTime * 10.0f;

        float diff = filter;
        //float diff = F32_Abs(filter - ln_after[i]);
        //if(diff <= 0.01f) diff = 0.0f;

        ln_before[i] = ln_after[i];
        ln_after[i] = diff;
    }

    Flow_Calculate();

    Vec2 middle = Vec2_Add(rect.p,Vec2_Mulf(rect.d,0.5f));
    Vec2 flowdir = flow[(int)middle.y * OUTPUT_WIDTH + (int)middle.x];
    //Vec2 flowdir = CalculateVectorGes((int)rect.p.x,(int)rect.p.y,(int)rect.d.x,(int)rect.d.y);
    rect_v = Vec2_Add(rect_v,Vec2_Mulf(flowdir,100.0f * w->ElapsedTime));
    rect_v = Vec2_Mulf(rect_v,0.99f);

    rect.p = Vec2_Add(rect.p,Vec2_Mulf(rect_v,1.0f * w->ElapsedTime));

    if(rect.p.x<0.0f){
        rect.p.x = 0.0f;
        rect_v.x *= -1.0f;
    }
    if(rect.p.y<0.0f){
        rect.p.y = 0.0f;
        rect_v.y *= -1.0f;
    }
    if(rect.p.x>OUTPUT_WIDTH - rect.d.x){
        rect.p.x = OUTPUT_WIDTH - rect.d.x;
        rect_v.x *= -1.0f;
    }
    if(rect.p.y>OUTPUT_HEIGHT - rect.d.y){
        rect.p.y = OUTPUT_HEIGHT - rect.d.y;
        rect_v.y *= -1.0f;
    }
    
    Sprite_Render(WINDOW_STD_ARGS,&captured,0.0f,0.0f);
    //BW_Render(WINDOW_STD_ARGS,ln_before,OUTPUT_WIDTH,OUTPUT_HEIGHT);
    RenderRect(rect.p.x,rect.p.y,rect.d.x,rect.d.y,RED);
}
void Delete(AlxWindow* w){
    RLCamera_Free(&rlc);
    Sprite_Free(&captured);
    Sprite_Free(&captured_old);
    
    if(ln_before) free(ln_before);
    ln_before = NULL;

    if(ln_after) free(ln_after);
    ln_after = NULL;


    if(vectorfield) free(vectorfield);
    vectorfield = NULL;
    
    if(flow) free(flow);
    flow = NULL;
}

int main(){
    if(Create("AR - Optical Flow - Scroll",OUTPUT_WIDTH,OUTPUT_HEIGHT,4,4,Setup,Update,Delete))
        Start();
    return 0;
}