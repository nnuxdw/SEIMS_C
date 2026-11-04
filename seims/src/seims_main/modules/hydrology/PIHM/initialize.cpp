#include "pihm.h"

#define MAX_TYPE    100

void Initialize(pihm_struct *pihm, N_Vector CV_Y, void **cvode_mem,int seims_tstep)
{
	int             i, j;
	int             bc;

	pihm_printf(VL_VERBOSE, "\n\nInitialize data structure\n");

	// Allocate memory for solver
	*cvode_mem = CVodeCreate(CV_BDF);
	if (*cvode_mem == NULL)
	{
		pihm_printf(VL_ERROR, "Error in allocating memory for solver.\n");
		pihm_exit(EXIT_FAILURE);
	}

	// Initialize PIHM structure
	pihm->elem = (elem_struct *)malloc(nelem * sizeof(elem_struct));
	pihm->river = (river_struct *)malloc(nriver * sizeof(river_struct));
	// 所有三角形属性初始化
	for (i = 0; i < nelem; i++)
	{
		pihm->elem[i].attrib.soil = pihm->atttbl.soil[i];
#if defined(_DGW_)
		pihm->elem[i].attrib.geol = pihm->atttbl.geol[i];
#endif
		pihm->elem[i].attrib.lc = pihm->atttbl.lc[i];
		// 三角形的三个边的边界条件
		for (j = 0; j < NUM_EDGE; j++)
		{
			// 边界条件类型
			bc = pihm->atttbl.bc[i][j];
			// 不出流 0
			if (bc == NO_FLOW)
			{
				pihm->elem[i].attrib.bc[j] = NO_FLOW;
			}
			// DIRICHLET 正/ Neumann负
			else
			{
				// Adjust bc_type flag so that positive values indicate Dirichlet type, and negative values indicate
				// Neumann type
				pihm->elem[i].attrib.bc[j] = (pihm->forc.bc[bc - 1].bc_type == DIRICHLET) ? bc : -bc;
			}

#if defined(_DGW_)
			bc = pihm->atttbl.bc_geol[i][j];

			if (bc == NO_FLOW)
			{
				pihm->elem[i].attrib.bc_geol[j] = NO_FLOW;
			}
			else
			{
				// Adjust bc_type flag so that positive values indicate Dirichlet type, and negative values indicate
				// Neumann type
				pihm->elem[i].attrib.bc_geol[j] = (pihm->forc.bc[bc - 1].bc_type == DIRICHLET) ? bc : -bc;
			}
#endif
		}
		// 每个三角形的气象和叶面积指数赋值
		pihm->elem[i].attrib.meteo = pihm->atttbl.meteo[i];
		pihm->elem[i].attrib.lai = pihm->atttbl.lai[i];

#if defined(_RT_)
		pihm->elem[i].attrib.prcp_conc = pihm->atttbl.prcpc[i];
		for (j = 0; j < NCHMVOL; j++)
		{
			pihm->elem[i].attrib.chem_ic[j] = pihm->atttbl.chem_ic[i][j];
		}
#endif
	}
	// 河道边界初始化
	for (i = 0; i < nriver; i++)
	{
		bc = pihm->rivtbl.bc[i];

		if (bc == NO_FLOW)
		{
			pihm->river[i].attrib.riverbc_type = NO_FLOW;
		}
		else
		{
			// Adjust bc_type flag so that positive values indicate Dirichlet type, and negative values indicate Neumann
			// type
			pihm->river[i].attrib.riverbc_type = (pihm->forc.riverbc[bc - 1].bc_type == DIRICHLET) ? bc : -bc;
		}
	}

	// Initialize element mesh structures 初始化三角形的三个顶点、三个相邻三角形、相邻河道
	InitMesh(&pihm->meshtbl, pihm->elem);

	// Initialize element topography 初始化三角形的三个顶点坐标、高程
	InitTopo(&pihm->meshtbl, pihm->elem);

	// Calculate average elevation and total area of model domain
	pihm->siteinfo.zmax = AvgElev(pihm->elem);
	pihm->siteinfo.zmin = AvgZmin(pihm->elem);
	pihm->siteinfo.area = TotalArea(pihm->elem);

	// Initialize element soil properties
#if defined(_NOAH_)
	InitSoil(&pihm->soiltbl, &pihm->noahtbl, &pihm->calib, pihm->elem);
#else
	InitSoil(&pihm->soiltbl, &pihm->calib, pihm->elem);
#endif

#if defined(_DGW_)
	// Initialize element geol properties
	InitGeol(&pihm->geoltbl, &pihm->calib, pihm->elem);
#endif

	// Initialize element land cover properties
	InitLc(&pihm->lctbl, &pihm->calib, pihm->elem);

	// Initialize element forcing
	// 水、气胁迫
#if defined(_RT_)
	InitForcing(&pihm->rttbl, &pihm->calib, &pihm->forc, pihm->elem);
#else
	InitForcing(&pihm->calib, &pihm->forc, pihm->elem);
#endif

	// Initialize river segment properties
	InitRiver(&pihm->meshtbl, &pihm->rivtbl, &pihm->shptbl, &pihm->matltbl, &pihm->calib, pihm->elem, pihm->river);

	// Correct element elevations to avoid sinks
	if (corr_mode)
	{
		CorrectElev(pihm->river, pihm->elem);
	}

	// Calculate distances between elements
	// 相邻三角形之间的距离
	InitSurfL(&pihm->meshtbl, pihm->elem);

#if defined(_NOAH_)
	// Initialize land surface module (Noah)
	InitLsm(pihm->filename.ice, &pihm->ctrl, &pihm->noahtbl, &pihm->calib, pihm->elem);
#endif

#if defined(_CYCLES_)
	InitCycles(&pihm->calib, &pihm->agtbl, pihm->mgmttbl, pihm->croptbl, &pihm->soiltbl, pihm->elem);
#endif

#if defined(_BGC_)
	// Initialize CN (Biome-BGC) module
	InitBgc(&pihm->epctbl, &pihm->calib, pihm->elem);
#endif

#if defined(_RT_)
	InitChem(pihm->filename.cdbs, &pihm->calib, &pihm->forc, pihm->chemtbl, pihm->kintbl, &pihm->rttbl, &pihm->chmictbl,
		pihm->elem);
#endif

#if defined(_BGC_) || defined(_CYCLES_) || defined(_RT_)
	InitSolute(pihm->elem);
#endif

	// Create hydrological and land surface initial conditions
	if (pihm->ctrl.init_type == RELAX)
	{
		// Relaxation mode
		// Noah initialization needs air temperature thus forcing is applied
#if defined(_RT_)
		ApplyForcing(pihm->ctrl.starttime, pihm->ctrl.rad_mode, &pihm->siteinfo, &pihm->rttbl, &pihm->forc, pihm->elem);
#elif defined(_NOAH_)
		ApplyForcing(pihm->ctrl.starttime, pihm->ctrl.rad_mode, &pihm->siteinfo, &pihm->forc, pihm->elem);
#endif
		// 设置初始水分、水位条件、河道水位条件
		RelaxIc(pihm->elem, pihm->river);
	}
	else if (pihm->ctrl.init_type == RST_FILE)
	{
		// Hot start (using .ic file)
		ReadIc(pihm->filename.ic, pihm->elem, pihm->river);
	}

	// Initialize state variables
	InitVar(pihm->elem, pihm->river, CV_Y);

#if defined(_CYCLES_)
	// Initialize Cycles module
	if (pihm->ctrl.read_cycles_restart)
	{
		ReadCyclesIc(pihm->filename.cyclesic, pihm->elem);
	}
	else
	{
		FirstDay(&pihm->soiltbl, &pihm->ctrl, pihm->elem);
	}

	InitAgVar(pihm->elem, pihm->river, CV_Y);
#endif

#if defined(_BGC_)
	// Initialize CN variables
	if (pihm->ctrl.read_bgc_restart)
	{
		ReadBgcIc(pihm->filename.bgcic, pihm->elem, pihm->river);
	}
	else
	{
		FirstDay(&pihm->cninit, pihm->elem, pihm->river);
	}

	InitBgcVar(pihm->elem, pihm->river, CV_Y);
#endif

#if defined(_RT_)
	if (pihm->ctrl.read_rt_restart)
	{
		ReadRtIc(pihm->filename.rtic, pihm->elem);
	}

	InitRTVar(pihm->chemtbl, &pihm->rttbl, pihm->elem, pihm->river, CV_Y);
#endif

	// Calculate model time steps
	//CalcModelSteps(&pihm->ctrl);
	// xiaodw, 根据SEIMS时间步长计算PIHM时间步长
	//CalcPIHMSteps(&pihm->ctrl, seims_tstep);

#if defined(_DAILY_)
	InitDailyStruct(pihm->elem);
#endif
}

void CorrectElev(const river_struct river[], elem_struct elem[])
{
	int             i, j;
	int             sink;
	int             river_flag = 0;
	double          nabr_zmax;
	double          new_elevation;

	pihm_printf(VL_VERBOSE, "Correct surface elevation.\n");

	for (i = 0; i < nelem; i++)
	{
		// Correction of surface elevation (artifacts due to coarse scale discretization). Not needed if there is lake
		// feature.
		sink = 1;

		for (j = 0; j < NUM_EDGE; j++)
		{
			if (elem[i].nabr[j] != 0)
			{
				nabr_zmax = (elem[i].nabr_river[j] == 0) ?
					elem[elem[i].nabr[j] - 1].topo.zmax : river[elem[i].nabr_river[j] - 1].topo.zmax;
				if (elem[i].topo.zmax >= nabr_zmax)
				{
					sink = 0;
					break;
				}
			}
		}

		if (sink == 1)
		{
			pihm_printf(VL_NORMAL, "Element %4d is a sink.\n", i + 1);

			// Note: Following correction is being applied for correction mode only
			pihm_printf(VL_NORMAL,
				"  Before correction: surface %7.2lf m, bedrock %7.2lf m. ", elem[i].topo.zmax, elem[i].topo.zmin);

			new_elevation = 1.0e7;
			for (j = 0; j < NUM_EDGE; j++)
			{
				if (elem[i].nabr[j] != 0)
				{
					nabr_zmax = (elem[i].nabr_river[j] == 0) ?
						elem[elem[i].nabr[j] - 1].topo.zmax : river[elem[i].nabr_river[j] - 1].topo.zmax;
					new_elevation = (nabr_zmax < new_elevation) ? nabr_zmax : new_elevation;
				}
			}

			// Lift bedrock elevation by the same amount
			elem[i].topo.zmin += new_elevation - elem[i].topo.zmax;

			// Apply new surface elevation
			elem[i].topo.zmax = new_elevation;

			pihm_printf(VL_NORMAL, "Corrected = %7.2lf m, %7.2lf m.\n", elem[i].topo.zmax, elem[i].topo.zmin);
		}
	}

	for (i = 0; i < nriver; i++)
	{
		if (river[i].down > 0)
		{
			if (river[i].topo.zbed < river[river[i].down - 1].topo.zbed)
			{
				river_flag = 1;
				pihm_printf(VL_NORMAL, "River %d is lower than downstream River %d.\n", i + 1, river[i].down);
			}
		}
		else
		{
			if (river[i].topo.zbed <= river[i].topo.node_zmax - river[i].shp.depth)
			{
				river_flag = 1;
				pihm_printf(VL_NORMAL, "River outlet is higher than the channel (River %d).\n", i + 1);
			}
		}
	}

	if (river_flag == 1)
	{
		pihm_printf(VL_NORMAL,
			"\nRiver elevation correction needed but PIHM will continue without fixing river elevation.\n\n");
	}
}

void InitSurfL(const meshtbl_struct *meshtbl, elem_struct elem[])
{
	int             i;

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		int             j;
		double          x[NUM_EDGE];
		double          y[NUM_EDGE];
		double          zmin[NUM_EDGE];
		double          zmax[NUM_EDGE];
		double          distx;
		double          disty;

		for (j = 0; j < NUM_EDGE; j++)
		{
			x[j] = meshtbl->x[elem[i].node[j] - 1];
			y[j] = meshtbl->y[elem[i].node[j] - 1];
			zmin[j] = meshtbl->zmin[elem[i].node[j] - 1];
			zmax[j] = meshtbl->zmax[elem[i].node[j] - 1];
		}

		for (j = 0; j < NUM_EDGE; j++)
		{
			// Note: Assumption here is that the formulation is circumcenter based
			// 质心到边界的距离，x、y方向
			switch (j)
			{
			case 0:
				distx = (elem[i].topo.x - 0.5 * (x[1] + x[2]));
				disty = (elem[i].topo.y - 0.5 * (y[1] + y[2]));
				break;
			case 1:
				distx = (elem[i].topo.x - 0.5 * (x[2] + x[0]));
				disty = (elem[i].topo.y - 0.5 * (y[2] + y[0]));
				break;
			case 2:
				distx = (elem[i].topo.x - 0.5 * (x[0] + x[1]));
				disty = (elem[i].topo.y - 0.5 * (y[0] + y[1]));
				break;
			}
			// 位于边界的三角形
			if (elem[i].nabr[j] == 0)
			{
				// 计算一个虚拟相邻三角形的质心，假设这个虚拟相邻三角形和当前三角形形状相同，当前质心x-2*质心与边界的垂距
				elem[i].topo.x_nabr[j] = elem[i].topo.x - 2.0 * distx;
				elem[i].topo.y_nabr[j] = elem[i].topo.y - 2.0 * disty;
				// 边界三角形的边界外相邻距离 = sqrt[(边长1*边长2*边长3/4/三角形面积)2 - (边界边长/2)2]
				elem[i].topo.dist_nabr[j] = sqrt(pow(elem[i].topo.edge[0] * elem[i].topo.edge[1] *
					elem[i].topo.edge[2] / (4.0 * elem[i].topo.area), 2) - pow(elem[i].topo.edge[j] / 2.0, 2));
			}
			// 位于内部的三角形
			else
			{
				elem[i].topo.x_nabr[j] = elem[elem[i].nabr[j] - 1].topo.x;
				elem[i].topo.y_nabr[j] = elem[elem[i].nabr[j] - 1].topo.y;
				// 正常相邻距离=sqrt（Δx2 + Δy2）
				elem[i].topo.dist_nabr[j] =
					(elem[i].topo.x - elem[i].topo.x_nabr[j]) * (elem[i].topo.x - elem[i].topo.x_nabr[j]);
				elem[i].topo.dist_nabr[j] +=
					(elem[i].topo.y - elem[i].topo.y_nabr[j]) * (elem[i].topo.y - elem[i].topo.y_nabr[j]);
				elem[i].topo.dist_nabr[j] = sqrt(elem[i].topo.dist_nabr[j]);
			}
		}
	}
}

double _WsAreaElev(int type, const elem_struct *elem)
{
	double          ans = 0.0;
	int             i;

	for (i = 0; i < nelem; i++)
	{
		switch (type)
		{
		case WS_ZMAX:
			ans += elem[i].topo.zmax;
			break;
		case WS_ZMIN:
			ans += elem[i].topo.zmin;
			break;
		case WS_AREA:
			ans += elem[i].topo.area;
			break;
		default:
			ans = BADVAL;
			pihm_printf(VL_ERROR, "Error: Return value type %d id not defined.\n", type);
			pihm_exit(EXIT_FAILURE);
		}
	}

	return (type == WS_AREA) ? ans : ans / (double)nelem;
}

void RelaxIc(elem_struct elem[], river_struct river[])
{
	int             i;
	const double    INIT_UNSAT = 0.1;
#if defined(_DGW_)
	const double    INIT_DGW = 5.0;
#endif

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		elem[i].ic.cmc = 0.0;
		elem[i].ic.sneqv = 0.0;
		elem[i].ic.surf = 0.0;
		// 非饱和层初始含水量=0.1m
		elem[i].ic.unsat = INIT_UNSAT;
		// 初始地下水位=土壤厚度-非饱和层初始含水量
		elem[i].ic.gw = elem[i].soil.depth - INIT_UNSAT;

#if defined(_DGW_)
		// 深层地下水位是位于基岩高程之上的深层地下水深，初始化为土壤厚度和5m中的较小值
		elem[i].ic.gw_geol = MIN(elem[i].geol.depth, INIT_DGW);
		elem[i].ic.unsat_geol = 0.5 * (elem[i].geol.depth - elem[i].ic.gw_geol);
#endif

#if defined(_NOAH_)
		int             j;
		double          sfctmp;

		sfctmp = elem[i].es.sfctmp;

		elem[i].ic.t1 = sfctmp;

		elem[i].ic.stc[0] = sfctmp + (sfctmp - elem[i].ps.tbot) / elem[i].ps.zbot * elem[i].ps.soil_depth[0] * 0.5;

		for (j = 1; j < MAXLYR; j++)
		{
			elem[i].ic.stc[j] = (elem[i].ps.soil_depth[j] > 0.0) ?
				elem[i].ic.stc[j - 1] + (sfctmp - elem[i].ps.tbot) / elem[i].ps.zbot * 0.5 *
				(elem[i].ps.soil_depth[j - 1] + elem[i].ps.soil_depth[j]) : BADVAL;
		}

		for (j = 0; j < MAXLYR; j++)
		{
			elem[i].ic.smc[j] = (elem[i].ps.soil_depth[j] > 0.0) ? elem[i].soil.smcmax : BADVAL;
			elem[i].ic.swc[j] = (elem[i].ps.soil_depth[j] > 0.0) ? elem[i].soil.smcmax : BADVAL;
		}

		elem[i].ic.snowh = 0.0;
#endif
	}

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nriver; i++)
	{
		river[i].ic.stage = 0.0;
	}
}

void InitVar(elem_struct elem[], river_struct river[], N_Vector CV_Y)
{
	int             i;

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	// State variables (initial conditions) 
	// ic是地表初始水分条件，ws是三角形上的水位，水位初始化为地表水分状态
	for (i = 0; i < nelem; i++)
	{
		elem[i].ws.cmc = elem[i].ic.cmc;
		elem[i].ws.sneqv = elem[i].ic.sneqv;

		elem[i].ws.surf = elem[i].ic.surf;
		elem[i].ws.unsat = elem[i].ic.unsat;
		elem[i].ws.gw = elem[i].ic.gw;
		// 将地表、非饱和层、地下水层、河流初始水位写入到CV_Y内存中
		NV_Ith(CV_Y, SURF(i)) = elem[i].ic.surf;
		NV_Ith(CV_Y, UNSAT(i)) = elem[i].ic.unsat;
		NV_Ith(CV_Y, GW(i)) = elem[i].ic.gw;

#if defined(_DGW_)
		elem[i].ws.unsat_geol = elem[i].ic.unsat_geol;
		elem[i].ws.gw_geol = elem[i].ic.gw_geol;

		NV_Ith(CV_Y, UNSAT_GEOL(i)) = elem[i].ic.unsat_geol;
		NV_Ith(CV_Y, GW_GEOL(i)) = elem[i].ic.gw_geol;
#endif

#if defined(_NOAH_)
		int             j;

		elem[i].es.t1 = elem[i].ic.t1;
		elem[i].ps.snowh = elem[i].ic.snowh;

		for (j = 0; j < MAXLYR; j++)
		{
			elem[i].es.stc[j] = elem[i].ic.stc[j];
			elem[i].ws.smc[j] = elem[i].ic.smc[j];
			elem[i].ws.swc[j] = elem[i].ic.swc[j];
		}
#endif

		elem[i].ws0 = elem[i].ws;
	}

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nriver; i++)
	{
		river[i].ws.stage = river[i].ic.stage;

		NV_Ith(CV_Y, RIVER(i)) = river[i].ic.stage;
	}

	// Other variables
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		InitWFlux(&elem[i].wf);

#if defined(_NOAH_)
		elem[i].ps.snotime1 = 0.0;
		elem[i].ps.ribb = 0.0;
		elem[i].ps.fcr = 1.0;
		elem[i].ps.snoalb = 0.75;
		elem[i].ps.zlvl = 3.0;
		elem[i].ps.emissi = 0.96;
		elem[i].ps.albedo = 0.18;
		elem[i].ps.z0 = 0.1;
		elem[i].ps.ch = 1.0E-4;
		elem[i].ps.cm = 1.0E-4;
		elem[i].ps.beta = BADVAL;
		elem[i].ps.sncovr = BADVAL;
		elem[i].ps.rc = BADVAL;
		elem[i].ps.pc = BADVAL;
		elem[i].ps.rcs = BADVAL;
		elem[i].ps.rct = BADVAL;
		elem[i].ps.rcsoil = BADVAL;
		elem[i].ps.q1 = BADVAL;
		elem[i].ps.z0brd = BADVAL;
		elem[i].ps.eta_kinematic = BADVAL;

		elem[i].ef.sheat = BADVAL;
		elem[i].ef.eta = BADVAL;
		elem[i].ef.fdown = BADVAL;
		elem[i].ef.ec = BADVAL;
		elem[i].ef.edir = BADVAL;
		elem[i].ef.ett = BADVAL;
		elem[i].ef.etp = BADVAL;
		elem[i].ef.ssoil = BADVAL;
		elem[i].ef.flx1 = BADVAL;
		elem[i].ef.flx2 = BADVAL;
		elem[i].ef.flx3 = BADVAL;
		elem[i].ef.esnow = BADVAL;

		elem[i].wf.runoff2 = BADVAL;
		elem[i].wf.runoff3 = BADVAL;
		elem[i].wf.pcpdrp = 0.0;
		elem[i].wf.drip = 0.0;
		elem[i].wf.dew = BADVAL;
		elem[i].wf.snomlt = BADVAL;

		elem[i].ws.soilm = BADVAL;
#endif
	}
}

void CalcModelSteps(ctrl_struct *ctrl)
{
	int             i;

	ctrl->nstep = (ctrl->endtime - ctrl->starttime) / ctrl->stepsize;

	ctrl->tout = (int *)malloc((ctrl->nstep + 1) * sizeof(int));

	for (i = 0; i < ctrl->nstep + 1; i++)
	{
		ctrl->tout[i] = (i == 0) ? ctrl->starttime : ctrl->tout[i - 1] + ctrl->stepsize;
	}

	ctrl->tout[ctrl->nstep] = (ctrl->tout[ctrl->nstep] < ctrl->endtime) ? ctrl->endtime : ctrl->tout[ctrl->nstep];
}

//int CalcCurSimulateTime(ctrl_struct *ctrl,int seims_tstep, int counter,int * cur_sim_step_ptr) {
//	*cur_sim_step_ptr = ctrl->starttime + counter * seims_tstep;
//	return *cur_sim_step_ptr;
//}

void CalcPIHMSteps(ctrl_struct *ctrl, int seims_tstep, int counter, int * cur_simu_time_ptr,int * last_sim_time_ptr) {

	int             i;

	//ctrl->nstep = seims_tstep / ctrl->stepsize;
	int last_sim_time = ctrl->starttime + (counter - 1) * seims_tstep;
	int cur_sim_time = ctrl->starttime + counter * seims_tstep;
	last_sim_time_ptr = &last_sim_time;
	cur_simu_time_ptr = &cur_sim_time;
	ctrl->nstep = (*cur_simu_time_ptr - *last_sim_time_ptr) / ctrl->stepsize;

	ctrl->tout = (int *)malloc((ctrl->nstep + 1) * sizeof(int));

	for (i = 0; i < ctrl->nstep + 1; i++)
	{
		ctrl->tout[i] = (i == 0) ? *last_sim_time_ptr : ctrl->tout[i - 1] + ctrl->stepsize;
		//ctrl->tout[i] = (i == 0) ? ctrl->starttime : ctrl->tout[i - 1] + ctrl->stepsize;
	}
	// xiaodw, 当前时步是相对于当前SEIMS时步内的，而ctrl->endtime是PIHM自己设置的结束时间
	ctrl->tout[ctrl->nstep] = (ctrl->tout[ctrl->nstep] < cur_sim_time) ? cur_sim_time : ctrl->tout[ctrl->nstep];
	//ctrl->tout[ctrl->nstep] = (ctrl->tout[ctrl->nstep] < ctrl->endtime) ? ctrl->endtime : ctrl->tout[ctrl->nstep];
}


void InitWFlux(wflux_struct *wf)
{
	int             j;

	for (j = 0; j < NUM_EDGE; j++)
	{
		wf->overland[j] = 0.0;
		wf->subsurf[j] = 0.0;
	}
	wf->prcp = 0.0;
	wf->pcpdrp = 0.0;
	wf->infil = 0.0;
	wf->eqv_infil = 0.0;
	wf->recharge = 0.0;
	wf->drip = 0.0;
	wf->edir = 0.0;
	wf->ett = 0.0;
	wf->ec = 0.0;
	wf->etp = 0.0;
	wf->eta = 0.0;
	wf->edir_surf = 0.0;
	wf->edir_unsat = 0.0;
	wf->edir_gw = 0.0;
	wf->ett_unsat = 0.0;
	wf->ett_gw = 0.0;
	wf->esnow = 0.0;

#if defined(_DGW_)
	wf->infil_geol = 0.0;
	wf->rechg_geol = 0.0;
	for (j = 0; j < NUM_EDGE; j++)
	{
		wf->dgw[j] = 0.0;
	}
#endif

#if defined(_NOAH_)
	int             k;

	for (k = 0; k < MAXLYR; k++)
	{
		wf->et[k] = 0.0;
		wf->runoff2_lyr[k] = 0.0;
		wf->smflx[k] = 0.0;
	}
	wf->runoff2 = 0.0;
	wf->runoff3 = 0.0;
	wf->dew = 0.0;
	wf->snomlt = 0.0;
	wf->etns = 0.0;
#endif
#if defined(_CYCLES_)
	wf->irrig = 0.0;
#endif
}

void InitRiverWFlux(river_wflux_struct *wf)
{
	int             j;
	// 一个河段上有6种流量交换：上游、下游、地表左右、含水层左右
	for (j = 0; j < NUM_RIVFLX; j++)
	{
		wf->rivflow[j] = 0.0;
	}
}

void InitEFlux(eflux_struct *ef)
{
	ef->soldn = 0.0;

#if defined(_NOAH_)
	int             k;

	for (k = 0; k < MAXLYR; k++)
	{
		ef->et[k] = 0.0;
	}
	ef->solnet = 0.0;
	ef->etp = 0.0;
	ef->ssoil = 0.0;
	ef->eta = 0.0;
	ef->sheat = 0.0;
	ef->fdown = 0.0;
	ef->lwdn = 0.0;
	ef->ec = 0.0;
	ef->edir = 0.0;
	ef->ett = 0.0;
	ef->esnow = 0.0;
	ef->soldir = 0.0;
	ef->soldif = 0.0;
	ef->longwave = 0.0;
	ef->flx1 = 0.0;
	ef->flx2 = 0.0;
	ef->flx3 = 0.0;
#endif
#if defined(_BGC_)
	ef->swabs_per_plaisun = 0.0;
	ef->swabs_per_plaishade = 0.0;
#endif
}
