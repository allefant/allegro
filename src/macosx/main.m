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
*      main() function replacement for MacOS X.
*
*      By Angelo Mottola.
*
*      See readme.txt for copyright information.
*/


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif
#undef main


/* For compatibility with the unix code */
extern int    __crt0_argc;
extern char **__crt0_argv;
extern void *_mangled_main_address;

static char *arg0, *arg1 = NULL;
static int refresh_rate = 70;

static BOOL in_bundle(void)
{
	/* This comes from the ADC tips & tricks section: how to detect if the app
    * lives inside a bundle
    */
	FSRef processRef;
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	FSCatalogInfo processInfo;
	GetProcessBundleLocation(&psn, &processRef);
	FSGetCatalogInfo(&processRef, kFSCatInfoNodeFlags, &processInfo, NULL, NULL, NULL);
	if (processInfo.nodeFlags & kFSNodeIsDirectoryMask) 
		return YES;
	else
		return NO;
}


@interface AllegroAppDelegate : NSObject
- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename;
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification;
- (void)applicationDidChangeScreenParameters: (NSNotification *)aNotification;
+ (void)app_main: (id)arg;
- (NSApplicationTerminateReply) applicationShouldTerminate: (id)sender;
@end

@interface AllegroWindowDelegate : NSObject
- (BOOL)windowShouldClose: (id)sender;
- (void)windowDidDeminiaturize: (NSNotification *)aNotification;
@end

@implementation AllegroAppDelegate

- (BOOL)application: (NSApplication *)theApplication openFile: (NSString *)filename
{
	arg1 = strdup([filename lossyCString]);
	return YES;
}



/* applicationDidFinishLaunching:
*  Called when the app is ready to run. 
*/
- (void)applicationDidFinishLaunching: (NSNotification *)aNotification
{
	CFDictionaryRef mode;
	NSString* exename, *resdir;
	NSFileManager* fm;
	BOOL isDir;
	
	
	if (in_bundle() == YES)   
	{
		/* In a bundle, so chdir to the containing directory,
		* or to the 'magic' resource directory if it exists.
		* (see the readme.osx file for more info)
		*/
		NSBundle* osx_bundle = [NSBundle mainBundle];
		exename = [[osx_bundle executablePath] lastPathComponent];
		resdir = [[osx_bundle resourcePath] stringByAppendingPathComponent: exename];
		fm = [NSFileManager defaultManager];
		if ([fm fileExistsAtPath: resdir isDirectory: &isDir] && isDir) {
			/* Yes, it exists inside the bundle */
			[fm changeCurrentDirectoryPath: resdir];
		}
		else {
			/* No, change to the 'standard' OSX resource directory if it exists*/
			if ([fm fileExistsAtPath: [osx_bundle resourcePath] isDirectory: &isDir] && isDir)
			{
				[fm changeCurrentDirectoryPath: [osx_bundle resourcePath]];
			}
			/* It doesn't exist - this is unusual for a bundle. Don't chdir */
		}
		arg0 = strdup([[osx_bundle bundlePath] fileSystemRepresentation]);
		if (arg1) {
			static char *args[2];
			args[0] = arg0;
			args[1] = arg1;
			__crt0_argv = args;
			__crt0_argc = 2;
		}
		else {
			__crt0_argv = &arg0;
			__crt0_argc = 1;
		}
	}
	/* else: not in a bundle so don't chdir */
	
	mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
	CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &refresh_rate);
	if (refresh_rate <= 0)
		refresh_rate = 70;
	
	[NSThread detachNewThreadSelector: @selector(app_main:)
							 toTarget: [AllegroAppDelegate class]
						   withObject: nil];
	return;
}



/* applicationDidChangeScreenParameters:
*  Invoked when the screen did change resolution/color depth.
*/
- (void)applicationDidChangeScreenParameters: (NSNotification *)aNotification
{
	CFDictionaryRef mode;
	int new_refresh_rate;
	
	//   if ((osx_window) && (osx_gfx_mode == OSX_GFX_WINDOW)) 
	//   {
	//      osx_setup_colorconv_blitter();
	//      [osx_window display];
	//   }
	mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
	CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &new_refresh_rate);
	if (new_refresh_rate <= 0)
		new_refresh_rate = 70;
	refresh_rate = new_refresh_rate;
}



/* Call the user main() */
static void call_user_main(void)
{
	int (*real_main)(int, char*[]) = (int (*)(int, char*[])) _mangled_main_address;
	exit(real_main(__crt0_argc, __crt0_argv));
}



/* app_main:
*  Thread dedicated to the user program; real main() gets called here.
*/
+ (void)app_main: (id)arg
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	call_user_main();
	[pool release];
}



/* applicationShouldTerminate:
*  Called upon Command-Q or "Quit" menu item selection.
*  Post a message but do not quit directly
*/
- (NSApplicationTerminateReply) applicationShouldTerminate: (id)sender
{
	_al_osx_post_quit();
	return NSTerminateCancel;
}

/* end of AllegroAppDelegate implementation */
@end



/* This prevents warnings that 'NSApplication might not
* respond to setAppleMenu' on OS X 10.4
*/
@interface NSApplication(AllegroOSX)
- (void)setAppleMenu:(NSMenu *)menu;
@end



/* main:
*  Replacement for main function.
*/
int main(int argc, char *argv[])
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	AllegroAppDelegate *app_delegate = [[AllegroAppDelegate alloc] init];
	NSMenu *menu;
	NSMenuItem *menu_item, *temp_item;
	
	__crt0_argc = argc;
	__crt0_argv = argv;
	
	if (!_al_osx_bootstrap_ok()) /* not safe to use NSApplication */
		call_user_main();
	
	[NSApplication sharedApplication];
	
	/* Load the main menu nib if possible */
	if ((!in_bundle()) || ([NSBundle loadNibNamed: @"MainMenu"
											owner: NSApp] == NO))
	{
		/* Didn't load the nib; create a default menu programmatically */
		NSString* title = nil;
		NSDictionary* app_dictionary = [[NSBundle mainBundle] infoDictionary];
		if (app_dictionary) 
		{
			title = [app_dictionary objectForKey: @"CFBundleName"];
		}
		if (title == nil) 
		{
			title = [[NSProcessInfo processInfo] processName];
		}
      NSMenu* main_menu = [[NSMenu allocWithZone: [NSMenu menuZone]] initWithTitle: @"temp"];
		[NSApp setMainMenu: main_menu];
		menu = [[NSMenu allocWithZone: [NSMenu menuZone]] initWithTitle: @"temp"];
		temp_item = [[NSMenuItem allocWithZone: [NSMenu menuZone]]
		     initWithTitle: @"temp"
					action: NULL
		     keyEquivalent: @""];
		[main_menu addItem: temp_item];
		[main_menu setSubmenu: menu forItem: temp_item];
      [temp_item release];
		[NSApp setAppleMenu: menu];
		NSString *quit = [@"Quit " stringByAppendingString: title];
		menu_item = [[NSMenuItem allocWithZone: [NSMenu menuZone]]
		     initWithTitle: quit
					action: @selector(terminate:)
		     keyEquivalent: @"q"];
		[menu_item setKeyEquivalentModifierMask: NSCommandKeyMask];
		[menu addItem: menu_item];
      [menu_item release];
      [menu release];
      [main_menu release];
	}
	// setDelegate: doesn't retain the delegate here (a Cocoa convention)
   // therefore we don't release it.
	[NSApp setDelegate: app_delegate];
	[NSApp run];
	/* Can never get here */
	[app_delegate release];
	[pool release];
	return 0;
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* c-file-style: "linux" */
/* End:                   */
