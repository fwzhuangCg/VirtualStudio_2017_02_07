#ifndef CLOTH_MOTION_H
#define CLOTH_MOTION_H

#include <memory>
#include <vector>
#include <fstream>
#include "simulation\simcloth.h"

struct Simulation;
struct Mesh;
struct SimCloth;
struct Timer;
struct Velocity;

typedef std::tr1::shared_ptr<SimCloth> SmtClothPtr;

class QPainterPath;

class ClothHandler
{
public:
	typedef double * const DoubleDataBuffer;
	typedef int * const IntDataBuffer;

	ClothHandler();

	void init_avatars_to_handler(
		DoubleDataBuffer position, 
		DoubleDataBuffer texcoords, 
		IntDataBuffer indices,
		size_t faceNum
		);
	/*void add_clothes_to_handler(
	DoubleDataBuffer position, 
	DoubleDataBuffer texcoords, 
	IntDataBuffer indices,
	size_t faceNum
	);*/
	void update_avatars_to_handler(DoubleDataBuffer position);

	// Temporary used to import obj cloth file
	void add_clothes_to_handler(const char * filename);
	void add_clothes_to_handler(SmtClothPtr cloth) {clothes_.push_back(cloth);}
	void update_buffer();
	std::vector<float> get_position() { return position_buffer_; }
	std::vector<float> get_normal() { return normal_buffer_; }
	std::vector<float> get_texcoord() { return texcoord_buffer_; }

	bool begin_simulate();
	bool sim_next_step();
	bool load_cmfile_to_replay(const char * fileName);
	void load_frame(int frame);
	void transform_cloth(const float * transform, size_t clothIndex);

	void init_cmfile(const char * fileName, int totalFrame);
	void write_frame(int frame);
	void save_cmfile();

	size_t face_count();
	size_t cloth_num();

	static SmtClothPtr load_cloth_from_obj(const char * filename);
	static SmtClothPtr load_cloth_from_contour(const QPainterPath &path);

private:
	void init_simulation();
	static void init_cloth(SimCloth &cloth);
	static void apply_velocity(Mesh &mesh, const Velocity &vel);

	std::tr1::shared_ptr<Simulation> sim_;
	int frame_;
	std::tr1::shared_ptr<Timer> fps_;
	std::vector<SmtClothPtr> & clothes_;
	std::vector<std::vector<SmtClothPtr > > clothes_frame_;
	std::vector<float> position_buffer_;
	std::vector<float> normal_buffer_;
	std::vector<float> texcoord_buffer_;
	std::ofstream clothMotionFile_;

	static const double shrinkFactor;
};

#endif