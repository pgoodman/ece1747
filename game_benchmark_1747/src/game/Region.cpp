#include "Region.h"
#include <iostream>
#include <math.h>

RegionGroup::RegionGroup(RegionGroup *parent_)
    : num_player_interactions_(ATOMIC_VAR_INIT(0)) {
  num_regions = 1;
  sub_regions[0] = nullptr;
  sub_regions[1] = nullptr;
  sub_regions[2] = nullptr;
  sub_regions[3] = nullptr;
  num_players = 0;
  player_mutex = SDL_CreateMutex();
  num_player_interactions = 0;
  parent = parent_;
}

RegionGroup::RegionGroup(int num_regions_, RegionGroup *parent_)
    : num_player_interactions_(ATOMIC_VAR_INIT(0)) {
  num_regions = num_regions_;
	std::cout <<"new region nb: "<<num_regions<<std::endl;
  if (4 == num_regions) {
    sub_regions[0] = new Region(this);
    sub_regions[1] = new Region(this);
    sub_regions[2] = new Region(this);
    sub_regions[3] = new Region(this);
  } else {
    auto num_sub_regions = num_regions / 4;
    sub_regions[0] = new RegionGroup(num_sub_regions, this);
    sub_regions[1] = new RegionGroup(num_sub_regions, this);
    sub_regions[2] = new RegionGroup(num_sub_regions, this);
    sub_regions[3] = new RegionGroup(num_sub_regions, this);
  }
  num_players = 0;
  player_mutex = SDL_CreateMutex();
  num_player_interactions = 0;
  parent = parent_;
}

RegionGroup::~RegionGroup(void)
{
}

void RegionGroup::update(void)
{
  num_player_interactions = num_player_interactions_.load(std::memory_order_relaxed);
  num_player_interactions_.store(0, std::memory_order_relaxed);

  num_players = 0;
  for(auto sub_region: sub_regions)
  {
    sub_region->update();
    num_players += sub_region->num_players;
  }
}

Region *RegionGroup::find(int x, int y) {
  //the sqrt gives use the side lenght of the regiongroup
  //and we want to divide that by 2 to get the split
  auto split = sqrt(num_regions)/2 ;

  auto x_split = x - split;
  auto y_split = y - split;
  if (x < split) {
    if (y < split) {
      return sub_regions[0]->find(x, y);
    } else {
      return sub_regions[1]->find(x, y_split);
    }
  } else {
    if (y < split) {
      return sub_regions[2]->find(x_split, y);
    } else {
      return sub_regions[3]->find(x_split, y_split);
    }
  }
}

void RegionGroup::addInteraction(RegionGroup *g1, RegionGroup *g2) {
/*  while (g1 != g2) {
    g1 = g1->parent;
    g2 = g2->parent;
  }
  assert(g1 && g2);
  g1->num_player_interactions_.fetch_add(1); */
  g1->num_player_interactions_.fetch_add(1);
  g2->num_player_interactions_.fetch_add(1);
}

Region::Region(RegionGroup *parent_)
    : RegionGroup(parent_) {
  t_id = -1;
}

Region::~Region(void)
{}

void Region::update(void) {
  num_player_interactions = num_player_interactions_.load(std::memory_order_relaxed);
  num_player_interactions_.store(0, std::memory_order_relaxed);
}

Region *Region::find(int x, int y) {
  return this;
}

void initRegion(int id, Region* r, Vector2D p, Vector2D sz, int t_id,
                std::list<GameObject*> objs, std::list<Player*> pls, SDL_mutex *mutex) {
  r->region_id = id;
  r->pos = p;
  r->size = sz;

  r->t_id = t_id;

  r->objects = objs;
  r->players = pls;
  r->num_players = pls.size();

  r->mutex = mutex;
}

int Region_addPlayer(Region* r, Player* p) {
  for (auto obj : r->objects) {
    if (obj->pos.x == p->pos.x && obj->pos.y == p->pos.y) {
      return 0;
    }
  }

  SDL_LockMutex(r->mutex);

  for (auto player : r->players) {
    if (player->pos.x == p->pos.x && player->pos.y == p->pos.y) {
      SDL_UnlockMutex(r->mutex);
      return 0;
    }
  }

  r->players.push_front(p);
  r->num_players++;

  SDL_UnlockMutex(r->mutex);

  return 1;
}

void Region_removePlayer(Region* r, Player* p) {
  SDL_LockMutex(r->mutex);

 std::list<Player*>::iterator ip;  //iterator for players
  for (ip = r->players.begin(); ip != r->players.end(); ip++) {
    if ((*ip) == p) {
      r->players.erase(ip);
      r->num_players--;
      break;
    }
  }

  SDL_UnlockMutex(r->mutex);
}

bool Region_movePlayer(Region* r_old, Region* r_new, Player* p,
                       Vector2D n_pos) {
  bool res = false;
  if (*(int*) r_old < *(int*) r_new) {
    SDL_LockMutex(r_old->mutex);
    SDL_LockMutex(r_new->mutex);
  }
  if (*(int*) r_old > *(int*) r_new) {
    SDL_LockMutex(r_new->mutex);
    SDL_LockMutex(r_old->mutex);
  }
  if (*(int*) r_old == *(int*) r_new) {
    SDL_LockMutex(r_old->mutex);
  }

  /* client tries to move over another player */
 std::list<Player*>::iterator ip;
  for (ip = r_new->players.begin(); ip != r_new->players.end(); ip++)
    if ((*ip)->pos.x == n_pos.x && (*ip)->pos.y == n_pos.y) {
      goto movePlayer_end;
    }

  /* move player */
  p->pos = n_pos;
  if (r_old != r_new) {
    for (ip = r_old->players.begin(); ip != r_old->players.end(); ip++)
      if ((*ip) == p) {
        r_old->players.erase(ip);
        r_old->num_players--;
        break;
      }

    r_new->players.push_front(p);
    r_new->num_players++;
  }

  res = true;

  movePlayer_end:

  if (*(int*) r_old < *(int*) r_new) {
    SDL_UnlockMutex(r_old->mutex);SDL_UnlockMutex(r_new->mutex);
  }
  if (*(int*) r_old > *(int*) r_new) {
    SDL_UnlockMutex(r_new->mutex);SDL_UnlockMutex(r_old->mutex);
  }
  if (*(int*) r_old == *(int*) r_new) {
    SDL_UnlockMutex(r_old->mutex);
  }

  return res;
}

Player* Region_getPlayer(Region* r, Vector2D loc) {
  SDL_LockMutex(r->mutex);

  std::list<Player*>::iterator ip;  //iterator for players
  Player *p = NULL;
  for (ip = r->players.begin(); ip != r->players.end(); ip++) {
    if ((*ip)->pos.x == loc.x && (*ip)->pos.y == loc.y) {
      p = (Player*) *ip;
      break;
    }
  }

  SDL_UnlockMutex(r->mutex);
  return p;
}

int Region_addObject(Region* r, GameObject *o, int min_res, int max_res) {
  std::list<GameObject*>::iterator oi;  //iterator for objects
  for (oi = r->objects.begin(); oi != r->objects.end(); oi++) {
    if ((*oi)->pos.x == o->pos.x && (*oi)->pos.y == o->pos.y) {
      return 0;
    }
  }

  o->attr = rand() % 256;
  o->quantity = min_res + rand() % (max_res - min_res + 1);
  r->objects.push_front(o);

  return 1;
}

GameObject* Region_getObject(Region* r, Vector2D loc) {
  std::list<GameObject*>::iterator io;  //iterator for objects
  GameObject *o = NULL;
  for (io = r->objects.begin(); io != r->objects.end(); io++) {
    if ((*io)->pos.x == loc.x && (*io)->pos.y == loc.y) {
      o = (GameObject*) *io;
      break;
    }
  }
  return o;
}

void Region_regenerateObjects(Region* r, int max_res) {
  std::list<GameObject*>::iterator io;  //iterator for objects
  for (io = r->objects.begin(); io != r->objects.end(); io++) {
    if ((*io)->quantity < max_res) {
      (*io)->quantity++;
    }
  }
}

void Region_rewardPlayers(Region* r, int bonus, int max_life) {
  std::list<Player*>::iterator ip;  //iterator for players
  for (ip = r->players.begin(); ip != r->players.end(); ip++) {
    (*ip)->life = std::min((*ip)->life + bonus, max_life);
  }
}
