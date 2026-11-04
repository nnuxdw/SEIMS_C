//#include "pihm.h"
//#include <time.h>
//#include <stdio.h>
//
//// Global variables
////int             verbose_mode;
////int             debug_mode;
////int             append_mode;
////int             corr_mode;
////int             spinup_mode;
////int             fixed_length;
////char            project[MAXSTRING];
////int             nelem;
////int             nriver;
//#if defined(_OPENMP)
//int             nthreads = 1;               // Default value
//#endif
//#if defined(_BGC_)
//int             nsolute = 1;
//#elif defined(_CYCLES_)
//int             nsolute = 2;
//#elif defined(_RT_)
//int             nsolute;
//#endif
//#if defined(_BGC_)
//int             first_balance;
//#endif
//
//int main(int argc, char *argv[])
//{
//	// xdw，记录模型运行时间
//	clock_t start_time, end_time;
//	double elapsed_time;
//	start_time = clock();
//	time_t start_datetime = time(NULL);
//	printf("Start time: %s", ctime(&start_datetime));
//
//	char            outputdir[MAXSTRING];
//	char pihm_dir[MAXSTRING];
//	pihm_struct     *pihm_strc;
//	ctrl_struct    *ctrl;
//	N_Vector        CV_Y;
//	void           *cvode_mem;
//	SUNLinearSolver sun_ls;
//#if defined(_OPENMP)
//	double          start_omp;
//#else
//	clock_t         start;
//#endif
//	double          cputime, cputime_dt;    // Time cpu duration
//
//#if defined(unix) || defined(__unix__) || defined(__unix)
//	feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
//#endif
//
//#if defined(_OPENMP)
//	// Set the number of threads to use
//	nthreads = omp_get_max_threads();
//#endif
//
//#if defined(_DEBUG_)
//	// When in debug mode, print PID and host name to a text file for gdb to attach to
//	char            hostname[MAXSTRING];
//	FILE           *fp;
//
//	gethostname(hostname, MAXSTRING);
//	fp = fopen("debug.txt", "w");
//	fprintf(fp, "pid %d at %s\n", getpid(), hostname);
//	fflush(fp);
//	fclose(fp);
//#endif
//
//
//	memset(outputdir, 0, MAXSTRING);
//	strcpy(pihm_dir, "");
//
//	// Read command line arguments
//	ParseCmdLineParam(argc, argv, pihm_dir,outputdir);
//
//	// Print AscII art
//	StartupScreen();
//
//	// Allocate memory for model data structure
//	//pihm_strc = (pihm_struct)malloc(sizeof(*pihm_strc));
//	pihm_strc = (pihm_struct*)malloc(sizeof(pihm_struct));
//#if defined(_STATISTIC_TIME_)
//	struct time_struct time_calculator;
//	pihm_strc->ptime_calculator = &time_calculator;
//	init_time_struct(pihm_strc->ptime_calculator);
//#endif
//	//char pihm_dir[MAXSTRING];
//	// Read PIHM input files
//	ReadAlloc(pihm_strc, pihm_dir);
//
//	// Initialize CVODE state variables 三角形数量*3 + 河流数量
//	CV_Y = N_VNew(NumStateVar());
//	if (CV_Y == NULL)
//	{
//		pihm_printf(VL_ERROR, "Error creating CVODE state variable vector.\n");
//		pihm_exit(EXIT_FAILURE);
//	}
//
//	// Initialize PIHM structure
//	Initialize(pihm_strc, CV_Y, &cvode_mem,1);
//
//	// Create output directory
//	CreateOutputDir(outputdir);
//
//	// Create output structures
//#if defined(_CYCLES_)
//	MapOutput(outputdir, pihm_strc->ctrl.prtvrbl, pihm_strc->croptbl, pihm_strc->elem, pihm_strc->river, &pihm_strc->print);
//#elif defined(_RT_)
//	MapOutput(outputdir, pihm_strc->ctrl.prtvrbl, pihm_strc->chemtbl, &pihm_strc->rttbl, pihm_strc->elem, pihm_strc->river, &pihm_strc->print);
//#else
//	MapOutput(outputdir, pihm_strc->ctrl.prtvrbl, pihm_strc->elem, pihm_strc->river, &pihm_strc->print);
//#endif
//
//	// Backup input files
//#if !defined(_MSC_VER)
//	if (!append_mode)
//	{
//		BackupInput(outputdir, &pihm_strc->filename);
//	}
//#endif
//
//	InitOutputFiles(outputdir, pihm_strc->ctrl.waterbal, pihm_strc->ctrl.ascii, &pihm_strc->print);
//
//	pihm_printf(VL_VERBOSE, "\n\nSolving ODE system ... \n\n");
//
//	// Set solver parameters///////////////////////////////////////////////
//	SetCVodeParam(pihm_strc, cvode_mem, &sun_ls, CV_Y);
//	// 这里开始es->surf有可能是负值
//#if defined(_BGC_)
//	first_balance = 1;
//#endif
//#if defined(_STATISTIC_TIME_)
//	// xdw, calculate time
//	pihm_strc->ptime_calculator->t1 = clock();
//	pihm_strc->ptime_calculator->init_time = ((double)(pihm_strc->ptime_calculator->t1 - start_time)) / CLOCKS_PER_SEC;
//#endif
//
//	// Run PIHM
//#if defined(_OPENMP)
//	start_omp = omp_get_wtime();
//#else
//	start = clock();
//#endif
//
//	ctrl = &pihm_strc->ctrl;
//
//	if (spinup_mode)
//	{
//		//Spinup(pihm_strc, CV_Y, cvode_mem, &sun_ls, SeimsVariables);
//
//		// In spin-up mode, initial conditions are always printed
//		PrintInit(outputdir, ctrl->endtime, ctrl->starttime, ctrl->endtime, ctrl->prtvrbl[IC_CTRL], pihm_strc->elem,
//			pihm_strc->river);
//
//#if defined(_BGC_)
//		WriteBgcIc(outputdir, pihm_strc->elem, pihm_strc->river);
//#endif
//
//#if defined(_CYCLES_)
//		WriteCyclesIc(outputdir, pihm_strc->elem);
//#endif
//
//#if defined(_RT_)
//		WriteRtIc(outputdir, pihm_strc->chemtbl, &pihm_strc->rttbl, pihm_strc->elem);
//#endif
//#if defined(_STATISTIC_TIME_)
//		// xdw, calculate time
//		pihm_strc->ptime_calculator->t2 = clock();
//		pihm_strc->ptime_calculator->spinup_time = ((double)(pihm_strc->ptime_calculator->t2 - pihm_strc->ptime_calculator->t1)) / CLOCKS_PER_SEC;
//#endif
//	}
//	else
//	{
//		for (ctrl->cstep = 0; ctrl->cstep < ctrl->nstep; ctrl->cstep++)
//			//for (ctrl->cstep = 0; ctrl->cstep < 100; ctrl->cstep++)
//		{
//#if defined(_OPENMP)
//			RunTime(start_omp, &cputime, &cputime_dt);
//#else
//			RunTime(start, &cputime, &cputime_dt);
//#endif
//
//			// Run PIHM time step///////////////////////////////////
//			//PIHM(cputime, pihm_strc, cvode_mem, CV_Y);
//
//			// Adjust CVODE max step to reduce oscillation
//			AdjCVodeMaxStep(cvode_mem, &pihm_strc->ctrl);
//
//			// Print CVODE performance and statistics
//			if (debug_mode)
//			{
//				PrintPerf(ctrl->tout[ctrl->cstep + 1], ctrl->starttime, cputime_dt, cputime, ctrl->maxstep,
//					pihm_strc->print.cvodeperf_file, cvode_mem);
//			}
//
//			// Write init files
//			if (ctrl->write_ic)
//			{
//				PrintInit(outputdir, ctrl->tout[ctrl->cstep + 1], ctrl->starttime, ctrl->endtime,
//					ctrl->prtvrbl[IC_CTRL], pihm_strc->elem, pihm_strc->river);
//			}
//
//		}
//
//#if defined(_BGC_)
//		if (ctrl->write_bgc_restart)
//		{
//			WriteBgcIc(outputdir, pihm_strc->elem, pihm_strc->river);
//		}
//#endif
//
//#if defined(_CYCLES_)
//		if (ctrl->write_cycles_restart)
//		{
//			WriteCyclesIc(outputdir, pihm_strc->elem);
//		}
//#endif
//
//#if defined(_RT_)
//		if (ctrl->write_rt_restart)
//		{
//			WriteRtIc(outputdir, pihm_strc->chemtbl, &pihm_strc->rttbl, pihm_strc->elem);
//		}
//#endif
//	}
//
//	if (debug_mode)
//	{
//		PrintCVodeFinalStats(cvode_mem);
//	}
//#if defined(_STATISTIC_TIME_)
//	pihm_strc->ptime_calculator->t10 = clock();
//	pihm_strc->ptime_calculator->other_time += ((double)(pihm_strc->ptime_calculator->t10 - pihm_strc->ptime_calculator->t9)) / CLOCKS_PER_SEC;
//	// 打印耗时
//	print_time_struct(pihm_strc->ptime_calculator);
//#endif
//	// Free memory
//	N_VDestroy(CV_Y);
//
//	// Free integrator memory
//
//	CVodeFree(&cvode_mem);
//	SUNLinSolFree(sun_ls);
//	FreeMem(pihm_strc);
//	free(pihm_strc);
//
//	pihm_printf(VL_BRIEF, "Simulation completed.\n");
//	// xdw，记录模型运行结束时间
//	time_t end_datetime = time(NULL);
//	printf("End time: %s", ctime(&end_datetime));
//	end_time = clock();
//	elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
//	int hours = (int)elapsed_time / 3600;
//	int minutes = (int)(elapsed_time - hours * 3600) / 60;
//	int seconds = (int)elapsed_time % 60;
//	printf("Elapsed time: %d hours, %d minutes, %d seconds.\n", hours, minutes, seconds);
//
//
//
//	return EXIT_SUCCESS;
//}
