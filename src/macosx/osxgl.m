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
*      MacOS X OpenGL gfx driver
*
*      By Peter Hull.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/platform/aintosx.h"
#include "./osxgl.h"
#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif

#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

/* Module Variables */
static BOOL _osx_mouse_installed = NO, _osx_keyboard_installed = NO;
static NSPoint last_window_pos;
static unsigned int next_display_group = 1;
static BOOL in_fullscreen = NO;

/* Module functions */
NSView* osx_view_from_display(ALLEGRO_DISPLAY* disp);
ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver(void);
ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver_win(void);
ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver_fs(void);
NSOpenGLContext* CreateShareableContext(NSOpenGLPixelFormat* fmt, unsigned int* group);

/* _al_osx_keyboard_was_installed:
 * Called by the keyboard driver when the driver is installed or uninstalled.
 * Set the variable so we can decide to pass events or not.
 */
void _al_osx_keyboard_was_installed(BOOL install) {
   _osx_keyboard_installed = install;
}

/* The main additions to this view are event-handling functions */
@interface ALOpenGLView : NSOpenGLView
{
	/* This is passed onto the event functions so we know where the event came from */
	ALLEGRO_DISPLAY* dpy_ptr;
}
-(void)setDisplay: (ALLEGRO_DISPLAY*) ptr;
-(ALLEGRO_DISPLAY*) display;
-(void) reshape;
-(BOOL) acceptsFirstResponder;
-(void) drawRect: (NSRect) rc;
-(void) keyDown:(NSEvent*) event;
-(void) keyUp:(NSEvent*) event; 
-(void) flagsChanged:(NSEvent*) event;
-(void) mouseDown: (NSEvent*) evt;
-(void) mouseUp: (NSEvent*) evt;
-(void) mouseDragged: (NSEvent*) evt;
-(void) rightMouseDown: (NSEvent*) evt;
-(void) rightMouseUp: (NSEvent*) evt;
-(void) rightMouseDragged: (NSEvent*) evt;
-(void) otherMouseDown: (NSEvent*) evt;
-(void) otherMouseUp: (NSEvent*) evt;
-(void) otherMouseDragged: (NSEvent*) evt;
-(void) mouseMoved: (NSEvent*) evt;
-(void) scrollWheel: (NSEvent*) evt;
-(void) viewDidMoveToWindow;
-(void) viewWillMoveToWindow: (NSWindow*) newWindow;
-(void) mouseEntered: (NSEvent*) evt;
-(void) mouseExited: (NSEvent*) evt;
/* Window delegate methods */
-(void) windowDidBecomeMain:(NSNotification*) notification;
-(void) windowDidResignMain:(NSNotification*) notification;
@end

/* _al_osx_mouse_was_installed:
 * Called by the mouse driver when the driver is installed or uninstalled.
 * Set the variable so we can decide to pass events or not, and notify all
 * existing displays that they need to set up their tracking rectangles.
 */
void _al_osx_mouse_was_installed(BOOL install) {
   unsigned int i;
   if (_osx_mouse_installed == install) {
      // done it already
      return;
   }
   _osx_mouse_installed = install;
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   for (i = 0; i < _al_vector_size(dpys); ++i) {
         ALLEGRO_DISPLAY* dpy = *(ALLEGRO_DISPLAY**) _al_vector_ref(dpys, i);
         NSView* view = osx_view_from_display(dpy);
         [[view window] setAcceptsMouseMovedEvents: _osx_mouse_installed];
   }
}

@implementation ALOpenGLView
/* setDisplay:
* Set the display this view is associated with
*/
-(void) setDisplay: (ALLEGRO_DISPLAY*) ptr {
	dpy_ptr = ptr;
}

/* display 
* return the display this view is associated with
*/
-(ALLEGRO_DISPLAY*) display {
	return dpy_ptr;
}

/* reshape
* Called when the view changes size */
- (void) reshape 
{
	if ([NSOpenGLContext currentContext] != nil) {
		NSRect rc = [self bounds];
		glViewport(0, 0, NSWidth(rc), NSHeight(rc));
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, NSWidth(rc), NSHeight(rc), 0, -1, 1);
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
      // To do: update the tracking rect
	}
}
/* acceptsFirstResponder
* Overridden to return YES, so that
* this view will receive events
*/
-(BOOL) acceptsFirstResponder
{
	return YES;
}

/* drawRect:
* Called when OS X wants the view to be redrawn
*/
-(void) drawRect: (NSRect) rc
{
	// To do: pass on notifications to the display queue
}

/* Keyboard event handler */
-(void) keyDown:(NSEvent*) event
{
	if (_osx_keyboard_installed)
		osx_keyboard_handler(TRUE, event, dpy_ptr);
}
-(void) keyUp:(NSEvent*) event 
{
	if (_osx_keyboard_installed)
		osx_keyboard_handler(FALSE, event, dpy_ptr);
}
-(void) flagsChanged:(NSEvent*) event
{
	if (_osx_keyboard_installed) {
		osx_keyboard_modifiers([event modifierFlags], dpy_ptr);
	}
}

/* Mouse handling 
 */
-(void) mouseDown: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
      osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseUp: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseDragged: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseDown: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseUp: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) rightMouseDragged: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseDown: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseUp: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) otherMouseDragged: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseMoved: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) scrollWheel: (NSEvent*) evt
{
	if (_osx_mouse_installed) 
	osx_mouse_generate_event(evt, dpy_ptr);
}
/* Cursor handling */
- (void) viewDidMoveToWindow {
   ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   dpy->tracking = [self addTrackingRect:[self bounds] owner:self userData:nil assumeInside:NO];
}

- (void) viewWillMoveToWindow: (NSWindow*) newWindow {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   if (([self window] != nil) && (dpy->tracking != 0)) {
      [self removeTrackingRect:dpy->tracking];
   }
}

-(void) mouseEntered: (NSEvent*) evt
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   if (dpy->show_cursor) {
      [dpy->cursor set];
   }
   else {
      [NSCursor hide];
   }
   osx_mouse_generate_event(evt, dpy_ptr);
}
-(void) mouseExited: (NSEvent*) evt
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy =  (ALLEGRO_DISPLAY_OSX_WIN*) dpy_ptr;
   if (!dpy->show_cursor) {
      [NSCursor unhide];
   }
	osx_mouse_generate_event(evt, dpy_ptr);
}

/* windowShouldClose:
* Veto the close and post a message
*/
- (BOOL)windowShouldClose:(id)sender
{
	ALLEGRO_EVENT_SOURCE* src = &([self display]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT* evt = _al_event_source_get_unused_event(src);
	evt->type = ALLEGRO_EVENT_DISPLAY_CLOSE;
	_al_event_source_emit_event(src, evt);
	_al_event_source_unlock(src);
	return NO;
}
/* Window switch in/out */
-(void) windowDidBecomeMain:(NSNotification*) notification
{
	ALLEGRO_EVENT_SOURCE* src = &([self display]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT* evt = _al_event_source_get_unused_event(src);
	evt->type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
	_al_event_source_emit_event(src, evt);
	_al_event_source_unlock(src);
}
-(void) windowDidResignMain:(NSNotification*) notification
{
	ALLEGRO_EVENT_SOURCE* src = &([self display]->es);
	_al_event_source_lock(src);
	ALLEGRO_EVENT* evt = _al_event_source_get_unused_event(src);
	evt->type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
	_al_event_source_emit_event(src, evt);
	_al_event_source_unlock(src);
}

/* End of ALOpenGLView implementation */
@end

/* osx_view_from_display:
 * given an ALLEGRO_DISPLAY, return the associated Cocoa View or nil
 * if fullscreen 
 */
NSView* osx_view_from_display(ALLEGRO_DISPLAY* disp)
{
	NSWindow* window = ((ALLEGRO_DISPLAY_OSX_WIN*) disp)->win;
   return window == nil ? nil : [window contentView];
}

/* set_current_display:
* Set the current windowed display to be current.
*/
bool set_current_display(ALLEGRO_DISPLAY* d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
	if (dpy->ctx != nil) {
		[dpy->ctx makeCurrentContext];
	}
   _al_ogl_set_extensions(d->ogl_extras->extension_api);
   return true;
}

/* Helper to set up GL state as we want it. */
static void setup_gl(ALLEGRO_DISPLAY *d)
{
	glViewport(0, 0, d->w, d->h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, d->w, d->h, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

static int decode_allegro_format(int format, int* glfmt, int* glsize, int* depth) {
	static int glformats[][4] = {
		/* Skip pseudo formats */
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0},
		/* Actual formats */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA, 32}, /* ARGB_8888 */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA, 32}, /* RGBA_8888 */
	{GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_BGRA, 16}, /* ARGB_4444 */
	{GL_RGB8, GL_UNSIGNED_BYTE, GL_BGR, 32}, /* RGB_888 */
	{GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB, 16}, /* RGB_565 */
	{0, 0, 0, 16}, /* RGB_555 */ //FIXME: how?
	{GL_INTENSITY, GL_UNSIGNED_BYTE, GL_COLOR_INDEX, 8}, /* PALETTE_8 */
	{GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA, 16}, /* RGBA_5551 */
	{0, 0, 0, 16}, /* ARGB_1555 */ //FIXME: how?
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA, 32}, /* ABGR_8888 */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA, 32}, /* XBGR_8888 */
	{GL_RGB8, GL_UNSIGNED_BYTE, GL_RGB, 32}, /* BGR_888 */
	{GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_BGR, 32}, /* BGR_565 */
	{0, 0, 0, 16}, /* BGR_555 */ //FIXME: how?
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA, 32}, /* RGBX_8888 */
	{GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA, 32}, /* XRGB_8888 */
	};
	// To do: map the pseudo formats
	if (format >= ALLEGRO_PIXEL_FORMAT_ARGB_8888 && format <= ALLEGRO_PIXEL_FORMAT_XRGB_8888) {
		if (glfmt != NULL) *glfmt = glformats[format][2];
		if (glsize != NULL) *glsize = glformats[format][1];
		if (depth != NULL) *depth = glformats[format][3];
		return 0;
	}
	else {
		return -1;
	}
}

/* The purpose of this object is to provide a receiver for "perform on main 
 * thread" messages - we can't call C function directly so we do it
 * like this.
 * The perform on main thread mechanism is a simple way to avoid threading
 * issues.
 */
@interface ALDisplayHelper : NSObject 
+(void) initialiseDisplay: (NSValue*) display_object;
+(void) destroyDisplay: (NSValue*) display_object;
+(void) runFullScreenDisplay: (NSValue*) display_object;
@end

@implementation ALDisplayHelper
+(void) initialiseDisplay: (NSValue*) display_object {
   ALLEGRO_DISPLAY_OSX_WIN* dpy = [display_object pointerValue];
	NSRect rc = NSMakeRect(0, 0, dpy->parent.w,  dpy->parent.h);
	NSWindow* win = dpy->win = [NSWindow alloc]; 
   unsigned int mask = (dpy->parent.flags & ALLEGRO_NOFRAME) ? NSBorderlessWindowMask : 
      (NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask);
	[win initWithContentRect: rc
				   styleMask: mask
					 backing: NSBackingStoreBuffered
					   defer: NO];
	int depth;
	decode_allegro_format(dpy->parent.format, &dpy->gl_fmt, 
      &dpy->gl_datasize, &depth);
   NSOpenGLPixelFormatAttribute attrs[] = 
	{
		NSOpenGLPFAColorSize,
		depth, 
		NSOpenGLPFAWindow,
		0
	};
	NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs];
	ALOpenGLView* view = [[ALOpenGLView alloc] initWithFrame: rc];
   dpy->ctx = CreateShareableContext(fmt, &dpy->display_group);
   [view setOpenGLContext: dpy->ctx];
	/* Hook up the view to its display */
	[view setDisplay: &dpy->parent];
	/* Realize the window on the main thread */
	[win setContentView: view];
	[win setDelegate: view];
	[win setReleasedWhenClosed: YES];
	[win setAcceptsMouseMovedEvents: _osx_mouse_installed];
	[win setTitle: @"Allegro"];
   if (NSEqualPoints(last_window_pos, NSZeroPoint)) {
      /* We haven't positioned a window before */
      [win center];
   }
   last_window_pos = [win cascadeTopLeftFromPoint:last_window_pos];
   [win makeKeyAndOrderFront:self];
	if (mask != NSBorderlessWindowMask) [win makeMainWindow];
	[view release];
}
+(void) destroyDisplay: (NSValue*) display_object {
   int i;
   ALLEGRO_DISPLAY_OSX_WIN* dpy = [display_object pointerValue];
   _al_vector_find_and_delete(&al_system_driver()->displays, &dpy);
   ALLEGRO_DISPLAY_OSX_WIN* other = NULL;
   // Check for other displays in this display group
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY_OSX_WIN* d = *(ALLEGRO_DISPLAY_OSX_WIN**) _al_vector_ref(dpys, i);
      if (d->display_group == dpy->display_group) {
         other = d;
         break;
      }
   }
   if (other != NULL) {
      // Found another compatible display. Transfer our bitmaps to it.
      _AL_VECTOR* bmps = &dpy->parent.bitmaps;
      for (i = 0; i<_al_vector_size(bmps); ++i) {
         *(ALLEGRO_BITMAP**) _al_vector_alloc_back(&other->parent.bitmaps) = *(ALLEGRO_BITMAP**)_al_vector_ref(bmps, i);
      }
   }
   else {
      // This is the last in its group. Convert all its bitmaps to mem bmps
      _AL_VECTOR* bmps = &dpy->parent.bitmaps;
      ALLEGRO_BITMAP** tmp = (ALLEGRO_BITMAP**) _AL_MALLOC(_al_vector_size(bmps) * sizeof(ALLEGRO_BITMAP*));
      for (i=0; i< _al_vector_size(bmps); ++i) {
         tmp[i] = *(ALLEGRO_BITMAP**) _al_vector_ref(bmps, i);
      }
      for (i=0; i< _al_vector_size(bmps); ++i) {
         _al_convert_to_memory_bitmap(tmp[i]);
      }
      _AL_FREE(tmp);
   }
   _al_vector_free(&dpy->parent.bitmaps);
   // Disconnect from its view or exit fullscreen mode
   [dpy->ctx clearDrawable];
   // Unlock the screen 
   if (dpy->parent.flags & ALLEGRO_FULLSCREEN) {
      CGDisplayRelease(CGMainDisplayID());
      in_fullscreen = FALSE;
   }
   else {
      // Destroy the containing window if there is one
      [dpy->win close];
      dpy->win = nil;      
   }
}
/* runFullScreen:
 * Capture the display and enter fullscreen mode. Do not leave this function
 * until full screen is cancelled
 */
+(void) runFullScreenDisplay: (NSValue*) display_object
{
   ALLEGRO_DISPLAY* display = (ALLEGRO_DISPLAY*) [display_object pointerValue];
   while (in_fullscreen) {
      NSEvent* event = [NSApp nextEventMatchingMask:NSAnyEventMask 
                                          untilDate:[NSDate distantFuture] 
                                             inMode:NSDefaultRunLoopMode 
                                            dequeue:YES];
      switch ([event type]) {
         case NSKeyDown:
            osx_keyboard_handler(TRUE,event,display);
            break;
         case NSKeyUp:
            osx_keyboard_handler(FALSE,event,display);
            break;
         case NSFlagsChanged:
            osx_keyboard_modifiers([event modifierFlags],display);
            break;
         case NSLeftMouseDown:
         case NSLeftMouseUp:
         case NSRightMouseDown:
         case NSRightMouseUp:
         case NSOtherMouseDown:
         case NSOtherMouseUp:
         case NSMouseMoved:
         case NSLeftMouseDragged:
         case NSRightMouseDragged:
         case NSOtherMouseDragged:
            if (_osx_mouse_installed) 
               osx_mouse_generate_event(event, display);
            break;
         default:
            [NSApp sendEvent: event];
            break;
      }
   }
}
/* End of ALDisplayHelper implementation */
@end

NSOpenGLContext* CreateShareableContext(NSOpenGLPixelFormat* fmt, unsigned int* group)
{
   // Iterate through all existing displays and try and find one that's compatible
   _AL_VECTOR* dpys = &al_system_driver()->displays;
   int i;
   NSOpenGLContext* compat = nil;
   
   for (i = 0; i < _al_vector_size(dpys); ++i) {
      ALLEGRO_DISPLAY_OSX_WIN* other = *(ALLEGRO_DISPLAY_OSX_WIN**) _al_vector_ref(dpys, i);
      compat = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext: other->ctx];
      if (compat != nil) {
      // OK, we can share with this one
         *group = other->display_group;
         break;
      }
   }
   if (compat == nil) {
      // Set to a new group
      *group = next_display_group++;
      compat = [[NSOpenGLContext alloc] initWithFormat:fmt shareContext: nil];      
   }
   return compat;
}
/* create_display_fs:
* Create a fullscreen display - capture the display
*/
static ALLEGRO_DISPLAY* create_display_fs(int w, int h) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = _AL_MALLOC(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
	if (dpy == NULL) {
		//ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
		return NULL;
	}
   memset(dpy, 0, sizeof(*dpy));
	/* Set up the ALLEGRO_DISPLAY part */
	dpy->parent.vt = osx_get_display_driver_fs();
	dpy->parent.format = ALLEGRO_PIXEL_FORMAT_RGBA_8888; // To do: use the actual format and flags
	dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
	dpy->parent.flags = al_get_new_display_flags() | ALLEGRO_OPENGL | ALLEGRO_FULLSCREEN;
	_al_event_source_init(&dpy->parent.es);
   dpy->cursor = [[NSCursor arrowCursor] retain];
   int depth;
  	decode_allegro_format(dpy->parent.format, &dpy->gl_fmt, 
                         &dpy->gl_datasize, &depth);
   NSOpenGLPixelFormatAttribute attrs[] = {
      
      // Specify that we want a full-screen OpenGL context.
      NSOpenGLPFAFullScreen,
      
      // Take over the main screen.
      NSOpenGLPFAScreenMask, CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay),
      
      NSOpenGLPFAColorSize, depth,
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFAAccelerated,
      0
   };
   
   NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: attrs];
   if (fmt == nil) return NULL;
   NSOpenGLContext* context = CreateShareableContext(fmt, &dpy->display_group);
   [fmt release];
   if (context == nil) return NULL;
   [context makeCurrentContext];
   dpy->ctx = context;
   dpy->parent.w = w;
   dpy->parent.h = h;
   CGDisplayCapture(CGMainDisplayID());
   CFDictionaryRef mode = CGDisplayBestModeForParametersAndRefreshRate(CGMainDisplayID(), depth, w, h, dpy->parent.refresh_rate, NO);
   CGDisplaySwitchToMode(CGMainDisplayID(), mode);
   [context setFullScreen];
   dpy->parent.ogl_extras = _AL_MALLOC(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
	dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
	/* Set up GL as we want */
	setup_gl(&dpy->parent);
   /* Clear and flush (for double buffering) */
   glClear(GL_COLOR_BUFFER_BIT);
   [context flushBuffer];
   glClear(GL_COLOR_BUFFER_BIT);

   /* Add to the display list */
	ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_system_driver()->displays);
	*add = &dpy->parent;
   in_fullscreen = YES;
   /* Begin the 'private' event loop */
   [ALDisplayHelper performSelectorOnMainThread: @selector(runFullScreenDisplay:) 
                                     withObject: [NSValue valueWithPointer:dpy] 
                                  waitUntilDone: NO];
   return &dpy->parent;
}

/* create_display_win:
* Create a windowed display - create the window with an ALOpenGLView
* to be its content view
*/
static ALLEGRO_DISPLAY* create_display_win(int w, int h) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = _AL_MALLOC(sizeof(ALLEGRO_DISPLAY_OSX_WIN));
	if (dpy == NULL) {
		return NULL;
	}
   memset(dpy, 0, sizeof(*dpy));
	/* Set up the ALLEGRO_DISPLAY part */
	dpy->parent.vt = osx_get_display_driver_win();
	dpy->parent.format = ALLEGRO_PIXEL_FORMAT_RGBA_8888; // To do: use the actual format and flags
	dpy->parent.refresh_rate = al_get_new_display_refresh_rate();
	dpy->parent.flags = al_get_new_display_flags() | ALLEGRO_OPENGL | ALLEGRO_WINDOWED | ALLEGRO_SINGLEBUFFER;
	dpy->parent.w = w;
	dpy->parent.h = h;
	_al_event_source_init(&dpy->parent.es);
   dpy->cursor = [[NSCursor arrowCursor] retain];
   
   if (_al_vector_is_empty(&al_system_driver()->displays)) {
      last_window_pos = NSZeroPoint;
   }
   /* OSX specific part - finish the initialisation on the main thread */
   [ALDisplayHelper performSelectorOnMainThread: @selector(initialiseDisplay:) 
      withObject: [NSValue valueWithPointer:dpy] 
      waitUntilDone: YES];
	[dpy->ctx makeCurrentContext];
   dpy->parent.ogl_extras = _AL_MALLOC(sizeof(ALLEGRO_OGL_EXTRAS));
   memset(dpy->parent.ogl_extras, 0, sizeof(ALLEGRO_OGL_EXTRAS));
   _al_ogl_manage_extensions(&dpy->parent);
   _al_ogl_set_extensions(dpy->parent.ogl_extras->extension_api);
	dpy->parent.ogl_extras->backbuffer = _al_ogl_create_backbuffer(&dpy->parent);
	/* Set up GL as we want */
	setup_gl(&dpy->parent);
	ALLEGRO_DISPLAY **add = _al_vector_alloc_back(&al_system_driver()->displays);
	return *add = &dpy->parent;
}
static void destroy_display(ALLEGRO_DISPLAY* d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   _al_ogl_unmanage_extensions(&dpy->parent);
   [ALDisplayHelper performSelectorOnMainThread: @selector(destroyDisplay:) 
      withObject: [NSValue valueWithPointer:dpy] 
      waitUntilDone: YES];
	[dpy->ctx release];
   [dpy->cursor release];
	_al_event_source_free(&d->es);
   _AL_FREE(d->ogl_extras);
   _AL_FREE(d);
}
/* create_display:
 * Create a display either fullscreen or windowed depending on flags
 */
ALLEGRO_DISPLAY* create_display(int w, int h)
{
   int flags = al_get_new_display_flags();
   if (flags & ALLEGRO_DIRECT3D) {
      // Can't handle this if the user asked for it
      return NULL;
   }
   if (flags & ALLEGRO_FULLSCREEN) {
      return create_display_fs(w,h);
   }
   else {
      return create_display_win(w,h);
   }
}
/* Note: in windowed mode, contexts always behave like single-buffered
 * though in fact they are composited offscreen */
static void flip_display(ALLEGRO_DISPLAY *disp) 
{
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) disp;
   if (disp->ogl_extras->opengl_target->is_backbuffer) {
      if (disp->flags & ALLEGRO_SINGLEBUFFER) {
         glFlush();
      }
      else {
         [dpy->ctx flushBuffer];
      }
   }
}

static bool show_cursor_win(ALLEGRO_DISPLAY *d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
	//ALOpenGLView* view = (ALOpenGLView*) [dpy->win contentView];
   dpy->show_cursor = YES;
   [NSCursor unhide];
   return true;
}
static bool hide_cursor_win(ALLEGRO_DISPLAY *d) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
	//ALOpenGLView* view = (ALOpenGLView*) [dpy->win contentView];
   dpy->show_cursor = NO;
   [NSCursor hide];
   return true;
}
static bool show_cursor_fs(ALLEGRO_DISPLAY *d) {
// stub
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   return false;
}
static bool hide_cursor_fs(ALLEGRO_DISPLAY *d) {
// stub
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   return false;
}
static bool resize_display_win(ALLEGRO_DISPLAY *d, int w, int h) {
	ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) d;
   NSWindow* window = dpy->win;
   NSRect current = [window frame];
   NSRect rc = [window frameRectForContentRect: NSMakeRect(0.0f, 0.0f, (float) w, (float) h)];
   rc.origin = current.origin;
   [window setFrame:rc display:YES animate:YES];
	return true;
}
static bool is_compatible_bitmap(ALLEGRO_DISPLAY* disp, ALLEGRO_BITMAP* bmp) {
   return (bmp->display == disp)
      || (((ALLEGRO_DISPLAY_OSX_WIN*) bmp->display)->display_group == ((ALLEGRO_DISPLAY_OSX_WIN*) disp)->display_group);
}

/* set_window_position:
 * Set the position of the window that owns this display
 * Slightly complicated because Allegro measures from the top down to
 * the top left corner, OS X from the bottom up to the bottom left corner.
 */
static void set_window_position(ALLEGRO_DISPLAY* display, int x, int y) 
{
   ALLEGRO_DISPLAY_OSX_WIN* d = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   NSWindow* window = d->win;
   NSRect rc = [window frame];
   NSRect sc = [[window screen] frame];
   rc.origin.x = (float) x;
   rc.origin.y = sc.size.height - rc.size.height - ((float) y);
   // Couldn't resist animating it!
   [window setFrame:rc display:YES animate:YES];
}

/* get_window_position:
 * Get the position of the window that owns this display. See comment for
 * set_window_position.
 */
static void get_window_position(ALLEGRO_DISPLAY* display, int* px, int* py) 
{
   ALLEGRO_DISPLAY_OSX_WIN* d = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   NSWindow* window = d->win;
   NSRect rc = [window frame];
   NSRect sc = [[window screen] frame];
   *px = (int) rc.origin.x;
   *py = (int) (sc.size.height - rc.origin.y - rc.size.height);
}

/* set_window_title:
 * Set the title of the window with this display
 */
void set_window_title(ALLEGRO_DISPLAY *display, AL_CONST char *title)
{
   ALLEGRO_DISPLAY_OSX_WIN* dpy = (ALLEGRO_DISPLAY_OSX_WIN*) display;
   [dpy->win setTitle: [NSString stringWithCString:title]];
}
/* set_display_icon:
 * Set the icon - OS X doesn't have per-window icons so 
 * ignore the display parameter
 */
void set_icon(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP* bitmap)
{
   [NSApp setApplicationIconImage: NSImageFromAllegroBitmap(bitmap)];
}

ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver_win(void)
{
	static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display_win;
      vt->destroy_display = destroy_display;
      vt->set_current_display = set_current_display;
      vt->flip_display = flip_display;
      vt->resize_display = resize_display_win;
      vt->create_bitmap = _al_ogl_create_bitmap;
      vt->set_target_bitmap = _al_ogl_set_target_bitmap;
      vt->get_backbuffer = _al_ogl_get_backbuffer;
      vt->is_compatible_bitmap = is_compatible_bitmap;
      vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
      vt->show_mouse_cursor = show_cursor_win;
      vt->hide_mouse_cursor = hide_cursor_win;
      vt->get_window_position = get_window_position;
      vt->set_window_position = set_window_position;
      vt->set_window_title = set_window_title;
      vt->set_icon = set_icon;
      _al_ogl_add_drawing_functions(vt);
   }
	return vt;
}   

ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver_fs(void)
{
	static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display_fs;
      vt->destroy_display = destroy_display;
      vt->set_current_display = set_current_display;
      vt->flip_display = flip_display;
      vt->create_bitmap = _al_ogl_create_bitmap;
      vt->set_target_bitmap = _al_ogl_set_target_bitmap;
      vt->show_mouse_cursor = show_cursor_fs;
      vt->hide_mouse_cursor = hide_cursor_fs;
      vt->get_backbuffer = _al_ogl_get_backbuffer;
      vt->is_compatible_bitmap = is_compatible_bitmap;
      vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
      _al_ogl_add_drawing_functions(vt);
   }
	return vt;
}  
 
/* Mini VT just for creating displays */
ALLEGRO_DISPLAY_INTERFACE* osx_get_display_driver(void)
{
	static ALLEGRO_DISPLAY_INTERFACE* vt = NULL;
   if (vt == NULL) {
      vt = _AL_MALLOC(sizeof(*vt));
      memset(vt, 0, sizeof(ALLEGRO_DISPLAY_INTERFACE));
      vt->create_display = create_display;
   }
	return vt;
}   

