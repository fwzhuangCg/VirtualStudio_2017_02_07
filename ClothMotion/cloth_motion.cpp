#include "cloth_motion.h"
#include "simulation\simulation.h"
#include "simulation\separateobs.h"
#include "simulation\io.h"
#include "simulation\magic.h"
#include "triangulate.h"
#include <assert.h>
#include <QProgressDialog>
#include <QString>

struct Velocity
{ 
	Velocity() : v(0, 0, 0), w(0, 0, 0), o(0, 0, 0) {}
	Vec3 v, w, o; 
};

ClothHandler::ClothHandler() : frame_(0), sim_(new Simulation), fps_(new Timer), clothes_(sim_->cloths) {}

const double ClothHandler::shrinkFactor = 1.f;

void ClothHandler::init_avatars_to_handler(
	DoubleDataBuffer position, 
	DoubleDataBuffer texcoords, 
	IntDataBuffer indices,
	size_t faceNum
	)
{
	Obstacle obs;

	for(int index = 0; index != faceNum; ++index)
	{
		for(int nindex = 0; nindex != 3; ++nindex)
		{
			Vec3 x;
			x[0] = position[index * 9 + nindex * 3 ];
			x[1] = position[index * 9 + nindex * 3 + 1];
			x[2] = position[index * 9 + nindex * 3 + 2];
			obs.base_mesh.add(new Node(x, Vec3(0)));

			Vec2 u;
			u[0] = texcoords[index * 6 + nindex * 2];
			u[0] = texcoords[index * 6 + nindex * 2 + 1];
			obs.base_mesh.add(new Vert(u));
		}	
	}

	for(int index = 0; index != faceNum; ++index)
	{
		std::vector<Vert*> verts;
		std::vector<Node*> nodes;
		for(int nindex = 0; nindex < 3; ++nindex)
		{
			nodes.push_back(obs.base_mesh.nodes[indices[index * 3 + nindex]]);
			verts.push_back(obs.base_mesh.verts[indices[index * 3 + nindex]]);
		}

		for (int v = 0; v < verts.size(); v++)
			connectvn(verts[v], nodes[v]);
		std::vector<Face*> faces = triangulate(verts);
		for (int f = 0; f < faces.size(); f++)
			obs.base_mesh.add(faces[f]);
	}

	mark_nodes_to_preserve(obs.base_mesh);
	compute_ms_data(obs.base_mesh);

	obs.transform_spline = NULL;
	obs.start_time = 0.f;
	obs.end_time = infinity;
	obs.get_mesh(0);
	sim_->obstacles.push_back(obs);
}

void ClothHandler::update_avatars_to_handler(DoubleDataBuffer position)
{
	Mesh & mesh = sim_->obstacles[0].curr_state_mesh;

	for(size_t index = 0; index != mesh.nodes.size(); ++index)
	{
		Vec3 x;
		x[0] = position[index * 3 ];
		x[1] = position[index * 3 + 1];
		x[2] = position[index * 3 + 2];
		Node * node = mesh.nodes[index];
		node->x = x;
		node->v = (node->x - node->x0) / sim_->step_time; 
	}

	mark_nodes_to_preserve(mesh);
	compute_ms_data(mesh);
}

// used to import obj cloth file
void ClothHandler::add_clothes_to_handler(const char * filename)
{
	SmtClothPtr cloth(new SimCloth);
	load_obj(cloth->mesh, filename);

	std::fstream fs("parameters/parameter.txt");
	assert(fs.is_open());

	std::string tab;
	double v1, v2, v3, v4;

	// init transformation
	Transformation transform;
	fs >> tab >> v1 >> v2 >> v3;
	transform.translation = Vec3(v1, v2, v3);
	fs >> tab >> v1;
	transform.scale = v1;
	Vec<4> rot(0); 
	fs >> tab >> v1 >> v2 >> v3 >> v4;
	rot = Vec<4>(v1, v2, v3, v4); 
	transform.rotation = Quaternion::from_axisangle(Vec3(rot[1], rot[2], rot[3]), rot[0]*M_PI/180);
	if (transform.scale != 1.0f)
		for (int v = 0; v < cloth->mesh.verts.size(); v++)
			cloth->mesh.verts[v]->u *= transform.scale;
	compute_ms_data(cloth->mesh);
	apply_transformation(cloth->mesh, transform);

	// init velocity
	Velocity velocity;
	apply_velocity(cloth->mesh, velocity);

	// init material
	std::string mat_file;
	std::tr1::shared_ptr<SimCloth::Material> material(new SimCloth::Material);
	fs >> tab >> mat_file;
	mat_file = "material/" + mat_file + ".txt";
	std::fstream matfs(mat_file);
	assert(matfs.is_open());

	matfs >> tab >> material->density;

	// init stretching data
	StretchingData data;
	matfs >> tab;
	matfs >> tab >> v1 >> v2 >> v3 >> v4;
	data.d[0][0] = Vec4(v1, v2, v3, v4);
	for (int i = 1; i < 5; i++)
		data.d[0][i] = data.d[0][0];
	for (int i = 0; i < 5; i++)
	{
		matfs >> tab >> v1 >> v2 >> v3 >> v4;
		data.d[1][i] = Vec4(v1, v2, v3, v4);
	}
	evaluate_stretching_samples(material->stretching, data);

	// init bending data
	matfs >> tab;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			matfs >> material->bending.d[i][j];
		}
	}

	matfs.close();

	// init mult factor
	double density_mult, stretching_mult, bending_mult, thicken;
	fs >> tab >> v1;
	density_mult = v1;
	fs >> tab >> v1;
	stretching_mult = v1;
	fs >> tab >> v1;
	bending_mult = v1;
	fs >> tab >> v1;
	thicken = v1;
	density_mult *= thicken;
	stretching_mult *= thicken;
	bending_mult *= thicken;
	material->density *= density_mult;
	for (int i = 0; i < sizeof(material->stretching.s)/sizeof(Vec4); i++)
		((Vec4*)&material->stretching.s)[i] *= stretching_mult;
	for (int i = 0; i < sizeof(material->bending.d)/sizeof(double); i++)
		((double*)&material->bending.d)[i] *= bending_mult;
	// init others

	fs >> tab >> material->damping;
	fs >> tab >> material->strain_min;
	fs >> tab >> material->strain_max;
	material->yield_curv = infinity;
	fs >> tab >> material->weakening;
	cloth->materials.push_back(material);

	// init remeshing parameter
	fs >> tab >> cloth->remeshing.refine_angle;
	fs >> tab >> cloth->remeshing.refine_compression;
	fs >> tab >> cloth->remeshing.refine_velocity;
	fs >> tab >> cloth->remeshing.size_min;
	fs >> tab >> cloth->remeshing.size_max;
	fs >> tab >> cloth->remeshing.aspect_min;

	fs.close();
	// save to simulation
	sim_->cloths.push_back(cloth);
}

void ClothHandler::transform_cloth(const float * transform, size_t clothIndex)
{
	if(sim_->cloths.empty())
		return;
	SmtClothPtr & cloth = sim_->cloths[clothIndex];

	Transformation trans;
	trans.translation = Vec3(transform[0], transform[1], transform[2]);// edit
	trans.scale = transform[3]; // edit temp_modify
	trans.rotation.v = Vec3(transform[4], transform[5], transform[6]);
	trans.rotation.s = transform[7];
	if (abs(trans.scale - 1.0f) > 0.0001f)
		for (int v = 0; v < cloth->mesh.verts.size(); v++)
			cloth->mesh.verts[v]->u *= trans.scale;

	apply_transformation(cloth->mesh, trans);
	compute_ms_data(cloth->mesh);
}

void ClothHandler::update_buffer()
{
	position_buffer_.clear();
	normal_buffer_.clear();
	texcoord_buffer_.clear();
	Mesh & mesh = sim_->cloths[0]->mesh;
	for(auto face_it = mesh.faces.begin(); face_it != mesh.faces.end(); ++face_it)
	{
		Face * face = *face_it;
		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < 3; ++j)
			{
				position_buffer_.push_back(static_cast<float>(face->v[i]->node->x[j]));	
				normal_buffer_.push_back(static_cast<float>(face->v[i]->node->n[j]));
			}
			for(int j = 0; j < 2; ++j)
				texcoord_buffer_.push_back(static_cast<float>(face->v[i]->u[j]));
		}
	}
}

/*void ClothHandler::add_clothes_to_handler(
DoubleDataBuffer position, 
DoubleDataBuffer texcoords, 
IntDataBuffer indices,
size_t faceNum)
{}*/

void ClothHandler::init_simulation()
{
	if(sim_->cloths.empty())
		return;

	std::fstream fs("parameters/simulation_parameter.txt");
	assert(fs.is_open());

	// init time frame
	std::string label;
	fs >> label >> sim_->frame_time;
	fs >> label >> sim_->frame_steps;
	sim_->step_time = sim_->frame_time / sim_->frame_steps;
	sim_->time = 0.0f;

	// init gravity
	double v1, v2, v3;
	fs >> label >> v1 >> v2 >> v3;
	sim_->gravity = Vec3(v1, v2, v3);
	 
	// init wind
	fs >> label;
	fs >> label >> sim_->wind.density;
	fs >> label >> v1 >> v2 >> v3;
	sim_->wind.velocity = Vec3(v1, v2, v3);
	fs >> label >> sim_->wind.drag;

	// init friction
	fs >> label >> sim_->friction;
	fs >> label >> sim_->obs_friction;

	// init magic
	fs >> label >> magic.repulsion_thickness;
	fs >> label >> magic.collision_stiffness;

	// functional ability
	bool has_strain_limits = false, has_plasticity = false;
	for (int c = 0; c < sim_->cloths.size(); c++)
	{
		for (int m = 0; m < sim_->cloths[c]->materials.size(); m++) 
		{
			std::tr1::shared_ptr<SimCloth::Material> mat = sim_->cloths[c]->materials[m];
			if (mat->strain_min != infinity || mat->strain_max != infinity)
				has_strain_limits = true;
			if (mat->yield_curv != infinity)
				has_plasticity = true;
		}
	}
	sim_->enabled[Simulation::Proximity] = true;
	sim_->enabled[Simulation::Physics] = true;
	sim_->enabled[Simulation::Collision] = true;
	sim_->enabled[Simulation::Remeshing] = true;
	sim_->enabled[Simulation::Separation] = true;
	sim_->enabled[Simulation::PopFilter] = true;
	if (!has_strain_limits)
		sim_->enabled[Simulation::StrainLimiting] = false;
	if (!has_plasticity)
		sim_->enabled[Simulation::Plasticity] = false;

	fs.close();
}

bool ClothHandler::begin_simulate()
{
	if(clothes_frame_.size())
	{
		for(size_t i = 0; i < clothes_frame_.size(); ++i)
		{
			clothes_frame_[i].clear();
		}
		clothes_frame_.clear();
	}
	clothes_frame_.resize(clothes_.size());
	init_simulation();
	prepare(*sim_);
	return relax_initial_state(*sim_);
}

bool ClothHandler::sim_next_step()
{
	fps_->tick();
	if(!advance_step(*sim_))
		return false;
	//separate_obstacles(sim_->obstacle_meshes, sim_->cloth_meshes);
	fps_->tock();
	return true;
	// end condition
	/*if (sim_->time >= sim_->end_time || sim_->frame >= sim_->end_frame)
	exit(EXIT_SUCCESS);*/
}

bool ClothHandler::load_cmfile_to_replay(const char * fileName)
{
	/*std::ifstream ifs(fileName);
	std::string tag;
	int cloth, frame, total_frame;

	clothes_frame_.clear();

	ifs >> tag >> cloth;
	if(tag.compare("cloth") != 0)
		return false;

	ifs >> tag >> total_frame;
	if(tag.compare("total_frame") != 0)
		return false;

	QProgressDialog process(NULL);  
	process.setLabelText(QString("loading..."));  
	process.setRange(0, total_frame);  
	process.setModal(true);  
	process.setCancelButtonText(QString("cancel"));

	while(ifs >> tag >> frame)
	{
		process.setValue(frame);
		if(process.wasCanceled())  
			break;
		if(tag.compare("frame") != 0)
			return false;
		for(int cloth_index = 0; cloth_index < cloth; ++cloth_index)
		{
			SmtClothPtr cloth(new SimCloth);
			Mesh & mesh = cloth->mesh;
			int vertexNum = 0;
			ifs >> tag >> vertexNum;
			if(tag.compare("vertex") != 0)
				return false;
			int faceNum = vertexNum / 3;
			for(int face_index = 0; face_index < faceNum; ++face_index)
			{
				Vert * vert[3];
				for(int face_vert_index = 0; face_vert_index < 3; ++face_vert_index)
				{
					double x, y, z;
					ifs >> x >> y >> z;
					vert[face_vert_index] = new Vert;
					Node * node = new Node(Vec3(x, y, z));
					vert[face_vert_index]->node = node;
					mesh.verts.push_back(vert[face_vert_index]);
					mesh.nodes.push_back(node);
				}
				Face * face = new Face(vert[0], vert[1], vert[2]);
				vert[0]->adjf.push_back(face);
				vert[1]->adjf.push_back(face);
				vert[2]->adjf.push_back(face);
				mesh.faces.push_back(face);
			}	
			compute_ws_data(cloth->mesh);
			clothes_frame_.push_back(cloth);
		}
	}
	sim_->cloths.resize(1);*/
	return true;
}

void ClothHandler::init_cmfile(const char * fileName, int totalFrame)
{
	// abandonded

	/*clothMotionFile_.open(fileName);
	clothMotionFile_ << "cloth " << clothes_.size() << std::endl;
	clothMotionFile_ << "total_frame " << totalFrame << std::endl;*/
}

void ClothHandler::write_frame(int frame)
{
	for(size_t i = 0; i < clothes_.size(); i++)
	{
		SmtClothPtr copy_cloth(new SimCloth);
		copy_cloth->mesh = deep_copy(clothes_[i]->mesh);
		clothes_frame_[i].push_back(copy_cloth);
	}
}

void ClothHandler::save_cmfile()
{
	// abandonded

	/*if(clothMotionFile_.is_open())
	{
		clothMotionFile_.close();
	}*/
}

void ClothHandler::apply_velocity(Mesh &mesh, const Velocity &vel)
{
	for (int n = 0; n < mesh.nodes.size(); n++)
		mesh.nodes[n]->v = vel.v + cross(vel.w, mesh.nodes[n]->x - vel.o);
}

size_t ClothHandler::face_count() { return sim_->cloths[0]->mesh.faces.size(); }

size_t ClothHandler::cloth_num() { return clothes_.size(); }

void ClothHandler::load_frame(int frame)
{
	if(frame > static_cast<int>(clothes_frame_[0].size()) - 1)
		frame = static_cast<int>(clothes_frame_[0].size()) - 1;
	for(size_t i = 0; i < sim_->cloths.size(); ++i)
		clothes_[i]->mesh = clothes_frame_[i][frame]->mesh;
}

void ClothHandler::init_cloth(SimCloth &cloth)
{
	std::fstream fs("parameters/parameter.txt");
	assert(fs.is_open());

	std::string tab;
	double v1, v2, v3, v4;

	// init transformation
	Transformation transform;
	fs >> tab >> v1 >> v2 >> v3;
	transform.translation = Vec3(v1, v2, v3);
	fs >> tab >> v1;
	transform.scale = v1;
	Vec<4> rot(0); 
	fs >> tab >> v1 >> v2 >> v3 >> v4;
	rot = Vec<4>(v1, v2, v3, v4); 
	transform.rotation = Quaternion::from_axisangle(Vec3(rot[1], rot[2], rot[3]), rot[0]*M_PI/180);
	if (transform.scale != 1.0f)
		for (int v = 0; v < cloth.mesh.verts.size(); v++)
			cloth.mesh.verts[v]->u *= transform.scale;
	compute_ms_data(cloth.mesh);
	apply_transformation(cloth.mesh, transform);

	// init velocity
	Velocity velocity;
	apply_velocity(cloth.mesh, velocity);

	// init material
	std::string mat_file;
	std::tr1::shared_ptr<SimCloth::Material> material(new SimCloth::Material);
	fs >> tab >> mat_file;
	mat_file = "material/" + mat_file + ".txt";
	std::fstream matfs(mat_file);
	assert(matfs.is_open());

	matfs >> tab >> material->density;

	// init stretching data
	StretchingData data;
	matfs >> tab;
	matfs >> tab >> v1 >> v2 >> v3 >> v4;
	data.d[0][0] = Vec4(v1, v2, v3, v4);
	for (int i = 1; i < 5; i++)
		data.d[0][i] = data.d[0][0];
	for (int i = 0; i < 5; i++)
	{
		matfs >> tab >> v1 >> v2 >> v3 >> v4;
		data.d[1][i] = Vec4(v1, v2, v3, v4);
	}
	evaluate_stretching_samples(material->stretching, data);

	// init bending data
	matfs >> tab;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			matfs >> material->bending.d[i][j];
		}
	}

	matfs.close();

	// init mult factor
	double density_mult, stretching_mult, bending_mult, thicken;
	fs >> tab >> v1;
	density_mult = v1;
	fs >> tab >> v1;
	stretching_mult = v1;
	fs >> tab >> v1;
	bending_mult = v1;
	fs >> tab >> v1;
	thicken = v1;
	density_mult *= thicken;
	stretching_mult *= thicken;
	bending_mult *= thicken;
	material->density *= density_mult;
	for (int i = 0; i < sizeof(material->stretching.s)/sizeof(Vec4); i++)
		((Vec4*)&material->stretching.s)[i] *= stretching_mult;
	for (int i = 0; i < sizeof(material->bending.d)/sizeof(double); i++)
		((double*)&material->bending.d)[i] *= bending_mult;
	// init others

	fs >> tab >> material->damping;
	fs >> tab >> material->strain_min;
	fs >> tab >> material->strain_max;
	material->yield_curv = infinity;
	fs >> tab >> material->weakening;
	cloth.materials.push_back(material);

	// init remeshing parameter
	fs >> tab >> cloth.remeshing.refine_angle;
	fs >> tab >> cloth.remeshing.refine_compression;
	fs >> tab >> cloth.remeshing.refine_velocity;
	fs >> tab >> cloth.remeshing.size_min;
	fs >> tab >> cloth.remeshing.size_max;
	fs >> tab >> cloth.remeshing.aspect_min;

	fs.close();
}

SmtClothPtr ClothHandler::load_cloth_from_obj(const char * filename)
{
	SmtClothPtr cloth(new SimCloth);
	load_obj(cloth->mesh, filename);

	init_cloth(*cloth);
	return cloth;
}

SmtClothPtr ClothHandler::load_cloth_from_contour(const QPainterPath &path)
{
	SmtClothPtr cloth(new SimCloth);
	QPolygonF polygon = path.toFillPolygon();
	Vector2dVector contour, zdmesh;
	for(int i = 0; i < polygon.size() - 1; ++i) {
		QPointF p = polygon[i];
		contour.push_back(Vector2d(p.x(), p.y()));
	}
	Triangulate::Process(contour, zdmesh);
	generate_obj(cloth->mesh, zdmesh);
	init_cloth(*cloth);
	return cloth;
}