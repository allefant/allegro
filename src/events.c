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
 *      Event queues.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Event queues
 *
 * An event queue buffers events generated by event sources that were
 * registered with the queue.
 */


#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_system.h"



struct ALLEGRO_EVENT_QUEUE
{
   _AL_VECTOR sources;  /* vector of (ALLEGRO_EVENT_SOURCE *) */
   _AL_VECTOR events;   /* vector of ALLEGRO_EVENT, used as circular array */
   unsigned int events_head;  /* write end of circular array */
   unsigned int events_tail;  /* read end of circular array */
   _AL_MUTEX mutex;
   _AL_COND cond;
};



/* to prevent concurrent modification of user event reference counts */
static _AL_MUTEX user_event_refcount_mutex = _AL_MUTEX_UNINITED;



/* forward declarations */
static void shutdown_events(void);
static bool do_wait_for_event(ALLEGRO_EVENT_QUEUE *queue,
   ALLEGRO_EVENT *ret_event, ALLEGRO_TIMEOUT *timeout);
static void copy_event(ALLEGRO_EVENT *dest, const ALLEGRO_EVENT *src);
static void ref_if_user_event(ALLEGRO_EVENT *event);
static void unref_if_user_event(ALLEGRO_EVENT *event);
static void discard_events_of_source(ALLEGRO_EVENT_QUEUE *queue,
   const ALLEGRO_EVENT_SOURCE *source);



/* _al_init_events:
 *  Initialise globals for the event system.
 */
void _al_init_events(void)
{
   _al_mutex_init(&user_event_refcount_mutex);
   _al_add_exit_func(shutdown_events, "shutdown_events");
}



/* shutdown_events:
 *  Clean up after _al_init_events.
 */
static void shutdown_events(void)
{
   _al_mutex_destroy(&user_event_refcount_mutex);
}



/* Function: al_create_event_queue
 */
ALLEGRO_EVENT_QUEUE *al_create_event_queue(void)
{
   ALLEGRO_EVENT_QUEUE *queue = al_malloc(sizeof *queue);

   ASSERT(queue);

   if (queue) {
      _al_vector_init(&queue->sources, sizeof(ALLEGRO_EVENT_SOURCE *));

      _al_vector_init(&queue->events, sizeof(ALLEGRO_EVENT));
      _al_vector_alloc_back(&queue->events);
      queue->events_head = 0;
      queue->events_tail = 0;

      _AL_MARK_MUTEX_UNINITED(queue->mutex);
      _al_mutex_init(&queue->mutex);
      _al_cond_init(&queue->cond);

      _al_register_destructor(_al_dtor_list, queue,
         (void (*)(void *)) al_destroy_event_queue);
   }

   return queue;
}



/* Function: al_destroy_event_queue
 */
void al_destroy_event_queue(ALLEGRO_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   _al_unregister_destructor(_al_dtor_list, queue);

   /* Unregister any event sources registered with this queue.  */
   while (_al_vector_is_nonempty(&queue->sources)) {
      ALLEGRO_EVENT_SOURCE **slot = _al_vector_ref_back(&queue->sources);
      al_unregister_event_source(queue, *slot);
   }

   ASSERT(_al_vector_is_empty(&queue->sources));
   _al_vector_free(&queue->sources);

   ASSERT(queue->events_head == queue->events_tail);
   _al_vector_free(&queue->events);

   _al_cond_destroy(&queue->cond);
   _al_mutex_destroy(&queue->mutex);

   al_free(queue);
}



/* Function: al_register_event_source
 */
void al_register_event_source(ALLEGRO_EVENT_QUEUE *queue,
   ALLEGRO_EVENT_SOURCE *source)
{
   ALLEGRO_EVENT_SOURCE **slot;
   ASSERT(queue);
   ASSERT(source);

   if (!_al_vector_contains(&queue->sources, &source)) {
      _al_event_source_on_registration_to_queue(source, queue);
      _al_mutex_lock(&queue->mutex);
      slot = _al_vector_alloc_back(&queue->sources);
      *slot = source;
      _al_mutex_unlock(&queue->mutex);
   }
}



/* Function: al_unregister_event_source
 */
void al_unregister_event_source(ALLEGRO_EVENT_QUEUE *queue,
   ALLEGRO_EVENT_SOURCE *source)
{
   ALLEGRO_EVENT_SOURCE_REAL *rsource = (ALLEGRO_EVENT_SOURCE_REAL *)source;
   bool found;
   ASSERT(queue);
   ASSERT(source);

   /* Remove source from our list. */
   _al_mutex_lock(&queue->mutex);
   found = _al_vector_find_and_delete(&queue->sources, &source);
   _al_mutex_unlock(&queue->mutex);

   if (found) {
      /* Tell the event source that it was unregistered. */
      _al_event_source_on_unregistration_from_queue(source, queue);

      /* Drop all the events in the queue that belonged to the source. */
      _al_mutex_lock(&queue->mutex);
      discard_events_of_source(queue, source);
      _al_mutex_unlock(&queue->mutex);

      _al_mutex_destroy(&rsource->mutex);
   }
}

/* Function: al_set_event_source_data
 */
void al_set_event_source_data(ALLEGRO_EVENT_SOURCE *source, intptr_t data)
{
   ALLEGRO_EVENT_SOURCE_REAL *const rsource = (ALLEGRO_EVENT_SOURCE_REAL *)source;
   rsource->data = data;
}

/* Function: al_get_event_source_data
 */
intptr_t al_get_event_source_data(const ALLEGRO_EVENT_SOURCE *source)
{
   const ALLEGRO_EVENT_SOURCE_REAL *const rsource = (ALLEGRO_EVENT_SOURCE_REAL *)source;
   return rsource->data;
}

/* Function: al_is_event_queue_empty
 */
bool al_is_event_queue_empty(ALLEGRO_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   return (queue->events_head == queue->events_tail);
}



/* circ_array_next:
 *  Return the next index in a circular array.
 */
static unsigned int circ_array_next(const _AL_VECTOR *vector, unsigned int i)
{
   return (i + 1) % _al_vector_size(vector);
}



/* get_next_event_if_any: [primary thread]
 *  Helper function.  It returns a pointer to the next event in the
 *  queue, or NULL.  Optionally the event is removed from the queue.
 *  However, the event is _not released_ (which is the caller's
 *  responsibility).  The event queue must be locked before entering
 *  this function.
 */
static ALLEGRO_EVENT *get_next_event_if_any(ALLEGRO_EVENT_QUEUE *queue,
   bool delete)
{
   ALLEGRO_EVENT *event;

   if (al_is_event_queue_empty(queue)) {
      return NULL;
   }

   event = _al_vector_ref(&queue->events, queue->events_tail);
   if (delete) {
      queue->events_tail = circ_array_next(&queue->events, queue->events_tail);
   }
   return event;
}



/* Function: al_get_next_event
 */
bool al_get_next_event(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_EVENT *ret_event)
{
   ALLEGRO_EVENT *next_event;
   ASSERT(queue);
   ASSERT(ret_event);

   _al_mutex_lock(&queue->mutex);

   next_event = get_next_event_if_any(queue, true);
   if (next_event) {
      copy_event(ret_event, next_event);
      /* Don't increment reference count on user events. */
   }

   _al_mutex_unlock(&queue->mutex);

   return (next_event ? true : false);
}



/* Function: al_peek_next_event
 */
bool al_peek_next_event(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_EVENT *ret_event)
{
   ALLEGRO_EVENT *next_event;
   ASSERT(queue);
   ASSERT(ret_event);

   _al_mutex_lock(&queue->mutex);

   next_event = get_next_event_if_any(queue, false);
   if (next_event) {
      copy_event(ret_event, next_event);
      ref_if_user_event(ret_event);
   }

   _al_mutex_unlock(&queue->mutex);

   return (next_event ? true : false);
}



/* Function: al_drop_next_event
 */
bool al_drop_next_event(ALLEGRO_EVENT_QUEUE *queue)
{
   ALLEGRO_EVENT *next_event;
   ASSERT(queue);

   _al_mutex_lock(&queue->mutex);

   next_event = get_next_event_if_any(queue, true);
   if (next_event) {
      unref_if_user_event(next_event);
   }

   _al_mutex_unlock(&queue->mutex);

   return (next_event ? true : false);
}



/* Function: al_flush_event_queue
 */
void al_flush_event_queue(ALLEGRO_EVENT_QUEUE *queue)
{
   unsigned int i;
   ASSERT(queue);

   _al_mutex_lock(&queue->mutex);

   /* Decrement reference counts on all user events. */
   i = queue->events_tail;
   while (i != queue->events_head) {
      ALLEGRO_EVENT *old_ev = _al_vector_ref(&queue->events, i);
      unref_if_user_event(old_ev);
      i = circ_array_next(&queue->events, i);
   }

   queue->events_head = queue->events_tail = 0;
   _al_mutex_unlock(&queue->mutex);
}



/* [primary thread] */
/* Function: al_wait_for_event
 */
void al_wait_for_event(ALLEGRO_EVENT_QUEUE *queue, ALLEGRO_EVENT *ret_event)
{
   ALLEGRO_EVENT *next_event = NULL;

   ASSERT(queue);

   _al_mutex_lock(&queue->mutex);
   {
      while (al_is_event_queue_empty(queue)) {
         _al_cond_wait(&queue->cond, &queue->mutex);
      }

      if (ret_event) {
         next_event = get_next_event_if_any(queue, true);
         copy_event(ret_event, next_event);
      }
   }
   _al_mutex_unlock(&queue->mutex);
}



/* [primary thread] */
/* Function: al_wait_for_event_timed
 */
bool al_wait_for_event_timed(ALLEGRO_EVENT_QUEUE *queue,
   ALLEGRO_EVENT *ret_event, float secs)
{
   ALLEGRO_TIMEOUT timeout;

   ASSERT(queue);
   ASSERT(secs >= 0);

   if (secs < 0.0)
      al_init_timeout(&timeout, 0);
   else
      al_init_timeout(&timeout, secs);

   return do_wait_for_event(queue, ret_event, &timeout);
}



/* Function: al_wait_for_event_until
 */
bool al_wait_for_event_until(ALLEGRO_EVENT_QUEUE *queue,
   ALLEGRO_EVENT *ret_event, ALLEGRO_TIMEOUT *timeout)
{
   ASSERT(queue);

   return do_wait_for_event(queue, ret_event, timeout);
}



static bool do_wait_for_event(ALLEGRO_EVENT_QUEUE *queue,
   ALLEGRO_EVENT *ret_event, ALLEGRO_TIMEOUT *timeout)
{
   bool timed_out = false;
   ALLEGRO_EVENT *next_event = NULL;

   _al_mutex_lock(&queue->mutex);
   {
      int result = 0;

      /* Is the queue is non-empty?  If not, block on a condition
       * variable, which will be signaled when an event is placed into
       * the queue.
       */
      while (al_is_event_queue_empty(queue) && (result != -1)) {
         result = _al_cond_timedwait(&queue->cond, &queue->mutex, timeout);
      }

      if (result == -1)
         timed_out = true;
      else if (ret_event) {
         next_event = get_next_event_if_any(queue, true);
         copy_event(ret_event, next_event);
      }
   }
   _al_mutex_unlock(&queue->mutex);

   if (timed_out)
      return false;

   return true;
}



/* expand_events_array:
 *  Expand the circular array holding events.
 */
static void expand_events_array(ALLEGRO_EVENT_QUEUE *queue)
{
   /* The underlying vector grows by powers of two. */
   const size_t old_size = _al_vector_size(&queue->events);
   const size_t new_size = old_size * 2;
   unsigned int i;

   for (i = old_size; i < new_size; i++) {
      _al_vector_alloc_back(&queue->events);
   }

   /* Move wrapped-around elements at the start of the array to the back. */
   if (queue->events_head < queue->events_tail) {
      for (i = 0; i < queue->events_head; i++) {
         ALLEGRO_EVENT *old_ev = _al_vector_ref(&queue->events, i);
         ALLEGRO_EVENT *new_ev = _al_vector_ref(&queue->events, old_size + i);
         copy_event(new_ev, old_ev);
      }
      queue->events_head += old_size;
   }
}


/* alloc_event:
 *
 *  The event source must be _locked_ before calling this function.
 *
 *  [runs in background threads]
 */
static ALLEGRO_EVENT *alloc_event(ALLEGRO_EVENT_QUEUE *queue)
{
   ALLEGRO_EVENT *event;
   unsigned int adv_head;

   adv_head = circ_array_next(&queue->events, queue->events_head);
   if (adv_head == queue->events_tail) {
      expand_events_array(queue);
      adv_head = circ_array_next(&queue->events, queue->events_head);
   }

   event = _al_vector_ref(&queue->events, queue->events_head);
   queue->events_head = adv_head;
   return event;
}



/* copy_event:
 *  Copies the contents of the event SRC to DEST.
 */
static void copy_event(ALLEGRO_EVENT *dest, const ALLEGRO_EVENT *src)
{
   ASSERT(dest);
   ASSERT(src);

   *dest = *src;
}



/* Increment a user event's reference count, if the event passed is a user
 * event and requires it.
 */
static void ref_if_user_event(ALLEGRO_EVENT *event)
{
   if (ALLEGRO_EVENT_TYPE_IS_USER(event->type)) {
      ALLEGRO_USER_EVENT_DESCRIPTOR *descr = event->user.__internal__descr;
      if (descr) {
         _al_mutex_lock(&user_event_refcount_mutex);
         descr->refcount++;
         _al_mutex_unlock(&user_event_refcount_mutex);
      }
   }
}



/* Decrement a user event's reference count, if the event passed is a user
 * event and requires it.
 */
static void unref_if_user_event(ALLEGRO_EVENT *event)
{
   if (ALLEGRO_EVENT_TYPE_IS_USER(event->type)) {
      al_unref_user_event(&event->user);
   }
}



/* Internal function: _al_event_queue_push_event
 *  Event sources call this function when they have something to add to
 *  the queue.  If a queue cannot accept the event, the event's
 *  refcount will not be incremented.
 *
 *  If no event queues can accept the event, the event should be
 *  returned to the event source's list of recyclable events.
 */
void _al_event_queue_push_event(ALLEGRO_EVENT_QUEUE *queue,
   const ALLEGRO_EVENT *orig_event)
{
   ALLEGRO_EVENT *new_event;
   ASSERT(queue);
   ASSERT(orig_event);

   _al_mutex_lock(&queue->mutex);
   {
      new_event = alloc_event(queue);
      copy_event(new_event, orig_event);
      ref_if_user_event(new_event);

      /* Wake up threads that are waiting for an event to be placed in
       * the queue.
       */
      _al_cond_broadcast(&queue->cond);
   }
   _al_mutex_unlock(&queue->mutex);
}



/* contains_event_of_source:
 *  Return true iff the event queue contains an event from the given source.
 *  The queue must be locked.
 */
static bool contains_event_of_source(const ALLEGRO_EVENT_QUEUE *queue,
   const ALLEGRO_EVENT_SOURCE *source)
{
   ALLEGRO_EVENT *event;
   unsigned int i;

   i = queue->events_tail;
   while (i != queue->events_head) {
      event = _al_vector_ref(&queue->events, i);
      if (event->any.source == source) {
         return true;
      }
      i = circ_array_next(&queue->events, i);
   }

   return false;
}



/* Helper to get smallest fitting power of two. */
static int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}



/* discard_events_of_source:
 *  Discard all the events in the queue that belong to the source.
 *  The queue must be locked.
 */
static void discard_events_of_source(ALLEGRO_EVENT_QUEUE *queue,
   const ALLEGRO_EVENT_SOURCE *source)
{
   _AL_VECTOR old_events;
   ALLEGRO_EVENT *old_event;
   ALLEGRO_EVENT *new_event;
   size_t old_size;
   size_t new_size;
   unsigned int i;

   if (!contains_event_of_source(queue, source)) {
      return;
   }

   /* Copy elements we want to keep from the old vector to a new one. */
   old_events = queue->events;
   _al_vector_init(&queue->events, sizeof(ALLEGRO_EVENT));

   i = queue->events_tail;
   while (i != queue->events_head) {
      old_event = _al_vector_ref(&old_events, i);
      if (old_event->any.source != source) {
         new_event = _al_vector_alloc_back(&queue->events);
         copy_event(new_event, old_event);
      }
      else {
         unref_if_user_event(old_event);
      }
      i = circ_array_next(&old_events, i);
   }

   queue->events_tail = 0;
   queue->events_head = _al_vector_size(&queue->events);

   /* The circular array always needs at least one unused element. */
   old_size = _al_vector_size(&queue->events);
   new_size = pot(old_size + 1);
   for (i = old_size; i < new_size; i++) {
      _al_vector_alloc_back(&queue->events);
   }

   _al_vector_free(&old_events);
}



/* Function: al_unref_user_event
 */
void al_unref_user_event(ALLEGRO_USER_EVENT *event)
{
   ALLEGRO_USER_EVENT_DESCRIPTOR *descr;
   int refcount;

   ASSERT(event);

   descr = event->__internal__descr;
   if (descr) {
      _al_mutex_lock(&user_event_refcount_mutex);
      ASSERT(descr->refcount > 0);
      refcount = --descr->refcount;
      _al_mutex_unlock(&user_event_refcount_mutex);

      if (refcount == 0) {
         (descr->dtor)(event);
         al_free(descr);
      }
   }
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
