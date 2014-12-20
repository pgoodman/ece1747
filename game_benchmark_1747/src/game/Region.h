#ifndef REGION_H_
#define REGION_H_

#include <list>
#include <atomic>


#include "../General.h"

#include "Player.h"
#include "GameObject.h"

struct Region;

struct RegionGroup {
  RegionGroup(RegionGroup *);
  RegionGroup(int num_regions_, RegionGroup *parent_=nullptr);
  virtual ~RegionGroup(void);
  virtual void update(void);
  virtual Region *find(int x, int y);
  RegionGroup * sub_regions[4];

  // Calculated on-line.
  std::atomic<int> num_player_interactions_;

  // "cached" version of the number of interactions, so that we can reset
  // `num_player_interactions` when we call `update`.
  int num_player_interactions;
  int num_players;
  int num_regions;

  int player_lock_count;
  int player_contention_count;

  SDL_mutex *player_mutex;
  RegionGroup *parent;

  static void addInteraction(RegionGroup *g1, RegionGroup *g2);
};

struct Region : public RegionGroup {
  Region(void) = delete;
  Region(RegionGroup *);
  virtual ~Region();
  virtual Region *find(int x, int y);
  virtual void update();
  
  int region_id;
  Vector2D pos;			// coordinates of the upper left corner of the region
  Vector2D size;			// size of the region

  int t_id;	// = thread_id of the thread handling the players from this region

  std::list<Player*> players;

  std::list<GameObject*> objects;

  SDL_mutex *mutex;
};

void initRegion(int id, Region* r, Vector2D p, Vector2D sz, int _layout,
                std::list<GameObject*> objs, std::list<Player*> pls, SDL_mutex *mutex);
int Region_addPlayer(Region* r, Player* p);
void Region_removePlayer(Region* r, Player* p);
bool Region_movePlayer(Region* r_old, Region* r_new, Player* p, Vector2D n_pos);
Player* Region_getPlayer(Region* r, Vector2D loc);

int Region_addObject(Region* r, GameObject *o, int min_res, int max_res);
GameObject* Region_getObject(Region* r, Vector2D loc);

void Region_rewardPlayers(Region* r, int bonus, int max_life);
void Region_regenerateObjects(Region* r, int max_res);

#endif /*REGION_H_*/
