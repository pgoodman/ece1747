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

#ifndef __WORLD_UPDATE_MODULE
#define __WORLD_UPDATE_MODULE

#include "../General.h"
#include "../utils/SDL_barrier.h"
#include "../comm/MessageQueue.h"
#include "../comm/MessageModule.h"

class WorldUpdateModule : public Module {
 protected:
  /* general data */
  int t_id;
  SDL_barrier *barrier;

  MessageModule* comm;

 public:
  double avg_wui;			// average_world_update_interval
  double avg_rui;			// average_regular_update_interval
  double avg_num_req_recvd; //moving average of number a request
  double avg_time_proc_req; //moving average of time spent processing requests
  double avg_num_update_sent; //moving average of number a request
  double avg_time_send_update; //moving average of time spent processing requests
 public:
  /* Constructor and setup methods */
  WorldUpdateModule(int id, MessageModule *_comm, SDL_barrier *_barr);

  /* main loop */
  void run();

  /* message handlers */
  void handleClientJoinRequest(Player* p, IPaddress addr);
  void handleClientLeaveRequest(Player* p);

  void handle_move(Player* p, int _dir);
};

#endif
