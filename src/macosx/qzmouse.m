/*         ______   ___    ___
*        /\  _  \ /\_ \  /\_ \
*        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
*         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
*          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
*           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
*            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
*                                           /\____/
*                                           \_/__/
*
*      MacOS X mouse driver.
*
*      By Angelo Mottola.
*
*      Modified for 4.9 mouse API by Peter Hull.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro5.h"
#include "allegro5/a5_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"
#include "./osxgl.h"
#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif

typedef struct ALLEGRO_MOUSE AL_MOUSE;
typedef struct ALLEGRO_MOUSE_STATE AL_MOUSE_STATE;

static bool osx_init_mouse(void);
static unsigned int osx_get_mouse_num_buttons(void);
static unsigned int osx_get_mouse_num_axes(void);
static bool osx_set_mouse_axis(int axis, int value);
static ALLEGRO_MOUSE* osx_get_mouse(void);

/* Mouse info - includes extra info for OS X */
static struct {
	ALLEGRO_MOUSE parent;
	unsigned int button_count;
	unsigned int axis_count;
	int minx, miny, maxx, maxy;
	ALLEGRO_MOUSE_STATE state;
	float z_axis, w_axis;
	BOOL captured;
	NSCursor* cursor;
} osx_mouse;

/* _al_osx_switch_keyboard_focus:
 *  Handle a focus switch event.
 */
void _al_osx_switch_mouse_focus(ALLEGRO_DISPLAY *dpy, bool switch_in)
{
	_al_event_source_lock(&osx_mouse.parent.es);

   if (switch_in)
      osx_mouse.state.display = dpy;
   else
      osx_mouse.state.display = NULL;

	_al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_get_mouse:
* Return the Allegro mouse structure
*/
static ALLEGRO_MOUSE* osx_get_mouse(void)
{
	return (ALLEGRO_MOUSE*) &osx_mouse.parent;
}

/* _al_osx_mouse_generate_event:
* Convert an OS X mouse event to an Allegro event
* and push it into a queue.
* First check that the event is wanted.
*/
void _al_osx_mouse_generate_event(NSEvent* evt, ALLEGRO_DISPLAY* dpy)
{
	NSPoint pos;
	int type, b_change = 0, dx = 0, dy = 0, dz = 0, dw = 0, b = 0;
	switch ([evt type])
	{
		case NSMouseMoved:
			type = ALLEGRO_EVENT_MOUSE_AXES;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		case NSLeftMouseDragged:
		case NSRightMouseDragged:
		case NSOtherMouseDragged:
			type = ALLEGRO_EVENT_MOUSE_AXES;
			b = [evt buttonNumber]+1;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		case NSLeftMouseDown:
		case NSRightMouseDown:
		case NSOtherMouseDown:
			type = ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
			b = [evt buttonNumber]+1;
			b_change = 1;
			break;
		case NSLeftMouseUp:
		case NSRightMouseUp:
		case NSOtherMouseUp:
			type = ALLEGRO_EVENT_MOUSE_BUTTON_UP;
			b = [evt buttonNumber]+1;
			b_change = 1;
			break;
		case NSScrollWheel:
			type = ALLEGRO_EVENT_MOUSE_AXES;
			dx = 0;
			dy = 0;
			osx_mouse.w_axis += [evt deltaX];
			osx_mouse.z_axis += [evt deltaY];
			dw = osx_mouse.w_axis - osx_mouse.state.w;
			dz = osx_mouse.z_axis - osx_mouse.state.z;
			break;
		case NSMouseEntered:
			type = ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY;
			b = [evt buttonNumber]+1;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		case NSMouseExited:
			type = ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY;
			b = [evt buttonNumber]+1;
			dx = [evt deltaX];
			dy = [evt deltaY];
			break;
		default:
			return;
	}
	pos = [evt locationInWindow];
	BOOL within = true;			
	if ([evt window])
	{
		NSRect frm = [[[evt window] contentView] frame];
		within = NSMouseInRect(pos, frm, NO);
		// Y-coordinates in OS X start from the bottom.
		pos.y = NSHeight(frm) - pos.y;
	}
	else 
	{
      pos.y = [[NSScreen mainScreen] frame].size.height - pos.y;
	}
	_al_event_source_lock(&osx_mouse.parent.es);
	if ((within || osx_mouse.captured) && _al_event_source_needs_to_generate_event(&osx_mouse.parent.es))
	{
		ALLEGRO_EVENT new_event;
		ALLEGRO_MOUSE_EVENT* mouse_event = &new_event.mouse;
		mouse_event->type = type;
		// Note: we use 'allegro time' rather than the time stamp 
		// from the event 
		mouse_event->timestamp = al_current_time();
      mouse_event->display = dpy;
		mouse_event->button = b;
		mouse_event->x = pos.x;
		mouse_event->y = pos.y; 
		mouse_event->z = osx_mouse.z_axis;
		mouse_event->dx = dx;
		mouse_event->dy = dy;
		mouse_event->dz = dz;
		_al_event_source_emit_event(&osx_mouse.parent.es, &new_event);
	}
	// Record current state
	osx_mouse.state.x = pos.x;
	osx_mouse.state.y = pos.y;
	osx_mouse.state.w = osx_mouse.w_axis;
	osx_mouse.state.z = osx_mouse.z_axis;
	if (b_change)
		osx_mouse.state.buttons ^= (1<<(b-1));
	_al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_init_mouse:
*  Initializes the driver.
*/
static bool osx_init_mouse(void)
{
	HID_DEVICE_COLLECTION devices={0,0,NULL};
	int i = 0, j = 0;
	int axes = 0, buttons = 0;
	HID_DEVICE* device = nil;
	NSString* desc = nil;
	
	if (floor(NSAppKitVersionNumber) <= NSAppKitVersionNumber10_1) {
		/* On 10.1.x mice and keyboards aren't available from the HID Manager,
		* so we can't autodetect. We assume an 1-button mouse to always be
		* present.
		*/
		buttons = 1;
		axes = 2;
	}
	else {
		osx_hid_scan(HID_MOUSE, &devices);
		if (devices.count > 0) 
		{
            device=&devices.devices[i];
            buttons = 0;
            axes = 0;
            for (j = 0; j < device->num_elements; j++) 
			{
				switch (device->element[j].type)
				{
					case HID_ELEMENT_BUTTON:
                        buttons ++;
                        break;
					case HID_ELEMENT_AXIS:
					case HID_ELEMENT_AXIS_PRIMARY_X:
					case HID_ELEMENT_AXIS_PRIMARY_Y:
					case HID_ELEMENT_STANDALONE_AXIS:
                        axes ++;
                        break;
				}
			}
            desc = [NSString stringWithFormat: @"%s %s",
 device->manufacturer ? device->manufacturer : "",
		   device->product ? device->product : ""];
		}
		osx_hid_free(&devices);
	}
	if (buttons <= 0) return false;
	_al_event_source_init(&osx_mouse.parent.es);
	osx_mouse.button_count = buttons;
	osx_mouse.axis_count = axes;
	memset(&osx_mouse.state, 0, sizeof(ALLEGRO_MOUSE_STATE));
   _al_osx_mouse_was_installed(YES);
	return true;
}

/* osx_exit_mouse:
* Shut down the mouse driver
*/
static void osx_exit_mouse(void)
{
   _al_osx_mouse_was_installed(NO);
}

/* osx_get_mouse_num_buttons:
* Return the number of buttons on the mouse
*/
static unsigned int osx_get_mouse_num_buttons(void)
{
	return osx_mouse.button_count;
}

/* osx_get_mouse_num_buttons:
* Return the number of buttons on the mouse
*/
static unsigned int osx_get_mouse_num_axes(void)
{
	return osx_mouse.axis_count;
}


static void osx_get_mouse_state(ALLEGRO_MOUSE_STATE *ret_state)
{
	_al_event_source_lock(&osx_mouse.parent.es);
	memcpy(ret_state, &osx_mouse.state, sizeof(ALLEGRO_MOUSE_STATE));
	_al_event_source_unlock(&osx_mouse.parent.es);
}

/* osx_set_mouse_axis:
* Set the axis value of the mouse
*/
static bool osx_set_mouse_axis(int axis, int value) 
{
	_al_event_source_lock(&osx_mouse.parent.es);	
	switch (axis)
	{
		case 0:
		case 1:
			// By design, this doesn't apply to (x, y)
			break;
		case 2:
			osx_mouse.z_axis = value;
			return true;
		case 3:
			osx_mouse.w_axis = value;
			return true;
	}
	_al_event_source_unlock(&osx_mouse.parent.es);
	return false;
}
/* Mouse driver */
static ALLEGRO_MOUSE_DRIVER osx_mouse_driver =
{
   0, //int  msedrv_id;
   "OSXMouse", //const char *msedrv_name;
   "Driver for Mac OS X",// const char *msedrv_desc;
   "OSX Mouse", //const char *msedrv_ascii_name;
   osx_init_mouse, //AL_METHOD(bool, init_mouse, (void));
   osx_exit_mouse, //AL_METHOD(void, exit_mouse, (void));
   osx_get_mouse, //AL_METHOD(ALLEGRO_MOUSE*, get_mouse, (void));
   osx_get_mouse_num_buttons, //AL_METHOD(unsigned int, get_mouse_num_buttons, (void));
   osx_get_mouse_num_axes, //AL_METHOD(unsigned int, get_mouse_num_axes, (void));
   NULL, //AL_METHOD(bool, set_mouse_xy, (int x, int y));
   osx_set_mouse_axis, //AL_METHOD(bool, set_mouse_axis, (int which, int value));
   NULL, //AL_METHOD(bool, set_mouse_range, (int x1, int y1, int x2, int y2));
   osx_get_mouse_state, //AL_METHOD(void, get_mouse_state, (ALLEGRO_MOUSE_STATE *ret_state));
};
ALLEGRO_MOUSE_DRIVER* _al_osx_get_mouse_driver(void)
{
   return &osx_mouse_driver;
}

/* list the available drivers */
_DRIVER_INFO _al_mouse_driver_list[] =
{
   {  1, &osx_mouse_driver, 1 },
   {  0,  NULL,  0  }
};

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
