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

#include "ServerData.h"

void ServerData::dataFromConfigurator(Configurator &conf) {
  /* server */
  this->num_threads = conf.getIntAttribute("server.number_of_threads");
  this->regular_update_interval = conf.getIntAttribute(
      "server.regular_update_interval");

  printf("Number of threads: %d\n", this->num_threads);
  if (this->num_threads > MAX_THREADS) {
    printf("The maximum number of threads is %d\n", MAX_THREADS);
    throw "Config file: Too many threads";
  }
  for(int i = 0; i < this->num_threads; ++i)
  {
    this->zigzag_tids.push_back(i); 
  }
  for(int i = this->num_threads; i-->0 ;)
  {
    this->zigzag_tids.push_back(i); 
  }
  if (this->regular_update_interval < 0)
    throw "Config file: Regular update interval must be positive";

  /* load balance */
  strcpy(this->algorithm_name, conf.getAttribute("server.balance"));
  this->load_balance_limit = conf.getIntAttribute("server.load_balance_limit")
      * 1000;
  this->overloaded_level = conf.getFloatAttribute("server.overloaded_level");
  this->light_level = conf.getFloatAttribute("server.light_level");

  this->num_sla_violations.reserve(this->num_threads);

  if (this->load_balance_limit <= 0) {
    printf(
        "[WARNING] Config file error: load_balance_limit must be greater than 0. Default value 10 used\n");
    this->overloaded_level = 10 * 1000;
  }
  if (this->overloaded_level > 1.0) {
    printf(
        "[WARNING] Config file error: overloaded_level must be less of equal to 1. Default value 0.8 used\n");
    this->overloaded_level = 0.8;
  }
  if (this->light_level < 0 || this->light_level > this->overloaded_level) {
    printf(
        "[WARNING] Config file error: light_level must be greater or equal to 0 and smaller than overloaded_level. Default value 0.2 used\n");
    this->light_level = 0.2;
  }

  /* Map and region size */
  this->wm.size.x = conf.getIntAttribute("map.width") * CLIENT_MATRIX_SIZE;
  this->wm.size.y = this->wm.size.x; //conf.getIntAttribute("map.height") * CLIENT_MATRIX_SIZE;
  this->wm.regmin.x = conf.getIntAttribute(
      "map.region_min_width") * CLIENT_MATRIX_SIZE;
  this->wm.regmin.y = this->wm.regmin.x;
  this->wm.regmax.x = this->wm.regmin.x;
  this->wm.regmax.y =this->wm.regmin.x;
  if (this->wm.size.x <= 0 || this->wm.size.y <= 0)
    throw "Config file: Invalid map dimensions";

  /* Region data */
  this->wm.blocks = conf.getIntAttribute("map.blocks");
  this->wm.resources = conf.getIntAttribute("map.resources");
  this->wm.min_res = conf.getIntAttribute("map.min_res");
  this->wm.max_res = conf.getIntAttribute("map.max_res");

  /* Player data */
  this->player_min_life = conf.getIntAttribute("player.min_life");
  this->player_max_life = conf.getIntAttribute("player.max_life");
  this->player_min_attr = conf.getIntAttribute("player.min_attr");
  this->player_max_attr = conf.getIntAttribute("player.max_attr");

  /* Quest data */
  this->quest_min = conf.getIntAttribute("quest.min") * 1000;
  this->quest_max = conf.getIntAttribute("quest.max") * 1000;
  this->quest_between = conf.getIntAttribute("quest.between") * 1000;
  this->quest_bonus = conf.getIntAttribute("quest.bonus");
  if (this->quest_min <= 0 || this->quest_max <= this->quest_min
      || this->quest_bonus < 0 || this->quest_bonus > 100
      || this->quest_between <= 0)
    throw "Invalid or incomplete values for quest properties";

  send_start_quest = 0;
  send_end_quest = 0;

  /* ServerOutput */
  this->display_all_warnings = conf.getIntAttribute("display.all_warnings");
  this->display_quests = conf.getIntAttribute("display.quests");
  this->display_actions = conf.getIntAttribute("display.actions");
  this->display_user_on_off = conf.getIntAttribute("display.user_on_off");
  this->display_migrations = conf.getIntAttribute("display.migrations");
  this->balance_players = conf.getIntAttribute("server.balance_players");
}

ServerData::ServerData(char* conf_file) {
  Configurator conf;
  if (!conf.addFile(conf_file))
    throw "Invalid configuration file";
  dataFromConfigurator(conf);
}

