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
 *      Top level font reading routines.
 *
 *      By Evert Glebbeek.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"

#include "allegro5/a5_font.h"

#include "font.h"


typedef struct FONT_TYPE_INFO
{
   char *ext;
   ALLEGRO_FONT *(*load)(const char *filename, void *param);
   struct FONT_TYPE_INFO *next;
} FONT_TYPE_INFO;

static FONT_TYPE_INFO *font_type_list = NULL;



/* Function: al_font_register_font_file_type
 */
void al_font_register_font_file_type(const char *ext, ALLEGRO_FONT *(*load)(const char *filename, void *param))
{
   char tmp[32], *aext;
   FONT_TYPE_INFO *iter = font_type_list;

   aext = uconvert_toascii(ext, tmp);
   if (strlen(aext) == 0) return;

   if (!iter) 
      iter = font_type_list = _AL_MALLOC(sizeof(struct FONT_TYPE_INFO));
   else {
      for (iter = font_type_list; iter->next; iter = iter->next);
      iter = iter->next = _AL_MALLOC(sizeof(struct FONT_TYPE_INFO));
   }

   if (iter) {
      iter->load = load;
      iter->ext = strdup(aext);
      iter->next = NULL;
   }
}



/* Function: al_font_load_font
 */
ALLEGRO_FONT *al_font_load_font(const char *filename, void *param)
{
   char tmp[32], *aext;
   FONT_TYPE_INFO *iter;
   ASSERT(filename);

   aext = uconvert_toascii(ustrrchr(filename,'.'), tmp);
   
   for (iter = font_type_list; iter; iter = iter->next) {
      if (stricmp(iter->ext, aext) == 0) {
	 if (iter->load)
	    return iter->load(filename, param);
	 return NULL;
      }
   }
   
   /* Try to load the file as a bitmap image and grab the font from there */
   return al_font_load_bitmap_font(filename, param);
}



/* _al_font_register_font_file_type_exit:
 *  Free list of registered bitmap file types.
 */
static void _al_font_register_font_file_type_exit(void)
{
   FONT_TYPE_INFO *iter = font_type_list, *next;

   while (iter) {
      next = iter->next;
      _AL_FREE(iter->ext);
      _AL_FREE(iter);
      iter = next;
   }
   
   font_type_list = NULL;

   /* If we are using a destructor, then we only want to prune the list
    * down to valid modules. So we clean up as usual, but then reinstall
    * the internal modules.
    */
   #ifdef ALLEGRO_USE_CONSTRUCTOR
      _al_font_register_font_file_type_init();
   #endif

   _al_remove_exit_func(_al_font_register_font_file_type_exit);
}



/* _al_font_register_font_file_type_init:
 *  Register built-in font file types.
 */
void _al_font_register_font_file_type_init(void)
{
   _al_add_exit_func(_al_font_register_font_file_type_exit,
		  "_al_font_register_font_file_type_exit");
}



#ifdef ALLEGRO_USE_CONSTRUCTOR
   CONSTRUCTOR_FUNCTION(static void font_filetype_constructor(void));
   DESTRUCTOR_FUNCTION(static void font_filetype_destructor(void));

   /* font_filetype_constructor:
    *  Register font filetype functions if this object file is linked
    *  in. This isn't called if the load_font() function isn't used 
    *  in a program, thus saving a little space in statically linked 
    *  programs.
    */
   void font_filetype_constructor(void)
   {
      _al_font_register_font_file_type_init();
   }

   /* font_filetype_destructor:
    *  Since we only want to destroy the whole list when we *actually*
    *  quit, not just when al_uninstall_system() is called, we need to use a
    *  destructor to accomplish this.
    */
   static void font_filetype_destructor(void)
   {
      FONT_TYPE_INFO *iter = font_type_list, *next;

      while (iter) {
         next = iter->next;
         _AL_FREE(iter->ext);
         _AL_FREE(iter);
         iter = next;
      }
   
      font_type_list = NULL;

      _al_remove_exit_func(_al_font_register_font_file_type_exit);
   }
#endif
