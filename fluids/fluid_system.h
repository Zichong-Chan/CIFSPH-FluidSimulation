/*
  FLUIDS v.3 - SPH Fluid Simulator for CPU and GPU
  Copyright (C) 2012. Rama Hoetzlein, http://fluids3.com

  Fluids-ZLib license (* see part 1 below)
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. Acknowledgement of the
	 original author is required if you publish this in a paper, or use it
	 in a product. (See fluids3.com for details)
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/********************************************************************************************/
/*   PCISPH is integrated by Xiao Nie for NVIDIA��s 2013 CUDA Campus Programming Contest    */
/*                     https://github.com/Gfans/ISPH_NVIDIA_CUDA_CONTEST                    */
/*   For the PCISPH, please refer to the paper "Predictive-Corrective Incompressible SPH"   */
/********************************************************************************************/
#ifndef DEF_FLUID_SYS
#define DEF_FLUID_SYS

#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>

#include "vector.h"
#include "gl_helper.h"
#include "xml_settings.h"
#include "common_defs.h"
#include "vector_functions.h"
#include "Matrix3x3.h"
#include "FluidParticle.h"
#include "camera3d.h"


#define MAX_PARAM				60
#define GRID_UCHAR				0xFF
#define GRID_UNDEF				0xFFFFFFFF		

#define RUN_CPU_SPH				0
#define RUN_CUDA_INDEX_SPH		1	
#define RUN_CUDA_FULL_SPH		2
#define RUN_CPU_PCISPH			3
#define RUN_CUDA_INDEX_PCISPH	4 
#define RUN_CUDA_FULL_PCISPH	5

// ��������
#define PRUN_MODE				0	
#define PMAXNUM					1	
#define PEXAMPLE				2	
#define PSIMSIZE				3	  
#define PSIMSCALE				4	
#define PGRID_DENSITY			5	
#define PGRIDSIZEREALSCALE		6	
#define PVISC					7	
#define PRESTDENSITY			8	
#define PMASS					9	
#define PCOLLISIONRADIUS		10
#define PSPACINGREALWORLD		11	
#define PSMOOTHRADIUS			12
#define PGASCONSTANT			13	
#define PBOUNDARYSTIFF			14	
#define PBOUNDARYDAMP			15	
#define PACCEL_LIMIT			16  
#define PVEL_LIMIT				17	
#define PSPACINGGRAPHICSWORLD	18	
#define PGROUND_SLOPE			19	
#define PFORCE_MIN				20	
#define PFORCE_MAX				21	
#define PMAX_FRAC				22	
#define PDRAWMODE				23	
#define PDRAWSIZE				24	
#define PDRAWTEXT				26	
#define PCLR_MODE				27
#define PPOINT_GRAV_AMT			28	
#define PSTAT_OCCUPANCY			29	
#define PSTAT_GRIDCOUNT			30	
#define PSTAT_NEIGHCNT			31	
#define PSTAT_NEIGHCNTMAX		32	
#define PSTAT_SEARCHCNT			33	
#define PSTAT_SEARCHCNTMAX		34	
#define PSTAT_PMEM				35	
#define PSTAT_GMEM				36	
#define PTIME_INSERT			37	
#define PTIME_SORT				38
#define PTIME_COUNT				39
#define PTIME_PRESS				40
#define PTIME_FORCE				41
#define PTIME_ADVANCE			42
#define PTIME_RECORD			43	
#define PTIME_RENDER			44	
#define PTIME_TOGPU				45	
#define PTIME_FROMGPU			46	
#define PFORCE_FREQ				47	
#define PTIME_OTHER_FORCE		48
#define PTIME_PCI_STEP			49
#define	PDENSITYERRORFACTOR		50
#define PMINLOOPPCISPH			51
#define PMAXLOOPPCISPH			52
#define PMAXDENSITYERRORALLOWED 53  
#define PKERNELSELF				54
#define PINITIALIZEDENSITY		55


#define PGRIDVOLUMEMIN			0	
#define PGRIDVOLUMEMAX			1	
#define PBOUNDARYMIN			2	
#define PBOUNDARYMAX			3	
#define PINITPARTICLEMIN		4	
#define PINITPARTICLEMAX		5	
#define PEMIT_POS				6
#define PEMIT_ANG				7
#define PEMIT_DANG				8	
#define PEMIT_SPREAD			9
#define PEMIT_RATE				10
#define PPOINT_GRAV_POS			11	
#define PPLANE_GRAV_DIR			12	

// ��������
#define PPAUSE					0
#define PDEBUG					1	
#define PUSE_CUDA				2	
#define	PUSE_GRID				3	
#define PWRAP_X					4	
#define PWALL_BARRIER			5	
#define PLEVY_BARRIER			6	
#define PDRAIN_BARRIER			7	
#define PPLANE_GRAV_ON			11	
#define PPROFILE				12
#define PCAPTURE				13	
#define PDRAWGRIDCELLS			14  
#define PPRINTDEBUGGINGINFO		15
#define PDRAWDOMAIN				16
#define	PDRAWGRIDBOUND			17
#define PUSELOADEDSCENE			18

#define SPH						0
#define FLIP					1
#define RIGID					2
#define METHOD_IISPH			0
#define METHOD_DFSPH			1
#define METHOD_CISPH			2

#define MYPI	3.14159

const int max_num_adj_grid_cells_cpu = 27;

// ʹ��SOA ���� AOS �ṹ��Particleֻ����������������ռ���ڴ��С�ĸ������ݽṹ
struct Particle 
{															// offset - TOTAL: 120 (must be multiple of 12 = sizeof(Vector3DF) )
	Vector3DF		fpos;									// 0
	Vector3DF		fpredicted_pos;							// 12
	Vector3DF		fvel;									// 24
	Vector3DF		fvel_eval;								// 36
	Vector3DF		fcorrection_pressure_force;				// 48
	Vector3DF		fforce;									// 60
	Vector3DF		fsum_gradw;								// 72
	float			fsum_gradw_dot;							// 84
	float			fpressure;								// 88
	float			fcorrection_pressure;					// 92
	float			fdensity_reciprocal;					// 96
	float			fdensityError;							// 100
	int				fparticle_grid_cell_index;				// 104
	int				fnext_particle_index_in_the_same_cell;	// 108			
	DWORD			fclr;									// 112
	int				fpadding1;								// 116
	int				fpadding2;								// 120	����ֽ� ��Ա���룬�μ������������������ָ��C/C++���ԡ��������棩P147�� 8.1.4 ��Ա����
};


struct Pparticle {
	Vector3DF pos;
};

class ParticleSystem {
public:
	ParticleSystem ();
	~ParticleSystem();

	// ��Ⱦ����
	void		Draw ( Camera3D& cam, float rad );
	void		DrawDomain(Vector3DF& domain_min, Vector3DF& domain_max);
	void		DrawGrid ();
	void		DrawText ();
	void		DrawCell ( int gx, int gy, int gz );
	void		DrawParticle ( int p, int r1, int r2, Vector3DF clr2 );
	void		DrawParticleInfo ();
	void		DrawParticleInfo ( int p );
	void		DrawNeighbors ( int p );
	void		DrawCircle ( Vector3DF pos, float r, Vector3DF clr, Camera3D& cam );

	// ʵ�ù��ߺ���
	void		AllocateTemporalParticlesMemory ( uint num_particles );
	void		AllocateParticlesMemory ( int cnt );
	int			AddParticleToBuf ();
	int			num_points ();
	void		DeallocateTemporalParticleMemory();

	// ���ú���
	void		Setup ( bool bStart );
	void		SetupRender ();
	void		SetupKernels ();
	void		SetupDefaultParams ();				
	void		SetupExampleParams ( bool bStart );
	void		SetupSpacing ();
	void		SetupBoundaryParams();
	void		SetupSampleParticleVolumePCISPH ( const Vector3DF & minCorner, const Vector3DF & maxCorner, const float particleSpacing, const float jitter);
	void		SetupInitParticleVolume ( const Vector3DF &minCorner, const Vector3DF &maxCorner, const float particleSpacing, const float jitter );
	void		SetupAdditonalParticleVolume ( const Vector3DF &minCorner, const Vector3DF &maxCorner, const float particleSpacing, const float jitter);
	void		SetupInitParticleVolumeLoad(const Vector3DF& minVec, const Vector3DF& maxVec);
	void		SetupInitParticleVolumeFromFile(const Vector3DF& minVec, const Vector3DF& maxVec, int mode);
	void		SetupGridAllocate ( const Vector3DF &grid_volume_min, const Vector3DF &grid_volume_max, const float sim_scale, const float cell_size, const float border );
	void		ParseXML ( std::string name, int id, bool bStart );

	//�ھӲ��Һ���
	void		Search ();
	void		InsertParticlesCPU (uint num_particle);
	void		FindNeighborsGrid (uint num_particle);

	// ����
	void		Run(int w, int h);
	void		RunCPUSPH();
	void		RunCUDAIndexSPH();
	void		RunCUDAFullSPH();

	void		computeGasConstAndTimeStep(float densityVariation);
	void		ComputeOtherForceCpu();
	void        ComputeDensityErrorFactor(uint num_particles);
	void		CreatePreParticlesISPH(uint num_particles);
	void		ComputeGradWValues(uint num_particles);
	void		ComputeGradWValuesSimple(uint num_particles, uint& max_num_neighbors, uint& index);
	void		ComputeFactor(uint num_particles);
	void		ComputeFactorSimple(uint num_particles, uint& max_num_neighbors, uint index);
	void		collisionHandlingSimScale(Vector3DF& pos, Vector3DF& vel, int index);

	// ����ģ��
	void		BoundaryCollisions(Vector3DF* ipos, Vector3DF* iveleval, Vector3DF& acceleration);
	void		AdvanceStepCPU (float time_step);
	void		AdvanceStepSimpleCollision (float time_step);
	void		ExitParticleSystem ();
	void		TransferToCUDA ();
	void		TransferFromCUDA ();

	float		time_step();

	// ģ�ͼ���
	void		readInFluidParticles(const char* filename, int& num_points, Vector3DF& minVec, Vector3DF& maxVec);
	int			readInFluidParticleNum(const char* filename);

	// ����
	void		SaveResults ();
	void		DebugPrintMemory ();
	void		Record ( int param, std::string, mint::Time& start );
	int			SelectParticle ( int x, int y, int wx, int wy, Camera3D& cam );
	int		    selected ();

	// �������ݽṹ---����
	int		    GetGridCell ( int p, Vector3DI& gc );
	int		    GetGridCell ( Vector3DF& p, Vector3DI& gc );
	int		    grid_total ();	
	int			grid_adj_cnt ();		
	Vector3DI   GetCell ( int gc );
	Vector3DF   grid_res ();		
	Vector3DF   grid_min ();		
	Vector3DF   grid_max ();		
	Vector3DF   grid_delta ();

	// �������ݽṹ---�ھӱ�
	void		ClearNeighborTable ();
	void		ResetNeighbors ();
	int			neighbor_num ();
	void		ClearNeighbors ( int i );
	int			AddNeighbor();
	int			AddNeighbor( int i, int j, float d );

	// SPH����	
	void		ComputePressureGrid ();			
	void		ComputeForceGrid ();			
	void		ComputeForceGridNC ();

	void		DFSPH23(float sr);
	void		CISPH23(float sr);
	void		IISPH23(float sr);	
	void		FLIP_SPH23(int mode);					
	void		NaiveSPH23();
	void		ComputeDensityAlpha23(float sr);		// dfsph
	void		ComputeForceAdv23(float sr);			// dfsph
	void		ConstantDensitySolver23(float sr);		// dfsph	
	void		DivergenceFreeSolver23(float sr);		// dfsph
	void		ComputeVelStarSPH23(float sr);			// flip-sph
	void		ComputeVelFLIP23(float sr);				// flip-sph
	void		PredictAdvection23(float sr);			// iisph
	void		PressureSlover23(float sr);				// iisph
	void		Integration23(float sr);					// iisph

	// for rigid 
	void		ComputeDensityDelta23(float sr);
	void		TwoWayCoupleForce23(float sr);
	void		CorrectDivergenceErrorFR23(float sr);
	void		CorrectDensityErrorFR23(float sr);

	// GPUʵ�ú���
	void		AllocatePackBuf ();

	// ��ȡģ�ⷽ��
	std::string GetModeStr ();

	// ��������			
	void		SetParam (int p, float v );
	void		SetParam (int p, int v );
	float		GetParam ( int p );
	float		GetDT ();
	float		SetParam ( int p, float v, float mn, float mx );
	float		IncParam ( int p, float v, float mn, float mx );
	Vector3DF   GetVec ( int p );			
	void		SetVec ( int p, Vector3DF v );	
	void		Toggle ( int p );				
	bool		GetToggle ( int p );		
	std::string	GetSceneName ();

	// �⻬�˺���
	float kernelM4(float dist, float sr);
	float kernelM4Lut(float dist, float sr);
	float kernelPressureGrad(float dist, float sr);	
	float kernelPressureGradLut(float dist, float sr);	
	float kernelCubicSpline(float dist, float sr);
	float kernelCubicSplineLut(float dist, float sr);
	float kernelCubicSplineGrid(float dist, float sr);
	float kernelCubicSplineGridLut(float dist, float sr);

	// �߽���ײ����
	Vector3DF boxBoundaryForce(const uint i);

	void writeObj(std::string filepath, int mode);

	vector<FluidParticle> particles;

private:

	std::string	scene_name_;

	int			frame_;		
	float		time_step_;	
	float		time_step_sph_;
	float		time_step_wcsph_;
	float		time_step_pcisph_;
	float		time_;	

	// ģ�����
	float		param_  [ MAX_PARAM ];							
	Vector3DF	vec_	[ MAX_PARAM ];
	bool		toggle_ [ MAX_PARAM ];

	// SPH�⻬�˺���ϵ��
	float		poly6_kern_;
	float		lap_kern_;
	float		spiky_kern_;		

	int			num_points_;
	int			max_points_;
	int			good_points_;

	Vector3DF*	pos_;	
	Vector3DF*	detapos_;					// iisph -> sumdijpj
	Vector3DF*	predictedPosition_;								
	Vector3DF*	vel_;
	Vector3DF*	vel_eval_;
	Vector3DF*	correction_pressure_force_;
	Vector3DF*	force_;
	Vector3DF*  sumGradW_;
	Vector3DF*  labetalv;					// iisph -> dii
	float*		sumGradWDot_;
	float*		pressure_;
	float*		correction_pressure_;
	float*		density_;
	float*		labata_;
	float*		factor_alpha_;
	float*		kappa_;
	float*		Drho_div_Dt_;
	float*		beiyong1;					// iisph -> aii
	float*		beiyong2;					// iisph -> rho_adv
	float*		predicted_density_;			// iisph -> pressure_loop
	float*      densityError_;
	float*		max_predicted_density_array_;
	uint*		particle_grid_cell_index_;
	uint*		next_particle_index_in_the_same_cell_;	
	uint*		index_;		
	DWORD*		clr_;
	int*		ptype_;
	Vector4DF*	pcolor_;
	bool*       neighbor_has_unwant_;
	Vector3DF*  pos_pre_;
	Vector3DF*	strain_;
	mint::Time start;

	// for rigid particle
	float*		delta_;
	int			fstart, fend;	//	indexs of fluid particles
	int			rstart, rend;	//  indexs of rigid particles
	float		ballradius;
	int ADDITIONSCENE;	// 1->bunny, 2->aneurysm

	// for aneurysm
	float x0, y0, z0;
	float R, R0;
	float dm, dl, ds;
	Vector3DF Nl, Ns, Nm;
	int particleperlayer;
	int num_out_, num_out_sph_, num_out_flip_;
	int* mark_list_;
	int* mark_sph_list_;
	int* mark_flip_list_;

	float			KE, KC, AE, AC, aWALL, rWALL, massWall, LWALL;	// for wall
	float			densityCSF, cCSF;								// for CSF

	// for elastic test
	Vector3DF pcenter;
	Vector3DF limit_center;
	bool expand;
	float wall_viscosity;
	float		pradius;
	float		limit_radius;
	float		out_force_accerlation;
	Vector3DF* lastp;


	uint*		cluster_cell_;
	ushort*		age_;
	uint*		neighbor_index_;
	uint*		neighbor_particle_numbers_;

	// �������ݽṹ---������ر���
	uint*		grid_head_cell_particle_index_array_;			
	uint*		grid_particles_number_;
	int			grid_total_;									
	Vector3DI	grid_res_;										
	Vector3DF	grid_min_;										
	Vector3DF	grid_max_;		
	Vector3DF	grid_size_;										
	Vector3DF	grid_delta_;									
	int			grid_search_;
	int			grid_adj_cnt_;
	int			grid_neighbor_cell_index_offset_[max_num_adj_grid_cells_cpu];

	// �������ݽṹ---�ھӱ���ر���
	int			neighbor_particles_num_;
	int			neighbor_particles_max_num_;
	int*		neighbor_table_;
	float*		neighbor_dist_;									

	char*		pack_fluid_particle_buf_;						
	int*		pack_grid_buf_;									

	int			vbo_[3];	
	int			texture_[1];				

	int			sphere_points_;

	int			selected_;

	Image		image_;

	float maxPredictedDensity;

	uint*		save_neighbor_index_;
	uint*		save_neighbor_cnt_;
	uint*		save_neighbors_;

	// XML �����ļ�
	XmlSettings	xml;

	static int const lutSize = 100000;
	float lutKernelM4[lutSize];
	float lutKernelPressureGrad[lutSize];
	float lutKernelCubicSpline[lutSize];
	float lutKernelCubicSplineGrad[lutSize];

	// �߽���ײ������ر���
	bool addBoundaryForce;
	float maxBoundaryForce;
	float boundaryForceFactor;
	float forceDistance;

	ofstream outfileParticles;
	ifstream infileParticles;

	int factor;

};	

inline void		
	ParticleSystem::DrawParticleInfo ()		
{ 
	DrawParticleInfo ( selected_ ); 
}

inline int		
	ParticleSystem::num_points ()
{
	return num_points_; 
}

inline float		
	ParticleSystem::time_step ()
{
	return time_step_; 
}

inline int		
	ParticleSystem::selected ()
{
	return selected_;
}

inline int		
	ParticleSystem::grid_total ()
{ 
	return grid_total_; 
}

inline int		
	ParticleSystem::grid_adj_cnt ()
{ 
	return grid_adj_cnt_; 
}

inline Vector3DF		
	ParticleSystem::grid_res ()
{ 
	return grid_res_;
}

inline Vector3DF		
	ParticleSystem::grid_min ()
{ 
	return grid_min_;
}

inline Vector3DF		
	ParticleSystem::grid_max ()
{ 
	return grid_max_;
}

inline Vector3DF		
	ParticleSystem::grid_delta ()
{ 
	return grid_delta_;
}


inline int		
	ParticleSystem::neighbor_num ()
{ 
	return neighbor_particles_num_;
}

inline void		
	ParticleSystem::SetParam (int p, float v )
{ 
	param_[p] = v;
}

inline void		
	ParticleSystem::SetParam (int p, int v )
{ 
	param_[p] = (float) v;
}

inline float		
	ParticleSystem::GetParam ( int p )
{ 
	return (float) param_[p];
}

inline float
	ParticleSystem::GetDT()
{
	return time_step_;
}

inline float		
	ParticleSystem::SetParam ( int p, float v, float mn, float mx )
{ 
	param_[p] = v ; 
	if ( param_[p] > mx ) 
		param_[p] = mn; 

	return param_[p];
}

inline float		
	ParticleSystem::IncParam ( int p, float v, float mn, float mx )
{ 
	param_[p] += v; 
	if ( param_[p] < mn ) param_[p] = mn; 
	if ( param_[p] > mx ) param_[p] = mn; 
	return param_[p];
}

inline Vector3DF		
	ParticleSystem::GetVec ( int p )	
{ 
	return vec_[p];
}

inline void		
	ParticleSystem::SetVec ( int p, Vector3DF v )	
{ 
	vec_[p] = v;
}

inline void		
	ParticleSystem::Toggle ( int p )
{ 
	toggle_[p] = !toggle_[p];
}

inline bool		
	ParticleSystem::GetToggle ( int p )	
{ 
	return toggle_[p];
}

inline std::string		
	ParticleSystem::GetSceneName ()
{ 
	return scene_name_;
}

#endif
