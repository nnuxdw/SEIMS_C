#include "pihm.h"
#include <time.h>
#include <stdio.h>
//#include <iostream>
//using namespace std;
int Ode(realtype t, N_Vector CV_Y, N_Vector CV_Ydot, void *pihm_data)

{

	int             i;
	double         *y;
	double         *dy;
	/*pihm_struct     pihm_strc;*/
	pihm_struct     *pihm;
	elem_struct    *elem;
	river_struct   *river;
	// xiaodw
	PIHMToolDataStruct* PIHMTool_Data;
	SeimsVariablesStruct * SeimsVariables;

	y = NV_DATA(CV_Y);
	dy = NV_DATA(CV_Ydot);
	//pihm_strc = (pihm_struct)pihm_data;
	pihm = (pihm_struct*)pihm_data;
#if defined(_STATISTIC_TIME_)
	clock_t ode_start = clock();
#endif
	elem = &pihm->elem[0];
	river = &pihm->river[0];
	PIHMTool_Data = pihm->PIHMToolData;
	SeimsVariables = pihm->SeimsVariables;

	if (PIHMTool_Data != nullptr && PIHMTool_Data->hru_ids != nullptr) {
		for (int id : *(PIHMTool_Data->hru_ids)) {
			cout << "HRU ID: " << id << endl;
		}
	}
	else {
		cout << "PIHMTool_Data or hru_ids is null" << endl;
	}
	map<int, int> upstream_hru_down_map;
	map<int, map<int, float>> upstream_hru__down_tris_map;
	std::unordered_set<int> upstream_hru_id_keys;
	if (PIHMTool_Data != nullptr && PIHMTool_Data->hrus != nullptr) {
		for (hru_struct hru : *(PIHMTool_Data->hrus)) {
			//cout << "HRU ID: " << hru.key << " down_type: " << hru.down_type << endl;
			if (hru.down_type == 1 && !hru.down_ids.empty()) {
				//upstream_hru_down_map[hru.key] = hru.down_id;
				upstream_hru_id_keys.insert(hru.key);
				upstream_hru__down_tris_map[hru.key] = hru.down_ids;
				//cout << "down_ids: ";
				//for (const auto& pair : hru.down_ids) {
				//	cout << "Key: " << pair.first << ", Value: " << pair.second << "; ";
				//}
				//cout << endl;
				
			}
			else if (hru.down_type == 0  ){
				//cout << "down_id: " << hru.down_id << endl;
			}
			else {
				cout << "error in hru up down relation !!!!!!!!!!!!!!!!" << endl;
			}

		}
	}
	else {
		cout << "PIHMTool_Data or hru_ids is null" << endl;
	}
	map<int, double> tri_id_streamq_map;
	pihm->exchange = new exchange_struct();
	pihm->exchange->elem_upstream_surfq = new double[nelem];
	int curSubbasinId = -1;
	if (SeimsVariables != nullptr && SeimsVariables->m_surfRftotal != nullptr) {
		for (int i = 0; i < SeimsVariables->m_nCells; i++) {
			// 如果是上游HRU, 将上游地表径流量分配给下游三角形的地表水深
			if (upstream_hru_id_keys.find(i) != upstream_hru_id_keys.end()) {
				for (map<int, float>::iterator it = upstream_hru__down_tris_map[i].begin(); it != upstream_hru__down_tris_map[i].end(); ++it) {
					//std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
					//tri_id_streamq_map[(int)it->first] = SeimsVariables->m_surfRftotal[i] * pihm->ctrl.stepsize * (double)it->second;
					// 地表水：流量m3/s * 步长s * 比例  
					pihm->exchange->elem_upstream_surfq[int(it->first)] = SeimsVariables->m_surfRftotal[i] * pihm->ctrl.stepsize * (double)it->second;
					// 土壤水：每层地下水量m3* 比例 * pihm的步长s/seims的步长s
					for (int j = 0; j < static_cast<int>(SeimsVariables->m_nSoilLyrs[i]); j++) {
						pihm->exchange->elem_upstream_subsurvol[int(it->first)] += SeimsVariables->m_subSurfRfVol[i][j] * pihm->ctrl.stepsize *  (double)it->second / SeimsVariables->m_TimeStep; /// m^3/s
					}
					// 地下水：每个上游HRU所在子流域的所有地下水
					curSubbasinId = SeimsVariables->m_subbsnID[i];
					pihm->exchange->elem_upstream_gwStorage[int(it->first)] = SeimsVariables->m_gwStorage[curSubbasinId] * SeimsVariables->subbasin_area[curSubbasinId] * (double)it->second
						* pihm->ctrl.stepsize / SeimsVariables->m_TimeStep ;

					// 输出变量值在一行
					std::cout << "tri_id: " << it->first
						<< ", surfRftotal[" << i << "]: " << SeimsVariables->m_surfRftotal[i]
						<< ", stepsize: " << pihm->ctrl.stepsize
						<< ", percent: " << it->second
						<< ", surfq: " << pihm->exchange->elem_upstream_surfq[int(it->first)]
						<< std::endl;
				}
				
				cout << "upstream Key " << i << " has downstream." << std::endl;
			}
		}


		for (int i = 0; i < nelem; i++){
			cout << "HRU RUNOFF: " << SeimsVariables->m_surfRftotal[i] << endl;
		}
	}
	else {
		cout << "HRU RUNOFF is null" << endl;
	}
	pihm->exchange->elem_upstream_subsurvol = new double[nelem];
	/*
	for (int ilyr = 0; ilyr < SeimsVariables->m_nRteLyrs; ilyr++) {
		// There are not any flow relationship within each routing layer.
		// So parallelization can be done here.
		//int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
		int ncells = static_cast<int>(SeimsVariables->m_rteLyrs[ilyr][0]);
		
		// 遍历每个并行层的所有地块，xiaodw
#pragma omp parallel for reduction(+: errCount)
		for (int icell = 1; icell <= ncells; icell++) {
			// 地块id，xiaodw
			float qiAllLayers = 0.f;
			int id = static_cast<int>(SeimsVariables->m_rteLyrs[ilyr][icell]);
			for (int j = 0; j < static_cast<int>(SeimsVariables->m_nSoilLyrs[id]); j++) {
				if (SeimsVariables->m_subSurfRfVol[i][j] > UTIL_ZERO) {
					if (upstream_hru_id_keys.find(i) != upstream_hru_id_keys.end()) {
						for (map<int, float>::iterator it = upstream_hru__down_tris_map[i].begin(); it != upstream_hru__down_tris_map[i].end(); ++it) {
							pihm->exchange->elem_upstream_subsurvol[id] += SeimsVariables->m_subSurfRfVol[i][j] * pihm->ctrl.stepsize / SeimsVariables->m_TimeStep; /// m^3/s
						}
					}
				}
			}
		}
	}
	*/
	// Initialization of RHS of ODEs
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	// 三角形数量*3 + 河流数量
	for (i = 0; i < NumStateVar(); i++)
	{
		dy[i] = 0.0;
	}

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		// 从CVODE内存中取出水位状态变量
		elem[i].ws.surf = MAX(y[SURF(i)], 0.0);
		elem[i].ws.unsat = MAX(y[UNSAT(i)], 0.0);
		elem[i].ws.gw = MAX(y[GW(i)], 0.0);

#if defined(_DGW_)
		elem[i].ws.unsat_geol = MAX(y[UNSAT_GEOL(i)], 0.0);
		elem[i].ws.gw_geol = MAX(y[GW_GEOL(i)], 0.0);
#endif

#if defined(_BGC_)
		elem[i].ns.sminn = MAX(y[SOLUTE_SOIL(i, 0)], 0.0);
#endif

#if defined(_CYCLES_)
		elem[i].ps.no3 = MAX(y[SOLUTE_SOIL(i, NO3)], 0.0);
		elem[i].ps.nh4 = MAX(y[SOLUTE_SOIL(i, NH4)], 0.0);
#endif

#if defined(_RT_)
		int             k;

		for (k = 0; k < nsolute; k++)
		{
			elem[i].chms.tot_mol[k] = MAX(y[SOLUTE_SOIL(i, k)], 0.0);
# if defined(_DGW_)
			elem[i].chms_geol.tot_mol[k] = MAX(y[SOLUTE_GEOL(i, k)], 0.0);
# endif
		}
#endif
	}

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nriver; i++)
	{
		river[i].ws.stage = MAX(y[RIVER(i)], 0.0);

#if defined(_BGC_)
		river[i].ns.streamn = MAX(y[SOLUTE_RIVER(i, 0)], 0.0);
#endif

#if defined(_CYCLES_)
		river[i].ns.no3 = MAX(y[SOLUTE_RIVER(i, NO3)], 0.0);
		river[i].ns.nh4 = MAX(y[SOLUTE_RIVER(i, NH4)], 0.0);
#endif

#if defined(_RT_)
		int             k;

		for (k = 0; k < nsolute; k++)
		{
			river[i].chms.tot_mol[k] = MAX(y[SOLUTE_RIVER(i, k)], 0.0);
		}
#endif
	}

	// PIHM Hydrology fluxes 关键方法，计算地表水、地下水流量
#if defined(_STATISTIC_TIME_)
	Hydrol(&pihm->ctrl, pihm->elem, pihm->river, pihm->ptime_calculator);
#else 
	Hydrol(&pihm->ctrl, pihm->elem, pihm->river,pihm->exchange);
#endif
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t5_1 = clock();
	pihm->ptime_calculator->solvecvode_hydro_time += ((double)(pihm->ptime_calculator->t5_1 - ode_start)) / CLOCKS_PER_SEC;
#endif
	// Calculate solute concentrations
#if defined(_BGC_)
	SoluteConc(pihm->elem, pihm->river);
#elif defined(_CYCLES_)
	SoluteConc(t + (realtype)pihm->ctrl.tout[0] - (realtype)pihm->ctrl.tout[pihm->ctrl.cstep], pihm->elem, pihm->river);
#elif defined(_RT_)
	SoluteConc(pihm->chemtbl, &pihm->rttbl, pihm->elem, pihm->river);
#endif


#if defined(_BGC_) || defined(_CYCLES_)
	SoluteTranspt(0.0, 0.0, 0.0, pihm->elem, pihm->river);
#elif defined(_RT_)
	SoluteTranspt(pihm->rttbl.diff_coef, pihm->rttbl.disp_coef, pihm->rttbl.cementation, pihm->elem, pihm->river);
#endif

	// Build RHS of ODEs
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		int             j;
		// dy(m)
		// Vertical water fluxes for surface and subsurface
		// xiaodw, 在这里加入上游HRU的来水
		dy[SURF(i)] += elem[i].wf.pcpdrp - elem[i].wf.infil - elem[i].wf.edir_surf;
		dy[UNSAT(i)] += elem[i].wf.infil - elem[i].wf.recharge - elem[i].wf.edir_unsat - elem[i].wf.ett_unsat;
		dy[GW(i)] += elem[i].wf.recharge - elem[i].wf.edir_gw - elem[i].wf.ett_gw;

#if defined(_DGW_)
		// Vertical water fluxes for deep zone
		dy[GW(i)] -= elem[i].wf.infil_geol;

		dy[UNSAT_GEOL(i)] += elem[i].wf.infil_geol - elem[i].wf.rechg_geol;
		dy[GW_GEOL(i)] += elem[i].wf.rechg_geol;
#endif

		// Horizontal water fluxes
		for (j = 0; j < NUM_EDGE; j++)
		{
			dy[SURF(i)] -= elem[i].wf.overland[j] / elem[i].topo.area;
			dy[GW(i)] -= elem[i].wf.subsurf[j] / elem[i].topo.area;
#if defined(_DGW_)
			dy[GW_GEOL(i)] -= elem[i].wf.dgw[j] / elem[i].topo.area;
#endif
		}
		// dy 单位是m，为什么要比孔隙度啊？因为dy是指有效水深，即设定没有土壤占据空间的水深，除以孔隙度代表被土壤占据一定空间后的实际水深
		dy[UNSAT(i)] /= elem[i].soil.porosity;
		dy[GW(i)] /= elem[i].soil.porosity;
#if defined(_DGW_)
		dy[UNSAT_GEOL(i)] /= elem[i].geol.porosity;
		dy[GW_GEOL(i)] /= elem[i].geol.porosity;
#endif

#if defined(_CYCLES_) || defined(_BGC_) || defined(_RT_)
		int             k;

		for (k = 0; k < nsolute; k++)
		{
# if defined(_CYCLES_)
			dy[SOLUTE_SOIL(i, k)] += elem[i].solute[k].infil + Profile(elem[i].ps.nlayers, elem[i].solute[k].snksrc);
# else
			dy[SOLUTE_SOIL(i, k)] += elem[i].solute[k].infil + elem[i].solute[k].snksrc;
# endif

# if defined(_DGW_)
			dy[SOLUTE_SOIL(i, k)] -= elem[i].solute[k].infil_geol;

			dy[SOLUTE_GEOL(i, k)] += elem[i].solute[k].infil_geol + elem[i].solute[k].snksrc_geol;
# endif

			for (j = 0; j < NUM_EDGE; j++)
			{
				dy[SOLUTE_SOIL(i, k)] -= elem[i].solute[k].subflux[j] / elem[i].topo.area;
# if defined(_DGW_)
				dy[SOLUTE_GEOL(i, k)] -= elem[i].solute[k].dgwflux[j] / elem[i].topo.area;
# endif
			}
		}
#endif
	}

	// ODEs for river segments
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nriver; i++)
	{
		int             j;

		for (j = 0; j < NUM_RIVFLX; j++)
		{
			// Note the limitation due to d(v) / dt = a * dy / dt + y * da / dt for cs other than rectangle
			dy[RIVER(i)] -= river[i].wf.rivflow[j] / river[i].topo.area;
		}

#if defined(_CYCLES_) || defined(_BGC_) || defined(_RT_)
		int             k;

		for (k = 0; k < nsolute; k++)
		{
			for (j = 0; j < NUM_RIVFLX; j++)
			{
				dy[SOLUTE_RIVER(i, k)] -= river[i].solute[k].flux[j] / river[i].topo.area;
			}
		}
#endif
	}
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t5_2 = clock();
	pihm->ptime_calculator->solvecvode_bgc_time += ((double)(pihm->ptime_calculator->t5_2 - pihm->ptime_calculator->t5_1)) / CLOCKS_PER_SEC;
#endif
	return 0;
}

int NumStateVar(void)
{
	// Return number of state variables
	int             nsv;

	nsv = 3 * nelem + nriver;

#if defined(_BGC_) || defined(_CYCLES_) || defined(_RT_)
	nsv += nsolute * (nelem + nriver);
#endif

#if defined(_DGW_)
	nsv += 2 * nelem;
# if defined(_BGC_) || defined(_CYCLES_) || defined(_RT_)
	nsv += nsolute * nelem;
# endif
#endif

	return nsv;
}

void SetCVodeParam(pihm_struct *pihm, void *cvode_mem, SUNLinearSolver *sun_ls, N_Vector CV_Y)
{
	int             cv_flag;
	static int      reset;
	N_Vector        abstol;
#if defined(_BGC_) || defined(_CYCLES_)
	const double    TRANSP_TOL = 1.0E-5;
#elif defined(_RT_)
	const double    TRANSP_TOL = 1.0E-8;
#endif

	pihm->ctrl.maxstep = pihm->ctrl.stepsize;

	if (reset)
	{
		// When model spins-up and recycles forcing, use CVodeReInit to reset solver time, which does not allocates
		// memory
		cv_flag = CVodeReInit(cvode_mem, 0.0, CV_Y);
		CheckCVodeFlag(cv_flag);
	}
	else
	{
		// 这里设置Ode方法，求解流量
		cv_flag = CVodeInit(cvode_mem, Ode, 0.0, CV_Y);
		CheckCVodeFlag(cv_flag);
		reset = 1;

		*sun_ls = SUNLinSol_SPGMR(CV_Y, PREC_NONE, 0);

		// Attach the linear solver
		CVodeSetLinearSolver(cvode_mem, *sun_ls, NULL);

		// When BGC, Cycles, or RT module is turned on, both water storage and transport variables are in the CVODE
		// vector. A vector of absolute tolerances is needed to specify different absolute tolerances for water storage
		// variables and transport variables
		abstol = N_VNew(NumStateVar());
#if defined(_BGC_) || defined(_CYCLES_) || defined(_RT_)
		SetAbsTolArray(pihm->ctrl.abstol, TRANSP_TOL, abstol);
#else
		SetAbsTolArray(pihm->ctrl.abstol, abstol);
#endif

		cv_flag = CVodeSVtolerances(cvode_mem, (realtype)pihm->ctrl.reltol, abstol);
		CheckCVodeFlag(cv_flag);

		N_VDestroy(abstol);

		// Specifies PIHM data block and attaches it to the main cvode memory block
		cv_flag = CVodeSetUserData(cvode_mem, pihm);
		CheckCVodeFlag(cv_flag);
		//xiaodw
		//cv_flag = CVodeSetUserData(cvode_mem, PIHMToolData);
		//CheckCVodeFlag(cv_flag);

		// Specifies the initial step size
		cv_flag = CVodeSetInitStep(cvode_mem, (realtype)pihm->ctrl.initstep);
		CheckCVodeFlag(cv_flag);

		// Indicates if the BDF stability limit detection algorithm should be used
		cv_flag = CVodeSetStabLimDet(cvode_mem, SUNTRUE);
		CheckCVodeFlag(cv_flag);

		// Specifies an upper bound on the magnitude of the step size
		cv_flag = CVodeSetMaxStep(cvode_mem, (realtype)pihm->ctrl.maxstep);
		CheckCVodeFlag(cv_flag);

		// Specifies the maximum number of steps to be taken by the solver in its attempt to reach the next output time
		cv_flag = CVodeSetMaxNumSteps(cvode_mem, pihm->ctrl.stepsize * 10);
		CheckCVodeFlag(cv_flag);
	}
}

#if defined(_BGC_) || defined(_CYCLES_) || defined(_RT_)
void SetAbsTolArray(double hydrol_tol, double transp_tol, N_Vector abstol)
#else
void SetAbsTolArray(double hydrol_tol, N_Vector abstol)
#endif
{
	int             i;
	int             num_hydrol_var;

	num_hydrol_var = 3 * nelem + nriver;

#if defined(_DGW_)
	num_hydrol_var += 2 * nelem;
#endif

	// Set absolute errors for hydrologic state variables
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < num_hydrol_var; i++)
	{
		NV_Ith(abstol, i) = (realtype)hydrol_tol;
	}

#if defined(_BGC_) || defined(_CYCLES_) || defined(_RT_)
	// Set absolute errors for solute state variables
# if defined(_OPENMP)
#  pragma omp parallel for
# endif
	for (i = num_hydrol_var; i < NumStateVar(); i++)
	{
		NV_Ith(abstol, i) = (realtype)transp_tol;
	}
#endif
}

void SolveCVode(double cputime, const ctrl_struct *ctrl, int *t, void *cvode_mem, N_Vector CV_Y)
{
	realtype        solvert;
	realtype        tout;
	pihm_t_struct   pihm_time;
	int             starttime;
	int             nextptr;
	int             cv_flag;
	double          progress;

	starttime = ctrl->starttime;
	nextptr = ctrl->tout[ctrl->cstep + 1];

	tout = (realtype)(nextptr - starttime);

	// Specifies the value of the independent variable t past which the solution is not to proceed
	cv_flag = CVodeSetStopTime(cvode_mem, tout);
	CheckCVodeFlag(cv_flag);

	cv_flag = CVode(cvode_mem, tout, CV_Y, &solvert, CV_NORMAL);
	CheckCVodeFlag(cv_flag);

	*t = roundi(solvert) + starttime;

	pihm_time = PIHMTime(*t);

	progress = ((double)ctrl->cstep + 1.0) / (double)ctrl->nstep;

	if (ctrl->cstep == 0)
	{
		pihm_printf(VL_NORMAL, "\n");
	}

	if (debug_mode)
	{
		pihm_printf(VL_NORMAL, "\033[1A\rStep = %s (t = %d)\n", pihm_time.str, *t);
		ProgressBar(progress);
	}
	else if (spinup_mode)
	{
		if (pihm_time.t % DAYINSEC == 0)
		{
			pihm_printf(VL_NORMAL, "\033[1A\rStep = %s\n", pihm_time.str);
			ProgressBar(progress);
		}
	}
	else if (pihm_time.t % 3600 == 0)
	{
		pihm_printf(VL_NORMAL, "\033[1A\rStep = %s (cputime %8.2f s)\n", pihm_time.str, cputime);
		ProgressBar(progress);
	}
}

void AdjCVodeMaxStep(void *cvode_mem, ctrl_struct *ctrl)
{
	// Variable CVODE max step (to reduce oscillations)
	long int        nst;
	long int        ncfn;
	long int        nni;
	static long int nst0;
	static long int ncfn0;
	static long int nni0;
	int             cv_flag;
	double          nsteps;
	double          nfails;
	double          niters;

	// Gets the cumulative number of internal steps taken by the solver (total so far)
	cv_flag = CVodeGetNumSteps(cvode_mem, &nst);
	CheckCVodeFlag(cv_flag);

	// Gets the number of nonlinear convergence failures that have occurred
	cv_flag = CVodeGetNumNonlinSolvConvFails(cvode_mem, &ncfn);
	CheckCVodeFlag(cv_flag);

	// Gets the number of nonlinear iterations performed
	cv_flag = CVodeGetNumNonlinSolvIters(cvode_mem, &nni);
	CheckCVodeFlag(cv_flag);

	nsteps = (double)(nst - nst0);
	nfails = (double)(ncfn - ncfn0) / nsteps;
	niters = (double)(nni - nni0) / nsteps;

	ctrl->maxstep /= (nfails > ctrl->nncfn || niters >= ctrl->nnimax) ? ctrl->decr : 1.0;

	ctrl->maxstep *= (nfails == 0.0 && niters <= ctrl->nnimin) ? ctrl->incr : 1.0;

	ctrl->maxstep = MIN(ctrl->maxstep, ctrl->stepsize);
	ctrl->maxstep = MAX(ctrl->maxstep, ctrl->stmin);

	// Updates the upper bound on the magnitude of the step size
	cv_flag = CVodeSetMaxStep(cvode_mem, (realtype)ctrl->maxstep);
	CheckCVodeFlag(cv_flag);

	nst0 = nst;
	ncfn0 = ncfn;
	nni0 = nni;
}
