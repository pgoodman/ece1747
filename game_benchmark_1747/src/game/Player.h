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

#ifndef __PLAYER_H
#define __PLAYER_H

#include "../General.h"
#include "GameObject.h"

struct Region;

class Player {
 public:
  /* player identification */
  char name[MAX_PLAYER_NAME];
  IPaddress address;

  /* player game state */
  Vector2D pos; /* position */
  int life, attr; /* attributes */
  int dir; /* direction (0,1,2,3) = (up,right,down,left) */

  // The mutex that currently guards this player. This might be `owned_mutex`
  // or a region's mutex.
  SDL_mutex *mutex;

  // The actual player mutex.
  SDL_mutex * const owned_mutex;

  /* parameter used in operations requiring retries */
  int count;
  Uint32 time_of_last_message;

 public:
  /* constructors / destructor */
  Player(IPaddress adr);
  ~Player();

 public:
  /* public methods */
  void setName(char *name);
  double getDistance(Player* pl);

  void useObject(GameObject* o);
  void attackPlayer(Player* p2, Region *r1, Region *r2);
};

#endif
