#ifndef LOCALOPT_HPP
#define LOCALOPT_HPP

#include "geometry.h"
#include "constraint.h"

template<Space s>
void local_opt(std::vector<Node*>& nodes, std::vector<Face*>& faces, std::vector<Edge*>& edges,
               const std::vector<Constraint*>& cons);

#endif
