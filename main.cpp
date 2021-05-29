#define OLC_PGE_APPLICATION
#include <vector>
#include <math.h>
#include <vector>
using namespace std;
#include "olc.h"
#include "ppm.h"
#include "obj.h"
#define width 400
#define height 300
#define render_distance 16

short newmap[height*width],oldmap[height*width];char font[1<<14],str[256],sp;

/* XXX the following are for the parser, and operate on space-separated, null-terminated strings
XXX these were copied from nodes, please copy them back when implemented
bool match_string(char*src,char*dest,int n){} // returns true if the nth substring of src is dest
char*get_string(char*src,int n){} // returns the nth substring
int get_int(char*src,int n){} // returns the nth substring, evaluated as an integer
*/

World world;Entity player;

class Game : public olc::PixelGameEngine{public:
	Game(){sAppName="Game";}
	bool OnUserCreate()override{
		for(int x=0;x<height;x++)
		for(int y=0;y<width;y++)
		newmap[x*width+y]=0;
		render();
		return 1;
	}

	olc::Pixel sga(short color){
	int r = (color&0x0f00)>>4;
	int g = (color&0x00f0);
	int b = (color&0x000f)<<4;
	r=r|r>>4;g=g|g>>4;b=b|b>>4;
	return olc::Pixel(r,g,b);}

	void p(short c,int x,int y){Draw(y,x,sga(c));}

	void pc(char c,int sx,int sy,short fgc,short bgc){
		for(int x=0;x<8;x++)for(int y=0;y<8;y++)
		newmap[(sx*8+x)*width+sy*8+y]=(font[(unsigned char)c*8+x]&(char)(128>>y))?fgc:bgc;
	}

	void ps(const char*c,int sx,int sy,short fgc,short bgc){
		for(int i=0;c[i];i++){
			pc(c[i]==10?' ':c[i],sx,sy++,fgc,bgc);
			if(sy==width||c[i]==10){sy=0;sx++;} // XXX do we want to reset at initial xy?
		}
	}

	void draw_sprite(sprite s,int dx,int dy){ // XXX
		for(int x=0;x<s.x;x++)for(int y=0;y<s.y;y++)
		newmap[(dx+x)*width+dy+y]=s.data[x*s.y+y];
	}

	void refresh(){
		for(int x=0;x<height;x++)
		for(int y=0;y<width;y++)
		if(newmap[x*width+y]!=oldmap[x*width+y]){
			p(newmap[x*width+y],x,y);
			oldmap[x*width+y]=newmap[x*width+y];
		}
	}

	void render(){
		world.render(newmap,width,height,player,render_distance);
		newmap[height*width/2+width/2]=0x0fff;
		// interface overlay
	}

	bool OnUserUpdate(float fElapsedTime)override{
		int x=GetMouseY(),y=GetMouseX();
		if(GetKey(olc::ESCAPE).bPressed)return 0;

		float inc=.2;
		if(GetKey(olc::CTRL).bHeld)inc=.02;

		if(GetKey(olc::SPACE).bHeld)player.loc.z+=inc;
		if(GetKey(olc::SHIFT).bHeld)player.loc.z-=inc;

		Q roll  = Q(0,0,0,1);//.rot(player.rot.inv());
		Q pitch = Q(0,1,0,0).rot(player.rot.inv());
		Q yaw   = Q(0,0,1,0).rot(player.rot.inv());
			yaw   = Q(0,yaw.x,yaw.y,0).norm();

		if(GetKey(olc::W).bHeld)player.loc-=yaw*inc;
		if(GetKey(olc::S).bHeld)player.loc+=yaw*inc;
		if(GetKey(olc::A).bHeld)player.loc+=pitch*inc;
		if(GetKey(olc::D).bHeld)player.loc-=pitch*inc;

		if(GetKey(olc::UP).bHeld){
		pitch.w=-inc/10;
		player.rot *= pitch.R();
		}

		if(GetKey(olc::DOWN).bHeld){
		pitch.w=inc/10;
		player.rot *= pitch.R();
		}

		if(GetKey(olc::LEFT).bHeld){
		roll.w=inc/10;
		player.rot *= roll.R();
		}

		if(GetKey(olc::RIGHT).bHeld){
		roll.w=-inc/10;
		player.rot *= roll.R();
		}

		printf("(%i, %.3f s/f)\n",world.chunks.size(),1/fElapsedTime);
		player.loc.print();
		player.rot.print();

		// get server data somewhere here?
		// get user data
		// calculate response (physics etc)
		// chunk offloading etc

		render();
		refresh();
		putchar(10);
		return 1;
	}
};

int main(){
	FILE*f=fopen("font","r");for(int i=0;i<3200;i++)*(font+i)=getc(f);fclose(f);
	player.loc=Q(2,2.8,5.5);
	player.rot=Q(.921,.237,-.078,-.3).norm();

	Game game;if(game.Construct(width,height,2,2))game.Start();return(0);
}
