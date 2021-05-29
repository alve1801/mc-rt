#include <unistd.h>
#include <thread>
#include "perlin.h"
#include "quat.h"
#define rad 360
#define fov 2 // basically a corrective factor

int mod(int a,int b){a%=b;return(a>=0?a:b+a);}
int min(int a,int b){return(a>b?b:a);}
int max(int a,int b){return(a<b?b:a);}
int abs(int a){return a>0?a:-a;}
float sq(float a){return a*a;}

class vi{public:
	int x,y,z;
	vi(int x=0,int y=0,int z=0):x(x),y(y),z(z){}
	vi operator+(vi a){return vi(x+a.x,y+a.y,z+a.z);}
	vi operator-(vi a){return vi(x-a.x,y-a.y,z-a.z);}
	vi operator+(int a){return vi(x+a,y+a,z+a);}
	vi operator-(int a){return *this+(-a);}
	bool operator==(vi a){return x==a.x&&y==a.y&&z==a.z;}
	bool operator!=(vi a){return !(*this==a);}
	vi operator>>(int a){return vi((int)x>>a,(int)y>>a,(int)z>>a);} // XXX do we need this
	void print(){printf("%i %i %i\n",x,y,z);}
};

vi qvi(Q a){return vi((int)a.x,(int)a.y,(int)a.z);}

class Chunk{public:
	vi loc;
	char data[16*16*16]; // indicates block types XXX should prolly use an enum...
	Chunk(){}
	Chunk(vi l):loc(l){
		printf("newchunk ");l.print();
		for(int x=0;x<16;x++)for(int y=0;y<16;y++)for(int z=0;z<16;z++)
		data[(x*16+y)*16+z]=(l.z*16+z>(perlin::perl(l.x*16+x,l.y*16+y,4)>>6))?0:1;
		//data[(x*16+y)*16+z]=l.z*16+z+l.x+l.y>0?0:1;
		//data[(x*16+y)*16+z]=l.z*16+z>0?0:1;
	}

	Chunk(FILE*f,vi l){
		printf("loadchunk\n");
		// XXX try to load from file
		//Chunk(l); XXX gdi
	}

	char getblock(vi l){return data[(mod(l.x,16)*16+mod(l.y,16))*16+mod(l.z,16)];}
};

class Entity{public:
	Q loc,rot,dir;
	Entity(Q loc=Q()):loc(loc),dir(Q()),rot(Q().R()){}
};

char ind(char*data,int s,vi loc){
	//printf("fetch %i : ",s);loc.print();
	s*=2;
	return data[(loc.x*s+loc.y)*s+loc.z];
	if(data[(loc.x*s+loc.y)*s+loc.z]){
		printf("hit!\n");
		return 1;
	}
	return 0;
}

class World{public:
	vector<Entity>entities;
	vector<Chunk>chunks;
	FILE*f; // savefile
	World(){}
	World(char*filename){} // XXX

	char getblock(vi l){
		for(Chunk c:chunks)
			if(c.loc==l>>4)
				return c.getblock(l);
		// XXX do we want to create a new chunk?
		//chunks.push_back(Chunk(f,l>>4));
		chunks.push_back(Chunk(l>>4));
		return chunks[chunks.size()-1].getblock(l);
	}

	char*getrange(char*data,vi loc,int r){
		// like the above, but returns a whole bunch of them (we can optimize it some (i hope))
		// also this definitely generates chunks
		for(int x=0;x<r*2;x++)for(int y=0;y<r*2;y++)for(int z=0;z<r*2;z++)
		data[(x*r*2+y)*r*2+z]=getblock(loc+vi(x-r,y-r,z-r));
		return data;
	}

	short*render(short*canvas,int w,int h,Entity p,int rend){
		char*worlddata=(char*)malloc(rend*rend*rend*8);
		worlddata=getrange(worlddata,qvi(p.loc),rend);

		vector<thread>threads;

		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, 0,   h/2, 0,   w/2, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h/2, h,   0,   w/2, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, 0,   h/2, w/2, w,   p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h/2, h,   w/2, w,   p,rend);});

		/* 9 threads, gives roughly the same performance tho
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, 0, h/3, 0, w/3, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, 0, h/3, w/3, w*2/3, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, 0, h/3, w*2/3, w, p,rend);});

		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h/3, h*2/3, 0, w/3, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h/3, h*2/3, w/3, w*2/3, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h/3, h*2/3, w*2/3, w, p,rend);});

		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h*2/3, h, 0, w/3, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h*2/3, h, w/3, w*2/3, p,rend);});
		threads.emplace_back([&](){render_part(canvas,w,h,worlddata, h*2/3, h, w*2/3, w, p,rend);});
		*/

		for(auto&t:threads)t.join();

		free(worlddata);

		return canvas;
	}

	void render_part(short*canvas,int w,int h,char*worlddata,int x1,int x2,int y1,int y2,Entity p,int rend){
		Q delta,current,corr=p.loc.round()-Q(rend,rend,rend);
		vi t;
		for(int sx=x1;sx<x2;sx++)for(int sy=y1;sy<y2;sy++){
			Q depth=Q(1000,1000,1000);

			Q rot=Q((float)(sy-w/2)/min(h,w)*fov,1,(float)(sx-h/2)/min(h,w)*fov).rot(p.rot.inv()) *-1;


			// X
			//delta=rot/fabs(rot.x);
			delta=rot*((rot.x>0?1:-1)/rot.x);
			current=p.loc+delta*fmod(p.loc.x,1)*(delta.x>0?-1:1);
			while((p.loc-current).sqabs()<rend*rend){
				t=qvi(current-corr);
				if(delta.x<0)t.x--;
				if(ind(worlddata,rend,t)){
					depth.x=(p.loc-current).abs();
					break;
				}
				current+=delta;
			}


			// Y
			delta=rot*((rot.y>0?1:-1)/rot.y);
			current=p.loc+delta*fmod(p.loc.y,1)*(delta.y>0?-1:1);
			while((p.loc-current).sqabs()<rend*rend){
				t=qvi(current-corr);
				if(delta.y<0)t.y--;
				if(ind(worlddata,rend,t)){
					depth.y=(p.loc-current).abs();
					break;
				}
				current+=delta;
			}


			// Z
			delta=rot*((rot.z>0?1:-1)/rot.z);
			current=p.loc+delta*fmod(p.loc.z,1)*(delta.z>0?-1:1);
			while((p.loc-current).sqabs()<rend*rend){
				t=qvi(current-corr);
				if(delta.z<0)t.z--;
				if(ind(worlddata,rend,t)){
					depth.z=(p.loc-current).abs();
					break;
				}
				current+=delta;
			}


			// XXX store exact collision point on each axis for texture mapping
			if(depth.x==1000&&depth.y==1000&&depth.z==1000)canvas[sx*w+sy]=0;
			else if(depth.x<depth.y&&depth.x<depth.z)canvas[sx*w+sy]=0x0f22;
			else if(depth.y<depth.z)canvas[sx*w+sy]=0x02f2;
			else canvas[sx*w+sy]=0x022f;
		}
	}

};
