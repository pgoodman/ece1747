/***************************************************************************************************
 *
 * SUBJECT:
 *    A Benckmark for Massive Multiplayer Online Games
 *    Game Server and Client
 *
 * AUTHOR:
 *    Mihai Paslariu
 *    Politehnica University of Bucharest, Bucharest, Romania
 *    mihplaesu@yahoo.com
 *
 * TIME AND PLACE:
 *    University of Toronto, Toronto, Canada
 *    March - August 2007
 *
 ***************************************************************************************************/

#include "General.h"
#include "Player.h"
#include "Region.h"
#include <math.h>
#include <pthread.h>

Player::Player(IPaddress adr)
    : owned_mutex(SDL_CreateMutex()) {
  int i;
  memset(name, 0, MAX_PLAYER_NAME);
  for (i = 0; i < 8; i++)
    name[i] = 'a' + rand() % ('z' - 'a');
  sprintf(name + i, "_%X_%d", adr.host, adr.port);
  memset(&address, 0, sizeof(address));
  address.host = adr.host;
  address.port = adr.port;

  pos.x = pos.y = 0;
  life = 0;
  attr = 0;
  dir = 0;
  count = 0;
  mutex = owned_mutex;
  time_of_last_message = 0;
}

Player::~Player() {
  if (mutex != NULL)
    SDL_DestroyMutex(mutex);
}

void Player::setName(char *name_) {
  strncpy(name, name_, MAX_PLAYER_NAME);
}

double Player::getDistance(Player* pl) {
  return sqrt(
      (pos.x - pl->pos.x) * (pos.x - pl->pos.x)
          + (pos.y - pl->pos.y) * (pos.y - pl->pos.y));
}

void Player::useObject(GameObject* o) {
  SDL_LockMutex(mutex);
  if (life < 100) {
    o->quantity--;
    life++;
  }
  SDL_UnlockMutex(mutex);
}

__thread int lock_count = 0;
__thread int contention_count = 0;

void Lock(SDL_mutex *m, Region *r) {
  ++r->player_lock_count;
  if (!pthread_mutex_trylock((pthread_mutex_t *) m)) {
    ++r->player_contention_count;
    SDL_LockMutex(m);
  }
}

void Player::attackPlayer(Player* p2, Region *r1, Region *r2) {
  if (this->mutex < p2->mutex) {
    Lock(this->mutex, r1);Lock(p2->mutex, r2);
  } else if (this->mutex > p2->mutex) {
    Lock(p2->mutex, r2);Lock(this->mutex, r1);
  } else {
    Lock(this->mutex, r1);
  }

  p2->dir = -dir;

  /* update players */
  /*if (p2->life > 1) {
    p2->life -= 1;
    if (0 >= p2->life) p2->life = 100;
  }*/
  //  p2->life--,
  //if (life < 100)
  //  life++;

  if (this->mutex < p2->mutex) {
    SDL_UnlockMutex(p2->mutex);SDL_UnlockMutex(this->mutex);
  } else if (this->mutex > p2->mutex) {
    SDL_UnlockMutex(this->mutex);SDL_UnlockMutex(p2->mutex);
  } else {
    SDL_UnlockMutex(this->mutex);
  }
}

