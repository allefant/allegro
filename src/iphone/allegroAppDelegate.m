#import "allegroAppDelegate.h"
#import "EAGLView.h"

#import <UIKit/UIKit.h>
#include <pthread.h>

#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_iphone.h"
#include "allegro5/internal/aintern_opengl.h"

ALLEGRO_DEBUG_CHANNEL("iphone")

void _al_iphone_run_user_main(void);

static allegroAppDelegate *global_delegate;
static UIImageView *splashview;
static UIApplication *app = NULL;
static volatile bool waiting_for_program_halt = false;
static float scale_override = -1.0;

/* Function: al_iphone_get_window
 */
UIWindow *al_iphone_get_window(void)
{
	return [global_delegate window];
}

/* Function: al_iphone_get_view
 */
UIView *al_iphone_get_view(void)
{
	return [global_delegate view];
}

static bool is_ipad(void)
{
#if (__IPHONE_OS_VERSION_MAX_ALLOWED >= 30200)
	if ([[UIDevice currentDevice] respondsToSelector: @selector(userInterfaceIdiom)])
		return ([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad);
#endif
	return false;
}

static int iphone_orientation_to_allegro(UIDeviceOrientation orientation)
{
   switch (orientation) {
      case UIDeviceOrientationPortrait:
         return ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES;

      case UIDeviceOrientationPortraitUpsideDown:
         return ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES;

      case UIDeviceOrientationLandscapeRight:
         return ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES;

      case UIDeviceOrientationLandscapeLeft:
         return ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES;

      case UIDeviceOrientationFaceUp:
         return ALLEGRO_DISPLAY_ORIENTATION_FACE_UP;

      case UIDeviceOrientationFaceDown:
         return ALLEGRO_DISPLAY_ORIENTATION_FACE_DOWN;

      default:
         return ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN;
   }
}

static void iphone_send_orientation_event(ALLEGRO_DISPLAY* display, int orientation)
{
   _al_event_source_lock(&display->es);
   if (_al_event_source_needs_to_generate_event(&display->es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_ORIENTATION;
      event.display.timestamp = al_get_time();
      event.display.source = display;
      event.display.orientation = orientation;
      _al_event_source_emit_event(&display->es, &event);
   }
   _al_event_source_unlock(&display->es);
}


void _al_iphone_acknowledge_drawing_halt(ALLEGRO_DISPLAY *display)
{
   (void)display;
   waiting_for_program_halt = false;
}

/* Function: al_iphone_get_screen_scale
 */
float al_iphone_get_screen_scale(void)
{
   float ret = 0.0;

   if (scale_override > 0.0) {
      ret = scale_override;
   }
   else if ([[UIScreen mainScreen] respondsToSelector:NSSelectorFromString(@"scale")]) {
      ret = [[UIScreen mainScreen] scale];
   }
   else {
      ret = 1.0f;
   }

   return ret;
}

/* Function: al_iphone_override_screen_scale
 */
void al_iphone_override_screen_scale(float scale)
{
   scale_override = scale;

   ALLEGRO_INFO("Scale overridden. New scale: %f\n", scale);
}

/* Function: al_iphone_set_statusbar_orientation
 */
void al_iphone_set_statusbar_orientation(int o)
{
	UIInterfaceOrientation orientation = UIInterfaceOrientationPortrait;

	if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT)
		orientation = UIInterfaceOrientationPortrait;
	else if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_PORTRAIT_UPSIDE_DOWN)
		orientation = UIInterfaceOrientationPortraitUpsideDown;
	else if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_LANDSCAPE_RIGHT)
		orientation = UIInterfaceOrientationLandscapeRight;
	else if (o == ALLEGRO_IPHONE_STATUSBAR_ORIENTATION_LANDSCAPE_LEFT)
		orientation = UIInterfaceOrientationLandscapeLeft;

	[app setStatusBarOrientation:orientation animated:NO];
}

void _al_iphone_add_view(ALLEGRO_DISPLAY *display)
{
   [global_delegate set_allegro_display:display];
   /* This is the same as
    * [global_delegate.view add_view];
    * except it will run in the main thread.
    */
   [global_delegate performSelectorOnMainThread: @selector(add_view) 
      withObject:nil
      waitUntilDone: YES];
}

void _al_iphone_make_view_current(void)
{
    [global_delegate.view make_current];
}

void _al_iphone_recreate_framebuffer(ALLEGRO_DISPLAY *display)
{
   EAGLView *view = global_delegate.view;
   [view destroyFramebuffer];
   [view createFramebuffer];
   display->w = view.backingWidth;
   display->h = view.backingHeight;
   _al_ogl_resize_backbuffer(display->ogl_extras->backbuffer, display->w, display->h);
}

void _al_iphone_flip_view(void)
{
   static bool splash_removed = false;
   NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

   [global_delegate.view flip];

   if (!splash_removed) {
      splash_removed = true;

      /* remove splash screen */
      if (splashview) {
         [splashview removeFromSuperview];
         [splashview release];
         splashview = nil;
      }
   
      [global_delegate.view becomeFirstResponder];
   }
    
   [pool drain];
}

void _al_iphone_reset_framebuffer(void)
{
   [global_delegate.view reset_framebuffer];
}

/* Use a frequency to start receiving events at the freuqency, 0 to shut off
 * the accelerometer (according to Apple, it drains a bit of battery while on).
 */
void _al_iphone_accelerometer_control(int frequency)
{
    if (frequency) {
        [[UIAccelerometer sharedAccelerometer] setUpdateInterval:(1.0 / frequency)];
        [[UIAccelerometer sharedAccelerometer] setDelegate:global_delegate];
    }
    else {
        [[UIAccelerometer sharedAccelerometer] setDelegate:nil];
    }
}

void _al_iphone_get_screen_size(int *w, int *h)
{
   UIScreen* screen = [UIScreen mainScreen];

   if (NULL != screen) {

      CGRect bounds = [screen bounds];
      CGFloat scale = 1.0f;
      
      if ([screen respondsToSelector:NSSelectorFromString(@"scale")]) {
         scale = [screen scale];
      }
      
      *w = (int)(bounds.size.width  * scale);
      *h = (int)(bounds.size.height * scale);
   }
   else {
   
      ASSERT("You should never see this message, unless Apple changed their policy and allows for removing screens from iDevices." && false);
   }
}

int _al_iphone_get_orientation()
{
   UIDevice* device = [UIDevice currentDevice];
  
   if (NULL != device)
      return iphone_orientation_to_allegro([device orientation]);
   else
      return ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN;
}

@implementation allegroAppDelegate

@synthesize window;
@synthesize view;
@synthesize view_controller;

+ (void)run:(int)argc:(char **)argv {
    UIApplicationMain(argc, argv, nil, @"allegroAppDelegate");
}

/* When applicationDidFinishLaunching() returns, the current view gets visible
 * for at least one frame. Since we have no OpenGL context in the main thread
 * we cannot draw anything into our OpenGL view so it means there's a short
 * black flicker before the first al_flip_display() no matter what.
 *
 * To prevent the black flicker we create a dummy view here which loads the
 * Default.png just as apple does internally. This way the moment the user
 * view is first displayed in the user thread we switch from displaying the
 * splash screen to the first user frame, without any flicker.
 *
 * NOTE: Default-Portrait.png is loaded if on iPad
 */
- (void)display_splash_screen
{
   UIScreen *screen = [UIScreen mainScreen];
   window = [[UIWindow alloc] initWithFrame:[screen bounds]];
   UIImage *img = nil;
   if (is_ipad())
   	img = [UIImage imageNamed:@"Default-Portrait.png"];
   else if (al_iphone_get_screen_scale() == 2.0)
        img = [UIImage imageNamed:@"Default@2x.png"];
   else
   	img = [UIImage imageNamed:@"Default.png"];
   if (img != nil) {
      splashview = [[UIImageView alloc] initWithImage:img];
      [window addSubview:splashview];
   }
   else {
   	ALLEGRO_WARN("img is nil in display_splash_screen.\n");
   }
   [window makeKeyAndVisible];
}

- (void)orientation_change:(NSNotification *)notification
{
   ALLEGRO_DISPLAY *d = allegro_display;
   
   (void)notification;
    
   int orientation = _al_iphone_get_orientation();

   if (d == NULL)
      return;
      
   iphone_send_orientation_event(d, orientation);
}

- (void)applicationDidFinishLaunching:(UIApplication *)application {

   ALLEGRO_INFO("App launched.\n");
    
   //application.statusBarHidden = true;
   global_delegate = self;
   app = application;

   // Register for device orientation notifications
   [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
   [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientation_change:)
        name:UIDeviceOrientationDidChangeNotification object:nil];

   [self display_splash_screen];

   _al_iphone_run_user_main();
}

/* This may never get called on iOS 4 */
- (void)applicationWillTerminate:(UIApplication *)application {
    (void)application;
    ALLEGRO_EVENT event;
    ALLEGRO_DISPLAY *d = allegro_display;
    ALLEGRO_SYSTEM_IPHONE *iphone = (void *)al_get_system_driver();
    iphone->wants_shutdown = true;
    
    ALLEGRO_INFO("Terminating.\n");
   
   [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];

    _al_event_source_lock(&d->es);
    
    if (_al_event_source_needs_to_generate_event(&d->es)) {
        event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
        event.display.timestamp = al_get_time();
        _al_event_source_emit_event(&d->es, &event);
    }
    _al_event_source_unlock(&d->es);
    
    /* When this method returns, the application terminates - so lets wait a bit
     * so the user app can shutdown properly, e.g. to save state as is usually
     * required by iphone apps.
     */
    _al_iphone_await_termination();
}

- (void)applicationWillResignActive:(UIApplication *)application {
   ALLEGRO_DISPLAY *d = allegro_display;
   ALLEGRO_EVENT event;
   
   (void)application;
    
   ALLEGRO_INFO("Application becoming inactive.\n");

   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
       event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
       event.display.timestamp = al_current_time();
       _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    ALLEGRO_DISPLAY *d = allegro_display;
	ALLEGRO_EVENT event;
	
	(void)application;
    
    ALLEGRO_INFO("Application becoming active.\n");

   if (!d)
      return;

    _al_event_source_lock(&d->es);
    if (_al_event_source_needs_to_generate_event(&d->es)) {
        event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
        event.display.timestamp = al_current_time();
        _al_event_source_emit_event(&d->es, &event);
    }
    _al_event_source_unlock(&d->es);
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
   ALLEGRO_DISPLAY *d = allegro_display;
   ALLEGRO_EVENT event;
   
   (void)application;
    
   ALLEGRO_INFO("Application entering background.\n");

   waiting_for_program_halt = true;

   _al_event_source_lock(&d->es);
   if (_al_event_source_needs_to_generate_event(&d->es)) {
       event.display.type = ALLEGRO_EVENT_DISPLAY_HALT_DRAWING;
       event.display.timestamp = al_current_time();
       _al_event_source_emit_event(&d->es, &event);
   }
   _al_event_source_unlock(&d->es);

   while (waiting_for_program_halt) {
   	// do nothing, this should be quick
	al_rest(0.001);
   }
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    ALLEGRO_DISPLAY *d = allegro_display;
	ALLEGRO_EVENT event;
	
	(void)application;
    
    ALLEGRO_INFO("Application coming back to foreground.\n");

    _al_event_source_lock(&d->es);
    if (_al_event_source_needs_to_generate_event(&d->es)) {
        event.display.type = ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING;
        event.display.timestamp = al_current_time();
        _al_event_source_emit_event(&d->es, &event);
    }
    _al_event_source_unlock(&d->es);
}

- (void)set_allegro_display:(ALLEGRO_DISPLAY *)d {
   allegro_display = d;
}

/* Note: This must be called from the main thread. Ask Apple why - but I tried
 * it and otherwise things simply don't work (the screen just stays black).
 */
- (void)add_view {
   view_controller = [[ViewController alloc]init];
   view = (EAGLView *)view_controller.view;
    ALLEGRO_DEBUG("thread %p: view %p\n", pthread_self(), view);
   [view set_allegro_display:allegro_display];
   [window addSubview:view];
}

- (void)accelerometer:(UIAccelerometer*)accelerometer didAccelerate:(UIAcceleration*)acceleration
{
    (void)accelerometer;
    _al_iphone_generate_joystick_event(acceleration.x, acceleration.y, acceleration.z);
}

- (void)dealloc {
   [window release];
   [super dealloc];
}

@end
