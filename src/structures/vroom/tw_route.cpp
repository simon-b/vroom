/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "structures/vroom/tw_route.h"

tw_route::tw_route(const input& input, index_t i)
  : _input(input),
    m(_input.get_matrix()),
    v(_input._vehicles[i]),
    has_start(_input._vehicles[i].has_start()),
    has_end(_input._vehicles[i].has_end()),
    v_start(_input._vehicles[i].tw.start),
    v_end(_input._vehicles[i].tw.end) {
}

inline bool is_margin_ok(const std::pair<duration_t, duration_t>& margin,
                         const std::vector<time_window_t>& tws) {
  auto overlap_candidate =
    std::find_if(tws.begin(), tws.end(), [&](const auto& tw) {
      return margin.first <= tw.end;
    });

  // The situation where there is no TW candidate should have been
  // previously filtered in is_valid_addition_for_tw.
  assert(overlap_candidate != tws.end());
  return overlap_candidate->start <= margin.second;
}

duration_t tw_route::new_earliest(index_t job_rank, index_t rank) {
  const auto& j = _input._jobs[job_rank];

  duration_t previous_earliest = v_start;
  duration_t previous_service = 0;
  duration_t previous_travel = 0;
  if (rank > 0) {
    const auto& previous_job = _input._jobs[route[rank - 1]];
    previous_earliest = earliest[rank - 1];
    previous_service = previous_job.service;
    previous_travel = m[previous_job.index()][j.index()];
  } else {
    if (has_start) {
      previous_travel = m[v.start.get().index()][j.index()];
    }
  }

  return previous_earliest + previous_service + previous_travel;
}

duration_t tw_route::new_latest(index_t job_rank, index_t rank) {
  const auto& j = _input._jobs[job_rank];

  duration_t next_latest = v_end;
  duration_t next_travel = 0;
  if (rank == route.size()) {
    if (has_end) {
      next_travel = m[j.index()][v.end.get().index()];
    }
  } else {
    next_latest = latest[rank];
    next_travel = m[j.index()][_input._jobs[route[rank]].index()];
  }

  assert(j.service + next_travel <= next_latest);
  return next_latest - j.service - next_travel;
}

bool tw_route::is_valid_addition_for_tw(const index_t job_rank,
                                        const index_t rank) {
  const auto& j = _input._jobs[job_rank];

  duration_t job_earliest = new_earliest(job_rank, rank);

  if (j.tws.back().end < job_earliest) {
    // Early abort if we're after the latest deadline for current job.
    return false;
  }

  duration_t next_latest = v_end;
  duration_t next_travel = 0;
  if (rank == route.size()) {
    if (has_end) {
      next_travel = m[j.index()][v.end.get().index()];
    }
  } else {
    next_latest = latest[rank];
    next_travel = m[j.index()][_input._jobs[route[rank]].index()];
  }

  bool valid = job_earliest + j.service + next_travel <= next_latest;

  if (valid) {
    duration_t new_latest = next_latest - j.service - next_travel;
    auto margin = std::make_pair(job_earliest, new_latest);
    valid &= is_margin_ok(margin, j.tws);
  }

  return valid;
}

void tw_route::add(const index_t job_rank, const index_t rank) {
  assert(rank <= route.size());

  // Among all job TW compatible with current constraint margin, pick
  // the one that yields the biggest final margin.
  duration_t job_earliest = new_earliest(job_rank, rank);
  duration_t job_latest = new_latest(job_rank, rank);

  const auto& tws = _input._jobs[job_rank].tws;
  auto candidate = std::find_if(tws.begin(), tws.end(), [&](const auto& tw) {
    return job_earliest <= tw.end;
  });
  assert(candidate != tws.end());
  auto best_candidate = candidate;
  duration_t best_margin = 0;

  for (; candidate != tws.end(); ++candidate) {
    if (candidate->start > job_latest) {
      break;
    }
    duration_t earliest_candidate = std::max(job_earliest, candidate->start);
    duration_t latest_candidate = std::min(job_latest, candidate->end);
    assert(earliest_candidate <= latest_candidate);
    duration_t margin = latest_candidate - earliest_candidate;
    if (margin > best_margin) {
      best_candidate = candidate;
      best_margin = margin;
    }
  }

  job_earliest = std::max(job_earliest, best_candidate->start);
  job_latest = std::min(job_latest, best_candidate->end);

  tw_ranks.insert(tw_ranks.begin() + rank,
                  std::distance(tws.begin(), best_candidate));

  // Needs to be done after TW stuff as new_[earliest|latest] rely on
  // route size before addition ; but before earliest/latest date
  // propagation that rely on route structure after addition.
  route.insert(route.begin() + rank, job_rank);

  // Update earliest/latest date for new job, then propagate
  // constraints.
  earliest.insert(earliest.begin() + rank, job_earliest);
  latest.insert(latest.begin() + rank, job_latest);

  duration_t previous_earliest = job_earliest;
  for (index_t i = rank + 1; i < route.size(); ++i) {
    const auto& previous_j = _input._jobs[route[i - 1]];
    const auto& next_j = _input._jobs[route[i]];
    duration_t next_earliest = previous_earliest + previous_j.service +
                               m[previous_j.index()][next_j.index()];

    if (next_earliest <= earliest[i]) {
      break;
    } else {
      assert(next_earliest <= latest[i]);
      earliest[i] = next_earliest;
      previous_earliest = next_earliest;
    }
  }

  // Update latest date for new (and all precedent) jobs.
  duration_t next_latest = job_latest;
  for (index_t next_i = rank; next_i > 0; --next_i) {
    const auto& previous_j = _input._jobs[route[next_i - 1]];
    const auto& next_j = _input._jobs[route[next_i]];

    duration_t gap = previous_j.service + m[previous_j.index()][next_j.index()];
    assert(gap <= next_latest);
    duration_t previous_latest = next_latest - gap;

    if (latest[next_i - 1] <= previous_latest) {
      break;
    } else {
      assert(earliest[next_i - 1] <= previous_latest);
      latest[next_i - 1] = previous_latest;
      next_latest = previous_latest;
    }
  }
}
