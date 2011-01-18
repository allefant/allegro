#ifndef __al_included_allegro5_transformations_h
#define __al_included_allegro5_transformations_h

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_TRANSFORM
 */
typedef struct ALLEGRO_TRANSFORM ALLEGRO_TRANSFORM;

struct ALLEGRO_TRANSFORM {
   float m[4][4];
};

/* Transformations*/
AL_FUNC(void, al_use_transform, (const ALLEGRO_TRANSFORM* trans));
AL_FUNC(void, al_copy_transform, (ALLEGRO_TRANSFORM* dest, const ALLEGRO_TRANSFORM* src));
AL_FUNC(void, al_identity_transform, (ALLEGRO_TRANSFORM* trans));
AL_FUNC(void, al_build_transform, (ALLEGRO_TRANSFORM* trans, float x, float y, float sx, float sy, float theta));
AL_FUNC(void, al_translate_transform, (ALLEGRO_TRANSFORM* trans, float x, float y));
AL_FUNC(void, al_rotate_transform, (ALLEGRO_TRANSFORM* trans, float theta));
AL_FUNC(void, al_scale_transform, (ALLEGRO_TRANSFORM* trans, float sx, float sy));
AL_FUNC(void, al_transform_coordinates, (const ALLEGRO_TRANSFORM* trans, float* x, float* y));
AL_FUNC(void, al_compose_transform, (ALLEGRO_TRANSFORM* trans, const ALLEGRO_TRANSFORM* other));
AL_FUNC(const ALLEGRO_TRANSFORM*, al_get_current_transform, (void));
AL_FUNC(const ALLEGRO_TRANSFORM*, al_get_current_inverse_transform, (void));
AL_FUNC(void, al_invert_transform, (ALLEGRO_TRANSFORM *trans));
AL_FUNC(int, al_check_inverse, (const ALLEGRO_TRANSFORM *trans, float tol));
AL_FUNC(void, al_ortho_transform, (ALLEGRO_TRANSFORM *trans, float left, float right, float top, float bottom, float n, float f));
AL_FUNC(ALLEGRO_TRANSFORM *, al_get_projection_transform, (ALLEGRO_DISPLAY *display));
AL_FUNC(void, al_set_projection_transform, (ALLEGRO_DISPLAY *display, ALLEGRO_TRANSFORM *t));

#ifdef __cplusplus
   }
#endif

#endif
