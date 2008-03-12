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
 *      File System Hook, stdio "driver".
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#define ALLEGRO_FS_FILE_DEFINED
#define ALLEGRO_FS_DIR_DEFINED

#include <stdio.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_DIRENT_H
   #include <sys/types.h>
   #include <dirent.h>
   #define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
   #define dirent direct
   #define NAMLEN(dirent) ((dirent)->d_namlen)
   #ifdef HAVE_SYS_NDIR_H
      #include <sys/ndir.h>
   #endif
   #ifdef HAVE_SYS_DIR_H
      #include <sys/dir.h>
   #endif
   #ifdef HAVE_NDIR_H
      #include <ndir.h>
   #endif
#endif

#ifdef TIME_WITH_SYS_TIME
   #include <sys/time.h>
   #include <time.h>
#else
   #ifdef HAVE_SYS_TIME_H
      #include <sys/time.h>
   #else
      #include <time.h>
   #endif
#endif

#define PREFIX_I "al-fs-stdio INFO: "

/* FIXME:

should AL_FS_ENTRY carry a vtable? so the io handler can be changed without fucking up

*/

typedef struct AL_FS_ENTRY AL_FS_ENTRY;
struct AL_FS_ENTRY {
   AL_FS_HOOK_FILE_VTABLE *vtable;

   uint32_t isdir;
   union {
      FILE *handle;
      DIR *dir;
   };

   struct stat st;
   char *path;  // stores the path given by the user.
   char *found; // used to store the proper path to a file opened and found via the search path.
   char mode[6];
   uint32_t free_on_fclose;
   uint32_t ulink;
   uint32_t stat_mode;
};

#include "allegro.h"
#include "allegro/debug.h"
#include "allegro/fshook.h"
#include "allegro/internal/fshook.h"

static char **search_path = NULL;
static uint32_t search_path_count = 0;

uint32_t al_fs_init_stdio(void)
{
   al_fs_set_hook(AL_FS_HOOK_CREATE_HANDLE,  al_fs_stdio_create_handle);
   al_fs_set_hook(AL_FS_HOOK_DESTROY_HANDLE, al_fs_stdio_destroy_handle);
   al_fs_set_hook(AL_FS_HOOK_OPEN_HANDLE,    al_fs_stdio_open_handle);
   al_fs_set_hook(AL_FS_HOOK_CLOSE_HANDLE,   al_fs_stdio_close_handle);

   al_fs_set_hook(AL_FS_HOOK_ENTRY_OPEN,  al_fs_stdio_fopen);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_CLOSE, al_fs_stdio_fclose);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_READ,  al_fs_stdio_fread);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_WRITE, al_fs_stdio_fwrite);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_FLUSH, al_fs_stdio_fflush);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_SEEK,  al_fs_stdio_fseek);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_TELL,  al_fs_stdio_ftell);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_ERROR, al_fs_stdio_ferror);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_EOF,   al_fs_stdio_feof);

   al_fs_set_hook(AL_FS_HOOK_ENTRY_STAT, al_fs_stdio_fstat);

   al_fs_set_hook(AL_FS_HOOK_OPENDIR,  al_fs_stdio_opendir);
   al_fs_set_hook(AL_FS_HOOK_CLOSEDIR, al_fs_stdio_closedir);
   al_fs_set_hook(AL_FS_HOOK_READDIR,  al_fs_stdio_readdir);

   al_fs_set_hook(AL_FS_HOOK_ENTRY_MODE,  al_fs_stdio_file_mode);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_ATIME, al_fs_stdio_file_atime);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_MTIME, al_fs_stdio_file_mtime);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_CTIME, al_fs_stdio_file_ctime);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_SIZE,  al_fs_stdio_file_size);

   al_fs_set_hook(AL_FS_HOOK_ENTRY_UNLINK, al_fs_stdio_unlink);
   al_fs_set_hook(AL_FS_HOOK_ENTRY_EXISTS, al_fs_stdio_exists);

   al_fs_set_hook(AL_FS_HOOK_MKTEMP, al_fs_stdio_mktemp);
   al_fs_set_hook(AL_FS_HOOK_GETCWD, al_fs_stdio_getcwd);
   al_fs_set_hook(AL_FS_HOOK_CHDIR,  al_fs_stdio_chdir);

   al_fs_set_hook(AL_FS_HOOK_ADD_SEARCH_PATH,   al_fs_stdio_add_search_path);
   al_fs_set_hook(AL_FS_HOOK_SEARCH_PATH_COUNT, al_fs_stdio_search_path_count);
   al_fs_set_hook(AL_FS_HOOK_GET_SEARCH_PATH,   al_fs_stdio_get_search_path);

   al_fs_set_hook(AL_FS_HOOK_STAT_MODE,  al_fs_stdio_stat_mode);
   al_fs_set_hook(AL_FS_HOOK_STAT_ATIME, al_fs_stdio_stat_atime);
   al_fs_set_hook(AL_FS_HOOK_STAT_MTIME, al_fs_stdio_stat_mtime);
   al_fs_set_hook(AL_FS_HOOK_STAT_CTIME, al_fs_stdio_stat_ctime);
   al_fs_set_hook(AL_FS_HOOK_STAT_SIZE,  al_fs_stdio_stat_size);

   al_fs_set_hook(AL_FS_HOOK_PATH_TO_SYS, al_fs_stdio_path_to_sys);
   al_fs_set_hook(AL_FS_HOOK_PATH_TO_UNI, al_fs_stdio_path_to_uni);

   al_fs_set_hook(AL_FS_HOOK_UNLINK, al_fs_stdio_unlink);
   al_fs_set_hook(AL_FS_HOOK_EXISTS, al_fs_stdio_exists);

   return 0;
}

AL_FS_ENTRY *al_fs_stdio_create_handle(AL_CONST char *path)
{
   AL_FS_ENTRY *fh = NULL;
   uint32_t len = 0;
   uint32_t fnd = 0;
   char *tmp = NULL;

   fh = AL_MALLOC(sizeof(AL_FS_ENTRY));
   if(!fh) {
      *allegro_errno = errno;
      return NULL;
   }

   memset(fh, 0, sizeof(AL_FS_ENTRY));

   len = strlen( path );
   fh->path = AL_MALLOC(len+1);
   if(!fh->path) {
      *allegro_errno = errno;
      AL_FREE(fh);
      return NULL;
   }

   memcpy(fh->path, path, len+1);

   /* lookup real file path if given non abs path */
   if(fh->path[0] != '/') {
      for(spi = 0; spi < search_path_count; ++spi) {
         uint32_t splen = strlen( search_path[spi] );
         struct stat st;

         tmp = _AL_REALLOC( tmp, splen + len + 1 );
         memcpy(tmp, search_path[spi], MIN(splen, PATH_MAX));
         if(tmp[splen-1] == '/') {
            tmp[splen] = '/';
            splen++;
         }

         memcpy(tmp+splen, fh->path, MIN(len + splen, PATH_MAX));
         tmp[splen+len] = '\0';

         if(stat(tmp, &st) != 0) {
            AL_FREE(tmp);
            break;
         }

         fnd = 1;
         fh->st = st;
         fh->found = tmp;
         break;
      }
   }

   if(!fnd) {
      if(fh->found)
         stat(fh->found, &(fh->st));
      else
         stat(fh->path, &(fh->st));
   }

   fh->stat_mode = 0;

   if(S_ISDIR(fh->st.st_mode))
      fh->stat_mode |= AL_FM_ISDIR;

   if(S_ISREG(fh->st.st_mode))
      fh->stat_mode |= AL_FM_ISFILE;

   if(fh->st.st_mode & S_IRUSR || fh->st.st_mode & S_IRGRP)
      fh->stat_mode |= AL_FM_READ;

   if(fh->st.st_mode & S_IWUSR || fh->st.st_mode & S_IWGRP)
      fh->stat_mode |= AL_FM_WRITE;

   if(fh->st.st_mode & S_IXUSR || fh->st.st_mode & S_IXGRP)
      fh->stat_mode |= AL_FM_EXECUTE;

#ifdef ALLEGRO_WINDOWS
   {
      DWORD attrib = GetFileAttributes(fh->found ? fh->found : fh->path);
      if(attrib & FILE_ATTRIBUTE_HIDDEN)
         fh->stat_mode |= AL_FM_HIDDEN;
   }
#else
   if(fh->found)
      fh->stat_mode |= ( fh->found[0] == '.' ? AL_FM_HIDDEN : 0 );
   else
      fh->stat_mode |= ( fh->path[0] == '.' ? AL_FM_HIDDEN : 0 );
#endif

   return fh;
}

void al_fs_stdio_destroy_handle(AL_FS_ENTRY *handle)
{
   if(handle->found) {
      if(handle->ulink)
         unlink( handle->found );

      AL_FREE(handle->found);
   }
   else {
      if(handle->ulink)
         unlink( handle->path );
   }

   if(handle->path)
      AL_FREE(handle->path);

   if(handle->handle)
      fclose(handle->handle);

   if(handle->dir)
      closedir(handle->dir);

   memset(handle, 0, sizeof(AL_FS_ENTRY));
   AL_FREE(handle);
}

int32_t al_fs_stdio_open_handle(AL_FS_ENTRY *fh, AL_CONST char *mode)
{
   char *tmp = NULL;

   tmp = fh->found ? fh->found : fh->path;
   if(fh->stat_mode & AL_FM_ISDIR) {
      fh->dir = opendir( tmp );
      if(!fh->dir) {
         *allegro_errno = errno;
         return -1;
      }
   else {
      fh->handle = fopen( tmp, mode );
      if(!fh->handle) {
         *allegro_errno = errno;
         return -1;
      }
   }

   return 0;
}

void al_fs_stdio_close_handle(AL_FS_ENTRY *handle)
{
   int32_t ret = 0;

   if(handle->handle) {
      ret = fclose(handle->handle);

      /* unlink on close */
      if(handle->found && handle->ulink) {
         unlink( handle->found );
         AL_FREE( handle->found );
         handle->found = NULL;
      }
      else if(handle->path && handle->ulink) {
         unlink( handle->found );
      }

      handle->handle = NULL;
   }

   if(handle->dir) {
      /* think about handling the unlink on close.. may only be usefull if mktemp can do dirs as well, or a mkdtemp is provided */
      closedir(handle->dir);
      if(handle->found) {
         AL_FREE(handle->found);
         handle->found = NULL;
      }
      handle->dir = NULL;
   }

}

/* Has to use the base al_fs_* functions and not the stdio functions,
   due to the base functions allocating and freeing the vtable. */
AL_FS_ENTRY *al_fs_stdio_fopen(const char *path, const char *mode)
{
   AL_FS_ENTRY *fh = al_fs_create_handle(path);
   if(al_fs_open_handle(fh, mode) != 0) {
      al_fs_destroy_handle(fh);
      return NULL;
   }

   fh->free_on_fclose = 1;
   return fh;
}

void al_fs_stdio_fclose(AL_FS_ENTRY *fp)
{
   al_fs_close_handle(fp);

   if(fp->free_on_fclose)
      al_fs_destroy_handle(fp);
}

size_t al_fs_stdio_fread(void *ptr, size_t size, AL_FS_ENTRY *fp)
{
   size_t ret = fread(ptr, 1, size, fp->handle);
   if(ret == 0) {
      *allegro_errno = errno;
   }

   return ret;
}

size_t al_fs_stdio_fwrite(const void *ptr, size_t size, AL_FS_ENTRY *fp)
{
   size_t ret = fwrite(ptr, 1, size, fp->handle);
   if(ret == 0) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_fflush(AL_FS_ENTRY *fp)
{
   int32_t ret = fflush(fp->handle);
   if(ret == EOF) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_fseek(AL_FS_ENTRY *fp, uint32_t offset, uint32_t whence)
{
   int32_t ret = 0;

   switch(whence) {
      case AL_SEEK_SET: whence = SEEK_SET; break;
      case AL_SEEK_CUR: whence = SEEK_CUR; break;
      case AL_SEEK_END: whence = SEEK_END; break;
   }

   ret = fseek(fp->handle, offset, whence);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_ftell(AL_FS_ENTRY *fp)
{
   int32_t ret = ftell(fp->handle);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_ferror(AL_FS_ENTRY *fp)
{
   return ferror(fp->handle);
}

int32_t al_fs_stdio_feof(AL_FS_ENTRY *fp)
{
   return feof(fp->handle);
}


int32_t al_fs_stdio_fstat(AL_FS_ENTRY *fp)
{
   int32_t ret = 0;

   ret = stat(path, &(fp->st));
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}


AL_FS_ENTRY *al_fs_stdio_opendir(const char *path)
{
   AL_FS_ENTRY *dir = AL_MALLOC(sizeof(AL_FS_ENTRY));
   if(!dir)
      return NULL;

   dir->handle = opendir(path);
   if(!dir) {
      AL_FREE(dir);
      *allegro_errno = errno;
   }

   return dir;
}

int32_t al_fs_stdio_closedir(AL_FS_ENTRY *dir)
{
   int32_t ret = closedir(dir->handle);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   dir->handle = NULL;
   AL_FREE(dir);

   return ret;
}

int32_t al_fs_stdio_readdir(AL_FS_ENTRY *dir, size_t size, char *buf)
{
   struct dirent *ent = readdir(dir->handle);
   int32_t ent_len = 0;

   if(!ent) {
      *allegro_errno = errno;
      return NULL;
   }

   ent_len = strlen(ent->d_name);
   memcpy(buf, ent->d_name, MIN(ent_len+1, size));

   return ent;
}

uint32_t al_fs_stdio_file_mode(AL_FS_ENTRY *st)
{

}

time_t al_fs_stdio_file_atime(AL_FS_ENTRY *st)
{

}

time_t al_fs_stdio_file_mtime(AL_FS_ENTRY *st)
{

}

time_t al_fs_stdio_file_cime(AL_FS_ENTRY *st)
{

}

#define MAX_MKTEMP_TRIES 1000
const char mktemp_ok_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

void _al_fs_mktemp_replace_XX(char *template, char *dst)
{
   size_t len = strlen(template);
   uint32_t i = 0;

   for(i=0; i<len; ++i) {
      if(template[i] != 'X') {
         dst[i] = template[i];
      }
      else {
         dst[i] = mktemp_ok_chars[ _al_rand() % (sizeof(mktemp_ok_chars)-1) ];
      }
   }
}

/* FIXME: provide the filename created. */
/* might have to make AL_FS_ENTRY a strust to provide the filename, and unlink hint. */
/* by default, temp file is NOT unlink'ed automatically */

AL_FS_ENTRY *al_fs_stdio_mktemp(const char *template, uint32_t ulink)
{
   int32_t fd = -1, tries = 0;
   int32_t template_len = 0, tmpdir_len = 0;
   AL_FS_ENTRY *fh = NULL;
   char *dest = NULL;
   char tmpdir[PATH_MAX];

   template_len = strlen( template );

   if(al_get_path(AL_TEMP_PATH, tmpdir, PATH_MAX) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to find temp directory"));
      return NULL;
   }

   tmpdir_len = strlen( tmpdir_len );

   dest = AL_MALLOC( template_len + tmpdir_len + 2 );
   if(!dest) {
      *allegro_errno = errno;
      return NULL;
   }

   memset(dest, 0, template_len + tmpdir_len + 2);
   memcpy(dest, tmpdir, strlen( tmpdir ) );

   /* doing this check makes the path prettier, no extra / laying around */
   if(dest[tmpdir_len-1] != '/') {
      dest[tmpdir_len] = '/';
      tmpdir_len++;
   }

   memcpy(dest + tmpdir_len, template, template_len);

   for(i=0; i<MAX_MKTEMP_TRIES; ++i) {
      _al_fs_mktemp_replace_XX(template, dest + tmpdir_len);
      fd = open( dest, O_EXCL | O_CREAT | O_RDWR );
      if(fd == -1)
         continue;

      // changing the hook for create handle in a separate thread will cause some nice errors here,
      //    if you expect it to return a stdio handle ;)
      fh = al_fs_create_handle( dest );
      if(!fh) {
         close(fd);
         free(dest);
         return NULL;
      }

      fh->handle = fdopen( fd, "r+" );
      if(!fh->handle) {
         al_fs_stdio_destroy_handle(fh);
         close(fd);
         free(dest);
         return NULL;
      }

      if(ulink == AL_FS_MKTEMP_UNLINK_NOW)
         unlink( dest );
      else if(ulink == AL_FS_MKTEMP_UNLINK_ON_CLOSE)
         fh->ulink = 1;

      fh->free_on_fclose = 1;
      fh->path = dest;

      return fh;
   }

   free(dest);
   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to create a uniqe temporary file"));
   return NULL;
}

int32_t al_fs_stdio_getcwd(char *buf, size_t len)
{
   char *cwd = getcwd(buf, len);
   if(!cwd) {
      *allegro_errno = errno;
      return -1;
   }

   return 0;
}

int32_t al_fs_stdio_chdir(const char *path)
{
   int32_t ret = chdir(path);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_add_search_path(const char *path)
{
   char **new_search_path = NULL;
   char *new_path = NULL;

   /* dup path first to elimiate need to re-resize search_path if dup fails */
   new_path = ustrdup(path);
   if(!new_path) {
      return -1;
   }

   /* extend search_path, store temporarily so original var isn't overwritten with NULL on failure */
   new_search_path = (char **)_al_realloc(search_path, sizeof(char *) * (search_path_count + 1));
   if(!new_search_path) {
      free(new_path);
      *allegro_errno = errno;
      return -1;
   }

   search_path = new_search_path;
   search_path[search_path_count] = new_path;
   search_path_count++;

   return 0;
}

uint32_t al_fs_stdio_search_path_count()
{
   return search_path_count;
}

/* FIXME: is this the best way to handle the "search path" ? */
int32_t al_fs_stdio_get_search_path(uint32_t idx, char *dest, uint32_t len)
{
   if(idx < search_path_count) {
      uint32_t slen = strlen( search_path[idx] );

      memcpy( dest, search_path[idx], MIN( slen, len-1 ) );
      dest[len] = '\0';
   }

   return -1;
}


uint32_t al_fs_stdio_drive_sep(size_t len, char *sep)
{
#ifdef ALLEGRO_WINDOWS
   char *s = ":";
   _al_sane_strncpy(sep, s, MIN(ustrlen(sep), ustrlen(s)));
   return 0;
#else
   if(len >= 1)
      sep[0] = '\0';

   return 0;
#endif
}

uint32_t al_fs_stdio_path_sep(size_t len, char *sep)
{
#ifdef ALLEGRO_WINDOWS
   char c = '\\';
#else
   char c = '/';
#endif

   if(len >= 2) {
      sep[0] = c;
      sep[1] = '\0';
   }

   return 0;
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup. */
int32_t al_fs_stdio_path_to_sys(const char *orig, uint32_t len, char *path)
{
   return 0;
}

int32_t al_fs_stdio_path_to_uni(const char *orig, uint32_t len, char *path)
{
   return 0;
}


int32_t al_fs_stdio_fexists(AL_FS_ENTRY *handle)
{
   struct stat st;
   if(!stat(handle->name, &st)) {
      if(errno == ENOEXIST) {
         return 0;
      }
      else {
         return -1;
      }
   }

   return 1;
}

// FIXME
int32_t al_fs_stdio_file_exists(AL_CONST char *path)
{
   struct stat st;
   if(!stat(path, &st)) {
      if(errno == ENOEXIST) {
         return 0;
      }
      else {
         return -1;
      }
   }

   return 1;
}

// FIXME
uint32_t al_fs_stdio_dir_exists(AL_CONST char *)
{

}

int32_t al_fs_stdio_rmdir(AL_CONST char *path)
{
   int32_t err = rmdir(path);
   if(err != 0) {
      *allegro_errno = errno;
      return -1;
   }

   return 0;
}

void al_fs_stdio_fname(AL_FS_ENTRY *fp, size_t size, char *buf)
{
   int32_t len = strlen(fp->path);

   memcpy(buf, fp->path, MIN(len+1, size));
}

#endif

