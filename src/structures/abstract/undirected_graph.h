#ifndef UNDIRECTED_GRAPH_H
#define UNDIRECTED_GRAPH_H

/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include <list>
#include <unordered_map>
#include <vector>

#include "structures/abstract/edge.h"
#include "structures/abstract/matrix.h"

template <class T> class undirected_graph {

private:
  unsigned _size;
  // Embedding two representations for different uses depending on
  // context.
  std::vector<edge<T>> _edges;
  std::unordered_map<index_t, std::vector<index_t>> _adjacency_list;

public:
  undirected_graph();

  undirected_graph(const matrix<T>& m);

  undirected_graph(std::vector<edge<T>> edges);

  std::size_t size() const;

  std::vector<edge<T>> get_edges() const;

  std::unordered_map<index_t, std::list<index_t>> get_adjacency_list() const;
};

#endif
