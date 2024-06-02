#include "pihm.h"

void UpdateVar(double stepsize, elem_struct elem[], river_struct river[], N_Vector CV_Y, PIHMDataStruct* PIHMData, PIHMToolDataStruct* PIHMToolData, ctrl_struct  *ctrl)
{
	double         *y;
	int             i;
	// 将 CV_Y 中的数据指针赋值给 y 变量
	y = NV_DATA(CV_Y);

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		double          subrunoff;
		// 在这里手动检查和修改 y[SURF(i)] 的值
		if (y[SURF(i)] < 0.0)
		{
			y[SURF(i)] = 0.0;
		}
		// 这几句代码是在每个步长ODE计算完成后，用ODE中的值更新内存中的值
		elem[i].ws.surf = y[SURF(i)];
		elem[i].ws.unsat = y[UNSAT(i)];
		elem[i].ws.gw = y[GW(i)];

		// 如果是边界处的三角形
#pragma omp critical
		if (PIHMToolData->all_adj_tris_ids[i] != -1) {
			int id_index = PIHMToolData->all_adj_tris_ids[i];
			PIHMData->elem_sufh[ctrl->cstep][id_index] = elem[i].ws.surf;
			PIHMData->elem_gwh[ctrl->cstep][id_index] = elem[i].ws.gw;
		}
		// xdw 修改，求解完后修改值，避免出现负值
		//elem[i].ws.surf = MAX(y[SURF(i)], 0.0);
		//elem[i].ws.unsat = MAX(y[UNSAT(i)], 0.0);
		//elem[i].ws.gw = MAX(y[GW(i)], 0.0);

#if defined(_DGW_)
		elem[i].ws.unsat_geol = y[UNSAT_GEOL(i)];
		elem[i].ws.gw_geol = y[GW_GEOL(i)];
#endif

#if defined(_DGW_)
		AdjustFluxes(elem[i].topo.area, stepsize, &elem[i].soil, &elem[i].geol, &elem[i].ws, &elem[i].ws0, &subrunoff,
			&elem[i].wf);
#else
		AdjustFluxes(elem[i].topo.area, stepsize, &elem[i].soil, &elem[i].ws, &elem[i].ws0, &subrunoff, &elem[i].wf);
#endif

#if defined(_NOAH_)
		elem[i].wf.runoff2 = subrunoff;
# if defined(_DGW_)
		elem[i].wf.runoff2 += elem[i].wf.infil_geol;
# endif

		elem[i].ps.nwtbl = FindWaterTable(elem[i].ps.nlayers, elem[i].ws.gw, elem[i].ps.soil_depth, elem[i].ps.satdpth);

		CalcLateralFlux(&elem[i].ps, &elem[i].wf);
#endif

		elem[i].ws0 = elem[i].ws;

#if defined(_BGC_)
		elem[i].ns.sminn = MAX(y[SOLUTE_SOIL(i, 0)], 0.0);

		elem[i].ns.nleached_snk += elem[i].nt.sminn0 - elem[i].ns.sminn + elem[i].solute[0].snksrc * stepsize;

		elem[i].nt.sminn0 = elem[i].ns.sminn;
#endif

#if defined(_CYCLES_)
		elem[i].ps.no3 = MAX(y[SOLUTE_SOIL(i, NO3)], 0.0);
		elem[i].ps.nh4 = MAX(y[SOLUTE_SOIL(i, NH4)], 0.0);

		UpdateNProfile(stepsize, &elem[i].soil, &elem[i].ws, &elem[i].ns, elem[i].solute, elem[i].ns.no3,
			elem[i].ns.nh4, &elem[i].ps, &elem[i].nf);

		elem[i].ns0 = elem[i].ns;
		elem[i].ps.no3_prev = elem[i].ps.no3;
		elem[i].ps.nh4_prev = elem[i].ps.nh4;
#endif

#if defined(_RT_)
		int             k;
		double          storage;

		storage = (elem[i].ws.gw + elem[i].ws.unsat) * elem[i].soil.porosity + elem[i].soil.smcmin * elem[i].soil.depth;

		for (k = 0; k < nsolute; k++)
		{
			elem[i].chms.tot_mol[k] = MAX(y[SOLUTE_SOIL(i, k)], 0.0);

			// Calculate concentrations
			elem[i].chms.tot_conc[k] = elem[i].chms.tot_mol[k] / storage;
			elem[i].chms.tot_conc[k] = MAX(elem[i].chms.tot_conc[k], ZERO_CONC);
		}

# if defined(_DGW_)
		storage = (elem[i].ws.unsat_geol + elem[i].ws.gw_geol) * elem[i].geol.porosity +
			elem[i].geol.smcmin * elem[i].geol.depth;
		storage = MAX(storage, 0.0);

		for (k = 0; k < nsolute; k++)
		{
			elem[i].chms_geol.tot_mol[k] = MAX(y[SOLUTE_GEOL(i, k)], 0.0);

			// Calculate concentrations
			elem[i].chms_geol.tot_conc[k] = (storage > 0.0) ? elem[i].chms_geol.tot_mol[k] / storage : 0.0;
			elem[i].chms_geol.tot_conc[k] = MAX(elem[i].chms_geol.tot_conc[k], ZERO_CONC);
		}
# endif
#endif

	}

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nriver; i++)
	{
		river[i].ws.stage = y[RIVER(i)];

#if defined(_BGC_)
		river[i].ns.streamn = MAX(y[SOLUTE_RIVER(i, 0)], 0.0);
#endif

#if defined(_CYCLES_)
		river[i].ns.no3 = MAX(y[SOLUTE_RIVER(i, NO3)], 0.0);
		river[i].ns.nh4 = MAX(y[SOLUTE_RIVER(i, NH4)], 0.0);
#endif

#if defined(_RT_)
		int             k;
		double          storage;

		storage = MAX(river[i].ws.stage, 0.0);

		for (k = 0; k < nsolute; k++)
		{
			river[i].chms.tot_mol[k] = MAX(y[SOLUTE_RIVER(i, k)], 0.0);

			// Calculate concentrations
			river[i].chms.tot_conc[k] = (storage > DEPTHR) ? river[i].chms.tot_mol[k] / storage : ZERO_CONC;
			river[i].chms.tot_conc[k] = MAX(river[i].chms.tot_conc[k], ZERO_CONC);
		}
#endif
	}
}
// 根据土壤水分饱和度，调整入渗速率
// The AdjustFluxes subroutine adjusts model infiltration rate and recharge rate based on mass balance in the soil
// column. This subroutine also calculates an equivalent infiltration rate, which is used as the boundary condition
// in the Noah soil moisture calculation. Unlike the infiltration rate, the equivalent infiltration rate is calculated
// without considering oversturation, and is based on the mass balance of the whole soil column, instead of just the
// unsaturated zone.
#if defined(_DGW_)
void AdjustFluxes(double area, double stepsize, const soil_struct *soil, const soil_struct *geol,
	const wstate_struct *ws, const wstate_struct *ws0, double *subrunoff, wflux_struct *wf)
#else
void AdjustFluxes(double area, double stepsize, const soil_struct *soil, const wstate_struct *ws,
	const wstate_struct *ws0, double *subrunoff, wflux_struct *wf)
#endif
{
	int             j;
	double          soilw0, soilw1;
#if defined(_DGW_)
	double          geolw0, geolw1;
	double          geol_runoff;
#endif

	// Adjust recharge
	soilw0 = ws0->gw;
	soilw1 = ws->gw;
	// 计算三角形三个边的地下水径流量之和
	*subrunoff = 0.0;
	for (j = 0; j < NUM_EDGE; j++)
	{
		*subrunoff += wf->subsurf[j] / area;
	}
	// 计算三角形的地下水补给量=（本时刻地下水位-上一时刻地下水位）*孔隙度/步长 + 侧向补给或损失 + 地下水蒸发 + 地下水散发
	// 地下水补给量指非饱和层向地下水层的输移量，等于地下水位实际变化量+蒸散发损失量
	wf->recharge = (soilw1 - soilw0) * soil->porosity / stepsize + *subrunoff + wf->edir_gw + wf->ett_gw;

	// Adjust infiltration
	soilw0 = ws0->unsat;
	soilw1 = ws->unsat;
	// 计算三角形的非饱和层入渗量 = （本时刻非饱和层水储量-上一时刻非饱和层水储量）*孔隙度/步长 + 地下水补给量 + 非饱和层蒸发 + 非饱和层散发
	// 非饱和层入渗量指地表向非饱和层的入渗量，等于非饱和层含水量的实际变化量+蒸散发损失量
	wf->infil = (soilw1 - soilw0) * soil->porosity / stepsize + wf->recharge + wf->edir_unsat + wf->ett_unsat;

#if defined(_DGW_)
	// Adjust bedrock recharge
	geolw0 = ws0->gw_geol;
	geolw1 = ws->gw_geol;

	geol_runoff = 0.0;
	for (j = 0; j < NUM_EDGE; j++)
	{
		geol_runoff += wf->dgw[j] / area;
	}

	wf->rechg_geol = (geolw1 - geolw0) * geol->porosity / stepsize + geol_runoff;

	// Adjust bedrock infiltration
	geolw0 = ws0->unsat_geol;
	geolw1 = ws->unsat_geol;

	wf->infil_geol = (geolw1 - geolw0) * geol->porosity / stepsize + wf->rechg_geol;

	// Further adjust soil infiltration and recharge rate to take into account bedrock leakage
	wf->recharge += wf->infil_geol;
	wf->infil += wf->infil_geol;
#endif

	// Calculate equivalent infiltration based on mass conservation
	soilw0 = ws0->gw + ws0->unsat;
	soilw0 = MIN(soilw0, soil->depth);
	soilw0 = MAX(soilw0, 0.0);

	soilw1 = ws->gw + ws->unsat;
	soilw1 = MIN(soilw1, soil->depth);
	soilw1 = MAX(soilw1, 0.0);
	// 总体入渗当量 = 地表向非饱和层的入渗 + 非饱和层向地下水的补给
	wf->eqv_infil = (soilw1 - soilw0) * soil->porosity / stepsize + *subrunoff + wf->edir_unsat + wf->edir_gw +
		wf->ett_unsat + wf->ett_gw;

#if defined(_DGW_)
	geolw0 = ws0->gw_geol + ws0->unsat_geol;
	geolw1 = ws->gw_geol + ws->unsat_geol;

	geol_runoff = 0.0;
	for (j = 0; j < NUM_EDGE; j++)
	{
		geol_runoff += wf->dgw[j] / area;
	}

	wf->infil_geol = (geolw1 - geolw0) * geol->porosity / stepsize + geol_runoff;

	wf->eqv_infil += wf->infil_geol;
#endif
	// 如果总体入渗当量 < 0，说明地下水是损失的，则从地下径流深度中扣除损失量
	if (wf->eqv_infil < 0.0)
	{
		*subrunoff -= wf->eqv_infil;
		wf->eqv_infil = 0.0;
	}
}
