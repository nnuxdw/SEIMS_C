#include "pihm.h"
#include "text.h"

PIHM::PIHM() :
    m_nCells(-1) {
}

PIHM::~PIHM() {
}

void PIHM::SetValue(const char* key, float value) {
}

void PIHM::SetValueByIndex(const char* key, int index, float value) {
}

void PIHM::Set1DData(const char* key, int n, float* data) {
}

void PIHM::Set2DData(const char* key, int n, int col, float** data) {
}

void PIHM::SetReaches(clsReaches* rches) {
}

void PIHM::SetSubbasins(clsSubbasins* subbsns) {
}

void PIHM::SetScenario(Scenario* sce) {
}

bool PIHM::CheckInputData() {
    return true;
}

void PIHM::InitialOutputs() {
	memset(outputdir, 0, MAXSTRING);
	// Read command line arguments
	ParseCmdLineParam(argc, argv, outputdir);
	pihm = (pihm_struct*)malloc(sizeof(pihm_struct));
}



int PIHM::Execute() {
	cout << "hello pihm..." << endl;
    return 0;
}

TimeStepType PIHM::GetTimeStepType() {
    return TIMESTEP_HILLSLOPE;
}


void PIHM::GetValue(const char* key, float* value) {
}

void PIHM::Get1DData(const char* key, int* n, float** data) {
}

void PIHM::Get2DData(const char* key, int* n, int* col, float*** data) {
}

void PIHM::Initialize() {

}

void PIHM(double cputime, pihm_struct *pihm, void *cvode_mem, N_Vector CV_Y)
{
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t2 = clock();
#endif
	int             t;
#if defined(_RT_)
	const int       SPECIATION_STEP = 3600;
#endif

	t = pihm->ctrl.tout[pihm->ctrl.cstep];

	// Apply boundary conditions
#if defined(_RT_)
	ApplyBc(t, &pihm->rttbl, &pihm->forc, pihm->elem, pihm->river);
#else
	ApplyBc(t, &pihm->forc, pihm->elem, pihm->river);
#endif
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t3 = clock();
	pihm->ptime_calculator->applybc_time += ((double)(pihm->ptime_calculator->t3 - pihm->ptime_calculator->t2)) / CLOCKS_PER_SEC;
#endif
	// Apply forcing and simulate land surface processes
	if ((t - pihm->ctrl.starttime) % pihm->ctrl.etstep == 0)
	{
		// Apply forcing
#if defined(_RT_)
		ApplyForcing(t, pihm->ctrl.rad_mode, &pihm->siteinfo, &pihm->rttbl, &pihm->forc, pihm->elem);
#elif defined(_NOAH_)
		ApplyForcing(t, pihm->ctrl.rad_mode, &pihm->siteinfo, &pihm->forc, pihm->elem);
#else
		// 气象强迫和叶面积指数
		ApplyForcing(t, &pihm->forc, pihm->elem);
#endif

#if defined(_NOAH_)
		// Calculate surface energy balance
		Noah((double)pihm->ctrl.etstep, &pihm->lctbl, &pihm->calib, pihm->elem);
#else
		// Calculate Interception storage and ET
		IntcpSnowEt(t, (double)pihm->ctrl.etstep, &pihm->calib, pihm->elem);
#endif

		// Update print variables for land surface step variables
		UpdatePrintVar(pihm->print.nprint, LS_STEP, pihm->print.varctrl);
	}
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t4 = clock();
	pihm->ptime_calculator->landsurface_time += ((double)(pihm->ptime_calculator->t4 - pihm->ptime_calculator->t3)) / CLOCKS_PER_SEC;
#endif
#if defined(_RT_)
	// Reaction
	if (pihm->rttbl.transpt_flag == KIN_REACTION)
	{
		if ((t - pihm->ctrl.starttime) % pihm->ctrl.AvgScl == 0)
		{
			Reaction((double)pihm->ctrl.AvgScl, pihm->chemtbl, pihm->kintbl, &pihm->rttbl, pihm->elem);
		}
	}
#endif

#if defined(_CYCLES_)
	if ((t - pihm->ctrl.starttime) % DAYINSEC == 0)
	{
		Cycles(t, &pihm->co2ctrl, &pihm->forc, pihm->elem);

		// Update print variables for CN (daily) step variables
		UpdatePrintVar(pihm->print.nprint, CN_STEP, pihm->print.varctrl);
	}
#endif
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t5 = clock();
	pihm->ptime_calculator->reaction_time += ((double)(pihm->ptime_calculator->t5 - pihm->ptime_calculator->t4)) / CLOCKS_PER_SEC;
#endif
	// Solve PIHM hydrology ODE using CVode 关键方法
	SolveCVode(cputime, &pihm->ctrl, &t, cvode_mem, CV_Y);
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t6 = clock();
	pihm->ptime_calculator->solvecvode_time += ((double)(pihm->ptime_calculator->t6 - pihm->ptime_calculator->t5)) / CLOCKS_PER_SEC;
#endif
	// Use mass balance to calculate model fluxes or variables
	UpdateVar((double)pihm->ctrl.stepsize, pihm->elem, pihm->river, CV_Y);


#if defined(_NOAH_)
#if defined(_STATISTIC_TIME_)
	NoahHydrol((double)pihm->ctrl.stepsize, pihm->elem, pihm->ptime_calculator);
#else
	NoahHydrol((double)pihm->ctrl.stepsize, pihm->elem);
#endif
#endif

	// Update print variables for hydrology step variables
	UpdatePrintVar(pihm->print.nprint, HYDROL_STEP, pihm->print.varctrl);
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t7 = clock();
	pihm->ptime_calculator->noahhydro_time += ((double)(pihm->ptime_calculator->t7 - pihm->ptime_calculator->t6)) / CLOCKS_PER_SEC;
#endif
#if defined(_RT_)
	// Update chemical concentrations
	if (pihm->rttbl.transpt_flag == KIN_REACTION)
	{
		if ((t - pihm->ctrl.starttime) % SPECIATION_STEP == 0)
		{
			// Speciation
			RiverSpeciation(pihm->chemtbl, &pihm->rttbl, pihm->river);
		}
	}
	else
	{
		UpdatePrimConc(&pihm->rttbl, pihm->elem, pihm->river);
	}

	UpdatePrintVar(pihm->print.nprint, RT_STEP, pihm->print.varctrl);
#endif
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t8 = clock();
	pihm->ptime_calculator->chemical_time += ((double)(pihm->ptime_calculator->t8 - pihm->ptime_calculator->t7)) / CLOCKS_PER_SEC;
#endif
#if defined(_DAILY_)
	DailyVar(t, pihm->ctrl.starttime, pihm->elem);

	// Daily timestep modules
	if ((t - pihm->ctrl.starttime) % DAYINSEC == 0)
	{
# if defined(_BGC_)
		// Daily BGC processes
		DailyBgc(t - DAYINSEC, pihm);

		// Update print variables for CN (daily) step variables
		UpdatePrintVar(pihm->print.nprint, CN_STEP, pihm->print.varctrl);
# endif

		// Initialize daily structures
		InitDailyStruct(pihm->elem);
	}
#endif

	// Print outputs
	// Print water balance
	if (pihm->ctrl.waterbal)
	{
		PrintWaterBalance(t, pihm->ctrl.starttime, pihm->ctrl.stepsize, pihm->elem, pihm->river,
			pihm->print.watbal_file);
	}

	// Print binary and txt output files
	PrintData(pihm->print.nprint, t, t - pihm->ctrl.starttime, pihm->ctrl.ascii, pihm->print.varctrl);
#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t9 = clock();
	pihm->ptime_calculator->dailybgc_time += ((double)(pihm->ptime_calculator->t9 - pihm->ptime_calculator->t8)) / CLOCKS_PER_SEC;
#endif
}
