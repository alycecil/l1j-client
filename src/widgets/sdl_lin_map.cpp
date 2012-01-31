#include <SDL.h>
#include "SDL_endian.h"
#include <stdio.h>

#include "client.h"
#include "resources/coord.h"
#include "resources/map_coord.h"
#include "resources/pixel_coord.h"
#include "resources/screen_coord.h"
#include "resources/tile.h"
#include "globals.h"
#include "resources/pack.h"
#include "sdl_lin_map.h"
#include "sdl_widget.h"

sdl_lin_map::sdl_lin_map(tile *thetiles, client *who, int x, int y, int w, int h)
	: sdl_widget(x, y, who)
{
	tile_data = thetiles;
	one = 0;
	this->who = who;
	
	for (int i = 0; i < 4; i++)
	{
		segs[i].graphic = 0;
		segs[i].mapdata = 0;
	}
	
	one = new sdl_graphic(x, y, w, h);
}

sdl_lin_map::~sdl_lin_map()
{
	for (int i = 0; i < 4; i++)
	{
		if (segs[i].graphic != 0)
			delete segs[i].graphic;
		if (segs[i].mapdata != 0)
			delete segs[i].mapdata;
	}
}

//southeast	+24, +12
//northwest	-24, -12
//northeast	+24, -12
//southwest	-24, +12
//north		  0, +24
//south		  0, -24
//east		  0, +48
//west		  0, -48
	
lin_map_segment sdl_lin_map::get_map(int mapnum, int x, int y)
{
//	Uint32 timecheck = SDL_GetTicks();
	lin_map_segment ret;
	
//begin loading map data
	int modx, mody;
	modx = (x>>6) + 0x7e00;
	mody = (y>>6) + 0x7e00;
	
	int beg_x2, beg_y2;
	beg_x2 = x & (int)~0x3F;
	beg_y2 = y & (int)~0x3F;
	
	char name[256];
	int size;
	sprintf(name, "map/%d/%04x%04x.s32", mapnum, modx, mody);
	char *buffer;
	SDL_RWops *sdl_buf;
	buffer = (char*)who->getfiles->load_file(name, &size, FILE_REGULAR1, 0);
	if (buffer == 0)
	{
		ret.graphic = 0;
		ret.mapdata = 0;
		ret.map = 0;
		ret.x = 0;
		ret.y = 0;
		ret.offsetx = 0;
		ret.offsety = 0;
//		printf("Took %d millis to load nothing\n", SDL_GetTicks() - timecheck);
		return ret;
	}
	
	sdl_buf = SDL_RWFromConstMem(buffer, size);
	ret.mapdata = new lin_map_data;
	
	//read tile data
	for (int tx = 0; tx < 64; tx++)
	{
		for (int ty = 0; ty < 64; ty++)
		{
			SDL_RWread(sdl_buf, &(ret.mapdata->floor[ty][tx*2]), 4, 1);
			SDL_RWread(sdl_buf, &(ret.mapdata->floor[ty][tx*2+1]), 4, 1);
		}
	}
	
	//mystery data
	SDL_RWread(sdl_buf, &ret.mapdata->num_unknown2, 2, 1);
	
	if (ret.mapdata->num_unknown2 > 0)
	{	//7d068
		unsigned char *waste = new unsigned char[ret.mapdata->num_unknown2 * 6];
		SDL_RWread(sdl_buf, waste, 6, ret.mapdata->num_unknown2);
		delete [] waste;
	}	
		
	//read attributes for each half tile
	for (int tx = 0; tx < 64; tx++)
	{
		for (int ty = 0; ty < 64; ty++)
		{
			ret.mapdata->attr[tx][ty*2] = 0;
			ret.mapdata->attr[tx][ty*2+1] = 0;
			SDL_RWread(sdl_buf, &ret.mapdata->attr[ty][tx*2], 2, 1);
			SDL_RWread(sdl_buf, &ret.mapdata->attr[ty][tx*2+1], 2, 1);
		}
	}
	
	
	
	int bla;
	SDL_RWread(sdl_buf, &bla, 4, 1);
	printf("amount for HideObjs is %d\n", bla);
	
	for (int i = 0; i < bla; i++)
	{
		short a;
		SDL_RWread(sdl_buf, &a, 2, 1);
		SDL_RWread(sdl_buf, &a, 2, 1);
		printf("\t%d\n", a);
		for (int j = 0; j < a; j++)
		{
			unsigned char b, c;
			SDL_RWread(sdl_buf, &b, 1, 1);
			SDL_RWread(sdl_buf, &c, 1, 1);
			if ((b == 0xcd) && (c == 0xcd))
			{
				char dummy[5];
				SDL_RWread(sdl_buf, dummy, 1, 5);
				//skip 5 bytes
				j--;
				a--;
				//decrement j and a
				//destination increments by 4
			}
			else
			{
				char dummy[5];
				SDL_RWread(sdl_buf, dummy, 1, 5);
				//read a byte
				//read an int
				//destination increments by 12
			}
		}
	}
	
	if (bla > 0)
	{	//7d12c
		//bla bla putObjecTile()
	}
	
	SDL_RWread(sdl_buf, &bla, 4, 1);
	printf("amount for HideSwitches is %d\n", bla);
	for (int i = 0; i < bla; i++)
	{
		unsigned char a, b, d;
		unsigned short c;
		SDL_RWread(sdl_buf, &a, 1, 1);
		SDL_RWread(sdl_buf, &b, 1, 1);
		SDL_RWread(sdl_buf, &c, 2, 1);
		SDL_RWread(sdl_buf, &d, 1, 1);
	}
	
	SDL_RWread(sdl_buf, &bla, 4, 1);
	printf("amount for tilesets is %d\nset: ", bla);
	for (int i = 0; i < bla; i++)
	{
		int set;
		SDL_RWread(sdl_buf, &set, 4, 1);
		printf("%d, ", set);
	}
	printf("\n");
	
	//portalList
	//array of Portal (uchar xOff, uchar yOff, targetmap, short tx, short ty)
	
	//TODO : check for end of file?
	unsigned short num_portals;
	SDL_RWread(sdl_buf, &num_portals, 2, 1);
	printf("There are %d portals ", num_portals);
	for (int i = 0; i < num_portals; i++)
	{	//7cf54
		char a, b, c;
		short d, e, f;
		
		SDL_RWread(sdl_buf, &a, 1, 1);
		//skip a bytes of file?
		SDL_RWread(sdl_buf, &b, 1, 1);
		SDL_RWread(sdl_buf, &c, 1, 1);
		SDL_RWread(sdl_buf, &d, 2, 1);
		SDL_RWread(sdl_buf, &e, 2, 1);
		SDL_RWread(sdl_buf, &f, 2, 1);
		printf("Data %d %d %d %d %d %d\n", a, b, c, d, e, f);
	}

	//if no at the end of file?
	{	//7d24c
		
	}
		
	printf("Finish loading\n");
	
	SDL_RWclose(sdl_buf);

//end loading map data

//	printf("Took %d millis to load a section\n", SDL_GetTicks() - timecheck);

	int beg_x, beg_y;
	beg_x = x & (int)~0x3F;
	beg_y = y & (int)~0x3F;
	
	ret.graphic = new sdl_graphic(0, 0, 3072, 1535);
	
	//draw all the tiles for the map section
	int offsetx, offsety;
	map_coord themap(x, y);
	screen_coord thescreen = themap.get_screen();
	offsetx = 0 - thescreen.get_x();
	offsety = 756 - thescreen.get_y();
	
	for (int tx = 0; tx < 64; tx++)
	{
		for (int ty = 0; ty < 64; ty++)
		{
			sdl_graphic *left, *right;
			map_coord tempmap(x + tx, y + ty);
			int dx, dy;
			int selal, selbl, selar, selbr;
			selal = ret.mapdata->floor[tx][ty*2]>>8;
			selbl = ret.mapdata->floor[tx][ty*2] & 0xFF;
			
			selar = ret.mapdata->floor[tx][ty*2+1]>>8;
			selbr = ret.mapdata->floor[tx][ty*2+1] & 0xFF;
			
			if (selal == 0)
				selal++;
			if (selar == 0)
				selar++;
			
			tile_data[selal].load(selal, who);
			tile_data[selar].load(selar, who);
			
			dx = tempmap.get_screen().get_x() + offsetx;
			dy = tempmap.get_screen().get_y() + offsety;
			left = tile_data[selal].get_tile_left(selbl);
			right = tile_data[selar].get_tile_right(selbr);
			
			left->drawat(dx, dy, ret.graphic->get_surf());
			right->drawat(dx+24, dy, ret.graphic->get_surf());
		}
	}

	ret.map = mapnum;
	ret.x = x & (~0x3F);
	ret.y = y & (~0x3F);
	ret.offsetx = offsetx;
	ret.offsety = offsety;
	
	printf("\t%s %d_%d_%d\n", name, mapnum, ret.x, ret.y);
	
//	sprintf(name, "%d_%d_%d.bmp", mapnum, ret.x, ret.y);
//	SDL_SaveBMP(ret.graphic->get_surf(), name);
	
//	printf("Took %d millis to load/draw a section\n", SDL_GetTicks() - timecheck);
	return ret;
}

void sdl_lin_map::check_sections()
{
	int width, height;
	width = one->getw();
	height = one->geth();
	
	int corner_x[4], corner_y[4];
	int goal_x[4], goal_y[4];
	int present[4];	//covers the 4 possible sections required
	int lowx, lowy;
	
	screen_coord corner1(0 - master_offsetx, 0 - master_offsety);
	corner_x[0] = corner1.get_map().get_x() & (~0x3F);
	corner_y[0] = corner1.get_map().get_y() & (~0x3F);

	screen_coord corner2(width - master_offsetx, 0 - master_offsety);
	corner_x[1] = corner2.get_map().get_x() & (~0x3F);
	corner_y[1] = corner2.get_map().get_y() & (~0x3F);
	
	screen_coord corner3(0 - master_offsetx, height - master_offsety);
	corner_x[2] = corner3.get_map().get_x() & (~0x3F);
	corner_y[2] = corner3.get_map().get_y() & (~0x3F);
	
	screen_coord corner4(width - master_offsetx, height - master_offsety);
	corner_x[3] = corner4.get_map().get_x() & (~0x3F);
	corner_y[3] = corner4.get_map().get_y() & (~0x3F);
	
	lowx = corner_x[0];
	lowy = corner_y[0];
	
	for (int i = 0; i < 4; i++)
	{
		present[i] = -1;
		if (corner_x[i] < lowx)
			lowx = corner_x[i];
		if (corner_y[i] < lowy)
			lowy = corner_y[i];
	}
	
	goal_x[0] = lowx;
	goal_y[0] = lowy;
	
	goal_x[1] = lowx;
	goal_y[1] = lowy + 64;
	
	goal_x[2] = lowx + 64;
	goal_y[2] = lowy;
	
	goal_x[3] = lowx + 64;
	goal_y[3] = lowy + 64;

	for (int i = 0; i < 4; i++)
	{	//check to see if each segment is loaded properly
		if ((segs[i].x != goal_x[i]) ||
			(segs[i].y != goal_y[i]) ||
			(segs[i].map != map))
		{
			int match = -1;
			for (int j = i + 1; j < 4; j++)
			{	//check the other segments to see if they match
				if ((segs[j].x == goal_x[i]) &&
					(segs[j].y == goal_y[i]) &&
					(segs[j].map == map))
				{
					match = j;
				}
			}
			if (match != -1)
			{	//swap the segments because a match was found
				lin_map_segment temp;
				temp = segs[i];
				segs[i] = segs[match];
				segs[match] = temp;
			}
			else
			{	//load a section because it's not already loaded
				segs[i] = get_map(map, goal_x[i], goal_y[i]);
			}
		}
	}
}

void sdl_lin_map::draw(SDL_Surface *display)
{
	SDL_FillRect(one->get_surf(), NULL, 0x1234);

	check_sections();
	for (int i = 0; i < 4; i++)
	{
		int temp_offx, temp_offy;
			
		if (segs[i].graphic != 0)
		{
			temp_offx = master_offsetx - segs[i].offsetx;
			temp_offy = master_offsety - segs[i].offsety;
		
			segs[i].graphic->drawat(temp_offx, temp_offy, one->get_surf());
		}
	}
	
	//TODO!
	//draws the full map onto display
	one->draw(display);
}

void sdl_lin_map::set_hotspot(int mapn, int x, int y)
{
	int width, height;
	
	map = mapn;
	width = one->getw();
	height = one->geth();
	
	map_coord themap(x, y);
	screen_coord thescreen = themap.get_screen();

	master_offsetx = (width/2) - thescreen.get_x();
	master_offsety = (height/2) - thescreen.get_y();	
}