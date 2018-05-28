#ifndef MORPH_H
#define MORPH_H

#include "mesh.h"

struct Morph {
    Mesh *mesh;
    std::vector<Mesh> targets;
    typedef std::vector<double> Weights;
    Spline<Weights> weights;
    Spline<double> log_stiffness;
    Vec3 pos (double t, const Vec3 &u) const;
};

void apply (const Morph &morph, double t);

#endif
