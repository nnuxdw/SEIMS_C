#include "pihm.h"
#include "text.h"

using namespace std;
//// Global variables
int verbose_mode;
int debug_mode;
int append_mode ;
int  corr_mode;
int  spinup_mode;
int  fixed_length;
char*  project;
int  nelem;
int  nriver;
double flow_tolarence = 1e-6;
int curSubbasinId = -1;
double surfFlowExchange = 0.0;
double subsurfFlowExchange = 0.0;
double gwFlowExchange = 0.0;
#if defined(_OPENMP)
int             nthreads = 1;               // Default value
#endif
#if defined(_BGC_)
int             nsolute = 1;
#elif defined(_CYCLES_)
int             nsolute = 2;
#elif defined(_RT_)
int             nsolute;
#endif
#if defined(_BGC_)
int             first_balance;
#endif

#if defined(_OPENMP)
double          start_omp;
#else
clock_t         start;
#endif

PIHM::PIHM() :
	//m_TimeStep(-1), m_nCells(-1), m_CellWth(NODATA_VALUE), m_cellArea(NODATA_VALUE),
	//	m_nSubbsns(-1), m_inputSubbsnID(-1), m_subbsnID(nullptr),
	//	 m_surfRf(nullptr),
	//m_OL_Flow(nullptr), m_surfRftotal(nullptr)
	initial_flag(false)
{
	pihm_strc = (pihm_struct*)malloc(sizeof(pihm_struct));
	pihm_strc->SeimsVariables = new SeimsVariablesStruct();
	pihm_strc->SeimsVariables->m_TimeStep = -1;
	pihm_strc->SeimsVariables->m_nCells = -1;
	pihm_strc->SeimsVariables->m_CellWth = NODATA_VALUE;
	pihm_strc->SeimsVariables->m_cellArea = NODATA_VALUE;
	pihm_strc->SeimsVariables->m_nSubbsns = -1;
	pihm_strc->SeimsVariables->m_inputSubbsnID = -1;
	pihm_strc->SeimsVariables->m_subbsnID = nullptr;
	pihm_strc->SeimsVariables->m_surfRf = nullptr;
	pihm_strc->SeimsVariables->m_OL_Flow = nullptr;
	pihm_strc->SeimsVariables->m_pcp = nullptr;
	pihm_strc->SeimsVariables->m_surfRftotal = nullptr;
	pihm_strc->SeimsVariables->m_gwStorage = nullptr; 
	pihm_strc->SeimsVariables->m_subSurfRfVol = nullptr;
	pihm_strc->SeimsVariables->m_maxSoilLyrs = -1;
	
	nelem = -1;
}

PIHM::~PIHM() {
}

void PIHM::SetValue(const char* key, const float value) {
	string sk(key);
	if (StringMatch(sk, Tag_TimeStep)) {
		pihm_strc->SeimsVariables->m_TimeStep = CVT_INT(value);
	}
	else if (StringMatch(sk, Tag_CellSize)) {
		//pihm_strc->SeimsVariables->m_nCells = CVT_INT(value);
	}
	else if (StringMatch(sk, Tag_CellWidth)) pihm_strc->SeimsVariables->m_CellWth = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) pihm_strc->SeimsVariables->m_nSubbsns = CVT_INT(value);
	else if (StringMatch(sk, Tag_SubbasinId)) pihm_strc->SeimsVariables->m_inputSubbsnID = CVT_INT(value);
	else {
		throw ModelException(MID_IUH_OL, "SetValue", "Parameter " + sk + " does not exist.");
	}
}


void PIHM::SetValueByIndex(const char* key, int index, float value) {
}

void PIHM::Set1DData(const char* key, int n, float* data) {
	
	string sk(key);
	if (StringMatch(sk, VAR_SUBBSN)) {
		pihm_strc->SeimsVariables->m_subbsnID = data;
	}
	// HRU的地表水深
	else if (StringMatch(sk, VAR_SURU)) {
		pihm_strc->SeimsVariables->m_surfRf = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_PCP)) {
		pihm_strc->SeimsVariables->m_pcp = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_TMEAN)) {
		pihm_strc->SeimsVariables->m_meanTemp = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_TMAX)) {
		pihm_strc->SeimsVariables->m_maxTemp = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_TMIN)) {
		pihm_strc->SeimsVariables->m_minTemp = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, DataType_RelativeAirMoisture)) {
		pihm_strc->SeimsVariables->m_rhd = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_AHRU)) {
		pihm_strc->SeimsVariables->m_area = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_STREAM_LINK)) {
		pihm_strc->SeimsVariables->m_rchID = data;
	}
	else if (StringMatch(sk, VAR_WS)) {
		pihm_strc->SeimsVariables->m_WindSpeed = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, DataType_SolarRadiation)) {
		pihm_strc->SeimsVariables->m_SR = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_SURFRFTOTAL)) {
		pihm_strc->SeimsVariables->m_surfRftotal = data;
		CheckInputSize(MID_PIHM, key, n, pihm_strc->SeimsVariables->m_nCells);
	}
	else if (StringMatch(sk, VAR_SBGS)) {
		pihm_strc->SeimsVariables->m_gwStorage = data;
	}
	else if (StringMatch(sk, VAR_SOILLAYERS)) {
		pihm_strc->SeimsVariables->m_nSoilLyrs = data;
	}
	else if (StringMatch(sk, VAR_GW_SUBBASIN_AREA)) {
		pihm_strc->SeimsVariables->subbasin_area = data;
	}
	else {
		throw ModelException(MID_PIHM, "Set1DData", "Parameter " + sk + " does not exist.");
	}
}

void PIHM::Set2DData(const char* key, int nrows, int col, float** data) {
	string sk(key);
	pihm_strc->SeimsVariables->m_maxSoilLyrs = col;
	if (StringMatch(sk, VAR_SSRUVOL)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, col, pihm_strc->SeimsVariables->m_nCells, pihm_strc->SeimsVariables->m_maxSoilLyrs);
		pihm_strc->SeimsVariables->m_subSurfRfVol = data;
	}
	
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
	if (nullptr == pihm_strc->SeimsVariables->m_subSurfRfVol)
		Initialize2DArray(pihm_strc->SeimsVariables->m_nCells, pihm_strc->SeimsVariables->m_maxSoilLyrs, pihm_strc->SeimsVariables->m_subSurfRfVol, 0.f);
	// xdw，记录模型运行时间
	start_time = clock();
	time_t start_datetime = time(NULL);
	printf("Start time: %s", ctime(&start_datetime));

	pihm_tools = new PIHM_TOOLS_DEV();
	pihm_strc->PIHMToolData = new PIHMToolDataStruct();

	// Read args.txt
	args_file = new char[MAXSTRING];
	pihm_strc->PIHMToolData->args = (arg_struct*)malloc(sizeof(arg_struct));
	pihm_dir = new char[MAXSTRING];
	strcpy(pihm_dir, PIHM_DATA_PATH);
	sprintf(args_file, "%s/input/%s/%s", pihm_dir,PIHM_PROJECT,"args.txt");
	//strcpy(args_file, "G:\\program\\seims\\SEIMS_C\\data\\gongba_subbasin\\model_configs\\args.txt");
	pihm_tools->ReadArgumentsFromFile_dev(args_file, pihm_strc->PIHMToolData->args->argc, pihm_strc->PIHMToolData->args->argv);
	// Read command line arguments

	outputdir = new char[MAXSTRING];
	project = new char[MAXSTRING];
	//strcpy(project, PIHM_PROJECT);
	ParseCmdLineParam(pihm_strc->PIHMToolData->args->argc, pihm_strc->PIHMToolData->args->argv, pihm_dir, outputdir);

	// Read PIHM input files
	ReadAlloc(pihm_strc, pihm_dir);

	// Read final_downstream_file.txt
	final_downstream_file = new char[MAXSTRING];
	sprintf(final_downstream_file, "%s/input/%s/final_downstream_file.txt",pihm_dir, project);
	pihm_strc->PIHMToolData->hrus = new vector<hru_struct>();
	pihm_tools->ReadUpDownStreamFile_dev(final_downstream_file, pihm_strc->PIHMToolData->hrus);

	// Read select_hand_ids.txt 
	pihm_strc->PIHMToolData->hru_ids = new vector<int>();
	hru_ids_file = new char[MAXSTRING];
	sprintf(hru_ids_file, "%s/input/%s/select_hand_ids.txt", pihm_dir, project);
	pihm_tools->read_ids_from_file_dev(hru_ids_file, pihm_strc->PIHMToolData->hru_ids);

	// Read hru_tri_map_file.txt
	hru_tri_map_file = new char[MAXSTRING];
	sprintf(hru_tri_map_file, "%s/input/%s/hru_tri_map_file.txt", pihm_dir, project);
	pihm_strc->PIHMToolData->hru_tri_id_map = new map<int, int*>();
	pihm_tools->read_map_from_file(hru_tri_map_file, pihm_strc->PIHMToolData->hru_tri_id_map);

	all_adj_tri_ids_file = new char[MAXSTRING];
	sprintf(all_adj_tri_ids_file, "%s/input/%s/all_adj_tris_id.txt", pihm_dir, project);
	pihm_strc->PIHMToolData->all_adj_tris_ids = new int[nelem];
	for (int i = 0; i < nelem; i++)
	{
		pihm_strc->PIHMToolData->all_adj_tris_ids[i] = -1;
	}
	pihm_tools->read_adj_tri_ids_from_file(all_adj_tri_ids_file, pihm_strc->PIHMToolData->all_adj_tris_ids, &pihm_strc->PIHMToolData->len_all_adj_tris_ids);

	pihm_strc->SeimsMetros = new SeimsMeteoStruct();
	pihm_strc->SeimsMetros->pihm_pcp = new float[nelem]();
	pihm_strc->SeimsMetros->pihm_tmean = new float[nelem]();
	pihm_strc->SeimsMetros->pihm_ws = new float[nelem]();
	pihm_strc->SeimsMetros->pihm_rhd = new float[nelem]();
	pihm_strc->SeimsMetros->pihm_sr = new float[nelem]();
	pihm_strc->ExchangeData = new exchange_struct();
	// 数组index从1开始
	int exchange_len = pihm_strc->PIHMToolData->len_all_adj_tris_ids + 1;
	// 为节省存储空间，流量交换数组只为需要交换的三角形开辟空间
	pihm_strc->ExchangeData->elem_upstream_surfq = new double[exchange_len]();
	pihm_strc->ExchangeData->elem_upstream_subsurvol = new double[exchange_len]();
	pihm_strc->ExchangeData->elem_upstream_gwStorage = new double[exchange_len]();
	if (pihm_strc->PIHMToolData != nullptr && pihm_strc->PIHMToolData->hrus != nullptr) {
		for (hru_struct hru : *(pihm_strc->PIHMToolData->hrus)) {
			//cout << "HRU ID: " << hru.key << " down_type: " << hru.down_type << endl;
			if (hru.down_type == 1 && !hru.down_ids.empty()) {
				pihm_strc->PIHMToolData->upstream_hru_id_keys.insert(hru.key);
				pihm_strc->PIHMToolData->upstream_hru_down_tris_map[hru.key] = hru.down_ids;
			}
			else if (hru.down_type == 0) {
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
	// 存储、输出数据
	pihm_output_file = new char[MAXSTRING];
	pihm_strc->PIHMData = new PIHMDataStruct();
	initial_flag = true;
	finish_times = 10;
	counter = 0;

	// Initialize CVODE state variables 三角形数量*3 + 河流数量
	CV_Y = N_VNew(NumStateVar());
	if (CV_Y == NULL)
	{
		pihm_printf(VL_ERROR, "Error creating CVODE state variable vector.\n");
		pihm_exit(EXIT_FAILURE);
	}
	// Initialize PIHM structure
	Initialize(pihm_strc, CV_Y, &cvode_mem, pihm_strc->SeimsVariables->m_TimeStep);


	// Create output directory
	CreateOutputDir(outputdir);

	// Create output structures
#if defined(_CYCLES_)
	MapOutput(outputdir, pihm->ctrl.prtvrbl, pihm->croptbl, pihm->elem, pihm->river, &pihm->print);
#elif defined(_RT_)
	MapOutput(outputdir, pihm->ctrl.prtvrbl, pihm->chemtbl, &pihm->rttbl, pihm->elem, pihm->river, &pihm->print);
#else
	MapOutput(outputdir, pihm_strc->ctrl.prtvrbl, pihm_strc->elem, pihm_strc->river, &pihm_strc->print);
#endif

	InitOutputFiles(outputdir, pihm_strc->ctrl.waterbal, pihm_strc->ctrl.ascii, &pihm_strc->print);

	pihm_printf(VL_VERBOSE, "\n\nSolving ODE system ... \n\n");

	// Set solver parameters///////////////////////////////////////////////
	SetCVodeParam(pihm_strc, cvode_mem, &sun_ls, CV_Y);

	// 这里开始es->surf有可能是负值
#if defined(_BGC_)
	first_balance = 1;
#endif

	// Run PIHM
#if defined(_OPENMP)
	start_omp = omp_get_wtime();
#else
	start = clock();
#endif

}


int PIHM::Execute() {
#if defined(_OPENMP)
	RunTime(start_omp, &cputime, &cputime_dt);
#else
	RunTime(start, &cputime, &cputime_dt);
#endif
	if (!initial_flag) {
		InitialOutputs();
	}
	++counter;

	// xiaodw, 计算PIHM时间步长及当前时间
	CalcPIHMSteps(&pihm_strc->ctrl, pihm_strc->SeimsVariables->m_TimeStep, counter,cur_sim_time_ptr, last_sim_time_ptr);


	int time = pihm_strc->ctrl.tout[0];
	pihm_t_struct   pihm_time = PIHMTime(time);
	cout << "current pihm time: " << pihm_time.str << endl;
	ctrl_struct*  ctrl = &pihm_strc->ctrl;

	// 初始化输出的变量
	const int adj_tri_number = pihm_strc->PIHMToolData->len_all_adj_tris_ids;
	pihm_strc->PIHMData->timeseries = new int[ctrl->nstep]();
	// 上游地表、壤中流、地下水输入
	pihm_strc->PIHMData->elem_upstream_surfq = new double*[ctrl->nstep];
	pihm_strc->PIHMData->elem_upstream_subsurq = new double*[ctrl->nstep];
	pihm_strc->PIHMData->elem_upstream_gwq = new double*[ctrl->nstep];
	// 地表水位
	pihm_strc->PIHMData->elem_sufh = new double*[ctrl->nstep];
	// 地下水位
	pihm_strc->PIHMData->elem_gwh = new double*[ctrl->nstep];
	for (int i = 0; i < ctrl->nstep; i++) {
		pihm_strc->PIHMData->elem_upstream_surfq[i] = new double[adj_tri_number]();
		pihm_strc->PIHMData->elem_upstream_subsurq[i] = new double[adj_tri_number]();
		pihm_strc->PIHMData->elem_upstream_gwq[i] = new double[adj_tri_number]();
		pihm_strc->PIHMData->elem_sufh[i] = new double[adj_tri_number]();
		pihm_strc->PIHMData->elem_gwh[i] = new double[adj_tri_number]();
		pihm_strc->PIHMData->timeseries[i] = 0;

	}

	
	for (ctrl->cstep = 0; ctrl->cstep < ctrl->nstep; ctrl->cstep++){
		// Run PIHM time step///////////////////////////////////
		RUN_PIHM(cputime, pihm_strc, cvode_mem, CV_Y, pihm_strc->SeimsVariables, pihm_strc->SeimsMetros);

		// Adjust CVODE max step to reduce oscillation
		AdjCVodeMaxStep(cvode_mem, &pihm_strc->ctrl);

		// Print CVODE performance and statistics
		//debug_mode = 1;
		debug_mode = 0;
		if (debug_mode)
		{
			PrintPerf(ctrl->tout[ctrl->cstep + 1], ctrl->starttime, cputime_dt, cputime, ctrl->maxstep,
				pihm_strc->print.cvodeperf_file, cvode_mem);
		}

		// Write init files
		if (ctrl->write_ic)
		{
			PrintInit(outputdir, ctrl->tout[ctrl->cstep + 1], ctrl->starttime, ctrl->endtime,
				ctrl->prtvrbl[IC_CTRL], pihm_strc->elem, pihm_strc->river);
		}
	}
	
	PostExcute();

	//if (counter >= finish_times){
	//	PostExcute();
	//}
	ctrl->cstep = 0;
    return 0;
}

void PIHM::PostExcute() {
	#if defined(_BGC_)
		if (ctrl->write_bgc_restart)
		{
			WriteBgcIc(outputdir, pihm->elem, pihm->river);
		}
	#endif

	#if defined(_CYCLES_)
		if (ctrl->write_cycles_restart)
		{
			WriteCyclesIc(outputdir, pihm->elem);
		}
	#endif

	#if defined(_RT_)
		if (ctrl->write_rt_restart)
		{
			WriteRtIc(outputdir, pihm->chemtbl, &pihm->rttbl, pihm->elem);
		}
	#endif
	

	if (debug_mode)
	{
		PrintCVodeFinalStats(cvode_mem);
	}

	#if defined(_STATISTIC_TIME_)
	pihm->ptime_calculator->t10 = clock();
	pihm->ptime_calculator->other_time += ((double)(pihm->ptime_calculator->t10 - pihm->ptime_calculator->t9)) / CLOCKS_PER_SEC;
	// 打印耗时
	print_time_struct(pihm->ptime_calculator);
	#endif

	sprintf(pihm_output_file, "%s/output/%s/pihm_output_gwh.txt", pihm_dir, project);
	write_struct_to_file(pihm_output_file, pihm_strc->PIHMData->timeseries, pihm_strc->PIHMData->elem_gwh, pihm_strc->PIHMToolData->all_adj_tris_ids, pihm_strc->ctrl.nstep, pihm_strc->PIHMToolData->len_all_adj_tris_ids);
	sprintf(pihm_output_file, "%s/output/%s/pihm_output_sufh.txt", pihm_dir, project);
	write_struct_to_file(pihm_output_file, pihm_strc->PIHMData->timeseries, pihm_strc->PIHMData->elem_sufh, pihm_strc->PIHMToolData->all_adj_tris_ids, pihm_strc->ctrl.nstep, pihm_strc->PIHMToolData->len_all_adj_tris_ids);
	sprintf(pihm_output_file, "%s/output/%s/pihm_output_ex_gwq.txt", pihm_dir, project);
	write_struct_to_file(pihm_output_file, pihm_strc->PIHMData->timeseries, pihm_strc->PIHMData->elem_upstream_gwq, pihm_strc->PIHMToolData->all_adj_tris_ids, pihm_strc->ctrl.nstep, pihm_strc->PIHMToolData->len_all_adj_tris_ids);
	sprintf(pihm_output_file, "%s/output/%s/pihm_output_ex_subsurq.txt", pihm_dir, project);
	write_struct_to_file(pihm_output_file, pihm_strc->PIHMData->timeseries, pihm_strc->PIHMData->elem_upstream_subsurq, pihm_strc->PIHMToolData->all_adj_tris_ids, pihm_strc->ctrl.nstep, pihm_strc->PIHMToolData->len_all_adj_tris_ids);
	sprintf(pihm_output_file, "%s/output/%s/pihm_output_ex_surfq.txt", pihm_dir, project);
	write_struct_to_file(pihm_output_file, pihm_strc->PIHMData->timeseries,pihm_strc->PIHMData->elem_upstream_surfq, pihm_strc->PIHMToolData->all_adj_tris_ids, pihm_strc->ctrl.nstep, pihm_strc->PIHMToolData->len_all_adj_tris_ids);
	// Free memory
	N_VDestroy(CV_Y);

	// Free integrator memory

	CVodeFree(&cvode_mem);
	SUNLinSolFree(sun_ls);
	FreeMem(pihm_strc);
	free(pihm_strc);

	pihm_printf(VL_BRIEF, "Simulation completed.\n");
	// xdw，记录模型运行结束时间
	time_t end_datetime = time(NULL);
	printf("End time: %s", ctime(&end_datetime));
	end_time = clock();
	elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
	int hours = (int)elapsed_time / 3600;
	int minutes = (int)(elapsed_time - hours * 3600) / 60;
	int seconds = (int)elapsed_time % 60;
	printf("Elapsed time: %d hours, %d minutes, %d seconds.\n", hours, minutes, seconds);

	return ;
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





//void PIHM(double cputime, pihm_struct *pihm, void *cvode_mem, N_Vector CV_Y, SeimsVariablesStruct * SeimsVariables)
//{
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t2 = clock();
//#endif
//	int             t;
//#if defined(_RT_)
//	const int       SPECIATION_STEP = 3600;
//#endif
//
//	t = pihm->ctrl.tout[pihm->ctrl.cstep];
//
//	// Apply boundary conditions
//#if defined(_RT_)
//	ApplyBc(t, &pihm->rttbl, &pihm->forc, pihm->elem, pihm->river);
//#else
//	ApplyBc(t, &pihm->forc, pihm->elem, pihm->river);
//#endif
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t3 = clock();
//	pihm->ptime_calculator->applybc_time += ((double)(pihm->ptime_calculator->t3 - pihm->ptime_calculator->t2)) / CLOCKS_PER_SEC;
//#endif
//	// Apply forcing and simulate land surface processes
//	if ((t - pihm->ctrl.starttime) % pihm->ctrl.etstep == 0)
//	{
//		// Apply forcing
//#if defined(_RT_)
//		ApplyForcing(t, pihm->ctrl.rad_mode, &pihm->siteinfo, &pihm->rttbl, &pihm->forc, pihm->elem);
//#elif defined(_NOAH_)
//		ApplyForcing(t, pihm->ctrl.rad_mode, &pihm->siteinfo, &pihm->forc, pihm->elem);
//#else
//		// 气象强迫和叶面积指数
//		//ApplyForcing(t, &pihm->forc, pihm->elem,SeimsVariables->m_pcp);
//#endif
//
//#if defined(_NOAH_)
//		// Calculate surface energy balance
//		Noah((double)pihm->ctrl.etstep, &pihm->lctbl, &pihm->calib, pihm->elem);
//#else
//		// Calculate Interception storage and ET
//		IntcpSnowEt(t, (double)pihm->ctrl.etstep, &pihm->calib, pihm->elem);
//#endif
//
//		// Update print variables for land surface step variables
//		UpdatePrintVar(pihm->print.nprint, LS_STEP, pihm->print.varctrl);
//	}
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t4 = clock();
//	pihm->ptime_calculator->landsurface_time += ((double)(pihm->ptime_calculator->t4 - pihm->ptime_calculator->t3)) / CLOCKS_PER_SEC;
//#endif
//#if defined(_RT_)
//	// Reaction
//	if (pihm->rttbl.transpt_flag == KIN_REACTION)
//	{
//		if ((t - pihm->ctrl.starttime) % pihm->ctrl.AvgScl == 0)
//		{
//			Reaction((double)pihm->ctrl.AvgScl, pihm->chemtbl, pihm->kintbl, &pihm->rttbl, pihm->elem);
//		}
//	}
//#endif
//
//#if defined(_CYCLES_)
//	if ((t - pihm->ctrl.starttime) % DAYINSEC == 0)
//	{
//		Cycles(t, &pihm->co2ctrl, &pihm->forc, pihm->elem);
//
//		// Update print variables for CN (daily) step variables
//		UpdatePrintVar(pihm->print.nprint, CN_STEP, pihm->print.varctrl);
//	}
//#endif
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t5 = clock();
//	pihm->ptime_calculator->reaction_time += ((double)(pihm->ptime_calculator->t5 - pihm->ptime_calculator->t4)) / CLOCKS_PER_SEC;
//#endif
//	// Solve PIHM hydrology ODE using CVode 关键方法
//	SolveCVode(cputime, &pihm->ctrl, &t, cvode_mem, CV_Y);
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t6 = clock();
//	pihm->ptime_calculator->solvecvode_time += ((double)(pihm->ptime_calculator->t6 - pihm->ptime_calculator->t5)) / CLOCKS_PER_SEC;
//#endif
//	// Use mass balance to calculate model fluxes or variables
//	UpdateVar((double)pihm->ctrl.stepsize, pihm->elem, pihm->river, CV_Y);
//
//
//#if defined(_NOAH_)
//#if defined(_STATISTIC_TIME_)
//	NoahHydrol((double)pihm->ctrl.stepsize, pihm->elem, pihm->ptime_calculator);
//#else
//	NoahHydrol((double)pihm->ctrl.stepsize, pihm->elem);
//#endif
//#endif
//
//	// Update print variables for hydrology step variables
//	UpdatePrintVar(pihm->print.nprint, HYDROL_STEP, pihm->print.varctrl);
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t7 = clock();
//	pihm->ptime_calculator->noahhydro_time += ((double)(pihm->ptime_calculator->t7 - pihm->ptime_calculator->t6)) / CLOCKS_PER_SEC;
//#endif
//#if defined(_RT_)
//	// Update chemical concentrations
//	if (pihm->rttbl.transpt_flag == KIN_REACTION)
//	{
//		if ((t - pihm->ctrl.starttime) % SPECIATION_STEP == 0)
//		{
//			// Speciation
//			RiverSpeciation(pihm->chemtbl, &pihm->rttbl, pihm->river);
//		}
//	}
//	else
//	{
//		UpdatePrimConc(&pihm->rttbl, pihm->elem, pihm->river);
//	}
//
//	UpdatePrintVar(pihm->print.nprint, RT_STEP, pihm->print.varctrl);
//#endif
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t8 = clock();
//	pihm->ptime_calculator->chemical_time += ((double)(pihm->ptime_calculator->t8 - pihm->ptime_calculator->t7)) / CLOCKS_PER_SEC;
//#endif
//#if defined(_DAILY_)
//	DailyVar(t, pihm->ctrl.starttime, pihm->elem);
//
//	// Daily timestep modules
//	if ((t - pihm->ctrl.starttime) % DAYINSEC == 0)
//	{
//# if defined(_BGC_)
//		// Daily BGC processes
//		DailyBgc(t - DAYINSEC, pihm);
//
//		// Update print variables for CN (daily) step variables
//		UpdatePrintVar(pihm->print.nprint, CN_STEP, pihm->print.varctrl);
//# endif
//
//		// Initialize daily structures
//		InitDailyStruct(pihm->elem);
//	}
//#endif
//
//	// Print outputs
//	// Print water balance
//	if (pihm->ctrl.waterbal)
//	{
//		PrintWaterBalance(t, pihm->ctrl.starttime, pihm->ctrl.stepsize, pihm->elem, pihm->river,
//			pihm->print.watbal_file);
//	}
//
//	// Print binary and txt output files
//	PrintData(pihm->print.nprint, t, t - pihm->ctrl.starttime, pihm->ctrl.ascii, pihm->print.varctrl);
//#if defined(_STATISTIC_TIME_)
//	pihm->ptime_calculator->t9 = clock();
//	pihm->ptime_calculator->dailybgc_time += ((double)(pihm->ptime_calculator->t9 - pihm->ptime_calculator->t8)) / CLOCKS_PER_SEC;
//#endif
//}
