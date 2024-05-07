#include "pihm.h"

void LateralFlow(const river_struct river[], elem_struct elem[])
{
	int             i;
	double         *dh_dx;
	double         *dh_dy;

	dh_dx = (double *)malloc(nelem * sizeof(double));
	dh_dy = (double *)malloc(nelem * sizeof(double));
	// 计算每个三角形的综合摩擦坡度（即基于三个方向相邻三角形或河道高差、距离计算出来的综合摩擦坡度），放入dh_dx，dh_dy
	FrictionSlope(elem, river, dh_dx, dh_dy);

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		int             j;
		double          avg_sf;
		elem_struct    *nabr;

		for (j = 0; j < NUM_EDGE; j++)
		{
			// 如果三角形的相邻三角形在边界外，即为0
			if (elem[i].nabr[j] == 0)   // Boundary condition flux
			{
				// 基于达西定律，根据边界水头差计算地表水、地下水出流量
				BoundFluxElem(elem[i].attrib.bc[j], j, &elem[i].topo, &elem[i].soil, &elem[i].bc, &elem[i].ws,
					&elem[i].wf);
			}
			else
			{
				// 相邻三角形的索引
				nabr = &elem[elem[i].nabr[j] - 1];

				// Subsurface flow between triangular elements
				// 达西定律根据地下水位差计算地下水侧向流动
				elem[i].wf.subsurf[j] = SubsurfFlow(j, &elem[i], nabr);

				// Surface flow between triangular elements
				// 如果相邻不是河道，曼宁公式计算地表水流量
				if (elem[i].nabr_river[j] == 0)
				{
					// Surface flux between triangular elements
					// avg_sf not needed in kinematic mode
					avg_sf = 0.5 *
						(sqrt(dh_dx[i] * dh_dx[i] + dh_dy[i] * dh_dy[i]) +
							sqrt(dh_dx[nabr->ind - 1] * dh_dx[nabr->ind - 1] +
								dh_dy[nabr->ind - 1] * dh_dy[nabr->ind - 1]));

					elem[i].wf.overland[j] = OvlFlowElemToElem(j, avg_sf, &elem[i], nabr);
				}
			}
		}   // End of neighbor loop
	}   // End of element loop

	free(dh_dx);
	free(dh_dy);

#if defined(_DGW_)
	// Lateral deep groundwater flow
# if defined(_OPENMP)
#  pragma omp parallel for
# endif
	for (i = 0; i < nelem; i++)
	{
		int             j;
		elem_struct    *nabr;

		for (j = 0; j < NUM_EDGE; j++)
		{
			if (elem[i].nabr[j] == 0)
			{
				elem[i].wf.dgw[j] = DeepBoundFluxElem(elem[i].attrib.bc_geol[j], j, &elem[i].topo, &elem[i].geol,
					&elem[i].bc_geol, &elem[i].ws);
			}
			else
			{
				nabr = &elem[elem[i].nabr[j] - 1];

				// Groundwater flow modeled by Darcy's Law
				elem[i].wf.dgw[j] = DeepFlowElemToElem(elem[i].topo.dist_nabr[j], elem[i].topo.edge[j], &elem[i], nabr);
			}
		}
	}
#endif
}

void FrictionSlope(const elem_struct elem[], const river_struct river[], double dh_dx[], double dh_dy[])
{
	int             i;
#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		int             j;
		double          surfh[NUM_EDGE];
		double          x[NUM_EDGE];
		double          y[NUM_EDGE];
		const elem_struct *nabr;
		const river_struct *river_nabr;

		for (j = 0; j < NUM_EDGE; j++)
		{
			// 边界三角形
			if (elem[i].nabr[j] == 0)
			{
				// 相邻三角形的地表水面高程=当前三角形地表高程+地表实际水深
				surfh[j] = elem[i].topo.zmax + elem[i].ws.surfh;
				// 相邻三角形的中心点坐标
				x[j] = elem[i].topo.x_nabr[j];
				y[j] = elem[i].topo.y_nabr[j];
			}
			// 不与河道相邻的三角形
			else if (elem[i].nabr_river[j] == 0)
			{
				nabr = &elem[elem[i].nabr[j] - 1];
				// 相邻三角形的地表水面高程=相邻三角形地表高程+相邻三角形地表实际水深
				surfh[j] = nabr->topo.zmax + nabr->ws.surfh;
				x[j] = elem[i].topo.x_nabr[j];
				y[j] = elem[i].topo.y_nabr[j];
			}
			// 与河道相邻的三角形
			else
			{
				river_nabr = &river[elem[i].nabr_river[j] - 1];
				// 相邻河道水面高程 = 河道水深 > 河道深度 ？河道底部高程+水深：河堤高程
				surfh[j] = (river_nabr->ws.stage > river_nabr->shp.depth) ?
					river_nabr->topo.zbed + river_nabr->ws.stage : river_nabr->topo.zmax;
				x[j] = river[elem[i].nabr_river[j] - 1].topo.x;
				y[j] = river[elem[i].nabr_river[j] - 1].topo.y;
			}
		}
		// 摩擦坡度 x,y,surfh是相邻三角形或河道的质心坐标、实际水面高程
		dh_dx[i] = DhByDl(y, x, surfh);
		dh_dy[i] = DhByDl(x, y, surfh);
	}
}

double AvgHsurf(double diff, double h_surf, double h_nabr)
{
	return (diff > 0.0) ? MAX(h_surf - DEPRSTG, 0.0) : MAX(h_nabr - DEPRSTG, 0.0);
}

double AvgH(double diff, double h_sub, double h_nabr)
{
	return (diff > 0.0) ? MAX(h_sub, 0.0) : MAX(h_nabr, 0.0);
}

double DhByDl(const double l1[], const double l2[], const double surfh[])
{
	// 对于dh_dx：y1*
	return -(l1[2] * (surfh[1] - surfh[0]) + l1[1] * (surfh[0] - surfh[2]) + l1[0] * (surfh[2] - surfh[1])) /
		(l2[2] * (l1[1] - l1[0]) + l2[1] * (l1[0] - l1[2]) + l2[0] * (l1[2] - l1[1]));
}

double EffKh(double gw, const soil_struct *soil)
{
	double          k1, k2;
	double          d1, d2;

	gw = MAX(gw, 0.0);

	if (gw > soil->depth - soil->dmac)
	{
		k1 = soil->kmach * soil->areafv + soil->ksath * (1.0 - soil->areafv);
		k2 = soil->ksath;

		if (gw > soil->depth)
		{
			d1 = soil->dmac;
			d2 = soil->depth - soil->dmac;
		}
		else
		{
			d1 = gw - (soil->depth - soil->dmac);
			d2 = soil->depth - soil->dmac;
		}

		return (k1 * d1 + k2 * d2) / (d1 + d2);
	}
	else
	{
		return soil->ksath;
	}
}

double OverLandFlow(double avg_h, double grad_h, double avg_sf, double cross_area, double avg_rough)
{
	return cross_area * pow(avg_h, 0.6666667) * grad_h / (sqrt(avg_sf) * avg_rough);
}
// 达西定律，根据水力坡度计算流量
double SubsurfFlow(int j, const elem_struct *elem_ptr, const elem_struct *nabr)
{
	double          diff_h;
	double          avg_h;
	double          grad_h;
	double          effk, effk_nabr;
	double          avg_ksat;

	// Subsurface lateral flux calculation between triangular elements
	// 相邻三角形之间的地下水位落差= [地下水位(指土壤底部高程之上的地下水深度)+土壤底部高程] 之差
	diff_h = (elem_ptr->ws.gw + elem_ptr->topo.zmin) - (nabr->ws.gw + nabr->topo.zmin);
	// 平均地下水位取相邻三角形中较高一方的地下水位
	avg_h = AvgH(diff_h, elem_ptr->ws.gw, nabr->ws.gw);
	// 水力坡度
	grad_h = diff_h / elem_ptr->topo.dist_nabr[j];

	// Take into account macropore effect
	effk = EffKh(elem_ptr->ws.gw, &elem_ptr->soil);
	effk_nabr = EffKh(nabr->ws.gw, &nabr->soil);
	avg_ksat = 0.5 * (effk + effk_nabr);

	// Groundwater flow modeled by Darcy's Law
	// 达西定律，根据水力坡度计算流量
	return avg_ksat * grad_h * avg_h * elem_ptr->topo.edge[j];
}

double OvlFlowElemToElem(int j, double avg_sf, const elem_struct *elem_ptr, const elem_struct *nabr)
{
	double          diff_h;
	double          avg_h;
	double          grad_h;
	double          avg_rough;
	double          cross_area;

	diff_h = (elem_ptr->ws.surfh + elem_ptr->topo.zmax) - (nabr->ws.surfh + nabr->topo.zmax);
	avg_h = AvgHsurf(diff_h, elem_ptr->ws.surfh, nabr->ws.surfh);
	grad_h = MAX(diff_h / elem_ptr->topo.dist_nabr[j], GRADMIN);
	avg_sf = MAX(avg_sf, GRADMIN);
	avg_rough = 0.5 * (elem_ptr->lc.rough + nabr->lc.rough);
	cross_area = avg_h * elem_ptr->topo.edge[j];

	return OverLandFlow(avg_h, grad_h, avg_sf, cross_area, avg_rough);
}

// todo：为什么假设地表水出流量为0？
void BoundFluxElem(int bc_type, int j, const topo_struct *topo, const soil_struct *soil, const bc_struct *bc,
	const wstate_struct *ws, wflux_struct *wf)
{
	double          diff_h;
	double          avg_h;
	double          effk;
	double          avg_ksat;
	double          grad_h;

	// Assume no flow for surface
	wf->overland[j] = 0.0;

	// No flow (natural) boundary condition is default
	// 边界j上的出流流量
	// 不允许边界出流
	if (bc_type == NO_FLOW)
	{
		wf->subsurf[j] = 0.0;
	}
	// Note: ideally different boundary conditions need to be incorporated for surf and subsurf respectively
	// 边界条件为水头，根据水头差计算地下水出流量
	else if (bc_type > 0)
	{
		// 水头差 = i的地下水高程 - 边界水头
		diff_h = ws->gw + topo->zmin - bc->head[j];
		// 平均水头差
		avg_h = AvgH(diff_h, ws->gw, bc->head[j] - topo->zmin);
		// Minimum distance from circumcenter to the edge of the triangle on which boundary condition is defined
		effk = EffKh(ws->gw, soil);
		avg_ksat = effk;
		grad_h = diff_h / topo->dist_nabr[j];
		// 达西定律计算边界出流量
		wf->subsurf[j] = avg_ksat * grad_h * avg_h * topo->edge[j];
	}
	// 边界条件为流量，以固定流量出流
	else
	{
		// Negative sign is added so the positive numbers in forcing time series represents source
		// + 代表流量的源，- 代表汇
		wf->subsurf[j] = -bc->flux[j];
	}
}

#if defined(_DGW_)
double DeepFlowElemToElem(double distance, double edge, const elem_struct *elem_ptr, const elem_struct *nabr)
{
	double          diff_h;
	double          avg_h;
	double          grad_h;
	double          effk, effk_nabr;
	double          avg_ksat;

	diff_h = (elem_ptr->ws.gw_geol + elem_ptr->topo.zbed) - (nabr->ws.gw_geol + nabr->topo.zbed);
	avg_h = AvgH(diff_h, elem_ptr->ws.gw_geol, nabr->ws.gw_geol);
	grad_h = diff_h / distance;

	effk = EffKh(elem_ptr->ws.gw_geol, &elem_ptr->geol);
	effk_nabr = EffKh(nabr->ws.gw_geol, &nabr->geol);
	avg_ksat = 0.5 * (effk + effk_nabr);

	return avg_ksat * grad_h * avg_h * edge;
}

double DeepBoundFluxElem(int bc_type, int j, const topo_struct *topo, const soil_struct *geol, const bc_struct *bc,
	const wstate_struct *ws)
{
	double          diff_h;
	double          avg_h;
	double          effk;
	double          grad_h;
	double          flux;

	// No flow (natural) boundary condition is default
	if (bc_type == NO_FLOW)
	{
		flux = 0.0;
	}
	else if (bc_type > 0)
	{
		// Dirichlet boundary conditions
		diff_h = ws->gw_geol + topo->zbed - bc->head[j];
		avg_h = AvgH(diff_h, ws->gw_geol, bc->head[j] - topo->zbed);
		// Minimum distance from circumcenter to the edge of the triangle on which boundary condition is defined
		effk = geol->ksath;
		grad_h = diff_h / topo->dist_nabr[j];
		flux = effk * grad_h * avg_h * topo->edge[j];
	}
	else
	{
		// Neumann boundary conditions
		flux = -bc->flux[j];
	}

	return flux;
}
#endif
