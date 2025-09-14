#ifndef UDTSIMULATE_H
#define UDTSIMULATE_H

#include "packet.h"


int packetCorrupted(Packet *pkt, float corruptionProbability);


int packetLost(float lossProbability);

#endif 