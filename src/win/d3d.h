#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"
#include "platform/ald3d.h"

#include <windows.h>
#include <d3d8.h>
// FIXME: these are for gcc
#define D3DXINLINE static inline
#include <d3dx8.h>


#define AL_COLOR_TO_D3D(color) \
	(D3DCOLOR_COLORVALUE(color.r, color.g, color.b, color.a))

#define AL_COLOR_TO_ARGB_8888(color) \
	(((int)(color.a * 255) << 24) | \
	((int)(color.r * 255) << 16) | \
	((int)(color.g * 255) << 8) | \
	(int)(color.b * 255))


/* Flexible vertex formats */
#define D3DFVF_COLORED_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DFVF_TL_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)


typedef struct AL_SYSTEM_D3D AL_SYSTEM_D3D;
typedef struct AL_BITMAP_D3D AL_BITMAP_D3D;


/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_D3D
{
	AL_SYSTEM system; /* This must be the first member, we "derive" from it. */
};

struct AL_BITMAP_D3D
{
	AL_BITMAP bitmap; /* This must be the first member. */

	/* Driver specifics. */

	unsigned int texture_w;
	unsigned int texture_h;

	LPDIRECT3DTEXTURE8 video_texture;
	LPDIRECT3DTEXTURE8 system_texture;

	bool created;

	unsigned int xo;	/* offsets for sub bitmaps */
	unsigned int yo;
};


/* Colored vertex */
typedef struct D3D_COLORED_VERTEX
{
	float x;
	float y;
	float z;
	D3DCOLOR color;
} D3D_COLORED_VERTEX; 


/* Transformed & lit vertex */
typedef struct D3D_TL_VERTEX
{
	float x;		/* x position */
	float y;		/* y position */
	float z;		/* z position */
	D3DCOLOR color;		/* color */
	float tu;		/* texture coordinate */
	float tv;		/* texture coordinate */
} D3D_TL_VERTEX;


AL_SYSTEM_D3D *_al_d3d_system;

AL_BITMAP_INTERFACE *_al_bitmap_d3d_driver(int flag);
AL_SYSTEM_INTERFACE *_al_system_d3d_driver(void);
AL_DISPLAY_INTERFACE *_al_display_d3d_driver(void);

AL_VAR(LPDIRECT3D8, _al_d3d);
AL_VAR(LPDIRECT3DDEVICE8, _al_d3d_device);

AL_VAR(AL_DISPLAY_D3D *, _al_d3d_last_created_display);

AL_VAR(bool, _al_d3d_keyboard_initialized);

void _al_d3d_delete_from_vector(_AL_VECTOR *vec, void *item);

bool _al_d3d_init_display();
AL_BITMAP *_al_d3d_create_bitmap(AL_DISPLAY *d,
	unsigned int w, unsigned int h,
	int flags);
void _al_d3d_get_current_ortho_projection_parameters(float *w, float *h);
void _al_d3d_set_ortho_projection(float w, float h);

bool _al_d3d_init_keyboard();
void _al_d3d_set_kb_cooperative_level(HWND window);

HWND _al_d3d_create_hidden_window(void);
HWND _al_d3d_create_window(int, int);
HWND _al_d3d_win_get_window();
int _al_d3d_init_window();
void _al_d3d_win_ungrab_input();

void _al_d3d_release_default_pool_textures();
void _al_d3d_refresh_texture_memory();

/* Helper to get smallest fitting power of two. */
static inline int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}

void _al_convert_bitmap_data(void *src, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	unsigned int x, unsigned int y,
	unsigned int width, unsigned int height);

