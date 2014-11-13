#include <iostream>
#include <algorithm>
#include <iomanip>
#include <set>

#include "ServerData.h"
#include "WorldMap.h"

#include "../tracing/trace.h"
#define P( ...)

void WorldMap::generate() {
  int i, j;
  Vector2D pos;

  /* generate terrain */
  terrain = new char*[size.x];
  for (i = 0; i < size.x; i++) {
    terrain[i] = new char[size.y];
    for (j = 0; j < size.y; j++)
      terrain[i][j] = (rand() % 1000 < blocks) ? 1 : 0;
  }

  players = new PlayerBucket[sd->num_threads];
  n_players = 0;

  std::list<Player*> pls;
  std::list<GameObject*> objs;
  thread_regions = new std::set<Region *>[sd->num_threads];

  n_regs.x = size.x / regmin.x;
  n_regs.y = size.y / regmin.y;
  int regions_per_thread = (n_regs.x * n_regs.y - 1) / sd->num_threads + 1;
  regions = new Region*[n_regs.x];
  for (i = 0, pos.x = 0; i < n_regs.x; i++, pos.x += regmin.x) {
    regions[i] = new Region[n_regs.y];
    for (j = 0, pos.y = 0; j < n_regs.y; j++, pos.y += regmin.y) {
      auto t_id = (i * n_regs.y + j) / regions_per_thread;
      auto region = &(regions[i][j]);
      initRegion(region, pos, regmin, t_id, objs, pls);
      thread_regions[t_id].insert(region);
      all_regions.insert(region);
    }

  }

  /* generate objects */
  GameObject *o;
  Region* r;

  for (i = 0; i < resources * size.x * size.y / 1000; i++) {
    o = new GameObject();
    while (1) {
      o->pos.x = rand() % size.x;
      o->pos.y = rand() % size.y;
      if (terrain[o->pos.x][o->pos.y] != 0)
        continue;
      r = getRegionByLocation(o->pos);
      assert(r);
      if (Region_addObject(r, o, min_res, max_res))
        break;
    }
  }
}

Player* WorldMap::addPlayer(IPaddress a) {
  /* create a new player */
  Player* p = new Player(a);
  assert(p);

  /* set player name and initial attributes */
  char pname[MAX_PLAYER_NAME];
  sprintf(pname, "Player_%X_%d", a.host, a.port);
  p->setName(pname);
  p->life = sd->player_min_life
      + rand() % (sd->player_max_life - sd->player_min_life + 1);
  p->attr = sd->player_min_attr
      + rand() % (sd->player_max_attr - sd->player_min_attr + 1);
  p->time_of_last_message = SDL_GetTicks();

  Region* r = NULL;

  while (1) {
    p->pos.x = rand() % size.x;
    p->pos.y = rand() % size.y;
    if (terrain[p->pos.x][p->pos.y] != 0)
      continue;
    r = getRegionByLocation(p->pos);
    assert(r);
    if (Region_addPlayer(r, p))
      break;
  }

  players[r->t_id].insert(p);

  return p;
}

Player* WorldMap::findPlayer(IPaddress a, int t_id) {
  int i;
  Player* p = NULL;
  for (i = 0; i < sd->num_threads; i++) {
    p = players[(t_id + i) % sd->num_threads].find(a);
    if (p)
      break;
  }
  return p;
}

void WorldMap::removePlayer(Player* p) {
  Region* r = getRegionByLocation(p->pos);
  Region_removePlayer(r, p);
  players[r->t_id].erase(p);
}

void WorldMap::movePlayer(Player* p) {
  Vector2D n_pos = p->pos;
  if (p->dir == 0)
    n_pos.y = p->pos.y + 1;		// DOWN
  if (p->dir == 1)
    n_pos.x = p->pos.x + 1;		// RIGHT
  if (p->dir == 2)
    n_pos.y = p->pos.y - 1;		// UP
  if (p->dir == 3)
    n_pos.x = p->pos.x - 1;		// LEFT

  /* the player is on the edge of the map */
  if (n_pos.x < 0 || n_pos.x >= size.x || n_pos.y < 0 || n_pos.y >= size.y)
    return;
  /* client tries to move to a blocked area */
  if (terrain[n_pos.x][n_pos.y] != 0)
    return;

  /* get old and new region */
  Region *r_old = getRegionByLocation(p->pos);
  Region *r_new = getRegionByLocation(n_pos);
  assert(r_old && r_new);

  bool res = Region_movePlayer(r_old, r_new, p, n_pos);
  if (r_old->t_id != r_new->t_id && res) {
    players[r_old->t_id].erase(p);
    players[r_new->t_id].insert(p);
  }
}

void WorldMap::useGameObject(Player* p) {
  assert(p);
  Region* r = getRegionByLocation(p->pos);
  assert(r);
  GameObject* o = Region_getObject(r, p->pos);

  // Fix for UDP out of order bug
  if (o) {
    if (o->quantity > 0)
      p->useObject(o);
  }
}

void WorldMap::attackPlayer(Player* p, int attack_dir) {
  assert(p);
  /* get second player */
  Vector2D pos2 = p->pos;
  if (attack_dir == 0)
    pos2.y = p->pos.y + 1;		// DOWN
  if (attack_dir == 1)
    pos2.x = p->pos.x + 1;		// RIGHT
  if (attack_dir == 2)
    pos2.y = p->pos.y - 1;		// UP
  if (attack_dir == 3)
    pos2.x = p->pos.x - 1;		// LEFT

  /* check if coordinates are inside the map  */
  if (pos2.x < 0 || pos2.x >= size.x || pos2.y < 0 || pos2.y >= size.y)
    return;

  /* get second player */
  Region* r = getRegionByLocation(pos2);
  assert(r);
  Player* p2 = Region_getPlayer(r, pos2);
  if (p2 != NULL)
    p->attackPlayer(p2);

  if (sd->display_actions)
    printf("Player %s attacks %s\n", p->name, p2->name);
}

void packRegion(Region* r, Serializator* s, Player* p, Vector2D pos1,
                Vector2D pos2) {
  Player* p2;
  GameObject* o;
  std::list<Player*>::iterator ip;
  std::list<GameObject*>::iterator io;

  for (ip = r->players.begin(); ip != r->players.end(); ip++) {
    p2 = *ip;
    if (p2 == p)
      continue;
    if (p2->pos.x < pos1.x || p2->pos.y < pos1.y || p2->pos.x >= pos2.x
        || p2->pos.y >= pos2.y)
      continue;

    *s << CELL_PLAYER;
    *s << p2->pos.x;
    *s << p2->pos.y;
    *s << p2->life;
    *s << p2->attr;
    *s << p2->dir;
    /* IPaddress used as an ID: */
    s->putBytes((char*) &p2->address, sizeof(IPaddress));
  }

  for (io = r->objects.begin(); io != r->objects.end(); io++) {
    o = *io;
    if (o->quantity <= 0)
      continue;
    if (o->pos.x < pos1.x || o->pos.y < pos1.y || o->pos.x >= pos2.x
        || o->pos.y >= pos2.y)
      continue;

    *s << CELL_OBJECT;
    *s << o->pos.x;
    *s << o->pos.y;
    *s << o->attr;
    *s << o->quantity;
  }
}

void WorldMap::updatePlayer(Player* p, Serializator* s) {
  int i, j;
  Vector2D pos1, pos2; /* rectangular region visible to the player */
  Vector2D loc;

  /* determine the region visible to the player */
  pos1.x = max(p->pos.x - MAX_CLIENT_VIEW, 0);
  pos1.y = max(p->pos.y - MAX_CLIENT_VIEW, 0);
  pos2.x = min(p->pos.x + MAX_CLIENT_VIEW + 1, size.x);
  pos2.y = min(p->pos.y + MAX_CLIENT_VIEW + 1, size.y);

  /* pack data: position, attributes */
  *s << p->pos.x;
  *s << p->pos.y;
  *s << pos1.x;
  *s << pos1.y;
  *s << pos2.x;
  *s << pos2.y;
  *s << p->life;
  *s << p->attr;
  *s << p->dir;

  /* pack data: terrain, objects, players */
  for (i = pos1.x; i < pos2.x; i++)
    for (j = pos1.y; j < pos2.y; j++)
      *s << terrain[i][j];

  loc.x = pos1.x;
  loc.y = pos1.y;
  packRegion(getRegionByLocation(loc), s, p, pos1, pos2);

  if ((pos2.x - 1) / regmin.x != pos1.x / regmin.x) {
    loc.x = pos2.x - 1;
    loc.y = pos1.y;
    packRegion(getRegionByLocation(loc), s, p, pos1, pos2);
  }
  if ((pos2.y - 1) / regmin.y != pos1.y / regmin.y) {
    loc.x = pos1.x;
    loc.y = pos2.y - 1;
    packRegion(getRegionByLocation(loc), s, p, pos1, pos2);
  }
  if ((pos2.x - 1) / regmin.x != pos1.x / regmin.x
      && (pos2.y - 1) / regmin.y != pos1.y / regmin.y) {
    loc.x = pos2.x - 1;
    loc.y = pos2.y - 1;
    packRegion(getRegionByLocation(loc), s, p, pos1, pos2);
  }

  *s << CELL_NONE;
}

Region* WorldMap::getRegionByLocation(Vector2D loc) {
  return &regions[loc.x / regmin.x][loc.y / regmin.y];
}

void WorldMap::printRegions() {
  cout << " X\t";
  for (int i = 0; i < n_regs.x; ++i) {
    cout << i << "\t";
  }
  cout << endl << "Y" << endl;

  for (int j = 0; j < n_regs.y; ++j) {
    cout << j << "\t";
    for (int i = 0; i < n_regs.x; ++i) {
      //cout<<regions[i][j].pos.x<<","<<regions[i][j].pos.y<<" "<<regions[i][j].t_id <<"\t";
      cout << regions[i][j].t_id << "\t";
    }
    cout << endl;
  }

}
void WorldMap::regenerateObjects() {
  Region_regenerateObjects(&regions[rand() % n_regs.x][rand() % n_regs.y],
                           max_res);
}

void WorldMap::rewardPlayers(Vector2D quest_pos) {
  Region_rewardPlayers(getRegionByLocation(quest_pos), sd->quest_bonus,
                       sd->player_max_life);
}

void WorldMap::reassignRegion(Region* r, int t_id) {
  if (r->t_id == t_id) {
    return;
  }
  for (auto player : r->players) {
    players[r->t_id].erase(player);
    players[t_id].insert(player);
  }

  thread_regions[r->t_id].erase(r);
  r->t_id = t_id;
  thread_regions[r->t_id].insert(r);
}

struct LoadedThread {
  int load;
  int num_players;
  int t_id;

  inline LoadedThread(double load_, int num_players_, int t_id_)
      : load((int) (100 * load_)),
        num_players(num_players_),
        t_id(t_id_) {
  }

  bool operator<(const LoadedThread &that) const {
    if (load < that.load)
      return true;
    else if (load > that.load)
      return false;

    if (num_players < that.num_players)
      return true;
    else if (num_players > that.num_players)
      return false;

    return t_id < that.t_id;
  }
};

// Returns the region owned by a thread that has the fewest number of players.
//
// The purpose of this is that we hope to avoid the bad case of a thread
// shedding a super loaded region over to another thread, then having that
// heavy region just bounce between threads.
Region *RegionWithFewestPlayers(WorldMap *wm, int t_id) {
  Region *lightest_region(nullptr);
  for (auto region : wm->thread_regions[t_id]) {
    if (region->num_players) {
      if (!lightest_region
          || region->num_players < lightest_region->num_players) {
        lightest_region = region;
      }
    }
  }
  assert(nullptr != lightest_region);
  return lightest_region;
}

void WorldMap::balance_lightest() {
  std::set<LoadedThread> overloaded_threads;

  // Underloaded threads, ordered by least loaded to greatest loaded.
  std::set<LoadedThread> underloaded_threads;

  int total_num_players = 0;

  // Find the over- and under-loaded threads.
  for (int t_id = 0; t_id < sd->num_threads; ++t_id) {
    int num_players_ = sd->wm.players[t_id].size();
    total_num_players += num_players_;

    double num_players = (double) num_players_;
    double num_sla_violations = (double) sd->num_sla_violations[t_id];
    assert(num_sla_violations <= num_players);
    double load_ratio = !num_players_ ? 0.0 : num_sla_violations / num_players;

    P(cout << "(" << t_id << ", " << std::setprecision(2) << load_ratio
        << ", " << num_players_ << ") ";)

    // Store regions from most overloaded to least overloaded.
    if (load_ratio > sd->overloaded_level) {
      overloaded_threads.insert(
          LoadedThread(1.0 - load_ratio, num_players_, t_id));

      // Store regions from having the least load to being up to the light load
      // level.
    } else if (load_ratio < sd->light_level) {
      underloaded_threads.insert(LoadedThread(load_ratio, num_players_, t_id));
    }
  }

  P(cout << " = " << total_num_players << endl;)

  if (underloaded_threads.empty()) {
    P( cout << "Could not re-balance overloaded threads! No threads are "
        << "underloaded." << endl;)
    return;
  }

  LoadedThread uthread(0.0, 0, -1);
  for (const auto &othread : overloaded_threads) {
    auto region = RegionWithFewestPlayers(this, othread.t_id);

    // Shed to either the last underloaded thread that we found, or to the
    // next underloaded thread.
    if (!underloaded_threads.empty()) {
      auto it = underloaded_threads.begin();
      uthread = *it;
      underloaded_threads.erase(it);
    }

    reassignRegion(region, uthread.t_id);

  P(cout << "Thread " << othread.t_id << " overloaded ("
      << (1.0 - othread.load) << "). Shedding region to " << uthread.t_id
      << endl;)
}

if (!overloaded_threads.empty()) {
P( printRegions();)
}
}

struct ordered_region {
Region * r;
ordered_region(Region * r_)
  : r(r_) {
}
    //Sorted from largest number of player to smallest seems a better idea
    //so we don't get stuck at the end of with several avgly loaded thread and
    //a highly populated region to give.
bool operator<(const ordered_region &that) const {
if (r->num_players > that.r->num_players)
  return true;
else if (r->num_players < that.r->num_players)
  return false;
else
  return r > that.r;
}
};

void WorldMap::balance_spread() {
int targeted_avg = (int) (n_players / (double) sd->num_threads);
std::set<ordered_region> regions;
vector<int> num_players(sd->num_threads);

   // order all of our regions
for (auto region : all_regions) {
regions.insert(region);
}

for (bool made_progress = true; made_progress;) {

made_progress = false;
for (auto tid : sd->zigzag_tids) {
  if (regions.empty()) {
    goto end;
  }

  auto oregion_it = regions.begin();
  auto region = oregion_it->r;

  // Try to assign a region to a thread.
  if (!num_players[tid]
      || targeted_avg < (num_players[tid] + region->num_players)) {
    reassignRegion(region, tid);
    num_players[tid] += region->num_players;
    regions.erase(oregion_it);
    made_progress = true;
  }
}
}

for (auto oregion : regions) {
//Tid with the minimum number of players
auto tid = std::min_element(num_players.begin(), num_players.end())
    - num_players.begin();
auto region = oregion.r;

reassignRegion(region, tid);
num_players[tid] += region->num_players;
}
end: P(printRegions();)
return;
}

void WorldMap::balance() {
Uint32 now = SDL_GetTicks();
if (now - last_balance < sd->load_balance_limit)
return;
last_balance = now;

if (!strcmp(sd->algorithm_name, "static"))
return;

n_players = 0;
for (int i = 0; i < sd->num_threads; i++)
n_players += players[i].size();
if (n_players == 0)
return;

if (!strcmp(sd->algorithm_name, "lightest")) {
balance_lightest();
return;
}

if (!strcmp(sd->algorithm_name, "spread")) {
balance_spread();
return;
}

printf("Algorithm %s is not implemented.\n", sd->algorithm_name);
return;
}
