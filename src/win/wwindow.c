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
 *      New Windows window handling
 *
 *      By Trent Gamblin.
 *
 */

/* Raw input */
#define _WIN32_WINNT 0x0501
#ifndef WINVER
#define WINVER 0x0501
#endif

#include <windows.h>
#include <windowsx.h>

/* Only used for Vista and up. */
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

#include <allegro5/allegro.h>
#include <process.h>

#include "allegro5/allegro_direct3d.h"
#include "allegro5/allegro_windows.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/platform/aintwin.h"


ALLEGRO_DEBUG_CHANNEL("wwindow")

static WNDCLASS window_class;

static bool resize_postponed = false;
static bool we_hid_the_mouse = false;


UINT _al_win_msg_call_proc = 0;
UINT _al_win_msg_suicide = 0;



/*
 * Find the top left position of the client area of a window.
 */
static void get_window_pos(HWND window, RECT *pos)
{
   RECT with_decorations;
   RECT adjusted;
   int top;
   int left;
   WINDOWINFO wi;

   wi.cbSize = sizeof(WINDOWINFO);

   GetWindowRect(window, &with_decorations);
   memcpy(&adjusted, &with_decorations, sizeof(RECT));

   GetWindowInfo(window, &wi);
   AdjustWindowRectEx(&adjusted, wi.dwStyle, false, wi.dwExStyle);

   top = with_decorations.top - adjusted.top;
   left = with_decorations.left - adjusted.left;

   pos->top = with_decorations.top + top;
   pos->left = with_decorations.left + left;
}

HWND _al_win_create_hidden_window()
{
   HWND window = CreateWindowEx(0, 
      "ALEX", "hidden", WS_POPUP,
      -5000, -5000, 0, 0,
      NULL,NULL,window_class.hInstance,0);
   return window;
}

static void _al_win_get_window_center(
   ALLEGRO_DISPLAY_WIN *win_display, int width, int height, int *out_x, int *out_y)
{
   int a = win_display->adapter;
   bool *is_fullscreen;
   ALLEGRO_MONITOR_INFO info;
   RECT win_size;

   ALLEGRO_SYSTEM *sys = al_get_system_driver();
   unsigned int num;
   unsigned int i;
   unsigned int fullscreen_found = 0;
   num = al_get_num_video_adapters();
   is_fullscreen = al_calloc(num, sizeof(bool));
   for (i = 0; i < sys->displays._size; i++) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&sys->displays, i);
      ALLEGRO_DISPLAY *d = *dptr;
      if (d->flags & ALLEGRO_FULLSCREEN) {
         ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)d;
         is_fullscreen[win_display->adapter] = true;
         fullscreen_found++;
      }
   }
   if (fullscreen_found && fullscreen_found < num) {
      for (i = 0; i < num; i++) {
         if (is_fullscreen[i] == false) {
            a = i;
            break;
         }
      }
   }
   al_free(is_fullscreen);

   al_get_monitor_info(a, &info);

   win_size.left = info.x1 + (info.x2 - info.x1 - width) / 2;
   win_size.top = info.y1 + (info.y2 - info.y1 - height) / 2;

   *out_x = win_size.left;
   *out_y = win_size.top;
}

HWND _al_win_create_window(ALLEGRO_DISPLAY *display, int width, int height, int flags)
{
   HWND my_window;
   DWORD style;
   DWORD ex_style;
   int pos_x, pos_y;
   bool center = false;
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
   WINDOWINFO wi;
   int lsize, rsize, tsize, bsize; // left, right, top, bottom border sizes

   wi.cbSize = sizeof(WINDOWINFO);

   if (!(flags & ALLEGRO_FULLSCREEN)) {
      if  (flags & ALLEGRO_RESIZABLE) {
         style = WS_OVERLAPPEDWINDOW;
         ex_style = WS_EX_APPWINDOW|WS_EX_OVERLAPPEDWINDOW;
      }
      else {
         style = WS_SYSMENU | WS_MINIMIZEBOX;
         ex_style = WS_EX_APPWINDOW;
      }
   }
   else {
      style = WS_POPUP;
      ex_style = WS_EX_APPWINDOW;
   }

   al_get_new_window_position(&pos_x, &pos_y);
   if ((flags & ALLEGRO_FULLSCREEN) || (flags & ALLEGRO_FULLSCREEN_WINDOW)) {
      pos_x = pos_y = 0;
   }
   else if (pos_x == INT_MAX) {
      pos_x = pos_y = 0;
      center = true;
   }

   if (center) {
      _al_win_get_window_center(win_display, width, height, &pos_x, &pos_y);
   }

   my_window = CreateWindowEx(ex_style,
      "ALEX", "Allegro", style,
      pos_x, pos_y, width, height,
      NULL,NULL,window_class.hInstance,0);

   GetWindowInfo(my_window, &wi);

   lsize = (wi.rcClient.left - wi.rcWindow.left);
   tsize = (wi.rcClient.top - wi.rcWindow.top);
   rsize = (wi.rcWindow.right - wi.rcClient.right);
   bsize = (wi.rcWindow.bottom - wi.rcClient.bottom);

   SetWindowPos(my_window, 0, 0, 0,
      width+lsize+rsize,
      height+tsize+bsize,
      SWP_NOZORDER | SWP_NOMOVE);
   SetWindowPos(my_window, 0, pos_x-lsize, pos_y-tsize,
      0, 0,
      SWP_NOZORDER | SWP_NOSIZE);

   if (flags & ALLEGRO_NOFRAME) {
      SetWindowLong(my_window, GWL_STYLE, WS_VISIBLE);
      SetWindowLong(my_window, GWL_EXSTYLE, WS_EX_APPWINDOW);
      SetWindowPos(my_window, 0, pos_x, pos_y, width, height, SWP_NOZORDER | SWP_FRAMECHANGED);
   }

   ShowWindow(my_window, SW_SHOW);

   if (!(flags & ALLEGRO_RESIZABLE) && !(flags & ALLEGRO_FULLSCREEN)) {
      /* Make the window non-resizable */
      HMENU menu;
      menu = GetSystemMenu(my_window, false);
      DeleteMenu(menu, SC_SIZE, MF_BYCOMMAND);
      DeleteMenu(menu, SC_MAXIMIZE, MF_BYCOMMAND);
      DrawMenuBar(my_window);
   }

   return my_window;
}


HWND _al_win_create_faux_fullscreen_window(LPCTSTR devname, ALLEGRO_DISPLAY *display, 
	int x1, int y1, int width, int height, int refresh_rate, int flags)
{ 
   HWND my_window;
   DWORD style;
   DWORD ex_style;
   DEVMODE mode;
   LONG temp;

   (void)display;
   (void)flags;

   style = WS_VISIBLE;
   ex_style = WS_EX_TOPMOST;

   my_window = CreateWindowEx(ex_style,
      "ALEX", "Allegro", style,
      x1, y1, width, height,
      NULL,NULL,window_class.hInstance,0);

   temp = GetWindowLong(my_window, GWL_STYLE);
   temp &= ~WS_CAPTION;
   SetWindowLong(my_window, GWL_STYLE, temp);
   SetWindowPos(my_window, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_FRAMECHANGED);

   /* Go fullscreen */
   memset(&mode, 0, sizeof(DEVMODE));
   mode.dmSize = sizeof(DEVMODE);
   mode.dmDriverExtra = 0;
   mode.dmBitsPerPel = al_get_new_display_option(ALLEGRO_COLOR_SIZE, NULL);
   mode.dmPelsWidth = width; 
   mode.dmPelsHeight = height;
   mode.dmDisplayFlags = 0;
   mode.dmDisplayFrequency = refresh_rate;
   mode.dmPosition.x = x1;
   mode.dmPosition.y = y1;
   mode.dmFields = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT|DM_DISPLAYFLAGS|
   	DM_DISPLAYFREQUENCY|DM_POSITION;

   ChangeDisplaySettingsEx(devname, &mode, NULL, 0, NULL/*CDS_FULLSCREEN*/);
   
   return my_window;
}


/* _al_win_grab_input:
 * Makes the passed display grab the input. All consequent input events will be
 * generated the this display's window. The display's window must the the
 * foreground window.
 */
void _al_win_grab_input(ALLEGRO_DISPLAY_WIN *win_disp)
{
   _al_win_wnd_schedule_proc(win_disp->window,
                             _al_win_joystick_dinput_grab,
                             win_disp);
}

/* Generate a resize event if the size has changed. We cannot asynchronously
 * change the display size here yet, since the user will only know about a
 * changed size after receiving the resize event. Here we merely add the
 * event to the queue.
 */
static void win_generate_resize_event(ALLEGRO_DISPLAY_WIN *win_display)
{
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)win_display;
   ALLEGRO_EVENT_SOURCE *es = &display->es;
   WINDOWINFO wi;
   int x, y, w, h;

   wi.cbSize = sizeof(WINDOWINFO);
   GetWindowInfo(win_display->window, &wi);
   x = wi.rcClient.left;
   y = wi.rcClient.top;
   w = wi.rcClient.right - wi.rcClient.left;
   h = wi.rcClient.bottom - wi.rcClient.top;

   if (display->w != w || display->h != h) {
      _al_event_source_lock(es);
      if (_al_event_source_needs_to_generate_event(es)) {
         ALLEGRO_EVENT event;
         event.display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
         event.display.timestamp = al_get_time();
         event.display.x = x;
         event.display.y = y;
         event.display.width = w;
         event.display.height = h;
         event.display.source = display;
         _al_event_source_emit_event(es, &event);

         /* Generate an expose event. */
         /* This seems a bit redundant after a resize. */
         if (win_display->display.flags & ALLEGRO_GENERATE_EXPOSE_EVENTS) {
            event.display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
            _al_event_source_emit_event(es, &event);
         }
      }
      _al_event_source_unlock(es);
   }
}

static void postpone_thread_proc(void *arg)
{
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)arg;
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;

   Sleep(50);

   if (win_display->ignore_resize) {
      win_display->ignore_resize = false;
   }
   else {
      win_generate_resize_event(win_display);
   }

   resize_postponed = false;
   win_display->can_acknowledge = true;
}


static void handle_mouse_capture(bool down, HWND hWnd)
{
   int i;
   bool any_button_down = false;
   ALLEGRO_MOUSE_STATE state;

   if (!al_is_mouse_installed())
      return;

   al_get_mouse_state(&state);
   for (i = 1; i <= 5; i++) {
      any_button_down |= al_mouse_button_down(&state, i);
   }

   if (down && GetCapture() != hWnd) {
      SetCapture(hWnd);
   }
   else if (!any_button_down) {
      ReleaseCapture();
   }
}

static void break_window_message_pump(ALLEGRO_DISPLAY_WIN *win_display, HWND hWnd)
{
  /* Get the ID of the thread which created the HWND and is processing its messages */
   DWORD wnd_thread_id = GetWindowThreadProcessId(hWnd, NULL);

   /* Set the "end_thread" flag to stop the message pump */
   win_display->end_thread = true;

   /* Wake-up the message pump so the thread can read the new value of "end_thread" */
   PostThreadMessage(wnd_thread_id, WM_NULL, 0, 0);
}

static LRESULT CALLBACK window_callback(HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam)
{
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_DISPLAY_WIN *win_display = NULL;
   WINDOWINFO wi;
   unsigned int i;
   ALLEGRO_EVENT_SOURCE *es = NULL;
   ALLEGRO_SYSTEM *system = al_get_system_driver();

   wi.cbSize = sizeof(WINDOWINFO);

   if (message == _al_win_msg_call_proc) {
      ((void (*)(void*))wParam) ((void*)lParam);
      return 0;
   }

   if (!system) {
      return DefWindowProc(hWnd,message,wParam,lParam); 
   }

   if (message == _al_win_msg_suicide && wParam) {
      win_display = (ALLEGRO_DISPLAY_WIN*)wParam;
      break_window_message_pump(win_display, hWnd);
      DestroyWindow(hWnd);
      return 0;
   }

   for (i = 0; i < system->displays._size; i++) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&system->displays, i);
      d = *dptr;
      win_display = (void*)d;
      if (win_display->window == hWnd) {
         es = &d->es;
         break;
      }
   }

   if (i == system->displays._size)
      return DefWindowProc(hWnd,message,wParam,lParam); 

   if (message == _al_win_msg_suicide) {
      break_window_message_pump(win_display, hWnd);
      DestroyWindow(hWnd);
      return 0;
   }

   switch (message) {
      case WM_INPUT: 
      {
         /* RAW Input is currently unused. */
          UINT dwSize;
          LPBYTE lpb;
          RAWINPUT* raw;

          /* We can't uninstall WM_INPUT mesages. */
          if (!al_is_mouse_installed())
             break;

          GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, 
                          sizeof(RAWINPUTHEADER));
          lpb = al_malloc(sizeof(BYTE)*dwSize);
          if (lpb == NULL) 
              break;

          GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
          raw = (RAWINPUT*)lpb;

          if (raw->header.dwType != RIM_TYPEMOUSE) {
             al_free(lpb); 
             break;
          }

       {
          RAWMOUSE *rm = &raw->data.mouse;
          int x = raw->data.mouse.lLastX;
          int y = raw->data.mouse.lLastY;
          bool abs = rm->usFlags & (MOUSE_MOVE_ABSOLUTE
                                 || MOUSE_VIRTUAL_DESKTOP);
          if (abs || x || y)
             _al_win_mouse_handle_move(x, y, abs, win_display);

          if (rm->usButtonFlags & RI_MOUSE_BUTTON_1_DOWN)
             _al_win_mouse_handle_button(1, true, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_1_UP)
             _al_win_mouse_handle_button(1, false, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_2_DOWN)
             _al_win_mouse_handle_button(2, true, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_2_UP)
             _al_win_mouse_handle_button(2, false, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
             _al_win_mouse_handle_button(3, true, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_3_UP)
             _al_win_mouse_handle_button(3, false, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
             _al_win_mouse_handle_button(4, true, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_4_UP)
             _al_win_mouse_handle_button(4, false, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
             _al_win_mouse_handle_button(5, true, x, y, abs, win_display);
          if (rm->usButtonFlags & RI_MOUSE_BUTTON_5_UP)
             _al_win_mouse_handle_button(5, false, x, y, abs, win_display);

          if (rm->usButtonFlags & RI_MOUSE_WHEEL) {
             SHORT z = (SHORT)rm->usButtonData;
             _al_win_mouse_handle_wheel(z / WHEEL_DELTA, false, win_display);
          }
       }

          al_free(lpb); 
          break;
      }
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP: {
         int mx = GET_X_LPARAM(lParam);
         int my = GET_Y_LPARAM(lParam);
         bool down = (message == WM_LBUTTONDOWN);
         _al_win_mouse_handle_button(1, down, mx, my, true, win_display);
         handle_mouse_capture(down, hWnd);
         break;
      }
      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP: {
         int mx = GET_X_LPARAM(lParam);
         int my = GET_Y_LPARAM(lParam);
         bool down = (message == WM_MBUTTONDOWN);
         _al_win_mouse_handle_button(3, down, mx, my, true, win_display);
         handle_mouse_capture(down, hWnd);
         break;
      }
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP: {
         int mx = GET_X_LPARAM(lParam);
         int my = GET_Y_LPARAM(lParam);
         bool down = (message == WM_RBUTTONDOWN);
         _al_win_mouse_handle_button(2, down, mx, my, true, win_display);
         handle_mouse_capture(down, hWnd);
         break;
      }
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP: {
         int mx = GET_X_LPARAM(lParam);
         int my = GET_Y_LPARAM(lParam);
         int button = HIWORD(wParam);
         bool down = (message == WM_XBUTTONDOWN);
         if (button == XBUTTON1)
            _al_win_mouse_handle_button(4, down, mx, my, true, win_display);
         else if (button == XBUTTON2)
            _al_win_mouse_handle_button(5, down, mx, my, true, win_display);
         handle_mouse_capture(down, hWnd);
         return TRUE;
      }
      case WM_MOUSEWHEEL: {
         int d = GET_WHEEL_DELTA_WPARAM(wParam);
         _al_win_mouse_handle_wheel(d / WHEEL_DELTA, false, win_display);
         return TRUE;
      }
      case WM_MOUSEHWHEEL: {
         int d = GET_WHEEL_DELTA_WPARAM(wParam);
         _al_win_mouse_handle_hwheel(d / WHEEL_DELTA, false, win_display);
         return TRUE;
      }
      case WM_MOUSEMOVE: {
         TRACKMOUSEEVENT tme;
         int mx = GET_X_LPARAM(lParam);
         int my = GET_Y_LPARAM(lParam);

         if (win_display->mouse_cursor_shown && we_hid_the_mouse) {
            we_hid_the_mouse = false;
            win_display->display.vt->hide_mouse_cursor((void*)win_display);
         }

         _al_win_mouse_handle_move(mx, my, true, win_display);
         if (mx >= 0 && my >= 0 && mx < d->w && my < d->h) {
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_QUERY;
            if (TrackMouseEvent(&tme) && !tme.hwndTrack) {
               tme.dwFlags = TME_LEAVE;
               tme.hwndTrack = hWnd;
               tme.dwHoverTime = 0;
               TrackMouseEvent(&tme);
               _al_win_mouse_handle_enter(win_display);
            }
         }

         break;
      }
      case WM_MOUSELEAVE: {
         _al_win_mouse_handle_leave(win_display);
         break;
      }
      case WM_CAPTURECHANGED: {
         if (al_is_mouse_installed()) {
            int i;
            ALLEGRO_MOUSE_STATE state;
            if (!lParam || (HWND)lParam == hWnd)
               break;
            al_get_mouse_state(&state);
            for (i = 1; i <= 5; i++) {
               if (al_mouse_button_down(&state, i))
                  _al_win_mouse_handle_button(i, 0, 0, 0, true, win_display);
            }
         }
         break;
      }
      case WM_NCMOUSEMOVE: {
         if (!win_display->mouse_cursor_shown) {
            we_hid_the_mouse = true;
            win_display->display.vt->show_mouse_cursor((void*)win_display);
         }
         break;
      }
      case WM_SYSKEYDOWN: {
         int vcode = wParam;
         bool extended = (lParam >> 24) & 0x1;
         bool repeated  = (lParam >> 30) & 0x1;
         _al_win_kbd_handle_key_press(0, vcode, extended, repeated, win_display);
         break;
      }
      case WM_KEYDOWN: {
         int vcode = wParam; 
         int scode = (lParam >> 16) & 0xff;
         bool extended = (lParam >> 24) & 0x1;
         bool repeated = (lParam >> 30) & 0x1;
         /* We can't use TranslateMessage() because we don't know if it will
            produce a WM_CHAR or not. */
         _al_win_kbd_handle_key_press(scode, vcode, extended, repeated, win_display);
         break;
      }
      case WM_SYSKEYUP:
      case WM_KEYUP: {
         int vcode = wParam;
         int scode = (lParam >> 16) & 0xff;
         bool extended = (lParam >> 24) & 0x1;
         _al_win_kbd_handle_key_release(scode, vcode, extended, win_display);
         break;
      }
      case WM_SYSCOMMAND: {
         if (_al_win_disable_screensaver &&
               ((wParam & 0xfff0) == SC_MONITORPOWER || (wParam & 0xfff0) == SC_SCREENSAVE)) {
            return 0;
         }
         else if ((wParam & 0xfff0) == SC_KEYMENU) {
            /* Prevent Windows from intercepting the ALT key.
               (Disables opening menus via the ALT key.) */
            return 0;
         }
         break;
      }
      case WM_PAINT: {
         if (win_display->display.flags & ALLEGRO_GENERATE_EXPOSE_EVENTS) {
            RECT r;
            HRGN hrgn;
            GetWindowRect(win_display->window, &r);
            hrgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
            if (GetUpdateRgn(win_display->window, hrgn, false) != ERROR) {
               PAINTSTRUCT ps;
               DWORD size;
               LPRGNDATA rgndata;
               int n;
               int i;
               RECT *rects;
               BeginPaint(win_display->window, &ps);
               size = GetRegionData(hrgn, 0, NULL);
               rgndata = al_malloc(size);
               GetRegionData(hrgn, size, rgndata);
               n = rgndata->rdh.nCount;
               rects = (RECT *)rgndata->Buffer;
               //GetWindowInfo(win_display->window, &wi);
               _al_event_source_lock(es);
               if (_al_event_source_needs_to_generate_event(es)) {
                  ALLEGRO_EVENT event;
                  event.display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
                  event.display.timestamp = al_get_time();
                  for (i = 0; i < n; i++) {
                     event.display.x = rects[i].left;
                     event.display.y = rects[i].top;
                     event.display.width = rects[i].right - rects[i].left;
                     event.display.height = rects[i].bottom - rects[i].top;
                     _al_event_source_emit_event(es, &event);
                  }
               }
               _al_event_source_unlock(es);
               al_free(rgndata);
               EndPaint(win_display->window, &ps);
               DeleteObject(hrgn);
            }
            return 0;
         }
         break;
      }

      case WM_SETCURSOR:
         switch (LOWORD(lParam)) {
            case HTLEFT:
            case HTRIGHT:
               SetCursor(LoadCursor(NULL, IDC_SIZEWE));
               break;
            case HTBOTTOM:
            case HTTOP:
               SetCursor(LoadCursor(NULL, IDC_SIZENS));
               break;
            case HTBOTTOMLEFT:
            case HTTOPRIGHT:
               SetCursor(LoadCursor(NULL, IDC_SIZENESW));
               break;
            case HTBOTTOMRIGHT:
            case HTTOPLEFT:
               SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
               break;
            default:
               if (win_display->mouse_cursor_shown) {
                  SetCursor(win_display->mouse_selected_hcursor);
               }
               else {
                  SetCursor(NULL);
               }
               break;
         }
         return 1;
      case WM_ACTIVATE:
         if (LOWORD(wParam) != WA_INACTIVE) {
            // Make fullscreen windows TOPMOST again
            if (d->flags & ALLEGRO_FULLSCREEN_WINDOW) {
               SetWindowPos(win_display->window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            if (d->vt->switch_in)
               d->vt->switch_in(d);
            _al_event_source_lock(es);
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT event;
               memset(&event, 0, sizeof(event));
               event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_IN;
               event.display.timestamp = al_get_time();
               _al_event_source_emit_event(es, &event);
            }
            _al_event_source_unlock(es);
            _al_win_grab_input(win_display);
            return 0;
         }
         else {
            // Remove TOPMOST flag from fullscreen windows so we can alt-tab. Also must raise the new activated window
            if (d->flags & ALLEGRO_FULLSCREEN_WINDOW) {
               SetWindowPos(win_display->window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
               SetWindowPos(GetForegroundWindow(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            }
            // Show the taskbar in case we hid it
            SetWindowPos(FindWindow("Shell_traywnd", ""), 0, 0, 0, 0, 0, SWP_SHOWWINDOW);
            if (d->flags & ALLEGRO_FULLSCREEN) {
               d->vt->switch_out(d);
            }
            _al_event_source_lock(es);
            if (_al_event_source_needs_to_generate_event(es)) {
               ALLEGRO_EVENT event;
               memset(&event, 0, sizeof(event));
               event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
               event.display.timestamp = al_get_time();
               _al_event_source_emit_event(es, &event);
            }
            _al_event_source_unlock(es);
            return 0;
         }
         break;
      case WM_MENUCHAR :
         return (MNC_CLOSE << 16) | (wParam & 0xffff);
      case WM_CLOSE:
         _al_event_source_lock(es);
         if (_al_event_source_needs_to_generate_event(es)) {
            ALLEGRO_EVENT event;
            memset(&event, 0, sizeof(event));
            event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
            event.display.timestamp = al_get_time();
            _al_event_source_emit_event(es, &event);
         }
         _al_event_source_unlock(es);
         return 0;
      case WM_SIZE:
         if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED || wParam == SIZE_MINIMIZED) {
            /*
             * Delay the resize event so we don't get bogged down with them
             */
            if (!resize_postponed) {
               resize_postponed = true;
               _beginthread(postpone_thread_proc, 0, (void *)d);
            }
         }
         return 0;
      case WM_ENTERSIZEMOVE:
         /* DefWindowProc for WM_ENTERSIZEMOVE enters a modal loop, which also
          * ends up blocking the loop in d3d_display_thread_proc (which is
          * where we are called from, if using D3D).  Rather than batching up
          * intermediate resize events which the user cannot acknowledge in the
          * meantime anyway, make it so only a single resize event is generated
          * at WM_EXITSIZEMOVE.
          */
         if (d->flags & ALLEGRO_DIRECT3D) {
            resize_postponed = true;
         }
         break;
      case WM_EXITSIZEMOVE:
         if (resize_postponed) {
            win_generate_resize_event(win_display);
            win_display->ignore_resize = false;
            resize_postponed = false;
            win_display->can_acknowledge = true;
         }
         break;
   } 

   return DefWindowProc(hWnd,message,wParam,lParam); 
}

int _al_win_init_window()
{
   // Create A Window Class Structure 
   window_class.cbClsExtra = 0;
   window_class.cbWndExtra = 0; 
   window_class.hbrBackground = NULL;
   window_class.hCursor = NULL; 
   window_class.hIcon = NULL; 
   window_class.hInstance = GetModuleHandle(NULL);
   window_class.lpfnWndProc = window_callback;
   window_class.lpszClassName = "ALEX";
   window_class.lpszMenuName = NULL;
   window_class.style = CS_VREDRAW|CS_HREDRAW|CS_OWNDC;

   RegisterClass(&window_class);

   if (_al_win_msg_call_proc == 0 && _al_win_msg_suicide == 0) {
      _al_win_msg_call_proc = RegisterWindowMessage("Allegro call proc");
      _al_win_msg_suicide = RegisterWindowMessage("Allegro window suicide");
   }

   return true;
}


void _al_win_set_display_icon(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_BITMAP *scaled_bmp;
   HICON icon, old_small, old_big;
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
   ALLEGRO_STATE backup;

   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   scaled_bmp = al_create_bitmap(32, 32);
   al_set_target_bitmap(scaled_bmp);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_draw_scaled_bitmap(bmp, 0, 0,
      al_get_bitmap_width(bmp),
      al_get_bitmap_height(bmp),
      0, 0, 32, 32, 0);

   al_restore_state(&backup);

   icon = _al_win_create_icon(win_display->window, scaled_bmp, 0, 0, false, false);

   old_small = (HICON)SendMessage(win_display->window, WM_SETICON,
      ICON_SMALL, (LPARAM)icon);
   old_big = (HICON)SendMessage(win_display->window, WM_SETICON,
      ICON_BIG, (LPARAM)icon);

   if (old_small)
      DestroyIcon(old_small);
   if (old_big)
      DestroyIcon(old_big);

   al_destroy_bitmap(scaled_bmp);
}

void _al_win_set_window_position(HWND window, int x, int y)
{
   SetWindowPos(
      window,
      HWND_TOP,
      x,
      y,
      0,
      0,
      SWP_NOSIZE | SWP_NOZORDER);
}

void _al_win_get_window_position(HWND window, int *x, int *y)
{
   RECT r;

   get_window_pos(window, &r);

   if (x) {
      *x = r.left;
   }
   if (y) {
      *y = r.top;
   }
}


void _al_win_toggle_window_frame(ALLEGRO_DISPLAY *display, HWND hWnd,
   int w, int h, bool onoff)
{
   if (onoff) {
      display->flags &= ~ALLEGRO_NOFRAME;
   }
   else {
      display->flags |= ALLEGRO_NOFRAME;
   }

   if (display->flags & ALLEGRO_NOFRAME) {
      SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE);
      SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
      SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
   }
   else {
      RECT r;
      DWORD style;
      DWORD exStyle;

      if (display->flags & ALLEGRO_RESIZABLE) {
         style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
         exStyle = WS_EX_OVERLAPPEDWINDOW | WS_EX_APPWINDOW;
      }
      else {
         style = WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
         exStyle = WS_EX_APPWINDOW;
      }

      GetWindowRect(hWnd, &r);
      AdjustWindowRectEx(&r, style, false, exStyle);

      w = r.right - r.left;
      h = r.bottom - r.top;

      SetWindowLong(hWnd, GWL_STYLE, style);
      SetWindowLong(hWnd, GWL_EXSTYLE, exStyle);
      SetWindowPos(hWnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
   }
}


bool _al_win_toggle_display_flag(ALLEGRO_DISPLAY *display, int flag, bool onoff)
{
   ALLEGRO_DISPLAY_WIN *win_display = (void*)display;
   //double timeout;
   ALLEGRO_MONITOR_INFO mi;

   memset(&mi, 0, sizeof(mi));

   switch(flag) {
      case ALLEGRO_NOFRAME: 
         _al_win_toggle_window_frame(display, win_display->window, display->w, display->h, onoff);
         return true;

      case ALLEGRO_FULLSCREEN_WINDOW:
         if ((display->flags & ALLEGRO_FULLSCREEN_WINDOW) && onoff) {
            ALLEGRO_DEBUG("Already a fullscreen window\n");
            return true;
         }
         if (!(display->flags & ALLEGRO_FULLSCREEN_WINDOW) && !onoff) {
            ALLEGRO_DEBUG("Already a non-fullscreen window\n");
            return true;
         }

         _al_win_toggle_display_flag(display, ALLEGRO_NOFRAME, !onoff);

         if (onoff) {
            int adapter = win_display->adapter;
            al_get_monitor_info(adapter, &mi);
            display->flags |= ALLEGRO_FULLSCREEN_WINDOW;
            display->w = mi.x2 - mi.x1;
            display->h = mi.y2 - mi.y1;
         }
         else {
            display->flags &= ~ALLEGRO_FULLSCREEN_WINDOW;
            display->w = win_display->toggle_w;
            display->h = win_display->toggle_h;
         }

         ASSERT(!!(display->flags & ALLEGRO_FULLSCREEN_WINDOW) == onoff);

         // Hide the window temporarily
         SetWindowPos(win_display->window, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);

         al_resize_display(display, display->w, display->h);

         /* acknowledge resize is no longer needed after al_resize_display (or so I think)
         timeout = al_get_time() + 3; // 3 seconds...
         while (al_get_time() < timeout) {
            if (win_display->can_acknowledge) {
               al_acknowledge_resize(display);
               break;
            }
         }
         */

         if (onoff) {
            // Re-set the TOPMOST flag and move to position
            SetWindowPos(win_display->window, HWND_TOPMOST, mi.x1, mi.y1, 0, 0, SWP_NOSIZE);

            // Hide the taskbar if fullscreening on primary monitor
            if (win_display->adapter == 0) {
               SetWindowPos(
                  FindWindow("Shell_traywnd", ""),
                  0,
                  0, 0,
                  0, 0,
                  SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
            }
         }
         else {
            int pos_x = 0;
            int pos_y = 0;
            WINDOWINFO wi;
            int bw, bh;
            
            // Unset the topmost flag
            SetWindowPos(win_display->window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            // Show the taskbar
            SetWindowPos(
               FindWindow("Shell_traywnd", ""), 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
            // Center the window
            _al_win_get_window_center(win_display, display->w, display->h, &pos_x, &pos_y);
            GetWindowInfo(win_display->window, &wi);
            bw = (wi.rcClient.left - wi.rcWindow.left) + (wi.rcWindow.right - wi.rcClient.right),
            bh = (wi.rcClient.top - wi.rcWindow.top) + (wi.rcWindow.bottom - wi.rcClient.bottom),
            SetWindowPos(
               win_display->window, HWND_TOP, 0, 0, display->w+bw, display->h+bh, SWP_NOMOVE
            );
            SetWindowPos(
               win_display->window, HWND_TOP, pos_x-bw/2, pos_y-bh/2, 0, 0, SWP_NOSIZE
            );
         }
         // Show the window again
         SetWindowPos(win_display->window, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);

         ASSERT(!!(display->flags & ALLEGRO_FULLSCREEN_WINDOW) == onoff);
         return true;
   }
   return false;
}


void _al_win_set_window_title(ALLEGRO_DISPLAY *display, const char *title)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
   SetWindowText(win_display->window, title);
}


/* _al_win_wnd_call_proc:
 *  instructs the specifed window thread to call the specified procedure. Waits
 *  until the procedure has returned.
 */
void _al_win_wnd_call_proc(HWND wnd, void (*proc) (void*), void* param)
{
   ASSERT(_al_win_msg_call_proc);
   SendMessage(wnd, _al_win_msg_call_proc, (WPARAM)proc, (LPARAM)param);
}


/* _al_win_wnd_schedule_proc:
 *  instructs the specifed window thread to call the specified procedure but
 *  doesn't wait until the procedure has returned.
 */
void _al_win_wnd_schedule_proc(HWND wnd, void (*proc) (void*), void* param)
{
   ASSERT(_al_win_msg_call_proc);
   if (!PostMessage(wnd, _al_win_msg_call_proc, (WPARAM)proc, (LPARAM)param)) {
      ALLEGRO_ERROR("_al_win_wnd_schedule_proc failed.\n");
   }
}


/* Function: al_get_win_window_handle
 */
HWND al_get_win_window_handle(ALLEGRO_DISPLAY *display)
{
   if (!display)
      return NULL;
   return ((ALLEGRO_DISPLAY_WIN *)display)->window;
}


int _al_win_determine_adapter(void)
{
   int a = al_get_new_display_adapter();
   if (a == -1) {
      int num_screens = al_get_num_video_adapters();
      int cScreen = 0;
      ALLEGRO_MONITOR_INFO temp_info;
      for (cScreen = 0; cScreen < num_screens; cScreen++) {
         al_get_monitor_info(cScreen, &temp_info);
         if (temp_info.x1 == 0 && temp_info.y1 == 0) { // ..probably found primary display
            return cScreen;
         }
      }
      return 0; // safety measure, probably not necessary
   }
   return a;
}

/* vi: set ts=8 sts=3 sw=3 et: */
