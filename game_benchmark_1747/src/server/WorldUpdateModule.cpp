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

#include <iostream>
#include "ServerData.h"
#ifndef TRACEPOINT_DEFINE
#define TRACEPOINT_DEFINE
#include "../tracing/trace.h"
#endif
#include "WorldUpdateModule.h"

/***************************************************************************************************
 *
 * Constructors and setup methods
 *
 ***************************************************************************************************/

WorldUpdateModule::WorldUpdateModule(int id, MessageModule *_comm,
SDL_barrier *_barr) {
  assert(id >= 0 && id < sd->num_threads && _comm && _barr);
  t_id = id;
  barrier = _barr;

  comm = _comm;

  avg_wui = -1;
  avg_rui = -1;
  avg_num_req_recvd = -1;
  avg_time_proc_req = -1;

  assert(SDL_CreateThread( module_thread, (void*)this ) != NULL);
}

/***************************************************************************************************
 *
 * Main loop
 *
 ***************************************************************************************************/

int num_iterations = 0;
int last_quest_tid = -1;
const int max_num_iterations = 2000;
const int quest_begin = 300;
const int quest_end = 1300;

void WorldUpdateModule::run() {
  Uint32 start_time, processing_begin;
  Uint32 timeout;

  Message *m;
  IPaddress addr;
  int type;

  Player *p;
  PlayerBucket* bucket = &sd->wm.players[t_id];

  Serializator *s = NULL;
  MessageWithSerializator *ms = NULL;

  printf("WorldUpdateModule #%d started\n", t_id);

  tracepoint(trace_LB, tp_thread_begin, (int) t_id);

  /* main loop */
  while (true) {
    int num_req_recvd = 0;
    double processing_total = 0;
    start_time = SDL_GetTicks();
    timeout = sd->regular_update_interval;

    while ((m = comm->receive(timeout, t_id)) != NULL) {
      ++num_req_recvd;
      processing_begin = SDL_GetTicks();
      addr = m->getAddress();
      type = m->getType();
      p = sd->wm.findPlayer(addr, t_id);

      switch (type) {
        case MESSAGE_CS_JOIN:
          handleClientJoinRequest(p, addr);
          break;
        case MESSAGE_CS_LEAVE:
          handleClientLeaveRequest(p);
          break;

        case MESSAGE_CS_MOVE_DOWN:				// dir 0
        case MESSAGE_CS_MOVE_RIGHT:				// dir 1
        case MESSAGE_CS_MOVE_UP:				// dir 2
        case MESSAGE_CS_MOVE_LEFT:				// dir 3
          handle_move(p, type - MESSAGE_CS_MOVE_DOWN);
          break;

        case MESSAGE_CS_ATTACK_DOWN:			// dir 0
        case MESSAGE_CS_ATTACK_RIGHT:			// dir 1
        case MESSAGE_CS_ATTACK_UP:				// dir 2
        case MESSAGE_CS_ATTACK_LEFT:			// dir 3
          sd->wm.attackPlayer(p, type - MESSAGE_CS_ATTACK_DOWN);
          break;
        case MESSAGE_CS_USE:
          sd->wm.useGameObject(p);
          break;

        default:
          printf(
              "[WARNING] Unknown message (%d) received from %u.%u.%u.%u:%d\n",
              m->getType(), addr.host & 0xFF, (addr.host >> 8) & 0xFF,
              (addr.host >> 16) & 0xFF, addr.host >> 24, addr.port);
      }

      processing_total += (SDL_GetTicks() - processing_begin);
      delete m;
      timeout = sd->regular_update_interval - (SDL_GetTicks() - start_time);
      //printf("1 %d\n", processing_total);
      if (((int) timeout) < 0)
      {
        timeout = 0;
        //break;
      }
    }

    //Number of requests received per thread

    //Moving average of the time spent processing the requests per thread
    //one tick is 1 msec, so we multply by 1000 to get it in usec

    tracepoint(trace_LB, tp_first_stage, (int ) num_req_recvd,
               (int ) processing_total);

    SDL_WaitBarrier(barrier);

    if (t_id == 0) {
      sd->wm.balance();

      if (rand() % 100 < 10)
        sd->wm.regenerateObjects();

      sd->send_start_quest = 0;
      sd->send_end_quest = 0;

      if (num_iterations == quest_begin) {
        sd->quest_pos.x = (rand() % sd->wm.n_regs.x)
            * sd->wm.regmin.x + MAX_CLIENT_VIEW;
        sd->quest_pos.y = (rand() % sd->wm.n_regs.y)
            * sd->wm.regmin.y + MAX_CLIENT_VIEW;
        sd->send_start_quest = 1;
      }
      if (num_iterations >= quest_begin &&
          num_iterations < quest_end) {

        int tid = sd->wm.getRegionByLocation(sd->quest_pos)->t_id;
        if (last_quest_tid != tid) {
          tracepoint(trace_LB, tp_quest_manager, tid);
          last_quest_tid = tid;
        }

      } else if (num_iterations == quest_end) {
        sd->wm.rewardPlayers(sd->quest_pos);
        sd->send_end_quest = 1;
      }
      num_iterations++;
    }

    SDL_WaitBarrier(barrier);

    start_time = SDL_GetTicks();

    bool has_sla_violation = false;
    double num_sla_violations = 0.0;
    int num_update_sent = 0;
    int sending_time = 0;
    /* send updates to clients (map state) */
    bucket->start();
    while ((p = bucket->next()) != NULL) {
      ms = new MessageWithSerializator(MESSAGE_SC_REGULAR_UPDATE, t_id,
                                       p->address);
      assert(ms);
      s = ms->getSerializator();
      assert(s);

      sd->wm.updatePlayer(p, s);

      ms->prepare();
      comm->send(ms, t_id);
      num_update_sent++;
      if (sd->send_start_quest)
        comm->send(
            new MessageXY(MESSAGE_SC_NEW_QUEST, t_id, p->address,
                          sd->quest_pos),
            t_id);
      if (sd->send_end_quest)
        comm->send(new Message(MESSAGE_SC_QUEST_OVER, t_id, p->address), t_id);

      // Count the number of players for which there is an SLA violation, i.e.
      // the time it taks to process a request and respond exceeds the
      // regular update interval.
      if (has_sla_violation
          || (has_sla_violation = ((SDL_GetTicks() - start_time))
              > (unsigned int) sd->regular_update_interval)) {
        ++num_sla_violations;
      }
    }

    sd->num_sla_violations[t_id] = num_sla_violations;

    //Moving average of the time spent processing the requests per thread
    //one tick is 1 msec, so we multply by 1000 to get it in usec
    sending_time = SDL_GetTicks() - start_time;

    tracepoint(trace_LB, tp_third_stage, (int ) num_update_sent,
               (int ) sending_time);
    SDL_WaitBarrier(barrier);

    // Stop after 3000 iterations of the loop.
    if (t_id == 0 && num_iterations >= max_num_iterations) {
      exit(EXIT_SUCCESS);
    }
  }
}

/***************************************************************************************************
 *
 * Handle client requests
 *
 ***************************************************************************************************/

/* generate a new player, send an ok message */
void WorldUpdateModule::handleClientJoinRequest(Player* p, IPaddress addr) {
  if (p) {
    comm->send(new Message(MESSAGE_SC_NOK_JOIN, 0, p->address), t_id);
    printf(
        "[WARNING] Player already on server '%s' (send not ok to join message)\n",
        p->name);
    return;
  }

  p = sd->wm.addPlayer(addr);

  MessageOkJoin *mok = new MessageOkJoin(t_id, p->address, p->name, p->pos,
                                         sd->wm.size);
  comm->send((Message*) mok, t_id);
  tracepoint(trace_LB, tp_player_join);
  if (sd->display_user_on_off)
    printf("New player: %s (%d,%d)\n", p->name, p->pos.x, p->pos.y);
}

/* remove client from WorldMap and send an ok_leave message */
void WorldUpdateModule::handleClientLeaveRequest(Player* p) {
  assert(p);
  sd->wm.removePlayer(p);

  Message *mok = new Message(MESSAGE_SC_OK_LEAVE, t_id, p->address);
  comm->send(mok, t_id);

  tracepoint(trace_LB, tp_player_quit);
  if (sd->display_user_on_off)
    printf("Removing player %s\n", p->name);
  delete p;
}

void WorldUpdateModule::handle_move(Player* p, int _dir) {
  assert(p);
  p->dir = _dir;
  sd->wm.movePlayer(p);
}

