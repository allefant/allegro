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
 *      Configuration routines.
 *
 *      By Trent Gamblin.
 */

/* Title: Configuration routines
 */


#include <stdio.h>
#include <ctype.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_config.h"
#include "allegro5/internal/aintern_memory.h"


#define MAXSIZE 1024


static void *local_calloc1(size_t size)
{
   void *p;

   p = _AL_MALLOC(size);
   if (p) {
      memset(p, 0, size);
   }
   return p;
}


/* Function: al_config_create
 *  Create an empty configuration structure.
 */
ALLEGRO_CONFIG *al_config_create(void)
{
   ALLEGRO_CONFIG *config = local_calloc1(sizeof(ALLEGRO_CONFIG));
   ASSERT(config);

   return config;
}


static ALLEGRO_CONFIG_SECTION *find_section(const ALLEGRO_CONFIG *config,
   const ALLEGRO_USTR *section)
{
   ALLEGRO_CONFIG_SECTION *p = config->head;

   if (!p)
      return NULL;

   while (p != NULL) {
      if (al_ustr_equal(p->name, section))
         return p;
      p = p->next;
   }

   return NULL;
}


static ALLEGRO_CONFIG_ENTRY *find_entry(ALLEGRO_CONFIG_ENTRY *section_head,
   const ALLEGRO_USTR *key)
{
   ALLEGRO_CONFIG_ENTRY *p = section_head;

   while (p != NULL) {
      if (!p->is_comment && al_ustr_equal(p->key, key)) {
         return p;
      }
      p = p->next;
   }

   return NULL;
}


static void get_key_and_value(const ALLEGRO_USTR *buf,
   ALLEGRO_USTR *key, ALLEGRO_USTR *value)
{
   int eq = al_ustr_find_chr(buf, 0, '=');

   if (eq == -1) {
      al_ustr_assign(key, buf);
      al_ustr_assign_cstr(value, "");
   }
   else {
      al_ustr_assign_substr(key, buf, 0, eq);
      al_ustr_assign_substr(value, buf, eq + 1, al_ustr_size(buf));
   }

   al_ustr_trim_ws(key);
   al_ustr_trim_ws(value);
}


static ALLEGRO_CONFIG_SECTION *config_add_section(ALLEGRO_CONFIG *config,
   const ALLEGRO_USTR *name)
{
   ALLEGRO_CONFIG_SECTION *sec = config->head;
   ALLEGRO_CONFIG_SECTION *section;

   if ((section = find_section(config, name)))
      return section;

   section = local_calloc1(sizeof(ALLEGRO_CONFIG_SECTION));
   section->name = al_ustr_dup(name);

   if (sec == NULL) {
      config->head = section;
   }
   else {
      while (sec->next != NULL)
         sec = sec->next;
      sec->next = section;
   }

   return section;
}


/* Function: al_config_add_section
 *  Add a section to a configuration structure.
 */
void al_config_add_section(ALLEGRO_CONFIG *config, const char *name)
{
   ALLEGRO_USTR_INFO name_info;
   ALLEGRO_USTR *uname;

   uname = al_ref_cstr(&name_info, name);
   config_add_section(config, uname);
}


static void config_set_value(ALLEGRO_CONFIG *config,
   const ALLEGRO_USTR *section, const ALLEGRO_USTR *key,
   const ALLEGRO_USTR *value)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *entry;

   s = find_section(config, section);
   if (s) {
      entry = find_entry(s->head, key);
   }
   else {
      entry = NULL;
   }

   if (entry) {
      al_ustr_assign(entry->value, value);
      al_ustr_trim_ws(entry->value);
      return;
   }

   entry = local_calloc1(sizeof(ALLEGRO_CONFIG_ENTRY));
   entry->is_comment = false;
   entry->key = al_ustr_dup(key);
   entry->value = al_ustr_dup(value);
   al_ustr_trim_ws(entry->value);

   if (!s) {
      s = config_add_section(config, section);
   }

   if (s->head == NULL) {
      s->head = entry;
   }
   else {
      ALLEGRO_CONFIG_ENTRY *p = s->head;
      while (p->next != NULL)
         p = p->next;
      p->next = entry;
   }
}


/* Function: al_config_set_value
 *  Set a value in a section of a configuration.  If the section doesn't yet
 *  exist, it will be created.  If a value already existed for the given key,
 *  it will be overwritten.
 *  The section can be NULL or "" for the global section.
 */
void al_config_set_value(ALLEGRO_CONFIG *config,
   const char *section, const char *key, const char *value)
{
   ALLEGRO_USTR_INFO section_info;
   ALLEGRO_USTR_INFO key_info;
   ALLEGRO_USTR_INFO value_info;
   ALLEGRO_USTR *usection;
   ALLEGRO_USTR *ukey;
   ALLEGRO_USTR *uvalue;

   if (section == NULL) {
      section = "";
   }

   ASSERT(key);
   ASSERT(value);

   usection = al_ref_cstr(&section_info, section);
   ukey = al_ref_cstr(&key_info, key);
   uvalue = al_ref_cstr(&value_info, value);

   config_set_value(config, usection, ukey, uvalue);
}


static void config_add_comment(ALLEGRO_CONFIG *config,
   const ALLEGRO_USTR *section, const ALLEGRO_USTR *comment)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *entry;

   s = find_section(config, section);

   entry = local_calloc1(sizeof(ALLEGRO_CONFIG_ENTRY));
   entry->is_comment = true;
   entry->key = al_ustr_dup(comment);

   /* Replace all newline characters by spaces, otherwise the written comment
    * file will be corrupted.
    */
   al_ustr_find_replace_cstr(entry->key, 0, "\n", " ");

   if (!s) {
      s = config_add_section(config, section);
   }

   if (s->head == NULL) {
      s->head = entry;
   }
   else {
      ALLEGRO_CONFIG_ENTRY *p = s->head;
      while (p->next != NULL)
         p = p->next;
      p->next = entry;
   }
}


/* Function: al_config_add_comment
 *  Add a comment in a section of a configuration.  If the section doesn't yet
 *  exist, it will be created.
 *  The section can be NULL or "" for the global section.
 */
void al_config_add_comment(ALLEGRO_CONFIG *config,
   const char *section, const char *comment)
{
   ALLEGRO_USTR_INFO section_info;
   ALLEGRO_USTR_INFO comment_info;
   ALLEGRO_USTR *usection;
   ALLEGRO_USTR *ucomment;

   if (section == NULL) {
      section = "";
   }

   ASSERT(comment);

   usection = al_ref_cstr(&section_info, section);
   ucomment = al_ref_cstr(&comment_info, comment);

   config_add_comment(config, usection, ucomment);
}


/* Function: al_config_get_value
 *  Gets a pointer to an internal character buffer that will only remain valid
 *  as long as the ALLEGRO_CONFIG structure is not destroyed. Copy the value
 *  if you need a copy.
 *  The section can be NULL or "" for the global section.
 *  Returns NULL if the section or key do not exist.
 */
static bool config_get_value(const ALLEGRO_CONFIG *config,
   const ALLEGRO_USTR *section, const ALLEGRO_USTR *key,
   ALLEGRO_USTR **ret_value)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *e;

   s = find_section(config, section);
   if (!s)
      return false;
   e = s->head;

   e = find_entry(e, key);
   if (!e)
      return false;

   *ret_value = e->value;
   return true;
}


const char *al_config_get_value(const ALLEGRO_CONFIG *config,
   const char *section, const char *key)
{
   ALLEGRO_USTR_INFO section_info;
   ALLEGRO_USTR_INFO key_info;
   ALLEGRO_USTR *usection;
   ALLEGRO_USTR *ukey;
   ALLEGRO_USTR *value;

   if (section == NULL) {
      section = "";
   }

   usection = al_ref_cstr(&section_info, section);
   ukey = al_ref_cstr(&key_info, key);

   if (config_get_value(config, usection, ukey, &value))
      return al_cstr(value);
   else
      return NULL;
}


/* Function: al_config_read
 *  Read a configuration file.
 *  Returns NULL on error.
 */
ALLEGRO_CONFIG *al_config_read(const char *filename)
{
   ALLEGRO_CONFIG *config;
   ALLEGRO_CONFIG_SECTION *current_section = NULL;
   char buffer[MAXSIZE];
   ALLEGRO_USTR *line;
   ALLEGRO_USTR *section;
   ALLEGRO_USTR *key;
   ALLEGRO_USTR *value;
   ALLEGRO_FS_ENTRY *file;

   file = al_fopen(filename, "r");
   if (!file) {
      return NULL;
   }
   
   config = al_config_create();
   if (!config) {
      al_fclose(file);
      return NULL;
   }

   line = al_ustr_new("");
   section = al_ustr_new("");
   key = al_ustr_new("");
   value = al_ustr_new("");

   while (al_fgets(file, MAXSIZE, buffer)) {
      al_ustr_assign_cstr(line, buffer);
      al_ustr_trim_ws(line);

      if (al_ustr_has_prefix_cstr(line, "#") || al_ustr_size(line) == 0) {
         /* Preserve comments and blank lines */
         ALLEGRO_USTR *name;
         if (current_section)
            name = current_section->name;
         else
            name = al_ustr_empty_string();
         config_add_comment(config, name, line);
      }
      else if (al_ustr_has_prefix_cstr(line, "[")) {
         int rbracket = al_ustr_rfind_chr(line, al_ustr_size(line), ']');
         if (rbracket == -1)
            rbracket = al_ustr_size(line);
         al_ustr_assign_substr(section, line, 1, rbracket);
         current_section = config_add_section(config, section);
      }
      else {
         get_key_and_value(line, key, value);
         if (current_section == NULL)
            config_set_value(config, al_ustr_empty_string(), key, value);
         else
            config_set_value(config, current_section->name, key, value);
      }
   }

   al_ustr_free(line);
   al_ustr_free(section);
   al_ustr_free(key);
   al_ustr_free(value);

   al_fclose(file);

   return config;
}


static bool config_write_section(ALLEGRO_FS_ENTRY *file,
   const ALLEGRO_CONFIG_SECTION *s)
{
   ALLEGRO_CONFIG_ENTRY *e;

   if (al_ustr_size(s->name) > 0) {
      if (al_fputc(file, '[') == EOF) {
         return false;
      }
      if (al_fputs(file, al_cstr(s->name)) != 0) {
         return false;
      }
      if (al_fputs(file, "]\n") != 0) {
         return false;
      }
   }

   e = s->head;
   while (e != NULL) {
      if (e->is_comment) {
         if (al_ustr_size(e->key) > 0) {
            if (!al_ustr_has_prefix_cstr(e->key, "#")) {
               if (al_fputs(file, "# ")) {
                  return false;
               }
            }
            if (al_fputs(file, al_cstr(e->key))) {
               return false;
            }
         }
         if (al_fputs(file, "\n") != 0) {
            return false;
         }
      }
      else {
         if (al_fputs(file, al_cstr(e->key)) != 0) {
            return false;
         }
         if (al_fputs(file, "=") != 0) {
            return false;
         }
         if (al_fputs(file, al_cstr(e->value)) != 0) {
            return false;
         }
         if (al_fputs(file, "\n") != 0) {
            return false;
         }
      }
      e = e->next;
   }

   return true;
}


/* Function: al_config_write
 *  Write out a configuration file.
 *  Returns zero on success, non-zero on error.
 */
int al_config_write(const ALLEGRO_CONFIG *config, const char *filename)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_FS_ENTRY *file = al_fopen(filename, "w");

   if (!file) {
      return 1;
   }

   /* Save global section */
   s = config->head;
   while (s != NULL) {
      if (al_ustr_size(s->name) == 0) {
         if (!config_write_section(file, s)) {
            goto Error;
         }
         break;
      }
      s = s->next;
   }

   /* Save other sections */
   s = config->head;
   while (s != NULL) {
      if (al_ustr_size(s->name) > 0) {
         if (!config_write_section(file, s)) {
            goto Error;
         }
      }
      s = s->next;
   }

#if 0
   if (al_fs_entry_close(file)) {
      /* XXX do we delete the incomplete file? */
      return 1;
   }
#endif

   al_fclose(file);

   return 0;

Error:

   /* XXX do we delete the incomplete file? */
   al_fclose(file);
   return 1;
}


/* do_config_merge_into:
 *  Helper function for merging.
 */
void do_config_merge_into(ALLEGRO_CONFIG *master, const ALLEGRO_CONFIG *add,
   bool merge_comments)
{
   ALLEGRO_CONFIG_SECTION *s;
   ALLEGRO_CONFIG_ENTRY *e;
   ASSERT(master);

   if (!add) {
      return;
   }

   /* Save each section */
   s = add->head;
   while (s != NULL) {
      config_add_section(master, s->name);
      e = s->head;
      while (e != NULL) {
         if (!e->is_comment) {
            config_set_value(master, s->name, e->key, e->value);
         }
         else if (merge_comments) {
            config_add_comment(master, s->name, e->key);
         }
         e = e->next;
      }
      s = s->next;
   }
}


/* Function: al_config_merge_into
 *  Merge one configuration structure into another.
 *  Values in configuration 'add' override those in 'master'.
 *  'master' is modified.
 *  Comments from 'add' are not retained.
 */
void al_config_merge_into(ALLEGRO_CONFIG *master, const ALLEGRO_CONFIG *add)
{
   do_config_merge_into(master, add, false);
}


/* Function: al_config_merge
 *  Merge two configuration structures, and return the result as a new
 *  configuration.  Values in configuration 'cfg2' override those in 'cfg1'.
 *  Neither of the input configuration structures are
 *  modified.
 *  Comments from 'cfg2' are not retained.
 */
ALLEGRO_CONFIG *al_config_merge(const ALLEGRO_CONFIG *cfg1,
    const ALLEGRO_CONFIG *cfg2)
{
   ALLEGRO_CONFIG *config = al_config_create();

   do_config_merge_into(config, cfg1, true);
   do_config_merge_into(config, cfg2, false);

   return config;
}


/* Function: al_config_destroy
 *  Free the resources used by a configuration structure.
 *  Does nothing if passed NULL.
 */
void al_config_destroy(ALLEGRO_CONFIG *config)
{
   ALLEGRO_CONFIG_ENTRY *e;
   ALLEGRO_CONFIG_SECTION *s;

   if (!config) {
      return;
   }

   s = config->head;
   while (s) {
      ALLEGRO_CONFIG_SECTION *tmp = s->next;
      e = s->head;
      while (e) {
         ALLEGRO_CONFIG_ENTRY *tmp = e->next;
         al_ustr_free(e->key);
         al_ustr_free(e->value);
         _AL_FREE(e);
         e = tmp;
      }
      al_ustr_free(s->name);
      _AL_FREE(s);
      s = tmp;
   }

   _AL_FREE(config);
}

/* vim: set sts=3 sw=3 et: */
