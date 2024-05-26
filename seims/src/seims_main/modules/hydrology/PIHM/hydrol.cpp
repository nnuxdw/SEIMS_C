#include "pihm.h"
#if defined(_STATISTIC_TIME_)
void Hydrol(const ctrl_struct *ctrl, elem_struct elem[], river_struct river[], struct time_struct * ptime_calculator)
#else 
void Hydrol(const ctrl_struct *ctrl, elem_struct elem[], river_struct river[], exchange_struct * exchange)
#endif
{
	int             i;
#if defined(_STATISTIC_TIME_)
	clock_t hydro_start = clock();
#endif
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	// 根据等效地表水深，使用抛物线计算实际地表水深
	for (i = 0; i < nelem; i++)
	{
		// Calculate actual surface water depth
		elem[i].ws.surfh = SurfH(elem[i].ws.surf);
		// xiaodw , 在这里给三角形的地表水深增加上游来水
		// todo 
		elem[i].ws.surfh += exchange->elem_upstream_surfq[i] / elem[i].topo.area;
		elem[i].ws.gw += exchange->elem_upstream_subsurvol[i] / elem[i].topo.area;
		elem[i].ws.gw += exchange->elem_upstream_gwStorage[i] / elem[i].topo.area;
	}
#if defined(_STATISTIC_TIME_)
	ptime_calculator->t5_1_1 = clock();
	ptime_calculator->solvecvode_hydro_surfh_time += ((double)(ptime_calculator->t5_1_1 - hydro_start)) / CLOCKS_PER_SEC;
#endif
	// Determine which layers does ET extract water from
	// 根据地表水深、非饱和层含水量、地下水层水深，计算蒸散量
	EtUptake(elem);
#if defined(_STATISTIC_TIME_)
	ptime_calculator->t5_1_2 = clock();
	ptime_calculator->solvecvode_hydro_et_time += ((double)(ptime_calculator->t5_1_2 - ptime_calculator->t5_1_1)) / CLOCKS_PER_SEC;
#endif
	// Water flow
	// 地下水、地表水侧向流动
	LateralFlow(river, elem);
#if defined(_STATISTIC_TIME_)
	ptime_calculator->t5_1_3 = clock();
	ptime_calculator->solvecvode_hydro_lateralflow_time += ((double)(ptime_calculator->t5_1_3 - ptime_calculator->t5_1_2)) / CLOCKS_PER_SEC;
#endif
	// 垂向输移
	VerticalFlow((double)ctrl->stepsize, elem);
#if defined(_STATISTIC_TIME_)
	ptime_calculator->t5_1_4 = clock();
	ptime_calculator->solvecvode_hydro_verticalflow_time += ((double)(ptime_calculator->t5_1_4 - ptime_calculator->t5_1_3)) / CLOCKS_PER_SEC;
#endif
	// 河道流量，包括与三角形的地表水、地下水交换
	RiverFlow(elem, river);
#if defined(_STATISTIC_TIME_)
	ptime_calculator->t5_1_5 = clock();
	ptime_calculator->solvecvode_hydro_riverflow_time += ((double)(ptime_calculator->t5_1_5 - ptime_calculator->t5_1_4)) / CLOCKS_PER_SEC;
#endif
}

void EtUptake(elem_struct elem[])
{
	int             i;

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		// Source of direct evaporation
#if defined(_NOAH_)
		if (elem[i].ws.gw > elem[i].soil.depth - elem[i].soil.dinf)
		{
			elem[i].wf.edir_surf = 0.0;
			elem[i].wf.edir_unsat = 0.0;
			elem[i].wf.edir_gw = elem[i].wf.edir;
		}
		else
		{
			elem[i].wf.edir_surf = 0.0;
			elem[i].wf.edir_unsat = elem[i].wf.edir;
			elem[i].wf.edir_gw = 0.0;
		}
#else
		// 地表实际水深>0，地表水蒸发
		if (elem[i].ws.surfh >= DEPRSTG)
		{
			elem[i].wf.edir_surf = elem[i].wf.edir;
			elem[i].wf.edir_unsat = 0.0;
			elem[i].wf.edir_gw = 0.0;
		}
		// 地下水位高于可入渗的深度，地下水直接蒸发速率=土壤水分蒸发速率
		else if (elem[i].ws.gw > elem[i].soil.depth - elem[i].soil.dinf)
		{
			elem[i].wf.edir_surf = 0.0;
			elem[i].wf.edir_unsat = 0.0;
			elem[i].wf.edir_gw = elem[i].wf.edir;
		}
		// 地下水位较低，非饱和层直接蒸发速率=土壤水分蒸发速率
		else
		{
			elem[i].wf.edir_surf = 0.0;
			elem[i].wf.edir_unsat = elem[i].wf.edir;
			elem[i].wf.edir_gw = 0.0;
		}
#endif

		// Source of transpiration
#if defined(_NOAH_)
		elem[i].ps.gwet = GwTranspFrac(elem[i].ps.nwtbl, elem[i].ps.nroot, elem[i].wf.ett, elem[i].wf.et);
		elem[i].wf.ett_unsat = (1.0 - elem[i].ps.gwet) * elem[i].wf.ett;
		elem[i].wf.ett_gw = elem[i].ps.gwet * elem[i].wf.ett;
#else
		// 地下水位高于根系层，地下水散发速率=植物散发速率
		if (elem[i].ws.gw > elem[i].soil.depth - elem[i].ps.rzd)
		{
			elem[i].wf.ett_unsat = 0.0;
			elem[i].wf.ett_gw = elem[i].wf.ett;
		}
		// 地下水位较低，非饱和层散发速率=植物散发速率
		else
		{
			elem[i].wf.ett_unsat = elem[i].wf.ett;
			elem[i].wf.ett_gw = 0.0;
		}
#endif
	}
}
// 使用抛物线曲线来表示等效地表水深对应的实际地表水深
double SurfH(double surf_eqv)
{
	// Following Panday and Huyakorn (2004) AWR:
	// Use a parabolic curve to express the equivalent surface water depth (surf_eqv) in terms of actual flow depth
	// (surfh) when the actual flow depth is below depression storage; assume that
	// d(surf_eqv) / d(surfh) = 1.0 when surfh = DEPRSTG. Thus
	//   surf_eqv = (1 / 2 * DEPRSTG) * surfh ^ 2, i.e.
	//   surfh = sqrt(2 * DEPRSTG * surf_eqv)
	double          surfh;

	if (DEPRSTG == 0.0)
	{
		surfh = surf_eqv;
	}
	else
	{
		if (surf_eqv < 0.0)
		{
			surfh = 0.0;
		}
		else if (surf_eqv <= 0.5 * DEPRSTG)
		{
			surfh = sqrt(2.0 * DEPRSTG * surf_eqv);
		}
		else
		{
			surfh = DEPRSTG + (surf_eqv - 0.5 * DEPRSTG);
		}
	}

	return surfh;
}
