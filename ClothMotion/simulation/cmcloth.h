/*
  Copyright ©2013 The Regents of the University of California
  (Regents). All Rights Reserved. Permission to use, copy, modify, and
  distribute this software and its documentation for educational,
  research, and not-for-profit purposes, without fee and without a
  signed licensing agreement, is hereby granted, provided that the
  above copyright notice, this paragraph and the following two
  paragraphs appear in all copies, modifications, and
  distributions. Contact The Office of Technology Licensing, UC
  Berkeley, 2150 Shattuck Avenue, Suite 510, Berkeley, CA 94720-1620,
  (510) 643-7201, for commercial licensing opportunities.

  IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT,
  INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
  LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
  DOCUMENTATION, EVEN IF REGENTS HAS BEEN ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

  REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING
  DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS PROVIDED "AS
  IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
  UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/

#ifndef CMCLOTH_H
#define CMCLOTH_H

//#include "dde.hpp"
#include "mesh.h"
#include <memory>

typedef Vec<4> Vec4;

struct StretchingData {Vec4 d[2][5];};

struct StretchingSamples {Vec4 s[40][40][40];};

struct BendingData {double d[3][5];};

struct SimCloth {
	Mesh mesh;
	struct Material {
		double density; // area density
		StretchingSamples stretching;
		BendingData bending;
		double damping; // stiffness-proportional damping coefficient
		double strain_min, strain_max; // strain limits
		double yield_curv, weakening; // plasticity parameters
	};
	std::vector<std::tr1::shared_ptr<Material> > materials;
	struct Remeshing {
		double refine_angle, refine_compression, refine_velocity;
		double size_min, size_max; // size limits
		double aspect_min; // aspect ratio control
	} remeshing;
};

void compute_masses (SimCloth &cloth);

Vec4 evaluate_stretching_sample (const Mat2x2 &G, const StretchingData &data);

void evaluate_stretching_samples (StretchingSamples &samples,
	const StretchingData &data);

Vec4 stretching_stiffness (const Mat2x2 &G, const StretchingSamples &samples);

double bending_stiffness (const Edge *edge, int side, const BendingData &data,
	double initial_angle=0);

#endif
