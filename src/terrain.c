#include "game.h"

#define MAX_SEG 96
#define FAR 100000.0f
#define FIRST 720.0f
#define MIN_Y 220.0f
#define MAX_Y 400.0f
#define MIN_SPEED 3.0f
#define AIR_FRAMES 36.0f
#define MAX_GAP (MIN_SPEED * AIR_FRAMES)
#define RUNWAY 360.0f

enum { FLAT, SLOPE, GAP, LANDING };
typedef struct {
    int type;
    float start, end, y, slope;
    int ground;
} Seg;

static Seg segs[MAX_SEG];
static int seg_count;
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

static void add(int type, float start, float end, float y, float slope, int ground){
    Seg *s;
    if(seg_count>=MAX_SEG || end<=start) return;
    s=&segs[seg_count++];
    s->type=type; s->start=start; s->end=end;
    s->y=y; s->slope=slope; s->ground=ground;
}

static void fixed_route(void){
    add(FLAT,0,300,250,0,1);
    add(SLOPE,300,700,250,0.25f,1);
    ramp_start=560; ramp_end=680; ramp_y=315; ramp_slope=-0.25f;
    add(GAP,700,750,350,0,0);
    add(LANDING,750,1150,345,0.125f,1);
    add(FLAT,1150,FAR,395,0,1);
}

static void random_route(unsigned seed){
    float d=0,y=250,last_gap=-10000;
    rng=seed?seed:1u;
    add(FLAT,0,300,y,0,1);
    add(SLOPE,300,650,y,0.18f,1);
    y=313; d=FIRST;
    add(FLAT,650,d,y,0,1);

    while(seg_count<MAX_SEG-2 && d<15000){
        int pick=(int)(rnd()%10u);
        float len, slope, end;
        if(pick>=8 && d-last_gap<RUNWAY) pick=(int)(rnd()%8u);
        if(pick<4){
            len=rf(120,280);
            add(FLAT,d,d+len,y,0,1);
            d+=len;
        }else if(pick<8){
            float dir;
            len=rf(180,320);
            if(y>355) dir=-1;
            else if(y<255) dir=1;
            else dir=(rnd()&1u)?1:-1;
            slope=dir*rf(0.10f,0.22f);
            end=y+len*slope;
            if(end<MIN_Y || end>MAX_Y){
                slope=-slope; end=y+len*slope;
            }
            if(end<MIN_Y){ slope=(MIN_Y-y)/len; end=MIN_Y; }
            else if(end>MAX_Y){ slope=(MAX_Y-y)/len; end=MAX_Y; }
            add(SLOPE,d,d+len,y,slope,1);
            y=end; d+=len;
        }else{
            float gap=rf(48,92);
            float drop, ly, ll, ls;
            if(!terrain_gap_is_passable(gap,MIN_SPEED)) gap=MAX_GAP;
            add(GAP,d,d+gap,y,0,0);
            d+=gap;
            drop=rf(30,70);
            ly=clampf(y+drop,250,MAX_Y);
            ll=rf(100,170);
            ls=rf(0.06f,0.14f);
            if(ly+ll*ls>MAX_Y) ls=(MAX_Y-ly)/ll;
            add(LANDING,d,d+ll,ly,ls,1);
            y=clampf(ly+ll*ls,MIN_Y,MAX_Y);
            d+=ll;
            last_gap=d;
        }
    }
    if(seg_count<MAX_SEG) add(FLAT,d,FAR,y,0,1);
}

static int find_seg(float distance){
    int i=0;
    while(i+1<seg_count && segs[i+1].start<=distance) i++;
    return i;
}

static void ensure(void){
    if(!seg_count) terrain_init(0);
}

void terrain_init(unsigned seed){
    seg_count=0;
    cur_seed=seed;
    ramp_start=ramp_end=ramp_y=ramp_slope=0;
    if(seed) random_route(seed); else fixed_route();
}

unsigned terrain_seed(void){
    return cur_seed;
}

int terrain_gap_is_passable(float width, float speed){
    if(width<0 || speed<=0) return 0;
    return width<=speed*AIR_FRAMES;
}

Ground ground_at(float distance){
    Seg *s;
    Ground g={0,0,0};
    ensure();
    s=&segs[find_seg(distance)];
    if(s->ground){
        g.has_ground=1;
        g.y=s->y+(distance-s->start)*s->slope;
        g.slope=s->slope;
    }else{
        g.y=s->y;
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

float terrain_safe_distance(void){
    int i;
    ensure();
    for(i=0;i<seg_count;i++){
        Seg *s=&segs[i];
        if(s->ground && s->start>=600 && s->end-s->start>=100)
            return s->start+30;
    }
    return 0;
}
