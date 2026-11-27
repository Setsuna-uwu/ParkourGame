#include "game.h"

#define MAX_SEG 96
#define MAX_ITEM 96
#define FAR 100000.0f
#define FIRST 720.0f
#define MIN_Y 220.0f
#define MAX_Y 400.0f
#define MIN_SPEED 3.0f
#define RUNWAY 360.0f

enum { FLAT, SLOPE, CURVE, GAP, DROP };
typedef struct {
    int type;
    float start, end, y, y1, slope;
    int ground;
} Seg;

static Seg segs[MAX_SEG];
static Pickup items[MAX_ITEM];
static int seg_count, item_count;
static unsigned cur_seed, rng;
static float ramp_start, ramp_end, ramp_y, ramp_slope;

static float clampf(float v, float lo, float hi){
    return v<lo?lo:v>hi?hi:v;
}

static unsigned rnd(void){
    unsigned x=rng;
    x^=x<<13; x^=x>>17; x^=x<<5;
    if(!x) x=0x6D2B79F5u;
    rng=x;
    return x;
}

static float rf(float lo, float hi){
    return lo+(hi-lo)*(float)(rnd()>>8)*(1.0f/16777216.0f);
}

static void add(int type, float start, float end, float y, float y1, float slope, int ground){
    Seg *s;
    if(seg_count>=MAX_SEG || end<=start) return;
    s=&segs[seg_count++];
    s->type=type; s->start=start; s->end=end;
    s->y=y; s->y1=y1; s->slope=slope; s->ground=ground;
}

static void fixed_route(void){
    add(FLAT,0,300,250,250,0,1);
    add(SLOPE,300,700,250,350,0.25f,1);
    ramp_start=560; ramp_end=680; ramp_y=315; ramp_slope=-0.25f;
    add(GAP,700,750,350,350,0,0);
    add(SLOPE,750,1150,345,395,0.125f,1);
    add(FLAT,1150,FAR,395,395,0,1);
}

static void random_route(unsigned seed){
    float d=0,y=250,last_gap=-10000;
    rng=seed?seed:1u;
    add(FLAT,0,300,y,y,0,1);
    add(CURVE,300,650,y,313,0,1);
    y=313; d=FIRST;
    add(FLAT,650,d,y,y,0,1);

    while(seg_count<MAX_SEG-2 && d<15000){
        int pick=(int)(rnd()%10u);
        float len, y1, gap, drop;
        if(pick>=7 && d-last_gap<RUNWAY) pick=(int)(rnd()%7u);
        if(pick<3){
            len=rf(120,280);
            add(FLAT,d,d+len,y,y,0,1);
            d+=len;
        }else if(pick<7){
            len=rf(180,320);
            float dir=(y>350)?-1.0f:(y<260)?1.0f:((rnd()&1u)?1.0f:-1.0f);
            y1=clampf(y+dir*rf(25,55),MIN_Y,MAX_Y);
            add(CURVE,d,d+len,y,y1,0,1);
            y=y1; d+=len;
        }else{
            int is_drop=pick==9;
            gap=is_drop?rf(18,32):rf(70,95);
            drop=is_drop?rf(55,85):rf(25,45);
            if(!terrain_gap_is_passable(gap,MIN_SPEED,0,drop)) gap=is_drop?30.0f:90.0f;
            add(is_drop?DROP:GAP,d,d+gap,y,y,0,0);
            d+=gap;
            y=clampf(y+drop,250,MAX_Y);
            len=rf(100,170);
            y1=clampf(y+rf(-15,20),MIN_Y,MAX_Y);
            add(CURVE,d,d+len,y,y1,0,1);
            d+=len; y=y1; last_gap=d;
        }
    }
    if(seg_count<MAX_SEG) add(FLAT,d,FAR,y,y,0,1);
}

static int find_seg(float distance){
    int i=0;
    while(i+1<seg_count && segs[i+1].start<=distance) i++;
    return i;
}

static void ensure(void){
    if(!seg_count) terrain_init(0);
}

static void item(int kind, float d, float y){
    if(item_count>=MAX_ITEM) return;
    items[item_count].id=(unsigned)item_count;
    items[item_count].kind=(unsigned)kind;
    items[item_count].distance=d;
    items[item_count].y=y;
    item_count++;
}

static void make_items(void){
    int i,j,life=0,rocks=0;
    for(i=0;i<seg_count;i++){
        Seg *s=&segs[i];
        float len=s->end-s->start;
        if(s->type==GAP || s->type==DROP){
            for(j=0;j<3;j++)
                item(ITEM_COIN,s->start+len*(j+1)/4.0f,s->y-40-(j==1?14:0));
            continue;
        }
        if(!s->ground) continue;
        if(len>=110 && (i%2==0 || len>300)){
            for(j=0;j<3;j++){
                float d=s->start+25+j*24;
                item(ITEM_COIN,d,ground_at(d).y-12);
            }
        }
        if(s->type==FLAT && i%3==1){
            item(ITEM_ROCK,s->start+105,ground_at(s->start+105).y-8);
            rocks=1;
        }
        if(!life && s->type==FLAT && s->start>700 && len>=180){
            item(ITEM_LIFE,s->start+140,ground_at(s->start+140).y-18);
            life=1;
        }
    }
    if(!life) for(i=0;i<seg_count;i++)
        if(segs[i].ground && segs[i].end-segs[i].start>100 && segs[i].start>600){
            item(ITEM_LIFE,segs[i].start+90,ground_at(segs[i].start+90).y-18);
            break;
        }
    if(!rocks) for(i=0;i<seg_count;i++)
        if(segs[i].ground && segs[i].end-segs[i].start>120){
            item(ITEM_ROCK,segs[i].start+90,ground_at(segs[i].start+90).y-8);
            break;
        }
}

void terrain_init(unsigned seed){
    seg_count=item_count=0;
    cur_seed=seed;
    ramp_start=ramp_end=ramp_y=ramp_slope=0;
    if(seed) random_route(seed); else fixed_route();
    make_items();
}

unsigned terrain_seed(void){
    return cur_seed;
}

int terrain_gap_is_passable(float width, float speed, float slope, float drop){
    float air=34.0f+drop*0.06f-slope*10.0f;
    if(air<30) air=30;
    return width>0 && speed>0 && width<=speed*air;
}

Ground ground_at(float distance){
    Seg *s;
    Ground g={0,0,0};
    ensure();
    s=&segs[find_seg(distance)];
    if(!s->ground){
        g.y=s->y;
        return g;
    }
    g.has_ground=1;
    if(s->type==CURVE){
        float t=(distance-s->start)/(s->end-s->start);
        float u=t*t*(3-2*t);
        g.y=s->y+(s->y1-s->y)*u;
        g.slope=(s->y1-s->y)*6*t*(1-t)/(s->end-s->start);
    }else{
        g.y=s->y+(distance-s->start)*s->slope;
        g.slope=s->slope;
    }
    return g;
}

Ground ramp_at(float distance){
    Ground g={0,0,0};
    ensure();
    if(ramp_end>ramp_start && distance>=ramp_start && distance<ramp_end){
        g.has_ground=1;
        g.y=ramp_y+(distance-ramp_start)*ramp_slope;
        g.slope=ramp_slope;
    }
    return g;
}

static int rock_near(float d){
    int i;
    for(i=0;i<item_count;i++)
        if(items[i].kind==ITEM_ROCK && items[i].distance>d-80 && items[i].distance<d+80) return 1;
    return 0;
}

float terrain_safe_distance(float from){
    int i,k;
    ensure();
    for(i=0;i<seg_count;i++){
        Seg *s=&segs[i];
        float d;
        if(!s->ground || s->type==GAP || s->type==DROP || s->end-s->start<100) continue;
        d=clampf(s->start+30,from+20,s->end-40);
        for(k=0;k<3;k++,d+=70)
            if(d+40<s->end && !rock_near(d)) return d;
    }
    return 0;
}

const Pickup *terrain_pickups(int *count){
    ensure();
    if(count) *count=item_count;
    return items;
}
