#ifndef SOLOMON_H
#define SOLOMON_H

/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "structures/vroom/input/input.h"
#include "structures/vroom/tw_route.h"

using tw_solution = std::vector<tw_route>;

// Initialization types.
enum class INIT_T { NONE, HIGHER_AMOUNT, EARLIEST_DEADLINE, FURTHEST };

// Implementation of a variant of the Solomon I1 heuristic.
tw_solution basic_heuristic(const input& input, INIT_T init, float lambda);

// Adjusting the above for situation with heterogeneous fleet.
tw_solution dynamic_vehicle_choice_heuristic(const input& input,
                                             INIT_T init,
                                             float lambda);

#endif
