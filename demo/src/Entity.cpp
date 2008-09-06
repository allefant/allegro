#include "a5teroids.hpp"

bool Entity::logic(int step)
{
   if (hp <= 0) {
      spawn();
      ResourceManager& rm = ResourceManager::getInstance();
      Player *p = (Player *)rm.getData(RES_PLAYER);
      p->addScore(points);
      return false;
   }
   
   if (hilightCount > 0) {
      hilightCount -= step;
   }

   return true;
}

float Entity::getX(void)
{
   return x;
}

float Entity::getY(void)
{
   return y;
}

float Entity::getRadius(void)
{
   return radius;
}

bool Entity::getDestructable(void)
{
   return isDestructable;
}

bool Entity::isHighlighted(void)
{
   return hilightCount > 0;
}

bool Entity::isUFO(void)
{
   return ufo;
}

void Entity::setPowerUp(int type)
{
   powerup = type;
}

void Entity::wrap(void)
{
   x += dx;
   y += dy;
   if (x < 0) x += BB_W;
   if (x >= BB_W) x -= BB_W;
   if (y < 0) y += BB_H;
   if (y >= BB_H) y -= BB_H;
}

void Entity::render_four(void)
{
   int ox = 0;
   render(0, 0);
   if (x > BB_W / 2) {
      ox = -BB_W;
      render(ox, 0);
   }
   else {
      ox = BB_W;
      render(ox, 0);
   }
   if (y > BB_H / 2) {
      render(0, -BB_H);
      render(ox, -BB_H);
   }
   else {
      render(0, BB_H);
      render(ox, BB_H);
   }
   
   #ifdef DEBUG_COLLISION_CIRCLES
   float x1 = x + cos(0) * radius;
   float y1 = y + sin(0) * radius;
   for (int i = 0; i < 30; i++) {
      float x2 = x + cos((i + 1) * AL_PI * 2 / 30) * radius;
      float y2 = y + sin((i + 1) * AL_PI * 2 / 30) * radius;
      al_draw_line(x1, y1, x2, y2, al_map_rgba_f(1, 0, 0, 1));
      x1 = x2;
      y1 = y2;
   }
   #endif
   
}

Entity *Entity::checkCollisions(std::list<Entity *>& e)
{
   std::list<Entity *>::iterator it;

   for (it = e.begin(); it != e.end(); it++) {
      Entity *entity = *it;
      if (entity == this || !entity->getDestructable())
         continue;
      float ex = entity->getX();
      float ey = entity->getY();
      float er = entity->getRadius();
      if (checkCircleCollision(ex, ey, er, x, y, radius))
         return entity;
   }

   return 0;
}

Entity *Entity::getPlayerCollision(void)
{
   std::list<Entity *> e;

   ResourceManager& rm = ResourceManager::getInstance();
   Player *player = (Player *)rm.getData(RES_PLAYER);

   e.push_back(player);

   Entity *ret = checkCollisions(e);
   e.clear();
   return ret;
}

Entity *Entity::getEntityCollision(void)
{
   return checkCollisions(entities);
}

Entity *Entity::getAllCollision(void)
{
   Entity *e = getEntityCollision();
   if (e) return e;
   return getPlayerCollision();
}

// Returns true if dead
bool Entity::hit(int damage)
{
   hp-=damage;

   hilightCount = 500;

   bool ret;

   if (hp <= 0) {
      ret = true;
      explode();
   }
   else
      ret = false;

   return ret;
}

void Entity::explode(void)
{
   bool big;
   if (radius >= 32) big = true;
   else big = false;
   Explosion *e = new Explosion(x, y, big);
   new_entities.push_back(e);
   if (big) my_play_sample(RES_BIGEXPLOSION);
   else my_play_sample(RES_SMALLEXPLOSION);
}

void Entity::spawn(void)
{
   if (powerup >= 0) {
      PowerUp *p = new PowerUp(x, y, powerup);
      new_entities.push_back(p);
   }
}

Entity::Entity() :
   isDestructable(true),
   hp(1),
   powerup(-1),
   hilightCount(0),
   points(0),
   ufo(false)
{
}

