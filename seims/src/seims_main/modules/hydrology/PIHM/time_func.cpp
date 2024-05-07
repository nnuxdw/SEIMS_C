#include "pihm.h"
pihm_t_struct PIHMTime(int t)
{
	pihm_t_struct   pihm_time;
	struct tm      *timestamp;
	time_t          rawtime;

	rawtime = (time_t)t;
	timestamp = gmtime(&rawtime);

	pihm_time.t = t;
	pihm_time.year = timestamp->tm_year + 1900;
	pihm_time.month = timestamp->tm_mon + 1;
	pihm_time.day = timestamp->tm_mday;
	pihm_time.hour = timestamp->tm_hour;
	pihm_time.minute = timestamp->tm_min;
	strftime(pihm_time.str, 17, "%Y-%m-%d %H:%M", timestamp);
	strftime(pihm_time.strshort, 13, "%Y%m%d%H%M", timestamp);

	return pihm_time;
}

int StrTime(const char timestr[])
{
	struct tm      *timestamp;
	int             t = 0;

	timestamp = (struct tm *)malloc(sizeof(struct tm));

	switch (strlen(timestr))
	{
	case 4:
		if (sscanf(timestr, "%d", &timestamp->tm_year) != 1)
		{
			t = BADVAL;
			pihm_printf(VL_ERROR, "Error converting from time string to time.\n");
		}
		else
		{
			timestamp->tm_year -= 1900;
			timestamp->tm_mon = 0;
			timestamp->tm_mday = 1;
			timestamp->tm_hour = 0;
			timestamp->tm_min = 0;
			timestamp->tm_sec = 0;
			timestamp->tm_isdst = 0;
		}
		break;
	case 16:
		if (sscanf(timestr, "%d-%d-%d %d:%d", &timestamp->tm_year, &timestamp->tm_mon, &timestamp->tm_mday,
			&timestamp->tm_hour, &timestamp->tm_min) != 5)
		{
			t = BADVAL;
			pihm_printf(VL_ERROR, "Error converting from time string to time.\n");
		}
		else
		{
			timestamp->tm_year -= 1900;
			timestamp->tm_mon--;
			timestamp->tm_sec = 0;
			timestamp->tm_isdst = 0;
		}
		break;
	default:
		t = BADVAL;
		pihm_printf(VL_ERROR, "Error converting from time string to time.\n");
		break;
	}

	t = (t == BADVAL) ? BADVAL : (int)timegm(timestamp);

	free(timestamp);

	return t;
}

#if defined(_STATISTIC_TIME_)
void init_time_struct(struct time_struct* time_calculator) {
	time_calculator->init_time = 0.0;
	time_calculator->spinup_time = 0.0;
	time_calculator->applybc_time = 0.0;
	time_calculator->landsurface_time = 0.0;
	time_calculator->reaction_time = 0.0;
	time_calculator->solvecvode_time = 0.0;
	time_calculator->solvecvode_hydro_time = 0.0;
	time_calculator->solvecvode_hydro_surfh_time = 0.0;
	time_calculator->solvecvode_hydro_et_time = 0.0;
	time_calculator->solvecvode_hydro_lateralflow_time = 0.0;
	time_calculator->solvecvode_hydro_verticalflow_time = 0.0;
	time_calculator->solvecvode_hydro_surfh_time = 0.0;
	time_calculator->solvecvode_hydro_riverflow_time = 0.0;
	time_calculator->solvecvode_bgc_time = 0.0;
	time_calculator->noahhydro_time = 0.0;
	time_calculator->noahhydro_watertable_time = 0.0;
	time_calculator->noahhydro_smflx_time = 0.0;
	time_calculator->chemical_time = 0.0;
	time_calculator->dailybgc_time = 0.0;
	time_calculator->other_time = 0.0;

}

void print_time_struct(struct time_struct* time_calculator) {
	printf("init_time cost: %f s\n", time_calculator->init_time);
	if (spinup_mode) {
		printf("spinup_time cost: %f s\n", time_calculator->spinup_time);
	}
	printf("applybc_time cost: %f s\n", time_calculator->applybc_time);
	printf("landsurface_time cost: %f s\n", time_calculator->landsurface_time);
	printf("reaction_time cost: %f s\n", time_calculator->reaction_time);
	printf("solvecvode_time cost: %f s\n", time_calculator->solvecvode_time);
	printf("solvecvode_hydro_time cost: %f s\n", time_calculator->solvecvode_hydro_time);
	printf("solvecvode_hydro_surfh_time cost: %f s\n", time_calculator->solvecvode_hydro_surfh_time);
	printf("solvecvode_hydro_et_time cost: %f s\n", time_calculator->solvecvode_hydro_et_time);
	printf("solvecvode_hydro_lateralflow_time cost: %f s\n", time_calculator->solvecvode_hydro_lateralflow_time);
	printf("solvecvode_hydro_verticalflow_time cost: %f s\n", time_calculator->solvecvode_hydro_verticalflow_time);
	printf("solvecvode_hydro_riverflow_time cost: %f s\n", time_calculator->solvecvode_hydro_riverflow_time);
	printf("solvecvode_bgc_time cost: %f s\n", time_calculator->solvecvode_bgc_time);
	printf("noahhydro_time cost: %f s\n", time_calculator->noahhydro_time);
	printf("noahhydro_watertable_time cost: %f s\n", time_calculator->noahhydro_watertable_time);
	printf("noahhydro_smflx_time cost: %f s\n", time_calculator->noahhydro_smflx_time);
	printf("chemical_time cost: %f s\n", time_calculator->chemical_time);
	printf("dailybgc_time cost: %f s\n", time_calculator->dailybgc_time);
	printf("other_time cost: %f s\n", time_calculator->other_time);

}
#endif

#if defined(_OPENMP)
void RunTime(double start_omp, double *cputime, double *cputime_dt)
{
	static double   ptime_omp;
	double          ct_omp;

	ptime_omp = (ptime_omp == 0.0) ? start_omp : ptime_omp;
	ct_omp = omp_get_wtime();
	*cputime_dt = (double)(ct_omp - ptime_omp);
	*cputime = (double)(ct_omp - start_omp);
	ptime_omp = ct_omp;
}
#else
void RunTime(clock_t start, double *cputime, double *cputime_dt)
{
	static clock_t  ptime;
	clock_t         ct;

	ptime = (ptime == 0.0) ? start : ptime;
	ct = clock();
	*cputime_dt = ((double)(ct - ptime)) / CLOCKS_PER_SEC;
	*cputime = ((double)(ct - start)) / CLOCKS_PER_SEC;
	ptime = ct;
}
#endif
