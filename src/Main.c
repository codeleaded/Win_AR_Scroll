#include "/home/codeleaded/System/Static/Library/WindowEngine1.0.h"
#include "/home/codeleaded/System/Static/Library/RLCamera.h"
#include "/home/codeleaded/System/Static/Library/ImageFilter.h"
#include "/home/codeleaded/System/Static/Library/Random.h"
#include "/home/codeleaded/System/Static/Library/OpticalFlow.h"


#define OUTPUT_WIDTH    (RLCAMERA_WIDTH / 2)
#define OUTPUT_HEIGHT   (RLCAMERA_HEIGHT / 2)


RLCamera rlc;
OpticalFlow of;

Rect rect;
Vec2 rect_v;

float xscroll;
float yscroll;
Vector selection;


void BW_Render(Pixel* target,unsigned int width,unsigned int height,float* bw_buffer,int w,int h){
    for(int i = 0;i<h;i++){
        for(int j = 0;j<w;j++){
            const float l = bw_buffer[i * w + j];
            target[i * width + j] = Pixel_toRGBA(l,l,l,1.0f);
        }
    }
}
void VF_Render(Pixel* target,unsigned int width,unsigned int height,Vec2* bw_buffer,int w,int h){
    for(int i = 0;i<h;i++){
        for(int j = 0;j<w;j++){
            const Vec2 flower = Vec2_Mulf(bw_buffer[i * w + j],0.5f);
            const Vec2 pos = { j,i };
            const float len = F32_Clamp(Vec2_Mag(flower),0.0f,1.0f);
            target[i * width + j] = Pixel_toRGBA(len,len,len,1.0f);

            if((int)F32_Abs(flower.x * 6.0f) || (int)F32_Abs(flower.y * 6.0f)){
                const Vec2 tar = Vec2_Add(pos,Vec2_Mulf(Vec2_Norm(flower),5.0f));
                RenderLine(pos,tar,RED,1.0f);
            }
        }
    }
}

void Setup(AlxWindow* w){
    rlc = RLCamera_New("/dev/video0",RLCAMERA_WIDTH,RLCAMERA_HEIGHT);
    of = OpticalFlow_New(OUTPUT_WIDTH,OUTPUT_HEIGHT);
    
    rect = Rect_New((Vec2){ OUTPUT_WIDTH * 0.5f,OUTPUT_HEIGHT * 0.5f },(Vec2){ 10.0f,10.0f });
    rect_v = (Vec2){ 0.0f,0.0f };

    xscroll = 0;
    yscroll = 0;
    selection = Vector_New(sizeof(Vector));
    for(int i = 0;i<10U;i++){
        Vector add = Vector_New(sizeof(CStr));
        for(int j = 0;j<10U;j++){
            Vector_Push(&add,(CStr[]){ CStr_Format("Hello %d and %d",i,j) });
        }
        Vector_Push(&selection,&add);
    }
}
void Update(AlxWindow* w){
    if(RLCamera_Ready(&rlc)){
        RLCamera_Update(&rlc);
        OpticalFlow_Set(&of,&rlc.lastimg,w->ElapsedTime);
    }

    Clear(BLACK);

    const Vec2 max = OpticalFlow_Max(&of);
    const Vec2 sig = Vec2_Mulf(OpticalFlow_Area(&of,max),0.5f);
    
    if(Vec2_Mag(sig) > 3.0f){
        //rect_v = Vec2_Div(Vec2_Sub(max,rect.p),(Vec2){ OUTPUT_WIDTH,OUTPUT_HEIGHT }); 
        rect_v = Vec2_Mulf(Vec2_Norm(Vec2_Sub(max,rect.p)),0.1f);
    }else{
        rect.p = max;
        rect_v = (Vec2){ 0.0f,0.0f };
    }
    
    rect.p = Vec2_Add(rect.p,Vec2_Mulf(rect_v,100.0f * w->ElapsedTime));
    
    if(F32_Abs(rect_v.x) > F32_Abs(rect_v.y))   xscroll += rect_v.x * 10.0f * w->ElapsedTime;
    else                                        yscroll += -rect_v.y * 10.0f * w->ElapsedTime;

    VF_Render(WINDOW_STD_ARGS,of.flow,OUTPUT_WIDTH,OUTPUT_HEIGHT);

    RenderLine(max,Vec2_Add(max,Vec2_Mulf(Vec2_Norm(of.flow[(int)max.y * of.captured.w + (int)max.x]),50.0f)),BLUE,1.0f);

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


    const float lx = 100.0f;
    const float ly = 100.0f;
    const float px = 10.0f;
    const float py = 10.0f;

    if(Stroke(ALX_KEY_W).DOWN)   yscroll += 1.0f * w->ElapsedTime;
    if(Stroke(ALX_KEY_S).DOWN)   yscroll -= 1.0f * w->ElapsedTime;
    if(Stroke(ALX_KEY_A).DOWN)   xscroll += 1.0f * w->ElapsedTime;
    if(Stroke(ALX_KEY_D).DOWN)   xscroll -= 1.0f * w->ElapsedTime;

    if(yscroll < 0.0f)          yscroll = 0.0f;
    if(xscroll < 0.0f)          xscroll = 0.0f;
    if(yscroll > 10.0f - 2.0f)  yscroll = 10.0f - 2.0f;
    if(xscroll > 10.0f - 2.0f)  xscroll = 10.0f - 2.0f;

    for(int i = 0;i<selection.size;i++){
        Vector* add = (Vector*)Vector_Get(&selection,i);
        for(int j = 0;j<add->size;j++){
            CStr* cstr = (CStr*)Vector_Get(add,j);
            RenderRectAlpha(j * (lx + px) - xscroll * (lx + px),i * (ly + py) - yscroll * (ly + py),lx,ly,0x44FF0000);
        }
    }

    RenderRect(rect.p.x,rect.p.y,rect.d.x,rect.d.y,GREEN);
}
void Delete(AlxWindow* w){
    RLCamera_Free(&rlc);
    OpticalFlow_Free(&of);

    for(int i = 0;i<selection.size;i++){
        Vector* add = (Vector*)Vector_Get(&selection,i);
        for(int j = 0;j<add->size;j++){
            CStr* cstr = (CStr*)Vector_Get(add,j);
            CStr_Free(cstr);
        }
        Vector_Free(add);
    }
    Vector_Free(&selection);
}

int main(){
    if(Create("AR - Optical Flow - Scroll",OUTPUT_WIDTH,OUTPUT_HEIGHT,4,4,Setup,Update,Delete))
        Start();
    return 0;
}