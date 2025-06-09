/*
* sgm_fast.cpp
*
*  Created on: 14 May 2014
*      Author: td14281
*/

#include "../lisflood.h"
#include "../utility.h"
#include "sgm_fast.h"
#include <math.h>
#include <omp.h>

#include "../sgc.h"
#include "lis2_output.h"
#include "file_tool.h"
#include "../time_tool.h"


#if defined (__INTEL_COMPILER) && _PROFILE_MODE > 0
#include "ittnotify.h"
#endif

/*
OpenMP reduction variables defined as global.
This enables the pragma omp parallel to be defined in a separate
function to the pragma omp for reduction(...)
Generally a bad practice to define variables as global
as they can soon become a mess.
*/
NUMERIC_TYPE reduce_Hmax;
NUMERIC_TYPE reduce_evap_loss;
NUMERIC_TYPE reduce_infil_loss;
NUMERIC_TYPE reduce_interflow_loss;
NUMERIC_TYPE reduce_rain_total;
NUMERIC_TYPE reduce_Qpoint_timestep_pos;
NUMERIC_TYPE reduce_Qpoint_timestep_neg;

NUMERIC_TYPE reduce_flood_area;
NUMERIC_TYPE reduce_domain_volume;

NUMERIC_TYPE evap_deltaH_step;
NUMERIC_TYPE rain_deltaH_step;
NUMERIC_TYPE snow_deltaH_step;
NUMERIC_TYPE temperature_step;

int itCount;
double total_write_time = C(0.0);

//NUMERIC_TYPE infilAcc = C(0.0);
NUMERIC_TYPE infil = C(0.0);
int infilCount = 0;

///
/// Calculate channel flow using inertial wave equation --- in this case q_old and returned q will be in m3s-1
/// 
inline NUMERIC_TYPE CalculateQ(const NUMERIC_TYPE surface_slope,
	NUMERIC_TYPE R, // hydraulic radius
	const NUMERIC_TYPE delta_time,
	const NUMERIC_TYPE g,
	const NUMERIC_TYPE area, //flow area (not cell area)
	const NUMERIC_TYPE g_friction_squared,
	const NUMERIC_TYPE q_old,
	const NUMERIC_TYPE max_Froude)
{

	// calculate flow based on m^3 formula (note power is 4/3, profiling shows performance gain by multiply and cuberoot)
#if _CALCULATE_Q_MODE == 0
	NUMERIC_TYPE pow_tmp1, pow_tmp, abs_q, calc_num, calc_den;

	//R = getmin(R, 10.0); // removed so depth of flow doesn't max out at 10m (JCN)

	pow_tmp1 = R * R * R * R;
	pow_tmp = CBRT(pow_tmp1); // 4 multiplies and 1 cube root profiled faster than POW(R,4/3)

	abs_q = FABS(q_old);

	calc_num = (q_old - g * area * delta_time * surface_slope);
	calc_den = (1 + delta_time * g_friction_squared * abs_q / (pow_tmp * area));
	
	return calc_num / calc_den;
	
#else
#if _CALCULATE_Q_MODE == 1
	NUMERIC_TYPE pow_tmp1, pow_tmp, abs_q, calc_num, calc_den;

	//R = getmin(R, 10.0); // removed so depth of flow doesn't max out at 10m (JCN)

	pow_tmp1 = R * R * R * R;
	pow_tmp = CBRT(pow_tmp1); // 4 multiplies and 1 cube root profiled faster than POW(R,4/3)

	abs_q = FABS(q_old);
	// 根据水力半径，河流断面面积（河道水深*河道宽度），上个时间步长的流量，时间步长，曼宁系数，水力坡度计算下个时间步长的河道流量
	calc_num = (q_old - g * area * delta_time * surface_slope);
	calc_den = (1 + delta_time * g_friction_squared * abs_q / (pow_tmp * area));

	// limit to max Froude
	calc_num = calc_num / calc_den; // calculate Q as calc_num
	calc_den = max_Froude*area*SQRT(g*R); // Calculate max Q for max Froude as calcden

	if (FABS(calc_num) < calc_den) return calc_num; // return calc_num if its less than calc_den
	else return copysign(1.0, calc_num)*calc_den; // else return calc_den but get the sign from the surface slope

#else
#if _CALCULATE_Q_MODE == 2
	NUMERIC_TYPE pow_tmp1, pow_tmp, abs_q, calc_num, calc_den;

	pow_tmp = POW(R, C(4.0)/C(3.0));

	abs_q = FABS(q_old);

	calc_num = (q_old - g * area * delta_time * surface_slope);
	calc_den = (1 + delta_time * g_friction_squared * abs_q / (pow_tmp * area));
	return calc_num / calc_den;
#else

	return (q_old - g * area * delta_time * surface_slope) / ((1 + delta_time * g_friction_squared * FABS(q_old) / (POW(R, C(4.0)/C(3.0)) * area)));
#endif
#endif
#endif
}

inline NUMERIC_TYPE SGC2_CalculateVelocity(const int index, const int index_next,
	const NUMERIC_TYPE * Q_grid,
	const NUMERIC_TYPE * h_grid, const NUMERIC_TYPE * dem_grid, const NUMERIC_TYPE width)
{
	if (Q_grid[index_next] != C(0.0) && (h_grid[index] > C(0.0) || h_grid[index_next] > C(0.0)))
	{
		NUMERIC_TYPE h0 = h_grid[index];
		NUMERIC_TYPE h1 = h_grid[index_next];
		NUMERIC_TYPE z0 = dem_grid[index];
		NUMERIC_TYPE z1 = dem_grid[index_next];

		NUMERIC_TYPE surface_elevation0 = z0 + h0;
		NUMERIC_TYPE surface_elevation1 = z1 + h1;
		// Calculating hflow based on floodplain levels
		NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1)
			- getmax(z0, z1);

		return (Q_grid[index_next] / width) / hflow;
	}
	else
	{
		return C(0.0);
	}
}

//-----------------------------------------------------------------------------------
// This function calculates the hydraulic radius of a sub-grid channel given the channel type
inline NUMERIC_TYPE SGC2_CalcR(int gr, NUMERIC_TYPE h, NUMERIC_TYPE hbf, NUMERIC_TYPE w, NUMERIC_TYPE wbf, NUMERIC_TYPE A, const SGCprams *SGCptr)
{
#if defined _ONLY_RECT && _ONLY_RECT == 1
	NUMERIC_TYPE  R = getmin(h, hbf); // don't exceed bankfull
	return R = A / (w + 2 * R); // calculate hydraulic radius
#else
	// This function calculates the hydraulic redius of the channel given the flow area (A) and depth (h)
	NUMERIC_TYPE R = C(0.0), sl, hp, b1, b2;

	switch (SGCptr->SGCchantype[gr])
	{
	case 1: // Rectangular channel (default) - This model has a top width and bed elevation and top 
		R = getmin(h, hbf); // don't exceed bankfull
		R = A / (w + 2 * R); // calculate hydraulic radius
		break; // Break terminates the switch statement

	case 2: // Exponent channel       
		h = getmin(h, hbf); // don't exceed bankfull
		hp = 2 * h / wbf; // use half width
		// get beta parameters for the channel group number
		if (hp <= SGCptr->SGCbetahmin)
		{
			// beta parameters for flow depths below SGCbetahmin (C(0.05)) depth/bankfull width
			b1 = SGCptr->SGCbeta1[gr]; b2 = SGCptr->SGCbeta2[gr];
			R = b1*hp + b2*hp*hp; // calculate wetted perimiter component
		}
		else
		{
			// beta parameters for flow depths above or equal to SGCbetahmin (C(0.05)) depth/bankfull width
			b1 = SGCptr->SGCbeta3[gr]; b2 = SGCptr->SGCbeta4[gr];
			hp -= SGCptr->SGCbetahmin; // decrease hp to account for for wetted perimiter fraction below C(0.05) depth/bankful depth
			R = SGCptr->SGCbeta5[gr] + b1*hp + b2*hp*hp; // calculate wetted perimiter component
		}
		R = A / (w + R*wbf);  // calculate hydraulic radius
		break;	// Break terminates the switch statement

	case 3: // linear slope - This model has a bed elevation, slope, top width and top elevation. 
		if (h < hbf)	R = A / (h + SQRT(h  *h + w*w));	// within bank flow hydraulic radius
		else			R = A / (hbf + SQRT(hbf*hbf + w*w));   // out of bank hydraulic radius (wetted perimeter is actually constant !!)
		break;	// Break terminates the switch statement

	case 4: // triangular channel - This model has the bed elevation, slope, top width and top elevation 
		w = C(0.5)*w;
		if (h < hbf)	R = A / (2 * SQRT(h  *h + w*w));   // within bank flow hydraulic radius
		else			R = A / (2 * SQRT(hbf*hbf + w*w));   // out of bank hydraulic radius (wetted perimeter is actually constant !!)
		break;	// Break terminates the switch statement  

	case 5: // parabolic channel       
		h = getmin(h, hbf); // don't exceed bankfull
		w = w / C(2.0); // half width
		R = SQRT(w*w + C(16.0)*h*h);
		R = C(0.5)*R + (w*w) / (C(8.0)*h) * log((C(4.0)*h + R) / w);
		R = C(2.0)*R;
		R = A / R;  // calculate hydraulic radius  */
		break;	// Break terminates the switch statement

	case 6: // Rectangular channel (no banks) - This model has a top width and bed elevation and top 
		R = A / w; // calculate hydraulic radius
		break; // Break terminates the switch statement

	case 7: // trapazoidal channel
		sl = SGCptr->SGCs[gr];
		if (h < hbf)	R = A / (w + 2 * h   * SQRT(1 + sl*sl));   // within bank flow hydraulic radius
		else			R = A / (w + 2 * hbf * SQRT(1 + sl*sl));   // out of bank hydraulic radius (wetted perimeter is actually constant !!)
		break;	// Break terminates the switch statement

	default: // its all gone wrong
		printf("should not be here! Something is wrong with the SGC channel model R calculation");
		break;
	}
	return(R);
#endif
}

inline void SGC2_CalcA(int gr, NUMERIC_TYPE hflow, NUMERIC_TYPE bf, NUMERIC_TYPE *A, NUMERIC_TYPE *we, const SGCprams *SGCptr)
{
#if defined _ONLY_RECT && _ONLY_RECT == 1
	(*A) = (*we)*hflow;
#else

	// This function calculates the area of flow (A) for a given flow depth (hflow), in some cases it
	// also returnes the widths of flow (We).
	NUMERIC_TYPE sl;
	// switch depending on the channel type
	switch (SGCptr->SGCchantype[gr])
	{
	case 1: // Rectangular channel (default) - This model has a top width and bed elevation and top 
		(*A) = (*we)*hflow;
		break;	// Break terminates the switch statement

	case 2: // Power shaped channel. h = x^sl channel       
		sl = SGCptr->SGCs[gr];
		if (hflow < bf)
		{
			(*we) = (*we)*pow(hflow / bf, C(1.0) / sl);
			(*A) = hflow*(*we)*(C(1.0) - C(1.0) / (sl + C(1.0)));
		}
		else (*A) = bf*(*we)*(1 - 1 / (sl + 1)) + (*we)*(hflow - bf); // out of bank flow area
		break;	// Break terminates the switch statement

	case 3: // linear slope. 
		if (hflow < bf)
		{
			(*we) = ((*we) / bf)*hflow;
			(*A) = (*we)*hflow*C(0.5);    // within bank flow area
		}
		else  (*A) = (*we)*bf*C(0.5) + (*we)*(hflow - bf); // out of bank flow area
		break;	// Break terminates the switch statement

	case 4: // triangular channel
		if (hflow < bf)
		{
			(*we) = ((*we) / bf)*hflow;
			(*A) = (*we)*hflow*C(0.5);    // within bank flow area
		}
		else  (*A) = (*we)*bf*C(0.5) + (*we)*(hflow - bf); // out of bank flow area
		break;	// Break terminates the switch statement  

	case 5: // parabolic channel       
		if (hflow < bf)
		{
			(*we) = (*we)*SQRT(hflow / bf);
			(*A) = hflow*(*we)*(C(2.0) / C(3.0));
		}
		else    (*A) = bf*   (*we)*(C(2.0) / C(3.0)) + (*we)*(hflow - bf); // out of bank flow area
		break;	// Break terminates the switch statement

	case 6: // Rectangular channel (default) - This model has a top width and bed elevation and top 
		(*A) = (*we)*hflow;
		break;	// Break terminates the switch statement

	case 7: // trapazoidal channel
		sl = SGCptr->SGCs[gr];
		if (hflow < bf) (*A) = ((*we) + sl*hflow)*hflow;    // within bank flow area
		else            (*A) = ((*we) + sl*bf)*bf + ((*we) + sl*bf)*(hflow - bf);
		break;	// Break terminates the switch statement

	default: // its all gone wrong
		printf("should not be here! Something is wrong with the SGC channel model A calculation");
		break;
	}
	return;
#endif
}

/*

case 7: // trapazoidal channel
// calculate hydraulic radius
if (hflow < bf)	R = A / (we + 2* hflow * SQRT(1+sl*sl));   // within bank flow hydraulic radius
else			R = A / (we + 2* bf    * SQRT(1+sl*sl));   // out of bank hydraulic radius (wetted perimeter is actually constant !!)
break;	// Break terminates the switch statement

*/


/// cell_width: cross section width
/// 
/// output: Q_FP_corrected flood plain flow minus any region of flow above the sub-grid (Neal 2012 Figure 1 (C) )洪泛区流量
/// output: Q_FP_old flood flood plain flow (not corrected, use for next iteration input) 洪泛区+河道流量
/// output: Q_SG_old flood flood plain flow (use for next iteration input)河道流量（处理为正值）
// 计算洪泛区流量和河道流量，如果河道宽度>栅格宽度，则洪泛区流量为0
inline NUMERIC_TYPE SGC2_CalcPointFREE(const NUMERIC_TYPE hflow, const NUMERIC_TYPE SGC_width, const NUMERIC_TYPE Sf, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE cell_width, const NUMERIC_TYPE g, const NUMERIC_TYPE g_friction_squared_SG, const NUMERIC_TYPE g_friction_squared_FP, const NUMERIC_TYPE SGC_BankFullHeight, const int gr, const int sign,
	NUMERIC_TYPE *Q_FP_old, NUMERIC_TYPE *Q_SG_old,
	const SGCprams *SGCptr, const NUMERIC_TYPE max_Froude)
{
	NUMERIC_TYPE Q_FP_corrected;
	NUMERIC_TYPE A, R, SGC_width_current;
	NUMERIC_TYPE abs_q, SGC_hflow;
	SGC_width_current = SGC_width; // save bank full width
	if (SGC_width > C(0.0)) // channel present
	{
		SGC_hflow = hflow + SGC_BankFullHeight;

		SGC2_CalcA(gr, SGC_hflow, SGC_BankFullHeight, &A, &SGC_width_current, SGCptr); // calculate channel area for SGC
		R = SGC2_CalcR(gr, SGC_hflow, SGC_BankFullHeight, SGC_width_current, SGC_width, A, SGCptr); // calculate hydraulic radius for SGC
		// Calculate channel flow using inertial wave equation --- in this case Qxold will be in m3s-1 not m2s-1
		abs_q = FABS(*Q_SG_old);

		/// set Sf to be negative to ensure the numerator in CalculateQ will be positive (Sf used in subtraction)
		*Q_SG_old = sign * CalculateQ(-1 * FABS(Sf), R, delta_time, g, A, g_friction_squared_SG, abs_q, max_Froude);
	}
	//	else
	//	{
	//#ifdef _DEBUG
	//		// Toby Dunne removed redundant *Q_SG_old = C(0.0);
	//		if (*Q_SG_old != C(0.0))
	//			printf("SGC2_CalcPointFREE expect non sub grid cell to have no sub grid flow\n");
	//#endif
	//	}
	// multiply flux by -sign and use absolute value of q0 to get flux directions correctly assigned at boundaries
	// FABS on Sf and q0 always results in positive or no flow... sign then sorts out the direction(jcn)
	//if(hflow>Solverptr->DepthThresh && SGC_width_current < Parptr->dx) // only calculate floodpplain flow if the depth is above bank and the channel is narrower than a cell //CCS_deletion
	// 这里也说明了该subgrid模型只适合河道宽度<栅格宽度的情况，满足此条件才会计算洪泛区流量
	if (hflow > depth_thresh && SGC_width_current < cell_width) // only calculate floodpplain flow if the depth is above bank and the channel is narrower than a cell
	{
		// calculate FP flow
		//*qoldptr = sign*(FABS(*qoldptr) + FABS(g*Tstep*hflow*Sf)) / (1 + Tstep*g_friction_squared_FP*FABS(*qoldptr) / (pow(hflow, (C(7.) / C(3.)))));
		A = cell_width * hflow;
		R = hflow;
		abs_q = FABS(*Q_FP_old);

		NUMERIC_TYPE q;

		/// set Sf to be negative to ensure the numerator in CalculateQ will be positive (Sf used in subtraction)
		q = sign * CalculateQ(-1 * FABS(Sf), R, delta_time, g, A, g_friction_squared_FP, abs_q, max_Froude);
		*Q_FP_old = q;

		// subtract channel flow from flood plain q
		// Q_FP_corrected是洪泛区上的流量
		if (SGC_width_current > C(0.0))
		{
			NUMERIC_TYPE channel_ratio = min(SGC_width_current / cell_width, C(1.0));
			q = q - channel_ratio * q;
		}
		Q_FP_corrected = q;
	}
	else
	{
		*Q_FP_old = C(0.0);
		Q_FP_corrected = C(0.0);
	}
	return Q_FP_corrected;
}


//-----------------------------------------------------------------------------------
// This function updates H for a sub-grid channel given the channel type
inline NUMERIC_TYPE SGC2_CalcUpH(const NUMERIC_TYPE V, const NUMERIC_TYPE c, const int channel_group, const SGCprams *SGCptr)
{
#if defined _ONLY_RECT && _ONLY_RECT == 1
	return V / c;
#else
	NUMERIC_TYPE h = C(0.0);

	int SGCchan_type = SGCptr->SGCchantype[channel_group];
	NUMERIC_TYPE sl = SGCptr->SGCs[channel_group];

	// switch to the correct sub-grid channel, default 1 is the rectangular
	switch (SGCchan_type)
	{
	case 1: // Rectangular channel (default) - This model has a top width and bed elevation and top 
		h = V / c;
		break;

	case 2: // y = x^sl channel       
		h = pow(V / c, sl / (sl + C(1.0)));
		break;  // Break terminates the switch statement

	case 3: // Rectangular channel (default) - This model has a top width and bed elevation and top 
		h = V / c;
		break;

	case 4: // Rectangular channel (default) - This model has a top width and bed elevation and top 
		h = V / c;
		break;

	case 5: // parabiolic channel       
		h = pow(V / c, C(2.0) / C(3.0));
		break;  // Break terminates the switch statement

	case 6: // Rectangular channel (no banks) - This model has a top width and bed elevation and top 
		h = V / c;
		break;

	default: // its all gone wrong
		printf("should not be here! Something is wrong with the SGC channel model in SGC_UpdateH");
		break;
	} // end of switch statement
	return(h);
#endif
}

inline NUMERIC_TYPE SGC2_CalcUpV(const NUMERIC_TYPE h, const NUMERIC_TYPE c, const int channel_group, const SGCprams *SGCptr)
{
#if defined _ONLY_RECT && _ONLY_RECT == 1
	return h*c;
#else

	int SGCchan_type = SGCptr->SGCchantype[channel_group];
	NUMERIC_TYPE sl = SGCptr->SGCs[channel_group];

	// This function calculates the volume of a sub-grid channel given a depth
	NUMERIC_TYPE v = C(0.0);
	// switch to the correct sub-grid channel, default 1 is the rectangular
	switch (SGCchan_type)
	{
	case 1:
		v = h*c;
		break;

	case 2:
		v = c*pow(h, C(1.0) / sl + C(1.0));
		break;

	case 3:
		v = h*c;
		break;

	case 4:
		v = h*c;
		break;

	case 5:
		v = c*pow(h, C(3.0) / C(2.0));
		break;

	case 6: // Rectangular channel (no banks)
		v = h*c;
		break;

	default: // its all gone wrong
		printf("should not be here! Something is wrong with the SGC channel model in SGC2_CalcUpV");
		break;
	} // end of switch statement
	return(v);
#endif
}



inline NUMERIC_TYPE CalculateRoutingQ(const NUMERIC_TYPE delta_time,
	const NUMERIC_TYPE h0, NUMERIC_TYPE h1,
	const NUMERIC_TYPE z0, const NUMERIC_TYPE z1,
	const NUMERIC_TYPE route_V_ratio_per_sec, const NUMERIC_TYPE cell_area)
{
	//h1 = Arrptr->H[Arrptr->FlowDir[p0]] - Arrptr->SGCbfH[Arrptr->FlowDir[p0]];
	if (h1 < 0)
	{
		h1 = C(0.0);
	}	// If h1 negative due to below bankful SG channel cell, set h1 to zero 

	//z0 = Arrptr->DEM[p0]; //cell DEM height
	//z1 = Arrptr->DEM[Arrptr->FlowDir[p0]]; //lowest neighbour cell DEM height 

	NUMERIC_TYPE flow = (z0 + h0) - (z1 + h1);/*calculate the maximum possible flow into lowest neigbour cell by
											  comparing water surface elevations:*/
	/*where water surface elevation of neighbour cell is below DEM
	level of current cell, set maxflow to h0*/
	if (flow > h0)
	{
		flow = h0;
	}
	/*where water surface elevation of neighbour cell is above water TFD - can never happen as route only triggered if high surface slope
	surface elevation of current cell, set maxflow to 0*/
	if (flow < 0)
	{
		flow = 0;
	}

	NUMERIC_TYPE flow_fraction_s = route_V_ratio_per_sec; // fraction of cell volume to route in this time step
	if ((flow_fraction_s * delta_time) > 1)
		flow_fraction_s = 1 / delta_time; // more than 100 % of the volume being removed pre time step

	NUMERIC_TYPE flowQ = flow * cell_area * flow_fraction_s; // q is volume per second, when q converted to voume in SGC2_UpdateVol_floodplain_row q is multiplied by delta_time

	return flowQ;
}

//-----------------------------------------------------------------------------------
// CALCULATE WIER FLOW BETWEEN A POINT AND ITS W NEIGHBOUR
inline NUMERIC_TYPE CalcWeirQ(WeirLayout * weirs, const int grid_cols,
	const int grid_index_this, const int grid_index_next,
	const int weir_id,
	const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE delta_time,
	const NUMERIC_TYPE * h_grid, 
	const NUMERIC_TYPE * volume_grid,
	WetDryRowBound* wet_dry_bounds,
	const EDirection dir_positive, const EDirection dir_negative)
{
	NUMERIC_TYPE z0, z1, h0, h1, Q;
	//NUMERIC_TYPE usVel; // MT upstream velocity for energy gradient height calc.
	//NUMERIC_TYPE heg; // MT upstream energy gradient height
	//int p0, p1, pq0, weir_id;

	const int weir_pair_id0 = 2 * weir_id;
	const int weir_pair_id1 = weir_pair_id0 + 1;

	z0 = weirs->cell_pair.sg_cell_dem[weir_pair_id0];
	z1 = weirs->cell_pair.sg_cell_dem[weir_pair_id1];

	h0 = h_grid[grid_index_this];
	h1 = h_grid[grid_index_next];

	NUMERIC_TYPE surfaceElevation0 = h0 + z0;
	NUMERIC_TYPE surfaceElevation1 = h1 + z1;

	h0 += weirs->cell_pair.sg_cell_SGC_BankFullHeight[weir_pair_id0];
	h1 += weirs->cell_pair.sg_cell_SGC_BankFullHeight[weir_pair_id1];

	//int x = weirs->cell_pair.sg_cell_x[weir_pair_id0];
	//int y = weirs->cell_pair.sg_cell_y[weir_pair_id0];

	Q = C(0.0);
	if (h0 > depth_thresh || h1 > depth_thresh)
	{

		if ((surfaceElevation0) > (surfaceElevation1))		// Flow in + direction
		{
			if ((surfaceElevation0) > weirs->Weir_hc[weir_id] && h0 > 0) // check depth is above weir and that the cell is wet
			{
				if (weirs->Weir_Fixdir[weir_id] == DirectionNA || weirs->Weir_Fixdir[weir_id] == dir_positive) // check for one-directional flow (culvert)
				{
					NUMERIC_TYPE hu = surfaceElevation0 - weirs->Weir_hc[weir_id]; // upstream head
					NUMERIC_TYPE hd = surfaceElevation1 - weirs->Weir_hc[weir_id]; // downstream head
					if ((hd / hu) < weirs->Weir_m[weir_id])
						Q = weirs->Weir_Cd[weir_id] * weirs->Weir_w[weir_id] * pow(hu, (C(1.5))); // Free flow
					else
						Q = weirs->Weir_Cd[weir_id] * weirs->Weir_w[weir_id] * hu*(SQRT(hu - hd)) / SQRT(weirs->Weir_m[weir_id]); // Drowned flow
					NUMERIC_TYPE maxQ=((volume_grid[grid_index_this]/delta_time)*0.5);
					Q=getmin(Q,maxQ);				
}
			}
                                 
		}
		else if ((surfaceElevation0) < (surfaceElevation1))		// Flow in - direction
		{
			if ((surfaceElevation1) > weirs->Weir_hc[weir_id] && h1 > 0) // check depth is above weir and that the cell is wet
			{
				if (weirs->Weir_Fixdir[weir_id] == DirectionNA || weirs->Weir_Fixdir[weir_id] == dir_negative) // check for one-directional flow (culvert)
				{
					NUMERIC_TYPE hu = surfaceElevation1 - weirs->Weir_hc[weir_id]; // upstream head
					NUMERIC_TYPE hd = surfaceElevation0 - weirs->Weir_hc[weir_id]; // downstream head
					if ((hd / hu) < weirs->Weir_m[weir_id])
						Q = -weirs->Weir_Cd[weir_id] * weirs->Weir_w[weir_id] * pow(hu, (C(1.5))); // Free flow
					else
						Q = -weirs->Weir_Cd[weir_id] * weirs->Weir_w[weir_id] * hu*(SQRT(hu - hd)) / SQRT(weirs->Weir_m[weir_id]); // Drowned flow
						NUMERIC_TYPE maxQ=((volume_grid[grid_index_this]/delta_time)*0.5);
						Q=getmin(Q,maxQ);				
}
			}
		}
	}
	if (Q != C(0.0))
	{
		int x = weirs->cell_pair.sg_cell_x[weir_pair_id0];
		int y = weirs->cell_pair.sg_cell_y[weir_pair_id0];
		wet_dry_bounds->fp_vol[y].start = min(wet_dry_bounds->fp_vol[y].start, x);
		wet_dry_bounds->fp_vol[y].end = max(wet_dry_bounds->fp_vol[y].end, x + 1);
	}

	

	return(Q);
}


//-----------------------------------------------------------------------------------
// CALCULATE WIER FLOW BETWEEN A POINT AND ITS W NEIGHBOUR
inline NUMERIC_TYPE CalcBridgeQ(WeirLayout * weirs, const int grid_cols,
	const int grid_index_this, const int grid_index_next,
	const int weir_id,
	const NUMERIC_TYPE g, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE cell_length,
	const NUMERIC_TYPE * h_grid,
	WetDryRowBound* wet_dry_bounds,
	const SubGridState * sub_grid_state,
	const NUMERIC_TYPE max_Froude)
{
	NUMERIC_TYPE z0, z1, h0, h1, Q;
	NUMERIC_TYPE usVel; // MT upstream velocity for energy gradient height calc.
	NUMERIC_TYPE heg; // MT upstream energy gradient height
	//int p0, p1, pq0, weir_id;


	const int weir_pair_id0 = 2 * weir_id;
	const int weir_pair_id1 = weir_pair_id0 + 1;

	z0 = weirs->cell_pair.sg_cell_dem[weir_pair_id0];
	z1 = weirs->cell_pair.sg_cell_dem[weir_pair_id1];

	h0 = h_grid[grid_index_this];
	h1 = h_grid[grid_index_next];

	NUMERIC_TYPE surfaceElevation0 = h0 + z0;
	NUMERIC_TYPE surfaceElevation1 = h1 + z1;

	h0 += weirs->cell_pair.sg_cell_SGC_BankFullHeight[weir_pair_id0];
	h1 += weirs->cell_pair.sg_cell_SGC_BankFullHeight[weir_pair_id1];

	//int x = weirs->cell_pair.sg_cell_x[weir_pair_id0];
	//int y = weirs->cell_pair.sg_cell_y[weir_pair_id0];

	Q = C(0.0);
	if (h0 > depth_thresh || h1 > depth_thresh)
	{
		NUMERIC_TYPE Qoc; // open channel flow
		NUMERIC_TYPE Qp; // pressure(orifice) flow
		NUMERIC_TYPE Tz; // transit zone
		NUMERIC_TYPE Cd; // bridge Cd
		NUMERIC_TYPE Soffit; // bridge soffit elevation
		NUMERIC_TYPE Area; // bridge open area - precaclulated in input
		NUMERIC_TYPE Z; //bridge opening
		NUMERIC_TYPE Zratio; // flow depth to bridge opening ratio
		NUMERIC_TYPE Width; // bridge width
		NUMERIC_TYPE dh; // flow head change (for open channel flow calc)
		NUMERIC_TYPE Sf; // friction slope (for open channel flow calc)
		NUMERIC_TYPE hflow; // flow depth
		NUMERIC_TYPE A; // open channel flow area
		NUMERIC_TYPE R; // open channel hydraulic radius
		// get basic bridge params from global arrays;
		Tz = weirs->Weir_m[weir_id];
		Soffit = weirs->Weir_hc[weir_id];
		Cd = weirs->Weir_Cd[weir_id];
		Width = weirs->Weir_w[weir_id];

		// calculate some more bridge parameters from basic ones
		Z = getmin(Soffit - z1, Soffit - z0); // bridge opening (smallest opening)
		Area = Width*Z; // bridge flow area (again smallest opening)

		// get some basic paramters for the open channel flow calc
		NUMERIC_TYPE g_friction_sq = weirs->Weir_g_friction_sq[weir_id];// cn = C(0.5)* (SGCptr->SGCn[SGCgroup_grid[p0]] + SGCptr->SGCn[SGCgroup_grid[p1]]); // mean mannings (note n2)
		dh = surfaceElevation0 - surfaceElevation1; // difference in water level
		//Sf=-dh/Parptr->dx; //CCS_deletion
		Sf = -dh / cell_length; // CCS added for subgrid lat long compatibility.
		hflow = getmax(surfaceElevation0, surfaceElevation1) - getmax(z0, z1); // calculate the max flow depth
		A = Width*hflow; // calc open channel flow area
		R = A / (Width + 2 * hflow); // calc hydraulic radius from open channel flow

		// calculate open channel flow using SGC flow
		Qoc = CalculateQ(Sf, R, delta_time, g, A, g_friction_sq, weirs->Weir_Q_old_SG[weir_id], max_Froude);

		// orifice bridge flow
		if (surfaceElevation0 > surfaceElevation1) { // Positive flow
			Zratio = h0 / Z; // calc current flow depth to bridge opening ratio
			usVel = sub_grid_state->sg_flow_Q[weirs->Weir_pair_stream_flow_index[weir_pair_id0]] / (h0*weirs->cell_pair.sg_cell_SGC_width[weir_pair_id0]); // MT calculate upstream velocity
			heg = (usVel*usVel) / (2 * g); // MT calculate upstream energy gradient height
			Qp = Cd*Area*SQRT(2 * g*(surfaceElevation0 - surfaceElevation1 + heg)); // calc bridge orifice flow
		}
		else { // Negative flow
			Zratio = h1 / Z; // calc current flow depth to bridge opening ratio
			usVel = sub_grid_state->sg_flow_Q[weirs->Weir_pair_stream_flow_index[weir_pair_id1]] / (h1*weirs->cell_pair.sg_cell_SGC_width[weir_pair_id1]); // MT calculate upstream velocity
			heg = (usVel*usVel) / (2 * g); // MT calculate upstream energy gradient height
			Qp = -(Cd*Area*SQRT(2 * g*(surfaceElevation1 - surfaceElevation0 + heg))); // calc bridge orifice flow
		}


		if (surfaceElevation0 < Soffit && surfaceElevation1 < Soffit)
		{
			// flow is below the soffit so use SGC open channel flow 
			Q = Qoc;
		}
		else if (Zratio >= C(1.0) && Zratio <= Tz) // transition flow between open and orifice/pressure
		{
			Q = (Qoc*(Tz - Zratio) / (Tz - C(1.0))) + (Qp*(Zratio - C(1.0)) / (Tz - C(1.0)));
		}
		else if (Zratio > Tz) // pressure flow
		{
			Q = Qp;
		}
		else  // other flow - should not happen, but put here to catch other cases?
		{
			printf("WARNING: Unexpected Bridge flow calc fail at t=%.3" NUM_FMT" , Soffit %" NUM_FMT" m.\n", curr_time, Soffit); //Warning for fail
			Q = Qoc;
		}
	}
	if (Q != C(0.0))
	{
		int x = weirs->cell_pair.sg_cell_x[weir_pair_id0];
		int y = weirs->cell_pair.sg_cell_y[weir_pair_id0];
		wet_dry_bounds->fp_vol[y].start = min(wet_dry_bounds->fp_vol[y].start, x);
		wet_dry_bounds->fp_vol[y].end = max(wet_dry_bounds->fp_vol[y].end, x + 1);
	}
	return(Q);
}

/*
* Qx: next_cell_add = 1
* Qy: next_cell_add = grid_cols_padded
*/
void SGC2_UpdateVelocity_row(const int grid_row_index,
	const int row_start_prev, const int row_end_prev,
	const int row_start, const int row_end,
	const int next_cell_add,
	const NUMERIC_TYPE cell_width,
	const NUMERIC_TYPE * h_grid, const NUMERIC_TYPE * dem_grid, const NUMERIC_TYPE * Q_grid,
	NUMERIC_TYPE * V_grid, NUMERIC_TYPE * V_max_grid
	)
{

#pragma ivdep
#pragma simd
	// clear from start of prev bound to start of new bound
	for (int i = row_start_prev; i < row_start; i++)
	{
		int index_next = grid_row_index + i + next_cell_add;
		V_grid[index_next] = C(0.0);
	}
	if (row_end != -1)
	{
#pragma ivdep
#pragma simd
		// clear from end bound to end of prev bound
		for (int i = row_end; i < row_end_prev; i++)
		{
			int index_next = grid_row_index + i + next_cell_add;
			V_grid[index_next] = C(0.0);
		}
	}

#pragma simd
	for (int i = row_start; i < row_end; i++)
	{
		int index = grid_row_index + i;
		//next column
		int index_next = index + next_cell_add;

		// xiaodw, to jump the grids not in the basin
		if (dem_grid[index] == DEM_NO_DATA || dem_grid[index_next] == DEM_NO_DATA)
		{
			continue;
		}

		NUMERIC_TYPE velocity = SGC2_CalculateVelocity(index, index_next,
			Q_grid,
			h_grid, dem_grid, cell_width);
		V_grid[index_next] = velocity;
		V_max_grid[index_next] = getmax(FABS(velocity), V_max_grid[index_next]);
	}
}

/// update Q for weir flows
/// separate from update Q as the bridges depend on adjacent q values.
void SGC2_UpdateWeirsFlow_row(const int j, const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE delta_time,
	const NUMERIC_TYPE * h_grid, 
	const NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE * Qx_grid, 
	NUMERIC_TYPE * Qy_grid,
	WetDryRowBound * wet_dry_bounds,
	WeirLayout * weirs)
{
	const int weir_row_cols_padded = weirs->row_cols_padded;

	/// weirs are processed after UpdateQ completed
	/// bridges depend on q's upstream/downstream
	/// which if processed in updateQ, will never be updated in E->W, and randomly available in the case of N->S, S>N
	const int row_weir_start = j * weir_row_cols_padded;

	const int weir_Qx_row_count = weirs->weir_Qx_row_count[j];
	for (int weir_row_index = 0; weir_row_index < weir_Qx_row_count; weir_row_index++)
	{
		const int weir_id = weirs->weir_index_qx[weir_row_index + row_weir_start];

		const int grid_index_this = weirs->Weir_grid_index[weir_id];
		const int grid_index_next = grid_index_this + 1;

		NUMERIC_TYPE Q = CalcWeirQ(weirs, grid_cols,
			grid_index_this, grid_index_next, weir_id,
			depth_thresh, delta_time,
			h_grid, volume_grid,
			wet_dry_bounds, East, West);
		// update flow array
		weirs->Weir_Q_old_SG[weir_id] = Q;

		Qx_grid[grid_index_next] = Q;
	}
	const int weir_Qy_row_count = weirs->weir_Qy_row_count[j];
	for (int weir_row_index = 0; weir_row_index < weir_Qy_row_count; weir_row_index++)
	{
		const int weir_id = weirs->weir_index_qy[weir_row_index + row_weir_start];

		const int grid_index_this = weirs->Weir_grid_index[weir_id];
		const int grid_index_next = grid_index_this + grid_cols_padded;

		NUMERIC_TYPE Q = CalcWeirQ(weirs, grid_cols,
			grid_index_this, grid_index_next, weir_id,
			depth_thresh, delta_time,
			h_grid, volume_grid,
			wet_dry_bounds, South, North);
		// update flow array
		weirs->Weir_Q_old_SG[weir_id] = Q;

		Qy_grid[grid_index_next] = Q;
	}
}

/// update Q for weir flows
/// separate from update Q as the bridges depend on adjacent q values.
void SGC2_UpdateBridgesFlow_row(const int j, const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE g,
	const NUMERIC_TYPE * dx_col, const NUMERIC_TYPE * dy_col,
	const NUMERIC_TYPE * h_grid,
	NUMERIC_TYPE * Qx_grid, NUMERIC_TYPE * Qy_grid,
	WetDryRowBound * wet_dry_bounds,
	SubGridState * sub_grid_state,
	WeirLayout * weirs,
	const NUMERIC_TYPE max_Froude)
{
	const int weir_row_cols_padded = weirs->row_cols_padded;

	/// weirs are processed after UpdateQ completed
	/// bridges depend on q's upstream/downstream
	/// which if processed in updateQ, will never be updated in E->W, and randomly available in the case of N->S, S>N
	const int row_weir_start = j * weir_row_cols_padded;

	const int weir_Qx_row_count = weirs->weir_Qx_row_count[j];
	for (int weir_row_index = 0; weir_row_index < weir_Qx_row_count; weir_row_index++)
	{
		const int weir_id = weirs->weir_index_qx[weir_row_index + row_weir_start];

		const int grid_index_this = weirs->Weir_grid_index[weir_id];
		const int grid_index_next = grid_index_this + 1;

		NUMERIC_TYPE Q = CalcBridgeQ(weirs, grid_cols,
			grid_index_this, grid_index_next, weir_id,
			g, delta_time, curr_time, depth_thresh,
			dx_col[j], h_grid,
			wet_dry_bounds, sub_grid_state, max_Froude);
		// update flow array
		weirs->Weir_Q_old_SG[weir_id] = Q;

		Qx_grid[grid_index_next] = Q;
	}
	const int weir_Qy_row_count = weirs->weir_Qy_row_count[j];
	for (int weir_row_index = 0; weir_row_index < weir_Qy_row_count; weir_row_index++)
	{
		const int weir_id = weirs->weir_index_qy[weir_row_index + row_weir_start];

		const int grid_index_this = weirs->Weir_grid_index[weir_id];
		const int grid_index_next = grid_index_this + grid_cols_padded;

		NUMERIC_TYPE Q = CalcBridgeQ(weirs, grid_cols,
			grid_index_this, grid_index_next, weir_id,
			g, delta_time, curr_time, depth_thresh,
			dy_col[j], h_grid,
			wet_dry_bounds, sub_grid_state, max_Froude);
		// update flow array
		weirs->Weir_Q_old_SG[weir_id] = Q;

		Qy_grid[grid_index_next] = Q;
	}
}
// Dam Code FEOL 2016
void DamOpQ(const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE * h_grid, NUMERIC_TYPE * Qx_grid, NUMERIC_TYPE * Qy_grid, DamData *Damptr, const NUMERIC_TYPE g, const int ID)
{
	if (Damptr->DamOperationCode[ID] == 1)
	{
		Damptr->DamOperationQ[ID] = Damptr->DamMeanQ[ID];

		if (Damptr->DamOperationQ[ID] * delta_time > Damptr->DamVol[ID])
		{
			Damptr->DamOperationQ[ID] = Damptr->DamVol[ID] / delta_time;
		}
		
	}
	if (Damptr->DamOperationCode[ID] == 2) // Based off matlab code provided by Francesca Pianosi for constant release (regardless of storage)
	{
		NUMERIC_TYPE upper_limit = getmin((Damptr->Volmax[ID]/ delta_time), Damptr->DamMeanQ[ID]);
		NUMERIC_TYPE lower_limit = getmax((Damptr->Volmax[ID] - Damptr->DamVol[ID]), 0) / delta_time;
		Damptr->DamOperationQ[ID] = getmax(lower_limit,upper_limit);
		if (Damptr->DamOperationQ[ID] * delta_time > Damptr->DamVol[ID])
		{
			Damptr->DamOperationQ[ID] = Damptr->DamVol[ID]/delta_time;
		}
	}
	if (Damptr->DamOperationCode[ID] == 3) // Based off matlab code provided by Francesca Pianosi for releases kinearly proportional to of storage
	{
		NUMERIC_TYPE tmp = Damptr->DamMeanQ[ID] * ((Damptr->DamVol[ID] / Damptr->Volmax[ID]) + C(0.5)); // The Qmean flow is associated with 50% storage, tmp varies from 50% to 150% of Qmean
		NUMERIC_TYPE upper_limit = getmin((Damptr->Volmax[ID] / delta_time), tmp);
		NUMERIC_TYPE lower_limit = getmax((Damptr->DamVol[ID] - Damptr->Volmax[ID]), 0) / delta_time;
		Damptr->DamOperationQ[ID] = getmax(lower_limit, upper_limit);
		if (Damptr->DamOperationQ[ID] * delta_time > Damptr->DamVol[ID])
		{
			Damptr->DamOperationQ[ID] = Damptr->DamVol[ID] / delta_time;
		}
	}
	if (Damptr->DamOperationCode[ID] == 4) // Based off paper by Doll et al (2003). Used in WaterGAP
	{
		// Q= kS[S/Smax]^1.5, where k=0.01/d
		Damptr->DamOperationQ[ID] = (C(0.01) * Damptr->DamVol[ID] * pow((Damptr->DamVol[ID] / Damptr->Volmax[ID]), C(1.5))) / delta_time;
		if (Damptr->DamOperationQ[ID] * delta_time > Damptr->DamVol[ID])
		{
			Damptr->DamOperationQ[ID] = Damptr->DamVol[ID] / delta_time;
		}
	}
	if (Damptr->DamOperationCode[ID] == 5) // Based of Wada et al (2014). Used in PCR-GLOBWB
		//Q=(S-Smin)/(Smax-Smin)*Qmean; Smin=10% of Smax
	{
		Damptr->DamOperationQ[ID] = (((Damptr->DamVol[ID] - (Damptr->Volmax[ID] * C(0.2))) / (Damptr->Volmax[ID] * C(0.7))))*Damptr->DamMeanQ[ID]; // /delta_time;
		if (Damptr->DamOperationQ[ID] * delta_time > Damptr->DamVol[ID])
		{
			Damptr->DamOperationQ[ID] = Damptr->DamVol[ID] / delta_time;
		}
	}
	if (Damptr->DamOperationCode[ID] == 6) // Based of Wisser et al (2010). Used in WBMplus
	{
		NUMERIC_TYPE Kappa = C(0.16);
		NUMERIC_TYPE Lambda = C(0.6);

		Damptr->DamOperationQ[ID] = Kappa*(Damptr->DamVin[ID] / delta_time);

		if ((Damptr->DamVin[ID] / delta_time) < Damptr->DamMeanQ[ID])
		{
			Damptr->DamOperationQ[ID] = Lambda*(Damptr->DamVin[ID]/delta_time) + (Damptr->DamMeanQ[ID] - (Damptr->DamVin[ID]/delta_time));
		}

		if (Damptr->DamOperationQ[ID] * delta_time > Damptr->DamVol[ID])
		{
			Damptr->DamOperationQ[ID] = Damptr->DamVol[ID] / delta_time;
		}
	}
	if (Damptr->DamOperationCode[ID] == 7) // To be coded to follow the procedure of Hanasaki et al (2005). Used in HO8
	{
		NUMERIC_TYPE Alpha = C(0.85);
		NUMERIC_TYPE Storage = Damptr->Volmax[ID] / Damptr->DamMeanQ[ID];
		// Yearly Release
		// Do not need to caluclate yearly release only need to calculate Kappa which is the storage at beginning of year divided by alpha times max storage
		if (curr_time <= Damptr->DamYear[ID])
		{
			Damptr->OP7_Kappa[ID] = Damptr->DamVol[ID] / (Alpha*Damptr->Volmax[ID]);
			Damptr->DamYear[ID] += (C(365.0) * C(86400.0)); // corrected from 356 to 365, .0 added to prevent visual studio 2015 error
		}
		// Daily Release
		NUMERIC_TYPE DayRelease = Damptr->DamMeanQ[ID];

		// Switch
		Damptr->DamOperationQ[ID] = Damptr->OP7_Kappa[ID] *DayRelease;
		if (Storage < C(0.5))
		{
			Damptr->DamOperationQ[ID] = pow((Storage/0.5), 2)*Damptr->OP7_Kappa[ID] * DayRelease + (1 - pow((Storage/0.5), 2))*(Damptr->DamVin[ID] / delta_time);
		}

	}
}

void SGC2_UpdateDamFlowVolume(const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE * h_grid, NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE * Qx_grid, NUMERIC_TYPE * Qy_grid, DamData *Damptr, const SGCprams *SGCptr, const NUMERIC_TYPE g, const NUMERIC_TYPE max_Froude)
{
	
	delete[] Damptr->DamVin; // Needed to avoid memory leakage
	Damptr->DamVin = memory_allocate_zero_numeric_legacy(Damptr->NumDams);
	Damptr->DamMaxH = C(0.0);
	
	for (int i = 0; i < Damptr->TotalEdge; i++)
	{
	//	printf("Value of i: %d\n", i);
		fflush(stdout);
		const int grid_index_this = Damptr->DynamicEdge[2 * i];
		const int reservoir_index = Damptr->DynamicEdge[(2 * i) + 1];
		const int gr = Damptr->DynamicEdge[(2 * i) + 2];
		
		// Floodplain flowing into Dam
				//Calculate Q
				NUMERIC_TYPE h0 = h_grid[grid_index_this];
				
				NUMERIC_TYPE h1 = Damptr->InitialHeight[reservoir_index - 1];  //h1 is absolute height - not relative height(equivilent to h1 + dem[index])
				NUMERIC_TYPE z0 = Damptr->DynamicEdgeData[12 * i + 4];
				NUMERIC_TYPE zb0 = Damptr->DynamicEdgeData[12 * i + 6];
				//	NUMERIC_TYPE z1 = C(0.0); //Dam has infinite Depth NOT NEEDED!!!
				NUMERIC_TYPE friction = Damptr->DynamicEdgeData[12 * i + 3] * Damptr->DynamicEdgeData[12 * i + 3] * g;
				NUMERIC_TYPE friction1= Damptr->DynamicEdgeData[12 * i + 5] * Damptr->DynamicEdgeData[12 * i + 5] * g;
				NUMERIC_TYPE surface_elevation0 = z0 + h0;
				NUMERIC_TYPE surface_elevation1 = h1;
				NUMERIC_TYPE SGC_width = Damptr->DynamicEdgeData[12 * i + 11]; // SGC width
				NUMERIC_TYPE hflow = C(0.0);
				
				
				//
			
				Damptr->DynamicEdgeData[12 * i + 1] = C(0.0);
				NUMERIC_TYPE dh, surface_slope, A, R, deltaV;
					if (SGC_width > C(0.0)) //  check for sub-grid channel
					{
						hflow = getmax(surface_elevation0, surface_elevation1) - zb0;
						if (hflow > depth_thresh)
						{
							dh = surface_elevation1 - surface_elevation0;//, surface_elevation0 - Damptr->DynamicEdgeData[12 * i + 6]);
							surface_slope = dh / Damptr->DynamicEdgeData[12 * i + 8];
							//surface_slope = (-1 * abs(surface_slope));
							SGC2_CalcA(gr, hflow, Damptr->DynamicEdgeData[12 * i + 6], &A, &SGC_width, SGCptr); // calculate channel area for SGC
							R = SGC2_CalcR(gr, hflow, Damptr->DynamicEdgeData[12 * i + 6], SGC_width, SGC_width, A, SGCptr); // calculate hydraulic radius for SGC

							Damptr->DynamicEdgeData[12 * i + 7] = CalculateQ(surface_slope, R, delta_time, g, A, friction1, Damptr->DynamicEdgeData[12 * i + 7], max_Froude);
						}	// DynamicEdgeData{12i +7] = subgrid flow m3/s
						else Damptr->DynamicEdgeData[12 * i + 7] = C(0.0);
					}

					//hflow = getmax(getmax(surface_elevation1 - z0, h0), C(0.0));
					hflow = getmax(surface_elevation0, surface_elevation1) - z0;
					if (hflow > depth_thresh && SGC_width < Damptr->DynamicEdgeData[12 * i + 2])
					{
						dh = surface_elevation1 - surface_elevation0;
						surface_slope = dh / Damptr->DynamicEdgeData[12 * i + 8];
					//	surface_slope = (-1 * abs(surface_slope));
						// Set Cross-Section area equal to the min of cell size or flow cross section
						A = min(Damptr->DynamicEdgeData[12 * i + 2],Damptr->DynamicEdgeData[12*i+8]) * hflow;
						// calculate FP flow
						NUMERIC_TYPE Q;
						Q = CalculateQ(surface_slope, hflow, delta_time, g, A, friction, min(Damptr->DynamicEdgeData[12 * i + 2], Damptr->DynamicEdgeData[12 * i + 8]), max_Froude);
						Damptr->DynamicEdgeData[12 * i] = Q; // FP Flow in m3/s

						if (SGC_width > C(0.0))
						{
							NUMERIC_TYPE channel_ratio = min(SGC_width / min(Damptr->DynamicEdgeData[12 * i + 2], Damptr->DynamicEdgeData[12 * i + 8]), C(1.0));
							Q = Q - channel_ratio * Q;
						}
						
						Damptr->DynamicEdgeData[12*i+1] = Q; // DynamicEdgeData[12i+1] = TotalQ m3/s
					}
					else Damptr->DynamicEdgeData[12 * i] = C(0.0); //FP Q set to 0. 

					Damptr->DynamicEdgeData[12 * i + 1] += Damptr->DynamicEdgeData[12 * i + 7]; // Add Sub-grid Q to Total Q. m3/s

					deltaV = (Damptr->DynamicEdgeData[12 * i + 1] * delta_time);
					
					// Check Statement needed to ensure volume grid >= zero
					NUMERIC_TYPE DVratio = min(volume_grid[grid_index_this]/deltaV, C(1.0));;
					if (deltaV < C(0.0))
					{
						DVratio = C(1.0);
					
					}
					DVratio = C(1.0);
					volume_grid[grid_index_this] -= deltaV*DVratio;
					Damptr->DamVin[reservoir_index - 1] += deltaV*DVratio;
					Damptr->DynamicEdgeData[12 * i] = Damptr->DynamicEdgeData[12 * i] *DVratio;
					Damptr->DynamicEdgeData[12 * 7] = Damptr->DynamicEdgeData[12 * 7] *DVratio;

					Damptr->DamMaxH = getmax(Damptr->DamMaxH, hflow);
			}
	//		
	
	// Calculate Update Volume of Dam
	//NUMERIC_TYPE DamVol
	
	for (int d = 0; d < Damptr->NumDams; d++)
	{
		
		Damptr->DamOperationQ[d] = C(0.0);
		Damptr->DamTotalQ[d] = C(0.0);
		// Operational Outflows will go here;
		
		DamOpQ(delta_time,curr_time, h_grid, Qx_grid, Qy_grid, Damptr, g, d);
			Damptr->DamTotalQ[d] += Damptr->DamOperationQ[d];

		// Spill Way Calcs.
		if (Damptr->InitialHeight[d] <= Damptr->SpillHeight[d])
		{
			Damptr->SpillQ[d] = C(0.0);
		}
		else Damptr->SpillQ[d] = Damptr->Spill_Cd[d] * Damptr->SpillWidth[d] *pow(g,(C(0.5))) *pow((Damptr->InitialHeight[d] - Damptr->SpillHeight[d]), (C(1.5)));

		Damptr->DamTotalQ[d] += Damptr->SpillQ[d];

		// Volume and H update for Dams
		//		Mass balance for Dam is the change in reservoir storage
		Damptr->DamLoss = Damptr->DamLoss + Damptr->DamVol[d];
		// Update Reservoir Volume with the volume change in boundary cells
		Damptr->DamVol[d] += Damptr->DamVin[d];
		// Remove outflow from Reservoir Volume and Update DamLoss calculation
		Damptr->DamVol[d] -= (Damptr->DamTotalQ[d] * delta_time);
		Damptr->DamLoss = Damptr->DamLoss - Damptr->DamVol[d];
		//	Update Height of Reservoir
		Damptr->InitialHeight[d] = (Damptr->DamVol[d] / Damptr->Volmax[d])*Damptr->DamHeight[d] + (Damptr->SpillHeight[d] - Damptr->DamHeight[d]);

			// Output DamTotalQ to output cells
			//convert coordinates to grid cell numbers already done in input.cpp
		// convert to padded_cell_index
			int output = Damptr->OutputCellX[d] + Damptr->OutputCellY[d] * grid_cols_padded;
			volume_grid[output] += (Damptr->DamTotalQ[d] * delta_time);
	}
	
}

// Dam Code FEOL 2016 



/*
 * This function can be used to calculate the diffusive switch.
 * Can be used to handle either the Qx or Qy direction.
 * Qx: next_cell_add = 1
 * Qy: next_cell_add = grid_cols_padded
 * use appropriate Q_grid, Q_old_grid and g_friction_sq
 */
inline NUMERIC_TYPE SGC2_UpdateDiffusiveQ_row(const int grid_row_index, const int row_start, const int row_end,
	const NUMERIC_TYPE row_cell_length, const NUMERIC_TYPE row_cell_width,
	const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE froude_thresh, const NUMERIC_TYPE diffusive_max_hflow,
	const NUMERIC_TYPE g,
	const int next_cell_add, // qx next_cell_add = 1, qy next_cell_add = grid_cols_padded
	const NUMERIC_TYPE * h_grid,
	const NUMERIC_TYPE * dem_grid,
	const NUMERIC_TYPE * friction_grid,
	NUMERIC_TYPE * Q_grid, NUMERIC_TYPE * Q_old_grid)
{
#ifdef __INTEL_COMPILER
	__assume_aligned(h_grid, 64);
	__assume_aligned(dem_grid, 64);
	__assume_aligned(Q_grid, 64);
	__assume_aligned(Q_old_grid, 64);
	__assume_aligned(friction_grid, 64);
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
#endif

	int route_count = 0;
	for (int i = row_start; i < row_end; i++)
	{
		int index = grid_row_index + i;
		//next column
		int index_next = index + next_cell_add;
		// xiaodw, to jump the grids not in the basin
		if (dem_grid[index] == DEM_NO_DATA || dem_grid[index_next] == DEM_NO_DATA)
		{
			continue;
		}
		// if diffusive turns out to be better than routing - could use the tmp_row to store hflow instead of surface slope
		NUMERIC_TYPE h0 = h_grid[index];
		NUMERIC_TYPE h1 = h_grid[index_next];
		NUMERIC_TYPE z0 = dem_grid[index];
		NUMERIC_TYPE z1 = dem_grid[index_next];
		NUMERIC_TYPE surface_elevation0 = z0 + h0;
		NUMERIC_TYPE surface_elevation1 = z1 + h1;
		// Calculating hflow based on floodplain levels
		NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1)
			- getmax(z0, z1);

		if (hflow > depth_thresh)
		{
			NUMERIC_TYPE velocity = (Q_grid[index_next] / row_cell_width) / hflow;
			//NUMERIC_TYPE velocity = Velocity_grid[index_next];
			NUMERIC_TYPE froude = velocity / SQRT(g / hflow);

			/// hflow limiter - diffusive will be unstable if this is not used // TFD
			hflow = getmin(hflow, diffusive_max_hflow);

			if (FABS(froude) > froude_thresh)
			{
				// could be faster if using the regular 
				NUMERIC_TYPE fn = friction_grid[index_next];

				NUMERIC_TYPE Q;
				if (surface_elevation0 > surface_elevation1 && h0 > depth_thresh)
				{
					NUMERIC_TYPE dh = surface_elevation0 - surface_elevation1;
					NUMERIC_TYPE Sf = SQRT(dh / row_cell_length);
					Q = (POW(hflow, (C(5.0) / C(3.0)))*Sf*row_cell_width / fn);
				}
				else if (surface_elevation1 > surface_elevation0 && h1 > depth_thresh)
				{
					NUMERIC_TYPE dh = surface_elevation1 - surface_elevation0;
					NUMERIC_TYPE Sf = SQRT(dh / row_cell_length);
					Q = (-POW(hflow, (C(5.0) / C(3.0)))*Sf*row_cell_width / fn);
				}
				else
				{
					Q = C(0.0);
				}

				//printf("old %" NUM_FMT" new %" NUM_FMT"\n", Q_grid[index_next], Q);
				Q_grid[index_next] = Q;

				// if resetting to zero, the inertial model Q will change dramatically 
				// which in turn changes the froude causing jumping back and forward
				//Q_old_grid[index_next] = C(0.0);

				// update the Q_old with the diffusive Q
				// this will be the input to the inertial model
				Q_old_grid[index_next] = Q;

				// don't update Q_old_grid - the old inertial Q will be used
				// Q_old_grid[index_next] = Q_old_grid[index_next];
			}
			/*	else
				{
				printf("old %d %" NUM_FMT"\n",itCount, Q_grid[index_next]);
				}*/
		}
	}
	//printf("%d --\n", itCount);
	return route_count;
}

/*
// qx: next_cell_add = 1,
// qy: next_cell_add = grid_cols_padded
*/

inline NUMERIC_TYPE SGC2_UpdateRouteQ_row(const int grid_row_index, const int row_start, const int row_end,
	const NUMERIC_TYPE row_cell_area, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE route_slope_thresh,
	const int next_cell_add,
	const NUMERIC_TYPE * surface_slopes_row,
	const NUMERIC_TYPE * h_grid,
	const NUMERIC_TYPE * dem_grid,
	const NUMERIC_TYPE * route_V_ratio_per_sec_grid,
	NUMERIC_TYPE * Q_grid, NUMERIC_TYPE * Q_old_grid,
	int * route_list_i_lookup)
{
#ifdef __INTEL_COMPILER
	__assume_aligned(surface_slopes_row, 64);
	__assume_aligned(h_grid, 64);
	__assume_aligned(dem_grid, 64);
	__assume_aligned(route_V_ratio_per_sec_grid, 64);
	__assume_aligned(Q_grid, 64);
	__assume_aligned(Q_old_grid, 64);
	__assume_aligned(route_list_i_lookup, 64);
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
#endif

	int route_count = 0;
	for (int i = row_start; i < row_end; i++)
	{
		// always turn off the flood plain flow when slope above route_slope_thresh
		if (FABS(surface_slopes_row[i]) > route_slope_thresh)
		{
			int index = grid_row_index + i;
			//next column
			int index_next = index + next_cell_add;

			if (surface_slopes_row[i] > C(0.0) && route_V_ratio_per_sec_grid[index_next] > C(0.0))
			{
				NUMERIC_TYPE q_route = -CalculateRoutingQ(delta_time,
					h_grid[index_next], h_grid[index],
					dem_grid[index_next], dem_grid[index], route_V_ratio_per_sec_grid[index_next], row_cell_area);
#if defined (_DEBUG) && _DEBUG > 1
				//printf("[%d,%d] Route Q %" NUM_FMT" -> %" NUM_FMT"\n", i, lyr, Q_grid[index_next], q_route);
#endif
				Q_grid[index_next] = q_route;

				// 3 options for old Q -- Toby Dunne
				// - leave as inertial Q (could be unstable since switched)
				// - set to zero (sudden change could cause instability)
				// - set to the replaced Q ** chosen as this is the actual previous Q used, should result in more stable inertia
				Q_old_grid[index_next] = q_route;
				//Q_old_grid[index_next] = C(0.0);

				route_list_i_lookup[grid_row_index + route_count] = i;
				route_count++;
			}
			else if (surface_slopes_row[i] < C(0.0) && route_V_ratio_per_sec_grid[index_next] < C(0.0))
			{
				NUMERIC_TYPE q_route = CalculateRoutingQ(delta_time,
					h_grid[index], h_grid[index_next],
					dem_grid[index], dem_grid[index_next], -route_V_ratio_per_sec_grid[index_next], row_cell_area);
#if defined (_DEBUG) && _DEBUG > 1
				//printf("[%d,%d] Route Q %" NUM_FMT" -> %" NUM_FMT"\n", i, lyr, Q_grid[index_next], q_route);
#endif
				Q_grid[index_next] = q_route;

				// 3 options for old Q -- Toby Dunne
				// - leave as inertial Q (could be unstable since switched)
				// - set to zero (sudden change could cause instability)
				// - set to the replaced Q ** chosen as this is the actual previous Q used, should result in more stable inertia
				Q_old_grid[index_next] = q_route;
				//Q_old_grid[index_next] = C(0.0);

				route_list_i_lookup[grid_row_index + route_count] = i;
				route_count++;
			}
		}
	}
	return route_count;
}

///
/// checks each routing q
/// adds up all the out flow from the source cell
/// if the total outflow is greater than the cell volume, reduce the routing q
/// 
inline void SGC2_CorrectRouteFlow_row(const int j, const int grid_row_index, const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE delta_time,
	const RouteDynamicList * route_dynamic_list,
	const NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE* Qx_grid, NUMERIC_TYPE* Qy_grid)
{
	int route_count;
	route_count = route_dynamic_list->row_route_qx_count[j];
#pragma ivdep
	for (int route_index = 0; route_index < route_count; route_index++)
	{
		int i = route_dynamic_list->route_list_i_lookup_qx[route_index];
		int q_index = grid_row_index + i + 1;
		NUMERIC_TYPE q_route = Qx_grid[q_index];
		int source_cell_grid_index;
		if (q_route > 0) // route q is on the east face (source cell west)
		{
			source_cell_grid_index = grid_row_index + i;
			int flow_west = source_cell_grid_index; // west flow of source cell

			int flow_north = source_cell_grid_index;
			int flow_south = source_cell_grid_index + grid_cols_padded;

			NUMERIC_TYPE shallow_flow_out = C(0.0);
			shallow_flow_out -= (i > 0 && Qx_grid[flow_west] < C(0.0)) ? Qx_grid[flow_west] : C(0.0);
			shallow_flow_out -= getmin(Qy_grid[flow_north], C(0.0));
			shallow_flow_out += getmax(Qy_grid[flow_south], C(0.0));

			NUMERIC_TYPE total_out_flow;
			total_out_flow = shallow_flow_out + q_route; //+ east

			total_out_flow *= delta_time;
			if (total_out_flow > volume_grid[source_cell_grid_index])
			{
				q_route -= shallow_flow_out;
				q_route = getmax(q_route, C(0.0));
				//printf("routeQ %" NUM_FMT" shallow vol out: %" NUM_FMT" route => %" NUM_FMT" \n", Qx_grid[q_index], shallow_flow_out * delta_time, q_route);
				Qx_grid[q_index] = q_route;
			}
		}
		else //if (q_route < 0) // route q is on the west face (source cell east)
		{
			source_cell_grid_index = grid_row_index + i + 1;
			int flow_east = source_cell_grid_index + 1; // east flow of source cell

			int flow_north = source_cell_grid_index;
			int flow_south = source_cell_grid_index + grid_cols_padded;

			NUMERIC_TYPE shallow_flow_out = C(0.0);

			shallow_flow_out += (i < grid_cols - 2 && Qx_grid[flow_east] > C(0.0)) ? Qx_grid[flow_east] : C(0.0);
			shallow_flow_out -= getmin(Qy_grid[flow_north], C(0.0));
			shallow_flow_out += getmax(Qy_grid[flow_south], C(0.0));

			NUMERIC_TYPE total_out_flow;
			total_out_flow = shallow_flow_out - q_route; //- west

			total_out_flow *= delta_time;
			if (total_out_flow > volume_grid[source_cell_grid_index])
			{
				q_route += shallow_flow_out;
				q_route = getmin(q_route, C(0.0));
				//printf("routeQ %" NUM_FMT" shallow vol out: %" NUM_FMT" route => %" NUM_FMT" \n", Qx_grid[q_index], shallow_flow_out * delta_time, q_route);
				Qx_grid[q_index] = q_route;
				//total_out_flow = (shallow_flow_out - q_route) * delta_time;
				//printf("total out %" NUM_FMT" vol: %" NUM_FMT"\n", total_out_flow, volume_grid[source_cell_grid_index]);
			}
		}
	}

	route_count = route_dynamic_list->row_route_qy_count[j];
#pragma ivdep
	for (int route_index = 0; route_index < route_count; route_index++)
	{
		int i = route_dynamic_list->route_list_i_lookup_qy[route_index];
		int q_index = grid_row_index + i + grid_cols_padded;
		NUMERIC_TYPE q_route = Qy_grid[q_index];
		int source_cell_grid_index;
		if (q_route > 0) // route q is on south face (source cell north)
		{
			source_cell_grid_index = grid_row_index + i;
			int flow_north = source_cell_grid_index; // north face of the source cell

			int flow_west = source_cell_grid_index; // west face of source cell
			int flow_east = source_cell_grid_index + 1; // east face of source cell

			NUMERIC_TYPE shallow_flow_out = C(0.0);
			shallow_flow_out -= (j > 0 && Qy_grid[flow_north] < C(0.0)) ? Qy_grid[flow_north] : C(0.0);
			shallow_flow_out -= getmin(Qx_grid[flow_west], C(0.0));
			shallow_flow_out += getmax(Qx_grid[flow_east], C(0.0));

			NUMERIC_TYPE total_out_flow;
			total_out_flow = shallow_flow_out + q_route; //+ south

			total_out_flow *= delta_time;
			if (total_out_flow > volume_grid[source_cell_grid_index])
			{
				q_route -= shallow_flow_out;
				q_route = getmax(q_route, C(0.0));
				//printf("routeQ %" NUM_FMT" shallow: %" NUM_FMT" => %" NUM_FMT" \n", Qy_grid[q_index], shallow_flow_out, q_route);
				Qy_grid[q_index] = q_route;
			}
		}
		else //if (q_route < 0) //route is on north face (source cell south)
		{
			source_cell_grid_index = grid_row_index + i + grid_cols_padded;
			int flow_south = source_cell_grid_index + grid_cols_padded; // south face of the source cell

			int flow_west = source_cell_grid_index; // west face of source cell
			int flow_east = source_cell_grid_index + 1; // east face of source cell

			NUMERIC_TYPE shallow_flow_out = C(0.0);

			shallow_flow_out += (j < grid_rows - 2 && Qy_grid[flow_south] > C(0.0)) ? Qy_grid[flow_south] : C(0.0);
			shallow_flow_out -= getmin(Qx_grid[flow_west], C(0.0));
			shallow_flow_out += getmax(Qx_grid[flow_east], C(0.0));

			NUMERIC_TYPE total_out_flow;
			total_out_flow = shallow_flow_out - q_route; //- north

			total_out_flow *= delta_time;
			if (total_out_flow > volume_grid[source_cell_grid_index])
			{
				q_route += shallow_flow_out;
				q_route = getmin(q_route, C(0.0));
				//printf("routeQ %" NUM_FMT" shallow: %" NUM_FMT" => %" NUM_FMT" \n", Qy_grid[q_index], shallow_flow_out, q_route);
				Qy_grid[q_index] = q_route;
			}
		}
	}
}
// 计算sub grid河道上的流量
inline void ProcessSubGridQBlock(const int block_index, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE g,
	const SubGridRowList * sub_grid_layout, SubGridState * sub_grid_state, const SGCprams * SGCptr,
	NUMERIC_TYPE * h_grid,
	WetDryRowBound * wet_dry_bounds,
	NUMERIC_TYPE * Qx_grid, NUMERIC_TYPE * Qy_grid,
	NUMERIC_TYPE * Qx_old_grid, NUMERIC_TYPE * Qy_old_grid,
	const NUMERIC_TYPE max_Froude, NUMERIC_TYPE * Q_Ch_POI, NUMERIC_TYPE sgcStartH, States *Statesptr, NUMERIC_TYPE row_cell_area)
{
	const int * sg_pair_grid_index_lookup = sub_grid_layout->flow_info.flow_pair.sg_cell_grid_index_lookup;
	const NUMERIC_TYPE * sg_pair_SGC_BankFullHeight = sub_grid_layout->flow_info.flow_pair.sg_cell_SGC_BankFullHeight;
	const NUMERIC_TYPE * sg_pair_SGC_width = sub_grid_layout->flow_info.flow_pair.sg_cell_SGC_width;
	const NUMERIC_TYPE * sg_pair_dem = sub_grid_layout->flow_info.flow_pair.sg_cell_dem; // sg_cell_dem就是sub grid河道对应的DEM
	const int * sg_pair_SGC_group = sub_grid_layout->flow_info.flow_pair.sg_cell_SGC_group;

	//const NUMERIC_TYPE * sg_flow_cell_width = sub_grid_layout->flow_info.sg_flow_cell_width;
	// 
	NUMERIC_TYPE * sg_flow_Q = sub_grid_state->sg_flow_Q;
	//NUMERIC_TYPE * sg_flow_ChannelRatio = sub_grid_state->sg_flow_ChannelRatio;
	const NUMERIC_TYPE * sg_flow_effective_distance = sub_grid_layout->flow_info.sg_flow_effective_distance;
	const NUMERIC_TYPE * sg_flow_g_friction_sq = sub_grid_layout->flow_info.sg_flow_g_friction_sq;

	const int sg_row_start_index = block_index * sub_grid_layout->row_cols_padded;
	const int sg_row_pair_start_index = block_index * 2 * sub_grid_layout->row_cols_padded;
	const int flow_end = sub_grid_layout->flow_row_count[block_index];
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(sg_row_start_index % GRID_ALIGN_WIDTH == 0);
	__assume(sg_row_pair_start_index % GRID_ALIGN_WIDTH == 0);
#endif
#ifdef __INTEL_COMPILER
	__assume_aligned(sg_pair_SGC_BankFullHeight, 64);
	__assume_aligned(sg_pair_SGC_width, 64);
	__assume_aligned(sg_pair_dem, 64);
	__assume_aligned(sg_pair_grid_index_lookup, 64);
	__assume_aligned(h_grid, 64);
	__assume_aligned(Qx_grid, 64);
	__assume_aligned(Qy_grid, 64);
	__assume_aligned(Qx_old_grid, 64);
	__assume_aligned(Qy_old_grid, 64);
#endif

//#pragma ivdep
//#pragma simd
//	for (int flow_i = 0; flow_i < flow_end; flow_i++)
//	{
//		int flow_index = sg_row_start_index + flow_i;
//		int flow_pair_index = sg_row_pair_start_index + 2 * flow_i;
//		int flow_pair_index_next = flow_pair_index + 1;
//
//		int grid_index0 = sg_pair_grid_index_lookup[flow_pair_index];
//		int grid_index1 = sg_pair_grid_index_lookup[flow_pair_index_next]; // also the q index for this flow (in d4)
//
//		if (sgcStartH > 0)
//		{
//			h_grid[grid_index0] = sgcStartH;
//			h_grid[grid_index1] = sgcStartH;
//		}
//	}

#pragma ivdep
#pragma simd
	for (int flow_i = 0; flow_i < flow_end; flow_i++)
	{
		int flow_index = sg_row_start_index + flow_i;
		int flow_pair_index = sg_row_pair_start_index + 2 * flow_i;
		int flow_pair_index_next = flow_pair_index + 1;

		int grid_index0 = sg_pair_grid_index_lookup[flow_pair_index];
		int grid_index1 = sg_pair_grid_index_lookup[flow_pair_index_next]; // also the q index for this flow (in d4)

		//if ( sgcStartH > 0)
		//{
		//	h_grid[grid_index0] = sgcStartH;
		//	h_grid[grid_index1] = sgcStartH;
		//}

		NUMERIC_TYPE h0 = h_grid[grid_index0];
		NUMERIC_TYPE h1 = h_grid[grid_index1];

		NUMERIC_TYPE channel_Q = C(0.0);
		//NUMERIC_TYPE channel_ratio = C(0.0);

		NUMERIC_TYPE bankFullHeight0 = sg_pair_SGC_BankFullHeight[flow_pair_index];
		NUMERIC_TYPE bankFullHeight1 = sg_pair_SGC_BankFullHeight[flow_pair_index_next];

		if (h0 + bankFullHeight0 > depth_thresh ||
			h1 + bankFullHeight1 > depth_thresh)
		{
			//NUMERIC_TYPE cell_width = sg_flow_cell_width[flow_index];
			//NUMERIC_TYPE channel_area0, channel_area1;
			NUMERIC_TYPE channel_width0 = sg_pair_SGC_width[flow_pair_index];
			NUMERIC_TYPE channel_width1 = sg_pair_SGC_width[flow_pair_index_next];

			NUMERIC_TYPE z0 = sg_pair_dem[flow_pair_index];
			NUMERIC_TYPE z1 = sg_pair_dem[flow_pair_index_next];
			// z0: 河道所在的栅格单元高程，h0: 高出栅格单元高程以上的水深，surface_elevation0: 河水表面高程
			NUMERIC_TYPE surface_elevation0 = z0 + h0;
			NUMERIC_TYPE surface_elevation1 = z1 + h1;
			// Calculating hflow based on sub-channel elevation levels
			// bankFullHeight0: 河道深度，z0 - bankFullHeight0：河道底部高程，hflow：河水深度（包含河道内+河道之上）
			NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1)
				- getmax(z0 - bankFullHeight0, z1 - bankFullHeight1);
			// 计算subgrid河道流量时，是根据河水深度来的（包含河道内+河道之上）
			if (hflow > depth_thresh)
			{
				int channel_group0 = sg_pair_SGC_group[flow_pair_index];
				int channel_group1 = sg_pair_SGC_group[flow_pair_index_next];

				// SGC2_CalcA(channel_group0, hflow, bankFullHeight0, &channel_area0, &channel_width0, SGCptr);
				// SGC2_CalcA(channel_group1, hflow, bankFullHeight1, &channel_area1, &channel_width1, SGCptr);

				NUMERIC_TYPE area;
				NUMERIC_TYPE R;

				// always use smallest flow area
				// PFU, change to using smallest channel width
				// 计算sub grid channel断面的面积
				if (channel_width0 < channel_width1) // select the appropriate slope and area based on the smallest area
				{
					// calculate hydraulic radius for SGC
					SGC2_CalcA(channel_group0, hflow, bankFullHeight0, &area, &channel_width0, SGCptr);
					R = SGC2_CalcR(channel_group0, hflow, bankFullHeight0, channel_width0, sg_pair_SGC_width[flow_pair_index], area, SGCptr);

				}
				else
				{
					// calculate hydraulic radius for SGC
					SGC2_CalcA(channel_group1, hflow, bankFullHeight1, &area, &channel_width1, SGCptr);
					R = SGC2_CalcR(channel_group1, hflow, bankFullHeight1, channel_width1, sg_pair_SGC_width[flow_pair_index_next], area, SGCptr);
				}

				NUMERIC_TYPE effective_distance = sg_flow_effective_distance[flow_index];
				NUMERIC_TYPE g_friction_squared = sg_flow_g_friction_sq[flow_index];

				NUMERIC_TYPE dh = (surface_elevation0)-(surface_elevation1);
				NUMERIC_TYPE surface_slope = -dh / effective_distance;
				channel_Q = CalculateQ(surface_slope, R, delta_time, g, area, g_friction_squared, sg_flow_Q[flow_index], max_Froude);
			}
		}
		sg_flow_Q[flow_index] = channel_Q;
		//if (Statesptr->save_poi == ON)
		//{
		//	Q_Ch_POI[grid_index0] += channel_Q * delta_time  / row_cell_area;   // mm
		//}
		//sg_flow_ChannelRatio[flow_index] = getmin(channel_ratio, C(1.0)); //PFU set constant channel ratio in lisflood_processing

		// Update wet-dry 
		if (channel_Q != C(0.0))
		{
			int x = sub_grid_layout->flow_info.flow_pair.sg_cell_x[flow_pair_index];
			int y = sub_grid_layout->flow_info.flow_pair.sg_cell_y[flow_pair_index];
			wet_dry_bounds->fp_vol[y].start = min(wet_dry_bounds->fp_vol[y].start, x);
			wet_dry_bounds->fp_vol[y].end = max(wet_dry_bounds->fp_vol[y].end, x + 1);
		}
	}
}

void SGC2_UpdateVelocitySubGrid_block(const int block_index, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE delta_time,
	const SubGridRowList * sub_grid_layout, SubGridState * sub_grid_state, const SGCprams * SGCptr,
	const NUMERIC_TYPE * h_grid)
{
	const int * sg_pair_grid_index_lookup = sub_grid_layout->flow_info.flow_pair.sg_cell_grid_index_lookup;
	const NUMERIC_TYPE * sg_pair_dem = sub_grid_layout->flow_info.flow_pair.sg_cell_dem;
	const NUMERIC_TYPE * sg_pair_SGC_BankFullHeight = sub_grid_layout->flow_info.flow_pair.sg_cell_SGC_BankFullHeight;
	const NUMERIC_TYPE * sg_pair_SGC_width = sub_grid_layout->flow_info.flow_pair.sg_cell_SGC_width;
	const int * sg_pair_SGC_group = sub_grid_layout->flow_info.flow_pair.sg_cell_SGC_group;

	NUMERIC_TYPE * sg_flow_Q = sub_grid_state->sg_flow_Q;
	NUMERIC_TYPE * sg_velocity = sub_grid_state->sg_velocity;

	const int sg_row_start_index = block_index * sub_grid_layout->row_cols_padded;
	const int sg_row_pair_start_index = block_index * 2 * sub_grid_layout->row_cols_padded;
	const int flow_end = sub_grid_layout->flow_row_count[block_index];

#pragma ivdep
#pragma simd
	for (int flow_i = 0; flow_i < flow_end; flow_i++)
	{
		int flow_index = sg_row_start_index + flow_i;

		NUMERIC_TYPE q = sg_flow_Q[flow_index];
		if (FABS(q) > depth_thresh)
		{
			int flow_pair_index = sg_row_pair_start_index + 2 * flow_i;
			int flow_pair_index_next = flow_pair_index + 1;

			int grid_index0 = sg_pair_grid_index_lookup[flow_pair_index];
			int grid_index1 = sg_pair_grid_index_lookup[flow_pair_index_next]; // also the q index for this flow (in d4)

			NUMERIC_TYPE h0 = h_grid[grid_index0];
			NUMERIC_TYPE h1 = h_grid[grid_index1];
			NUMERIC_TYPE z0 = sg_pair_dem[flow_pair_index];
			NUMERIC_TYPE z1 = sg_pair_dem[flow_pair_index_next];
			NUMERIC_TYPE surface_elevation0 = z0 + h0;
			NUMERIC_TYPE surface_elevation1 = z1 + h1;
			NUMERIC_TYPE bankFullHeight0 = sg_pair_SGC_BankFullHeight[flow_pair_index];
			NUMERIC_TYPE bankFullHeight1 = sg_pair_SGC_BankFullHeight[flow_pair_index_next];

			// Calculating hflow based on sub-channel elevation levels
			NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1)
				- getmax(z0 - bankFullHeight0, z1 - bankFullHeight1);
			if (hflow > depth_thresh)
			{
				NUMERIC_TYPE channel_area0, channel_area1;
				NUMERIC_TYPE channel_width0 = sg_pair_SGC_width[flow_pair_index];
				NUMERIC_TYPE channel_width1 = sg_pair_SGC_width[flow_pair_index_next];

				int channel_group0 = sg_pair_SGC_group[flow_pair_index];
				int channel_group1 = sg_pair_SGC_group[flow_pair_index_next];

				SGC2_CalcA(channel_group0, hflow, bankFullHeight0, &channel_area0, &channel_width0, SGCptr);
				SGC2_CalcA(channel_group1, hflow, bankFullHeight1, &channel_area1, &channel_width1, SGCptr);

				NUMERIC_TYPE channel_area = getmin(channel_area0, channel_area1);

				sg_velocity[flow_index] = q / channel_area;
			}
		}
	}
}

inline void SGC2_UpdateQx_row(const int grid_cols,
	const int grid_row_index,
	const int row_start_x_prev, const int row_end_x_prev,
	const int row_start_x, const int row_end_x,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE row_dx, const NUMERIC_TYPE * Fp_ywidth,
	const NUMERIC_TYPE g, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time,
	NUMERIC_TYPE * tmp_row,
	const NUMERIC_TYPE * dem_grid, const NUMERIC_TYPE *h_grid,

	const NUMERIC_TYPE * g_friction_sq_x_grid,
	NUMERIC_TYPE *Qx_grid, NUMERIC_TYPE *Qx_old_grid,
	const NUMERIC_TYPE max_Froude)
{
#if defined (_DEBUG) && _DEBUG > 1

	// checking only
	int check_end = min(grid_cols - 1, row_start_x_prev);
	for (int i = 0; i < check_end; i++)
	{
		int index = grid_row_index + i;
		int index_next = index + 1;
		if (Qx_old_grid[index_next] != C(0.0))
			printf("Error: Qx %" NUM_FMT" @ %d (%d)  \n", Qx_old_grid[index_next], index_next, i);
	}
	// checking only
	if (row_end_x_prev != -1)
		for (int i = row_end_x_prev; i < grid_cols - 1; i++)
		{
			int index = grid_row_index + i;
			int index_next = index + 1;
			if (Qx_old_grid[index_next] != C(0.0))
				printf("Error: Qx %" NUM_FMT" @ %d (%d)  \n", Qx_old_grid[index_next], index_next, i);
		}

#endif

#ifdef __INTEL_COMPILER
	__assume_aligned(h_grid, 64);
	__assume_aligned(dem_grid, 64);
	__assume_aligned(g_friction_sq_x_grid, 64);
	__assume_aligned(Qx_grid, 64);
	__assume_aligned(Qx_old_grid, 64);
	__assume_aligned(Fp_ywidth, 64);
	__assume_aligned(tmp_row, 64);
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
#endif

#pragma ivdep
#pragma simd
	// clear from start of prev bound to start of new bound
	for (int i = row_start_x_prev; i < row_start_x; i++)
	{
		int index = grid_row_index + i;
		//next row
		int index_next = index + 1;
		Qx_grid[index_next] = C(0.0);
		Qx_old_grid[index_next] = C(0.0);
	}
	if (row_end_x != -1)
	{
#pragma ivdep
#pragma simd
		// clear from end bound to end of prev bound
		for (int i = row_end_x; i < row_end_x_prev; i++)
		{
			int index = grid_row_index + i;
			//next row
			int index_next = index + 1;
			Qx_grid[index_next] = C(0.0);
			Qx_old_grid[index_next] = C(0.0);
		}
	}

	// Calculate Qx (base model)
#pragma ivdep
#pragma simd // note this pragma is here to hint the compiler that this should be vectorized - the compiler will warn if not vectorized
	for (int i = row_start_x; i < row_end_x; i++)
	{
		int index = grid_row_index + i;

		//next column
		int index_next = index + 1;

		// xiaodw, to jump the grids not in the basin
		if (dem_grid[index] == DEM_NO_DATA || dem_grid[index_next] == DEM_NO_DATA)
		{
			continue;
		}

		// -ve h where sub grid channel
		//NUMERIC_TYPE h0 = getmax(h_grid[index], C(0.));
		//NUMERIC_TYPE h1 = getmax(h_grid[index_next], C(0.));
		NUMERIC_TYPE h0 = h_grid[index];
		NUMERIC_TYPE h1 = h_grid[index_next];
		NUMERIC_TYPE z0 = dem_grid[index];
		NUMERIC_TYPE z1 = dem_grid[index_next];

		NUMERIC_TYPE surface_elevation0 = z0 + h0;
		NUMERIC_TYPE surface_elevation1 = z1 + h1;
		// Calculating hflow based on floodplain levels
		NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1)
			- getmax(z0, z1);
		NUMERIC_TYPE q_tmp, surface_slope;
		if (hflow > depth_thresh)// && (h0 > depth_thresh || h1 > depth_thresh))
		{
			//NUMERIC_TYPE area = (row_dy)* hflow;
			// PFU use floodplain width corrected for sub grid channel ratio rather than cell width
			// todo1：验证是否可能是负值？
			// 计算flood plain的流量
			// flow_info->sg_flow_ChannelRatio[flow_index] = getmin(width0,width1)/grid_cell_width;
			// Fp_ywidth是1 - sg_flow_ChannelRatio就很重要了，sg_flow_ChannelRatio是 河宽/栅格宽度，如果sg_flow_ChannelRatio>cell width，Fp_ywidth可能出现负值;
			// 在InitSubGridStructureByRows方法中进行了特殊处理，当河道宽度>栅格单元宽度(即Fp_ywidth<0)时，令Fp_ywidth=0
			NUMERIC_TYPE area = Fp_ywidth[index_next]* hflow;
			NUMERIC_TYPE dh = (surface_elevation0)-(surface_elevation1);
			surface_slope = -dh / row_dx;
			q_tmp = CalculateQ(surface_slope, hflow, delta_time, g, area, g_friction_sq_x_grid[index_next], Qx_old_grid[index_next], max_Froude);
			//if (Fp_ywidth[index_next] <= 0.0)
			//{
			//	cout << "area: " << area << " dh: " << dh << " surface_slope: " << surface_slope << " q_tmp: " << q_tmp << endl;
			//}
		}
		else
		{
			surface_slope = C(0.0);
			q_tmp = C(0.0);
		}
		Qx_old_grid[index_next] = q_tmp;
		tmp_row[i] = surface_slope;
	}
	int count = row_end_x - row_start_x;
	if (count > 0)
		memcpy(Qx_grid + grid_row_index + row_start_x + 1, Qx_old_grid + grid_row_index + row_start_x + 1, sizeof(NUMERIC_TYPE) * count);
}

inline void SGC2_UpdateQy_row(const int grid_cols,
	const int grid_row_index,
	const int row_start_y_prev, const int row_end_y_prev,
	const int row_start_y, const int row_end_y,
	const int next_cell_add,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE * Fp_xwidth, const NUMERIC_TYPE row_dy,
	const NUMERIC_TYPE g, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time,
	NUMERIC_TYPE * tmp_row,
	const NUMERIC_TYPE * dem_grid, const NUMERIC_TYPE *h_grid,

	const NUMERIC_TYPE * g_friction_sq_y_grid,
	NUMERIC_TYPE *Qy_grid, NUMERIC_TYPE *Qy_old_grid,
	const NUMERIC_TYPE max_Froude)
{
#if defined (_DEBUG) && _DEBUG > 1
	// checking only
	for (int i = 0; i < row_start_y_prev; i++)
	{
		int index = grid_row_index + i;
		int index_next = index + next_cell_add;
		if (Qy_old_grid[index_next] != C(0.0))
			printf("Error: Qy %" NUM_FMT" @ %d (%d)  \n", Qy_old_grid[index_next], index_next, i);
	}
	// checking only
	if (row_end_y_prev != -1)
		for (int i = row_end_y_prev; i < grid_cols; i++)
		{
			int index = grid_row_index + i;
			int index_next = index + next_cell_add;
			if (Qy_old_grid[index_next] != C(0.0))
				printf("Error: Qy %" NUM_FMT" @ %d (%d)  \n", Qy_old_grid[index_next], index_next, i);
		}
#endif

#ifdef __INTEL_COMPILER
	__assume_aligned(h_grid, 64);
	__assume_aligned(dem_grid, 64);
	__assume_aligned(g_friction_sq_y_grid, 64);
	__assume_aligned(Qy_grid, 64);
	__assume_aligned(Qy_old_grid, 64);
	__assume_aligned(Fp_xwidth, 64);
	__assume_aligned(tmp_row, 64);
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
	__assume(next_cell_add % GRID_ALIGN_WIDTH == 0);
#endif

#pragma ivdep
#pragma simd
	// clear from start of prev bound to start of new bound
	for (int i = row_start_y_prev; i < row_start_y; i++)
	{
		//next row
		int index_next = grid_row_index + i + next_cell_add;
		Qy_grid[index_next] = C(0.0);
		Qy_old_grid[index_next] = C(0.0);
	}
	if (row_end_y != -1)
	{
#pragma ivdep
#pragma simd
		// clear from end bound to end of prev bound
		for (int i = row_end_y; i < row_end_y_prev; i++)
		{
			//next row
			int index_next = grid_row_index + i + next_cell_add;
			Qy_grid[index_next] = C(0.0);
			Qy_old_grid[index_next] = C(0.0);
		}
	}
	// Calculate Qy (base model)
#pragma ivdep
#pragma simd
	for (int i = row_start_y; i < row_end_y; i++)
	{
		int index = grid_row_index + i;
		//next row
		int index_next = index + next_cell_add;

		// xiaodw, to jump the grids not in the basin
		if (dem_grid[index] == DEM_NO_DATA || dem_grid[index_next] == DEM_NO_DATA)
		{
			continue;
		}
		//NUMERIC_TYPE h0 = getmax(h_grid[index], C(0.));
		//NUMERIC_TYPE h1 = getmax(h_grid[index_next], C(0.));
		NUMERIC_TYPE h0 = h_grid[index];
		NUMERIC_TYPE h1 = h_grid[index_next];
		NUMERIC_TYPE z0 = dem_grid[index];
		NUMERIC_TYPE z1 = dem_grid[index_next];

		NUMERIC_TYPE surface_elevation0 = z0 + h0;
		NUMERIC_TYPE surface_elevation1 = z1 + h1;
		// Calculating hflow based on floodplain levels
		// 计算河道上底之上的水流流量
		NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1)
			- getmax(z0, z1);
		NUMERIC_TYPE q_tmp, surface_slope;
		if (hflow > depth_thresh)// && (h0 > depth_thresh || h1 > depth_thresh))
		{
			//NUMERIC_TYPE area = (row_dx)* hflow;
			// PFU use floodplain width corrected for sub grid channel ratio rather than cell width
			NUMERIC_TYPE area = Fp_xwidth[index_next]* hflow;
			NUMERIC_TYPE dh = (surface_elevation0)-(surface_elevation1);
			surface_slope = -dh / row_dy;
			//if (Fp_xwidth[index_next] <= 0.0)
			//{
			//	cout << "area: " << area << " dh: " << dh << " surface_slope: " << surface_slope <<  endl;
			//}
			q_tmp = CalculateQ(surface_slope, hflow, delta_time, g, area, g_friction_sq_y_grid[index_next], Qy_old_grid[index_next], max_Froude);

		}
		else
		{
			surface_slope = C(0.0);
			q_tmp = C(0.0);
		}
		Qy_old_grid[index_next] = q_tmp;
		tmp_row[i] = surface_slope;
	}
	int count = row_end_y - row_start_y;
	if (count > 0)
		memcpy(Qy_grid + grid_row_index + row_start_y + next_cell_add, Qy_old_grid + grid_row_index + row_start_y + next_cell_add, sizeof(NUMERIC_TYPE) * count);
}

	//-----------------------------------------------------------------------------
	// FLOODPLAIN DISTRIBUTED INFILTRATION
	// with correction for sub grid channels
	inline NUMERIC_TYPE SGC2_Infil_floodplain_row(
		const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		const NUMERIC_TYPE row_cell_area,
		const NUMERIC_TYPE evap_deltaH_step,
		const NUMERIC_TYPE * h_row,
		const NUMERIC_TYPE * infil_row,
		NUMERIC_TYPE * volume_row, Pars *Parptr, const Solver *Solverptr)
	{
#ifdef __INTEL_COMPILER
		__assume_aligned(h_row, 64);
		__assume_aligned(volume_row, 64);
#endif

		NUMERIC_TYPE reduce_infil_loss = C(0.0);
#pragma ivdep
#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			NUMERIC_TYPE h_new, dV = C(0.0);
			NUMERIC_TYPE h_old = h_row[i];
			NUMERIC_TYPE evap_deltaV_step = infil_row[i] * row_cell_area *  Solverptr->SGCtmpTstep;
			if (h_old > depth_thresh) // There is water to evaporate on the flood plain
			{
				// update depth by subtracting evap depth
				// 新水深 = 旧水深 - 下渗深度
				h_new = h_old - infil_row[i] * Solverptr->SGCtmpTstep;
				//check for -ve depths
				// 水全部下渗
				if (h_new < C(0.0))
				{
					// reduce evap loss to account for dry bed (don't go below 0)
					dV = h_old * row_cell_area;
				}
				// 水部分下渗
				else
				{
					dV = evap_deltaV_step;
				}
				// 扣除下渗水量
				volume_row[i] -= dV;
			}
			reduce_infil_loss += dV; //mass-balance for a standard cell
		}
		return reduce_infil_loss;
	}

	inline NUMERIC_TYPE SGC2_Infil_floodplain_row_constant(
		const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		const NUMERIC_TYPE row_cell_area,
		const NUMERIC_TYPE * dem_row,
		const NUMERIC_TYPE evap_deltaH_step,
		const NUMERIC_TYPE * h_row,
		const NUMERIC_TYPE infil_row,
		NUMERIC_TYPE *soil_water_depth_row,
		NUMERIC_TYPE * volume_row, Pars *Parptr, 
		const Solver *Solverptr, NUMERIC_TYPE* Infilt_Row, 
		const int grid_row_index,const int sg_row_start,const int cell_row_count,
		const int * sg_cell_grid_index_lookup, const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight,
		const NUMERIC_TYPE * sg_cell_cell_area
	)
	{
#ifdef __INTEL_COMPILER
		__assume_aligned(h_row, 64);
		__assume_aligned(volume_row, 64);
#endif

		NUMERIC_TYPE reduce_infil_loss = C(0.0);

#pragma ivdep
#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			int grid_index = grid_row_index + i;
			if (dem_row[i] != DEM_NO_DATA) {
				NUMERIC_TYPE h_new, dV = C(0.0);
				// 加上当前步长内的降雨深度后，再扣除下渗
				//NUMERIC_TYPE h_old = h_row[i];
				NUMERIC_TYPE h_old = h_row[i] + volume_row[i] / row_cell_area;
				NUMERIC_TYPE infil_deltaV_step = infil_row * row_cell_area *  Solverptr->SGCtmpTstep;
				//if (h_old > depth_thresh) // There is water to infiltrate on the flood plain
				//{

				// floodplain或虽然是河道但是水位于floodplain之上，保证h_old为正值
				if (h_old > 0.0)
				{
					// 如果蓄满了，则此栅格不再下渗
					if (soil_water_depth_row[i] >= Parptr->saturation_value)
					{
						continue;
					}
					// 新水深 = 旧水深 - 下渗深度
					h_new = h_old - infil_row * Solverptr->SGCtmpTstep;
					//check for -ve depths
					// 水全部下渗
					if (h_new < C(0.0))
					{
						dV = h_old * row_cell_area;
						Infilt_Row[i] = h_old;
					}
					// 水部分下渗
					else
					{
						dV = infil_deltaV_step;
						Infilt_Row[i] = infil_deltaV_step / row_cell_area;
					}
					// 累计下渗深度
					soil_water_depth_row[i] += Infilt_Row[i];
					// 扣除栅格单元上的下渗水量
					volume_row[i] -= dV;
					reduce_infil_loss += dV; //mass-balance for a standard cell
				}
				//else
				//{
				//	int cell_end = cell_row_count;
				//	for (int cell_i = 0; cell_i < cell_end; cell_i++)
				//	{
				//		const int cell_index = sg_row_start + cell_i;
				//		//int x = sg_cell_x[cell_index];
				//		//int y = sg_cell_y[cell_index];
				//		int grid_index_from_lookup = sg_cell_grid_index_lookup[cell_index];// x + y * grid_cols_padded;
				//		// 如果是在河道上，且水面低于河道上底
				//		if (h_old < 0.0 && grid_index_from_lookup == grid_index)
				//		{
				//			NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
				//			h_old += sg_cell_SGC_BankFullHeight[cell_index];
				//			if (h_old >= 0.0)
				//			{
				//				// 新水深 = 旧水深 - 下渗深度
				//				h_new = h_old - infil_row * Solverptr->SGCtmpTstep;
				//				//check for -ve depths
				//				// 水全部下渗
				//				if (h_new < C(0.0))
				//				{
				//					// reduce evap loss to account for dry bed (don't go below 0)
				//					dV = h_old * cell_area;
				//					Infilt_Row[i] = h_old;
				//				}
				//				// 水部分下渗
				//				else
				//				{
				//					dV = infil_deltaV_step;
				//					Infilt_Row[i] = infil_deltaV_step / cell_area;
				//				}
				//				// 扣除栅格单元上的下渗水量
				//				volume_row[i] -= dV;
				//				reduce_infil_loss += dV; //mass-balance for a standard cell
				//			}

				//		}
				//	}
				//}

				//}


			}

		}
		// 注释掉模型原有逻辑，改为不管格子上有多少水，都要下渗
		//	if (h_old > depth_thresh) // There is water to infiltrate on the flood plain
		//	{
		//		// update depth by subtracting evap depth
		//		// 新水深 = 旧水深 - 下渗深度
		//		h_new = h_old - infil_row * Solverptr->SGCtmpTstep;
		//		//check for -ve depths
		//		// 水全部下渗
		//		if (h_new < C(0.0))
		//		{
		//			// reduce evap loss to account for dry bed (don't go below 0)
		//			dV = h_old * row_cell_area;
		//		}
		//		// 水部分下渗
		//		else
		//		{
		//			dV = infil_deltaV_step;
		//		}
		//		// 扣除栅格单元上的下渗水量
		//		volume_row[i] -= dV;
		//		Infilt_Row[i] = dV;
		//	}
		//	reduce_infil_loss += dV; //mass-balance for a standard cell
		//}
		return reduce_infil_loss;
	}
	// sgc channel should be processed specificly
	inline NUMERIC_TYPE SGC2_floodplain_perclation_singlelayer(
		const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		const NUMERIC_TYPE row_cell_area,
		const NUMERIC_TYPE groundwater_tstep,
		const NUMERIC_TYPE * h_row,
		//const NUMERIC_TYPE * infil_row,
		NUMERIC_TYPE * volume_row,
		Pars *Parptr, const Solver *Solverptr, int grid_row_index,int j, int start_y,int end_y,
		NUMERIC_TYPE *PercolationVol, int  sumNCells, NUMERIC_TYPE * sumGndQ2Rch, NUMERIC_TYPE * GwStorageDepth) {
		if (row_end - row_start > 0) {
			//***********************percolation************************
			for (int i = row_start; i < row_end; i++)
			{
				int index = i + grid_row_index;
				float moisture = Parptr->soilMoisturePD[index];
				Parptr->rechargePD[index] = 0.f;
				if (moisture > Parptr->fieldCapacityPD[index]) {
					// the water exceeds the porosity is added to percolation directly
					if (moisture > Parptr->porosityPD[index]) {
						Parptr->rechargePD[index] += (moisture - Parptr->porosityPD[index]) * Parptr->rootDepthPD[index];
						Parptr->soilMoisturePD[index] = Parptr->porosityPD[index];
					}

					// recharge capacity (mm)
					float dcIndex = 2.f + 2.f / Parptr->poreIndexPD[index]; // pore disconnectedness index
					//float rechargeCap = m_Conductivity[i] / 3600.f * m_timestep * pow((moisture - m_Residual[i])/temp, dcIndex);
					float rechargeCap =
						Parptr->ksPD[index] / 3600.f * groundwater_tstep * pow(moisture / Parptr->porosityPD[index], dcIndex); //Campbell, 1974
					float availableWater = (Parptr->soilMoisturePD[index] - Parptr->fieldCapacityPD[index]) * Parptr->rootDepthPD[index];
					if (rechargeCap >= availableWater) {
						rechargeCap = availableWater;
					}

					Parptr->rechargePD[index] += rechargeCap;
					Parptr->soilMoisturePD[index] -= Parptr->rechargePD[index] / Parptr->rootDepthPD[index];
				}
			}
			//***********************groundwater linear resovior************************
			// make all grid cells as an alone resovior 
			// get percolation for each subbasin
			//if (lyr == start_y)
			//{
			//	Parptr->PercolationVol = 0.f;
			//	Parptr->sumNCells = 0;
			//	Parptr->sumGndQ2Rch = 0.f;
			//	for (int i = row_start; i < row_end; i++) {
			//		int index = i + grid_row_index;
			//		Parptr->PercolationVol += Parptr->rechargePD[index];
			//	}
			//	Parptr->sumNCells += (row_end - row_start);
			//}
			if (j < end_y - 1)
			{
				for (int i = row_start; i < row_end; i++) {
					int index = i + grid_row_index;
					Parptr->PercolationVol += Parptr->rechargePD[index];
				}
				//Parptr->sumNCells += (row_end - row_start);
			}
			// xiaodw, calculate groundwater Q after the last row
			if (j == end_y - 1)
			{

				for (int i = row_start; i < row_end; i++) {
					int index = i + grid_row_index;
					Parptr->PercolationVol += Parptr->rechargePD[index];
				}
				//Parptr->sumNCells += (row_end - row_start);
				// xiaodw, average percolation mm of the basin
				float percolation = Parptr->PercolationVol * (1.f - Parptr->deepCoefficient) / Parptr->sumNCells;
				// depth of groundwater runoff(mm)
				float outFlowDepth = Parptr->recessionCoefficient * pow(Parptr->GwStorageDepth, Parptr->recessionExponent);
				// groundwater flow out of the subbasin at time t (m3/s)
				Parptr->sumGndQ2Rch += outFlowDepth / 1000.f * Parptr->sumNCells * Parptr->dx * Parptr->dy / groundwater_tstep;

				// water balance (mm)
				Parptr->GwStorageDepth += percolation - outFlowDepth;
			}
			

			// make each grid cell as an alone resovior 
			/*
			for (int i = row_start; i < row_end; i++) {
				int index = i + grid_row_index;
				Parptr->percolationPD[index] = Parptr->rechargePD[index];
				float percolation = Parptr->percolationPD[index] * (1.f - Parptr->deepCoefficient);
				// depth of groundwater runoff(mm)
				float outFlowDepth = Parptr->recessionCoefficient * pow(Parptr->gwStoragePD[index], Parptr->recessionExponent);
				// groundwater flow out of the subbasin at time t (m3/s)
				Parptr->gndQ2RchPD[index] = outFlowDepth / 1000.f * Parptr->dx * Parptr->dy / percolation_tstep;
				Parptr->sumGndQ2Rch = Parptr->sumGndQ2Rch + Parptr->gndQ2RchPD[index];

				// water balance (mm)
				Parptr->gwStoragePD[index] += percolation - outFlowDepth;
				Parptr->GwStorageDepth = Parptr->GwStorageDepth + Parptr->gwStoragePD[index];
				Parptr->PercolationVol = Parptr->PercolationVol + percolation;
			}
			*/


			


		}
		

	}

	inline NUMERIC_TYPE SGC2_floodplain_perclation_singlelayer(
		const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		const NUMERIC_TYPE percolation_tstep,
		Pars *Parptr,  int grid_row_index, const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area) {

		NUMERIC_TYPE recharge_vol_row = 0.0;
		if (row_end - row_start > 0) {
			//***********************percolation************************
			for (int i = row_start; i < row_end; i++)
			{
				int index = i + grid_row_index;
				if (dem_row[i] != DEM_NO_DATA) {
					float moisture = Parptr->soilMoisturePD[index];
					Parptr->rechargePD[index] = 0.f;
					// fieldCapacityPD(decimal,eg:0.2)
					if (moisture > Parptr->fieldCapacityPD[index]) {
						// the water exceeds the porosity is added to percolation directly
						// xiaodw, 当土壤超饱和时，土壤水也不会在一个时间步长上全部渗漏到地下水
						//if (moisture > Parptr->porosityPD[index]) {
						//	// rootDepthPD cm -> mm, rechargePD mm
						//	Parptr->rechargePD[index] += (moisture - Parptr->porosityPD[index]) * Parptr->rootDepthPD[index] * 10.f;
						//	Parptr->soilMoisturePD[index] = Parptr->porosityPD[index];
						//}

						// recharge capacity (mm)
						float dcIndex = 3.f + 2.f / Parptr->poreIndexPD[index]; // pore disconnectedness index
						// ksPD mm/h -> mm/s, rechargeCap mm
						//float rechargeCap = Parptr->ks_factor * Parptr->ksPD[index] / 3600.f * percolation_tstep * pow(moisture / Parptr->porosityPD[index], dcIndex); //Campbell, 1974
						// xiaodw modify, Parptr->soilMoisturePD[index] has subtracted the water exceeds the porosity, here just calculate water in the soil   
						float rechargeCap = Parptr->ks_factor *  Parptr->ksPD[index] / 3600.f * percolation_tstep * pow((Parptr->soilMoisturePD[index] - Parptr->fieldCapacityPD[index]) / (Parptr->porosityPD[index] - Parptr->fieldCapacityPD[index]), dcIndex); //Campbell, 1974
						 // mm
						float availableWater = (Parptr->soilMoisturePD[index] - Parptr->fieldCapacityPD[index]) * Parptr->rootDepthPD[index] * 10.f;
						if (rechargeCap >= availableWater) {
							rechargeCap = availableWater;
						}
						// mm 
						Parptr->rechargePD[index] += rechargeCap;
						// xiaodw modify, the excess water has been subtracted from soil moisture, shouldn't be subtracted again here.
						Parptr->soilMoisturePD[index] -= Parptr->rechargePD[index] / (Parptr->rootDepthPD[index] * 10.f);
						Parptr->soilWaterDepth[index] -= Parptr->rechargePD[index];
					}
				}
			}
			//***********************groundwater linear resovior************************
			// make all grid cells as an alone resovior 
			// get percolation for each subbasin

			// xiaodw, calculate groundwater Q after the last row
			// m3
			for (int i = row_start; i < row_end; i++) {
				int index = i + grid_row_index;
				if (dem_row[i] != DEM_NO_DATA) {
					recharge_vol_row += Parptr->rechargePD[index] / 1000.0 * row_cell_area;
				}
			}
		}
		return recharge_vol_row;
	}

	inline NUMERIC_TYPE SGC2_floodplain_perclation_multilayer(
		const int row_start, int row_end,const NUMERIC_TYPE depth_thresh,const NUMERIC_TYPE percolation_tstep,
		Pars *Parptr, const Solver *Solverptr, NUMERIC_TYPE * volume_grid, int grid_row_index, const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area) {

		NUMERIC_TYPE recharge_vol_row = 0.0;
		if (row_end - row_start > 0) {
			for (int row_i = row_start; row_i < row_end; row_i++)
			{
				int i = row_i + grid_row_index;
				if (dem_row[row_i] != DEM_NO_DATA) {
					// Note that, infiltration, pothole seepage, irrigation etc. have been added to
					// the first soil layer in other modules. By LJ
					float excessWater = 0.f, maxSoilWaterDepth = 0.f, fcSoilWaterDepth = 0.f;
					for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++) {
						// 每一层超出土壤孔隙度和田间持水量的水, 孔隙度水当量，田间持水量 mm
						excessWater = 0.f;
						maxSoilWaterDepth = Parptr->multi_soilPorosityPD[lyr][i] * Parptr->multi_soilThicknessPD[lyr][i] * 1000.0;
						fcSoilWaterDepth = Parptr->multi_soilFcPD[lyr][i] * Parptr->multi_soilThicknessPD[lyr][i] * 1000.0;
						Parptr->multi_soilWaterDepthPD[lyr][i] = Parptr->multi_soilMoisturePD[lyr][i] * Parptr->multi_soilThicknessPD[lyr][i] * 1000.0;
						// determine gravity drained water in layer
						excessWater += Parptr->multi_soilWaterDepthPD[lyr][i] - fcSoilWaterDepth;

						maxSoilWaterDepth = Max(0.f, maxSoilWaterDepth);
						// mm
						Parptr->multi_soilPercoPD[lyr][i] = 0.f;
						// No movement if soil moisture is below field capacity
						if (excessWater > 1.e-5f) {
							float maxPerc = maxSoilWaterDepth - fcSoilWaterDepth;
							if (maxPerc < 0.f) maxPerc = 0.1f;
							// ks is mm/h, convert it to mm/s firstly, this means how much time it needs to percolate all water in this layer
							float tt = 3600.f * maxPerc / Parptr->multi_soilKsPD[lyr][i];
							// 每次根据比例渗漏一部分，其余的下个步长渗漏
							//Parptr->multi_soilPercoPD[lyr][i] = excessWater * (1.f - exp(-Solverptr->SGCtmpTstep / tt)); // secs
							Parptr->multi_soilPercoPD[lyr][i] = excessWater * (1.f - exp(-percolation_tstep / tt)); // secs

							if (Parptr->multi_soilPercoPD[lyr][i] > maxPerc) {
								Parptr->multi_soilPercoPD[lyr][i] = maxPerc;
							}
							//Adjust the moisture content in the current layer, and the layer immediately below it
							// 每一层都会向下一层渗漏,mm
							Parptr->multi_soilWaterDepthPD[lyr][i] -= Parptr->multi_soilPercoPD[lyr][i];
							excessWater -= Parptr->multi_soilPercoPD[lyr][i];
							Parptr->multi_soilWaterDepthPD[lyr][i] = Max(0.0, Parptr->multi_soilWaterDepthPD[lyr][i]);
							// redistribute soil water if above field capacity (high water table), rewrite from sat_excess.f of SWAT
							if (lyr < Parptr->multi_nSoilLyrs - 1) {
								Parptr->multi_soilWaterDepthPD[lyr + 1][i] += Parptr->multi_soilPercoPD[lyr][i];
								Parptr->multi_soilPercoDepOfLyr[lyr] += Parptr->multi_soilPercoPD[lyr][i];
								Parptr->multi_soilPercoVolOfLyr[lyr] += Parptr->multi_soilPercoPD[lyr][i] * 0.001 * row_cell_area;

								// 如果上一层超饱和了，则超饱和的水全部渗漏到下一层
								//if (Parptr->multi_soilWaterDepthPD[lyr][i] - maxSoilWaterDepth > 1.e-4f) {
								//	Parptr->multi_soilWaterDepthPD[lyr + 1][i] += Parptr->multi_soilWaterDepthPD[lyr][i] - maxSoilWaterDepth;
								//	Parptr->multi_soilWaterDepthPD[lyr][i] = Parptr->multi_soilPorosityPD[lyr][i];
								//}

							}
							else {
								/// for the last soil layer
								/// 最后一层优先渗漏到地下水
								recharge_vol_row += Parptr->multi_soilPercoPD[lyr][i] * 0.001 * row_cell_area;
								Parptr->multi_soilPercoDepOfLyr[lyr] += Parptr->multi_soilPercoPD[lyr][i];
								Parptr->multi_soilPercoVolOfLyr[lyr] += Parptr->multi_soilPercoPD[lyr][i] * 0.001 * row_cell_area;
								Parptr->multi_soilWaterDepthPD[lyr][i] -= Parptr->multi_soilPercoPD[lyr][i];
								/// 如果最后一层超饱和，则超饱和的量会逐层向上层补给
								if (Parptr->multi_soilWaterDepthPD[lyr][i] - maxSoilWaterDepth > 1.e-4f) {
									float ul_excess = Parptr->multi_soilWaterDepthPD[lyr][i] - maxSoilWaterDepth;
									Parptr->multi_soilWaterDepthPD[lyr][i] = maxSoilWaterDepth;
									// xiaodw，最后一层饱和后又反过来检查上面每一层是否因为增加了渗漏量而超饱和
									for (int ly = Parptr->multi_nSoilLyrs - 2; ly >= 0; ly--) {
										Parptr->multi_soilWaterDepthPD[i][ly] += ul_excess;
										NUMERIC_TYPE tmp_maxSoilWaterDepth = Parptr->multi_soilPorosityPD[lyr][i] * Parptr->multi_soilThicknessPD[lyr][i] * 1000.0;
										if (Parptr->multi_soilWaterDepthPD[lyr][i] > tmp_maxSoilWaterDepth) {
											ul_excess = Parptr->multi_soilWaterDepthPD[lyr][i] - tmp_maxSoilWaterDepth;
											Parptr->multi_soilWaterDepthPD[lyr][i] = Parptr->multi_soilPorosityPD[lyr][i];
										}
										else {
											ul_excess = 0.f;
											break;
										}
										// 如果第一层都超饱和了，则更新第一层的入渗量，并将超饱和的水转入地表径流
										if (ly == 0 && ul_excess > 0.f) {

											volume_grid[i] += ul_excess * 0.001 * row_cell_area;
											//m_infil[i] -= ul_excess;
										}
									}
								}
							}

						}
						else {
							Parptr->multi_soilPercoPD[lyr][i] = 0.f;
						}
						// update soilMoisture
						Parptr->multi_soilMoisturePD[lyr][i] = Parptr->multi_soilWaterDepthPD[lyr][i] / (Parptr->multi_soilThicknessPD[lyr][i] * 1000.0);
					}
					/// update soil profile water
					Parptr->multi_soilWtrStoPrfl[i] = 0.f;
					for (int ly = 0; ly < Parptr->multi_nSoilLyrs; ly++) {
						Parptr->multi_soilWtrStoPrfl[i] += Parptr->multi_soilWaterDepthPD[ly][i];
					}
				}
			}
		}
		return recharge_vol_row;
	}

	inline NUMERIC_TYPE SGC2_Infil_floodplain_row_green_ampt(
		const int row_start, int row_end, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE row_cell_area, const NUMERIC_TYPE evap_deltaH_step,
		const NUMERIC_TYPE * h_row, const NUMERIC_TYPE * dem_row, NUMERIC_TYPE* Infilt_Row_POI, NUMERIC_TYPE *soil_water_depth_row,
		//const NUMERIC_TYPE * infil_row,
		NUMERIC_TYPE * volume_row, NUMERIC_TYPE * porosityPD, NUMERIC_TYPE * initSoilMoisturePD, NUMERIC_TYPE * capillarySuctionPD,
		NUMERIC_TYPE * ksPD, NUMERIC_TYPE ksFactor, NUMERIC_TYPE * accumuDepthPD, NUMERIC_TYPE * rootDepthPD, NUMERIC_TYPE * infilPD,
		NUMERIC_TYPE * infilCapacitySurplusPD, NUMERIC_TYPE * soilMoisturePD, NUMERIC_TYPE * soil_water_depth_Row_POI,
		Pars *Parptr, const Solver *Solverptr, const States *Statesptr, int grid_row_index, NUMERIC_TYPE *infilAvgBlock, int *infilValidCount)
	{
#ifdef __INTEL_COMPILER
		__assume_aligned(h_row, 64);
		__assume_aligned(volume_row, 64);
#endif
		//*infilAvgRow = C(0.0);
		//*infilAccRow = C(0.0);
		NUMERIC_TYPE infil_loss = C(0.0);
		//int counter = 0;
		NUMERIC_TYPE rowToalInfil = C(0.0);
#pragma ivdep
#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				NUMERIC_TYPE h_new, dV = C(0.0);
				//NUMERIC_TYPE h_old = h_row[i]; // m
				NUMERIC_TYPE h_old = h_row[i] + volume_row[i] / row_cell_area;
				//NUMERIC_TYPE evap_deltaV_step = infil_row[i] * row_cell_area;

				//**************************************start green-ampt***********************************
				int index = i + grid_row_index;
				// effective matric potential (m), capillarySuctionPD mm->m
				float matricPotential = (porosityPD[index] -initSoilMoisturePD[index]) *capillarySuctionPD[index] / 1000.f;
				//float matricPotential = (Parptr->porosityPD[index] - Parptr->initSoilMoisturePD[index]) *Parptr->capillarySuctionPD[index] / 1000.f;
				// algorithm of Li, 1996, uesd in C2SC2D
				float ks = ksPD[index] / 1000.f / 3600.f; // mm/h -> m/s
				if (ksFactor > 0.0)
				{
					ks = ks * ksFactor;
				}
				//ks = 0.0000001;
				float dt = Solverptr->SGCtmpTstep;
				float infilDepth = accumuDepthPD[index] / 1000.f; // mm ->m

				float p1 = (float)(ks * dt - 2.0 * infilDepth);
				float p2 = ks * (infilDepth + matricPotential);
				// infiltration rate (m/s) 根据累计入渗深度计算出来的当前时间步长的下渗能力（最大下渗速率）
				float infilRate = (float)((p1 + sqrt(pow(p1, 2.0f) + 8.0f * p2 * dt)) / (2.0f * dt));
				//infilCap是当前时间步长土壤还能容纳的最大入渗最大深度(m)
				// rootDepthPD cm->mm
				// todo: 这里是否应该soilMoisturePD*porosityPD？避免湿度大于孔隙度的情况
				float infilCap = (porosityPD[index] - soilMoisturePD[index]) * rootDepthPD[index] * 1000.f;
				if (infilRate >= 0.0)
				{
					//xiaodw, 改为只要有水就入渗
					if (h_old > 0.0) {               
					//if (h_old > depth_thresh) {
						//for saturation overland flow
						// 当前的土壤湿度 > 土壤孔隙度，代表土壤水饱和了，则不下渗了；土壤湿度会随着下渗量的增加而增加
						if (soilMoisturePD[i] > porosityPD[index]) {
							infilPD[index] = 0.0f;
							infilCapacitySurplusPD[index] = 0.f;

						}
						else {
							// 取较小值的意思是，如果入渗能力*时间步长 < 最大入渗深度，就代表在当前时间步长下，土壤还能容纳这么多水的下渗（入渗能力*时间步长）
							// 如果入渗能力*时间步长 > 最大入渗深度，就代表在当前时间步长下，土壤已经容纳不了这么多水的下渗（入渗能力*时间步长）
							infilPD[index] = min(infilRate * dt * 1000.f, infilCap); // mm

							//check if the infiltration potential exceeds the available water
							if (infilPD[index] > h_old * 1000.0) {
								infilCapacitySurplusPD[index] = infilPD[index] - h_old * 1000.0;
								//limit infiltration rate to available water supply
								infilPD[index] = h_old * 1000;
							}
							else {
								infilCapacitySurplusPD[index] = 0.f;
							}

							//Compute the cumulative depth of infiltration
							accumuDepthPD[index] += infilPD[index];  // mm

							// rootDepthPD cm -> mm
							if (rootDepthPD != NULL) {
								soilMoisturePD[index] += infilPD[index] / (rootDepthPD[index] * 1000.f);
							}
						}
						// 下渗量
						dV = infilPD[index] / 1000.0 * row_cell_area;   // m
						volume_row[i] -= dV;
						
						// 输出下渗量
						rowToalInfil += infilPD[index];
						//*infilAvgRow += infilPD[index];
						//*infilAccRow += accumuDepthPD[index];
						// todo : 为什么输出的AccDep是负值？
					}
					else {
						infilPD[index] = 0.0;
					}
					
				}
				else
				{
					infilPD[index] = 0.0;
				}
				*infilAvgBlock = *infilAvgBlock + infilPD[index];
				(*infilValidCount)++;

				//soil_water_depth_row[i]+= infilPD[index];
				soil_water_depth_row[i] = soilMoisturePD[index] * rootDepthPD[index] * 1000.0;
				// POI
				if (Statesptr->save_poi)
				{
					Infilt_Row_POI[i] += infilPD[index];   // mm
					soil_water_depth_Row_POI[i] = soilMoisturePD[index] * rootDepthPD[index] * 1000.0;
				}
				//Parptr->soilWaterDepthAvgPercell += soil_water_depth_row[i];
				infil_loss += dV; //mass-balance for a standard cell

			}

			//**************************************end green-ampt***********************************

		}
		//if (counter > 0)
		//{
		//	*infilAvgRow = rowToalInfil / counter;
		//	*infilAccRow = rowToalInfil / counter;
		//}
		return infil_loss;
	}

	inline NUMERIC_TYPE cal_infil_wetspa(Pars* Parptr, const Solver *Solverptr,NUMERIC_TYPE h_old,int index, NUMERIC_TYPE * soilMoisturePD, NUMERIC_TYPE * porosityPD,
		NUMERIC_TYPE *rootDepthPD,NUMERIC_TYPE *infilPD, NUMERIC_TYPE area) {
		NUMERIC_TYPE dV = 0.0;
		NUMERIC_TYPE infilCap = (porosityPD[index] - soilMoisturePD[index]) * rootDepthPD[index] * 1000.f;
		NUMERIC_TYPE pMaxStep = Parptr->pMax * Solverptr->SGCtmpTstep * 1.1574074074074073e-05f;   // mm/day -> mm this step
		NUMERIC_TYPE alpha = 1.0;
		if (Parptr->useAlphaType == 1)
		{
			alpha = Parptr->alpha;
		}
		else {
			alpha = Parptr->kRun - (Parptr->kRun - 1) * h_old / pMaxStep;
			if (h_old >= pMaxStep)
				alpha = 1.0;
		}


		//runoff percentage
		NUMERIC_TYPE runoffPercentage;
		NUMERIC_TYPE runoffCo = Parptr->useRunoffCoType == VALUE_TYPE ? Parptr->runoffCoVal : Parptr->runoffCoPD[index];

		if (runoffCo > 0.99f)
			runoffPercentage = 1.0f;
		else
			runoffPercentage = Parptr->runoffCoFactor * runoffCo * pow((soilMoisturePD[index] / porosityPD[index]), alpha);   /// xiaodw, 20250409, 增加径流系数的调节因子

		if (runoffPercentage < 0 || runoffPercentage > 1) runoffPercentage = 1.0f;

		NUMERIC_TYPE surfq = h_old * runoffPercentage; // mm
		infilPD[index] = h_old - surfq;  // mm

		infilPD[index] = min(infilPD[index], infilCap); // mm

		//check if the infiltration potential exceeds the available water
		if (infilPD[index] > h_old) {
			//limit infiltration rate to available water supply
			infilPD[index] = h_old;
		}
		//if (!IsNumber(infilPD[index]) || isinf(infilPD[index]) || !IsNumber(Parptr->rainExcessPD[index]) || isinf(Parptr->rainExcessPD[index]))
		//{
		//	ostringstream oss;
		//	oss << "The infiltration or runoff in cell (" << index << ") is out of reasonable range!" << endl;
		//	oss << "m_infil[i]: " << infilPD[index] << endl;
		//	oss << "m_pe[i]: " << Parptr->rainExcessPD[index] << endl;
		//	oss << "pNet: " << h_old << endl;
		//	oss << "surfq: " << surfq << endl;
		//	oss << "runoffPercentage: " << runoffPercentage << endl;
		//	oss << "runoffCo: " << runoffCo << endl;
		//	oss << "m_soilMoisture[i]: " << soilMoisturePD[index] << endl;
		//	oss << "porosity: " << porosityPD[index] << endl;
		//	oss << "alpha: " << alpha << endl;
		//	oss << "m_kRunoff: " << Parptr->kRun << endl;
		//	oss << "m_pMax: " << Parptr->pMax << endl;
		//}
		dV = infilPD[index] * area * 0.001;
		return dV;
	}


	/* from wetspa SUR_MR   */
	inline NUMERIC_TYPE SGC2_Infil_floodplain_row_wetspa(
		const int row_start, int row_end, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE row_cell_area, 
		const NUMERIC_TYPE * h_grid, const NUMERIC_TYPE * dem_grid, NUMERIC_TYPE* Infilt_Row_POI, NUMERIC_TYPE* Infilt_Ch_POI, NUMERIC_TYPE *soil_water_depth_row,
		NUMERIC_TYPE * volume_row, NUMERIC_TYPE *  volume_row_ch, NUMERIC_TYPE * porosityPD,
		 NUMERIC_TYPE * rootDepthPD, NUMERIC_TYPE * infilPD, NUMERIC_TYPE * infilChPD,
		 NUMERIC_TYPE * soilMoisturePD, NUMERIC_TYPE * soil_water_depth_Row_POI,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, const States *Statesptr, int grid_row_index, NUMERIC_TYPE *infilAvgBlock, int *infilValidCount,int j,
		const int cell_count, const int sg_row_start, const int * sg_cell_grid_index_lookup, const NUMERIC_TYPE * sg_cell_cell_area,
		const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume, const NUMERIC_TYPE * sg_cell_SGC_c,
		const SubGridFlowLookup * sg_cell_flow_lookup)
	{
#ifdef __INTEL_COMPILER
		__assume_aligned(h_row, 64);
		__assume_aligned(volume_row, 64);
#endif
		NUMERIC_TYPE infil_loss = C(0.0);
		NUMERIC_TYPE rowToalInfil = C(0.0);
#pragma ivdep
#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			int index = grid_row_index + i;
			if (dem_grid[index] != DEM_NO_DATA) {
				int source_index_this = j * Parptr->xsz + i;
				if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
					continue;
				}

				NUMERIC_TYPE h_new, dV = C(0.0);
				NUMERIC_TYPE h_old = (h_grid[index] + volume_row[i] / row_cell_area) * 1000.0;  // m->mm
				if (h_old > 0)
				{
					//for saturation overland flow
				    if (soilMoisturePD[index] >= porosityPD[index])
					{
						Parptr->rainExcessPD[index] = h_old;
						infilPD[index] = 0.0f;
					}
					else
					{
						dV = cal_infil_wetspa(Parptr,Solverptr, h_old, index, soilMoisturePD, porosityPD, rootDepthPD, infilPD,row_cell_area);

						Parptr->rainExcessPD[index] = h_old - infilPD[index];
						// 从地表水量中减去
						volume_row[i] -= dV;
						// 加入土壤水分
						soilMoisturePD[index] += infilPD[index] / (rootDepthPD[index] * 1000.f);
						soil_water_depth_row[i] = soilMoisturePD[index] * rootDepthPD[index] * 1000.f;
						//soil_water_depth_row[i] += infilPD[index];
					}
				}
				else
				{
					Parptr->rainExcessPD[index] = 0.0f;
					infilPD[index] = 0.0f;
				}

				// POI
				if (Statesptr->save_poi)
				{
					Infilt_Row_POI[i] += infilPD[index];   // mm
					soil_water_depth_Row_POI[i] = soilMoisturePD[index] * rootDepthPD[index] * 1000.0;
				}
				*infilAvgBlock = *infilAvgBlock + infilPD[index];
				(*infilValidCount)++;
				infil_loss += dV; //mass-balance for a standard cell
			}
		}

		for (int cell_i = 0; cell_i < cell_count; cell_i++)
		{
			int cell_index = sg_row_start + cell_i;
			
			int grid_index = sg_cell_grid_index_lookup[cell_index];
			int i = grid_index - grid_row_index;
			NUMERIC_TYPE dV = 0.0, dVCh = 0.0, dVFp = 0.0;
			//NUMERIC_TYPE dVFp2Ch = 0.0
			// 上个时间步长结束时的水深
			const NUMERIC_TYPE h_prev = h_grid[grid_index];
			// // 河道所在的栅格单元面积
			const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
			NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index] * 1000.0;
			NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];  // SGC河道单元底面积

			NUMERIC_TYPE fp_area = row_cell_area - SGC_c;
			//NUMERIC_TYPE h_old = h_grid[grid_index] * 1000;   // mm
			NUMERIC_TYPE h_old_fp = h_grid[grid_index] > 0.0? (h_grid[grid_index] + volume_row[i] / fp_area) * 1000.0 : volume_row[i] / fp_area * 1000.0;  // m->mm
			NUMERIC_TYPE h_old_ch = (h_grid[grid_index] + volume_row_ch[i] / SGC_c) * 1000.0 + SGC_BankFullHeight;  // m->mm

			//*************先计算蓄洪区的入渗******************
			if (h_old_fp > 0.0) {
				//for saturation overland flow
				if (soilMoisturePD[grid_index] >= porosityPD[grid_index])
				{
					Parptr->rainExcessPD[grid_index] = h_old_fp;
					infilPD[grid_index] = 0.0f;
					dVFp = 0.0;
				}
				else
				{
					// 如果蓄洪区的水没能入渗完，则剩余的水:
					// 1)来自降雨-蒸发剩余的水会保留在volume_row里，最终会用来更新河道水深 2) 如果蓄洪区上本来就有水，则它本来就属于河道总水量的一部分。 因此不需要手动将蓄洪区剩余的水移到河道里或dVCh里
					dVFp = cal_infil_wetspa(Parptr, Solverptr, h_old_fp, grid_index, soilMoisturePD, porosityPD, rootDepthPD, infilPD, fp_area);

					Parptr->rainExcessPD[grid_index] = h_old_fp - infilPD[grid_index];

					// 加入土壤水分
					soilMoisturePD[grid_index] += infilPD[grid_index] / (rootDepthPD[grid_index] * 1000.f);
					//soil_water_depth_row[i] += infilPD[grid_index];
					soil_water_depth_row[i] = soilMoisturePD[grid_index] * rootDepthPD[grid_index]  * 1000.f;
				}
			}
			else
			{
				Parptr->rainExcessPD[grid_index] = 0.0f;
				infilPD[grid_index] = 0.0f;
				dVFp = 0.0;
			}
			

			//*************计算河道的入渗******************

			// 河道里原本有水
			if (h_old_ch > 0.0) {
				// 河道土壤对应其栅格的第几层
				int bedLyr = Parptr->sgcBedSoilLyrPD[grid_index];
				// 河道底部所在层土壤饱和则不入渗
				if (Parptr->multi_soilMoisturePD[bedLyr][grid_index] >= Parptr->multi_soilPorosityPD[bedLyr][grid_index]) {
					infilChPD[grid_index] = 0.0f;
					dVCh = 0.0;
				}
				// 河道底部所在层土壤不饱和则入渗
				else {
					NUMERIC_TYPE ch_soilDepth = (Parptr->soilThicknessAllLyrsPD[grid_index] - SGC_BankFullHeight);
					// 当河道底部有土壤，且河道水深+降雨-蒸发>0(有待入渗的水,h_old + SGC_BankFullHeight > 0.0肯定满足)，且底部土壤湿度<孔隙度时，更新河道底部土壤的湿度
					if (ch_soilDepth > UTIL_ZERO  && Parptr->multi_soilMoisturePD[bedLyr][grid_index] < Parptr->multi_soilPorosityPD[bedLyr][grid_index])
					{
						dVCh = cal_infil_wetspa(Parptr, Solverptr, h_old_ch, grid_index, Parptr->multi_soilMoisturePD[bedLyr], Parptr->multi_soilPorosityPD[bedLyr], Parptr->multi_soilThicknessPD[bedLyr], infilChPD, SGC_c);
						Parptr->multi_soilMoisturePD[bedLyr][grid_index] += infilChPD[grid_index] * SGC_c / (Parptr->multi_soilThicknessPD[bedLyr][grid_index] * row_cell_area);
					}
				}
			}
			// 河道里原本没有水（已加降雨-蒸发）
			else
			{
				infilChPD[grid_index] = 0.0f;
				dVCh = 0.0;
			}

			// 从地表水量中减去
			volume_row[i] -= dVFp;
			// 从河道水量中减去
			volume_row_ch[i] -= dVCh;

			// POI
			if (Statesptr->save_poi)
			{
				Infilt_Row_POI[i] += infilPD[grid_index];   // mm
				Infilt_Ch_POI[i] += infilChPD[grid_index];
				soil_water_depth_Row_POI[i] = soilMoisturePD[grid_index] * rootDepthPD[grid_index] * 1000.0;
			}
			

		}

		return infil_loss;
	}

	bool IsNumber(float x)
	{
		// This looks like it should always be true, 
		// but it's false if x is a NaN.
		return (x == x);
	}

	inline NUMERIC_TYPE SGC2_interflow_singlelayer(
		const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		Pars *Parptr, const Solver *Solverptr, int grid_row_index,
		const NUMERIC_TYPE * dem_row,
		NUMERIC_TYPE *interflow_runoff_vol,
		NUMERIC_TYPE *interflow_2ch_vol,
		const NUMERIC_TYPE row_cell_area
		//NUMERIC_TYPE* interflow_Row_POI,
	)
	{
		float k = 0.f;   // mm/h
		float ks = 0.f;
		float maxSoilWaterVol = 0.f;
		float soilWaterVol = 0.f;
		float fieldCapacityVol = 0.f;
		float interflowMiosture = 0.f;
		float runoffVolCurStep = 0.f;
		NUMERIC_TYPE interflow_loss = C(0.0);
		for (int i = row_start; i < row_end; i++)
		{
			// 问题1：SEIMS里的单元有明确的上下游关系，这里要使用流向tif直接作为上下游关系的依据，还是用土壤水位差作为上下游的依据？
			// 问题2：green-ampt假设有一个明确的湿润锋面，适合干旱区的入渗；有人将其改造为适合湿润区的，但
			// 我们用的casc2d里的greenampt是否适合湿润区的模拟？
			// 问题3：为什么要除以流长（单元上的河道长度），对我而言流长是否是一个栅格单元的宽度？
			if (dem_row[i] != DEM_NO_DATA) {
				int index = i + grid_row_index;
				if (Parptr->soilMoisturePD[index] < Parptr->fieldCapacityPD[index])
				{
					continue;
				}
				if (Parptr->ks_factor > 0.0)
				{
					ks = Parptr->ks_factor *  Parptr->ksPD[index];
				}
				else
				{
					ks = Parptr->ksPD[index];
				}
				maxSoilWaterVol = Parptr->porosityPD[index] * Parptr->rootDepthPD[index] * 0.01f  * row_cell_area;  // m3
				if (Parptr->soilMoisturePD[index] > Parptr->porosityPD[index]) {
					k = ks;
				}
				else {
					/// Using Clapp and Hornberger (1978) equation to calculate unsaturated hydraulic conductivity.
					float dcIndex = 2.f * Parptr->poreIndexPD[index] + 2.f; // pore disconnectedness index
					k = ks * pow(Parptr->soilMoisturePD[index] / Parptr->porosityPD[index], dcIndex);
					//if (k <= 0.000001) k = 0.f;
				}
				// 1. / 3600. = 0.0002777777777777778
				// 当前土壤水分的当量水量
				soilWaterVol = Parptr->rootDepthPD[index] * 0.01f * Parptr->soilMoisturePD[index] * row_cell_area;  // m3
				// 田间持水量的当量水量
				fieldCapacityVol = Parptr->rootDepthPD[index] * 0.01f * Parptr->fieldCapacityPD[index] * row_cell_area;
				// interflowGenVolPD m3,k from mm/h -> m/s
				if (Parptr->slopePD[index] <= 0.0)
				{
					Parptr->slopePD[index] = 0.001;
				}
				// 加一个lag系数，汇流（SWAT文档，问娇娇）
				Parptr->interflowGenVolPD[index] = Parptr->interflow_cs * Parptr->rootDepthPD[index] * 0.01f * Parptr->slopePD[index]
					* k * 0.0002777777777777778 * 0.001
					* Parptr->soilMoisturePD[index]  * sqrt(row_cell_area)  * 	Solverptr->SGCtmpTstep;    // m3

				// the unit is mm
				// 如果地下水储量 - 地下水径流量后，依然超出土壤孔隙度（土壤最大储水量），则地下水径流量=土壤水储量-最大储水量，原有逻辑感觉适合日尺度，不适合秒尺度
				// 改为即便超饱和，地下水径流仍然以ks为速率流失, 避免出现河道流量突变
				//if (soilWaterDep - interflowDep > maxSoilWaterDep) {
				//	Parptr->interflowGenVolPD[index] = Parptr->soilMoisturePD[index] - maxSoilWaterDep;
				//}
				if (soilWaterVol - Parptr->interflowGenVolPD[index] > maxSoilWaterVol) {
					Parptr->interflowGenVolPD[index] = soilWaterVol - maxSoilWaterVol;
				}
				else if (soilWaterVol - Parptr->interflowGenVolPD[index] < fieldCapacityVol) {
					// 如果 减去后，小于田间持水量，则壤中流=土壤水储量-田间持水量，xiaodw
					Parptr->interflowGenVolPD[index] = soilWaterVol - fieldCapacityVol;
				}
				Parptr->interflowGenVolPD[index] = Max(0.f, Parptr->interflowGenVolPD[index]);
				interflowMiosture = Parptr->interflowGenVolPD[index] / (row_cell_area * Parptr->rootDepthPD[index] * 0.01f);  // m3/m3

				// 土壤水储量 - 壤中流径流量
				Parptr->soilMoisturePD[index] -= interflowMiosture;
				Parptr->soilWaterDepth[index] -= Parptr->interflowGenVolPD[index] * 1000.0 / row_cell_area;  // mm
				//*interflowAvgBlock = *interflowAvgBlock + Parptr->interflowGenVolPD[index];
				interflow_loss += Parptr->interflowGenVolPD[index];

				// 根据滞后系数计算实际汇流的量=(当前步长产流+之前的积累量)*滞后系数
				if (Parptr->interflow_lagindex > 0.0)
				{
					Parptr->interflow2ChVolPD[index] = (Parptr->interflowGenVolPD[index] + Parptr->interflowRunoffVolPD[index]) * Parptr->interflow_lagindex;
				}
				else
				{
					Parptr->interflow2ChVolPD[index] = (Parptr->interflowGenVolPD[index] + Parptr->interflowRunoffVolPD[index]) * (1 - exp(-Parptr->interflow_surlag / (Parptr->interflow_t_conc)));
					//Parptr->interflow2ChVolPD[index] = (Parptr->interflowGenVolPD[index] + Parptr->interflowRunoffVolPD[index]) * (1 - exp(-Parptr->interflow_surlag / (Parptr->interflow_t_conc / (Solverptr->SGCtmpTstep * 0.00027777f))));
				}

				//cout << index << "  " << Parptr->interflow2ChVolPD[index] << "   " << Parptr->interflowGenVolPD[index] << "   " << Parptr->interflowRunoffVolPD[index] << endl;
				// 更新壤中流形成的地表径流的库存量
				Parptr->interflowRunoffVolPD[index] = Parptr->interflowRunoffVolPD[index] + Parptr->interflowGenVolPD[index] - Parptr->interflow2ChVolPD[index];
				*interflow_2ch_vol = *interflow_2ch_vol + Parptr->interflow2ChVolPD[index];
				*interflow_runoff_vol = *interflow_runoff_vol + Parptr->interflowRunoffVolPD[index];
			}
		}
		return interflow_loss;
	}
	inline unsigned char fequal(NUMERIC_TYPE a, NUMERIC_TYPE b) {
		return ABSVAL(a - b) <= 0.000001;
	}




	inline int valid_cell(const int row, const int col,  const int row_start, const int row_end,const int grid_rows) {
		return row >= 0 && row < grid_rows && col >= row_start && col < row_end;
	}

	// dx 是 col， dy 是 row
	inline void slope_aspect(NUMERIC_TYPE dx, NUMERIC_TYPE dy, NUMERIC_TYPE celev, NUMERIC_TYPE* nelev,
		NUMERIC_TYPE *slope, NUMERIC_TYPE *aspect)
	{
		int n;
		float dzdx, dzdy;
		NUMERIC_TYPE *dummyelev;
		/* this dummy varaible is added for calculation of elev difference,
		in which the elev of OUTSIDEBASIN cells (which is ZERO) is
		replaced by the elev of the central cell */

		/* allocate memory */
		
		dummyelev = new  NUMERIC_TYPE[NNEIGHBORS];

		for (n = 0; n < NNEIGHBORS; n++) {
			if (nelev[n] == OUTSIDEBASIN) {
				dummyelev[n] = celev;
			}
			else
				dummyelev[n] = nelev[n];
		}
		// 认为7的权重是2
		dzdx = ((dummyelev[0] + 2 * dummyelev[7] + dummyelev[6]) -
			(dummyelev[2] + 2 * dummyelev[3] + dummyelev[4])) / (8 * dx);
		dzdy = ((dummyelev[0] + 2 * dummyelev[1] + dummyelev[2]) -
			(dummyelev[4] + 2 * dummyelev[5] + dummyelev[6])) / (8 * dy);

		*slope = sqrt(dzdx * dzdx + dzdy * dzdy);
		if (fequal(dzdx, 0.0) && fequal(dzdy, 0.0)) {
			*aspect = 0.0;
		}
		else {
			/* convert from radian to degree */
			*aspect = atan2(dzdx, dzdy);
		}
		//memory_free_legacy(&dummyelev);
		delete[] dummyelev;
		//free(dummyelev);
		return;
	}

	inline void flow_fractions(float dx, float dy, NUMERIC_TYPE slope, NUMERIC_TYPE aspect,
		NUMERIC_TYPE celev, NUMERIC_TYPE* nelev, NUMERIC_TYPE *grad,
		unsigned char dir[NDIRS], unsigned int *total_dir)
	{
		NUMERIC_TYPE cosine = cos(aspect);
		NUMERIC_TYPE sine = sin(aspect);
		NUMERIC_TYPE total_width, effective_width;
		NUMERIC_TYPE *cos, *sin;
		int n;
		NUMERIC_TYPE *drop = new NUMERIC_TYPE[NDIRS];
		NUMERIC_TYPE maxdrop;
		int steepest;

		/* allocate memory */
		
		cos = new  NUMERIC_TYPE [NDIRS / 2];
		sin = new  NUMERIC_TYPE [NDIRS / 2];
		//if (!(cos = (NUMERIC_TYPE*)memory_allocate(sizeof(NUMERIC_TYPE) * NDIRS / 2)))
		//	cout << "slope_aspect( )" << "allocate memory error" << endl;
		//if (!(sin = (NUMERIC_TYPE*)memory_allocate(sizeof(NUMERIC_TYPE) * NDIRS / 2)))
		//	cout << "slope_aspect( )" << "allocate memory error" << endl;

		switch (NDIRS) {
		case 4:
			/* fudge any cells which flow outside the basin by just pointing the
			   aspect in the opposite direction */
			if (cosine > 0 && nelev[5] == (float)OUTSIDEBASIN)
				cos[1] = -cosine;
			else cos[1] = cosine;
			if (cosine < 0 && nelev[1] == (float)OUTSIDEBASIN)
				cos[0] = -cosine;
			else cos[0] = cosine;
			if (sine > 0 && nelev[3] == (float)OUTSIDEBASIN)
				sin[0] = -sine;
			else sin[0] = sine;
			if (sine < 0 && nelev[7] == (float)OUTSIDEBASIN)
				sin[1] = -sine;
			else sin[1] = sine;

			/* compute flow widths */
			total_width = fabs(sine) * dx + fabs(cosine) * dy;
			*grad = slope * total_width;
			*total_dir = 0;
			for (n = 0; n < NDIRS; n++)
			{
				switch (n) {
				case 0:
					effective_width = (cos[1] > 0 ? cos[1] * dx : 0.0);
					break;
				case 2:
					effective_width = (cos[0] < 0 ? -cos[0] * dx : 0.0);
					break;
				case 1:
					effective_width = (sin[0] > 0 ? sin[0] * dy : 0.0);
					break;
				case 3:
					effective_width = (sin[1] < 0 ? -sin[1] * dy : 0.0);
					break;
				default:
					cout << "error flow_fractions " << 65 << endl;
				}
				dir[n] = (int)((effective_width / total_width) * 255.0 + 0.5);
				*total_dir += dir[n];
			}
			break;
		case 8:
			/*For D8 flow directions, water discharges to ONE of its eight neighbors:
			to one located in the direction of steepest descent. This requires the DEM
			to be pre-filled for D8 routing scheme as flat area will confuse the model*/
			steepest = -9999;
			maxdrop = -9999;
			/*Determine flow direction based on deepest drop */
			for (n = 0; n < NDIRS; n++) {
				/*Make sure flow is inside boundary*/
				if (nelev[n] == OUTSIDEBASIN) {
					dir[n] = 0;
					drop[n] = 0;
				}
				else {
					/*Find steepest descent*/
					if (n == 0 || n == 2 || n == 4 || n == 6)
						drop[n] = (celev - nelev[n]) / sqrt(dx * dx + dy * dy);
					else
						drop[n] = (celev - nelev[n]) / dx;

					if ((drop[n] < 0.0) && (drop[n] > -0.001)) {
						//printf("Reset minor negative flow slope from %f to 0.0\n", drop[n]);
						drop[n] = 0.0;
					}

					if (drop[n] >= 0 && drop[n] > maxdrop) {
						steepest = n;
						maxdrop = drop[n];
					}
				}
			}

			*total_dir = 0;
			if (steepest >= 0) {
				dir[steepest] = 1.0;
				*total_dir += dir[steepest];

				/* This requires dx = dy */
				if (steepest == 0 || steepest == 2 || steepest == 4 || steepest == 6)
					total_width = sqrt(dx * dx + dy * dy);
				else
					total_width = dx;
			}
			else {
				//cout << "one grid cell has minor sink, set flow width to cell size\n" << endl;
				total_width = dx;
			}
			*grad = slope * total_width;

			break;
		default:
			cout << "error flow_fractions " << 65 << endl;
		}
		delete[] sin;
		delete[] cos;
		delete[] drop;
		return;
	}



	inline void HeadSlopeAspect(Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const int row_start, int row_end, const int grid_cols_padded, NUMERIC_TYPE dx, NUMERIC_TYPE dy, WetDryRowBound* wet_dry_bounds)
	{
		int n;
		NUMERIC_TYPE neighbor_water_table[NNEIGHBORS];

		int index = 0;
		int neighbor_index;
		int neighbor_row;
		int neighbor_col;
		/* let's assume for now that WaterLevel is the SOILPIX map is computed elsewhere */
		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				index = i + grid_row_index;
				NUMERIC_TYPE slope, aspect;

				// todo: 这里是否应该只算右、右下、下、左下？
				for (n = 0; n < NNEIGHBORS; n++) {
					neighbor_index = index + Parptr->neighbor_ref[n];
					neighbor_row = row + Parptr->neighbor_row_ref[n];
					neighbor_col = i + Parptr->neighbor_col_ref[n];
					// 优先判断 row 合法性，再访问 dem_data
					if (neighbor_row >= 0 && neighbor_row < grid_rows) {
						int row_start = wet_dry_bounds->dem_data[neighbor_row].start;
						int row_end = wet_dry_bounds->dem_data[neighbor_row].end;

						if (valid_cell(neighbor_row, neighbor_col, row_start, row_end, grid_rows)) {
							neighbor_water_table[n] = Parptr->waterLevelPD[neighbor_index];
						}
						else {
							neighbor_water_table[n] = OUTSIDEBASIN;
						}
					}
					else {
						neighbor_water_table[n] = OUTSIDEBASIN;
					}
				}

				slope_aspect(dx, dy, Parptr->waterLevelPD[index], neighbor_water_table,
					&slope, &aspect);
				// D8：只有最陡坡向的dir被赋值为1，其余都为0；因此只流向最陡的方向
				// D4：按照水力坡降计算分配比例
				flow_fractions(dx, dy, slope, aspect, Parptr->waterLevelPD[index], neighbor_water_table,
					&(Parptr->subFlowGradPD[index]), Parptr->subDirPD[index], &(Parptr->subTotalDirPD[index]));
			}
		}

		return;
	}



	inline void HeadSlopeAspectForUpLyr(Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const int row_start, int row_end, const int grid_cols_padded, NUMERIC_TYPE dx, NUMERIC_TYPE dy, WetDryRowBound* wet_dry_bounds,int i
		//, const NUMERIC_TYPE *dem_grid
	)
	{
		int n;
		NUMERIC_TYPE neighbor_water_table[NNEIGHBORS];

		int neighbor_index;
		int neighbor_row;
		int neighbor_col;

		int index = i + grid_row_index;
		NUMERIC_TYPE slope, aspect;

		// todo: 这里是否应该只算右、右下、下、左下？
		for (n = 0; n < NNEIGHBORS; n++) {
			neighbor_index = index + Parptr->neighbor_ref[n];
			neighbor_row = row + Parptr->neighbor_row_ref[n];
			neighbor_col = i + Parptr->neighbor_col_ref[n];
			// 先根据dem计算流向，因为如果是上游第1层流向下游第1层，2层流向2层，则每一层的流向都会和dem计算出来的一致
			// 根据每个栅格第lyr层和其相邻栅格第lyr层的水头计算流向
			if (valid_cell(neighbor_row, neighbor_col, wet_dry_bounds->dem_data[neighbor_row].start, wet_dry_bounds->dem_data[neighbor_row].end, grid_rows))
			{
				neighbor_water_table[n] = Parptr->waterLevelUpLyrPD[index];		
			}
			else {
				neighbor_water_table[n] = OUTSIDEBASIN;
			}
		}
				
		slope_aspect(dx, dy, dem_row[i], neighbor_water_table,
			&slope, &aspect);
		// D8：只有最陡坡向的dir被赋值为1，其余都为0；因此只流向最陡的方向
		// D4：按照水力坡降计算分配比例
		flow_fractions(dx, dy, slope, aspect, Parptr->waterLevelUpLyrPD[index], neighbor_water_table,
			&(Parptr->subFlowGradUpLyrPD[index]), Parptr->subDirUpLyrPD[index], &(Parptr->subTotalDirUpLyrPD[index]));
			
		

		return;
	}

	inline float CalcTransmissivity(NUMERIC_TYPE SoilDepth, NUMERIC_TYPE WaterTable, NUMERIC_TYPE LateralKs,
		NUMERIC_TYPE KsExponent, NUMERIC_TYPE DepthThresh)
	{
		float Transmissivity;		/* Transmissivity (m^2/s) */
		float TransThresh;

		if (fequal(KsExponent, 0.0))
			Transmissivity = LateralKs * (SoilDepth - WaterTable);
		else {
			/* a smaller value of WaterTable variables indicates a higher actual water table depth */
			if (WaterTable < DepthThresh) {
				Transmissivity = (LateralKs / KsExponent) * (exp(-KsExponent * WaterTable) - exp(-KsExponent * SoilDepth));
			}
			else {
				TransThresh = (LateralKs / KsExponent) * (exp(-KsExponent * DepthThresh) - exp(-KsExponent * SoilDepth));
				if (SoilDepth < DepthThresh) {
					printf("Warning: Soil DepthThreshold (%.2f) > the soil depth (%.2f)!\n", DepthThresh, SoilDepth);
					printf("Transmissivity is set to zero!");
				}
				Transmissivity = (SoilDepth - WaterTable) / (SoilDepth - DepthThresh)*TransThresh;
			}
		}
		
		return Transmissivity ;
	}


	inline float CalcTransmissivity(NUMERIC_TYPE SoilDepth, NUMERIC_TYPE WaterTable, NUMERIC_TYPE LateralKs,
		NUMERIC_TYPE KsExponent, NUMERIC_TYPE DepthThresh, NUMERIC_TYPE*  ksFactorHOfLyr, int curLyr)
	{
		float Transmissivity;		/* Transmissivity (m^2/s) */
		float TransThresh;

		if (fequal(KsExponent, 0.0))
			Transmissivity = LateralKs * (SoilDepth - WaterTable);
		else {
			/* a smaller value of WaterTable variables indicates a higher actual water table depth */
			if (WaterTable < DepthThresh) {
				Transmissivity = (LateralKs / KsExponent) * (exp(-KsExponent * WaterTable) - exp(-KsExponent * SoilDepth));
			}
			else {
				TransThresh = (LateralKs / KsExponent) * (exp(-KsExponent * DepthThresh) - exp(-KsExponent * SoilDepth));
				if (SoilDepth < DepthThresh) {
					printf("Warning: Soil DepthThreshold (%.2f) > the soil depth (%.2f)!\n", DepthThresh, SoilDepth);
					printf("Transmissivity is set to zero!");
				}
				Transmissivity = (SoilDepth - WaterTable) / (SoilDepth - DepthThresh)*TransThresh;
			}
		}
		Transmissivity *= ksFactorHOfLyr[curLyr];
		return Transmissivity;
	}

	inline float CalcAvailableWater(int NRootLayers, NUMERIC_TYPE TotalDepth, NUMERIC_TYPE **RootDepth,
		NUMERIC_TYPE **Porosity, NUMERIC_TYPE **FCap, NUMERIC_TYPE *TableDepth, NUMERIC_TYPE **Adjust,int index)

	{
		float AvailableWater;		/* amount of water available for movement (m) */

		float DeepFCap;		    /* field capacity of the layer below the  deepest root layer */

		float DeepLayerDepth;		/* depth of layer below deepest root zone layer */

		float DeepPorosity;		/* porosity of the layer below the deepest root layer */

		float Depth;			    /* depth below the ground surface (m) */
		int lyr;			        /* counter */

		AvailableWater = 0;
		// 根据每一层的淹没情况（水位Depth），计算总共有多少水参与计算侧向壤中流
		Depth = 0.0;
		for (lyr = 0; lyr < NRootLayers && Depth < TotalDepth; lyr++) {
			if (RootDepth[lyr][index] < (TotalDepth - Depth))
				Depth += RootDepth[lyr][index];
			else
				Depth = TotalDepth;
			if (Depth > TableDepth[index]) {
				if ((Depth - TableDepth[index]) > RootDepth[lyr][index])  // 水位在i层之上
					AvailableWater += (Porosity[lyr][index] - FCap[lyr][index]) * RootDepth[lyr][index] * Adjust[lyr][index];
				else
					AvailableWater += (Porosity[lyr][index] - FCap[lyr][index]) * (Depth - TableDepth[index]) * Adjust[lyr][index];
			}
		}

		if (Depth < TotalDepth) {

			DeepPorosity = Porosity[NRootLayers][index];
			DeepFCap = FCap[NRootLayers][index];

			DeepLayerDepth = TotalDepth - Depth;
			Depth = TotalDepth;

			if ((Depth - TableDepth[index]) > DeepLayerDepth)
				AvailableWater += (DeepPorosity - DeepFCap) * DeepLayerDepth * Adjust[NRootLayers][index];
				
			else
				AvailableWater += (DeepPorosity - DeepFCap) * (Depth - TableDepth[index]) *Adjust[NRootLayers][index];
				
		}

		return AvailableWater;
	}


	inline float CalcAvailableWaterForUplyr(int NUpLayers, NUMERIC_TYPE TotalDepth, NUMERIC_TYPE **RootDepth,
		NUMERIC_TYPE **Porosity, NUMERIC_TYPE **FCap, NUMERIC_TYPE *TableDepth, NUMERIC_TYPE **Adjust, NUMERIC_TYPE** soilMoisturePD, int index)

	{
		float AvailableWater;		/* amount of water available for movement (m) */

		float DeepFCap;		    /* field capacity of the layer below the  deepest root layer */

		float DeepLayerDepth;		/* depth of layer below deepest root zone layer */

		float DeepPorosity;		/* porosity of the layer below the deepest root layer */

		float Depth;			    /* depth below the ground surface (m) */
		int lyr;			        /* counter */

		AvailableWater = 0;
		// 根据每一层的淹没情况（水位Depth），计算总共有多少水参与计算侧向壤中流
		Depth = 0.0;
		for (lyr = 0; lyr < NUpLayers && Depth < TotalDepth; lyr++) {
			
			if (soilMoisturePD[lyr][index] > FCap[lyr][index])
			{
				AvailableWater += (soilMoisturePD[lyr][index] - FCap[lyr][index]) * RootDepth[lyr][index] * Adjust[lyr][index];
			}
		}

		return AvailableWater;
	}

	// 直接将水量加入河道栅格的水量，河道水深会在SGC2_ProcessH_Row方法计算
	//inline void channel_grid_inc_inflow(NUMERIC_TYPE *volume_grid, int grid_index, NUMERIC_TYPE waterVol)
	//{
	//	volume_grid[grid_index] += waterVol;

	//}
	inline void channel_grid_inc_inflow(NUMERIC_TYPE * volume_row, int grid_row_index, NUMERIC_TYPE waterVol)
	{
		volume_row[grid_row_index] += waterVol;

	}

	
	/*
	0---- - 1---- - 2
	7---- - *---- - 3
	6---- - 5---- - 4
	*/
	inline void RouteSubSurface(const int row_start, int row_end,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid,
		NUMERIC_TYPE dx_col, NUMERIC_TYPE dy_col, WetDryRowBound* wet_dry_bounds, Pois*Poisptr, 
		const NUMERIC_TYPE * sg_cell_cell_area, int j,const int cell_count, const int sg_row_start, const int * sg_cell_grid_index_lookup,
		const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume, const NUMERIC_TYPE * sg_cell_SGC_c,
		const SubGridFlowLookup * sg_cell_flow_lookup) {

		NUMERIC_TYPE BankHeight;
		NUMERIC_TYPE *Adjust;
		NUMERIC_TYPE fract_used;
		NUMERIC_TYPE depth;
		NUMERIC_TYPE OutFlow;
		NUMERIC_TYPE water_out_road;
		NUMERIC_TYPE Transmissivity;
		NUMERIC_TYPE AvailableWater;
		int k;
		int index;
		

		/* 省略 reset the saturated subsurface flow to zero */
		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				// 省略 计算河床底部以上的侧面高程，如果BankHeight>土壤厚度说明/河床以上存在裸露岩石，反之说明河床高程之下是土壤
				//Adjust = Parptr->multi_adjustPD[index];
				fract_used = 0.0f;
				water_out_road = 0.0;
				int index = i + grid_row_index;
				int source_index_this = row * Parptr->xsz + i;
				// 计算河床底部以上的侧面高程，如果BankHeight>土壤厚度说明/河床以上存在裸露岩石，反之说明河床高程之下是土壤
				//BankHeight = (Network[y][x].BankHeight > SoilMap[y][x].Depth) ?  SoilMap[y][x].Depth : Network[y][x].BankHeight;
				// todo: 检查像元值对不对
				BankHeight = Arrptr->SGCbfH[source_index_this] > Parptr->soilThicknessAllLyrsPD[index] ? Parptr->soilThicknessAllLyrsPD[index] : Arrptr->SGCbfH[source_index_this];
				if (BankHeight < -1e-8)
				{
					cout << "index: " << index << BankHeight << endl;
				}
				int curSoilLyr = 0;
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					if (Parptr->tableDepthPD[index] > Parptr->multi_soilDepthPD[lyr][index])
					{
						curSoilLyr ++;
					}
				}
				//sg_cell_SGC_BankFullHeight[cell_index]
				// 如果该栅格是SGC河道
				if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {			/* cell has a stream channel */

					// 地下水位在河道底部之上，河堤的土壤水才补给河道
					if (Parptr->tableDepthPD[index] < BankHeight) {
						float gradient = 4.0 * (BankHeight - Parptr->tableDepthPD[index]);
						if (gradient < 0.0)
							gradient = 0.0;
						Transmissivity =
							CalcTransmissivity(BankHeight, Parptr->tableDepthPD[index],
								Parptr->ksLatPD[index] * 0.001*0.000277778,
								Parptr->KsLatExpValue,
								Parptr->soilWaterDepthThresh
								, Parptr->multi_ksFactorHOfLyr, curSoilLyr);

						OutFlow = (Transmissivity * gradient * Parptr->gwTstep) / row_cell_area;  // m

						/* check whether enough water is available for redistribution */
						AvailableWater =
							CalcAvailableWater(Parptr->multi_nRootLyrs,
								BankHeight, Parptr->multi_soilThicknessPD,
								Parptr->multi_soilPorosityPD,
								Parptr->multi_soilFcPD,
								Parptr->tableDepthPD, Parptr->multi_adjustPD, index);
						// 如果剩余可供侧向壤中流的水不够outFlow计算出来的应该流走的量，就只能流走AvailableWater
						OutFlow = (OutFlow > AvailableWater) ? AvailableWater : OutFlow;

						/* remove water going to channel from the grid cell */
						Parptr->satFlowPD[index] -= OutFlow;

						/* contribute to channel segment lateral inflow todo：这里认真检查是否加到河道上了 */
						//channel_grid_inc_inflow(volume_grid, index, OutFlow * row_cell_area);
						// 这个实现只能单线程
						//Parptr->subSurfaceLatFlow2ChTotal += OutFlow * row_cell_area;

						Parptr->satFlow2ChPD[index] += OutFlow * row_cell_area;  // m3

						if (Statesptr->save_poi == ON)
						{
							Poisptr->soil_lat_flowout_Grid_allLyr[index] += OutFlow * 1000.0;  // mm/gwstep
							Poisptr->soil_lat_flowout_Grid[curSoilLyr][index] += OutFlow * 1000.0;  // m/gwstep -> mm/gwstep
						}
					}
				}
				else {
					for (k = 0; k < NDIRS; k++) {
						fract_used += (NUMERIC_TYPE)Parptr->subDirPD[index][k];
					}
					if (Parptr->subTotalDirPD[index] > 0)
						fract_used /= (NUMERIC_TYPE)Parptr->subTotalDirPD[index];
					else
						fract_used = 0.;

					// 地下水位在土壤底部之上 
					if (Parptr->tableDepthPD[index] < Parptr->soilThicknessAllLyrsPD[index]) {
						// 地下水位在河道底部之下，depth取自身；否则，depth要取BankHeight，
						depth = ((Parptr->tableDepthPD[index] > BankHeight) ?
							Parptr->tableDepthPD[index] : BankHeight);
						
						Transmissivity = CalcTransmissivity(Parptr->soilThicknessAllLyrsPD[index], depth,
							Parptr->ksLatPD[index]*0.001*0.000277778,  // 每个土壤柱的侧向饱和水力传导度是空间异质的, mm/h->m/s
							Parptr->KsLatExpValue,   // DHSVM每个土壤柱只对应单一的土壤类型，这里将KsLatExp设置为用户可调的参数
							Parptr->soilWaterDepthThresh
							, Parptr->multi_ksFactorHOfLyr, curSoilLyr);

						//Transmissivity = CalcTransmissivity(Parptr->soilThicknessAllLyrsPD[index], depth,
						//	Parptr->ksLatPD[index],  // 每个土壤柱的侧向饱和水力传导度是空间异质的
						//	Parptr->KsLatExpValue,   // DHSVM每个土壤柱只对应单一的土壤类型，这里将KsLatExp设置为用户可调的参数
						//	Parptr->soilWaterDepthThresh);
						// 对于与河道相邻的栅格来说，如果它的侧向流补给河道栅格的土壤，则这里的outflow是按照栅格面积来计算的等效水深
						OutFlow = (Transmissivity * fract_used * Parptr->subFlowGradPD[index] * Parptr->gwTstep) / row_cell_area;

						/* check whether enough water is available for redistribution ,AvailableWater (m)*/
						AvailableWater =
							CalcAvailableWater(Parptr->multi_nRootLyrs,
								Parptr->soilThicknessAllLyrsPD[index], Parptr->multi_soilThicknessPD,
								Parptr->multi_soilPorosityPD, Parptr->multi_soilFcPD,
								Parptr->tableDepthPD, Parptr->multi_adjustPD, index);
						OutFlow = (OutFlow > AvailableWater) ? AvailableWater : OutFlow;
					}
					else {
						depth = Parptr->soilThicknessAllLyrsPD[index];
						OutFlow = 0.0f;
					}

					/* Subsurface Component - Decrease water change by outwater */

					Parptr->satFlowPD[index] -= OutFlow;
					
					/* Assign the water to appropriate surrounding pixels */
					if (Parptr->subTotalDirPD[index] > 0)
						OutFlow /= (NUMERIC_TYPE)Parptr->subTotalDirPD[index];
					else
						OutFlow = 0.;

					for (k = 0; k < NDIRS; k++) {
						int neighbor_index = i + grid_row_index + Parptr->neighbor_ref[k];
						int neighbor_row = row + Parptr->neighbor_row_ref[k];
						int neighbor_col = i + Parptr->neighbor_col_ref[k];
						if (valid_cell(neighbor_row, neighbor_col, wet_dry_bounds->dem_data[neighbor_row].start, wet_dry_bounds->dem_data[neighbor_row].end,grid_rows)) {

							Parptr->satFlowPD[neighbor_index] += OutFlow * Parptr->subDirPD[index][k];
							
							//#pragma omp critical
							//{
							//	Parptr->satFlowPD[neighbor_index] += OutFlow * Parptr->subDirPD[index][k];
							//}
							//Parptr->satFlow2NeiborPD[neighbor_index] = OutFlow * Parptr->subDirPD[index][k] * row_cell_area;
						}
					}
				}
			}
		}
		//for (int cell_i = 0; cell_i < cell_count; cell_i++)
		//{
		//	int cell_index = sg_row_start + cell_i;

		//	int grid_index = sg_cell_grid_index_lookup[cell_index];
		//	int i = grid_index - grid_row_index;

		//	// // 河道所在的栅格单元面积
		//	const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
		//	// SGC河道单元底面积
		//	NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];
		//	// 河道两侧蓄洪区面积
		//	NUMERIC_TYPE fp_area = row_cell_area - SGC_c;
		//	// 河道深度
		//	NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index] * 1000.0;
		//	int source_index_this = row * Parptr->xsz + i;
		//	BankHeight = Arrptr->SGCbfH[source_index_this] > Parptr->soilThicknessAllLyrsPD[grid_index] ? Parptr->soilThicknessAllLyrsPD[grid_index] : Arrptr->SGCbfH[source_index_this];
		//	if (BankHeight < -1e-8)
		//	{
		//		cout << "grid_index: " << grid_index << BankHeight << endl;
		//	}


		//	
		//}
		return;
	}

	inline void DistributeSatflow(const int row_start, int row_end,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid,
		 int NSoilLayers, int NRootLayers, Pois*Poisptr)
	{
		float DeepLayerDepth;		/* depth of the layer below the deepest root layer */
		int i;			        /* counter */

	   /*Following variables adde by Zhuoran*/
		float DeepFCap;		/* field capacity of the layer below the deepest root layer */
		float DeepPorosity;		/* porosity of the layer below the deepest root layer */
		float DeepAvaWater;
		float AvaWater;
		float DeepWaterGap;
		float WaterGap;
		float ExtracWater;
		float DeepExtracWater;
		float Depth;
		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				int index = i + grid_row_index;
				int source_index_this = row * Parptr->xsz + i;

				DeepPorosity = Parptr->multi_soilPorosityPD[NRootLayers][index];
				DeepFCap = Parptr->multi_soilFcPD[NRootLayers][index];

				/*end of adding variable*/
				// 最下层土壤层的厚度= 总厚度 - 根系层的所有厚度之和
				DeepLayerDepth = Parptr->soilThicknessAllLyrsPD[index];
				for (int lyr = 0; lyr < NRootLayers; lyr++)
					DeepLayerDepth -= Parptr->multi_soilThicknessPD[lyr][index];

				/* Added 06/09/2016 by Zhuoran Duan(zhuoran.duan@pnnl.gov) */
				/* When calculating lateral outflow, available water was calculated using all 3 root zone layers
				and the deep layer underneath but outflow was extracted from the bottom deep soil layer. This
				might lead to negative soil moisture in deep soil while the layer above it remains saturated. This tends
				to happen more often in dry climate. In order to avoid negative deep soil moisture, here I add a loop
				to redistribution of water extraction. The outflow starts from the (top) water table layer, extract the
				excess water larger than field capacity. While there's no enough water from the current layer, extract
				water from one layer below it until it reaches bottom layer. */

				
				/*New algorithm for SatFlow, remove water from top layer to bottom layer*/
				Depth = 0.0;
				// 如果是流出, 就从最顶层开始扣除
				if (Parptr->satFlowPD[index] < 0.0) {
					///* printf("SatFlow before distribution is %.6f\n",SatFlow);*/
					for (int lyr = 0; lyr < NRootLayers && Depth < Parptr->soilThicknessAllLyrsPD[index]; lyr++) {
						AvaWater = 0.0;
						ExtracWater = 0.0;
						if (Parptr->multi_soilThicknessPD[lyr][index] < (Parptr->soilThicknessAllLyrsPD[index] - Depth))
							Depth += Parptr->multi_soilThicknessPD[lyr][index];
						else
							Depth = Parptr->soilThicknessAllLyrsPD[index];

						/* if water table is in the ith root zone layer */
						// 水位高于第i层土壤
						if (Depth > Parptr->tableDepthPD[index]) {
							/* fully saturated in the current layer */
							if ((Depth - Parptr->tableDepthPD[index]) > Parptr->multi_soilThicknessPD[lyr][index])
								AvaWater = (Parptr->multi_soilPorosityPD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]) * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
							else
								AvaWater = (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]) * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];   // 这里和calcAvailableWater不一样，如果第i层没完全淹没，就只用Moist减去田间持水量
						}
						// 上个步长routeSubsurface里算出来的SatFlow如果比这里算出来的AvaWater大，说明之前计算的SatFlow太多了。ExtracWater是个负值，是第i层对应的新的可以扣除的水量
						ExtracWater = (-Parptr->satFlowPD[index] > AvaWater) ? -AvaWater : Parptr->satFlowPD[index];
						// 第i层的土壤湿度 - 需要扣除的比例

						Parptr->multi_soilMoisturePD[lyr][index] += ExtracWater / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
						
						if (Statesptr->save_poi == ON)
						{
							// 如果不是河道栅格，才在这里计算POI点有多少水流出，河道的流出在RouteSubSurface计算
							if (!(Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0))) {			/* cell has a stream channel */
								Poisptr->soil_lat_flowout_Grid[lyr][index] += -ExtracWater * 1000.0;  // m/gwstep -> mm/gwstep
								Poisptr->soil_lat_flowout_Grid_allLyr[index] += -ExtracWater * 1000.0;  // mm/gwstep
								//if (fabs(Poisptr->soil_lat_flowout_Grid[lyr][index]) <= 1e-8) {
								//	Poisptr->soil_lat_flowout_Grid[lyr][index] = 0.0;
								//}
							}
							// 河道栅格也需要在这里，根据流入和流出后的净水量更新土壤水深
							Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // cm
							
						}
						// 给上个步长计算的SatFlow加上新的可以扣除的水量，看是否等于0，即之前第i层是否扣多了

						Parptr->satFlowPD[index] -= ExtracWater;
						
						//printf("SatFlow after layer %d is %.6f\n",i,SatFlow);
						// 如果没扣多
						if (fabs(Parptr->satFlowPD[index]) <= 1e-8) {//  todo: 继续检查
							//cout << index << ": " << fabs(Parptr->satFlowPD[index]) << endl;

							Parptr->satFlowPD[index] = 0.0;
							
							break;
						}
					}
					// 如果所有根系层扣完新计算的量之后，还是发现之前计算的Parptr->satFlowPD没被消耗完，就开始扣最深层的
					if (Parptr->satFlowPD[index] < 0.0) {
						DeepAvaWater = 0.0;
						DeepExtracWater = 0.0;
						// 根系层之下还有土壤层
						if (Depth - Parptr->soilThicknessAllLyrsPD[index] <= -1e-8) {

							Depth = Parptr->soilThicknessAllLyrsPD[index];
							// 地下水位高于DeepLayer
							if ((Depth - Parptr->tableDepthPD[index] - DeepLayerDepth) >= 1e-8 )
								DeepAvaWater = (DeepPorosity - DeepFCap) * DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index];  // 仍然根据饱和水量计算最多有多少水可以侧向流出
							else
								DeepAvaWater = (Parptr->multi_soilMoisturePD[NRootLayers][index] - DeepFCap) * DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index];;  // 否则根据DeepLayer的土壤湿度计算，能有多少水侧向流出
						}

						//printf("Deep Layer Availabe Water is %.6f\n",DeepAvaWater);

						DeepExtracWater = (-Parptr->satFlowPD[index] > DeepAvaWater) ? -DeepAvaWater : Parptr->satFlowPD[index];

						Parptr->multi_soilMoisturePD[NRootLayers][index] += DeepExtracWater / (DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index]);
						
						if (Statesptr->save_poi == ON)
						{

							Poisptr->soil_lat_flowout_Grid[NRootLayers][index] += -DeepExtracWater * 1000.0;
							Poisptr->soil_lat_flowout_Grid_allLyr[index] += -DeepExtracWater * 1000.0;
							//if (fabs(Poisptr->soil_lat_flowin_Grid[NRootLayers][index]) <= 1e-8) {
							//	Poisptr->soil_lat_flowin_Grid[NRootLayers][index] = 0.0;
							//}
							Poisptr->soil_water_depth_Grid[NRootLayers][index] = Parptr->multi_soilMoisturePD[NRootLayers][index] * Parptr->multi_soilThicknessPD[NRootLayers][index] * 1000.0;  // cm
							
						}

						Parptr->satFlowPD[index] -= DeepExtracWater;
						

					}
				}
				// 如果是流入，就从最底层开始补给
				if (Parptr->satFlowPD[index] > 0.0) {

					Depth += DeepLayerDepth;
					DeepWaterGap = 0.0;
					DeepExtracWater = 0.0;
					if (Depth - (Parptr->soilThicknessAllLyrsPD[index] - Parptr->tableDepthPD[index]) >= 1e-8) {
						DeepWaterGap = (DeepPorosity - Parptr->multi_soilMoisturePD[NRootLayers][index]) * DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index];
						DeepExtracWater = (Parptr->satFlowPD[index] > DeepWaterGap) ? DeepWaterGap : Parptr->satFlowPD[index];

						Parptr->multi_soilMoisturePD[NRootLayers][index] += DeepExtracWater / (DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index]);
						
						if (Statesptr->save_poi == ON)
						{
							// 如果是河道栅格，则来自相邻像元补给的satFlowPD是以栅格面积计算的等效水深，这里仍然以栅格面积为基准计算
							Poisptr->soil_lat_flowin_Grid[NRootLayers][index] += DeepExtracWater * 1000.0; // m/gwstep-> mm/gwstep
							Poisptr->soil_lat_flowin_Grid_allLyr[index] += DeepExtracWater * 1000.0; // m/gwstep-> mm/gwstep
							//if (fabs(Poisptr->soil_lat_flowin_Grid[NRootLayers][index]) <= 1e-8) {
							//	Poisptr->soil_lat_flowin_Grid[NRootLayers][index] = 0.0;
							//}
							Poisptr->soil_water_depth_Grid[NRootLayers][index] = Parptr->multi_soilMoisturePD[NRootLayers][index] * Parptr->multi_soilThicknessPD[NRootLayers][index] * 1000.0;  // cm
							
						}

						Parptr->satFlowPD[index] -= DeepExtracWater;
						
					}

					if (Parptr->satFlowPD[index] > 0.0) {
						for (int lyr = NRootLayers - 1; lyr >= 0; lyr--) {
							WaterGap = 0.0;
							ExtracWater = 0.0;
							Depth += Parptr->multi_soilThicknessPD[lyr][index];
							if (Depth - (Parptr->soilThicknessAllLyrsPD[index] - Parptr->tableDepthPD[index]) >= 1e-8 ) {
								WaterGap = (Parptr->multi_soilPorosityPD[lyr][index] - Parptr->multi_soilMoisturePD[lyr][index]) * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
								ExtracWater = (Parptr->satFlowPD[index] > WaterGap) ? WaterGap : Parptr->satFlowPD[index];

								Parptr->multi_soilMoisturePD[lyr][index] += ExtracWater / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
								
								if (Statesptr->save_poi == ON)
								{

									Poisptr->soil_lat_flowin_Grid[lyr][index] += ExtracWater * 1000.0;   // m/gwstep -> mm/gwstep
									Poisptr->soil_lat_flowin_Grid_allLyr[index] += ExtracWater * 1000.0; // m/gwstep-> mm/gwstep
									//if (fabs(Poisptr->soil_lat_flowin_Grid[lyr][index]) <= 1e-8) {
									//	Poisptr->soil_lat_flowin_Grid[lyr][index] = 0.0;
									//}
									Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // cm
									
								}

								Parptr->satFlowPD[index] -= ExtracWater;
							
							}
							if (fabs(Parptr->satFlowPD[index]) <= 1e-8)
								break;

						}
					}
				}
				
				//// 如果土壤饱和了，壤中流还剩余，就补给到地表径流
				if (Parptr->satFlowPD[index] > 1e-8) {
					//volume_grid[index] += Parptr->satFlowPD[index] * row_cell_area;
					// 对于河道栅格，satFlowPD是以栅格面积为基准的等效水深，所以这里乘以row_cell_area等于体积
					Parptr->satFlow2SurfPD[index] += Parptr->satFlowPD[index] * row_cell_area;

					//float* latLong = getLatLongByIndex(index, grid_cols_padded, Parptr);
					//bool inCh = (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0));
					//cout << "Run WARNING:excess water in soil is " << Parptr->satFlowPD[index] << " index: " << index << " lat: " << latLong[0] << " lon: " << latLong[1] << " inCh: " << inCh << endl;
					//delete[] latLong;
					
					if (Statesptr->save_poi == ON)
					{
						Poisptr->soil_lat_flowin_Grid_allLyr[index] += Parptr->satFlowPD[index] * 1000.0; // m/gwstep-> mm/gwstep
						Poisptr->surf_water_depth_Grid[index] += Parptr->satFlowPD[index] * 1000.0;
						
					}
					
				}

				// ******************************上层壤中流*********************************
				// 如果是流出，就从最上层开始扣
				//if (Parptr->satFlowUpPD[index] < 0.0) {
				//	for (int lyr = 0; lyr < Parptr->lyrOfWaterTableUpLayer[index]; lyr++) {
				//		AvaWater = (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]) * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
				//		// 上个步长routeSubsurface里算出来的SatFlow如果比这里算出来的AvaWater大，说明之前计算的SatFlow太多了。ExtracWater是个负值，是第i层对应的新的可以扣除的水量
				//		ExtracWater = (-Parptr->satFlowUpPD[index] > AvaWater) ? -AvaWater : Parptr->satFlowPD[index];
				//		// 第i层的土壤湿度 - 需要扣除的比例
				//		Parptr->multi_soilMoisturePD[lyr][index] += ExtracWater / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
				//		if (Statesptr->save_poi == ON)
				//		{
				//			Poisptr->soil_lat_flowin_Grid[lyr][index] += ExtracWater * 100.0 * 3600 / Parptr->gwTstep;  // m -> cm/h
				//			if (fabs(Poisptr->soil_lat_flowin_Grid[lyr][index]) <= 1e-8) {
				//				Poisptr->soil_lat_flowin_Grid[lyr][index] = 0.0;
				//			}
				//			Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] * 100.0;  // cm
				//		}
				//		// 给上个步长计算的SatFlow加上新的可以扣除的水量，看是否等于0，即之前第i层是否扣多了
				//		Parptr->satFlowUpPD[index] -= ExtracWater;

				//		//printf("SatFlow after layer %d is %.6f\n",i,SatFlow);
				//		// 如果没扣多
				//		if (fabs(Parptr->satFlowUpPD[index]) <= 1e-8) {
				//			//cout << index << ": " << fabs(Parptr->satFlowPD[index]) << endl;
				//			Parptr->satFlowUpPD[index] = 0.0;
				//			break;
				//		}
				//	}
				//}
				// 如果是流入，就从最上层开始往下补给
				//if (Parptr->satFlowUpPD[index] > 1e-8)
				//{
				//	for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++) {

				//		WaterGap = (Parptr->multi_soilPorosityPD[lyr][index] - Parptr->multi_soilMoisturePD[lyr][index]) * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
				//		ExtracWater = (Parptr->satFlowUpPD[index] > WaterGap) ? WaterGap : Parptr->satFlowUpPD[index];

				//		Parptr->multi_soilMoisturePD[lyr][index] += ExtracWater / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
				//		if (Statesptr->save_poi == ON)
				//		{
				//			Poisptr->soil_lat_flowin_Grid[lyr][index] = ExtracWater * 100.0 * 3600 / Parptr->gwTstep;   // m -> cm/h
				//			if (fabs(Poisptr->soil_lat_flowin_Grid[lyr][index]) <= 1e-8) {
				//				Poisptr->soil_lat_flowin_Grid[lyr][index] = 0.0;
				//			}
				//			Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] * 100.0;  // cm
				//		}
				//		Parptr->satFlowUpPD[index] -= ExtracWater;
				//		if (fabs(Parptr->satFlowUpPD[index]) <= 1e-8)
				//			break;
				//	
				//	}

				//	//// 如果土壤饱和了，壤中流还剩余，就补给到地表径流
				//	if (Parptr->satFlowUpPD[index] > 1e-8) {
				//	
				//		Parptr->satFlow2SurfPD[index] += Parptr->satFlowUpPD[index] * row_cell_area;

				//		if (Statesptr->save_poi == ON)
				//		{
				//			Poisptr->surf_water_depth_Grid[index] += Parptr->satFlowUpPD[index];
				//		}

				//	}

				//}
				// ******************************上层壤中流*********************************
				

				
			}
		}
	}

	float* getLatLongByIndex(int index, int grid_cols_padded, Pars *Parptr) {
		int col = index % grid_cols_padded;
		int row = index / grid_cols_padded;
		// 计算像元中心点的经纬度
		float lon = Parptr->blx + (col + 0.5) * Parptr->dx;
		float lat = Parptr->tly - (row + 0.5) * Parptr->dy;

		// 动态分配数组，返回 lat 和 lon
		float* result = new float[2];

		result[0] = lat;
		result[1] = lon;
		return result;
	}

	inline void RouteSubSurfaceUpLayer(const int row_start, int row_end,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid,
		NUMERIC_TYPE dx_col, NUMERIC_TYPE dy_col, WetDryRowBound* wet_dry_bounds, Pois*Poisptr, const NUMERIC_TYPE *dem_grid) {

		NUMERIC_TYPE BankHeight;
		NUMERIC_TYPE *Adjust;
		NUMERIC_TYPE fract_used;
		NUMERIC_TYPE depth;
		NUMERIC_TYPE OutFlow;
		NUMERIC_TYPE water_out_road;
		NUMERIC_TYPE Transmissivity;
		NUMERIC_TYPE AvailableWater;
		int k;
		int index;
		NUMERIC_TYPE* waterLevelPD; NUMERIC_TYPE* subFlowGradPD; unsigned char **subDirPD; unsigned int *subTotalDirPD;

		NUMERIC_TYPE Storage = 0.0;
		NUMERIC_TYPE ExcessFCap = 0.0;
		
		/* 省略 reset the saturated subsurface flow to zero */
		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				// todo：先根据地下水位判断现在水位处于哪个层，如果不在表层，则：
				// 如果下游是坡面:先根据上层的湿度计算上层的等效水位，找到水力坡降最大的方向，再用seims的方法计算有多少侧向水传递给下游，将这些侧向水流加入下游栅格的第一层，第一层满了则加入第二层，依次类推；
				// 如果下游是河道，则上游根据上层的湿度计算上层的等效水位，下游水头取河道水面

				// 省略 计算河床底部以上的侧面高程，如果BankHeight>土壤厚度说明/河床以上存在裸露岩石，反之说明河床高程之下是土壤
				//Adjust = Parptr->multi_adjustPD[index];
				fract_used = 0.0f;
				water_out_road = 0.0;
				int index = i + grid_row_index;
				int source_index_this = row * Parptr->xsz + i;
				// 计算河床底部以上的侧面高程，如果BankHeight>土壤厚度说明/河床以上存在裸露岩石，反之说明河床高程之下是土壤
				//BankHeight = (Network[y][x].BankHeight > SoilMap[y][x].Depth) ?  SoilMap[y][x].Depth : Network[y][x].BankHeight;
				// todo: 检查像元值对不对
				BankHeight = Arrptr->SGCbfH[source_index_this] > Parptr->soilThicknessAllLyrsPD[index] ? Parptr->soilThicknessAllLyrsPD[index] : Arrptr->SGCbfH[source_index_this];
				if (BankHeight < -1e-8)
				{
					cout << "index: " << index << BankHeight << endl;
				}
				//int curWaterTableUpLayer = 0;
				NUMERIC_TYPE accSoilThickness = 0.0;
				// 如果地下水位>第1层的土壤厚度，则计算地下水位现在处于哪一层
				if (Parptr->tableDepthPD[index] > Parptr->multi_soilThicknessPD[0][index])
				{
					accSoilThickness = Parptr->multi_soilThicknessPD[0][index];
					for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs - 1; lyr++)
					{
						// 如果地下水位在lyr层之下，则计算lyr层的水位深度
						if (Parptr->tableDepthPD[index] > accSoilThickness)
						{
							Parptr->lyrOfWaterTableUpLayer[index]++;
							accSoilThickness += Parptr->multi_soilThicknessPD[lyr+1][index];
							Parptr->tableDepthUpLyrPD[index] = 0.0;
							// 如果第lyr层的土壤湿度超出田间持水量，则上层水位位于这一层，且水头深度等于第lyr层(土壤孔隙度-土壤湿度)*土壤厚度
							if (Parptr->multi_soilMoisturePD[lyr][index] > Parptr->multi_soilFcPD[lyr][index])
							{
								Storage = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] * (Parptr->multi_soilPorosityPD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]);
								ExcessFCap = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] * (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]);
								
								Parptr->tableDepthUpLyrPD[index] += (1 - ExcessFCap / Storage)*Parptr->multi_soilThicknessPD[lyr][index];
							}// 否则上层水位深度直接加上当前层的土壤厚度
							else {
								Parptr->tableDepthUpLyrPD[index] += Parptr->multi_soilThicknessPD[lyr][index];
							}
						}
					}
					// 计算上层的地下水位
					Parptr->waterLevelUpLyrPD[index] = dem_row[i] - Parptr->tableDepthUpLyrPD[index];
					// 计算流向
					HeadSlopeAspectForUpLyr(Parptr, Solverptr, Arrptr, grid_row_index, row, grid_rows, grid_cols, dem_row, row_start, row_end, grid_cols_padded, dx_col, dy_col, wet_dry_bounds,i);

					// 假如地下水位在第2层，则对第1层进行计算;地下水位在第3层则对第1、2层计算
					NUMERIC_TYPE curLyrAccThickness = 0.0;
					for (int lyr  = 0; lyr < Parptr->lyrOfWaterTableUpLayer[index]; lyr++)
					{
						// 如果是河道
						if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
							// 如果当前层的水位深度小于河堤，才能发生侧向流
							if (Parptr->tableDepthUpLyrPD[index] > 0.0 && Parptr->tableDepthUpLyrPD[index] < BankHeight) {
								// todo 根据SIMES or DHSVM的方法计算上层侧向流量，如果是根据DHSVM算，则直接套用原来的公式，需要理清cell grid soil thickness取上面的一两层土壤还是全部土层？
								// 如果是SEIMS，则一层一层算小于curWaterTableUpLayer的土壤层的侧向水流
								float gradient = 4.0 * (BankHeight - Parptr->tableDepthUpLyrPD[index]);
								if (gradient < 0.0)
									gradient = 0.0;
								Transmissivity =
									CalcTransmissivity(BankHeight, Parptr->tableDepthUpLyrPD[index],
										Parptr->ksLatPD[index] * 0.001*0.000277778,
										Parptr->KsLatExpValue,
										Parptr->soilWaterDepthThresh);

								OutFlow = (Transmissivity * gradient * Parptr->gwTstep) / row_cell_area;

								/* check whether enough water is available for redistribution */
								AvailableWater =
									CalcAvailableWaterForUplyr(Parptr->lyrOfWaterTableUpLayer[index],     // todo：待验证
										BankHeight, Parptr->multi_soilThicknessPD,Parptr->multi_soilPorosityPD,Parptr->multi_soilFcPD,
										Parptr->tableDepthUpLyrPD, Parptr->multi_adjustPD, Parptr->multi_soilMoisturePD, index);
								// 如果剩余可供侧向壤中流的水不够outFlow计算出来的应该流走的量，就只能流走AvailableWater
								OutFlow = (OutFlow > AvailableWater) ? AvailableWater : OutFlow;

								/* remove water going to channel from the grid cell */
								Parptr->satFlowUpPD[index] -= OutFlow;

								/* contribute to channel segment lateral inflow todo：这里认真检查是否加到河道上了 */
								//channel_grid_inc_inflow(volume_grid, index, OutFlow * row_cell_area);
								// 这个实现只能单线程
								//Parptr->subSurfaceLatFlow2ChTotal += OutFlow * row_cell_area;
								Parptr->satFlow2ChPD[index] += OutFlow * row_cell_area;
							}
						}
						else {  // 如果是坡面
							for (k = 0; k < NDIRS; k++) {
								fract_used += (NUMERIC_TYPE)Parptr->subDirUpLyrPD[index][k];
							}
							if (Parptr->subTotalDirUpLyrPD[index] > 0)
								fract_used /= (NUMERIC_TYPE)Parptr->subTotalDirUpLyrPD[index];
							else
								fract_used = 0.;

							// 上层水位在土壤底部之上 
							if (Parptr->tableDepthUpLyrPD[index] > 0.0 && Parptr->tableDepthUpLyrPD[index] < Parptr->soilThicknessAllLyrsPD[index]) {
								// 地下水位在河道底部之下，depth取自身；否则，depth要取BankHeight，

								Transmissivity = CalcTransmissivity(Parptr->soilThicknessAllLyrsPD[index], Parptr->tableDepthUpLyrPD[index],
									Parptr->ksLatPD[index] * 0.001*0.000277778,  // 每个土壤柱的侧向饱和水力传导度是空间异质的
									Parptr->KsLatExpValue,   // DHSVM每个土壤柱只对应单一的土壤类型，这里将KsLatExp设置为用户可调的参数
									Parptr->soilWaterDepthThresh);

								OutFlow = (Transmissivity * fract_used * Parptr->subFlowGradPD[index] * Parptr->gwTstep) / row_cell_area;

								/* check whether enough water is available for redistribution ,AvailableWater (m)*/
								AvailableWater =
									CalcAvailableWaterForUplyr(Parptr->lyrOfWaterTableUpLayer[index],     // todo：待验证
										Parptr->soilThicknessAllLyrsPD[index], Parptr->multi_soilThicknessPD, Parptr->multi_soilPorosityPD, Parptr->multi_soilFcPD,
										Parptr->tableDepthUpLyrPD, Parptr->multi_adjustPD, Parptr->multi_soilMoisturePD, index);
								OutFlow = (OutFlow > AvailableWater) ? AvailableWater : OutFlow;
							}
							else {
								depth = Parptr->soilThicknessAllLyrsPD[index];
								OutFlow = 0.0f;
							}

							/* Subsurface Component - Decrease water change by outwater */
							Parptr->satFlowUpPD[index] -= OutFlow;

							/* Assign the water to appropriate surrounding pixels */
							if (Parptr->subTotalDirUpLyrPD[index] > 0)
								OutFlow /= (NUMERIC_TYPE)Parptr->subTotalDirUpLyrPD[index];
							else
								OutFlow = 0.;

							for (k = 0; k < NDIRS; k++) {
								int neighbor_index = i + grid_row_index + Parptr->neighbor_ref[k];
								int neighbor_row = row + Parptr->neighbor_row_ref[k];
								int neighbor_col = i + Parptr->neighbor_col_ref[k];
								if (valid_cell(neighbor_row, neighbor_col, wet_dry_bounds->dem_data[neighbor_row].start, wet_dry_bounds->dem_data[neighbor_row].end, grid_rows)) {
									Parptr->satFlowUpPD[neighbor_index] += OutFlow * Parptr->subDirUpLyrPD[index][k];
									Parptr->satFlow2NeiborPD[neighbor_index] = OutFlow * Parptr->subDirUpLyrPD[index][k] * row_cell_area;
								}
							}
						
						}
					}
				}
			}
		}

		return;
	}

	float WaterTableDepth(Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, Pois*Poisptr, const NUMERIC_TYPE row_cell_area,  int NSoilLayers, int NRootLayers,int index)
	{
		NUMERIC_TYPE DeepFCap = 0.0;				/* field capacity of the layer below the deepest root layer */
		NUMERIC_TYPE DeepLayerDepth = 0.0;		/* depth of layer below deepest root zone layer */
		NUMERIC_TYPE DeepPorosity = 0.0;			/* porosity of the layer below the deepest  root layer */
		NUMERIC_TYPE TableDepth = 0.0;				/* depth of the water table (m) */
		NUMERIC_TYPE MoistureTransfer;				/* amount of soil moisture transferred from the current layer to the layer above (m) */
		int i;																/* counter */
		NUMERIC_TYPE TotalStorage = 0.0;
		NUMERIC_TYPE ExcessFCap = 0.0;
		NUMERIC_TYPE Storage = 0.0;
		NUMERIC_TYPE DeepStorage = 0.0;
		NUMERIC_TYPE DeepExcessFCap = 0.0;

		MoistureTransfer = 0.0;


		DeepLayerDepth = Parptr->soilThicknessAllLyrsPD[index];
		DeepPorosity = Parptr->multi_soilPorosityPD[NRootLayers][index];  // RootLayers的最后一层不是DeepLayer
		DeepFCap = Parptr->multi_soilFcPD[NRootLayers][index];

		for (int lyr = 0; lyr < NRootLayers; lyr++)
			DeepLayerDepth -= Parptr->multi_soilThicknessPD[lyr][index];
		
		/* Redistribute soil moisture.  I.e. water from supersaturated layers is
		transferred to the layer immediately above */
		// 如果deeplayer超饱和了，就计算其向上层传递多少水mm
		if (Parptr->multi_soilMoisturePD[NRootLayers][index] - DeepPorosity >= -1e-8) {
			MoistureTransfer = (Parptr->multi_soilMoisturePD[NRootLayers][index] - DeepPorosity) * DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index];
			if (MoistureTransfer <= 1e-8)
			{
				MoistureTransfer = 0.0;
			}
			// 将最下层的湿度更新为土壤孔隙度

			Parptr->multi_soilMoisturePD[NRootLayers][index] = DeepPorosity;
			
			if (Statesptr->save_poi == ON)
			{

				Poisptr->soil_water_depth_Grid[NRootLayers][index] = Parptr->multi_soilMoisturePD[NRootLayers][index] * Parptr->multi_soilThicknessPD[NRootLayers][index] * 1000.0;  // cm
				
			}
			// 逐层向上计算
			for (int lyr = NRootLayers - 1; lyr >= 0; lyr--) {

				Parptr->multi_soilMoisturePD[lyr][index] += MoistureTransfer / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
				
				// 移动湿润锋面
				if (Statesptr->use_change_acccum_depth && lyr == 0)
				{
					Parptr->accumuDepthPD[index] += MoistureTransfer * 1000.0;
					if (Parptr->accumuDepthPD[index] < 1e-8)
					{
						Parptr->accumuDepthPD[index] = 0.0;
					}
					if (Parptr->accumuDepthPD[index] > Parptr->multi_soilThicknessPD[lyr][index] * 1000.0)
					{
						Parptr->accumuDepthPD[index] = Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
					}
				}

				if (Statesptr->save_poi == ON)
				{
					if (MoistureTransfer <= 1e-8)
					{
						MoistureTransfer = 0.0;
					}

					//Poisptr->soil_perc_Grid[lyr][index] -= MoistureTransfer * 1000.0 / (Parptr->multi_adjustPD[lyr][index]);  // m/gwstep->mm/gwstep  相对于自己的深度变化
					Poisptr->soil_perc_Grid[lyr][index] -= MoistureTransfer * 1000.0;  // m/gwstep->mm/gwstep  相对于自己的深度变化
					Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // mm
					
				}
				//Parptr->multi_soilPercoPD[lyr][index] -= MoistureTransfer / Parptr->multi_adjustPD[lyr][index];
				Parptr->multi_soilPercoPD[lyr][index] -= MoistureTransfer;
				// 如果这一层也饱和了
				if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilPorosityPD[lyr][index] >= -1e-8) {
					{
						MoistureTransfer = (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilPorosityPD[lyr][index]) * Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
					}
					if (MoistureTransfer <= 1e-8)
					{
						MoistureTransfer = 0.0;
					}

					Parptr->multi_soilMoisturePD[lyr][index] = Parptr->multi_soilPorosityPD[lyr][index];
					
					if (Statesptr->save_poi == ON)
					{
						//Poisptr->soil_perc_Grid[lyr][index] -= MoistureTransfer * 1000.0 / (Parptr->multi_adjustPD[lyr][index]);  // m/gwstep->mm/gwstep
						Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // cm
						
					}
				}
				else {
					MoistureTransfer = 0.0;
					break;
				}
			}
		}
		else {
			// 如果deeplayer没有超饱和，则不向上补给
			MoistureTransfer = 0.0;
		}
		// 如果每一层都饱和了，则淹没地表，TableDepth为负值
		if (MoistureTransfer >= 1e-8) {
			/* Surface ponding occurs */
			TableDepth = -MoistureTransfer;
			if (Statesptr->save_poi == ON)
			{

				Poisptr->surf_water_depth_Grid[index] += -TableDepth * 1000.0;  // cm
				
			}
		}
		else {
			/* Warning added by Pascal Storck, 08/15/2000 */
			/* Based on a single bad parameter in a DHSVM input file (a third layer
			vertical hydraulic conductivity that was 10 times smaller than the layer
			above it), it was noted that DHSVM can develop what are basically perched
			water tables.  These perched water tables greatly complicate the calculation
			of the pixel water table depth because the soil below the perched table
			is not completely saturated above field capacity.
			For example, if we have three soil layers and a deep layer, all 1 meter thick,
			and we saturate the second layer from the surface, what is, or should be, the water
			table depth. Should we allow subsurface flow to occur, should we include the saturation
			of disconnected overlying layers in the calculation of the hydraulic gradient.
			At this point, just be cautious.  Using any combination of soil parameters or
			intial water states which can cause the lower layers of the soil profile
			to drain more quickly than water can flow down through the matrix will
			result in mass balance problems.  I.e. water will be forced out of the cell
			to the downslope, this water will be taken from the deepest soil layer, which
			can cause the deep layer soil moisture to go negative. */

			DeepStorage = DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index] * (DeepPorosity - DeepFCap);  // deeplayer最多容纳多少水
			DeepExcessFCap = DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index] * (Parptr->multi_soilMoisturePD[NRootLayers][index] - DeepFCap);  // deeplayer当前的水量-田间持水量
			// 如果deeplayer层不满
			if (DeepExcessFCap <= -1e-8) {
				TableDepth = Parptr->soilThicknessAllLyrsPD[index];  // deeplayer层的水也小于田间持水量，则地下水位设置为土壤厚度
			}
			else {
				// 从下到上逐层判断地下水埋深在哪一层
				TableDepth = Parptr->soilThicknessAllLyrsPD[index] - (DeepExcessFCap / DeepStorage)*DeepLayerDepth;   //  超出deeplayer以上的水深 = 土壤总厚度 -  deeplayer现存水量占deeplayer孔隙度的%数  * deeplayer的厚度

				if (Parptr->multi_soilMoisturePD[NRootLayers][index] - DeepPorosity >= -1e-8) {
					for (int lyr = NRootLayers - 1; lyr >= 0; lyr--) {
						Storage = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] * (Parptr->multi_soilPorosityPD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]);
						ExcessFCap = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] * (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]);
						// ExcessFCap < 0 意味着第i层土壤湿度小于田间持水量，则水位仍停留在下一层，一点也不涨
						if (ExcessFCap < 0.0)
							ExcessFCap = 0.0f;
						// 如果第i层没饱和，根据第i层的土壤湿度计算地下水埋深
						if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilPorosityPD[lyr][index] <= -1e-8) {
							TableDepth -= (ExcessFCap / Storage)*Parptr->multi_soilThicknessPD[lyr][index];
							break;

						}
						else {
							// 如果第i层饱和了，地下水埋深中就减去第i层的厚度
							TableDepth -= Parptr->multi_soilThicknessPD[lyr][index];
						}
					}
				}
			}
		}
		//printf("Table Depth is %.6f\n",TableDepth);
		if (TableDepth - Parptr->soilThicknessAllLyrsPD[index] > 1e-8)
			printf("TableDepth = %.4f, TotalDepth = %.4f\n", TableDepth, Parptr->soilThicknessAllLyrsPD[index]);
		if (fabs(TableDepth - TableDepth) > 1e-8)
			printf("TableDepth = %.2f", TableDepth);


		return TableDepth;
	}



	inline void UnsaturatedFlow(const int row_start, int row_end,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid,
		int NSoilLayers, int NRootLayers, Pois*Poisptr)
	{
		float DeepDrainage;		/* amount of drainage from the lowest root
									 zone to the layer below it (m) */
		float DeepLayerDepth;		/* depth of the layer below the deepest root layer */
		float Drainage;		    /* amount of water drained from each soil
									 layer during the current timestep */
		float Exponent;		    /* Brooks-Corey exponent */
		float FieldCapacity;		/* amount of water in soil at field capacity (m) */
		float MaxSoilWater;		/* maximum allowable amount of soil moiture in each layer (m) */
		float SoilWater;		    /* amount of water in each soil layer (m) */

		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				int index = i + grid_row_index;
				DeepLayerDepth = Parptr->soilThicknessAllLyrsPD[index];
				for (int lyr = 0; lyr < NRootLayers; lyr++)
					DeepLayerDepth -= Parptr->multi_soilThicknessPD[lyr][index];

				// xiaodw, 这里思路是先计算渗漏量，即使下层超饱和了，上层也向下层渗漏。在WaterTableDepth方法中再从下层往上层计算淹没，纠正过来
				/* from top to bottom soil layer */

				for (int lyr = 0; lyr < NRootLayers; lyr++) {

					/* No movement if soil moisture is below field capacity */
					if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index] >= 1e-8) {
						Exponent = 2.0 / Parptr->multi_soilPoreIndexPD[lyr][index] + 3.0;

						if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilPorosityPD[lyr][index] >= 1e-8)
							/* this can happen because the moisture content can exceed the
							porosity the way the algorithm is implemented */
							Drainage = Parptr->multi_soilKsPD[lyr][index] * Parptr->multi_ksFactorVOfLyr[lyr];
						else
							Drainage = Parptr->multi_soilKsPD[lyr][index] * Parptr->multi_ksFactorVOfLyr[lyr] * pow((double)(Parptr->multi_soilMoisturePD[lyr][index] / Parptr->multi_soilPorosityPD[lyr][index]), (double)Exponent);
						/* convert from mm/h to m */
						Drainage = Drainage * Parptr->gwTstep * 0.0002777778 * 0.001;

						/* percolation = drainage + perc from layer above渗透量（Perc[lyr]），它是当前步长与上步长渗透量的平均。 */
						//Parptr->multi_soilPercoPD[lyr][index] = 0.5 * (Parptr->multi_soilPercoPD[lyr][index] + Drainage) * row_cell_area;  这里注释掉是因为都不按照体积计算，统一按照深度计算 
						// xiaodw, 对河道只有两侧洪泛区发生渗漏 
						Parptr->multi_soilPercoPD[lyr][index] = 0.5 * (Parptr->multi_soilPercoPD[lyr][index] + Drainage) * Parptr->multi_adjustPD[lyr][index];
						
						// Parptr->multi_adjustPD[lyr][index]是为了调整河道所在栅格的土壤层的体积
						MaxSoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilPorosityPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
						SoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
						FieldCapacity = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilFcPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
						//MaxSoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilPorosityPD[lyr][index];
						//SoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilMoisturePD[lyr][index];
						//FieldCapacity = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilFcPD[lyr][index];

						/* No unsaturated flow if the moisture content drops below field capacity */

						if ((SoilWater - Parptr->multi_soilPercoPD[lyr][index] - FieldCapacity) <= 1e-8) {
							Parptr->multi_soilPercoPD[lyr][index] = SoilWater - FieldCapacity;
						}
						
						// 如果按照正常的扣除渗漏后，还是过饱和，则将过饱和的水都加入渗漏量
						SoilWater -= Parptr->multi_soilPercoPD[lyr][index];
						if (SoilWater - MaxSoilWater >= 1e-8) {
							Parptr->multi_soilPercoPD[lyr][index] += SoilWater - MaxSoilWater;
						}

						/* Adjust the moisture content in the current layer, and the layer immediately below it */
						Parptr->multi_soilMoisturePD[lyr][index] -= Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
						//Parptr->multi_soilMoisturePD[lyr][index] -= Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr][index]);
						
						if (lyr < (NRootLayers - 1)) {

							//Parptr->multi_soilMoisturePD[lyr + 1][index] += Parptr->multi_soilPercoPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr + 1][index] * Parptr->multi_adjustPD[lyr + 1][index]);
							Parptr->multi_soilMoisturePD[lyr + 1][index] += Parptr->multi_soilPercoPD[lyr][index]  / (Parptr->multi_soilThicknessPD[lyr + 1][index] * Parptr->multi_adjustPD[lyr + 1][index]);
							
						}
						if (Statesptr->use_change_acccum_depth && lyr == 0)
						{
							Parptr->accumuDepthPD[index] -= Parptr->multi_soilPercoPD[lyr][index] * 1000.0;
							if (Parptr->accumuDepthPD[index] < 1e-8)
							{
								Parptr->accumuDepthPD[index] = 0.0;
							}
							if (Parptr->accumuDepthPD[index] > Parptr->multi_soilThicknessPD[lyr][index] * 1000.0)
							{
								Parptr->accumuDepthPD[index] = Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
							}
						}
					}
					else {
						Parptr->multi_soilPercoPD[lyr][index] = 0.0;
					}
					Parptr->multi_soilWaterDepthPD[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
					if (Statesptr->save_poi == ON)
					{
						//Poisptr->soil_perc_Grid[lyr][index] = Parptr->multi_soilPercoPD[lyr][index] * 100.0 * 3600 / Parptr->gwTstep;  // m->cm /h
						Poisptr->soil_perc_Grid[lyr][index] += Parptr->multi_soilPercoPD[lyr][index] * 1000.0;  // m/gwstep->mm /gwstep
						Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // mm
						
					}
					
					/* convert back to straight 1-d flux */
					//Parptr->multi_soilPercoPD[lyr][index] /= row_cell_area;
				}

				// 之前是直接从最下层扣除侧向壤中流
				/* Change: dont extract outflow from the bottom layer only. The new function DistributeSatFlow
				extract water from top to bottom layer to avoid negative soil mositure (overdraw) in the bottom
				layer (below root zone layers */
				/* DeepDrainage = (Perc[NSoilLayers - 1] * PercArea[NSoilLayers - 1]) + SatFlow; */

				//DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index] * row_cell_area);
				DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index]);
				//DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index] * Parptr->multi_adjustPD[NRootLayers - 1][index]);

				Parptr->multi_soilMoisturePD[NRootLayers][index] += DeepDrainage / (DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index]);
				
				if (Statesptr->save_poi == ON)
				{
					Poisptr->soil_water_depth_Grid[NRootLayers][index] = Parptr->multi_soilMoisturePD[NRootLayers][index] * Parptr->multi_soilThicknessPD[NRootLayers][index] * 1000.0;  // cm
				}

				/* Calculate the depth of the water table based on the soil moisture
				profile and adjust the soil moisture profile, to assure that the soil
				moisture is never more than the maximum allowed soil moisture amount,
				i.e. the porosity.  A negative water table depth means that the water is
				ponding on the surface.  This amount of water becomes surface Runoff */

				Parptr->tableDepthPD[index] = WaterTableDepth(Parptr, Solverptr, Arrptr, Statesptr, Poisptr, row_cell_area, NSoilLayers, NRootLayers, index);
				

				// 地下水位高出地表
				if (Parptr->tableDepthPD[index] <= -1e-8) {
					//volume_grid[index] += -(Parptr->tableDepthPD[index]) * row_cell_area;
					//Parptr->PercExcess2SurfPD[index] += -(Parptr->tableDepthPD[index]) * row_cell_area;

					Parptr->tableDepthPD[index] = 0.0;
					
				}

			}
		}
	}


	inline void UnsaturatedFlowGwVersion(const int row_start, int row_end,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid,
		int NSoilLayers, int NRootLayers, Pois*Poisptr)
	{
		NUMERIC_TYPE DeepDrainage;		/* amount of drainage from the lowest root
									 zone to the layer below it (m) */
		NUMERIC_TYPE DeepLayerDepth;		/* depth of the layer below the deepest root layer */
		NUMERIC_TYPE Drainage;		    /* amount of water drained from each soil
									 layer during the current timestep */
		NUMERIC_TYPE Exponent;		    /* Brooks-Corey exponent */
		NUMERIC_TYPE FieldCapacity;		/* amount of water in soil at field capacity (m) */
		NUMERIC_TYPE MaxSoilWater;		/* maximum allowable amount of soil moiture in each layer (m) */
		NUMERIC_TYPE SoilWater;		    /* amount of water in each soil layer (m) */
		NUMERIC_TYPE gwDrainage;

		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				int index = i + grid_row_index;
				DeepLayerDepth = Parptr->soilThicknessAllLyrsPD[index];
				for (int lyr = 0; lyr < NRootLayers; lyr++)
					DeepLayerDepth -= Parptr->multi_soilThicknessPD[lyr][index];

				// xiaodw, 这里思路是先计算渗漏量，即使下层超饱和了，上层也向下层渗漏。在WaterTableDepth方法中再从下层往上层计算淹没，纠正过来
				// xiaodw，把最下层向地下水库的渗漏也计算出来
				/* from top to bottom soil layer */

				for (int lyr = 0; lyr <= NRootLayers; lyr++) {

					/* No movement if soil moisture is below field capacity */
					if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index] >= 1e-8) {
						Exponent = 2.0 / (Parptr->multi_soilPoreIndexPD[lyr][index] * Parptr->poreIndexScaleFactor) + 3.0;

						if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilPorosityPD[lyr][index] >= 1e-8)
							/* this can happen because the moisture content can exceed the
							porosity the way the algorithm is implemented */
							Drainage = Parptr->multi_soilKsPD[lyr][index] * Parptr->multi_ksFactorVOfLyr[lyr];
						else
							Drainage = Parptr->multi_soilKsPD[lyr][index] * Parptr->multi_ksFactorVOfLyr[lyr] * pow((double)(Parptr->multi_soilMoisturePD[lyr][index] / Parptr->multi_soilPorosityPD[lyr][index]), (double)Exponent);
						/* convert from mm/h to m */
						Drainage = Drainage * Parptr->gwTstep * 0.0002777778 * 0.001;

						/* percolation = drainage + perc from layer above渗透量（Perc[lyr]），它是当前步长与上步长渗透量的平均。 */
						//Parptr->multi_soilPercoPD[lyr][index] = 0.5 * (Parptr->multi_soilPercoPD[lyr][index] + Drainage) * row_cell_area;  这里注释掉是因为都不按照体积计算，统一按照深度计算 
						// xiaodw, 对河道只有两侧洪泛区发生渗漏 
						Parptr->multi_soilPercoPD[lyr][index] = 0.5 * (Parptr->multi_soilPercoPD[lyr][index] + Drainage) * Parptr->multi_adjustPD[lyr][index];

						// Parptr->multi_adjustPD[lyr][index]是为了调整河道所在栅格的土壤层的体积
						MaxSoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilPorosityPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
						SoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
						FieldCapacity = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilFcPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
						//MaxSoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilPorosityPD[lyr][index];
						//SoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilMoisturePD[lyr][index];
						//FieldCapacity = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilFcPD[lyr][index];

						/* No unsaturated flow if the moisture content drops below field capacity */

						if ((SoilWater - Parptr->multi_soilPercoPD[lyr][index] - FieldCapacity) <= 1e-8) {
							Parptr->multi_soilPercoPD[lyr][index] = SoilWater - FieldCapacity;
						}

						// 如果按照正常的扣除渗漏后，还是过饱和，则将过饱和的水都加入渗漏量
						SoilWater -= Parptr->multi_soilPercoPD[lyr][index];
						if (SoilWater - MaxSoilWater >= 1e-8) {
							Parptr->multi_soilPercoPD[lyr][index] += SoilWater - MaxSoilWater;
						}

						// 最后一层只计算有多少水渗漏，不计算下一层的土壤湿度变化

						/* Adjust the moisture content in the current layer, and the layer immediately below it */
						Parptr->multi_soilMoisturePD[lyr][index] -= Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);
						//Parptr->multi_soilMoisturePD[lyr][index] -= Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr][index]);

						if (lyr < (NRootLayers - 1)) {

							//Parptr->multi_soilMoisturePD[lyr + 1][index] += Parptr->multi_soilPercoPD[lyr][index] * Parptr->multi_adjustPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr + 1][index] * Parptr->multi_adjustPD[lyr + 1][index]);
							Parptr->multi_soilMoisturePD[lyr + 1][index] += Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr + 1][index] * Parptr->multi_adjustPD[lyr + 1][index]);

						}
						if (Statesptr->use_change_acccum_depth && lyr == 0)
						{
							Parptr->accumuDepthPD[index] -= Parptr->multi_soilPercoPD[lyr][index] * 1000.0;
							if (Parptr->accumuDepthPD[index] < 1e-8)
							{
								Parptr->accumuDepthPD[index] = 0.0;
							}
							if (Parptr->accumuDepthPD[index] > Parptr->multi_soilThicknessPD[lyr][index] * 1000.0)
							{
								Parptr->accumuDepthPD[index] = Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
							}
						}
						

						
					}
					else {
						Parptr->multi_soilPercoPD[lyr][index] = 0.0;
					}
					Parptr->multi_soilWaterDepthPD[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
					if (Statesptr->save_poi == ON)
					{
						//Poisptr->soil_perc_Grid[lyr][index] = Parptr->multi_soilPercoPD[lyr][index] * 100.0 * 3600 / Parptr->gwTstep;  // m->cm /h
						Poisptr->soil_perc_Grid[lyr][index] += Parptr->multi_soilPercoPD[lyr][index] * 1000.0;  // m/gwstep->mm /gwstep
						Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // mm

					}

					/* convert back to straight 1-d flux */
					//Parptr->multi_soilPercoPD[lyr][index] /= row_cell_area;
				}

				// 之前是直接从最下层扣除侧向壤中流
				/* Change: dont extract outflow from the bottom layer only. The new function DistributeSatFlow
				extract water from top to bottom layer to avoid negative soil mositure (overdraw) in the bottom
				layer (below root zone layers */
				/* DeepDrainage = (Perc[NSoilLayers - 1] * PercArea[NSoilLayers - 1]) + SatFlow; */

				//DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index] * row_cell_area);
				DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index]);
				//DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index] * Parptr->multi_adjustPD[NRootLayers - 1][index]);

				Parptr->multi_soilMoisturePD[NRootLayers][index] += DeepDrainage / (DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index]);

				if (Statesptr->save_poi == ON)
				{
					Poisptr->soil_water_depth_Grid[NRootLayers][index] = Parptr->multi_soilMoisturePD[NRootLayers][index] * Parptr->multi_soilThicknessPD[NRootLayers][index] * 1000.0;  // cm
				}

				/* Calculate the depth of the water table based on the soil moisture
				profile and adjust the soil moisture profile, to assure that the soil
				moisture is never more than the maximum allowed soil moisture amount,
				i.e. the porosity.  A negative water table depth means that the water is
				ponding on the surface.  This amount of water becomes surface Runoff */

				Parptr->tableDepthPD[index] = WaterTableDepth(Parptr, Solverptr, Arrptr, Statesptr, Poisptr, row_cell_area, NSoilLayers, NRootLayers, index);


				// 地下水位高出地表
				if (Parptr->tableDepthPD[index] <= -1e-8) {
					//volume_grid[index] += -(Parptr->tableDepthPD[index]) * row_cell_area;
					//Parptr->PercExcess2SurfPD[index] += -(Parptr->tableDepthPD[index]) * row_cell_area;

					Parptr->tableDepthPD[index] = 0.0;

				}

			}
		}
	}

	inline void CalSoilPerc(Pars *Parptr,  int index, int lyr) {

		float Drainage;		    /* amount of water drained from each soil
									 layer during the current timestep */
		float Exponent;		    /* Brooks-Corey exponent */
		float FieldCapacity;		/* amount of water in soil at field capacity (m) */
		float MaxSoilWater;		/* maximum allowable amount of soil moiture in each layer (m) */
		float SoilWater;		    /* amount of water in each soil layer (m) */

		Exponent = 2.0 / Parptr->multi_soilPoreIndexPD[lyr][index] + 3.0;

		if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilPorosityPD[lyr][index] >= 1e-8)
			/* this can happen because the moisture content can exceed the
			porosity the way the algorithm is implemented */
			Drainage = Parptr->multi_soilKsPD[lyr][index] * Parptr->multi_ksFactorVOfLyr[lyr];
		else
			Drainage = Parptr->multi_soilKsPD[lyr][index] * Parptr->multi_ksFactorVOfLyr[lyr] * pow((double)(Parptr->multi_soilMoisturePD[lyr][index] / Parptr->multi_soilPorosityPD[lyr][index]), (double)Exponent);
		/* convert from mm/h to m */
		Drainage = Drainage * Parptr->gwTstep * 0.0002777778 * 0.001;

		/* percolation = drainage + perc from layer above渗透量（Perc[lyr]），它是当前步长与上步长渗透量的平均。 */
		//Parptr->multi_soilPercoPD[lyr][index] = 0.5 * (Parptr->multi_soilPercoPD[lyr][index] + Drainage) * row_cell_area;  这里注释掉是因为都不按照体积计算，统一按照深度计算 
		// xiaodw, 对河道只有两侧洪泛区发生渗漏 
		Parptr->multi_soilPercoPD[lyr][index] = 0.5 * (Parptr->multi_soilPercoPD[lyr][index] + Drainage) * Parptr->multi_adjustPD[lyr][index];

		// Parptr->multi_adjustPD[lyr][index]是为了调整河道所在栅格的土壤层的体积
		MaxSoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilPorosityPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
		SoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
		FieldCapacity = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilFcPD[lyr][index] * Parptr->multi_adjustPD[lyr][index];
		//MaxSoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilPorosityPD[lyr][index];
		//SoilWater = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilMoisturePD[lyr][index];
		//FieldCapacity = Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_soilFcPD[lyr][index];

		/* No unsaturated flow if the moisture content drops below field capacity */

		if ((SoilWater - Parptr->multi_soilPercoPD[lyr][index] - FieldCapacity) <= 1e-8) {
			Parptr->multi_soilPercoPD[lyr][index] = SoilWater - FieldCapacity;
		}

		// 如果按照正常的扣除渗漏后，还是过饱和，则将过饱和的水都加入渗漏量
		SoilWater -= Parptr->multi_soilPercoPD[lyr][index];
		if (SoilWater - MaxSoilWater >= 1e-8) {
			Parptr->multi_soilPercoPD[lyr][index] += SoilWater - MaxSoilWater;
		}
		
	}

	inline void UnsaturatedFlowGwVersionV2(const int row_start, int row_end,
		Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, States *Statesptr, int grid_row_index, int row, const int grid_rows, const int grid_cols,
		const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE row_cell_area, const int grid_cols_padded, NUMERIC_TYPE * volume_grid,
		int NSoilLayers, int NRootLayers, Pois*Poisptr)
	{
		float DeepDrainage;		/* amount of drainage from the lowest root zone to the layer below it (m) */
		float DeepLayerDepth;		/* depth of the layer below the deepest root layer */
		float Drainage;		    /* amount of water drained from each soil layer during the current timestep (m) */
		float Exponent;		    /* Brooks-Corey exponent */
		float FieldCapacity;		/* amount of water in soil at field capacity (m) */
		float MaxSoilWater;		/* maximum allowable amount of soil moiture in each layer (m) */
		float SoilWater;		    /* amount of water in each soil layer (m) */
		

		for (int i = row_start; i < row_end; i++)
		{
			if (dem_row[i] != DEM_NO_DATA) {
				int index = i + grid_row_index;
				//DeepLayerDepth = Parptr->soilThicknessAllLyrsPD[index];
				//for (int lyr = 0; lyr < NRootLayers; lyr++)
				//	DeepLayerDepth -= Parptr->multi_soilThicknessPD[lyr][index];

				DeepLayerDepth = Parptr->multi_soilThicknessPD[NRootLayers][index];
				// 先计算最下层向地下水库渗漏
				if (Parptr->GwStorageDepth < Parptr->GwStorageDepthMax)
				{
					CalSoilPerc(Parptr, index, NRootLayers);
					// Parptr->multi_soilPercoPD[NRootLayers][index]   m->mm
					if (Parptr->GwStorageDepth + Parptr->multi_soilPercoPD[NRootLayers][index] * 1000.0 > Parptr->GwStorageDepthMax)
					{
						Parptr->multi_soilPercoPD[NRootLayers][index] = (Parptr->GwStorageDepthMax - Parptr->GwStorageDepth) * 0.001;
					}
					Parptr->multi_soilMoisturePD[NRootLayers][index] -= Parptr->multi_soilPercoPD[NRootLayers][index] / (Parptr->multi_soilThicknessPD[NRootLayers][index] * Parptr->multi_adjustPD[NRootLayers][index]);
				}
				else {
					Parptr->multi_soilPercoPD[NRootLayers][index] = 0.0;
				}
				//CalSoilPerc(Parptr, index, NRootLayers);
				//Parptr->multi_soilMoisturePD[NRootLayers][index] -= Parptr->multi_soilPercoPD[NRootLayers][index] / (Parptr->multi_soilThicknessPD[NRootLayers][index] * Parptr->multi_adjustPD[NRootLayers][index]);
				// xiaodw, 这里思路是先计算渗漏量，即使下层超饱和了，上层也向下层渗漏。在WaterTableDepth方法中再从下层往上层计算淹没，纠正过来
				/* from top to bottom soil layer */

				for (int lyr = 0; lyr < NRootLayers; lyr++) {

					/* No movement if soil moisture is below field capacity */
					if (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index] >= 1e-8) {

						CalSoilPerc(Parptr, index, lyr);

						/* Adjust the moisture content in the current layer, and the layer immediately below it */
						Parptr->multi_soilMoisturePD[lyr][index] -= Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr][index] * Parptr->multi_adjustPD[lyr][index]);

						if (lyr < (NRootLayers - 1)) {

							Parptr->multi_soilMoisturePD[lyr + 1][index] += Parptr->multi_soilPercoPD[lyr][index] / (Parptr->multi_soilThicknessPD[lyr + 1][index] * Parptr->multi_adjustPD[lyr + 1][index]);

						}
						if (Statesptr->use_change_acccum_depth && lyr == 0)
						{
							Parptr->accumuDepthPD[index] -= Parptr->multi_soilPercoPD[lyr][index] * 1000.0;
							if (Parptr->accumuDepthPD[index] < 1e-8)
							{
								Parptr->accumuDepthPD[index] = 0.0;
							}
							if (Parptr->accumuDepthPD[index] > Parptr->multi_soilThicknessPD[lyr][index] * 1000.0)
							{
								Parptr->accumuDepthPD[index] = Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
							}
						}
					}
					else {
						Parptr->multi_soilPercoPD[lyr][index] = 0.0;
					}
					Parptr->multi_soilWaterDepthPD[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
					if (Statesptr->save_poi == ON)
					{
						Poisptr->soil_perc_Grid[lyr][index] += Parptr->multi_soilPercoPD[lyr][index] * 1000.0;  // m/gwstep->mm /gwstep
						Poisptr->soil_water_depth_Grid[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;  // mm

					}

				}

				// 之前是直接从最下层扣除侧向壤中流
				/* Change: dont extract outflow from the bottom layer only. The new function DistributeSatFlow
				extract water from top to bottom layer to avoid negative soil mositure (overdraw) in the bottom
				layer (below root zone layers */
				DeepDrainage = (Parptr->multi_soilPercoPD[NRootLayers - 1][index]);

				Parptr->multi_soilMoisturePD[NRootLayers][index] += DeepDrainage / (DeepLayerDepth * Parptr->multi_adjustPD[NRootLayers][index]);


				if (Statesptr->save_poi == ON)
				{
					Poisptr->soil_water_depth_Grid[NRootLayers][index] = Parptr->multi_soilMoisturePD[NRootLayers][index] * Parptr->multi_soilThicknessPD[NRootLayers][index] * 1000.0;  // cm
				}

				/* Calculate the depth of the water table based on the soil moisture
				profile and adjust the soil moisture profile, to assure that the soil
				moisture is never more than the maximum allowed soil moisture amount,
				i.e. the porosity.  A negative water table depth means that the water is
				ponding on the surface.  This amount of water becomes surface Runoff */

				Parptr->tableDepthPD[index] = WaterTableDepth(Parptr, Solverptr, Arrptr, Statesptr, Poisptr, row_cell_area, NSoilLayers, NRootLayers, index);


				// 地下水位高出地表
				if (Parptr->tableDepthPD[index] <= -1e-8) {

					Parptr->tableDepthPD[index] = 0.0;

				}

			}
		}
	}

	 inline NUMERIC_TYPE SGC2_interflow_multilayer(
		 const int row_start, int row_end,
		 const NUMERIC_TYPE depth_thresh,
		 Pars *Parptr, const Solver *Solverptr, int grid_row_index,
		 const NUMERIC_TYPE * dem_row,
		 NUMERIC_TYPE *interflow_runoff_vol,
		 NUMERIC_TYPE *interflow_2ch_vol,
		 const NUMERIC_TYPE row_cell_area

		 //const NUMERIC_TYPE row_cell_area
		 //NUMERIC_TYPE* interflow_Row_POI,
	 )
	 {
		 float k = 0.f;   // mm/h
		 float ks = 0.f;
		 float maxSoilWaterVol = 0.f;
		 float soilWaterVol = 0.f;
		 float fieldCapacityVol = 0.f;
		 float interflowMiosture = 0.f;
		 float runoffVolCurStep = 0.f;
		 NUMERIC_TYPE interflow_genVol = C(0.0);
		 if (row_end - row_start > 0) {
			 for (int row_i = row_start; row_i < row_end; row_i++)
			 {
				 // 问题1：SEIMS里的单元有明确的上下游关系，这里要使用流向tif直接作为上下游关系的依据，还是用土壤水位差作为上下游的依据？
				 // 问题2：green-ampt假设有一个明确的湿润锋面，适合干旱区的入渗；有人将其改造为适合湿润区的，但
				 // 我们用的casc2d里的greenampt是否适合湿润区的模拟？
				 // 问题3：为什么要除以流长（单元上的河道长度），对我而言流长是否是一个栅格单元的宽度？
				 if (dem_row[row_i] != DEM_NO_DATA) {
					 int index = row_i + grid_row_index;
					 for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++) {
						 if (Parptr->multi_soilMoisturePD[lyr][index] < Parptr->multi_soilFcPD[lyr][index])
						 {
							 continue;
						 }
						 if (Parptr->multi_ksFactorVOfLyr[lyr] > 0.0)
						 {
							 ks = Parptr->multi_ksFactorVOfLyr[lyr] *  Parptr->multi_soilKsPD[lyr][index];
						 }
						 else
						 {
							 ks = Parptr->multi_soilKsPD[lyr][index];
						 }
						 maxSoilWaterVol = Parptr->multi_soilPorosityPD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index]   * row_cell_area;  // m3
						 if (Parptr->multi_soilMoisturePD[lyr][index] > Parptr->multi_soilPorosityPD[lyr][index]) {
							 k = ks;
						 }
						 else {
							 /// Using Clapp and Hornberger (1978) equation to calculate unsaturated hydraulic conductivity.
							 float dcIndex = 2.f * Parptr->multi_soilPoreIndexPD[lyr][index] + 2.f; // pore disconnectedness index
							 k = ks * pow(Parptr->multi_soilMoisturePD[lyr][index] / Parptr->multi_soilPorosityPD[lyr][index], dcIndex);
							 //if (k <= 0.000001) k = 0.f;
						 }
						 // 1. / 3600. = 0.0002777777777777778
						 // 当前土壤水分的当量水量
						 soilWaterVol = Parptr->multi_soilThicknessPD[lyr][index]  * Parptr->multi_soilMoisturePD[lyr][index] * row_cell_area;  // m3
						 // 田间持水量的当量水量
						 fieldCapacityVol = Parptr->multi_soilThicknessPD[lyr][index]  * Parptr->multi_soilFcPD[lyr][index] * row_cell_area;
						 // interflowGenVolPD m3,k from mm/h -> m/s
						 if (Parptr->slopePD[index] <= 0.0)
						 {
							 Parptr->slopePD[index] = 0.001;
						 }
						 // 加一个lag系数，汇流（SWAT文档，问娇娇）
						 Parptr->multi_interflowGenVolPD[lyr][index] = Parptr->multi_interflowCsValueOfLyr[lyr] * Parptr->multi_soilThicknessPD[lyr][index]  * Parptr->slopePD[index]
							 * k * 0.0002777777777777778 * 0.001 * Parptr->multi_soilMoisturePD[lyr][index] * sqrt(row_cell_area)  * 	Solverptr->SGCtmpTstep;    // m3

						 //Parptr->multi_interflowGenVolPD[lyr][index] = Parptr->multi_interflowCsValueOfLyr[lyr] * Parptr->multi_soilThicknessPD[lyr][index] * 0.01f * Parptr->slopePD[index]
							// * k * 0.0002777777777777778 * 0.001 * Parptr->multi_soilMoisturePD[lyr][index] * sqrt(row_cell_area)  * 	Solverptr->SGCtmpTstep;    // m3

						 // the unit is mm
						 // 如果地下水储量 - 地下水径流量后，依然超出土壤孔隙度（土壤最大储水量），则地下水径流量=土壤水储量-最大储水量，原有逻辑感觉适合日尺度，不适合秒尺度
						 // 改为即便超饱和，地下水径流仍然以ks为速率流失, 避免出现河道流量突变
						 //if (soilWaterDep - interflowDep > maxSoilWaterDep) {
						 //	Parptr->multi_interflowGenVolPD[lyr][index] = Parptr->multi_soilMoisturePD[lyr][index] - maxSoilWaterDep;
						 //}
						 if (soilWaterVol - Parptr->multi_interflowGenVolPD[lyr][index] > maxSoilWaterVol) {
							 Parptr->multi_interflowGenVolPD[lyr][index] = soilWaterVol - maxSoilWaterVol;
						 }
						 else if (soilWaterVol - Parptr->multi_interflowGenVolPD[lyr][index] < fieldCapacityVol) {
							 // 如果 减去后，小于田间持水量，则壤中流=土壤水储量-田间持水量，xiaodw
							 Parptr->multi_interflowGenVolPD[lyr][index] = soilWaterVol - fieldCapacityVol;
						 }
						 Parptr->multi_interflowGenVolPD[lyr][index] = Max(0.f, Parptr->multi_interflowGenVolPD[lyr][index]);
						 interflowMiosture = Parptr->multi_interflowGenVolPD[lyr][index] / (row_cell_area * Parptr->multi_soilThicknessPD[lyr][index] );  // m3/m3

						 // 土壤水储量 - 壤中流径流量
						 Parptr->multi_soilMoisturePD[lyr][index] -= interflowMiosture;
						 //*interflowAvgBlock = *interflowAvgBlock + Parptr->multi_interflowGenVolPD[lyr][index];
						 interflow_genVol += Parptr->multi_interflowGenVolPD[lyr][index];

						 // 根据滞后系数计算实际汇流的量=(当前步长产流+之前的积累量)*滞后系数
						 if (Parptr->interflow_lagindex > 0.0)
						 {
							 Parptr->multi_interflow2ChVolPD[lyr][index] = (Parptr->multi_interflowGenVolPD[lyr][index] + Parptr->multi_interflowRunoffVolPD[lyr][index]) * Parptr->interflow_lagindex;
						 }
						 else
						 {
							 Parptr->multi_interflow2ChVolPD[lyr][index] = (Parptr->multi_interflowGenVolPD[lyr][index] + Parptr->multi_interflowRunoffVolPD[lyr][index]) *
								 (1 - exp(-Parptr->interflow_surlag / (Parptr->interflow_t_conc)));
							 //Parptr->multi_interflow2ChVolPD[lyr][index] = (Parptr->multi_interflowGenVolPD[lyr][index] + Parptr->multi_interflowRunoffVolPD[lyr][index]) * (1 - exp(-Parptr->interflow_surlag / (Parptr->interflow_t_conc / (Solverptr->SGCtmpTstep * 0.00027777f))));
						 }

						 //cout << index << "  " << Parptr->multi_interflow2ChVolPD[lyr][index] << "   " << Parptr->multi_interflowGenVolPD[lyr][index] << "   " << Parptr->multi_interflowRunoffVolPD[lyr][index] << endl;
						 // 更新壤中流形成的地表径流的库存量
						 Parptr->multi_interflowRunoffVolPD[lyr][index] = Parptr->multi_interflowRunoffVolPD[lyr][index] + Parptr->multi_interflowGenVolPD[lyr][index] - Parptr->multi_interflow2ChVolPD[lyr][index];
						 *interflow_2ch_vol = *interflow_2ch_vol + Parptr->multi_interflow2ChVolPD[lyr][index];
						 *interflow_runoff_vol = *interflow_runoff_vol + Parptr->multi_interflowRunoffVolPD[lyr][index];
					 }
				 }
			 }
		 }
		 return interflow_genVol;
	 }
	//-----------------------------------------------------------------------------
	// FLOODPLAIN EVAPORATION
	// with correction for sub grid channels
	inline NUMERIC_TYPE SGC2_Evaporation_floodplain_row(
		const States *Statesptr, const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		const NUMERIC_TYPE row_cell_area,
		const NUMERIC_TYPE evap_deltaH_step,
		const NUMERIC_TYPE * evap_row,
		const NUMERIC_TYPE * h_row,
		NUMERIC_TYPE * volume_row, NUMERIC_TYPE * soilWaterDepth, NUMERIC_TYPE * soilMoisture, NUMERIC_TYPE * soilThickness, const int grid_row_index, NUMERIC_TYPE *Evap_Row_POI, NUMERIC_TYPE *soil_water_depth_row_POI)
	{
	#ifdef __INTEL_COMPILER
		__assume_aligned(h_row, 64);
		__assume_aligned(volume_row, 64);
	#endif

		NUMERIC_TYPE evap_loss = C(0.0);
	#pragma ivdep
	#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			NUMERIC_TYPE h_new, dV = C(0.0);
			NUMERIC_TYPE soil_water_depth_old, soil_water_depth_new, soil_moisture_new = C(0.0);
			NUMERIC_TYPE h_old = h_row[i];
			NUMERIC_TYPE evap_deltaV_step = evap_row[i] * row_cell_area;
			int index = grid_row_index + i;
			if (h_old > depth_thresh) // There is water to evaporate on the flood plain
			{
				// update depth by subtracting evap depth
				h_new = h_old - evap_row[i];
				//check for -ve depths
				if (h_new < C(0.0))
				{
					// reduce evap loss to account for dry bed (don't go below 0)
					dV = h_old * row_cell_area;
				}
				else
				{
					dV = evap_deltaV_step;
				}
				volume_row[i] -= dV;
			}
			else if (soilMoisture[i] > 0.0) {
				// 土壤水蒸发
				soilWaterDepth[i] = soilMoisture[i] * soilThickness[i];
				soil_water_depth_new = soilWaterDepth[i] - evap_row[i];
				
				if (soil_water_depth_new < 0.0)
				{
					dV = soilWaterDepth[i] * row_cell_area;
				}
				else
				{
					dV = evap_deltaV_step;
				}
				soilWaterDepth[i] -= dV / row_cell_area;
				soilMoisture[i] -= dV / row_cell_area / soilThickness[i];
				if (soilMoisture[i] < 0.0)
				{
					soilWaterDepth[i] = 0.0;
					soilMoisture[i] = 0.0;
				}
				//volume_row[i] -= dV;
			}
			else
			{
				// 无蒸发
				dV = 0.0;
			}
			evap_loss += dV; //mass-balance for a standard cell
			if (Statesptr->save_poi)
			{
				Evap_Row_POI[i] += dV * 1000.0 / row_cell_area;
				soil_water_depth_row_POI[i] = soilMoisture[i] * soilThickness[i] * 1000.0;
			}
			 
		}
		return evap_loss;
	}

	inline NUMERIC_TYPE Expo(NUMERIC_TYPE xx, NUMERIC_TYPE upper /* = 20.f */, NUMERIC_TYPE lower /* = -20.f */) {
		if (xx < lower) xx = lower;
		if (xx > upper) xx = upper;
		return exp(xx);
	}

	/* AET Priestley Talor Hargreaves */
	inline NUMERIC_TYPE SGC2_Evaporation_floodplain_row_PT(
		const States *Statesptr, const int row_start, int row_end,
		const NUMERIC_TYPE depth_thresh,
		const NUMERIC_TYPE row_cell_area,
		const NUMERIC_TYPE evap_deltaH_step,
		const NUMERIC_TYPE * evap_row,
		const NUMERIC_TYPE * h_row,
		NUMERIC_TYPE * volume_row, NUMERIC_TYPE * soilWaterDepth, NUMERIC_TYPE * soilMoisture, NUMERIC_TYPE * soilThickness, 
		const int grid_row_index, NUMERIC_TYPE *Evap_Row_POI, NUMERIC_TYPE *soil_water_depth_row_POI, Pars *Parptr)
	{
#ifdef __INTEL_COMPILER
		__assume_aligned(h_row, 64);
		__assume_aligned(volume_row, 64);
#endif

		NUMERIC_TYPE evap_loss = C(0.0);
#pragma ivdep
#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			NUMERIC_TYPE h_new, dV = C(0.0);
			NUMERIC_TYPE soil_water_depth_old, soil_water_depth_new, soil_moisture_new = C(0.0);
			NUMERIC_TYPE h_old = h_row[i];
			NUMERIC_TYPE evap_deltaV_step = evap_row[i] * row_cell_area;
			int index = grid_row_index + i;
			NUMERIC_TYPE evz = 0.0;
			NUMERIC_TYPE evzp = 0.0;  // 上一层的潜在蒸发
			NUMERIC_TYPE aet_lyr = 0.0;
			NUMERIC_TYPE xx = 0.0;		 // 干旱性指数
			NUMERIC_TYPE evap_deltaH_stepMM = evap_deltaH_step * 1000.0;
			NUMERIC_TYPE esleft = evap_deltaH_stepMM;
			NUMERIC_TYPE soilThicknessMM, soilDepthsMM = 0.0;
			NUMERIC_TYPE dep = 0.0;
			
			// 只对指定层计算蒸发
			for (int lyr = 0; lyr < Parptr->multi_nSoilEvapLyrs; lyr++)
			{

				soilDepthsMM = Parptr->multi_soilDepthPD[lyr][index] * 1000.0;
				soilThicknessMM = Parptr->multi_soilThicknessPD[lyr][index] * 1000.0;
				evz = evap_deltaH_stepMM * soilDepthsMM /
					(soilDepthsMM + exp(2.374f - 0.00713f * soilDepthsMM));
				aet_lyr = evz - evzp * Parptr->esco;

				evzp = evz;
				// 根据土壤水分调整
				if (Parptr->multi_soilMoisturePD[lyr][index] < Parptr->multi_soilFcPD[lyr][index]) {
					xx = 2.5f * (Parptr->multi_soilMoisturePD[lyr][index] - Parptr->multi_soilFcPD[lyr][index]) / Parptr->multi_soilFcPD[lyr][index]; ///  non dimension  
					aet_lyr *= Expo(xx, 20.0, -20.0);  /// 限制指数输入值的上下限
				}
				aet_lyr = min(aet_lyr, Parptr->multi_soilMoisturePD[lyr][index] * soilThicknessMM * Parptr->etco);
				if (aet_lyr < 0.f || aet_lyr != aet_lyr) aet_lyr = 0.0;
				if (aet_lyr > esleft) aet_lyr = esleft;
				/// adjust soil storage, potential evap
				if (Parptr->multi_soilMoisturePD[lyr][index] * soilThicknessMM > aet_lyr) {
					esleft -= aet_lyr;
					Parptr->multi_soilMoisturePD[lyr][index] = max(UTIL_ZERO, (Parptr->multi_soilMoisturePD[lyr][index] * soilThicknessMM - aet_lyr) / soilThicknessMM);
				}
				else {
					esleft -= soilThicknessMM;
					Parptr->multi_soilMoisturePD[lyr][index] = 0.f;
				}
			}

			if (Statesptr->save_poi)
			{
				Evap_Row_POI[i] += dV * 1000.0 / row_cell_area;
				soil_water_depth_row_POI[i] = soilMoisture[i] * soilThickness[i] * 1000.0;
			}

		}
		return evap_loss;
	}

	inline void Xaj_3Layers_UpdateSoil(int index, Pars *Parptr, const States *Statesptr, NUMERIC_TYPE *EvapGrid_POI, NUMERIC_TYPE ** soil_water_depth_POI, NUMERIC_TYPE ES,
		NUMERIC_TYPE wu0, NUMERIC_TYPE EU, NUMERIC_TYPE wl0, NUMERIC_TYPE EL, NUMERIC_TYPE wd0, NUMERIC_TYPE ED) {
		// ---------- 更新各层土壤水状态 ----------
		Parptr->multi_soilMoisturePD[0][index] = getmax(0.0, (wu0 - EU) * 0.001 / Parptr->multi_soilThicknessPD[0][index]);  // wu0 - EU（mm)->%
		Parptr->multi_soilMoisturePD[1][index] = getmax(0.0, (wl0 - EL) * 0.001 / Parptr->multi_soilThicknessPD[1][index]);
		Parptr->multi_soilMoisturePD[2][index] = getmax(0.0, (wd0 - ED) * 0.001 / Parptr->multi_soilThicknessPD[2][index]);
		// ---------- 保存蒸发输出 ----------
		Parptr->es[index] = ES;    // mm
		Parptr->eu[index] = EU;    // mm
		Parptr->el[index] = EL;
		Parptr->ed[index] = ED;
		// ---------- 保存观测点蒸发 ----------
		if (Statesptr->save_poi) {
			EvapGrid_POI[index] += ES + EU + EL + ED;

			soil_water_depth_POI[0][index] = Parptr->multi_soilMoisturePD[0][index] * Parptr->multi_soilThicknessPD[0][index] * 1000.0;  // mm
			soil_water_depth_POI[1][index] = Parptr->multi_soilMoisturePD[1][index] * Parptr->multi_soilThicknessPD[1][index] * 1000.0;  // mm
			soil_water_depth_POI[2][index] = Parptr->multi_soilMoisturePD[2][index] * Parptr->multi_soilThicknessPD[2][index] * 1000.0;  // mm
		}
	}
	inline NUMERIC_TYPE Xaj_3Layers_Evap(NUMERIC_TYPE SURF,NUMERIC_TYPE evap_remain, NUMERIC_TYPE * ES, NUMERIC_TYPE * EU, NUMERIC_TYPE * EL, NUMERIC_TYPE * ED,
		const NUMERIC_TYPE wu0,  const NUMERIC_TYPE wl0, const NUMERIC_TYPE lm, NUMERIC_TYPE fp_area,Pars *Parptr) {

		NUMERIC_TYPE dV = 0.0;
		if (SURF >= evap_remain) {
			//surf[i] = SURF - evap_remain;
			*EU = 0.0; 
			*EL = 0.0;  
			*ED = 0.0;
			*ES = evap_remain;
			dV = evap_remain * fp_area * 0.001;
			evap_remain = 0.0;

			//volume_row[i] -= dV;    // m3
		}
		else {

			// ---------- Step 0: 地表蒸发 ----------
			if (SURF > 0.0)
			{
				*ES = SURF;
				evap_remain -= SURF;
				dV = SURF * fp_area  * 0.001;
				//volume_row[i] -= dV;   // m3
			}

			// ---------- Step 1: 上层蒸发 ----------
			*EU = min(wu0, evap_remain);    // 原文是EU = K * EM, 这里直接使用PET而不是pan evaporation(EM)，因此水量充足时EU直接等于PET，不需要乘以k. 
			evap_remain -= *EU;

			// ---------- Step 2: 中层蒸发 ----------
			if (evap_remain <= UTIL_ZERO) {
				*EL = 0.0;
			}
			else {
				if (wl0 >= Parptr->c * lm) {  // 中层水量 大于 中层最大允许参与蒸发的量，中层水量充足
					*EL = evap_remain * wl0 / lm;  // 剩余蒸发量 * 中层水量 / 中层Fc. 同理，这里原文是EL = (K*EM-EU)* WL/LM，也不需要乘以K，evap_remain就是K×EM-EU
				}
				else if (wl0 >= Parptr->c * evap_remain) { //  中层水量 大于 中层应提供蒸发的量
					*EL = Parptr->c * evap_remain;
				}
				else {
					*EL = wl0;
				}
				evap_remain -= *EL;
			}

			// ---------- Step 3: 深层蒸发 ----------
			if (evap_remain > UTIL_ZERO && wl0 < Parptr->c * lm && wl0 < Parptr->c * (evap_remain + *EL)) { // 中层水量 < c*中层Fc
				*ED = Parptr->c * (evap_remain + *EL) - wl0;   // evap_remain + EL是中层蒸发之前的量，等于K* EM-EU，这里是中层蒸发之前的量-中层水量，全部交给深层蒸发
			}
			else {
				*ED = 0.0;
			}
		}

		return dV;
	}

	inline NUMERIC_TYPE SGC2_Evaporation_XAJ_3Layer(
		const States *Statesptr, const int row_start, int row_end,
		const NUMERIC_TYPE row_cell_area,
		const NUMERIC_TYPE * evap_grid, const NUMERIC_TYPE * h_grid, NUMERIC_TYPE * volume_row, NUMERIC_TYPE * volume_row_ch, const NUMERIC_TYPE * dem_grid,
		const int grid_row_index, NUMERIC_TYPE *EvapGrid_POI, NUMERIC_TYPE ** soil_water_depth_POI, Pars *Parptr, Arrays * Arrptr, const NUMERIC_TYPE * sg_cell_cell_area, int j,
		const int cell_count, const int sg_row_start, const int * sg_cell_grid_index_lookup,
		const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume, const NUMERIC_TYPE * sg_cell_SGC_c,
		const SubGridFlowLookup * sg_cell_flow_lookup) {

		NUMERIC_TYPE total_evap = C(0.0);

#pragma ivdep
#pragma simd
		for (int i = row_start; i < row_end; i++)
		{
			int index = grid_row_index + i;
			int source_index_this = j * Parptr->xsz + i;
			NUMERIC_TYPE PET = evap_grid[index] * 1000.0;          // 潜在蒸散发,mm
			NUMERIC_TYPE ES = 0.0, EU = 0.0, EL = 0.0, ED = 0.0;  // 地表, 上层，中层，下层蒸发

			if (dem_grid[index] != DEM_NO_DATA) {
				// 河道先跳过
				if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0))
				{
					continue;
				}
				// 蓄洪区
				NUMERIC_TYPE SURF = (h_grid[index] + volume_row[i] / row_cell_area) * 1000.0;        // 地表水 + 降雨
				NUMERIC_TYPE wu0 = Parptr->multi_soilMoisturePD[0][index] * Parptr->multi_soilThicknessPD[0][index] * 1000.0;           // 上层土壤水,mm
				NUMERIC_TYPE wl0 = Parptr->multi_soilMoisturePD[1][index] * Parptr->multi_soilThicknessPD[1][index] * 1000.0;            // 中层土壤水,mm
				NUMERIC_TYPE wd0 = Parptr->multi_soilMoisturePD[2][index] * Parptr->multi_soilThicknessPD[2][index] * 1000.0;            // 深层土壤水,mm
				NUMERIC_TYPE lm = Parptr->multi_soilFcPD[1][index] * Parptr->multi_soilThicknessPD[1][index] * 1000.0;               // 中层允许参与蒸发的最大水分，即fc,mm


				NUMERIC_TYPE evap_remain = PET;
				NUMERIC_TYPE h_new, dV = C(0.0);
				// 计算蒸发的地表水体积、地表水深度、上层深度、中层深度、底层深度
				dV = Xaj_3Layers_Evap(SURF, evap_remain, &ES, &EU, &EL, &ED, wu0, wl0, lm, row_cell_area, Parptr);
				// 更新土壤湿度
				Xaj_3Layers_UpdateSoil(index, Parptr, Statesptr, EvapGrid_POI, soil_water_depth_POI, ES, wu0, EU, wl0, EL, wd0, ED);
				volume_row[i] -= dV;
				total_evap += (ES + EU + EL + ED);
			}
		}

		for (int cell_i = 0; cell_i < cell_count; cell_i++)
		{
			int cell_index = sg_row_start + cell_i;

			int grid_index = sg_cell_grid_index_lookup[cell_index];
			int i = grid_index - grid_row_index;
			NUMERIC_TYPE PET = evap_grid[grid_index] * 1000.0;          // 潜在蒸散发,mm
			NUMERIC_TYPE ES = 0.0, EU = 0.0, EL = 0.0, ED = 0.0;  // 河道水, 河道底部土壤，地表(ES在这里特指洪泛区的地表), 上层，中层，下层蒸发
			NUMERIC_TYPE ES_CH = 0.0, EU_CH;  // 河道水, 河道底部土壤
			NUMERIC_TYPE dV = 0.0, dVFp = 0.0, dVCh = 0.0;
			// 上个时间步长结束时的水深
			const NUMERIC_TYPE h_prev = h_grid[grid_index];
			// // 河道所在的栅格单元面积
			const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
			// SGC河道单元底面积
			NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];
			// 河道两侧蓄洪区面积
			NUMERIC_TYPE fp_area = row_cell_area - SGC_c;
			// 河道深度
			NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index] * 1000.0;
			//if (h_grid == nullptr || h_grid == NULL) {
			//	std::cerr << "[ERROR] h_grid allocation failed!" << std::endl;
			//}
			NUMERIC_TYPE h_old_fp = h_grid[grid_index] > 0.0 ? (h_grid[grid_index] + volume_row[i] / fp_area) * 1000.0 : volume_row[i] / fp_area * 1000.0;  // m->mm
			NUMERIC_TYPE h_old_ch = (h_grid[grid_index] + volume_row_ch[i] / SGC_c) * 1000.0 + SGC_BankFullHeight;  // m->mm
			NUMERIC_TYPE h_new_ch = h_old_ch - PET;                // mm

			NUMERIC_TYPE wu0 = Parptr->multi_soilMoisturePD[0][grid_index] * Parptr->multi_soilThicknessPD[0][grid_index] * 1000.0;           // 上层土壤水,mm
			NUMERIC_TYPE wl0 = Parptr->multi_soilMoisturePD[1][grid_index] * Parptr->multi_soilThicknessPD[1][grid_index] * 1000.0;            // 中层土壤水,mm
			NUMERIC_TYPE wd0 = Parptr->multi_soilMoisturePD[2][grid_index] * Parptr->multi_soilThicknessPD[2][grid_index] * 1000.0;            // 深层土壤水,mm
			NUMERIC_TYPE lm = Parptr->multi_soilFcPD[1][grid_index] * Parptr->multi_soilThicknessPD[1][grid_index] * 1000.0;               // 中层允许参与蒸发的最大水分，即fc,mm

			// 蓄洪区的蒸发计算和普通格子方法一样，唯一区别是使用蓄洪区的面积
			// 计算蒸发的地表水体积、地表水深度、上层深度、中层深度、底层深度
			NUMERIC_TYPE evap_remain = PET;
			dVFp = Xaj_3Layers_Evap(h_old_fp, evap_remain, &ES, &EU, &EL, &ED, wu0, wl0, lm, fp_area, Parptr);
			// 更新蓄洪区土壤湿度
			Xaj_3Layers_UpdateSoil(grid_index, Parptr, Statesptr, EvapGrid_POI, soil_water_depth_POI, ES, wu0, EU, wl0, EL, wd0, ED);

			// 河道内部单独计算
			// 河道里原本有水
			if (h_old_ch > 0.0)
			{
				// 按PET蒸发之后还是有水
				if (h_new_ch > 0.0)
				{
					ES_CH = PET;
					EU_CH = 0.0;
					dVCh = ES_CH * SGC_c * 0.001;   // m3
				}
				else
				{
					// 先把河道的水扣完
					ES_CH = h_old_ch;
					dVCh = ES_CH * SGC_c * 0.001;
					NUMERIC_TYPE evap_remain = PET - ES_CH;
					// 如果土壤厚度 > 河堤深度, 河道底部土壤以中层蒸发速率(AET=PET*土壤湿度/田间持水量)继续蒸发
					NUMERIC_TYPE ch_soilDepth = (Parptr->soilThicknessAllLyrsPD[grid_index] - SGC_BankFullHeight);
					EU_CH = 0.0;
					// 河道土壤对应其栅格的第几层
					int bedLyr = Parptr->sgcBedSoilLyrPD[grid_index];
					// 当河道底部有土壤，且有剩余蒸发量，且底部土壤湿度大于0时，更新河道底部土壤层的湿度
					if (ch_soilDepth > UTIL_ZERO && evap_remain >= UTIL_ZERO && Parptr->multi_soilMoisturePD[bedLyr][grid_index] > 0.0)
					{
						EU_CH = evap_remain * Parptr->multi_soilMoisturePD[bedLyr][grid_index] / Parptr->multi_soilFcPD[bedLyr][grid_index];  // 剩余蒸发量 * 中层水量 / 中层Fc. 
					}
					EU_CH = min(Parptr->multi_soilMoisturePD[bedLyr][grid_index] * Parptr->multi_soilThicknessPD[bedLyr][grid_index], EU_CH);
					Parptr->multi_soilMoisturePD[bedLyr][grid_index] -= EU_CH * SGC_c / (Parptr->multi_soilThicknessPD[bedLyr][grid_index] * row_cell_area);
				}

				// 更新河道和蓄洪区水量
				//volume_row[grid_index- grid_row_index] -= dV;   // m3
				volume_row[i] -= dVFp;
				volume_row_ch[i] -= dVCh;
				total_evap += (ES + EU + EL + ED);

			}

			return total_evap;
		}
	}





	inline NUMERIC_TYPE Desorption(int Dt, float MoistContent, float Porosity, float Ks,
		float Press, float m)
	{
		float Sorptivity;		/* sorptivity */
		float DesorptionVolume;	/* total desorption volume during timestep */

		/* Eq. 46, Wigmosta et al [1994] */

		if (MoistContent > Porosity)
			MoistContent = Porosity;

		/*   Sorptivity =  */
		/*     pow((double) ((8 * Porosity * Ks * Press)/(3.0*(1 + 3*m) * (1 + 4*m))), */
		/* 	(double) 0.5) *  */
		/* 	  pow((double) (MoistContent/Porosity), (double) (1.0/(2.0 * m) + 2)); */

		Sorptivity = sqrt((double)
			((8 * Porosity * Ks * Press) /
			(3.0 * (1 + 3 * m) * (1 + 4 * m)))) *
			pow((double)(MoistContent / Porosity), (double)(1.0 / (2.0 * m) + 2));

		/* Eq. 45, Wigmosta et al [1994] */

	  /*  DesorptionVolume = Sorptivity * pow((double) Dt * SECPHOUR, (double) 0.5); */
		DesorptionVolume = Sorptivity * sqrt((double)Dt);

		return DesorptionVolume;
	}

	inline NUMERIC_TYPE SoilEvaporation_DHSVM(int Dt, float Temp, float Slope, float Gamma, float Lv,
		float AirDens, float Vpd, float NetRad, float RaSoil,
		float Transpiration, float Porosity, float FCap, float Ks,
		float Press, float m, float RootDepth,
		float *MoistContent, float Adjust)
	{
		float DesorptionVolume;	/* Amount of water the soil can deliver to the
									 atmosphere during a timestep (mm) */
		float EPot;			/* Potential evaporation from soil during timestep (mm) */
		float SoilEvap;		/* Amount of evaporation directly from the soil (mm) */
		float SoilMoisture;   /* Amount of water in surface soil layer (mm) */
		float MoistThrhld;    /* threshold that limits evap to maintain soil at a moisture level */
		float tmp;

		DesorptionVolume = Desorption(Dt, *MoistContent, Porosity, Ks, Press, m);

		/* Eq.4 Wigmosta et al [1994] */

		/* Calculate the density of pure water as a function of temperature.
		   Thiesen, Scheel-Diesselhorst Equation (in Handbook of hydrology, fig
		   11.1.1) */



		/* The potential evaporation rate accounts for the amount of moisture that
		   the atmosphere can absorb.  If we do not account for the amount of
		   evaporation from overlying evaporation, we can end up with the situation
		   that all vegetation layers and the soil layer transpire/evaporate at the
		   potential rate, resulting in an overprediction of the actual evaporation
		   rate.  Thus we subtract the amount of evaporation that has already
		   been calculated for overlying layers from the potential evaporation.
		   Another mechanism that could be used to account for this would be to
		   decrease the vapor pressure deficit while going down through the canopy
		   (not implemented here) */

		EPot = 0;

		/* Eq.8 Wigmosta et al [1994] */

		SoilEvap = MIN(EPot, DesorptionVolume);
		SoilEvap *= Adjust;
		SoilMoisture = *MoistContent * RootDepth * Adjust;

		MoistThrhld = FCap;
		tmp = MoistThrhld * RootDepth * Adjust;
		if (SoilEvap > SoilMoisture - tmp) {
			SoilEvap = SoilMoisture - tmp;
			//*MoistContent = MoistThrhld;
		}
		else {
			SoilMoisture -= SoilEvap;
			//*MoistContent = SoilMoisture / (RootDepth * Adjust);
		}
		return SoilEvap;
	}

	inline NUMERIC_TYPE SoilEvaporation_DHSVM_simple(int Dt, float Porosity, float FCap, float Ks,
		float Press, float m, float RootDepth,
		float *MoistContent, float Adjust)
	{
		float DesorptionVolume;	/* Amount of water the soil can deliver to the
									 atmosphere during a timestep (mm) */
		float EPot;			/* Potential evaporation from soil during timestep (mm) */
		float SoilEvap;		/* Amount of evaporation directly from the soil (mm) */
		float SoilMoisture;   /* Amount of water in surface soil layer (mm) */
		float MoistThrhld;    /* threshold that limits evap to maintain soil at a moisture level */
		float tmp;

		DesorptionVolume = Desorption(Dt, *MoistContent, Porosity, Ks, Press, m);

		/* Eq.4 Wigmosta et al [1994] */

		/* Calculate the density of pure water as a function of temperature.
		   Thiesen, Scheel-Diesselhorst Equation (in Handbook of hydrology, fig
		   11.1.1) */



		   /* The potential evaporation rate accounts for the amount of moisture that
			  the atmosphere can absorb.  If we do not account for the amount of
			  evaporation from overlying evaporation, we can end up with the situation
			  that all vegetation layers and the soil layer transpire/evaporate at the
			  potential rate, resulting in an overprediction of the actual evaporation
			  rate.  Thus we subtract the amount of evaporation that has already
			  been calculated for overlying layers from the potential evaporation.
			  Another mechanism that could be used to account for this would be to
			  decrease the vapor pressure deficit while going down through the canopy
			  (not implemented here) */

		EPot = 0;

		/* Eq.8 Wigmosta et al [1994] */

		SoilEvap = MIN(EPot, DesorptionVolume);
		SoilEvap *= Adjust;
		SoilMoisture = *MoistContent * RootDepth * Adjust;

		MoistThrhld = FCap;
		tmp = MoistThrhld * RootDepth * Adjust;
		if (SoilEvap > SoilMoisture - tmp) {
			SoilEvap = SoilMoisture - tmp;
			//*MoistContent = MoistThrhld;
		}
		else {
			SoilMoisture -= SoilEvap;
			//*MoistContent = SoilMoisture / (RootDepth * Adjust);
		}
		return SoilEvap;
	}

inline NUMERIC_TYPE SGC2_Freeze_floodplain_row(
	const int row_start, int row_end,
	const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE row_cell_area,
	Pars *Parptr,
	const Solver *Solverptr,
	NUMERIC_TYPE* Freeze_Row,
	NUMERIC_TYPE temperature_step, // 温度
	const NUMERIC_TYPE * h_row,
	NUMERIC_TYPE * volume_row, 
	NUMERIC_TYPE * snow)
{
#ifdef __INTEL_COMPILER
	__assume_aligned(h_row, 64);
	__assume_aligned(volume_row, 64);
#endif

	NUMERIC_TYPE reduce_freeze_loss = C(0.0);
#pragma ivdep
#pragma simd
	for (int i = row_start; i < row_end; i++)
	{
		NUMERIC_TYPE h_new, dV = C(0.0);
		NUMERIC_TYPE h_old = h_row[i];
		NUMERIC_TYPE freeze_deltaH_step = Parptr->FddSnow * Parptr->Frr * FABS(temperature_step - Parptr->melt_temperature)  * Solverptr->SGCtmpTstep / 1000;
		if (h_old > depth_thresh) // There is water to evaporate on the flood plain
		{
			// update depth by subtracting evap depth
			h_new = h_old - freeze_deltaH_step;
			//check for -ve depths
			if (h_new < C(0.0))
			{
				// reduce evap loss to account for dry bed (don't go below 0)
				dV = h_old * row_cell_area;
			}
			else
			{
				dV = freeze_deltaH_step * row_cell_area;
			}
			volume_row[i] -= dV;
			// update snow cover thickness
			Freeze_Row[i] = dV / row_cell_area;
			snow[i] += Freeze_Row[i];
			
		}
		reduce_freeze_loss += dV; //mass-balance for a standard cell
	}
	return reduce_freeze_loss;
}

inline NUMERIC_TYPE SGC2_Melt_floodplain_row(
	const int row_start, int row_end,
	const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE row_cell_area,
	Pars *Parptr,
	const Solver *Solverptr,
	NUMERIC_TYPE* Freeze_Row,
	NUMERIC_TYPE temperature_step, // 温度
	const NUMERIC_TYPE * h_row,
	NUMERIC_TYPE * volume_row,
	NUMERIC_TYPE * snow)
{
#ifdef __INTEL_COMPILER
	__assume_aligned(h_row, 64);
	__assume_aligned(volume_row, 64);
#endif

	NUMERIC_TYPE reduce_freeze_loss = C(0.0);
#pragma ivdep
#pragma simd
	for (int i = row_start; i < row_end; i++)
	{
		NUMERIC_TYPE h_new, dV = C(0.0);
		NUMERIC_TYPE h_old = h_row[i];
		NUMERIC_TYPE freeze_deltaH_step = Parptr->FddSnow * Parptr->Frr * (temperature_step - Parptr->melt_temperature)  * Solverptr->SGCtmpTstep;
		if (h_old > depth_thresh) // There is water to evaporate on the flood plain
		{
			// update depth by subtracting evap depth
			h_new = h_old - freeze_deltaH_step;
			//check for -ve depths
			if (h_new < C(0.0))
			{
				// reduce evap loss to account for dry bed (don't go below 0)
				dV = h_old * row_cell_area;
			}
			else
			{
				dV = freeze_deltaH_step * row_cell_area;
			}
			volume_row[i] -= dV;
			// update snow cover thickness
			snow[i] += dV / row_cell_area;
		}
		reduce_freeze_loss += dV; //mass-balance for a standard cell
	}
	return reduce_freeze_loss;
}

inline NUMERIC_TYPE SGC2_Snowfall_row(const int j, const NUMERIC_TYPE snow_deltaV_step, const NUMERIC_TYPE row_cell_area,
	const NUMERIC_TYPE * dem_row,
	NUMERIC_TYPE * snow,
	WetDryRowBound * wet_dry_bounds, const States *Statesptr, NUMERIC_TYPE* Snow_Row) {
	const int row_start = wet_dry_bounds->dem_data[j].start;
	const int row_end = wet_dry_bounds->dem_data[j].end;
	NUMERIC_TYPE snow_fall_block_loss = 0.0;
	for (int i = row_start; i < row_end; i++)
	{
		//NUMERIC_TYPE dV = (dem_row[i] != C(1e10)) ? rain_step_dV : C(0.0);
		NUMERIC_TYPE dV;
		if (dem_row[i] != DEM_NO_DATA)
		{
			// snow fall depth, mm
			Snow_Row[i] = snow_deltaV_step / row_cell_area;
			// snow thickness
			snow[i] += Snow_Row[i];
			dV = snow_deltaV_step;
			snow_fall_block_loss += dV;
		}
	}
	return snow_fall_block_loss;
}

// if snow melt when snow exists, glacier begin melting after snow thickness reduced to 0
inline NUMERIC_TYPE SGC2_Snow_Glacier_Melt_row(const int j,
	const NUMERIC_TYPE row_cell_area,
	const NUMERIC_TYPE * dem_row,
	NUMERIC_TYPE * volume_row,
	WetDryRowBound * wet_dry_bounds, const States *Statesptr, 
	Pars *Parptr,const Solver *Solverptr,
	NUMERIC_TYPE temperature_step, // 温度
	NUMERIC_TYPE* SnowMelt_Row, NUMERIC_TYPE * GlacierMelt_Row,
	NUMERIC_TYPE* Snow, NUMERIC_TYPE* Glacier
)
{
	NUMERIC_TYPE loc_rainfall_total = C(0.0);
	// 降雨是在整个dem栅格上进行的
	const int row_start = wet_dry_bounds->dem_data[j].start;
	const int row_end = wet_dry_bounds->dem_data[j].end;

	// update wet_dry_bounds as all dem cells will now be wet
	wet_dry_bounds->fp_vol[j] = wet_dry_bounds->dem_data[j];

#ifdef __INTEL_COMPILER
	__assume_aligned(dem_row, 64);
	__assume_aligned(volume_row, 64);
#endif

#pragma ivdep
#pragma simd reduction (+:loc_rainfall_total)
	NUMERIC_TYPE meltDeltaHStep = 0.0;
	for (int i = row_start; i < row_end; i++)
	{
		//NUMERIC_TYPE dV = (dem_row[i] != C(1e10)) ? rain_step_dV : C(0.0);
		NUMERIC_TYPE dV;
		if (dem_row[i] != DEM_NO_DATA)
		{
			if (Snow[i] > ZERO_LIMIT)
			{
				// todo：这里有一个小的优化点，就是在一个时间步长内，雪化完了冰也会融化一部分
				meltDeltaHStep = Parptr->FddSnow * (temperature_step - Parptr->melt_temperature) * Solverptr->SGCtmpTstep / 1000.0;
				if (meltDeltaHStep > Snow[i])
				{
					meltDeltaHStep = Snow[i];
					Snow[i] = 0.0;
				}
				else
				{
					Snow[i] -= meltDeltaHStep;
				}
				SnowMelt_Row[i] = meltDeltaHStep;
				GlacierMelt_Row[i] = 0.0;
				dV = meltDeltaHStep * row_cell_area;
			}else if (Glacier[i] > ZERO_LIMIT) {
				meltDeltaHStep = Parptr->FddGlacier * (temperature_step - Parptr->melt_temperature) * Solverptr->SGCtmpTstep / 1000.0;
				if (meltDeltaHStep > Glacier[i])
				{
					meltDeltaHStep = Glacier[i];
					Glacier[i] = 0.0;
				}
				else
				{
					Glacier[i] -= meltDeltaHStep;
				}
				GlacierMelt_Row[i] = meltDeltaHStep;
				SnowMelt_Row[i] = 0.0;
				dV = meltDeltaHStep * row_cell_area;
			}
			else
			{
				dV = 0.0;
			}

			volume_row[i] += dV; // add rainfall volume to cell		
			loc_rainfall_total += dV; // mass balance for local cell (cumulative)
		}

	}
	return loc_rainfall_total;
}

// routine for uniform rainfall
inline NUMERIC_TYPE SGC2_Uniform_Rainfall_row(const int j,
	const NUMERIC_TYPE rain_deltaV_step,const NUMERIC_TYPE row_cell_area,
	const NUMERIC_TYPE * dem_row,
	NUMERIC_TYPE * volume_row, NUMERIC_TYPE * delta_volume_row_ch,
	WetDryRowBound * wet_dry_bounds, const States *Statesptr, NUMERIC_TYPE* Rain_Row_POI,
	Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr, int grid_row_index,
	const int cell_count, const int sg_row_start, const int * sg_cell_grid_index_lookup, const NUMERIC_TYPE * sg_cell_cell_area,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume, const NUMERIC_TYPE * sg_cell_SGC_c,
	const SubGridFlowLookup * sg_cell_flow_lookup)
{
	NUMERIC_TYPE loc_rainfall_total = C(0.0);
	// 降雨是在整个dem栅格上进行的
	const int row_start = wet_dry_bounds->dem_data[j].start;
	const int row_end = wet_dry_bounds->dem_data[j].end;

	// update wet_dry_bounds as all dem cells will now be wet
	wet_dry_bounds->fp_vol[j] = wet_dry_bounds->dem_data[j];

#ifdef __INTEL_COMPILER
	__assume_aligned(dem_row, 64);
	__assume_aligned(volume_row, 64);
#endif

#pragma ivdep
#pragma simd reduction (+:loc_rainfall_total)
	
	for (int i = row_start; i < row_end; i++)
	{
		//NUMERIC_TYPE dV = (dem_row[i] != C(1e10)) ? rain_step_dV : C(0.0);
		NUMERIC_TYPE dV;
		int index = grid_row_index + i;
		int source_index_this = j * Parptr->xsz + i;
		if (dem_row[i] != DEM_NO_DATA)
		{
			// 如果该栅格是SGC河道，则降雨被分为河道上的降雨和其两侧蓄洪区的降雨，volume_row特指蓄洪区的降雨，volume_bed是河道上的降雨
			if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
				continue;
			}
			dV = rain_deltaV_step;
			volume_row[i] += dV; // add rainfall volume to cell		
			loc_rainfall_total += dV; // mass balance for local cell (cumulative)
			// xdw add, 记录栅格上的降雨深度
			if (Statesptr->save_poi)
			{
				Rain_Row_POI[i] += rain_deltaV_step * 1000.0 / row_cell_area;    // mm
			}
			
		}

	}
	for (int cell_i = 0; cell_i < cell_count; cell_i++)
	{
		int cell_index = sg_row_start + cell_i;

		int grid_index = sg_cell_grid_index_lookup[cell_index];
		int i = grid_index - grid_row_index;
		NUMERIC_TYPE dV = 0.0, dVCh = 0.0, dVFp = 0.0;

		// // 河道所在的栅格单元面积
		const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];

		NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index] * 1000.0;
		NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];  // SGC河道单元底面积
		// xiaodw, mm/h -> m/s -> m3
		dV = rain_deltaV_step ;
		dVCh = dV * SGC_c / row_cell_area;
		dVFp = dV - dVCh;
		volume_row[i] += dVFp;    // 蓄洪区
		delta_volume_row_ch[i] += dVCh;   // 河道
		// xdw add, 记录栅格上的降雨深度
		if (Statesptr->save_poi)
		{
			Rain_Row_POI[i] += rain_deltaV_step * 1000.0 / row_cell_area;    // mm
		}
	}
	return loc_rainfall_total;
}

// routine for distributed and distributed time varying rainfall
inline NUMERIC_TYPE SGC2_Distrubuted_Rainfall_row(const int j,
	const NUMERIC_TYPE delta_time,
	const NUMERIC_TYPE * dem_row, const NUMERIC_TYPE * rainmask_row,
	NUMERIC_TYPE * volume_row, NUMERIC_TYPE *  delta_volume_row_ch,
	WetDryRowBound * wet_dry_bounds, const States * Statesptr, const NUMERIC_TYPE row_cell_area, NUMERIC_TYPE* Rain_Row_POI,
	Pars *Parptr, const Solver *Solverptr, Arrays * Arrptr,  int grid_row_index,
	const int cell_count, const int sg_row_start, const int * sg_cell_grid_index_lookup, const NUMERIC_TYPE * sg_cell_cell_area,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume, const NUMERIC_TYPE * sg_cell_SGC_c,
	const SubGridFlowLookup * sg_cell_flow_lookup)
{
	NUMERIC_TYPE loc_rainfall_total = C(0.0);

	const int row_start = wet_dry_bounds->dem_data[j].start;
	const int row_end = wet_dry_bounds->dem_data[j].end;

	// update wet_dry_bounds as all dem cells will now be wet
	wet_dry_bounds->fp_vol[j] = wet_dry_bounds->dem_data[j];

#ifdef __INTEL_COMPILER
				__assume_aligned(dem_row, 64);
				__assume_aligned(volume_row, 64);
#endif

#pragma ivdep
#pragma simd reduction (+:loc_rainfall_total)

	for (int i = row_start; i < row_end; i++)
	{
		//NUMERIC_TYPE dV = (dem_row[i] != C(1e10)) ? rain_step_dV : C(0.0);
		NUMERIC_TYPE dV;
		int index = grid_row_index + i;
		int source_index_this = j * Parptr->xsz + i;
		
		if (dem_row[i] != DEM_NO_DATA)
		{
			// 如果该栅格是SGC河道，则降雨被分为河道上的降雨和其两侧蓄洪区的降雨，volume_row特指蓄洪区的降雨，volume_bed是河道上的降雨
			if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
				continue;
			}
			// xiaodw, mm/h -> m/s -> m3
			dV = rainmask_row[i] / (3600.0 * 1000.0)  * delta_time * row_cell_area;
			volume_row[i] += dV; // add rainfall volume to cell		
			loc_rainfall_total += dV; // mass balance for local cell (cumulative)
			if (Statesptr->save_poi)
			{
				//Rain_Row_POI[i] += rainmask_row[i] / (3600.0 * 1000.0) * delta_time;   // mm/h -> m
				Rain_Row_POI[i] += rainmask_row[i] / (3600.0) * delta_time;   // mm/h -> mm
			}
		}
	}
	NUMERIC_TYPE dV = 0.0, dVCh = 0.0, dVFp = 0.0;

	for (int cell_i = 0; cell_i < cell_count; cell_i++)
	{
		int cell_index = sg_row_start + cell_i;

		int grid_index = sg_cell_grid_index_lookup[cell_index];
		int i = grid_index - grid_row_index;

		// // 河道所在的栅格单元面积
		const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
		
		NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index] * 1000.0;
		NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];  // SGC河道单元底面积
		// xiaodw, mm/h -> m/s -> m3
		dV = rainmask_row[i] / (3600.0 * 1000.0)  * delta_time * row_cell_area;
		dVCh = dV * SGC_c / row_cell_area;
		dVFp = dV - dVCh;
		volume_row[i] += dVFp;    // 蓄洪区
		delta_volume_row_ch[i] += dVCh;   // 河道
		if (Statesptr->save_poi)
		{
			//Rain_Row_POI[i] += rainmask_row[i] / (3600.0 * 1000.0) * delta_time;   // mm/h -> m
			Rain_Row_POI[i] += rainmask_row[i] / (3600.0) * delta_time;   // mm/h -> mm
		}
	}

	return loc_rainfall_total;
}


//-----------------------------------------------------------------------------------
// BOUNDARY CONDITIONS
// Calculate Qx and Qy at edges of the domain in response to boundary
// conditions
// 此方法处理东南西北四个边界处的流量Qx和Qy
void SGC2_BCs(const int grid_cols, const int grid_rows, const int grid_cols_padded, const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE g,
	const NUMERIC_TYPE* dx_col, const NUMERIC_TYPE* dy_col,
	const NUMERIC_TYPE* h_grid,
	NUMERIC_TYPE* Qx_grid, NUMERIC_TYPE* Qy_grid, NUMERIC_TYPE* Qx_old_grid, NUMERIC_TYPE* Qy_old_grid,
	WetDryRowBound* wet_dry_bounds,
	const States *Statesptr, const Pars *Parptr, BoundaryCondition * boundary_cond, const SGCprams *SGCptr, const NUMERIC_TYPE max_Froude)
{
	//int i, lyr;
	//int BCi, index, sign, dir, edge, q_index, gr;
	//NUMERIC_TYPE h1, z1, hflow, dh, surface_slope, g, SGC_width_current, A, R, cell_length, cell_width;
	//NUMERIC_TYPE *q_FP_old, *q_SG_old, *q_FP_combined;
	NUMERIC_TYPE Q_multiplier = C(1.0);

	// CCS Multiplier for Q boundaries. If using regular grid, Qs are specified as m^2 and need to be multiplied by dx; 
	// if using lat-long Qs are specified in m^3 and therefore multiplier is C(1.) Note intialised above as C(1.0).
	if (Statesptr->latlong == OFF) Q_multiplier = Parptr->dx;
	// bc_info的存储顺序：上，右，左，下
	const int numBCs = boundary_cond->bc_info.count;
	WaterSource ws = boundary_cond->bc_info;

	NUMERIC_TYPE * q_SG_old = ws.Q_SG_old;

	NUMERIC_TYPE q_in = C(0.0);
	NUMERIC_TYPE q_out = C(0.0);
	// 获取每个边界像元的长和宽，以及qx，qy
	for (int BCi = 0; BCi < numBCs; BCi++)
	{
		if (ws.Ident[BCi] != NONE0) // if BCi = 0 do nothing
		{
			NUMERIC_TYPE *q_FP_old, *q_FP_combined;
			NUMERIC_TYPE cell_length, cell_width;
			int index, q_index, q_sg_old_index, q_fp_old_index;
			int i, j;
			int sign, gr;

			q_sg_old_index = BCi;
			//q_FP_old = ws.Q_FP_old + BCi;

			// First for each edge number work out where it is on the boundary,
			// the associated edge pixels and whether it's facing in the x or y
			// direction
			#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
			cout << "BCi: " << BCi << " ws.Ident: " << ws.Ident[BCi];
			#endif // IS_DEBUG

			
			// 根据BCi所处的位置给cell_length和cell_width赋值
			// 第一行
			if (BCi < grid_cols)
			{
				// N(lyr=0) edge
				j = 0;
				i = BCi;

				index = i;
				q_index = i;// + lyr * grid_cols_padded;
				q_fp_old_index = q_index;

				q_FP_combined = Qy_grid;
				q_FP_old = Qy_old_grid;

				//SGC_qptr = SGC_Qy_grid + q_index;
				sign = -1;
				cell_length = dy_col[j];
				cell_width = dx_col[j];
				#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
				cout << " north cell_length: " << cell_length << " cell_width: " << cell_width << endl;
				#endif
				//printf("North %d, %d\n", i, lyr);
				//int j2 = (int)floor((double)index / grid_cols_padded);
				//int i2 = index - lyr*grid_cols_padded;
				//printf("Check %d, %d\n", i2, j2);
			}
			// 最东边的边界
			else if (/*BCi >= grid_cols && */ BCi < grid_cols + grid_rows)
			{
				// E edge 对应每一行的最后一列
				j = BCi - grid_cols;
				i = grid_cols - 1;

				index = i + j * grid_cols_padded;
				q_index = index + 1;
				q_fp_old_index = q_index;

				q_FP_combined = Qx_grid;
				q_FP_old = Qx_old_grid;
				//SGC_qptr = SGC_Qx_grid + q_index;
				sign = 1;
				cell_length = dx_col[j];
				cell_width = dy_col[j];
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
				cout << " east cell_length: " << cell_length << " cell_width: " << cell_width << endl;
#endif
				//printf("East %d, %d\n", i, lyr);
				//int j2 = (int)floor((double)index / grid_cols_padded);
				//int i2 = index - lyr*grid_cols_padded;
				//printf("Check %d, %d\n", i2, j2);
			}
			// 最西边的边界
			else if (/*BCi >= grid_cols + grid_rows &&*/ BCi < 2 * grid_cols + grid_rows)
			{
				// S(lyr=ysz-1) edge
				j = grid_rows - 1;
				i = (2 * grid_cols + grid_rows) - BCi - 1;

				index = i + j * grid_cols_padded;
				q_index = index + grid_cols_padded; // next row
				q_fp_old_index = q_index;

				q_FP_combined = Qy_grid;
				q_FP_old = Qy_old_grid;
				//SGC_qptr = SGC_Qy_grid + q_index;
				sign = 1;
				cell_length = dy_col[j];
				cell_width = dx_col[j];
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
				cout << " south cell_length: " << cell_length << " cell_width: " << cell_width << endl;
#endif
				//printf("South %d, %d\n", i, lyr);
				//int j2 = (int)floor((double)index / grid_cols_padded);
				//int i2 = index - lyr*grid_cols_padded;
				//printf("Check %d, %d\n", i2, j2);
			}
			else
			{
				// W edge
				j = (2 * grid_cols + 2 * grid_rows) - BCi - 1;
				i = 0;

				index = i + j * grid_cols_padded;
				q_index = index;
				q_fp_old_index = q_index;

				q_FP_combined = Qx_grid;
				q_FP_old = Qx_old_grid;
				//SGC_qptr = SGC_Qx_grid + q_index;
				sign = -1;
				cell_length = dx_col[j];
				cell_width = dy_col[j];
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
				cout << " west cell_length: " << cell_length << " cell_width: " << cell_width << endl;
#endif
				//printf("West  %d, %d\n", i, lyr);
				//int j2 = (int)floor((double)index / grid_cols_padded);
				//int i2 = index - lyr*grid_cols_padded;
				//printf("Check %d, %d\n", i2, j2);
			}

			//// CCS Record cell length and cell width relative to direction of channel (for lat-long grids)
			//if (edge == 1 || edge == 3) // N or S boundary; assume flow is N-S or S-N
			//{
			//	cell_length = dy_col[lyr];
			//	cell_width = dx_col[lyr];
			//}
			//if (edge == 2 || edge == 4) // E or W boundary; assume flow is E-W or W-E
			//{
			//	cell_length = dx_col[lyr];
			//	cell_width = dy_col[lyr];
			//}

			gr = ws.ws_cell.sg_cell_SGC_group[BCi];

			NUMERIC_TYPE g_friction_squared_FP = ws.g_friction_squared_FP[BCi];
			NUMERIC_TYPE g_friction_squared_SG = ws.g_friction_squared_SG[BCi];
			// Now calculate flows
			switch (ws.Ident[BCi])
			{
			// 计算bci中规定的边界(边或点)上的流量之和
			case FREE1: // FREE boundary
			{
				if (h_grid[index] + ws.ws_cell.sg_cell_SGC_BankFullHeight[BCi] > depth_thresh)
				{
					NUMERIC_TYPE qcorrected;
					// calcQ
					// 计算点上的洪泛区流量和河道流量，如果河道宽度>栅格宽度，则洪泛区流量为0
					qcorrected = SGC2_CalcPointFREE(h_grid[index], ws.ws_cell.sg_cell_SGC_width[BCi], ws.Val[BCi],
						depth_thresh, delta_time, cell_width, g, g_friction_squared_SG, g_friction_squared_FP,
						ws.ws_cell.sg_cell_SGC_BankFullHeight[BCi], gr, sign, &q_FP_old[q_fp_old_index], &q_SG_old[q_sg_old_index], SGCptr, max_Froude);
					// 洪泛区流量+河道流量
					q_FP_combined[q_index] = qcorrected + q_SG_old[q_sg_old_index];
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
					cout << " free1 qcorrected: " << qcorrected << " q_SG_old: " << q_SG_old[q_sg_old_index] << " q_FP_combined: " << q_FP_combined[q_index] << endl;
#endif
				}
				else
				{
					q_FP_combined[q_index] = C(0.0);
					q_FP_old[q_fp_old_index] = C(0.0);
					q_SG_old[q_sg_old_index] = C(0.0);
				}
			}
			break;

			// 这两种方案的含义是计算指定点，位于指定高程与河道/洪泛区表面之间的流量
			case HFIX2:// HFIX & HVAR boundary
			case HVAR3:// HFIX & HVAR boundary
			{
				// 如果水深+河道深度>阈值
				if (h_grid[index] + ws.ws_cell.sg_cell_SGC_BankFullHeight[BCi] > depth_thresh)
				{
					NUMERIC_TYPE surface_elevation0;
					if (ws.Ident[BCi] == HFIX2)
						surface_elevation0 = ws.Val[BCi];   // boundary depth for HFIX
					else // boundary depth for HVAR 
						surface_elevation0 = InterpolateTimeSeries(ws.timeSeries[BCi], curr_time);
					//note: h0 is absolute height - not relative height (equivilent to h1+dem[index])
					NUMERIC_TYPE h1, z1, SGC_width_current, hflow, dh, surface_slope, R, A;

					h1 = h_grid[index];     // cell depth
					z1 = ws.ws_cell.sg_cell_dem[BCi];   // FP elevation
					SGC_width_current = ws.ws_cell.sg_cell_SGC_width[BCi]; // SGC width
					//如果点处于sgc河道上，计算河道上的流量
					if (SGC_width_current > C(0.0)) //  check for sub-grid channel
					{
						//surface_elevation0-z1 is depth above flood plain (may be negative) add bankfullheight to get depth above channel bed
						hflow = getmax(surface_elevation0 - z1, h1) + ws.ws_cell.sg_cell_SGC_BankFullHeight[BCi]; // use max of cell depth and boundary depth
						//h0 is a surface elevation
						dh = surface_elevation0 - (h1 + z1);

						surface_slope = dh / (cell_length*SGCptr->SGCm[gr]);
						//if (edge == 1 || edge == 4) surface_slope = -surface_slope;
						surface_slope *= sign;

						SGC2_CalcA(gr, hflow, ws.ws_cell.sg_cell_SGC_BankFullHeight[BCi], &A, &SGC_width_current, SGCptr); // calculate channel area for SGC
						R = SGC2_CalcR(gr, hflow, ws.ws_cell.sg_cell_SGC_BankFullHeight[BCi], SGC_width_current, ws.ws_cell.sg_cell_SGC_width[BCi], A, SGCptr); // calculate hydraulic radius for SGC

						q_SG_old[q_sg_old_index] = CalculateQ(surface_slope, R, delta_time, g, A, g_friction_squared_SG, q_SG_old[q_sg_old_index], max_Froude);
					}

					hflow = getmax(getmax(surface_elevation0 - z1, h1), C(0.0));
					// multiply flux by -sign and use absolute value of q0 to get flux directions correctly assigned at boundaries
					// FABS on surface_slope and q0 always results in positive or no flow... sign then sorts out the direction(jcn)
					//if(hflow>depth_thresh && SGC_width_current < Parptr->dx) //CCS_deletion
					// 计算洪泛区上的流量
					// 如果sgc河道宽度>栅格宽度，则洪泛区上的流量=0
					if (hflow > depth_thresh && SGC_width_current < cell_width)
					{
						dh = surface_elevation0 - (h1 + z1);

						surface_slope = dh / cell_length;

						surface_slope *= sign;

						A = cell_width * hflow;

						// calculate FP flow
						NUMERIC_TYPE q;
						q = CalculateQ(surface_slope, hflow, delta_time, g, A, g_friction_squared_FP, q_FP_old[q_fp_old_index], max_Froude);
						q_FP_old[q_fp_old_index] = q;

						if (SGC_width_current > C(0.0))
						{
							NUMERIC_TYPE channel_ratio = min(SGC_width_current / cell_width, C(1.0));
							q = q - channel_ratio * q;
						}
						q_FP_combined[q_index] = q;

					}
					else
					{
						q_FP_combined[q_index] = C(0.0);
						q_FP_old[q_fp_old_index] = C(0.0);
					}
					q_FP_combined[q_index] += q_SG_old[q_sg_old_index];
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
					cout << "HFIX2 surface_elevation0: " << surface_elevation0 << "dh: " << dh << " q_SG_old: " << q_SG_old[q_sg_old_index] << " q_FP_combined: " << q_FP_combined[q_index] << endl;
#endif
				}
				else
				{
					q_FP_combined[q_index] = C(0.0);
					q_FP_old[q_fp_old_index] = C(0.0);
					q_SG_old[q_sg_old_index] = C(0.0);
				}
			}
			break;

			case QFIX4:// QFIX boundary
			{
				//*qptr=-sign*ws.Val[BCi]*Parptr->dx; //CCS_deletion
				NUMERIC_TYPE q = -sign*ws.Val[BCi] * Q_multiplier;
				q_FP_combined[q_index] = q;
				q_FP_old[q_fp_old_index] = q;
				q_SG_old[q_sg_old_index] = C(0.0);
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
				cout << "QFIX4 Q_multiplier: " << Q_multiplier << "ws.Val: " << ws.Val[BCi] << " q_SG_old: " << q_SG_old[q_sg_old_index] << " q_FP_combined: " << q_FP_combined[q_index] << endl;
#endif
			}
			break;

			case QVAR5:// QVAR boundary
			{
				//*qptr=-sign*InterpolateTimeSeries(ws.TimeSeries[BCi],curr_time)*Parptr->dx; //CCS_deletion
				NUMERIC_TYPE q = -sign * InterpolateTimeSeries(ws.timeSeries[BCi], curr_time)*Q_multiplier;
				q_FP_combined[q_index] = q;
				q_FP_old[q_fp_old_index] = q;
				q_SG_old[q_sg_old_index] = C(0.0);
			}
			break;
			default:
				break;
			}
			NUMERIC_TYPE q = q_FP_combined[q_index];
			// ensure that any flow or change in volume is processed by subsequent steps
			if (q != C(0.0))
			{
				q *= sign;
				// q_out一开始=0，最后其实就是所有边界栅格的q之和。
				//在宜丰的案例中，边界被暂时设置为研究区的东南西北四条边，那边界q就是边界上所有流量之和，因为边界上没有dem，所以q=0
				if (q > 0)
					q_out += q;
				else
					q_in -= q;

				wet_dry_bounds->fp_vol[j].start = min(wet_dry_bounds->fp_vol[j].start, i);
				wet_dry_bounds->fp_vol[j].end = max(wet_dry_bounds->fp_vol[j].end, i + 1);
			}
		}
	}
	// todo: 如何得到合理的qout？
	boundary_cond->Qout = q_out;
	boundary_cond->Qin = q_in;

	return;
}


// xdw, 此方法计算水从一个点不断进入流域，类似于从SWMM的某个城市雨水井溢出水；
// 或计算水从一个点不断流出流域，例如一个sgc河道里的流域出口点（FREE6）
void SGC2_PointSources_Vol_row(const int y, const int grid_cols,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE g, const NUMERIC_TYPE Q_multiplier,
	const NUMERIC_TYPE *dx_col, const NUMERIC_TYPE *dy_col,
	const NUMERIC_TYPE * h_grid,
	const SGCprams *SGCptr,
	NUMERIC_TYPE * volume_row,
	PointSourceRowList * ps_layout,
	WetDryRowBound* wet_dry_bounds,
	NUMERIC_TYPE * out_Qpoint_timestep_pos, NUMERIC_TYPE * out_Qpoint_timestep_neg,
	const NUMERIC_TYPE max_Froude)
{
	NUMERIC_TYPE Qpoint_timestep_pos = C(0.0);
	NUMERIC_TYPE Qpoint_timestep_neg = C(0.0);
	const int ps_count = ps_layout->ps_row_count[y];
	const int row_cols_padded = ps_layout->row_cols_padded;
	const int row_start = y * row_cols_padded;
	WaterSource ps_info = ps_layout->ps_info;
	for (int i = 0; i < ps_count; i++)
	{
		int ws_index = row_start + i;

		NUMERIC_TYPE h;
		// Set initial dV and himp as zero
		NUMERIC_TYPE dV = C(0.0);
		int grid_index;
		int ps_x = ps_info.ws_cell.sg_cell_x[ws_index];
		int ps_y = ps_info.ws_cell.sg_cell_y[ws_index];
		// location in vector
		grid_index = ps_info.ws_cell.sg_cell_grid_index_lookup[ws_index];
		// different boundary conditions
		switch (ps_info.Ident[ws_index])
		{
			//NOTE HVAR and HFIX applied after update H
		case QVAR5: //QVAR ps.Val already set to the interpolated value
		case QFIX4:
			dV = ps_info.Val[ws_index] * Q_multiplier * delta_time; // QFIX // Calculate change in volume
			break;
		case FREE6:
			h = h_grid[grid_index] + ps_info.ws_cell.sg_cell_SGC_BankFullHeight[ws_index];
			if (h > depth_thresh)
			{
				NUMERIC_TYPE cell_width = getmin(dx_col[ps_y], dy_col[ps_y]);
				NUMERIC_TYPE FP_g_friction_squared = ps_info.g_friction_squared_FP[ws_index];
				NUMERIC_TYPE SGC_g_friction_squared = ps_info.g_friction_squared_SG[ws_index];

				NUMERIC_TYPE q_free_FP_corrected = SGC2_CalcPointFREE(h_grid[grid_index], ps_info.ws_cell.sg_cell_SGC_width[ws_index], ps_info.Val[ws_index],
					depth_thresh, delta_time, cell_width, g,
					SGC_g_friction_squared, FP_g_friction_squared,
					ps_info.ws_cell.sg_cell_SGC_BankFullHeight[ws_index], ps_info.ws_cell.sg_cell_SGC_group[ws_index],
					-1, &ps_info.Q_FP_old[ws_index], &ps_info.Q_SG_old[ws_index], SGCptr, max_Froude);

				NUMERIC_TYPE Qfree = q_free_FP_corrected + ps_info.Q_SG_old[ws_index];
				dV = Qfree * delta_time;
			}
			break;
		}

		if (dV != C(0.0))
		{
			wet_dry_bounds->fp_vol[ps_y].start = min(wet_dry_bounds->fp_vol[ps_y].start, ps_x);
			wet_dry_bounds->fp_vol[ps_y].end = max(wet_dry_bounds->fp_vol[ps_y].end, ps_x + 1);
			// update the cell volume change and in point source Q
			volume_row[ps_x] += dV; // Add volume to SGCdVol for use later by update H e.g wait for main update H before calculating H
			// Update Qpoint
			if (dV > 0)
				Qpoint_timestep_pos += dV;
			else
				Qpoint_timestep_neg += dV;
		}
	}
	(*out_Qpoint_timestep_pos) = Qpoint_timestep_pos;
	(*out_Qpoint_timestep_neg) = Qpoint_timestep_neg;
}

void SGC2_PointSources_H_row(const int y, const int grid_cols,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE *cell_area_col,
	const SGCprams *SGCptr,
	NUMERIC_TYPE * h_grid,
	NUMERIC_TYPE * volume_grid,
	PointSourceRowList * ps_layout,
	WetDryRowBound* wet_dry_bounds,
	NUMERIC_TYPE * out_Qpoint_timestep_pos, NUMERIC_TYPE * out_Qpoint_timestep_neg)
{
	// todo note - could be simpler to just calculate the new total volume based on height, rather than dv adjustment
	// less prone to errors and possibly faster

	NUMERIC_TYPE Qpoint_timestep_pos = C(0.0);
	NUMERIC_TYPE Qpoint_timestep_neg = C(0.0);

	const int ps_count = ps_layout->ps_row_count[y];
	const int row_cols_padded = ps_layout->row_cols_padded;
	const int row_start = y * row_cols_padded;
	WaterSource ps_info = ps_layout->ps_info;
	for (int i = 0; i < ps_count; i++)
	{
		int ws_index = row_start + i;
		// Set initial dV and himp as zero
		NUMERIC_TYPE V;
		NUMERIC_TYPE new_h = C(0.0);
		int gr;

		// location in grid

		const int ps_y = ps_info.ws_cell.sg_cell_y[ws_index];
		// location in vector
		const int grid_index = ps_info.ws_cell.sg_cell_grid_index_lookup[ws_index];
		// different boundary conditions
		if (ps_info.Ident[ws_index] == HFIX2 || ps_info.Ident[ws_index] == HVAR3) // HFIX or HVAR
		{
			// HVAR 'Val' already updated with the interpolated value for current time
			new_h = ps_info.Val[ws_index];
			new_h -= ps_info.ws_cell.sg_cell_dem[ws_index]; // get depth
			
			if (ps_info.ws_cell.sg_cell_SGC_width[ws_index] > C(0.0))
			{
				// sub-grid channel
				// ensure height is not below the bottom of the channel
				new_h = max(-ps_info.ws_cell.sg_cell_SGC_BankFullHeight[ws_index], new_h);

				// Calculate volume after update and subtract from before update
				gr = ps_info.ws_cell.sg_cell_SGC_group[ws_index]; // channel group number
				if (new_h < C(0.0) || ps_info.ws_cell.sg_cell_SGC_is_large[ws_index]) // if below the flood plain or cell is large Calculate channel volume
					V = SGC2_CalcUpV(new_h + ps_info.ws_cell.sg_cell_SGC_BankFullHeight[ws_index], ps_info.ws_cell.sg_cell_SGC_c[ws_index], gr, SGCptr);
				else // out of bank level
					V = ps_info.ws_cell.sg_cell_SGC_BankFullVolume[ws_index] + new_h * cell_area_col[ps_y];
			}
			else
			{
				// floodplain only cell
				new_h = max(C(0.0), new_h);
				V = new_h * cell_area_col[ps_y];
			}

			// ensure this point source is within the wet/dry bound (fp_vol_start,fp_vol_end used in update H to calculate the new fp_h_start)
			if (new_h > depth_thresh)
			{
				int ps_x = ps_info.ws_cell.sg_cell_x[ws_index];
				wet_dry_bounds->fp_vol[ps_y].start = min(wet_dry_bounds->fp_vol[ps_y].start, ps_x);
				wet_dry_bounds->fp_vol[ps_y].end = max(wet_dry_bounds->fp_vol[ps_y].end, ps_x + 1);
			}

			// calculate dV for mass balance
			NUMERIC_TYPE dV = V - volume_grid[grid_index];

			//h_grid[grid_index] = new_h;
			volume_grid[grid_index] = V;

			// Update Qpoint
			// xiaiodw, Qpoint_timestep_pos是点源流入，Qpoint_timestep_neg是流出
			if (dV > 0)
				Qpoint_timestep_pos += dV;
			else
				Qpoint_timestep_neg += dV;
		}
	}
	(*out_Qpoint_timestep_pos) = Qpoint_timestep_pos;
	(*out_Qpoint_timestep_neg) = Qpoint_timestep_neg;
}
// 根据流量更新干湿边界内每个栅格上的水量
// 在此方法中
inline void SGC2_UpdateVol_floodplain_row(const int j, const int grid_row_index, const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE row_cell_area,
	const NUMERIC_TYPE * Qx_grid, const NUMERIC_TYPE * Qy_grid,
	NUMERIC_TYPE * volume_grid, NUMERIC_TYPE * delta_volume_row, NUMERIC_TYPE * delta_volume_row_ch, NUMERIC_TYPE * h_grid,

	const WetDryRowBound * wet_dry_bounds, NUMERIC_TYPE * Q_Row_POI, NUMERIC_TYPE * Vol_Row_POI, Pars *Parptr,Arrays *Arrptr, Pois *Poisptr, const States *Statesptr,
	const int cell_count, const int sg_row_start, const int * sg_cell_grid_index_lookup)
{
	bool not_last_row = (j < grid_rows - 1);
	// flow from next cell, previous row, next row
	// row_start,row_end指向的都是列号（不是单元号），取当前行干湿边界向外扩一个
	int row_start = wet_dry_bounds->fp_vol[j].start - 1;   // 当前行干湿边界的开始 - 1
	int row_end = wet_dry_bounds->fp_vol[j].end + 1;    // 当前行干湿边界的结束 + 1
	if (j > 0)
	{
		row_start = min(row_start, wet_dry_bounds->fp_vol[j - 1].start);  // 上一行干湿边界的开始
		row_end = max(row_end, wet_dry_bounds->fp_vol[j - 1].end);     // 上一行干湿边界的结束
	}
	if (not_last_row)
	{
		row_start = min(row_start, wet_dry_bounds->fp_vol[j + 1].start);   // 下一行干湿边界的开始
		row_end = max(row_end, wet_dry_bounds->fp_vol[j + 1].end);      // 下一行干湿边界的结束
	}

	//ensure row_start, row_end are not out of bounds 确保每行的row_start, row_end在边界内
	row_start = max(wet_dry_bounds->dem_data[j].start, row_start);
	row_end = min(row_end, wet_dry_bounds->dem_data[j].end);

	// update bounds for subsequent updates
	//wet_dry_bounds->fp_vol[j].start = row_start;
	//wet_dry_bounds->fp_vol[j].end = row_end;
	// xiaodw 修改，暂时设为与dem计算范围一致，保证每个点都能计算到
	wet_dry_bounds->fp_vol[j].start = wet_dry_bounds->dem_data[j].start;
	wet_dry_bounds->fp_vol[j].end = wet_dry_bounds->dem_data[j].end;

	//__assume(row_start % GRID_ALIGN_WIDTH == 0);
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_cols_padded % GRID_ALIGN_WIDTH == 0);
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);

#endif
#ifdef __INTEL_COMPILER
	__assume_aligned(delta_volume_row, 64);
	__assume_aligned(Qx_grid, 64);
	__assume_aligned(Qy_grid, 64);
	__assume_aligned(volume_grid, 64);
#endif

	// 遍历每行干湿边界内的单元
	// 栅格单元上的水量 + 流量变化 + 降雨 - 蒸发 - 下渗
#pragma ivdep
#pragma simd
	for (int i = row_start; i < row_end; i++)  
	{
		int index = grid_row_index + i;
		int index_right = index + 1;
		int index_below = index + (grid_cols_padded);
		int source_index_this = j * Parptr->xsz + i;

		if (Arrptr->DEM[source_index_this] != DEM_NO_DATA) {
			NUMERIC_TYPE dV = 0.0;
			//NUMERIC_TYPE dV = delta_time * (Qx_grid[index] - Qx_grid[index_right] + Qy_grid[index] - Qy_grid[index_below]); // compute volume change in cell
			// xdw add, 记录流量------------------------???
			/*Q_Row[i] = dV;*/
			//if (Statesptr->save_poi)
			//{
			//	//  计算 格子上的 流量变化
			//	//  计算 格子上的 流量JING变化, 以mm形式输出
			//	Poisptr->Qx_Grid[index] += delta_time * (Qx_grid[index] - Qx_grid[index_right]) * 1000.0 / row_cell_area;
			//	Poisptr->Qy_Grid[index] += delta_time * (Qy_grid[index] - Qy_grid[index_below]) * 1000.0 / row_cell_area;
			//}
			
			// xiaodw，如果是河道像元则统计多少水来自于河道像元本身的产流，多少水来自于河道像元的直接侧向输出，多少水来自于上游单元的地表
			// xiaodw, 如果是河道则将河床的水量变化加入
			if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
				dV += delta_volume_row_ch[i];
				// 来自自身降雨扣除入渗后的水
				Parptr->hydro2ChPD[index] = delta_volume_row[i] + delta_volume_row_ch[i];
				delta_volume_row_ch[i] = 0.0;
				// 来自地下水库补给的水
				if (Statesptr->use_groundwater == ON)
				{
					dV += Parptr->gwQPerSgcCell * delta_time;
				}
			}
			// 把来自dhsvm垂向和侧向过程的水加入栅格水量
			dV += Parptr->delta_volumn_dhsvm_PD[index];
			dV += delta_volume_row[i]; // delta_volume_row是栅格单元上的降雨-蒸发-下渗量
			if (dV != C(0.0))
			{
				NUMERIC_TYPE V = volume_grid[index] + dV; // volume_grid是栅格单元上的水量
				volume_grid[index] = V;
				//h_grid[index] = V / row_cell_area;
			}

		}
	}
	int count = row_end - row_start;
	if (count > 0) {
		memset(delta_volume_row + row_start, 0, sizeof(NUMERIC_TYPE)*count); // delta_volume_row重置为0 
		//memset(delta_volume_row_ch + row_start, 0, sizeof(NUMERIC_TYPE)*count); // delta_volume_row重置为0 
	}
	// 更新河道水量
	//for (int cell_i = 0; cell_i < cell_count; cell_i++)
	//{
	//	int cell_index = sg_row_start + cell_i;

	//	int grid_index = sg_cell_grid_index_lookup[cell_index];
	//	int i = grid_index - grid_row_index;
	//	if (delta_volume_grid_ch[i] != C(0.0))
	//	{
	//		NUMERIC_TYPE V = volume_grid[grid_index] + delta_volume_grid_ch[i]; // volume_grid是栅格单元上的水量
	//		volume_grid[grid_index] = V;
	//		//h_grid[index] = V / row_cell_area;
	//	}
	//}
#if defined (_DEBUG) && _DEBUG > 1
	{
		int check_count = 0;
		for (int check = 0; check < grid_cols; check++)
		{
			if (delta_volume_row[check] != C(0.0))
			{
				check_count++;
				printf("dv!=0 %" NUM_FMT" (%d, %d),", delta_volume_row[check], check, j);
			}
		}
		if (check_count > 0)
			printf("\n");
	}
#endif
}


inline void SGC2_UpdateVol_floodplain_by_Q(const int j, const int grid_row_index, const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE row_cell_area,
	const NUMERIC_TYPE * Qx_grid, const NUMERIC_TYPE * Qy_grid,
	NUMERIC_TYPE * volume_grid, NUMERIC_TYPE * delta_volume_row, NUMERIC_TYPE * h_grid,

	const WetDryRowBound * wet_dry_bounds, NUMERIC_TYPE * Q_Row_POI, NUMERIC_TYPE * Vol_Row_POI, Pars *Parptr, Arrays *Arrptr, Pois *Poisptr, const States *Statesptr)
{
	bool not_last_row = (j < grid_rows - 1);
	// flow from next cell, previous row, next row
	// row_start,row_end指向的都是列号（不是单元号），取当前行干湿边界向外扩一个
	int row_start = wet_dry_bounds->fp_vol[j].start - 1;   // 当前行干湿边界的开始 - 1
	int row_end = wet_dry_bounds->fp_vol[j].end + 1;    // 当前行干湿边界的结束 + 1
	if (j > 0)
	{
		row_start = min(row_start, wet_dry_bounds->fp_vol[j - 1].start);  // 上一行干湿边界的开始
		row_end = max(row_end, wet_dry_bounds->fp_vol[j - 1].end);     // 上一行干湿边界的结束
	}
	if (not_last_row)
	{
		row_start = min(row_start, wet_dry_bounds->fp_vol[j + 1].start);   // 下一行干湿边界的开始
		row_end = max(row_end, wet_dry_bounds->fp_vol[j + 1].end);      // 下一行干湿边界的结束
	}

	//ensure row_start, row_end are not out of bounds 确保每行的row_start, row_end在边界内
	row_start = max(wet_dry_bounds->dem_data[j].start, row_start);
	row_end = min(row_end, wet_dry_bounds->dem_data[j].end);

	// update bounds for subsequent updates
	//wet_dry_bounds->fp_vol[j].start = row_start;
	//wet_dry_bounds->fp_vol[j].end = row_end;
	// xiaodw 修改，暂时设为与dem计算范围一致，保证每个点都能计算到
	wet_dry_bounds->fp_vol[j].start = wet_dry_bounds->dem_data[j].start;
	wet_dry_bounds->fp_vol[j].end = wet_dry_bounds->dem_data[j].end;

	//__assume(row_start % GRID_ALIGN_WIDTH == 0);
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_cols_padded % GRID_ALIGN_WIDTH == 0);
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);

#endif
#ifdef __INTEL_COMPILER
	__assume_aligned(delta_volume_row, 64);
	__assume_aligned(Qx_grid, 64);
	__assume_aligned(Qy_grid, 64);
	__assume_aligned(volume_grid, 64);
#endif

	// 遍历每行干湿边界内的单元
	// 栅格单元上的水量 + 流量变化 + 降雨 - 蒸发 - 下渗
#pragma ivdep
#pragma simd
	for (int i = row_start; i < row_end; i++)
	{
		int index = grid_row_index + i;
		int index_right = index + 1;
		int index_below = index + (grid_cols_padded);
		int source_index_this = j * Parptr->xsz + i;

		if (Arrptr->DEM[source_index_this] != DEM_NO_DATA) {
			NUMERIC_TYPE dV = delta_time * (Qx_grid[index] - Qx_grid[index_right] + Qy_grid[index] - Qy_grid[index_below]); // compute volume change in cell
			// xdw add, 记录流量------------------------???
			/*Q_Row[i] = dV;*/
			if (Statesptr->save_poi)
			{
				//  计算 格子上的 流量变化
				//  Poisptr->Qx_Grid[grid_row_index + i] = FABS(Qx_grid[grid_row_index + i]) + FABS(Qy_grid[grid_row_index + i]);
				//  计算 格子上的 流量JING变化, 以mm形式输出
				Poisptr->Qx_Grid[index] += delta_time * (Qx_grid[index] - Qx_grid[index_right]) * 1000.0 / row_cell_area;
				Poisptr->Qy_Grid[index] += delta_time * (Qy_grid[index] - Qy_grid[index_below]) * 1000.0 / row_cell_area;
			}

			// xiaodw，如果是河道像元则统计多少水来自于河道像元本身的产流，多少水来自于河道像元的直接侧向输出，多少水来自于上游单元的地表
			if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
				// 来自洪泛区地表的水
				Parptr->surflow2ChPD[index] = dV;
			}
			if (dV != C(0.0))
			{
				NUMERIC_TYPE V = volume_grid[index] + dV; // volume_grid是栅格单元上的水量
				volume_grid[index] = V;
				//delta_volume_row[i] = volume_grid[index];
				//h_grid[index] = V / row_cell_area;
			}

		}
	}
}

inline NUMERIC_TYPE SGC2_Infil_UpdateVol_sub_grid_row(const int sg_row_start, const int cell_count, const int grid_cols, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE evap_deltaH_step, const NUMERIC_TYPE delta_time,
	const int * sg_cell_grid_index_lookup,
	const int * sg_cell_x, const int * sg_cell_y,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume,
	const NUMERIC_TYPE * sg_cell_cell_area,

	const SubGridFlowLookup * sg_cell_flow_lookup,
	const NUMERIC_TYPE * sg_flow_Q,
	//const NUMERIC_TYPE * sg_flow_ChannelRatio,
	const NUMERIC_TYPE * Qx_grid,
	const NUMERIC_TYPE * Qy_grid,
	const int * sg_cell_SGC_group,
	const NUMERIC_TYPE * sg_cell_SGC_c,
	const int * sg_cell_SGC_is_large, // constant_channel_width > 0.5*(cell_dx + cell_dy)

	NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE * h_grid,
	const NUMERIC_TYPE infil_row,
	WetDryRowBound * wet_dry_bounds,
	const SGCprams *SGCptr,
	const int SGCd8flag, const Solver *Solverptr, NUMERIC_TYPE * volume_row, const int grid_row_index, NUMERIC_TYPE* Infilt_Row)
{
	NUMERIC_TYPE row_infil_loss = C(0.0);
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(sg_row_start % GRID_ALIGN_WIDTH == 0);
#endif
	// Calc sub grid infiltration
	if (infil_row* Solverptr->SGCtmpTstep > C(0.0))
	{
		// 遍历河道
#pragma ivdep
		for (int cell_i = 0; cell_i < cell_count; cell_i++)
		{
			int cell_index = sg_row_start + cell_i;

			int grid_index = sg_cell_grid_index_lookup[cell_index];
			// 地表水深
			const NUMERIC_TYPE h_prev = h_grid[grid_index];
			// 河道所在的栅格单元面积
			const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
			// 实际蒸发量
			NUMERIC_TYPE infil_dV = C(0.0);

			NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index];
			// 上个时间步长结束时的水深加上当前时间步长的降雨量
			NUMERIC_TYPE h_old = h_prev + volume_row[grid_index - grid_row_index] / cell_area;
			// 扣除河道内的下渗
			NUMERIC_TYPE h_new = h_old - infil_row* Solverptr->SGCtmpTstep;

			// 如果河道内的水超出了河道上底之上 或 河道内有水但未超过河道上底
			if ((h_old + SGC_BankFullHeight) > 0.0)
			{

				NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
				// 扣除蒸发后，河道里的水在河道上底之下
				if (h_new < C(0.0))
				{
					// 如果蒸发前河道内的水超过了河道上底且超过了depth_thresh，
					// 则之前的SGC2_Infil_floodplain_row_constant方法中已经扣除过一次蒸发了，先令实际蒸发量infil_dV=地表水深*河道所在栅格单元面积，相当于undo之前的操作
					if (h_old > 0.0)
					{
						infil_dV = h_old * cell_area;
					}
					// 河道内的总水深
					h_old += SGC_BankFullHeight;
					// 河道内的新总水深 = 河道内的总水深 - 下渗深度
					h_new = h_old - infil_row * Solverptr->SGCtmpTstep;

					//ensure evapouration doesn't remove water that isn't there
					// 保证新水深不能为负值
					if (h_new < C(0.0))
					{
						h_new = C(0.0);
					}

					int gr = sg_cell_SGC_group[cell_index]; // channel group number
					// SGC河道单元底面积
					NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];
					// sub-grid channel evap or transition evap
					// 如果河道水低于河道上表面 或 subgrid很大
					if (h_old < SGC_BankFullHeight || sg_cell_SGC_is_large[cell_index])
					{
						// calculate loss in vol
						// 能进入这个if，则infil_dV这时还等于0
						// 蒸发量 = （新水深 - 旧水深）*sgc河道单元底面积，infil_dV必然<0
						infil_dV -= SGC2_CalcUpV(h_old, SGC_c, gr, SGCptr); //Calculate channel volume 
						infil_dV += SGC2_CalcUpV(h_new, SGC_c, gr, SGCptr); //Calculate channel volume 
						Infilt_Row[grid_index - grid_row_index] = -infil_dV / SGC_c;
					}
					// old water level must be above bank height and the channel is smaller than a cell width
					// but the new water level is below bank height, evap mass loss for bank transition
					// 如果河道水高于河道上表面 且 河道宽度很小
					else
					{
						// infil_dV到这里时，等于h_old * cell_area，因为之前在***方法中已经将河道所在栅格的蒸发量扣除过，所以这里相当于undo之前的操作
						// 蒸发量 = floodplain之上的水深*河道所在栅格单元面积 - （新总水深-河道深度）*河道subgird底面积
						// mass lost from channel
						infil_dV -= SGC2_CalcUpV(SGC_BankFullHeight, SGC_c, gr, SGCptr); //Calculate bankfull area 河道内，上底之下的体积
						infil_dV += SGC2_CalcUpV(h_new, SGC_c, gr, SGCptr); //Calculate channel area 新总水深
						// mass lost from floodplain
						// 河水本来高出河道上底部分的水深
						NUMERIC_TYPE cell_evap = h_old - SGC_BankFullHeight;
						// floodplain上的下渗量，
						infil_dV -= (cell_evap * cell_area);
						Infilt_Row[grid_index - grid_row_index] = h_old - h_new;
					}
				}
				// 扣除蒸发前，河水就低于河道上底depth_thresh
				//else if (h_old <= depth_thresh)
				//{
				//	// normal flood plain evap - for the region between depth_thresh and zero.
				//	// this cell would have been skipped by the flood plain calculation, as the depth is below depth_thresh
				//	// evepouration needs to be calculated for this cell to allow for evapouration from the channel
				//	// 
				//	infil_dV -= (infil_row* Solverptr->SGCtmpTstep * cell_area);

				//} 
			}

			row_infil_loss -= infil_dV;
			// 从水量中扣除实际下渗量
			volume_row[grid_index - grid_row_index] += infil_dV;
			
			//volume_grid[grid_index] += infil_dV;
		}
	}
	return row_infil_loss;
}

inline NUMERIC_TYPE SGC2_Freeze_UpdateVol_sub_grid_row(const int sg_row_start, const int cell_count, const int grid_cols, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE evap_deltaH_step, const NUMERIC_TYPE delta_time,
	const int * sg_cell_grid_index_lookup,
	const int * sg_cell_x, const int * sg_cell_y,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume,
	const NUMERIC_TYPE * sg_cell_cell_area,

	const SubGridFlowLookup * sg_cell_flow_lookup,
	const NUMERIC_TYPE * sg_flow_Q,
	//const NUMERIC_TYPE * sg_flow_ChannelRatio,
	const NUMERIC_TYPE * Qx_grid,
	const NUMERIC_TYPE * Qy_grid,
	const int * sg_cell_SGC_group,
	const NUMERIC_TYPE * sg_cell_SGC_c,
	const int * sg_cell_SGC_is_large, // constant_channel_width > 0.5*(cell_dx + cell_dy)

	NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE * h_grid,
	const NUMERIC_TYPE freeze_row,
	WetDryRowBound * wet_dry_bounds,
	const SGCprams *SGCptr,
	const int SGCd8flag, const Solver *Solverptr, NUMERIC_TYPE * volume_row, 
	const int grid_row_index, NUMERIC_TYPE* Freeze_Row, NUMERIC_TYPE * snow)
{
	
	NUMERIC_TYPE row_freeze_loss = C(0.0);
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(sg_row_start % GRID_ALIGN_WIDTH == 0);
#endif
	// Calc sub grid infiltration
	if (freeze_row* Solverptr->SGCtmpTstep > C(0.0))
	{
		// 遍历河道
#pragma ivdep
		for (int cell_i = 0; cell_i < cell_count; cell_i++)
		{
			int cell_index = sg_row_start + cell_i;

			int grid_index = sg_cell_grid_index_lookup[cell_index];
			// 地表水深
			const NUMERIC_TYPE h_prev = h_grid[grid_index];
			// 河道所在的栅格单元面积
			const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
			// 实际冻结量
			NUMERIC_TYPE freeze_dV = C(0.0);

			NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index];
			// 上个时间步长结束时的水深加上当前时间步长的降雨量-下渗量
			NUMERIC_TYPE h_old = h_prev + volume_row[grid_index - grid_row_index] / cell_area;
			// 扣除冻结后的水深
			NUMERIC_TYPE h_new = h_old - freeze_row * Solverptr->SGCtmpTstep;

			// 如果河道内的水超出了河道上底之上 或 河道内有水但未超过河道上底
			if ((h_old + SGC_BankFullHeight) > 0.0)
			{

				NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
				// 扣除蒸发后，河道里的水在河道上底之下
				if (h_new < C(0.0))
				{
					// 如果蒸发前河道内的水超过了河道上底且超过了depth_thresh，
					// 则之前的SGC2_Infil_floodplain_row_constant方法中已经扣除过一次蒸发了，先令实际蒸发量infil_dV=地表水深*河道所在栅格单元面积，相当于undo之前的操作
					if (h_old > 0.0)
					{
						freeze_dV = h_old * cell_area;
					}
					// 河道内的总水深
					h_old += SGC_BankFullHeight;
					// 河道内的新总水深 = 河道内的总水深 - 下渗深度
					h_new = h_old - freeze_row * Solverptr->SGCtmpTstep;

					//ensure evapouration doesn't remove water that isn't there
					// 保证新水深不能为负值
					if (h_new < C(0.0))
					{
						h_new = C(0.0);
					}

					int gr = sg_cell_SGC_group[cell_index]; // channel group number
					// SGC河道单元底面积
					NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];
					// sub-grid channel evap or transition evap
					// 如果河道水低于河道上表面 或 subgrid很大
					if (h_old < SGC_BankFullHeight || sg_cell_SGC_is_large[cell_index])
					{
						// calculate loss in vol
						// 能进入这个if，则infil_dV这时还等于0
						// 蒸发量 = （新水深 - 旧水深）*sgc河道单元底面积，infil_dV必然<0
						freeze_dV -= SGC2_CalcUpV(h_old, SGC_c, gr, SGCptr); //Calculate channel volume 
						freeze_dV += SGC2_CalcUpV(h_new, SGC_c, gr, SGCptr); //Calculate channel volume 
						Freeze_Row[grid_index - grid_row_index] = -freeze_dV / SGC_c;
					}
					// old water level must be above bank height and the channel is smaller than a cell width
					// but the new water level is below bank height, evap mass loss for bank transition
					// 如果河道水高于河道上表面 且 河道宽度很小
					else
					{
						// infil_dV到这里时，等于h_old * cell_area，因为之前在***方法中已经将河道所在栅格的蒸发量扣除过，所以这里相当于undo之前的操作
						// 蒸发量 = floodplain之上的水深*河道所在栅格单元面积 - （新总水深-河道深度）*河道subgird底面积
						// mass lost from channel
						freeze_dV -= SGC2_CalcUpV(SGC_BankFullHeight, SGC_c, gr, SGCptr); //Calculate bankfull area 河道内，上底之下的体积
						freeze_dV += SGC2_CalcUpV(h_new, SGC_c, gr, SGCptr); //Calculate channel area 新总水深
						// mass lost from floodplain
						// 河水本来高出河道上底部分的水深
						NUMERIC_TYPE cell_freeze = h_old - SGC_BankFullHeight;
						// floodplain上的冻结量，
						freeze_dV -= (cell_freeze * cell_area);
						Freeze_Row[grid_index - grid_row_index] = h_old - h_new;
						
					}
				}
				// 扣除蒸发前，河水就低于河道上底depth_thresh
				//else if (h_old <= depth_thresh)
				//{
				//	// normal flood plain evap - for the region between depth_thresh and zero.
				//	// this cell would have been skipped by the flood plain calculation, as the depth is below depth_thresh
				//	// evepouration needs to be calculated for this cell to allow for evapouration from the channel
				//	// 
				//	infil_dV -= (infil_row* Solverptr->SGCtmpTstep * cell_area);

				//} 
			}

			row_freeze_loss -= freeze_dV;
			// 从水量中扣除冻结量
			volume_row[grid_index - grid_row_index] += freeze_dV;
			// 积雪加上冻结量，freeze_dV是个负值，所以要减去
			snow[grid_index] -= freeze_dV;

			//volume_grid[grid_index] += infil_dV;
		}
	}
	return row_freeze_loss;
}

inline NUMERIC_TYPE SGC2_Evap_UpdateVol_sub_grid_row(const int sg_row_start, const int cell_count, const int grid_cols, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE evap_deltaH_step, const NUMERIC_TYPE delta_time,
	const int * sg_cell_grid_index_lookup,
	const int * sg_cell_x, const int * sg_cell_y,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight, const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume,
	const NUMERIC_TYPE * sg_cell_cell_area,

	const SubGridFlowLookup * sg_cell_flow_lookup,
	const NUMERIC_TYPE * sg_flow_Q,
	//const NUMERIC_TYPE * sg_flow_ChannelRatio,
	const NUMERIC_TYPE * Qx_grid,
	const NUMERIC_TYPE * Qy_grid,
	const int * sg_cell_SGC_group,
	const NUMERIC_TYPE * sg_cell_SGC_c,
	const int * sg_cell_SGC_is_large, // constant_channel_width > 0.5*(cell_dx + cell_dy)

	NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE * h_grid,
	NetCDFVariable * evap_grid,
	WetDryRowBound * wet_dry_bounds,
	const SGCprams *SGCptr,
	const int SGCd8flag, NUMERIC_TYPE * Q_Ch_POI, const States *Statesptr)
{
	NUMERIC_TYPE row_evap_loss = C(0.0);
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(sg_row_start % GRID_ALIGN_WIDTH == 0);
#endif
#ifdef __INTEL_COMPILER
	__assume_aligned(sg_cell_flow_lookup, 64);
	__assume_aligned(sg_cell_SGC_BankFullHeight, 64);
	__assume_aligned(sg_cell_grid_index_lookup, 64);
	__assume_aligned(sg_cell_SGC_BankFullVolume, 64);
	__assume_aligned(sg_cell_x, 64);
	__assume_aligned(sg_cell_y, 64);
	__assume_aligned(sg_cell_SGC_group, 64);
	__assume_aligned(sg_cell_SGC_c, 64);
	__assume_aligned(sg_cell_cell_area, 64);
	__assume_aligned(sg_cell_SGC_is_large, 64);
	__assume_aligned(sg_flow_Q, 64);

	__assume_aligned(h_grid, 64);
	__assume_aligned(volume_grid, 64);
	__assume_aligned(Qx_grid, 64);
	__assume_aligned(Qy_grid, 64);
#endif
	// 把原有的根据蒸发更新河道水量的代码注释掉
	/*
	// Calc sub grid evaporation
	if (evap_deltaH_step > C(0.0))
	{
#pragma ivdep
		for (int cell_i = 0; cell_i < cell_count; cell_i++)
		{
			int cell_index = sg_row_start + cell_i;

			int grid_index = sg_cell_grid_index_lookup[cell_index];
			// 上个时间步长结束时的水深
			const NUMERIC_TYPE h_prev = h_grid[grid_index];
			// // 河道所在的栅格单元面积
			const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
			// 实际蒸发量
			NUMERIC_TYPE evap_dV = C(0.0);

			NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index];
			NUMERIC_TYPE h_old = h_prev;
			// 扣除河道所在栅格蒸发后的水深
			NUMERIC_TYPE h_new = h_old - evap_grid->data[grid_index];
			// if there is water to evapourate: h_old + SGC_BankFullHeight) > depth_thresh
			// and the new height is below the flood plain h_new < 0 then need to update
			// note - if no water or water is still above the flood plain (h_new >= 0) then the flood plain calculation is correct
			// 如果河道里之前本就没有水，或者扣除完蒸发后水仍然高于河道上底，则之前洪泛区下渗方法中的计算是正确的，这里不需要再校准
			// 如果河道内的水本来超出了河道上底之上且扣除蒸发后位于上底之下 或 河道内有水但未超过河道上底

			// 河道里本来有水
			if ((h_old + SGC_BankFullHeight) > depth_thresh)
			{
				
				NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
				// 扣除蒸发后，河道里的水在河道上底之下
				if (h_new < C(0.0))
				{
					// this is a sub-grid channel negative height allowed
					// therefore undo the flood plain calculation (which truncated to zero)
					// 如果蒸发前河道内的水超过了河道上底且超过了depth_thresh，
					// 则之前的洪泛区蒸发方法中已经计算过河道所在洪泛区上蒸发，
					// 先计算出河道上底之上的蒸发体积infil_dV=地表水深*河道所在栅格单元面积，相当于重复之前的计算操作
					if (h_old > depth_thresh)
						//if (h_old > C(0.0))
					{
						evap_dV = h_old * cell_area;
					}
					// 河道内的总水深
					h_old += SGC_BankFullHeight;
					// 河道内的新总水深 = 河道内的总水深 - 蒸发深度
					h_new = h_old - evap_grid->data[grid_index];

					//ensure evapouration doesn't remove water that isn't there
					// 保证新水深不能为负值
					if (h_new < C(0.0))
					{
						h_new = C(0.0);
					}

					int gr = sg_cell_SGC_group[cell_index]; // channel group number
					// SGC河道单元底面积
					NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];
					// sub-grid channel evap or transition evap
					// 如果扣除蒸发之前，河道水就低于河道上表面 或 subgrid很大，则实际蒸发量=河道内蒸发前的水体积 - 河道内蒸发后的水体积
					if (h_old < SGC_BankFullHeight || sg_cell_SGC_is_large[cell_index])
					{
						// calculate loss in vol
						// 能进入这个if，则evap_dV这时还等于0
						// 蒸发量 = （新水深 - 旧水深）*sgc河道单元底面积，evap_dV必然<0
						evap_dV -= SGC2_CalcUpV(h_old, SGC_c, gr, SGCptr); //Calculate channel volume 
						evap_dV += SGC2_CalcUpV(h_new, SGC_c, gr, SGCptr); //Calculate channel volume 
					}
					// old water level must be above bank height and the channel is smaller than a cell width
					// but the new water level is below bank height, evap mass loss for bank transition
					//  如果扣除蒸发之前，河道水就高于河道上表面 且 河道宽度很小
					else
					{
						// infil_dV到这里时，等于h_old * cell_area，因为之前在***方法中已经将河道所在栅格的蒸发量扣除过，所以这里相当于undo之前的操作
						// 蒸发量 = floodplain之上的水深*河道所在栅格单元面积 - （新总水深-河道深度）*河道subgird底面积
						// mass lost from channel
						evap_dV -= SGC2_CalcUpV(SGC_BankFullHeight, SGC_c, gr, SGCptr); //Calculate bankfull area 河道内，上底之下的体积
						evap_dV += SGC2_CalcUpV(h_new, SGC_c, gr, SGCptr); //Calculate channel area 新总水深
						// mass lost from floodplain
						// 河水本来高出河道上底部分的水深
						NUMERIC_TYPE cell_evap = h_old - SGC_BankFullHeight;
						// 把之前floodplain方法中扣除的蒸发量补回来
						evap_dV -= (cell_evap * cell_area);
					}
				}
				// 扣除蒸发后，河道里的水在河道上底之上，且扣除蒸发前，河水只超出河道上底介于0~depth_thresh，
				// 则之前洪泛区蒸发方法中没有扣除过这部分
				else if (h_old <= depth_thresh)
				{
					// normal flood plain evap - for the region between depth_thresh and zero.
					// this cell would have been skipped by the flood plain calculation, as the depth is below depth_thresh
					// evepouration needs to be calculated for this cell to allow for evapouration from the channel
					// 
					evap_dV -= (evap_grid->data[grid_index] * cell_area);

				} //else water is fully above the sub-grid, no need to update here
			}

			//dV = SGC2_Evaporation_SubGridCell(cell_index, grid_index, evap_deltaH_step, depth_thresh, sub_grid_cell_info, sub_grid_state, SGCptr);
			row_evap_loss -= evap_dV;
			//Cell_V[cell_index] = dV;
			//volume_grid[grid_index] += dV * delta_time;
			//dV += infil_dV;
			// 从水量中扣除实际蒸发量
			volume_grid[grid_index] += evap_dV;
		}
	}
	*/


	// PFU separate loop for subgrid flow_dV (needed if evap=0)
	#pragma ivdep
	for (int cell_i = 0; cell_i < cell_count; cell_i++)
	{
		int cell_index = sg_row_start + cell_i;

		int grid_index = sg_cell_grid_index_lookup[cell_index];

		SubGridFlowLookup sg_cell_flow_lookup_item = sg_cell_flow_lookup[cell_index];
		NUMERIC_TYPE flow_dV = C(0.0);

		// PFU use the subgrid flow add and sub variables to work out
		// how the subgrid flow contributes to the volume change
		// Replaces code in ProcessSubGridQBlock where channelQ was added to Q_grid

		// 0 qx (west side flows inward: add)
		int flow_index = sg_cell_flow_lookup_item.flow_add[0];
		//This adds the subgrid flow to the cell volume
		flow_dV += (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0); 
		
		// 0 qx (east side flows outward: subtract)
		flow_index = sg_cell_flow_lookup_item.flow_subtract[0];
		flow_dV -= (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);

		// 1 qy (north side flows inward: add)
		flow_index = sg_cell_flow_lookup_item.flow_add[1];
		flow_dV += (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);

		// 1 qy (south side flows outward: subtract)
		flow_index = sg_cell_flow_lookup_item.flow_subtract[1];
		flow_dV -= (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);

		// PFU: d8 flows
		if (SGCd8flag)
		{
			// 2 north west corner flows inward
			flow_index = sg_cell_flow_lookup_item.flow_add[2];
			flow_dV += (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);
			// 2 south east corner flows outward
			flow_index = sg_cell_flow_lookup_item.flow_subtract[2];
			flow_dV -= (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);

			// 3 north east corner flows inward
			flow_index = sg_cell_flow_lookup_item.flow_add[3];
			flow_dV += (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);
			// 3 south west corner flows outward
			flow_index = sg_cell_flow_lookup_item.flow_subtract[3];
			flow_dV -= (flow_index != -1) ? sg_flow_Q[flow_index] : C(0.0);
		}

		// PFU: removed channel ratio adjustment, now using adjusted floodplain width in Q_grid calculation

		//printf("Flow_dV: %" NUM_FMT,flow_dV);
		if (Statesptr->save_poi == ON)
		{
			// 上个时间步长结束时的水深
			const NUMERIC_TYPE h_old = h_grid[grid_index];
			// // 河道所在的栅格单元面积
			const NUMERIC_TYPE cell_area = sg_cell_cell_area[cell_index];
			// 实际蒸发量
			NUMERIC_TYPE evap_dV = C(0.0);
			// SGC河道单元底面积
			NUMERIC_TYPE SGC_c = sg_cell_SGC_c[cell_index];
			NUMERIC_TYPE SGC_BankFullHeight = sg_cell_SGC_BankFullHeight[cell_index];

			if (Statesptr->save_poi == ON)
			{
				if (h_old < 0.0)
				{
					Q_Ch_POI[grid_index] += flow_dV * delta_time * 1000.0 / SGC_c;   //m3 -> mm
				}
				else {
					Q_Ch_POI[grid_index] += flow_dV * delta_time * 1000.0 / cell_area;   //m3 -> mm
				}
			}

		}
		flow_dV *= delta_time;
		volume_grid[grid_index] += flow_dV;
	}
	return row_evap_loss;
}
// 根据干湿边界内每个单元上的水量更新水深
inline NUMERIC_TYPE SGC2_ProcessH_Row(const int j, const int grid_cols, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE row_cell_area,
	const NUMERIC_TYPE sg_row_mem_size, const int sg_cell_row_count,
	NUMERIC_TYPE * h_grid, NUMERIC_TYPE * volume_grid,
	const int * sg_cell_x, const int * sg_cell_y,
	const int * sg_cell_grid_index_lookup,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight,
	const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume,
	const NUMERIC_TYPE * sg_cell_cell_area,
	const int * sg_cell_SGC_group,
	const NUMERIC_TYPE * sg_cell_SGC_c,
	const int * sg_cell_SGC_is_large,
	WetDryRowBound * wet_dry_bounds,
	const SGCprams * SGCptr, const NUMERIC_TYPE *dem_grid, Pois *Poisptr)
{
	int index, grid_row_index;
	NUMERIC_TYPE row_Hmax = C(0.0);

	int fp_h_start = grid_cols;
	int fp_h_end = -1;

	int row_start = wet_dry_bounds->fp_vol[j].start;
	int row_end = wet_dry_bounds->fp_vol[j].end;

	//row_start = max(wet_dry_bounds->dem_data[j].start, row_start);
	//row_end = min(row_end, wet_dry_bounds->dem_data[j].end);

	grid_row_index = j * grid_cols_padded;

#ifdef __INTEL_COMPILER
	__assume_aligned(h_grid, 64);
	__assume_aligned(volume_grid, 64);
	__assume_aligned(sg_cell_x, 64);
	__assume_aligned(sg_cell_y, 64);
	__assume_aligned(sg_cell_grid_index_lookup, 64);
	__assume_aligned(sg_cell_SGC_BankFullHeight, 64);
	__assume_aligned(sg_cell_SGC_BankFullVolume, 64);
	__assume_aligned(sg_cell_cell_area, 64);
	__assume_aligned(sg_cell_SGC_group, 64);
	__assume_aligned(sg_cell_SGC_c, 64);
	__assume_aligned(sg_cell_SGC_is_large, 64);
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(grid_cols_padded % GRID_ALIGN_WIDTH == 0);
	__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
#endif

#pragma ivdep
	for (int i = row_start; i < row_end; i++)
	{
		index = grid_row_index + i;
		if (dem_grid[index] != DEM_NO_DATA) {
			NUMERIC_TYPE h;
			if (volume_grid[index] >= C(0.0))
			{
				// 水深 = 体积 / 面积
				h = volume_grid[index] / row_cell_area;
			}
			else
			{
#if defined (_DEBUG) && _DEBUG > 1
				//printf("Volume <0 %" NUM_FMT" (%d,%d) \n", volume_grid[index], i, lyr);
#endif
				h = C(0.0);
				volume_grid[index] = C(0.0);
			}
			// 更新地表水深
			h_grid[index] = h;
			// 水量
			Poisptr->Vol_Grid[index] = volume_grid[index];
		}
		
	}

	const int sg_row_start = j * sg_row_mem_size;//  sub_grid_layout->row_cols_padded;
	const int cell_end = sg_cell_row_count;//  sub_grid_layout->cell_row_count[lyr];
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
	__assume(sg_row_start % GRID_ALIGN_WIDTH == 0);
#endif
#pragma ivdep
	for (int cell_i = 0; cell_i < cell_end; cell_i++)
	{
		const int cell_index = sg_row_start + cell_i;
		//int x = sg_cell_x[cell_index];
		//int y = sg_cell_y[cell_index];
		const int grid_index = sg_cell_grid_index_lookup[cell_index];// x + y * grid_cols_padded;

		const NUMERIC_TYPE V = volume_grid[grid_index];
		NUMERIC_TYPE h;
		// 如果地表水体积 > 河道体积，水漫出河道
		if (V >= sg_cell_SGC_BankFullVolume[cell_index] && !sg_cell_SGC_is_large[cell_index])// there is a sub-grid channel above it bank
		{
			// use V as the volume of water above the flood plain
			// simple h calculation possible
			//h = V / sg_cell_cell_area[cell_index];
			// 如果超出河道上底的地表水深 > depth_thresh,就更新干湿边界
			h = (V - sg_cell_SGC_BankFullVolume[cell_index]) / row_cell_area;
			if (h > depth_thresh)
			{
				// note this check and lock is only required when not processing row per thread
				NUMERIC_TYPE old_h = h_grid[grid_index]; // check if the most up to date h is below the depth_threshhold
				// 如果旧地表水深 < depth_thresh
				if (old_h < depth_thresh)
				{
					//int y = sg_cell_y[cell_index];
					int x = sg_cell_x[cell_index];

					// note this should be rare as only occurrs when a sub grid flow first overflows onto the floodplain
					{
						// flow onto flood plain update flood plain wet/dry
						row_start = min(row_start, x);
						row_end = max(row_end, x + 1);
						//wet_dry_bounds->fp_vol[y].start = min(wet_dry_bounds->fp_vol[y].start, x);
						//wet_dry_bounds->fp_vol[y].end = max(wet_dry_bounds->fp_vol[y].end, x + 1);
					}
				}

			}
		}
		// 水没有漫出河道
		else if (V > C(0.0)) // there is a sub-grid channel and its within bank
		{
			int channel_group = sg_cell_SGC_group[cell_index];
			// h = 水体积 / 底面积
			h = SGC2_CalcUpH(V, sg_cell_SGC_c[cell_index], channel_group, SGCptr);
			h -= sg_cell_SGC_BankFullHeight[cell_index];
		}
		else // V < 0 没有水
		{
			//printf("check negative volume (sub grid) %" NUM_FMT"\n", V);
			h = -sg_cell_SGC_BankFullHeight[cell_index];
		}

		h_grid[grid_index] = h;

		h += sg_cell_SGC_BankFullHeight[cell_index];

		if (h > row_Hmax)
			row_Hmax = h;
	}

	row_start = wet_dry_bounds->fp_vol[j].start;
	row_end = wet_dry_bounds->fp_vol[j].end;

	row_start = max(wet_dry_bounds->dem_data[j].start, row_start);
	row_end = min(row_end, wet_dry_bounds->dem_data[j].end);


#pragma ivdep
	for (int i = row_start; i < row_end; i++)
	{
		index = grid_row_index + i;

		NUMERIC_TYPE h = h_grid[index];
		if (h > row_Hmax)
			row_Hmax = h;
		// update wet/dry boundary
		//if (h_grid[index] > C(0.0) /*depth_thresh*/)
		// 将干湿边界更新为地表水深>depth_thresh
		if (h > depth_thresh)
		{
			if (i < fp_h_start)
				fp_h_start = i;
			if (i > fp_h_end)
				fp_h_end = i;
		}
	}

	wet_dry_bounds->fp_h_prev[j].start = wet_dry_bounds->fp_h[j].start;
	wet_dry_bounds->fp_h_prev[j].end = wet_dry_bounds->fp_h[j].end;
	wet_dry_bounds->fp_h[j].start = fp_h_start;
	if (fp_h_end == -1)
	{
		wet_dry_bounds->fp_h[j].end = fp_h_end;
	}
	else
	{
		wet_dry_bounds->fp_h[j].end = min(fp_h_end + 1, wet_dry_bounds->dem_data[j].end);
	}
	return row_Hmax;
}

void SGC2_UpdateHazard_row(const int j, const int grid_row_index,
	const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE * Vx_grid, const NUMERIC_TYPE * Vy_grid,
	const NUMERIC_TYPE * h_grid, const NUMERIC_TYPE * dem_grid,
	const NUMERIC_TYPE row_dx, const NUMERIC_TYPE row_dy,
	const NUMERIC_TYPE *SGC_BankFullHeight_grid,
	NUMERIC_TYPE *maxVc_grid, NUMERIC_TYPE *maxVc_height_grid, NUMERIC_TYPE *maxHazard_grid,
	WetDryRowBound *wet_dry_bounds)
{
	bool not_last_row = (j < grid_rows - 1);
	// flow from next cell, previous row, next row
	int row_start = wet_dry_bounds->fp_vol[j].start - 1;
	int row_end = wet_dry_bounds->fp_vol[j].end + 1;
	if (j > 0)
	{
		row_start = min(row_start, wet_dry_bounds->fp_vol[j - 1].start);
		row_end = max(row_end, wet_dry_bounds->fp_vol[j - 1].end);
	}
	if (not_last_row)
	{
		row_start = min(row_start, wet_dry_bounds->fp_vol[j + 1].start);
		row_end = max(row_end, wet_dry_bounds->fp_vol[j + 1].end);
	}

	//ensure row_start, row_end are not out of bounds
	row_start = max(wet_dry_bounds->dem_data[j].start, row_start);
	row_end = min(row_end, wet_dry_bounds->dem_data[j].end);

	for (int i = row_start; i < row_end; i++)
	{
		int index = grid_row_index + i;

		// calculate velocities, ignore boundary velocities
		//NUMERIC_TYPE Vx_west = (i != 0) ? SGC2_CalculateVelocity(index - 1, index, Qx_grid, h_grid, dem_grid, row_dy) : C(0.0);
		//NUMERIC_TYPE Vx_east = (i < grid_cols - 1) ? SGC2_CalculateVelocity(index, index + 1, Qx_grid, h_grid, dem_grid, row_dy) : C(0.0);

		//NUMERIC_TYPE Vy_north = (lyr != 0) ? SGC2_CalculateVelocity(index - grid_cols_padded, index, Qy_grid, h_grid, dem_grid, row_dx) : C(0.0);
		//NUMERIC_TYPE Vy_south = (not_last_row) ? SGC2_CalculateVelocity(index, index + grid_cols_padded, Qy_grid, h_grid, dem_grid, row_dx) : C(0.0);
		NUMERIC_TYPE Vx_west = Vx_grid[index];
		NUMERIC_TYPE Vx_east = Vx_grid[index + 1];

		NUMERIC_TYPE Vy_north = Vy_grid[index];
		NUMERIC_TYPE Vy_south = Vy_grid[index + grid_cols_padded];

		NUMERIC_TYPE Vx = getmax(FABS(Vx_west), FABS(Vx_east));
		NUMERIC_TYPE Vy = getmax(FABS(Vy_north), FABS(Vy_south));

		NUMERIC_TYPE Vc = SQRT(Vx*Vx + Vy*Vy);
		NUMERIC_TYPE depth = h_grid[index] + SGC_BankFullHeight_grid[index];
		NUMERIC_TYPE hazard = depth * (Vc + C(1.5)); // Changed to equation from DEFRA 2006 (ALD)
		maxHazard_grid[index] = getmax(hazard, maxHazard_grid[index]);

		if (Vc > maxVc_grid[index])
		{
			maxVc_grid[index] = Vc;
			maxVc_height_grid[index] = depth;
		}
	}
}

//this function calculated the wetting front matric potential (mm)
//float CalculateCapillarySuction(float por, float clay, float sand) {
//	float cs = 10.0f * exp(6.5309f - 7.32561f * por + 0.001583f * pow(clay, 2) + 3.809479f * pow(por, 2)
//		+ 0.000344f * sand * clay - 0.049837f * por * sand
//		+ 0.001608f * pow(por, 2) * pow(sand, 2)
//		+ 0.001602f * pow(por, 2) * pow(clay, 2) - 0.0000136f * pow(sand, 2) * clay -
//		0.003479f * pow(clay, 2) * por - 0.000799f * pow(sand, 2) * por);
//
//	return cs;
//}

void POI_RAIN_COLLECT(NUMERIC_TYPE * volume_row, int j,
	const States *Statesptr, const Pars *Parptr, Pois * Poisptr, const SGCprams *SGCptr, const NUMERIC_TYPE max_Froude) {
	NUMERIC_TYPE Q_multiplier = C(1.0);
	if (Statesptr->latlong == OFF) Q_multiplier = Parptr->dx;
	NUMERIC_TYPE x, y;
	for (int i = 0; i < Poisptr->num; i++)
	{
		x = Poisptr->xpi[i];
		y = Poisptr->ypi[i];
		if (true)
		{

		}

	}
}

// DHSVM 总方法
//inline void DHSVM_Calculation() {
//	int infilt_row_start = wet_dry_bounds->dem_data[j].start;
//	int infilt_row_end = wet_dry_bounds->dem_data[j].end;
//	int grid_row_index = j * grid_cols_padded;
//
//	UnsaturatedFlow(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
//		dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid,volume_row, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);
//	RouteSubSurface(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
//		dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, volume_row, dx_col[j], dy_col[j], wet_dry_bounds, Poisptr);
//	DistributeSatflow(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
//		dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, volume_row, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);
//
//}

// 降雨、下渗、更新坡面和河道的流量
NUMERIC_TYPE SGC2_UpdateVolumeHeight_block(const int block_index,
	const int grid_cols, const int grid_rows, const int grid_cols_padded, const NUMERIC_TYPE curr_time,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE g,
	const NUMERIC_TYPE evap_deltaH_step, NetCDFVariable * evap_grid, const NUMERIC_TYPE rain_deltaH_step, const NUMERIC_TYPE snow_deltaH_step,
	const NUMERIC_TYPE * rain_grid, const NUMERIC_TYPE * dist_infil_grid,

	WetDryRowBound *wet_dry_bounds, NUMERIC_TYPE *tmp_row,
	const NUMERIC_TYPE* Qx_grid, const NUMERIC_TYPE* Qy_grid,
	const NUMERIC_TYPE * cell_area_col, const NUMERIC_TYPE * dx_col, const NUMERIC_TYPE * dy_col,
	const NUMERIC_TYPE * dem_grid, 

	const SubGridRowList * sub_grid_layout, const SubGridState * sub_grid_state,
	PointSourceRowList * ps_layout,
	const RouteDynamicList * route_dynamic_list,

	NUMERIC_TYPE * h_grid,
	NUMERIC_TYPE * volume_grid,

	const NUMERIC_TYPE * SGC_BankFullHeight_grid,
	const States *Statesptr,
	Pars *Parptr,
	Fnames *Fnameptr,
	const Solver *Solverptr,
	Pois *Poisptr, 
	const SGCprams *SGCptr, Arrays *Arrptr,
	//DynamicRain<> & dynamic_rain, removed JCN
	VolumeHeightUpdateInfo * update_info, NUMERIC_TYPE *infilAvgBlock, int * infilValidCount
	//NUMERIC_TYPE last_gw_time,NUMERIC_TYPE *PercolationVol, NUMERIC_INT  sumNCells, NUMERIC_TYPE * sumGndQ2Rch, NUMERIC_TYPE * GwStorageDepth
)
{
	NUMERIC_TYPE block_evap_loss = C(0.0);
	NUMERIC_TYPE block_infil_loss = C(0.0);
	NUMERIC_TYPE block_freeze_loss = C(0.0);
	NUMERIC_TYPE block_rain_total = C(0.0);
	NUMERIC_TYPE block_Qpoint_timestep_pos = C(0.0);
	NUMERIC_TYPE block_Qpoint_timestep_neg = C(0.0);

	NUMERIC_TYPE block_Hmax = C(0.0);

	NUMERIC_TYPE Q_multiplier = C(1.0); // for volume point sources
	// CCS Multiplier for Q inputs. If using regular grid, Qs are specified as m^2 and need to be multiplied by dx; 
	// if using lat-long Qs are specified in m^3 and therefore multiplier is C(1.) Note intialised above as C(1.0).
	if (Statesptr->latlong == OFF) Q_multiplier = Parptr->dx;

	SubGridFlowLookup * sg_cell_flow_lookup = sub_grid_layout->flow_info.sg_cell_flow_lookup;
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight = sub_grid_layout->cell_info.sg_cell_SGC_BankFullHeight;
	const int * sg_cell_grid_index_lookup = sub_grid_layout->cell_info.sg_cell_grid_index_lookup;
	const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume = sub_grid_layout->cell_info.sg_cell_SGC_BankFullVolume;
	const int * sg_cell_x = sub_grid_layout->cell_info.sg_cell_x;
	const int * sg_cell_y = sub_grid_layout->cell_info.sg_cell_y;
	const int * sg_cell_SGC_group = sub_grid_layout->cell_info.sg_cell_SGC_group;
	const NUMERIC_TYPE * sg_cell_SGC_c = sub_grid_layout->cell_info.sg_cell_SGC_c;
	const NUMERIC_TYPE * sg_cell_cell_area = sub_grid_layout->cell_info.sg_cell_cell_area;
	const int * sg_cell_SGC_is_large = sub_grid_layout->cell_info.sg_cell_SGC_is_large;

//	const NUMERIC_TYPE * sg_flow_ChannelRatio = sub_grid_state->sg_flow_ChannelRatio;
	const NUMERIC_TYPE * sg_flow_Q = sub_grid_state->sg_flow_Q;

	NUMERIC_TYPE q_pos, q_neg;

	const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
	const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;
	*infilAvgBlock = C(0.0);
	//*infilAccBlock = C(0.0);
	int countRows = 0;
	for (int j = start_y; j < end_y; j++)
	{
		const int grid_row_index = j * grid_cols_padded;
		const int grid_row_index_no_padding = j * Parptr->xsz;
		const NUMERIC_TYPE row_cell_area = cell_area_col[j];

		const int sg_row_start = j * sub_grid_layout->row_cols_padded;
		const int cell_row_count = sub_grid_layout->cell_row_count[j];
		
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
		__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
#endif
		// delta_volume_row是一维数组grid_cols_padded ，用来存储当前处理行的数据
		NUMERIC_TYPE * delta_volume_row = tmp_row;
		//NUMERIC_TYPE * delta_volume_grid_ch = tmp_row_ch;
		
		//NUMERIC_TYPE * delta_volume_row_dhsvm = new NUMERIC_TYPE[grid_cols_padded];
		//memset(delta_volume_row_dhsvm, 0, sizeof(NUMERIC_TYPE) * grid_cols_padded);
#ifdef __INTEL_COMPILER
		__assume_aligned(delta_volume_row, 64);

		__assume_aligned(h_grid, 64);
		__assume_aligned(dist_infil_grid, 64);
		__assume_aligned(volume_grid, 64);
		__assume_aligned(dem_grid, 64);
		__assume_aligned(cell_area_col, 64);
		__assume_aligned(Qx_grid, 64);
		__assume_aligned(Qy_grid, 64);

		__assume_aligned(sg_cell_flow_lookup, 64);
		__assume_aligned(sg_cell_SGC_BankFullHeight, 64);
		__assume_aligned(sg_cell_grid_index_lookup, 64);
		__assume_aligned(sg_cell_SGC_BankFullVolume, 64);
		__assume_aligned(sg_cell_x, 64);
		__assume_aligned(sg_cell_y, 64);
		__assume_aligned(sg_cell_SGC_group, 64);
		__assume_aligned(sg_cell_SGC_c, 64);
		__assume_aligned(sg_cell_cell_area, 64);
		__assume_aligned(sg_cell_SGC_is_large, 64);
//		__assume_aligned(sg_flow_ChannelRatio, 64);
		__assume_aligned(sg_flow_Q, 64);

#endif		

		SGC2_PointSources_Vol_row(j, grid_cols, delta_time, curr_time, depth_thresh, g, Q_multiplier, dx_col, dy_col, h_grid, SGCptr, delta_volume_row, ps_layout, wet_dry_bounds, &q_pos, &q_neg, Parptr->max_Froude);
		block_Qpoint_timestep_pos += q_pos;
		block_Qpoint_timestep_neg += q_neg;

		// xiaodw 20250318修订，根据流量变化更新栅格水量后，再计算入渗
		SGC2_UpdateVol_floodplain_by_Q(j, grid_row_index, grid_cols, grid_rows, grid_cols_padded, delta_time, row_cell_area, Qx_grid, Qy_grid,
			volume_grid, delta_volume_row, h_grid, wet_dry_bounds, Poisptr->Qx_Grid + grid_row_index, Poisptr->Vol_Grid + grid_row_index, Parptr, Arrptr, Poisptr, Statesptr);
		// xiaodw 20250318修订，根据流量变化更新栅格水深
		SGC2_ProcessH_Row(j, grid_cols, grid_cols_padded, depth_thresh, row_cell_area,
			sub_grid_layout->row_cols_padded, sub_grid_layout->cell_row_count[j],
			h_grid, volume_grid, sg_cell_x, sg_cell_y, sg_cell_grid_index_lookup,
			sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume, sg_cell_cell_area, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large, wet_dry_bounds, SGCptr, dem_grid, Poisptr);
		// xdw add, snowfall accumulation
		if (Statesptr->use_snow_glacier == ON)
		{
			// when there is snowfall and temperature <= threshold, add snowfall to snow storage thickness
			if (snow_deltaH_step > 0.0 & temperature_step <= Parptr->melt_temperature)
			{
				// add snow fall to snow storage directly
				const NUMERIC_TYPE snow_step_dV = snow_deltaH_step * row_cell_area;
				block_evap_loss -= SGC2_Snowfall_row(j, snow_step_dV, row_cell_area,
					dem_grid + grid_row_index, // pointer to start of row
					Parptr->snow + grid_row_index_no_padding, wet_dry_bounds, Statesptr, Poisptr->Snow_Grid + grid_row_index);
			}


		}

		// rainfall
		if ((Statesptr->dynamicrainfall == ON || Statesptr->rainfallmask == ON) && wet_dry_bounds->dem_data[j].start > -1)
		{
			block_rain_total += SGC2_Distrubuted_Rainfall_row(j, delta_time,
				dem_grid + grid_row_index, rain_grid + grid_row_index, // pointer to start of row
				delta_volume_row, Parptr->delta_volume_grid_ch + grid_row_index, wet_dry_bounds, Statesptr, row_cell_area, Poisptr->Rain_Grid + grid_row_index,
				Parptr, Solverptr, Arrptr, grid_row_index, sub_grid_layout->cell_row_count[j], sg_row_start, sg_cell_grid_index_lookup, sg_cell_cell_area, sg_cell_SGC_BankFullHeight,
				sg_cell_SGC_BankFullVolume, sg_cell_SGC_c, sg_cell_flow_lookup);
		}
		else if (rain_deltaH_step > C(0.0) && wet_dry_bounds->dem_data[j].start > -1)
		{
			const NUMERIC_TYPE rain_step_dV = rain_deltaH_step * row_cell_area; // area constant per row => rainfall constant per row
			// 记录当前时间步长每个栅格上的降雨量
			block_rain_total += SGC2_Uniform_Rainfall_row(j, rain_step_dV, row_cell_area,
				dem_grid + grid_row_index,  // pointer to start of row
				delta_volume_row, Parptr->delta_volume_grid_ch + grid_row_index, wet_dry_bounds, Statesptr,  Poisptr->Rain_Grid + grid_row_index,
				Parptr, Solverptr, Arrptr, grid_row_index, sub_grid_layout->cell_row_count[j], sg_row_start, sg_cell_grid_index_lookup, sg_cell_cell_area, sg_cell_SGC_BankFullHeight,
				sg_cell_SGC_BankFullVolume, sg_cell_SGC_c, sg_cell_flow_lookup);
		}

		// snow and glacier melt
		if (Statesptr->use_snow_glacier == ON)
		{

			if (temperature_step > Parptr->melt_temperature) {
				block_evap_loss -= SGC2_Snow_Glacier_Melt_row(j, row_cell_area,
					dem_grid + grid_row_index, // pointer to start of row
					delta_volume_row, wet_dry_bounds, Statesptr, 
					Parptr, Solverptr,  temperature_step, 
					Poisptr->SnowMelt_Grid + grid_row_index, Poisptr->GlacierMelt_Grid + grid_row_index,
					Parptr->snow + grid_row_index_no_padding, Parptr->glacier + grid_row_index_no_padding);
			}
		}

		// const_cell_evap always zero if evap Evaporation
		// floodplain evaporation
		if (evap_deltaH_step > C(0.0))
		{
			//int evap_row_start = wet_dry_bounds->fp_h[j].start;
			//int evap_row_end = min(wet_dry_bounds->fp_h[j].end, wet_dry_bounds->dem_data[j].end);
			int evap_row_start = wet_dry_bounds->dem_data[j].start;
			int evap_row_end = wet_dry_bounds->dem_data[j].end;
			if (Statesptr->use_percolation_singlelayer == ON || Statesptr->use_interflow_singlelayer == ON || Statesptr->use_green_ampt_singlelayer == ON)
			{
				block_evap_loss += SGC2_Evaporation_floodplain_row(Statesptr,evap_row_start, evap_row_end, depth_thresh, row_cell_area, evap_deltaH_step,
					evap_grid->data + grid_row_index,h_grid + grid_row_index, // pointer to start of row
					delta_volume_row, Parptr->soilWaterDepthPD + grid_row_index, Parptr->multi_soilMoisturePD[0] + grid_row_index, 
					Parptr->multi_soilThicknessPD[0] + grid_row_index, grid_row_index, Poisptr->Evap_Grid + grid_row_index, Poisptr->soil_water_depth_Grid[0] + grid_row_index);
			}
			else if (Statesptr->use_percolation_multilayer == ON || Statesptr->use_interflow_multilayer == ON || Statesptr->use_green_ampt_multilayer == ON)
			{
				block_evap_loss += SGC2_Evaporation_floodplain_row(Statesptr,evap_row_start, evap_row_end, depth_thresh, row_cell_area, evap_deltaH_step,
					evap_grid->data + grid_row_index, h_grid + grid_row_index, // pointer to start of row
					delta_volume_row, Parptr->multi_soilWaterDepthPD[0] + grid_row_index, Parptr->multi_soilMoisturePD[0] + grid_row_index, 
					Parptr->multi_soilThicknessPD[0] + grid_row_index, grid_row_index, Poisptr->Evap_Grid + grid_row_index, Poisptr->soil_water_depth_Grid[0] + grid_row_index);
			}
			else if (Statesptr->use_seims_aet == ON)
			{
				block_evap_loss += SGC2_Evaporation_floodplain_row_PT(Statesptr, evap_row_start, evap_row_end, depth_thresh, row_cell_area, evap_deltaH_step,
					evap_grid->data + grid_row_index, h_grid + grid_row_index, // pointer to start of row
					delta_volume_row, Parptr->multi_soilWaterDepthPD[0] + grid_row_index, Parptr->multi_soilMoisturePD[0] + grid_row_index,
					Parptr->multi_soilThicknessPD[0] + grid_row_index, grid_row_index, Poisptr->Evap_Grid + grid_row_index, Poisptr->soil_water_depth_Grid[0] + grid_row_index, Parptr);
			}
			else if (Statesptr->use_xaj_evap == ON) {
				block_evap_loss += SGC2_Evaporation_XAJ_3Layer(Statesptr,evap_row_start, evap_row_end, row_cell_area, evap_grid->data, h_grid, 
					delta_volume_row, Parptr->delta_volume_grid_ch + grid_row_index, dem_grid , grid_row_index, Poisptr->Evap_Grid, Poisptr->soil_water_depth_Grid, Parptr, Arrptr, sg_cell_cell_area, j,
					sub_grid_layout->cell_row_count[j], sg_row_start, sg_cell_grid_index_lookup, sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume, sg_cell_SGC_c, sg_cell_flow_lookup
					);
			}
			//  使用DHSVM的soil evaporation
			//if (Statesptr->use_dhsvm == ON)
			//{
			//	block_evap_loss += SoilEvaporation_DHSVM(Parptr->evapTstep,);
			//}

		}
		// xdw modify, couple green-ampt infiltration medthd
		// 下渗和降雨一样，也应该是在整个流域dem上都存在，从降水+积水中扣除下渗
		int infilt_row_start = wet_dry_bounds->dem_data[j].start;
		int infilt_row_end = wet_dry_bounds->dem_data[j].end;
		//int evap_row_start = wet_dry_bounds->dem_data[lyr].start;
		//int evap_row_end = wet_dry_bounds->dem_data[lyr].end;

		if (Statesptr->calc_distributed_infiltration == ON)
		{
			// 分布式下渗，但不是green-ampt
			block_infil_loss += SGC2_Infil_floodplain_row(infilt_row_start, infilt_row_end, depth_thresh, row_cell_area, evap_deltaH_step,
				h_grid + grid_row_index, // pointer to start of row
				dist_infil_grid + grid_row_index, // pointer to start of distributed infiltratioon grid
				delta_volume_row, Parptr, Solverptr);
		}
		else 	if (Statesptr->use_green_ampt_singlelayer == ON)
		{
			// allocate intermediate variables
			//int evap_start = evap_row_start + grid_row_index;
			//int evap_end = evap_row_end + grid_row_index;
			NUMERIC_TYPE infilAvgRow = C(0.0);
			NUMERIC_TYPE infilAccRow = C(0.0);
			block_infil_loss += SGC2_Infil_floodplain_row_green_ampt(infilt_row_start, infilt_row_end, depth_thresh, row_cell_area, evap_deltaH_step,
				h_grid + grid_row_index,dem_grid + grid_row_index,Poisptr->Infilt_Grid + grid_row_index,Parptr->soilWaterDepthPD + grid_row_index,
				//dist_infil_grid, // pointer to start of distributed infiltratioon grid
				delta_volume_row, Parptr->porosityPD, Parptr->initSoilMoisturePD, Parptr->capillarySuctionPD, Parptr->ksPD, Parptr->ks_factor,
				Parptr->accumuDepthPD, Parptr->rootDepthPD, Parptr->infilPD, Parptr->infilCapacitySurplusPD, Parptr->soilMoisturePD, Poisptr->soil_water_depth_Grid[0] + grid_row_index,
				Parptr, Solverptr, Statesptr,grid_row_index, infilAvgBlock, infilValidCount);
			//if (infilt_row_end - infilt_row_start > 0)
			//{
			//	countRows++;
			//	*infilAvgBlock += infilAvgRow;
			//	*infilAccBlock += infilAccRow;
			//}
		}
		else if (Statesptr->use_green_ampt_multilayer == ON) {
			NUMERIC_TYPE infilAvgRow = C(0.0);
			NUMERIC_TYPE infilAccRow = C(0.0);

			block_infil_loss += SGC2_Infil_floodplain_row_green_ampt(infilt_row_start, infilt_row_end, depth_thresh, row_cell_area, evap_deltaH_step,
				h_grid + grid_row_index, dem_grid + grid_row_index, Poisptr->Infilt_Grid + grid_row_index, Parptr->multi_soilWaterDepthPD[0] + grid_row_index,
				//dist_infil_grid, // pointer to start of distributed infiltratioon grid
				delta_volume_row, Parptr->multi_soilPorosityPD[0], Parptr->multi_soilInitMoisturePD[0], Parptr->capillarySuctionPD, Parptr->multi_soilKsPD[0], Parptr->ksFactorInfil,
				Parptr->accumuDepthPD, Parptr->multi_soilThicknessPD[0], Parptr->infilPD, Parptr->infilCapacitySurplusPD, Parptr->multi_soilMoisturePD[0], Poisptr->soil_water_depth_Grid[0] + grid_row_index,
				Parptr, Solverptr, Statesptr,grid_row_index, infilAvgBlock, infilValidCount);
			//block_infil_loss += SGC2_Infil_UpdateVol_sub_grid_row(sg_row_start, cell_row_count, grid_cols, grid_cols_padded,
			//	depth_thresh, evap_deltaH_step, delta_time, sg_cell_grid_index_lookup, sg_cell_x, sg_cell_y, sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume,
			//	sg_cell_cell_area, sg_cell_flow_lookup, sg_flow_Q, Qx_grid, Qy_grid, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large,
			//	volume_grid, h_grid, Parptr->InfilRate, wet_dry_bounds, SGCptr, Statesptr->SGCd8, Solverptr, delta_volume_row, grid_row_index, Poisptr->Infilt_Grid + grid_row_index);
		}
		else if (Statesptr->use_wetspa_sur_mr == ON) {
			NUMERIC_TYPE infilAvgRow = C(0.0);
			NUMERIC_TYPE infilAccRow = C(0.0);

			block_infil_loss += SGC2_Infil_floodplain_row_wetspa(infilt_row_start, infilt_row_end, depth_thresh, row_cell_area, 
				h_grid, dem_grid , Poisptr->Infilt_Grid + grid_row_index, Poisptr->InfiltCh_Grid + grid_row_index, Parptr->multi_soilWaterDepthPD[0] + grid_row_index,
				delta_volume_row, Parptr->delta_volume_grid_ch + grid_row_index, Parptr->multi_soilPorosityPD[0],  Parptr->multi_soilThicknessPD[0], Parptr->infilPD, Parptr->infilChPD, Parptr->multi_soilMoisturePD[0], Poisptr->soil_water_depth_Grid[0] + grid_row_index,
				Parptr, Solverptr, Arrptr,Statesptr, grid_row_index, infilAvgBlock, infilValidCount, j,
				sub_grid_layout->cell_row_count[j], sg_row_start, sg_cell_grid_index_lookup, sg_cell_cell_area,sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume, sg_cell_SGC_c, sg_cell_flow_lookup
				);
			
		}
		// constant value infiltration in sgc mode 
		else if (Statesptr->calc_infiltration == ON) {


			block_infil_loss += SGC2_Infil_floodplain_row_constant(infilt_row_start, infilt_row_end, depth_thresh, row_cell_area, dem_grid + grid_row_index,
				evap_deltaH_step, h_grid + grid_row_index, // pointer to start of row
				Parptr->InfilRate, // pointer to start of distributed infiltratioon grid
				Parptr->soilWaterDepth + grid_row_index,
				delta_volume_row, Parptr, Solverptr, Poisptr->Infilt_Grid + grid_row_index, grid_row_index, sg_row_start, cell_row_count, sg_cell_grid_index_lookup,
				sg_cell_SGC_BankFullHeight, sg_cell_cell_area );
			block_infil_loss += SGC2_Infil_UpdateVol_sub_grid_row(sg_row_start, cell_row_count, grid_cols, grid_cols_padded,
				depth_thresh, evap_deltaH_step, delta_time, sg_cell_grid_index_lookup, sg_cell_x, sg_cell_y, sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume,
				sg_cell_cell_area, sg_cell_flow_lookup, sg_flow_Q, Qx_grid, Qy_grid, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large,
				volume_grid, h_grid, Parptr->InfilRate, wet_dry_bounds, SGCptr, Statesptr->SGCd8, Solverptr, delta_volume_row,grid_row_index, Poisptr->Infilt_Grid + grid_row_index);
		}
		else {
			printf("Please input infiltration\infilfile\infilfile&green_ampt param in par file.");
		}

		// xiaodw, 在这里将DHSVM算出来的水量添加到delta_volume_row
		
		if (Statesptr->use_dhsvm == ON)
		{
			for (int i = infilt_row_start; i < infilt_row_end; i++)
			{
				int index = i + grid_row_index;
				if (dem_grid[index] != DEM_NO_DATA)
				{
					int source_index_this = j * Parptr->xsz + i;
					//if (Arrptr->ChanMask[source_index_this] > 0) {
					//	count++;
					//}
					// 如果该栅格是SGC河道
					if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {
						if (Parptr->satFlow2ChPD[index] > 0.0)
						{
							//channel_grid_inc_inflow(volume_grid, index, Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep);
							//channel_grid_inc_inflow(delta_volume_row, i, Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep);
							//delta_volume_row[i] += Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
							Parptr->delta_volumn_dhsvm_PD[index] += Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
							//if (index == 5148 && (int(curr_time) % (int(Parptr->gwTstep) * 10) == 0))
							//{
							//	cout << "index: " << index << "  curr_time: " << curr_time << "   add: " << Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep << " delta_volume_row: " << delta_volume_row[i] << endl;
							//}

						}

					}
					// 河道像元则将其蓄洪区的地表水全部加入河道水量
					if (Parptr->satFlow2SurfPD[index] > 0.0) {
						//volume_grid[index] += Parptr->satFlowPD[index] * cell_area_col[j] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
						//delta_volume_row[i] += Parptr->satFlow2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
						Parptr->delta_volumn_dhsvm_PD[index] += Parptr->satFlow2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
					}
					

					if (Parptr->PercExcess2SurfPD[index] > 0.0) {
						//volume_grid[index] += Parptr->satFlowPD[index] * cell_area_col[j] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
						//delta_volume_row[i] += Parptr->PercExcess2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
						Parptr->delta_volumn_dhsvm_PD[index] += Parptr->PercExcess2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
					}


					
				}
			}
		}
		

		// calculate water freeze
		if (Statesptr->use_snow_glacier == ON)
		{
			if (temperature_step <= Parptr->melt_temperature) {
				// for all the floodplain cells, when temperature < threshold and liquid water exists, water will freeze 
				int freeze_row_start = wet_dry_bounds->dem_data[j].start;
				int freeze_row_end = wet_dry_bounds->dem_data[j].end;

				block_freeze_loss += SGC2_Freeze_floodplain_row(freeze_row_start, freeze_row_end, depth_thresh, row_cell_area, 
					Parptr,Solverptr, Poisptr->Freeze_Grid + grid_row_index, temperature_step,
					h_grid + grid_row_index, // pointer to start of row
					delta_volume_row, Parptr->snow + grid_row_index_no_padding);
				NUMERIC_TYPE freeze_deltaH_rate= Parptr->FddSnow * Parptr->Frr * (temperature_step - Parptr->melt_temperature) ;
				block_evap_loss += SGC2_Freeze_UpdateVol_sub_grid_row(sg_row_start, cell_row_count, grid_cols, grid_cols_padded,
					depth_thresh, evap_deltaH_step, delta_time, sg_cell_grid_index_lookup, sg_cell_x, sg_cell_y, sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume,
					sg_cell_cell_area, sg_cell_flow_lookup, sg_flow_Q, Qx_grid, Qy_grid, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large,
					volume_grid, h_grid, freeze_deltaH_rate, wet_dry_bounds, SGCptr, Statesptr->SGCd8, Solverptr, delta_volume_row, grid_row_index, Poisptr->Freeze_Grid + grid_row_index, Parptr->snow + grid_row_index_no_padding);
			}

		}

		// 根据流量变化和降雨、蒸发、下渗更新洪泛区干湿边界内每个栅格上的水量
		// xiaodw 20250318修订，仅根据降雨、蒸发、下渗更新洪泛区干湿边界内每个栅格上的水量；根据流量变化更新栅格水量的方法是SGC2_UpdateVol_floodplain_by_Q，在前面
		//const int sg_row_start = j * sub_grid_layout->row_cols_padded;
		//const int cell_row_count = sub_grid_layout->cell_row_count[j];
		SGC2_UpdateVol_floodplain_row(j, grid_row_index, grid_cols, grid_rows, grid_cols_padded, delta_time, row_cell_area, Qx_grid, Qy_grid, 
			volume_grid, delta_volume_row, Parptr->delta_volume_grid_ch + grid_row_index, h_grid, wet_dry_bounds,Poisptr->Qx_Grid+ grid_row_index, Poisptr->Vol_Grid + grid_row_index,Parptr,Arrptr,Poisptr, Statesptr,
			cell_row_count, sg_row_start, sg_cell_grid_index_lookup);

		if (Statesptr->use_dhsvm == ON)
		{
			for (int i = infilt_row_start; i < infilt_row_end; i++)
			{
				int index = i + grid_row_index;
				Parptr->delta_volumn_dhsvm_PD[index] = 0.0;
			}
		}
#if defined (_DEBUG) && _DEBUG > 1
		for (int i = 0; i < grid_cols; i++)
		{
			if (delta_volume_row[i] != C(0.0))
				printf("delta_volume_row[%d] not 0\n", i);
		}
#endif


		// 根据蒸发量更新sub grid河道内的流量
		// sg_cell_SGC_c是河道sgc的底面积，sg_cell_cell_area是河道所在栅格单元的面积
		// note could use row_cell_area, currently looks up for each cell (SGC2_Evap_UpdateVol_sub_grid_row function can be used for single row or all rows with altered memory structure)
		block_evap_loss += SGC2_Evap_UpdateVol_sub_grid_row(sg_row_start, cell_row_count, grid_cols, grid_cols_padded,
			depth_thresh, evap_deltaH_step, delta_time, sg_cell_grid_index_lookup, sg_cell_x, sg_cell_y, sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume,
			sg_cell_cell_area, sg_cell_flow_lookup, sg_flow_Q,  Qx_grid, Qy_grid, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large,
			volume_grid, h_grid, evap_grid, wet_dry_bounds, SGCptr, Statesptr->SGCd8, Poisptr->Q_Ch,Statesptr);
		SGC2_PointSources_H_row(j, grid_cols, delta_time, curr_time, depth_thresh, cell_area_col, SGCptr, h_grid, volume_grid, ps_layout, wet_dry_bounds, &q_pos, &q_neg);
		block_Qpoint_timestep_pos += q_pos;
		block_Qpoint_timestep_neg += q_neg;
		// 根据干湿边界内每个fp和sgc河道单元上的水量更新水深，所有对volume_grid（栅格上水量）的改变在此之前实现就可以
		// delta_volume_row是volume_grid的一行，按行处理的，可以改delta_volume_row，也可以直接改volume_grid
		NUMERIC_TYPE row_Hmax = SGC2_ProcessH_Row(j, grid_cols, grid_cols_padded, depth_thresh, row_cell_area,
			sub_grid_layout->row_cols_padded, sub_grid_layout->cell_row_count[j],
			h_grid, volume_grid, sg_cell_x, sg_cell_y, sg_cell_grid_index_lookup,
			sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume, sg_cell_cell_area, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large, wet_dry_bounds, SGCptr, dem_grid, Poisptr);

		if (row_Hmax > block_Hmax)
			block_Hmax = row_Hmax;
	}
	//if (Statesptr->use_green_ampt_singlelayer == ON)
	//{
	//	*infilAvgBlock = *infilAvgBlock / countRows;
	//	*infilAccBlock = *infilAccBlock / countRows;
	//}
	update_info->infil_loss = block_infil_loss;
	update_info->evap_loss = block_evap_loss;
	update_info->rain_total = block_rain_total;
	update_info->Qpoint_timestep_pos = block_Qpoint_timestep_pos;
	update_info->Qpoint_timestep_neg = block_Qpoint_timestep_neg;

	return block_Hmax;
}



void SGC2_Inundation_block(const int block_index, const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh,
	const NUMERIC_TYPE current_time_hours,
	const NUMERIC_TYPE delta_time_hours,
	const NUMERIC_TYPE * h_grid,
	const SubGridRowList * sub_grid_layout, const SubGridState * sub_grid_state,
	WetDryRowBound* wet_dry_bounds,
	NUMERIC_TYPE * initHtm_grid,
	NUMERIC_TYPE * totalHtm_grid,
	NUMERIC_TYPE * maxH_grid,
	NUMERIC_TYPE * maxHtm_grid)
{
	const int * sg_cell_grid_index_lookup = sub_grid_layout->cell_info.sg_cell_grid_index_lookup;
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight = sub_grid_layout->cell_info.sg_cell_SGC_BankFullHeight;
	{
		const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
		const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;

		for (int j = start_y; j < end_y; j++)
		{
			int i, index, grid_row_index;
			grid_row_index = j * grid_cols_padded;

			int row_start = wet_dry_bounds->fp_h[j].start;
			int row_end = wet_dry_bounds->fp_h[j].end;

#ifdef __INTEL_COMPILER
			__assume_aligned(h_grid, 64);
			__assume_aligned(initHtm_grid, 64);
			__assume_aligned(totalHtm_grid, 64);
			__assume_aligned(maxH_grid, 64);
			__assume_aligned(maxHtm_grid, 64);
#endif
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
			__assume(grid_cols_padded % GRID_ALIGN_WIDTH == 0);
			__assume(grid_row_index % GRID_ALIGN_WIDTH == 0);
#endif

#pragma ivdep
#pragma simd
			for (int i = row_start; i < row_end; i++)
			{
				index = grid_row_index + i;
				if (h_grid[index] > depth_thresh)
				{
					if (initHtm_grid[index] == NULLVAL)
						initHtm_grid[index] = current_time_hours;

					totalHtm_grid[index] += delta_time_hours;
					// Update maximum water depths, and time of maximum (in hours)
					maxHtm_grid[index] = (h_grid[index] > maxH_grid[index]) ? current_time_hours : maxHtm_grid[index];
					maxH_grid[index] = (h_grid[index] > maxH_grid[index]) ? h_grid[index] : maxH_grid[index];
				}
			}

#ifdef __INTEL_COMPILER
			__assume_aligned(h_grid, 64);
			__assume_aligned(initHtm_grid, 64);
			__assume_aligned(totalHtm_grid, 64);
			__assume_aligned(maxH_grid, 64);
			__assume_aligned(maxHtm_grid, 64);
			__assume_aligned(sg_cell_grid_index_lookup, 64);
			__assume_aligned(sg_cell_SGC_BankFullHeight, 64);
#endif

			const int sg_row_start = j * sub_grid_layout->row_cols_padded;
			const int cell_end = sub_grid_layout->cell_row_count[j];
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
			__assume(sg_row_start % GRID_ALIGN_WIDTH == 0);
#endif
#pragma ivdep
			for (int cell_i = 0; cell_i < cell_end; cell_i++)
			{
				const int cell_index = sg_row_start + cell_i;
				const int grid_index = sg_cell_grid_index_lookup[cell_index];
				const NUMERIC_TYPE h_tmp = h_grid[grid_index] + sg_cell_SGC_BankFullHeight[cell_index];
				if (h_tmp > depth_thresh) // use depth_thresh as threshhold
					//if (depth > C(0.01)) // used to be hardcoded at 0.01 - maintain consistent output
				{
					if (initHtm_grid[grid_index] == NULLVAL)
						initHtm_grid[grid_index] = current_time_hours;
					//only add to time if it hasn't already been added by the flood plain calculation
					if (h_grid[grid_index] <= depth_thresh)
						totalHtm_grid[grid_index] += delta_time_hours;
					// Update maximum water depths, and time of maximum (in hours)
					if (h_tmp > maxH_grid[grid_index])
					{
						maxH_grid[grid_index] = h_tmp;
						maxHtm_grid[grid_index] = current_time_hours;
					}
				}
			}
		}
	}
}

NUMERIC_TYPE SGC2_InitHBounds(const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE depth_thresh,
	const SubGridRowList * sub_grid_layout,
	SubGridState * sub_grid_state,
	const NUMERIC_TYPE * cell_area_col,
	NUMERIC_TYPE * h_grid, NUMERIC_TYPE * volume_grid,
	WetDryRowBound* wet_dry_bounds, const SGCprams * SGCptr,const NUMERIC_TYPE * dem_grid, Pois *Poisptr)
{
	NUMERIC_TYPE Hmax = C(0.0);

	const int * sg_cell_x = sub_grid_layout->cell_info.sg_cell_x;
	const int * sg_cell_y = sub_grid_layout->cell_info.sg_cell_y;
	const int * sg_cell_grid_index_lookup = sub_grid_layout->cell_info.sg_cell_grid_index_lookup;
	const NUMERIC_TYPE * sg_cell_SGC_BankFullHeight = sub_grid_layout->cell_info.sg_cell_SGC_BankFullHeight;
	const NUMERIC_TYPE * sg_cell_SGC_BankFullVolume = sub_grid_layout->cell_info.sg_cell_SGC_BankFullVolume;
	const NUMERIC_TYPE * sg_cell_cell_area = sub_grid_layout->cell_info.sg_cell_cell_area;

	const NUMERIC_TYPE * sg_cell_SGC_c = sub_grid_layout->cell_info.sg_cell_SGC_c;
	const int * sg_cell_SGC_group = sub_grid_layout->cell_info.sg_cell_SGC_group;
	const int * sg_cell_SGC_is_large = sub_grid_layout->cell_info.sg_cell_SGC_is_large;

	// reduction to find the maximum h (note MS compiler OpenMP version does not support max reduction, use critical to prevent race condition)
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma omp parallel for default(shared) schedule(static)
#else
#pragma omp parallel for default(shared) reduction(max : Hmax) schedule(static)
#endif
	//for (int lyr = 0; lyr < grid_rows; lyr++)
	for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
	{
		const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
		const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;

		for (int j = start_y; j < end_y; j++)
		{
			NUMERIC_TYPE row_Hmax = SGC2_ProcessH_Row(j, grid_cols, grid_cols_padded, depth_thresh,
				cell_area_col[j],
				sub_grid_layout->row_cols_padded, sub_grid_layout->cell_row_count[j],
				h_grid, volume_grid, sg_cell_x, sg_cell_y, sg_cell_grid_index_lookup, sg_cell_SGC_BankFullHeight, sg_cell_SGC_BankFullVolume, sg_cell_cell_area, sg_cell_SGC_group, sg_cell_SGC_c, sg_cell_SGC_is_large, wet_dry_bounds, SGCptr,dem_grid, Poisptr);


#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma omp critical
#endif
			{
				if (row_Hmax > Hmax)
					Hmax = row_Hmax;
			}
		}
	}

	for (int j = 0; j < grid_rows; j++)
	{
		wet_dry_bounds->fp_vol[j].start = wet_dry_bounds->fp_h[j].start;
		wet_dry_bounds->fp_vol[j].end = wet_dry_bounds->fp_h[j].end;
	}

	return Hmax;
}


void SGC2_UpdateLoadBalance(const int grid_rows, const int grid_cols_padded,
	const SubGridRowList * sub_grid_layout,
	WetDryRowBound* wet_dry_bounds)
{
	//printf("SGC2_UpdateLoadBalance\n");
	const int block_count = wet_dry_bounds->block_count;

	IndexRange* block_row_bounds = wet_dry_bounds->block_row_bounds;

#ifdef __INTEL_COMPILER
	__assume_aligned(block_row_bounds, 64);
#endif

	//SetArrayValue(block_start_row, -1, THREAD_ROW_BLOCK);
	//SetArrayValue(block_end_row, -1, THREAD_ROW_BLOCK);
	if (grid_rows > block_count)
	{
		int total_work = 0;
		for (int j = 0; j < grid_rows; j++)
		{
			//int grid_row_index = lyr * grid_cols_padded;
#if _BALANCE_TYPE == 1
			int row_start = wet_dry_bounds->fp_h[j].start;
			int row_end = wet_dry_bounds->fp_h[j].end;
			int work_this_row = ((row_end == -1) ? 0 : (row_end - row_start)) + 10;
#else
			int work_this_row = 1;
#endif

			total_work += work_this_row;
		}


		float target_work_per_block = (float)total_work / block_count;
		int allocated_work = 0;
		int block_index = 0;

		block_row_bounds[0].start = 0;
#if defined (_DEBUG) && _DEBUG > 1
		//printf("total work: %d block target: %d\n", total_work, target_work_per_block);
#endif

		for (int j = 0; j < grid_rows; j++)
		{
			if (block_index < (block_count - 1) &&
				(allocated_work) >= (block_index + 1) * target_work_per_block)
			{
				block_row_bounds[block_index].end = j;
				block_index++;
				block_row_bounds[block_index].start = j;
			}


			int row_start = wet_dry_bounds->fp_h[j].start;
			int row_end = wet_dry_bounds->fp_h[j].end;
#if _BALANCE_TYPE == 1
			int work_this_row = ((row_end == -1) ? 0 : (row_end - row_start)) + 10;
#else
			int work_this_row = 1;
#endif

			allocated_work += work_this_row;

		}
		block_row_bounds[block_index].end = grid_rows;
		// xdw modify, remove #ifdef _DEBUG
//#ifdef _DEBUG
		for (int bi = 0; bi < block_count; bi++)
		{
			int rows_in_block = block_row_bounds[bi].end - block_row_bounds[bi].start;
			NUMERIC_TYPE percent_rows_in_block = ((NUMERIC_TYPE)rows_in_block / grid_rows) * 100;

			int work_this_block1 = 0;
			for (int j = block_row_bounds[bi].start; j < block_row_bounds[bi].end; j++)
			{
				int row_start = wet_dry_bounds->fp_h[j].start;
				int row_end = wet_dry_bounds->fp_h[j].end;
#if _BALANCE_TYPE == 1
				int work_this_row = ((row_end == -1) ? 0 : (row_end - row_start)) + 10;
#else
				int work_this_row = 1;
#endif
				work_this_block1 += work_this_row;
			}

			NUMERIC_TYPE percent_work1 = ((NUMERIC_TYPE)work_this_block1 / total_work) * 100;
			//NUMERIC_TYPE percent_work2 = ((NUMERIC_TYPE)work_this_block2 / total_work) * 100;
			//printf("block: %d row: %d -> %d [%d](%.2" NUM_FMT"%%) work: [%d : %.2" NUM_FMT"%%]\n",
			//	bi,
			//	block_row_bounds[bi].start, block_row_bounds[bi].end,
			//	rows_in_block, percent_rows_in_block, work_this_block1, percent_work1);

		}
//#endif
	}
	else
	{
		//int block_index = 0;
		for (int j = 0; j < grid_rows; j++)
		{
			block_row_bounds[j].start = j;
			block_row_bounds[j].end = j + 1;
		}
	}
#if defined (_DEBUG) && _DEBUG > 1
	//printf("FloodPlain, %d, %d %lf\n", wetCount, wetBound, (double)wetCount / wetBound);
#endif

}

//---------------------------------------------------------------------------
// CALCULATE VOLUME OF WATER IN CHANNEL AND FLOODPLAIN
// CALCULATE FLOOD AREA
void SGC2_DomainVolumeAndFloodArea_block(const int block_index, const int grid_cols, const int grid_rows, const int grid_cols_padded, const NUMERIC_TYPE depth_thresh,
	const WetDryRowBound* wet_dry_bounds,
	const NUMERIC_TYPE * h_grid, const NUMERIC_TYPE * cell_area_col,
	const NUMERIC_TYPE * volume_grid,
	NUMERIC_TYPE *out_flood_area, NUMERIC_TYPE *out_domain_volume)
{
	// Calculate flood area
	const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
	const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;

	NUMERIC_TYPE block_flood_area = C(0.0);
	NUMERIC_TYPE block_domain_volume = C(0.0);

	for (int j = start_y; j < end_y; j++)
	{
#ifdef __INTEL_COMPILER
		__assume_aligned(cell_area_col, 64);
		__assume_aligned(h_grid, 64);
		__assume_aligned(volume_grid, 64);
#endif
		NUMERIC_TYPE dA = cell_area_col[j]; // if latlong is on change dA to local cell area

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
		__assume(grid_cols_padded % GRID_ALIGN_WIDTH == 0);
#endif

#pragma simd reduction(+: block_flood_area)
		for (int i = 0; i < grid_cols; i++)
		{
			int index = i + j*grid_cols_padded;
			// 统计水深>阈值的单元的面积和
			// only count height above the flood plain ( If sub-grid used channel depth is negative)
			block_flood_area += (h_grid[index] > depth_thresh) ? dA : C(0.0);
		}
#pragma simd reduction(+: block_domain_volume)
		for (int i = 0; i < grid_cols; i++)
		{
			// 统计水深>阈值的单元的体积和
			int index = i + j*grid_cols_padded;
			block_domain_volume += volume_grid[index];
		}
	}
	(*out_flood_area) = block_flood_area;
	(*out_domain_volume) = block_domain_volume;
}


#ifdef RESULT_CHECK
NUMERIC_TYPE Do_Update_old(States *Statesptr, Pars *Parptr, Solver *Solverptr, Arrays * Arrptr, SGCprams * SGCptr, BoundCs *BCptr, ChannelSegmentType *ChannelSegments, vector<ChannelSegmentType> *ChannelSegmentsVecPtr)
{
	// sub grid floodplain models
	SGC_FloodplainQ(Statesptr, Parptr, Solverptr, Arrptr, SGCptr);

	SGC_BCs(Statesptr, Parptr, Solverptr, BCptr, ChannelSegments, Arrptr, SGCptr);
	// Infiltration, evaporation and rainfall routines after time step update (TJF)11
	if (Statesptr->calc_evap == ON) SGC_Evaporation(Parptr, Solverptr, Arrptr, SGCptr);
	if (Statesptr->rainfall == ON && Statesptr->routing == OFF) SGC_Rainfall(Parptr, Solverptr, Arrptr); // CCS rainfall with routing scheme disabled
	if (Statesptr->routing == ON) SGC_Routing(Statesptr, Parptr, Solverptr, Arrptr);	// CCS Routing scheme (controls rainfall if enabled; can be used without rainfall)
	if (Statesptr->hazard == ON) UpdateV(Statesptr, Parptr, Solverptr, BCptr, ChannelSegments, Arrptr);
	NUMERIC_TYPE Hmax = SGC_UpdateH(Statesptr, Parptr, Solverptr, BCptr, ChannelSegments, Arrptr, SGCptr);

	BoundaryFlux(Statesptr, Parptr, Solverptr, BCptr, ChannelSegments, Arrptr, ChannelSegmentsVecPtr);
	// NOTES: Update Q's handeled within SGC flux equations (SGC_FloodplainQ etc.) time-step calculation intergrated with UpdateH

	return Hmax;
}
#endif
//#pragma optimize("", off)
void Do_Update(const int grid_cols, const int grid_rows, const int grid_cols_padded,
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE curr_time, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE g,
	NUMERIC_TYPE *h_grid, NUMERIC_TYPE *volume_grid,
	NUMERIC_TYPE *Qx_grid, NUMERIC_TYPE *Qy_grid, NUMERIC_TYPE *Qx_old_grid, NUMERIC_TYPE *Qy_old_grid,
	NUMERIC_TYPE *initHtm_grid, NUMERIC_TYPE *maxHtm_grid, NUMERIC_TYPE *totalHtm_grid, NUMERIC_TYPE *maxH_grid,
	NUMERIC_TYPE * maxVc_grid, NUMERIC_TYPE * maxVc_height_grid, NUMERIC_TYPE * maxHazard_grid,
	NUMERIC_TYPE * Vx_grid, NUMERIC_TYPE * Vy_grid, NUMERIC_TYPE * Vx_max_grid, NUMERIC_TYPE * Vy_max_grid,
	const NUMERIC_TYPE * SGC_BankFullHeight_grid,
	const NUMERIC_TYPE *dem_grid,
	const NUMERIC_TYPE *g_friction_sq_x_grid, const NUMERIC_TYPE *g_friction_sq_y_grid,
	const NUMERIC_TYPE *friction_x_grid, const NUMERIC_TYPE *friction_y_grid,
	const NUMERIC_TYPE *dx_col, const NUMERIC_TYPE *dy_col, const NUMERIC_TYPE *cell_area_col,
	const NUMERIC_TYPE *Fp_xwidth, const NUMERIC_TYPE *Fp_ywidth,

	const SubGridRowList * sub_grid_layout_rows, SubGridState * sub_grid_state_rows,
	const SubGridRowList * sub_grid_layout_blocks, SubGridState * sub_grid_state_blocks,

	TimeSeries * evap_time_series,
	NetCDFVariable * evap_grid,
	TimeSeries * rain_time_series,
	TimeSeries * temperature_time_series,
	const NUMERIC_TYPE *rain_grid,
	const NUMERIC_TYPE *dist_infil_grid,

	WetDryRowBound* wet_dry_bounds,
	PointSourceRowList * ps_layout, BoundaryCondition * boundary_cond,
	WeirLayout * weir_weirs, WeirLayout * weir_bridges,
	RouteDynamicList * route_dynamic_list,
	const NUMERIC_TYPE *route_V_ratio_per_sec_qx, const NUMERIC_TYPE * route_V_ratio_per_sec_qy,

	States *Statesptr,
	Pars *Parptr,
	Fnames *Fnameptr,
	Solver *Solverptr,
	Pois *Poisptr,
	Arrays *Arrptr,
	const SGCprams * SGCptr,
	Files *Fptr,
	Stage *Locptr,
	DamData *Damptr,
	SuperGridLinksList *Super_linksptr,
	//DynamicRain<> & dynamic_rain, removed JCN
	NUMERIC_TYPE ** tmp_thread_data, NUMERIC_TYPE ** tmp_thread_data_ch,
	const int verbose, NUMERIC_TYPE last_gw_time, LfpCouplingInfo * LfpCouplingInfoPtr)
{
	//cout << "*********************************111*********************************** " << endl;
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
	struct timeval timstr;
	double stop_time_4;
	gettimeofday(&timstr, NULL);
	stop_time_4 = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	cout << "Do_Update start time: " << stop_time_4 << endl;
#endif
	//if (Parptr->gwTstep < 0.0)
	//{
	//	Parptr->gwTstep = delta_time;
	//}
	//if (Parptr->MassInt < 0.0)
	//{
	//	Parptr->MassInt = delta_time;
	//}
	//if (Parptr->PoiSaveInt < 0.0)
	//{
	//	Parptr->PoiSaveInt = delta_time;
	//}
	
	int thread_n = omp_get_thread_num();
	NUMERIC_TYPE * tmp_row = tmp_thread_data[omp_get_thread_num()];
	//NUMERIC_TYPE * tmp_row_ch = tmp_thread_data_ch[omp_get_thread_num()];

	//each thread clears the tmp_row, before using in SGC2_UpdateQ_block
	memset(tmp_row, 0, sizeof(NUMERIC_TYPE) * grid_cols_padded);
	//memset(tmp_row_ch, 0, sizeof(NUMERIC_TYPE) * grid_cols_padded);
	//cout << "volume_grid1: " << volume_grid[]
	/// nowait means that some threads may still continue after this function returns.
	/// note the #pragma omp barrier is required before the using or updating q
	// 更新floodplain上的流量
#pragma omp for schedule(static) nowait
	for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
	{
		const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
		const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;

		for (int j = start_y; j < end_y; j++)
		{
			const IndexRange dem_data_bound = wet_dry_bounds->dem_data[j];
			const int grid_row_index = j * grid_cols_padded;
			const NUMERIC_TYPE row_dx = dx_col[j];
			const NUMERIC_TYPE row_dy = dy_col[j];
			const NUMERIC_TYPE row_cell_area = cell_area_col[j];

			// first process Qx
			// xiaodw, 暂时改成在整个dem上计算
			//int row_start_x = max(dem_data_bound.start, wet_dry_bounds->fp_h[j].start - 1); // start one cell to the left (flow from cell on the right)
			//int row_end_x = min(wet_dry_bounds->fp_h[j].end, dem_data_bound.end - 1);
			int row_start_x = dem_data_bound.start; // start one cell to the left (flow from cell on the right)
			int row_end_x = dem_data_bound.end - 1;

			int row_start_x_prev = max(dem_data_bound.start, wet_dry_bounds->fp_h_prev[j].start - 1);
			int row_end_x_prev = min(wet_dry_bounds->fp_h_prev[j].end, dem_data_bound.end - 1);
			//cout << "loop " << itCount << " debug " << 2 << endl;
			// 更新flood plain上的流量， h_grid: 栅格单元上的水深
			SGC2_UpdateQx_row(grid_cols,
				grid_row_index,
				row_start_x_prev, row_end_x_prev,
				row_start_x, row_end_x,
				depth_thresh, row_dx, Fp_ywidth,
				g, delta_time, curr_time,
				tmp_row,
				dem_grid, h_grid,
				g_friction_sq_x_grid, Qx_grid, Qx_old_grid, Parptr->max_Froude);
			//cout << "loop " << itCount << " debug " << 3 << endl;

			if (Statesptr->routing == ON)
			{
				int route_count = SGC2_UpdateRouteQ_row(grid_row_index, row_start_x, row_end_x,
					row_cell_area, delta_time, Parptr->RouteSfThresh,
					1,
					tmp_row, h_grid, dem_grid,
					route_V_ratio_per_sec_qx, Qx_grid, Qx_old_grid,
					route_dynamic_list->route_list_i_lookup_qx);
				route_dynamic_list->row_route_qx_count[j] = route_count;
			}
			else if (Statesptr->diffusive_switch == ON)
			{
				SGC2_UpdateDiffusiveQ_row(grid_row_index, row_start_x, row_end_x,
					row_dy, row_dx,
					depth_thresh, Parptr->DiffusiveFroudeThresh, Solverptr->MaxHflow, g,
					1,
					h_grid, dem_grid, friction_x_grid, Qx_grid, Qx_old_grid);
			}
			//cout << "loop " << itCount << " debug " << 4 << endl;

			if ((curr_time >= Parptr->SaveTotal && Statesptr->voutput == ON) || // calculate v at save interval only
				Statesptr->voutput_max == ON) // v_max_output or hazard require velocity to be calculated each step
			{
				// velocity calculated before sub grid or weir updates
				// only calculates flood-plain value
				SGC2_UpdateVelocity_row(grid_row_index, row_start_x_prev, row_end_x_prev, row_start_x, row_end_x,
					1,
					row_dy, h_grid, dem_grid, Qx_grid, Vx_grid, Vx_max_grid);
			}

			bool not_last_row = (j < grid_rows - 1);
			// PFU: this is processing Qy
			//skip last row
			int row_start_y = 0;
			int row_end_y = 0;
			if (not_last_row)
			{
				row_start_y = min(wet_dry_bounds->fp_h[j].start, wet_dry_bounds->fp_h[j + 1].start);
				row_end_y = max(wet_dry_bounds->fp_h[j].end, wet_dry_bounds->fp_h[j + 1].end);

				int row_start_y_prev = min(wet_dry_bounds->fp_h_prev[j].start, wet_dry_bounds->fp_h_prev[j + 1].start);
				int row_end_y_prev = max(wet_dry_bounds->fp_h_prev[j].end, wet_dry_bounds->fp_h_prev[j + 1].end);
				// 更新flood plain栅格上的流量，当栅格上包含subgrid河道时，用Fp_xwidth作为栅格的宽度，只计算河道上底之上且处于河道宽度范围之外的栅格水深，参考Neal文章的figure1.c
				// todo4: 如果Fp_xwidth < 0 ，计算出的流量是否 < 0?
				SGC2_UpdateQy_row(grid_cols,
					grid_row_index,
					row_start_y_prev, row_end_y_prev,
					row_start_y, row_end_y,
					grid_cols_padded,
					depth_thresh, Fp_xwidth, row_dy,
					g, delta_time, curr_time,
					tmp_row,
					dem_grid, h_grid,
					g_friction_sq_y_grid, Qy_grid, Qy_old_grid, Parptr->max_Froude);

				if (Statesptr->routing == ON)
				{
					int route_count = SGC2_UpdateRouteQ_row(grid_row_index, row_start_y, row_end_y,
						row_cell_area, delta_time, Parptr->RouteSfThresh,
						grid_cols_padded,
						tmp_row, h_grid, dem_grid,
						route_V_ratio_per_sec_qy, Qy_grid, Qy_old_grid,
						route_dynamic_list->route_list_i_lookup_qy);
					route_dynamic_list->row_route_qy_count[j] = route_count;
				}
				else if (Statesptr->diffusive_switch == ON)
				{
					SGC2_UpdateDiffusiveQ_row(grid_row_index, row_start_y, row_end_y,
						row_dx, row_dy,
						depth_thresh, Parptr->DiffusiveFroudeThresh, Solverptr->MaxHflow, g,
						grid_cols_padded,
						h_grid, dem_grid, friction_y_grid, Qy_grid, Qy_old_grid);
				}
				//cout << "loop " << itCount << " debug " << 7 << endl;

				if ((curr_time >= Parptr->SaveTotal && Statesptr->voutput == ON) || // calculate v at save interval only
					Statesptr->voutput_max == ON) // v_max_output or hazard require velocity to be calculated each step
				{
					// velocity calculated before sub grid or weir updates
					// only calculates flood-plain value
					SGC2_UpdateVelocity_row(grid_row_index, row_start_y_prev, row_end_y_prev, row_start_y, row_end_y,
						grid_cols_padded,
						row_dx, h_grid, dem_grid, Qy_grid, Vy_grid, Vy_max_grid);
				}

			}

#if _SGM_BY_BLOCKS == 0
			// 计算sub grid河道上的流量，包含河道内+河道上底之上且范围在河道宽度内
			ProcessSubGridQBlock(j, grid_cols_padded, depth_thresh, delta_time, g, sub_grid_layout_rows, sub_grid_state_rows, SGCptr, h_grid,
				wet_dry_bounds, Qx_grid, Qy_grid, Qx_old_grid, Qy_old_grid, Parptr->max_Froude, Poisptr->Q_Ch, Parptr->sgcStartH, Statesptr,row_cell_area);
			if (curr_time >= Parptr->SaveTotal && Statesptr->SGCvoutput == ON)
			{
				SGC2_UpdateVelocitySubGrid_block(j, grid_cols_padded, depth_thresh, delta_time, sub_grid_layout_rows, sub_grid_state_rows, SGCptr, h_grid);
			}
#endif
			if (weir_weirs->row_cols_padded != 0)
			{
				SGC2_UpdateWeirsFlow_row(j, grid_cols, grid_rows, grid_cols_padded,
					depth_thresh, delta_time, h_grid, volume_grid, Qx_grid, Qy_grid,
					wet_dry_bounds, weir_weirs);
			}
		}
		//printf("Q done %d\n", omp_get_thread_num());
	}
	if (Parptr->sgcStartH > 0) {
		Parptr->sgcStartH = -1.0;
	}
#if _SGM_BY_BLOCKS == 1
	if (sub_grid_layout_blocks->row_cols_padded > 0)
	{
#pragma omp barrier
#pragma omp for schedule(static) nowait
		for (int bi = 0; bi < wet_dry_bounds->block_count; bi++)
		{
			ProcessSubGridQBlock(bi, grid_cols_padded, depth_thresh, delta_time, g, sub_grid_layout_blocks, sub_grid_state_blocks, SGCptr, h_grid,
				wet_dry_bounds, Qx_grid, Qy_grid, Qx_old_grid, Qy_old_grid);
			if (curr_time >= Parptr->SaveTotal && Statesptr->SGCvoutput == ON)
			{
				SGC2_UpdateVelocitySubGrid_block(j, grid_cols_padded, depth_thresh, delta_time, sub_grid_layout_blocks, sub_grid_state_blocks, SGCptr, h_grid);
			}
		}
	}	
	}
	{
		
#endif

	// this block of code is executed by the first thread that finishes it's updateQ (with nowait clause) 
	// nowait clause added - disable the implicit barrier, since and explicit barrier used (stops both the update q threads as well as the single thread)
#pragma omp single nowait 
//#pragma omp single  // xiaodw
	{
		//printf("overlapped single %d\n", omp_get_thread_num());
		reduce_Hmax = C(0.0);
		reduce_evap_loss = C(0.0);
		reduce_infil_loss = C(0.0);
		reduce_interflow_loss = C(0.0);
		reduce_rain_total = C(0.0);
		reduce_Qpoint_timestep_pos = C(0.0);
		reduce_Qpoint_timestep_neg = C(0.0);

		reduce_flood_area = C(0.0);
		reduce_domain_volume = C(0.0);

		evap_deltaH_step = C(0.0);
		rain_deltaH_step = C(0.0);

		// pre-calculate the TimeSeries interpolation for all point sources
		// * prevent any threading issues if a TimeSeries is used on multiple points
		// * reduce code complexity in calculating point source
		for (int j = 0; j < grid_rows; j++)
		{
			const int ps_count = ps_layout->ps_row_count[j];
			const int row_cols_padded = ps_layout->row_cols_padded;
			const int row_start = j * row_cols_padded;
			WaterSource ps_info = ps_layout->ps_info;
			for (int i = 0; i < ps_count; i++)
			{
				int ws_index = row_start + i;
				if (ps_info.timeSeries[ws_index] != NULL)
				{
					ps_info.Val[ws_index] = InterpolateTimeSeries(ps_info.timeSeries[ws_index], curr_time);
				}
			}
		}

		// SGC_BCs updates the Q flow at the boundaries
		// Updates Q_x and Q_y on the floodplain and also updates volume for any sub-grid flow
		SGC2_BCs(grid_cols, grid_rows, grid_cols_padded, delta_time, curr_time, depth_thresh, g,
			dx_col, dy_col, h_grid, Qx_grid, Qy_grid, Qx_old_grid, Qy_old_grid,
			wet_dry_bounds,
			Statesptr, Parptr, boundary_cond, SGCptr, Parptr->max_Froude);

		if (Statesptr->calc_evap == ON)
		  {
		    // evap_deltaH_step = InterpolateTimeSeries(evap_time_series, curr_time); //constant rate across whole floodplain
		    for (int j = 0; j < evap_time_series->count; j++) {
		      if (evap_time_series->time[j] > curr_time) {
			evap_deltaH_step = evap_time_series->value[j];
			break;
		      }
		    }
		    evap_deltaH_step *= delta_time;
		    for (int j = 0; j < grid_cols_padded * grid_rows; j++) {
		      evap_grid->data[j] = evap_deltaH_step;
		    }
		  }
		else if (Statesptr->calc_evap == TIME_SPACE)
		  {
		    for (int j =0; j < grid_cols_padded * grid_rows; j++) {
		      evap_grid->data[j] *= delta_time;
		      if (evap_grid->data[j] > 0.0)
			evap_deltaH_step = evap_grid->data[j];
		    }
		  }
		// 下渗累积量
		//if (Statesptr->calc_evap == ON)
		//{
		//	for (int lyr = 0; lyr < evap_time_series->count; lyr++) {
		//		if (evap_time_series->time[lyr] > curr_time) {
		//			evap_deltaH_step = evap_time_series->value[lyr];
		//			break;
		//		}
		//	}
		//	evap_deltaH_step *= delta_time;
		//	for (int lyr = 0; lyr < grid_cols_padded * grid_rows; lyr++) {
		//		evap_grid->data[lyr] = evap_deltaH_step;
		//	}
		//}
		// xdw add, support temperature
		if (Statesptr->use_temperature == ON)
		{
			temperature_step = InterpolateTimeSeries(temperature_time_series, curr_time); //constant rate across whole floodplain
		}
		if (Statesptr->rainfall == ON)
		{
			// 这里降雨单位已经换算成了m
			rain_deltaH_step = InterpolateTimeSeries(rain_time_series, curr_time); //constant rate across whole floodplain
			rain_deltaH_step *= delta_time;
			// xdw add, to differentiate snowfall and rainfall by temperature
			if (Statesptr->use_snow_glacier == ON)
			{
				if (Statesptr->use_temperature == OFF)
				{
					cout << "You have to specify a temperature timeseries file when using glacier and melt model. Please set 'temperature' file in par file."  << endl;
				}
				
				if (temperature_step <= Parptr->melt_temperature)
				{
					snow_deltaH_step = rain_deltaH_step;
					rain_deltaH_step = 0.0;
				}
				else
				{
					snow_deltaH_step = 0.0;
				}
			}

		}
		//cout << "loop " << itCount << " debug " << 10 << endl;

		//printf("overlapped done %d\n", omp_get_thread_num());
	}

	//printf("before barrier done %d\n", omp_get_thread_num());
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
	double stop_time_6;
	gettimeofday(&timstr, NULL);
	stop_time_6 = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	double interval_6 = stop_time_6 - stop_time_5;
	cout << "Do_Update interval 6: " << interval_6 << "s" << endl;
#endif
	// Toby suggest DamFlowVolume go here!!! FEOL
#pragma omp barrier // ensure all threads have finished their updateQ (nowait) and single section
	
	if (Statesptr->DamMode == ON)
	{
#pragma omp single
		{
			// Call Dam Function after all Q have being calculated!  FEOL
			SGC2_UpdateDamFlowVolume(grid_cols, grid_rows, grid_cols_padded, depth_thresh, delta_time, curr_time, h_grid, volume_grid, Qx_grid, Qy_grid, Damptr, SGCptr, g, Parptr->max_Froude);
		}
		//End Dam FEOL
	}
	// supergrid channel links... currently in OMP single section
	if (Statesptr->ChanMaskRead == ON)
	{
#pragma omp single
		{
			SGC2_CalcLinksQ(Super_linksptr, volume_grid, h_grid, delta_time, g, depth_thresh, Parptr->max_Froude, wet_dry_bounds);
		}
	}
	//printf("after barrier done %d\n", omp_get_thread_num());
	// 计算sgc模式下的流速
	if (Statesptr->save_stages == ON &&
		Statesptr->voutput_stage == ON &&
		curr_time >= Parptr->MassTotal)
	{
#pragma omp single
		{


			fprintf(Fptr->vel_fp, "%12.3" NUM_FMT"", curr_time);
			for (int i = 0; i < Locptr->Nstages; i++)
			{
				if (Locptr->stage_check[i] == 1)
				{
					int y = Locptr->stage_grid_y[i];
					int grid_index = Locptr->stage_grid_x[i] + y * grid_cols_padded;
					NUMERIC_TYPE Vx_west = (i != 0) ? SGC2_CalculateVelocity(grid_index - 1, grid_index, Qx_grid, h_grid, dem_grid, dy_col[y]) : C(0.0);
					NUMERIC_TYPE Vx_east = (i < grid_cols - 1) ? SGC2_CalculateVelocity(grid_index, grid_index + 1, Qx_grid, h_grid, dem_grid, dy_col[y]) : C(0.0);

					NUMERIC_TYPE Vy_north = (y != 0) ? SGC2_CalculateVelocity(grid_index - grid_cols_padded, grid_index, Qy_grid, h_grid, dem_grid, dx_col[y]) : C(0.0);
					NUMERIC_TYPE Vy_south = (y < grid_rows - 1) ? SGC2_CalculateVelocity(grid_index, grid_index + grid_cols_padded, Qy_grid, h_grid, dem_grid, dx_col[y]) : C(0.0);

					// v not calculated at each timestep 
					//NUMERIC_TYPE Vx_west = Vx_grid[grid_index];
					//NUMERIC_TYPE Vx_east = Vx_grid[grid_index + 1];

					//NUMERIC_TYPE Vy_north = Vy_grid[grid_index];
					//NUMERIC_TYPE Vy_south = Vy_grid[grid_index + grid_cols_padded];

					fprintf(Fptr->vel_fp, "%10.4" NUM_FMT"", SQRT(POW(getmax(FABS(Vx_east), FABS(Vx_west)), C(2.0)) + POW(getmax(FABS(Vy_north), FABS(Vy_south)), C(2.0))));
				}
				else
					fprintf(Fptr->vel_fp, "-\t");
			}
			fprintf(Fptr->vel_fp, "\n");
			fflush(Fptr->vel_fp); // force program to flush buffer to file - keeps file in sync with writes - user sometimes tracks progress through the file.
		}
	}

	
	
	//cout << "*********************************333*********************************** " << endl;
	if (Statesptr->routing_mass_check == ON || weir_bridges->row_cols_padded > 0 || Statesptr->hazard == ON)
	{
		/// This update must be performed after UpdateQ as it requires all Q's to be updated
		/// Since it also modifies Q, it must be before any reading of Q (in SGC2_UpdateVolumeHeight)
#pragma omp for schedule(static)
		//for (int lyr = 0; lyr < grid_rows; lyr++)
		for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
		{
			const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
			const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;

			for (int j = start_y; j < end_y; j++)
			{
				int grid_row_index = j * grid_cols_padded;
				if (Statesptr->routing_mass_check == ON)
				{
					SGC2_CorrectRouteFlow_row(j, grid_row_index, grid_cols, grid_rows, grid_cols_padded, delta_time,
						route_dynamic_list, volume_grid,
						Qx_grid, Qy_grid);
				}
				if (weir_bridges->row_cols_padded > 0)
				{
#if _SGM_BY_BLOCKS == 1
					SGC2_UpdateBridgesFlow_row(j, grid_cols, grid_rows, grid_cols_padded,
						delta_time, curr_time, depth_thresh, g, dx_col, dy_col, h_grid, Qx_grid, Qy_grid,
						wet_dry_bounds, sub_grid_state_blocks, weir_bridges);
#else
					SGC2_UpdateBridgesFlow_row(j, grid_cols, grid_rows, grid_cols_padded,
						delta_time, curr_time, depth_thresh, g, dx_col, dy_col, h_grid, Qx_grid, Qy_grid,
						wet_dry_bounds, sub_grid_state_rows, weir_bridges, Parptr->max_Froude);
#endif
				}
				if (Statesptr->hazard == ON)
				{
					SGC2_UpdateHazard_row(j, grid_row_index, grid_cols, grid_rows, grid_cols_padded,
						Vx_grid, Vy_grid, h_grid, dem_grid,
						dx_col[j], dy_col[j], SGC_BankFullHeight_grid, maxVc_grid, maxVc_height_grid, maxHazard_grid, wet_dry_bounds);
				}
			}
		}
		//#ifdef _DEBUG
		//			int total_routes = 0;
		//			for (int lyr = 0; lyr < grid_rows; lyr++)
		//			{
		//				total_routes += route_dynamic_list->row_route_qx_count[lyr];
		//				total_routes += route_dynamic_list->row_route_qy_count[lyr];
		//			}
		//			printf("Routes %d Rain %12.4e %.2" NUM_FMT" %% of cells\n", total_routes, rain_deltaH_step, (NUMERIC_TYPE)total_routes / (grid_cols*grid_rows) * 100);
		//#endif
	}


	const int * sg_cell_grid_index_lookup = sub_grid_layout_rows->cell_info.sg_cell_grid_index_lookup;
	/**/

	//cout << "*********************************444*********************************** " << endl;
	if (curr_time - last_gw_time >= Parptr->gwTstep) {

		///////////
		if (Statesptr->use_dhsvm == ON)
		{

			//#pragma omp barrier
			#pragma omp single
			{
				int padding_count = grid_cols_padded - grid_cols;
				int padding = sizeof(NUMERIC_TYPE) * padding_count;
				const int start_y = 0;
				const int end_y = Parptr->ysz;
				int count = 0;
				for (int j = start_y; j < end_y; j++)
				{
					int infilt_row_start = wet_dry_bounds->dem_data[j].start;
					int infilt_row_end = wet_dry_bounds->dem_data[j].end;
					int grid_row_index = j * grid_cols_padded;
					for (int i = infilt_row_start; i < infilt_row_end; i++)
					{
						int index = i + grid_row_index;
						if (dem_grid[i + grid_row_index] != DEM_NO_DATA) {
							Parptr->satFlowPD[index] = 0.0;
							Parptr->satFlow2ChPD[index] = 0.0;
							Parptr->satFlow2SurfPD[index] = 0.0;
							Parptr->satFlow2NeiborPD[index] = 0.0;
							Parptr->PercExcess2SurfPD[index] = 0.0;

							
							Parptr->waterLevelPD[index] = dem_grid[index] - Parptr->tableDepthPD[index];
							if (Statesptr->save_poi == ON) {
								/*Poisptr->Rain_Grid[index] = 0.0;
								Poisptr->Evap_Grid[index] = 0.0;
								Poisptr->Infilt_Grid[index] = 0.0;
								Poisptr->Qx_Grid[index] = 0.0;
								Poisptr->Q_Ch[index] = 0.0;*/
								Poisptr->Vol_Grid[index] = 0.0;
								//Poisptr->surf_water_depth_Grid[index] = 0.0;
								//Poisptr->soil_lat_flowin_Grid_allLyr[index] = 0.0;
								//Poisptr->soil_lat_flowout_Grid_allLyr[index] = 0.0;
								for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
								{
									// cm
									//Poisptr->soil_lat_flowin_Grid[lyr][index] = 0.0;
									//Poisptr->soil_lat_flowout_Grid[lyr][index] = 0.0;
									//Poisptr->soil_perc_Grid[lyr][index] = 0.0;
									Poisptr->soil_water_depth_Grid[lyr][index] = 0.0;
									
								}

							}
						}
					}
				}
			}

			// ----------------------------------------地下水多线程版----------------------------------
			#pragma omp barrier
			{
				#pragma omp for schedule(static)
				//#pragma omp for schedule(static) nowait
				for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
				{
					const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
					const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;
					for (int j = start_y; j < end_y; j++)
					{
						int infilt_row_start = wet_dry_bounds->dem_data[j].start;
						int infilt_row_end = wet_dry_bounds->dem_data[j].end;
						int grid_row_index = j * grid_cols_padded;
						if (Statesptr->use_groundwater == ON) {
							UnsaturatedFlowGwVersionV2(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
								dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);

						}
						else {
							UnsaturatedFlow(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
								dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);
						}


						
					}
				}
			}

#pragma omp barrier
			{
#pragma omp for schedule(static)
				//#pragma omp for schedule(static) nowait
				for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
				{
					const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
					const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;
					for (int j = start_y; j < end_y; j++)
					{
						int infilt_row_start = wet_dry_bounds->dem_data[j].start;
						int infilt_row_end = wet_dry_bounds->dem_data[j].end;
						int grid_row_index = j * grid_cols_padded;
						HeadSlopeAspect(Parptr, Solverptr, Arrptr, grid_row_index, j, grid_rows, grid_cols, dem_grid + grid_row_index, infilt_row_start, infilt_row_end, grid_cols_padded, dx_col[j], dy_col[j], wet_dry_bounds);
				
					}
				}
			}

#pragma omp barrier
			{
#pragma omp for schedule(static)
				//#pragma omp for schedule(static) nowait
				for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
				{
					const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
					const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;
					for (int j = start_y; j < end_y; j++)
					{
						int infilt_row_start = wet_dry_bounds->dem_data[j].start;
						int infilt_row_end = wet_dry_bounds->dem_data[j].end;
						int grid_row_index = j * grid_cols_padded;
						const int sg_row_start = j * sub_grid_layout_rows->row_cols_padded;
						RouteSubSurface(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
							dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, dx_col[j], dy_col[j], wet_dry_bounds, Poisptr,
							sub_grid_layout_rows->cell_info.sg_cell_cell_area, j,
							sub_grid_layout_rows->cell_row_count[j], sg_row_start, sg_cell_grid_index_lookup, sub_grid_layout_rows->cell_info.sg_cell_SGC_BankFullHeight, 
							sub_grid_layout_rows->cell_info.sg_cell_SGC_BankFullVolume, sub_grid_layout_rows->cell_info.sg_cell_SGC_c, sub_grid_layout_rows->flow_info.sg_cell_flow_lookup);
					}
				}
			}

#pragma omp barrier
			{
#pragma omp for schedule(static)
				//#pragma omp for schedule(static) nowait
				for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
				{
					const int start_y = wet_dry_bounds->block_row_bounds[block_index].start;
					const int end_y = wet_dry_bounds->block_row_bounds[block_index].end;
					for (int j = start_y; j < end_y; j++)
					{
						int infilt_row_start = wet_dry_bounds->dem_data[j].start;
						int infilt_row_end = wet_dry_bounds->dem_data[j].end;
						int grid_row_index = j * grid_cols_padded;

						DistributeSatflow(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
							dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);


					}
				}
			}
			// ----------------------------------------地下水单线程版----------------------------------
//#pragma omp barrier
//#pragma omp single
//			{
//				// xiaodw, 对于DHSVM计算的逐小时侧向壤中流和地表溢流，根据时间步长加权平均到每个sgc步长
//				if (Statesptr->use_dhsvm == ON)
//				{
//					//int thread_id = omp_get_thread_num();
//					//std::cout << "Thread " << thread_id << " calcu dhsvm\n";
//					const int start_y = 0;
//					const int end_y = Parptr->ysz;
//					int count = 0;
//					for (int j = start_y; j < end_y; j++)
//					{
//						int infilt_row_start = wet_dry_bounds->dem_data[j].start;
//						int infilt_row_end = wet_dry_bounds->dem_data[j].end;
//						int grid_row_index = j * grid_cols_padded;
//						UnsaturatedFlow(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
//							dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);
//						RouteSubSurface(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
//							dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, dx_col[j], dy_col[j], wet_dry_bounds, Poisptr);
//						DistributeSatflow(infilt_row_start, infilt_row_end, Parptr, Solverptr, Arrptr, Statesptr, grid_row_index, j, grid_rows, grid_cols,
//							dem_grid + grid_row_index, cell_area_col[j], grid_cols_padded, volume_grid, Parptr->multi_nSoilLyrs, Parptr->multi_nRootLyrs, Poisptr);
//					}
//				}
//			}

		}
	}
	// xiaodw, calculate sumNCells(all valuable dem cells) and sumNSgcCells at first time once
#pragma omp single nowait 
	{
	if (Parptr->sumNSgcCells == 0) {
		int sumNCells = 0;
		NUMERIC_TYPE sum_cell_area = 0.f;
		const int start_y = 0;
		const int end_y = Parptr->ysz;
		int count = 0;
		for (int j = start_y; j < end_y; j++)
		{
			int infilt_row_start = wet_dry_bounds->dem_data[j].start;
			int infilt_row_end = wet_dry_bounds->dem_data[j].end;
			int grid_row_index = j * grid_cols_padded;
			sumNCells += (infilt_row_end - infilt_row_start);
			sum_cell_area += cell_area_col[j] * (infilt_row_end - infilt_row_start);
		}
		Parptr->sumNCells = sumNCells;
		Parptr->avgCellArea = sum_cell_area / sumNCells;

		int sumNSgcCells = 0;
		if (Parptr->sumNSgcCells == 0) {

		for (int j = start_y; j < end_y; j++)
		{
			const int cell_count = sub_grid_layout_rows->cell_row_count[j];
			sumNSgcCells += cell_count;
		}
			
			Parptr->sumNSgcCells = sumNSgcCells;
		}
	}
	}
#pragma omp barrier
#pragma omp single
	{
		///////
		// xiaodw, 对于DHSVM计算的逐小时侧向壤中流和地表溢流，根据时间步长加权平均到每个sgc步长
		if (Statesptr->use_dhsvm == ON)
		{
			//int thread_id = omp_get_thread_num();
			//std::cout << "Thread " << thread_id << " calcu dhsvm2\n";
			for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
			{
				// cm
				Parptr->multi_soilWaterDepthOfLyr[lyr] = 0.0;
				//Parptr->multi_soilPercoDepOfLyr[lyr] = 0.0;

			}
			Parptr->PercolationDepth = 0.0;
			const int start_y = 0;
			const int end_y = Parptr->ysz;
			int count = 0;
			for (int j = start_y; j < end_y; j++)
			{
				int infilt_row_start = wet_dry_bounds->dem_data[j].start;
				int infilt_row_end = wet_dry_bounds->dem_data[j].end;
				int grid_row_index = j * grid_cols_padded;
				NUMERIC_TYPE row_cell_area = cell_area_col[j];
				for (int i = infilt_row_start; i < infilt_row_end; i++)
				{
					int index = i + grid_row_index;
					if ((dem_grid[i + grid_row_index] != DEM_NO_DATA)) {
						int source_index_this = j * Parptr->xsz + i;
						//if (Arrptr->ChanMask[source_index_this] > 0) {
						//	count++;
						//}
						// 如果该栅格是SGC河道
						if (Arrptr->SGCwidth[source_index_this] > C(0.0) && (Arrptr->DEM[source_index_this] != DEM_NO_DATA || Arrptr->ChanMask[source_index_this] > 0)) {

							//channel_grid_inc_inflow(volume_grid, index, Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep);

							//cout << "ch_add: " << index << "  " << Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep << endl;
							Parptr->subSurfaceLatFlow2ChTotal += Parptr->satFlow2ChPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
							// 来自洪泛区地表的水
							Parptr->surfaceFlow2ChTotal += Parptr->surflow2ChPD[index];
							
							// 来自自身降雨扣除入渗后的水
							Parptr->surfaceHydro2ChTotal += Parptr->hydro2ChPD[index];


							//if (Statesptr->save_poi == ON)
							//{
							//	for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
							//	{
							//		Poisptr->soil_lat_flowout_Grid[lyr][index] = Parptr->satFlow2ChPD[index] * 1000.0 / (row_cell_area * Parptr->gwTstep);  // m3/gwstep ->mm/gwstep
							//		
							//	}
							//}

						}
						

						if (Parptr->satFlow2SurfPD[index] > 0.0) {
							//volume_grid[index] += Parptr->satFlow2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
							Parptr->subSurfaceLatFlow2SurfTotal += Parptr->satFlow2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
						}
						
						// 渗漏造成的地表溢流
						if (Parptr->PercExcess2SurfPD[index] > 0.0)
						{
							Parptr->subSurfacePerc2SurfTotal += Parptr->PercExcess2SurfPD[index] * Solverptr->SGCtmpTstep / Parptr->gwTstep;
						}
						
						for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
						{
							// cm
							Parptr->multi_soilWaterDepthOfLyr[lyr] += Parptr->multi_soilMoisturePD[lyr][index] * Parptr->multi_soilThicknessPD[lyr][index] * 100.0;
							if (Parptr->multi_soilPercoPD[lyr][index] > 0.0)
							{
								Parptr->multi_soilPercoDepOfLyr[lyr] += Parptr->multi_soilPercoPD[lyr][index] * 100.0 * Solverptr->SGCtmpTstep / Parptr->gwTstep;  // m->cm
							}

						}
						




					}
				}

			}

	


			if (curr_time - last_gw_time >= Parptr->gwTstep)
			{


				if (Statesptr->use_groundwater == ON)
				{
					Parptr->PercolationDepth = 0.0;
					const int start_y = 0;
					const int end_y = Parptr->ysz;
					int count = 0;
					for (int j = start_y; j < end_y; j++)
					{
						int infilt_row_start = wet_dry_bounds->dem_data[j].start;
						int infilt_row_end = wet_dry_bounds->dem_data[j].end;
						int grid_row_index = j * grid_cols_padded;
						NUMERIC_TYPE row_cell_area = cell_area_col[j];
						for (int i = infilt_row_start; i < infilt_row_end; i++)
						{
							int index = i + grid_row_index;
							if ((dem_grid[index] != DEM_NO_DATA)) {
								Parptr->PercolationDepth += Parptr->multi_soilPercoPD[Parptr->multi_nSoilLyrs - 1][index] * 1000.0;   // m->mm
								//Parptr->PercolationDepth += Parptr->multi_soilPercoPD[Parptr->multi_nSoilLyrs - 1][index] * 1000.0 * Solverptr->SGCtmpTstep / Parptr->gwTstep;  // m->mm

							}
						}
					}
					
				}
				Parptr->PercolationDepth /= Parptr->sumNCells;

				// water balance (mm)
				Parptr->GwStorageDepth += Parptr->PercolationDepth;

				if (Parptr->GwStorageDepth < 0.0)
				{
					Parptr->GwStorageDepth = 0.0;
				}

				// mm
				//float percolation = Parptr->PercolationDepth * (1.f - Parptr->deepCoefficient);
				// depth of groundwater runoff(mm)
				NUMERIC_TYPE outFlowDepth = Parptr->recessionCoefficient * pow(Parptr->GwStorageDepth, Parptr->recessionExponent);

				Parptr->GwStorageDepth -= outFlowDepth;

				// record for mass output, m3
				Parptr->GwStorageVol = Parptr->GwStorageDepth * Parptr->avgCellArea * Parptr->sumNCells * 0.001;

				// groundwater flow out of the basin at time step t (m3/s), xiaodw: here we add this to each grid of the sgc channel
				Parptr->sumGndQ2Rch = outFlowDepth * 0.001 * Parptr->sumNCells * Parptr->avgCellArea / Parptr->gwTstep;

				// xiaodw, add subground flow Q to each subgrid channel cell,m3/s
				//const int * sg_cell_grid_index_lookup = sub_grid_layout_rows->cell_info.sg_cell_grid_index_lookup;
				Parptr->gwQPerSgcCell = Parptr->sumGndQ2Rch / Parptr->sumNSgcCells;    // m3/s

				
				/*Parptr->subSurfaceLatFlow2Channel_rate = (Parptr->subSurfaceLatFlow2ChTotal - Parptr->subSurfaceLatFlowTotal2Ch_Last) / Parptr->gwTstep;
				Parptr->subSurfaceLatFlowTotal2Ch_Last = Parptr->subSurfaceLatFlow2ChTotal;
				Parptr->subSurfaceLatFlow2Surf_rate = (Parptr->subSurfaceLatFlow2SurfTotal - Parptr->subSurfaceLatFlowTotal2Surf_Last) / Parptr->gwTstep;
				Parptr->subSurfaceLatFlowTotal2Surf_Last = Parptr->subSurfaceLatFlow2SurfTotal;
				Parptr->subSurfacePerc2Surf_rate = (Parptr->subSurfacePerc2SurfTotal - Parptr->subSurfacePercTotal2Surf_Last) / Parptr->gwTstep;
				Parptr->subSurfacePercTotal2Surf_Last = Parptr->subSurfacePerc2SurfTotal;*/
				//for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				//{
				//	Parptr->multi_soilPercoDepOfLyr_rate[lyr] = (Parptr->multi_soilPercoDepOfLyr[lyr] - Parptr->multi_soilPercoDepOfLyr_Last[lyr]) * 3600.0 / Parptr->gwTstep;
				//	Parptr->multi_soilPercoDepOfLyr_Last[lyr] = Parptr->multi_soilPercoDepOfLyr[lyr];
				//}
			}
		}
	}

	
	//cout << "*********************************555*********************************** " << endl;



	//each thread clears the tmp_row, before using in SGC2_UpdateVolumeHeight_block
	memset(tmp_row, 0, sizeof(NUMERIC_TYPE) * grid_cols_padded);
	//memset(tmp_row_ch, 0, sizeof(NUMERIC_TYPE) * grid_cols_padded);
	
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma omp for reduction (+:reduce_evap_loss, reduce_rain_total,reduce_infil_loss,reduce_Qpoint_timestep_pos, reduce_Qpoint_timestep_neg,infil,infilCount) schedule(static)
	//#pragma omp for reduction (+:reduce_evap_loss, reduce_rain_total,reduce_infil_loss,reduce_Qpoint_timestep_pos, reduce_Qpoint_timestep_neg,infil,infilCount) schedule(static)

#else
#pragma omp for reduction(+:reduce_evap_loss, reduce_rain_total,reduce_infil_loss, reduce_Qpoint_timestep_pos, reduce_Qpoint_timestep_neg,infil,infilCount) reduction( max : reduce_Hmax) schedule(static) // xiaodw add
	//#pragma omp for reduction(+:reduce_evap_loss, reduce_rain_total,reduce_infil_loss, reduce_Qpoint_timestep_pos, reduce_Qpoint_timestep_neg) reduction( max : reduce_Hmax) schedule(static)
#endif
	for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
	{
		VolumeHeightUpdateInfo update_info;
		NUMERIC_TYPE block_Hmax = C(0.0);
		NUMERIC_TYPE infilAvgBlock = C(0.0);
		//NUMERIC_TYPE infilAccBlock = C(0.0);
		int infilValidCount = 0;
		// SGC模式下计算下渗、蒸发，在此方法中耦合冰雪融化模块
		block_Hmax = SGC2_UpdateVolumeHeight_block(block_index, grid_cols, grid_rows, grid_cols_padded,
			curr_time, delta_time, depth_thresh, g,
			evap_deltaH_step, evap_grid, rain_deltaH_step, snow_deltaH_step, rain_grid, dist_infil_grid,
			wet_dry_bounds, tmp_row, 
			Qx_grid, Qy_grid,
			cell_area_col, dx_col, dy_col,
			dem_grid,
			sub_grid_layout_rows, sub_grid_state_rows,
			ps_layout, route_dynamic_list,
			h_grid, volume_grid,
			SGC_BankFullHeight_grid,
			Statesptr, Parptr, Fnameptr, Solverptr, Poisptr, SGCptr, Arrptr, &update_info, &infilAvgBlock, &infilValidCount
			//last_gw_time,&Parptr->PercolationVol, Parptr->sumNCells, &Parptr->sumGndQ2Rch, &Parptr->GwStorageDepth
		);
		if (Statesptr->use_green_ampt_singlelayer == ON)
		{
			infil += infilAvgBlock;
			//infilAcc += infilAccBlock;
			infilCount += infilValidCount;
		}
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma omp critical
#endif
		{
			if (block_Hmax > reduce_Hmax)
				reduce_Hmax = block_Hmax;
		}
		reduce_infil_loss += update_info.infil_loss;
		reduce_evap_loss += update_info.evap_loss;
		reduce_rain_total += update_info.rain_total;

		reduce_Qpoint_timestep_pos += update_info.Qpoint_timestep_pos;
		reduce_Qpoint_timestep_neg += update_info.Qpoint_timestep_neg;
	}

	//cout << "*********************************666*********************************** " << endl;

	// Update time of initial flood inundation (in hours) and total inundation time (in seconds)
	if ((Statesptr->reset_timeinit == ON) && (curr_time > Parptr->reset_timeinit_time))
	{
		//reset the time of initial inundation if called for in parameter file
		Statesptr->reset_timeinit = OFF;

#pragma omp for schedule(static)
		for (int j = 0; j < grid_rows; j++)
		{
			for (int i = 0; i < grid_cols; i++)
			{
				int index = i + j*grid_cols_padded;
				initHtm_grid[index] = (NULLVAL);
			}
		}
		if (verbose == ON)
		{
#pragma omp single
			printf("\n Time of initial inundation reset \n");
		}
	}
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
	double stop_time_8;
	gettimeofday(&timstr, NULL);
	stop_time_8 = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	double interval_8 = stop_time_8 - stop_time_7;
	cout << "Do_Update interval 8: " << interval_8 << "s" << endl;
#endif
	NUMERIC_TYPE time_next = curr_time + delta_time;
	if (time_next >= Parptr->MassTotal || Statesptr->mint_hk == OFF)
	{
		NUMERIC_TYPE delta_time_hours = delta_time / C(3600.0);
		NUMERIC_TYPE current_time_hours = time_next / C(3600.0);

#pragma omp for schedule(static) nowait
		for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
		{
			SGC2_Inundation_block(block_index, grid_cols, grid_rows, grid_cols_padded,
				depth_thresh,
				current_time_hours,
				delta_time_hours,
				h_grid,
				sub_grid_layout_rows, sub_grid_state_rows,
				wet_dry_bounds,
				initHtm_grid,
				totalHtm_grid,
				maxH_grid,
				maxHtm_grid);
		}
		if (time_next >= Parptr->MassTotal)
		{
#pragma omp for reduction ( + : reduce_flood_area, reduce_domain_volume) schedule(static) nowait
			for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
			{
				NUMERIC_TYPE out_flood_area, out_domain_volume;
				SGC2_DomainVolumeAndFloodArea_block(block_index, grid_cols, grid_rows, grid_cols_padded, depth_thresh,
					wet_dry_bounds,
					h_grid, cell_area_col, volume_grid, &out_flood_area, &out_domain_volume);
				reduce_flood_area += out_flood_area;
				reduce_domain_volume += out_domain_volume;
			}
		}

		// ensure inundation & volume/area threads finished 
#pragma omp barrier 
	}
#if defined (_XDW_DEBUG) && _XDW_DEBUG > 0
	double stop_time_9;
	gettimeofday(&timstr, NULL);
	stop_time_9 = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	double interval_9 = stop_time_9 - stop_time_8;
	cout << "Do_Update interval 9: " << interval_9 << "s" << endl;
#endif

#if _BALANCE_TYPE == 1
	SGC2_UpdateLoadBalance(grid_rows, grid_cols_padded, sub_grid_layout_rows, wet_dry_bounds);
#endif

	//return reduce_Hmax;
	//cout << "*********************************777*********************************** " << endl;
}




//-----------------------------------------------------------------------------
// ITERATE THROUGH TIME STEPS
///
///
///
/*! \fn void Fast_IterateLoop(...)
\brief
\param
\param
\param h_grid matrix grid_cols_padded*grid_rows
\param cell_area_col column 1*grid_rows
*/


// version of this function without inline
// used by lis2_output.cpp
void SGC2_CalcA_public(int gr, NUMERIC_TYPE hflow, NUMERIC_TYPE bf, NUMERIC_TYPE *A, NUMERIC_TYPE *we, const SGCprams *SGCptr)
{
	SGC2_CalcA(gr, hflow, bf, A, we, SGCptr);
}

// version of this function without inline
// used by lis2_output.cpp
NUMERIC_TYPE SGC2_CalculateVelocity_public(const int index, const int index_next,
	const NUMERIC_TYPE * Q_grid,
	const NUMERIC_TYPE * h_grid, const NUMERIC_TYPE * dem_grid, const NUMERIC_TYPE width)
{
	return SGC2_CalculateVelocity(index, index_next, Q_grid, h_grid, dem_grid, width);
}
void SGC2_CalcLinksQ(SuperGridLinksList * Super_linksptr, NUMERIC_TYPE * volume_grid, const NUMERIC_TYPE * h_grid, 
	const NUMERIC_TYPE delta_time, const NUMERIC_TYPE g, const NUMERIC_TYPE depth_thresh, const NUMERIC_TYPE max_Froude, WetDryRowBound * wet_dry_bounds)
{
	int i;
	// Loop through links
	for (i = 0; i < Super_linksptr->num_links; i++)
	{
		NUMERIC_TYPE h0 = h_grid[Super_linksptr->link_index_SGC[i]] + Super_linksptr->SGC_bfH[i]; // sub grid channel depth
		NUMERIC_TYPE h1 = h_grid[Super_linksptr->link_index_2D[i]];
		NUMERIC_TYPE z0 = Super_linksptr->SGC_z[i];
		NUMERIC_TYPE z1 = Super_linksptr->DEM_z[i];

		NUMERIC_TYPE surface_elevation0 = z0 + h0;
		NUMERIC_TYPE surface_elevation1 = z1 + h1;
		// Calculating hflow based on floodplain levels
		NUMERIC_TYPE hflow = getmax(surface_elevation0, surface_elevation1) - getmax(z0, z1);
		NUMERIC_TYPE q_tmp, surface_slope;
		if (hflow > depth_thresh)
		{
			NUMERIC_TYPE area = (Super_linksptr->dx[i])* hflow;
			NUMERIC_TYPE dh = (surface_elevation0)-(surface_elevation1);
			surface_slope = -dh / Super_linksptr->dx[i];
			q_tmp = CalculateQ(surface_slope, hflow, delta_time, g, area, Super_linksptr->gn2[i], Super_linksptr->Qold[i], max_Froude);
			NUMERIC_TYPE dv = q_tmp*delta_time;
			volume_grid[Super_linksptr->link_index_SGC[i]] -= dv;
			volume_grid[Super_linksptr->link_index_2D[i]] += dv;
		}
		else
		{
			surface_slope = C(0.0);
			q_tmp = C(0.0);
		}
		Super_linksptr->Qold[i] = q_tmp;
		if (q_tmp != C(0.0))
		{
			int x = Super_linksptr->link_index_2D_i[i];
			int y = Super_linksptr->link_index_2D_j[i];
			wet_dry_bounds->fp_vol[y].start = min(wet_dry_bounds->fp_vol[y].start, x);
			wet_dry_bounds->fp_vol[y].end = max(wet_dry_bounds->fp_vol[y].end, x + 1);
		}
	}

}

void Fast_RunStep(Arrays *Arrptr, Files *Fptr, Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, SGCprams * SGCptr, DamData *Damptr, Stage *Locptr,
	LISFLOODFPContext *LFPContext, SuperGridLinksList *Super_linksptr, LfpCouplingInfo * LfpCouplingInfoPtr)
{

#if (_NETCDF == 1)			
	//#pragma omp barrier  // xiaodw
#pragma omp single
	{
		// read in evaporation grid
		// xiaodw comment, we don't need to read evap netcdf now. Let's support it in future
		if (Statesptr->calc_evap == TIME_SPACE) {
			read_file_netCDF(LFPContext->evap_grid, LFPContext->curr_time);
			for (int j = 0; j < LFPContext->grid_cols_padded * LFPContext->grid_rows; j++)
				LFPContext->evap_grid->data[j] /= (86400. * 1000.);  // convert from mm/day to m/s
		}

		// read dynamic rain grid and write to rain grid
		if (Statesptr->rainfallmask)
		{
			// xiaodw 
			int current_timestamp = LFPContext->curr_time + LFPContext->rain_begin_timestamp;
			if (current_timestamp >= LFPContext->last_rain_time && current_timestamp <= LFPContext->rain_end_timestamp)
			{
				// read a new rain tif file
				LFPContext->nextRainTifName = createTifFilename(LFPContext->last_rain_time);
				buildFullFilePath(LFPContext->nextRainTifPath, Fnameptr->rainTifFolder, LFPContext->nextRainTifName);
				readTIFByGdal(LFPContext->nextRainTifPath, LFPContext->rainfall_no_padding);

				for (int j = 0; j < Parptr->ysz; j++)
				{
					int source_row_index = j * LFPContext->grid_cols;
					int dest_row_index = j * LFPContext->grid_cols_padded;

					int padding_count = LFPContext->grid_cols_padded - LFPContext->grid_cols;

					int source_bytes_per_row = sizeof(NUMERIC_TYPE) * LFPContext->grid_cols;
					int dest_bytes_per_row = sizeof(NUMERIC_TYPE) * LFPContext->grid_cols_padded;
					int padding = sizeof(NUMERIC_TYPE) * padding_count;
					// make padding rainfall
					memcpy(LFPContext->rain_grid + dest_row_index, LFPContext->rainfall_no_padding + source_row_index, source_bytes_per_row);
					memset(LFPContext->rain_grid + dest_row_index + LFPContext->grid_cols, 0, padding);
				}
				// update last_rain_time
				LFPContext->last_rain_time += Solverptr->rain_time_step;
			}
		}
	}
#endif

	// sub grid floodplain models
	NUMERIC_TYPE delta_time;
	delta_time = Solverptr->SGCtmpTstep;
	// *******************xiaodw, dhsvm time
	//Parptr->gwTstep = Solverptr->SGCtmpTstep;

	//cout << "*********************************begin do_update*********************************** "<< endl;
	Do_Update(LFPContext->grid_cols, LFPContext->grid_rows, LFPContext->grid_cols_padded, delta_time,
		LFPContext->curr_time, LFPContext->depth_thresh, Solverptr->g, LFPContext->h_grid, LFPContext->volume_grid, LFPContext->Qx_grid, LFPContext->Qy_grid, LFPContext->Qx_old_grid, LFPContext->Qy_old_grid,
		LFPContext->initHtm_grid, LFPContext->maxHtm_grid, LFPContext->totalHtm_grid, LFPContext->maxH_grid,
		LFPContext->maxVc_grid, LFPContext->maxVc_height_grid, LFPContext->maxHazard_grid,
		LFPContext->Vx_grid, LFPContext->Vy_grid, LFPContext->Vx_max_grid, LFPContext->Vy_max_grid,
		LFPContext->SGC_BankFullHeight_grid,
		LFPContext->dem_grid, LFPContext->g_friction_sq_x_grid, LFPContext->g_friction_sq_y_grid,
		LFPContext->friction_x_grid, LFPContext->friction_y_grid,
		LFPContext->dx_col, LFPContext->dy_col, LFPContext->cell_area_col,
		LFPContext->Fp_xwidth, LFPContext->Fp_ywidth,
		LFPContext->sub_grid_layout_rows, LFPContext->sub_grid_state_rows, LFPContext->sub_grid_layout_blocks, LFPContext->sub_grid_state_blocks,
		LFPContext->evap_time_series, LFPContext->evap_grid, LFPContext->rain_time_series, LFPContext->temperature_time_series, LFPContext->rain_grid, LFPContext->dist_infil_grid, LFPContext->wet_dry_bounds, LFPContext->ps_layout, LFPContext->boundary_cond,
		LFPContext->weirs_weirs, LFPContext->weirs_bridges,
		LFPContext->route_dynamic_list, LFPContext->route_V_ratio_per_sec_qx, LFPContext->route_V_ratio_per_sec_qy,
		Statesptr, Parptr, Fnameptr, Solverptr, Poisptr, Arrptr, SGCptr, Fptr, Locptr, Damptr, Super_linksptr,
		LFPContext->tmp_thread_data, LFPContext->tmp_thread_data_ch,
		LFPContext->verbose, LFPContext->last_gw_time, LfpCouplingInfoPtr);
#pragma omp barrier
#pragma omp single
	{

		Solverptr->Tstep = delta_time;
#if _DISABLE_WET_DRY == 1
		for (int j = 0; j < grid_rows; j++)
		{
			wet_dry_bounds->fp_h[j].start = 0;
			wet_dry_bounds->fp_h[j].end = grid_cols;
		}
#endif
		for (int j = 0; j < LFPContext->grid_rows; j++)
		{
			LFPContext->wet_dry_bounds->fp_vol[j].start = LFPContext->wet_dry_bounds->fp_h[j].start;
			LFPContext->wet_dry_bounds->fp_vol[j].end = LFPContext->wet_dry_bounds->fp_h[j].end;
		}
		Parptr->EvapTotalLoss += reduce_evap_loss;
		Parptr->InfilTotalLoss += reduce_infil_loss;
		Parptr->RainTotalLoss += reduce_rain_total;
		LFPContext->ps_layout->Qpoint_pos = reduce_Qpoint_timestep_pos / delta_time;
		LFPContext->ps_layout->Qpoint_neg = reduce_Qpoint_timestep_neg / delta_time;

		// Point sources
		// xiaodw，关键：这里才是point source作为输入和输出流量边界，汇总到boundary_cond。
		// Qpoint_pos的最终来源是SGC2_PointSources_Vol_row和SGC2_PointSources_H_row，专门计算点源输入和输出
		LFPContext->boundary_cond->Qin += LFPContext->ps_layout->Qpoint_pos;
		// Qout 是所有出水口的流量，输出的时候，打印的是对应时刻的流量，不是massint内对应的平均流量或总流量
		LFPContext->boundary_cond->Qout -= LFPContext->ps_layout->Qpoint_neg;

		// calculate volume in and volume out
		LFPContext->boundary_cond->VolInMT += LFPContext->boundary_cond->Qin*delta_time;
		LFPContext->boundary_cond->VolOutMT += LFPContext->boundary_cond->Qout*delta_time;

		Solverptr->vol2 = reduce_domain_volume;
		Solverptr->FArea = reduce_flood_area;

		// xdw modify, 不用自适应时间步长，改用固定时间步长试试
		Solverptr->SGCtmpTstep = getmin(Solverptr->cfl*Parptr->min_dx_dy*Parptr->SGC_m / (SQRT(Solverptr->g * getmax(reduce_Hmax, Damptr->DamMaxH))), Solverptr->InitTstep);
		//Solverptr->SGCtmpTstep = Solverptr->InitTstep;

		// xdw add, 在这里输出POI的降雨、流量、下渗、水量
		if (LFPContext->curr_time >= Parptr->PoiSaveTotal && Statesptr->save_poi == ON)
		{
			// todo 
			int x, y, poi_index, poi_index_no_padding;
			for (int k = 0; k < Poisptr->num; k++)
			{
				// 第k个poi点就对应编号为k的pois文件
				x = Poisptr->xpi[k];
				y = Poisptr->ypi[k];
				poi_index = y * LFPContext->grid_cols_padded + x;
				poi_index_no_padding = y * LFPContext->grid_cols + x;
				// m->mm/h
				NUMERIC_TYPE rain_rate = Poisptr->Rain_Grid[poi_index] * 1000 * 3600 / Parptr->PoiSaveInt;
				//Poisptr->Rain_Grid[poi_index] = 0.0;
				if (Statesptr->use_snow_glacier == ON)
				{
					NUMERIC_TYPE snow_rate = Poisptr->Snow_Grid[poi_index] * 1000 * 3600 / delta_time;
					NUMERIC_TYPE freeze_rate = Poisptr->Freeze_Grid[poi_index] * 1000 * 3600 / delta_time;
					NUMERIC_TYPE snow_melt_rate = Poisptr->SnowMelt_Grid[poi_index] * 1000 * 3600 / delta_time;
					NUMERIC_TYPE snow_thickness = Parptr->snow[poi_index_no_padding] * 1000;
					NUMERIC_TYPE glacier_melt_rate = Poisptr->GlacierMelt_Grid[poi_index] * 1000 * 3600 / delta_time;
					NUMERIC_TYPE glacier_thickness = Parptr->glacier[poi_index_no_padding] * 1000;
				}
				NUMERIC_TYPE infilt_rate = Poisptr->Infilt_Grid[poi_index] * 0.1 * 3600 / Parptr->PoiSaveInt;   // mm -> cm/h 
				//Poisptr->Infilt_Grid[poi_index] = 0.0;
				//NUMERIC_TYPE soilWaterDepth = Parptr->soilWaterDepth[poi_index] * 1000;
				NUMERIC_TYPE soilWaterDepth = 0.0;
				NUMERIC_TYPE soil_moisture;
				if (Statesptr->use_green_ampt_singlelayer == ON)
				{
					soil_moisture = Parptr->soilMoisturePD[poi_index];
				}
				else {
					soil_moisture = 0.0;
				}
				if (Statesptr->use_snow_glacier == ON) {
					// with snow and glacier

					fprintf(Fptr->pois_fp[k], "%-12.3" NUM_FMT" %-10.4" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT "\n",
						LFPContext->curr_time, delta_time, Poisptr->Rain_Grid[poi_index], Poisptr->Snow_Grid[poi_index] * 1000, Poisptr->Freeze_Grid[poi_index] * 1000, Poisptr->SnowMelt_Grid[poi_index] * 1000, Poisptr->GlacierMelt_Grid[poi_index] * 1000, Poisptr->Infilt_Grid[poi_index] * 1000,
						soilWaterDepth, Poisptr->Qx_Grid[poi_index], (LFPContext->h_grid[poi_index] + LFPContext->SGC_BankFullHeight_grid[poi_index]) * 1000, Poisptr->Vol_Grid[poi_index]);
				}
				else if (Statesptr->use_dhsvm) {
					NUMERIC_TYPE Rain_Grid = Poisptr->Rain_Grid[poi_index] - Poisptr->Rain_Grid_Last[poi_index];
					NUMERIC_TYPE Infilt_Grid = Poisptr->Infilt_Grid[poi_index] - Poisptr->Infilt_Grid_Last[poi_index];
					NUMERIC_TYPE InfiltCh_Grid = Poisptr->InfiltCh_Grid[poi_index] - Poisptr->InfiltCh_Grid_Last[poi_index];
					NUMERIC_TYPE Evap_Grid = Poisptr->Evap_Grid[poi_index] - Poisptr->Evap_Grid_Last[poi_index];
					NUMERIC_TYPE Qx_Grid = Poisptr->Qx_Grid[poi_index] - Poisptr->Qx_Grid_Last[poi_index];
					NUMERIC_TYPE Qy_Grid = Poisptr->Qy_Grid[poi_index] - Poisptr->Qy_Grid_Last[poi_index];
					NUMERIC_TYPE Q_Ch = Poisptr->Q_Ch[poi_index] - Poisptr->Q_Ch_Last[poi_index];
					NUMERIC_TYPE surf_water_depth_Grid_delta = Poisptr->surf_water_depth_Grid[poi_index] - Poisptr->surf_water_depth_Grid_Last[poi_index];
					NUMERIC_TYPE soil_lat_flowin_Grid_allLyr = Poisptr->soil_lat_flowin_Grid_allLyr[poi_index] - Poisptr->soil_lat_flowin_Grid_allLyr_Last[poi_index];
					NUMERIC_TYPE soil_lat_flowout_Grid_allLyr = Poisptr->soil_lat_flowout_Grid_allLyr[poi_index] - Poisptr->soil_lat_flowout_Grid_allLyr_Last[poi_index];
					fprintf(Fptr->pois_fp[k], "%-13.3" NUM_FMT"%-12.3" NUM_FMT"%-14.6" NUM_FMT"%-13.6" NUM_FMT"%-13.6" NUM_FMT"%-13.6" NUM_FMT"%-13.6" NUM_FMT"%-13.6" NUM_FMT"%-15.6" NUM_FMT "%-15.6" NUM_FMT "%-15.6" NUM_FMT " %-14.6" NUM_FMT" %-14.6" NUM_FMT" %-14.6" NUM_FMT,
						LFPContext->curr_time, delta_time, Rain_Grid, Infilt_Grid, InfiltCh_Grid, Evap_Grid, Qx_Grid, Qy_Grid, Q_Ch, LFPContext->h_grid[poi_index] * 1000, Poisptr->Vol_Grid[poi_index], surf_water_depth_Grid_delta, soil_lat_flowin_Grid_allLyr, soil_lat_flowout_Grid_allLyr);
					/*fprintf(Fptr->pois_fp[k], "%-13.3" NUM_FMT"%-12.3" NUM_FMT"%-14.3" NUM_FMT"%-13.5" NUM_FMT"%-13.5" NUM_FMT"%-13.3" NUM_FMT"%-13.3" NUM_FMT "%-13.3" NUM_FMT "%-13.3" NUM_FMT " %-14.3" NUM_FMT" %-14.6" NUM_FMT" %-14.6" NUM_FMT,
						curr_time, delta_time, Poisptr->Rain_Grid[poi_index], Poisptr->Infilt_Grid[poi_index], Poisptr->Evap_Grid[poi_index], Poisptr->Qx_Grid[poi_index], Poisptr->Q_Ch[poi_index], h_grid[poi_index] * 1000, Poisptr->Vol_Grid[poi_index], Poisptr->surf_water_depth_Grid[poi_index], soil_lat_flowin_Grid_allLyr, soil_lat_flowout_Grid_allLyr);*/

					Poisptr->Rain_Grid_Last[poi_index] = Poisptr->Rain_Grid[poi_index];
					Poisptr->Infilt_Grid_Last[poi_index] = Poisptr->Infilt_Grid[poi_index];
					Poisptr->InfiltCh_Grid_Last[poi_index] = Poisptr->InfiltCh_Grid[poi_index];
					Poisptr->Evap_Grid_Last[poi_index] = Poisptr->Evap_Grid[poi_index];
					Poisptr->Qx_Grid_Last[poi_index] = Poisptr->Qx_Grid[poi_index];
					Poisptr->Qy_Grid_Last[poi_index] = Poisptr->Qy_Grid[poi_index];
					Poisptr->Q_Ch_Last[poi_index] = Poisptr->Q_Ch[poi_index];
					Poisptr->surf_water_depth_Grid_Last[poi_index] = Poisptr->surf_water_depth_Grid[poi_index];
					Poisptr->soil_lat_flowin_Grid_allLyr_Last[poi_index] = Poisptr->soil_lat_flowin_Grid_allLyr[poi_index];
					Poisptr->soil_lat_flowout_Grid_allLyr_Last[poi_index] = Poisptr->soil_lat_flowout_Grid_allLyr[poi_index];
					// 每一层
					for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
					{
						NUMERIC_TYPE soil_lat_flowin_Grid_lyr = Poisptr->soil_lat_flowin_Grid[lyr][poi_index] - Poisptr->soil_lat_flowin_Grid_Last[lyr][poi_index];
						NUMERIC_TYPE soil_lat_flowout_Grid_lyr = Poisptr->soil_lat_flowout_Grid[lyr][poi_index] - Poisptr->soil_lat_flowout_Grid_Last[lyr][poi_index];
						NUMERIC_TYPE soil_perco_Grid_lyr = Poisptr->soil_perc_Grid[lyr][poi_index] - Poisptr->soil_perc_Grid_Last[lyr][poi_index];
						NUMERIC_TYPE soil_water_depth_delta = Poisptr->soil_water_depth_Grid[lyr][poi_index] - Poisptr->soil_water_depth_Grid_Last[lyr][poi_index];
						fprintf(Fptr->pois_fp[k], "%-16.6" NUM_FMT"%-18.6" NUM_FMT"%-18.6" NUM_FMT"%-20.6" NUM_FMT "%-20.6" NUM_FMT, soil_perco_Grid_lyr, soil_lat_flowin_Grid_lyr, soil_lat_flowout_Grid_lyr, Poisptr->soil_water_depth_Grid[lyr][poi_index], soil_water_depth_delta);
						Poisptr->soil_lat_flowin_Grid_Last[lyr][poi_index] = Poisptr->soil_lat_flowin_Grid[lyr][poi_index];
						Poisptr->soil_lat_flowout_Grid_Last[lyr][poi_index] = Poisptr->soil_lat_flowout_Grid[lyr][poi_index];
						Poisptr->soil_perc_Grid_Last[lyr][poi_index] = Poisptr->soil_perc_Grid[lyr][poi_index];
						Poisptr->soil_water_depth_Grid_Last[lyr][poi_index] = Poisptr->soil_water_depth_Grid[lyr][poi_index];

					}

					fprintf(Fptr->pois_fp[k], "\n");
				}
				else {
					fprintf(Fptr->pois_fp[k], "%-12.3" NUM_FMT" %-10.4" NUM_FMT" %-10.4" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT " %-13.6" NUM_FMT "\n",
						LFPContext->curr_time, delta_time, rain_rate, infilt_rate,
						soilWaterDepth, soil_moisture, Poisptr->Qx_Grid[poi_index], LFPContext->h_grid[poi_index] * 1000, Poisptr->Vol_Grid[poi_index]);
				}

				fflush(Fptr->pois_fp[k]);
			}
			// 每个时间步长结束之后，将格网的值重置为0

			for (int j = 0; j < LFPContext->grid_rows; j++)
			{
				int dest_row_index = j * LFPContext->grid_cols_padded;
				int padding_count = LFPContext->grid_cols_padded - LFPContext->grid_cols;
				int padding = sizeof(NUMERIC_TYPE) * padding_count;
				if (Statesptr->use_snow_glacier == ON) {
					memset(Poisptr->Snow_Grid + dest_row_index, 0, sizeof(NUMERIC_TYPE) * LFPContext->grid_cols_padded);
					memset(Poisptr->SnowMelt_Grid + dest_row_index, 0, sizeof(NUMERIC_TYPE) * LFPContext->grid_cols_padded);
					memset(Poisptr->GlacierMelt_Grid + dest_row_index, 0, sizeof(NUMERIC_TYPE) * LFPContext->grid_cols_padded);
					memset(Poisptr->Freeze_Grid + dest_row_index, 0, sizeof(NUMERIC_TYPE) * LFPContext->grid_cols_padded);
					memset(Poisptr->Snow_Grid + dest_row_index + LFPContext->grid_cols, -1, padding);
					memset(Poisptr->SnowMelt_Grid + dest_row_index + LFPContext->grid_cols, -1, padding);
					memset(Poisptr->GlacierMelt_Grid + dest_row_index + LFPContext->grid_cols, -1, padding);
					memset(Poisptr->Freeze_Grid + dest_row_index + LFPContext->grid_cols, -1, padding);
				}

				memset(Poisptr->Vol_Grid + dest_row_index, 0, sizeof(NUMERIC_TYPE) * LFPContext->grid_cols_padded);
				memset(Poisptr->Vol_Grid + dest_row_index + LFPContext->grid_cols, -1, padding);
			}
			Parptr->PoiSaveTotal += Parptr->PoiSaveInt;
		}
		// Calculate mass balance error (very rare)
		if (LFPContext->curr_time >= Parptr->MassTotal)
		{
			// calc losses for this mass interval
			NUMERIC_TYPE loss = (Parptr->InfilTotalLoss - Parptr->InfilLoss) + (Parptr->EvapTotalLoss - Parptr->EvapLoss) - (Parptr->RainTotalLoss - Parptr->RainLoss);

			//Solverptr->Qerror=boundary_cond->Qin-boundary_cond->Qout-(Solverptr->vol2+loss-Solverptr->vol1)/Parptr->MassInt;
			// New version using VolInMT and VolOutMT
			// volume error

			Solverptr->Verror = LFPContext->boundary_cond->VolInMT - LFPContext->boundary_cond->VolOutMT - (Solverptr->vol2 + loss - Solverptr->vol1) + Damptr->DamLoss;

			// Q error
			Solverptr->Qerror = Solverptr->Verror / Parptr->MassInt;
			// reset to 0.0
			LFPContext->boundary_cond->VolInMT = C(0.0);
			LFPContext->boundary_cond->VolOutMT = C(0.0);
			Damptr->DamLoss = C(0.0);
			double infil_rate = (Parptr->InfilTotalLoss - Parptr->InfilLoss) / Parptr->MassInt;  // m3/s
			double evap_rate = (Parptr->EvapTotalLoss - Parptr->EvapLoss) / Parptr->MassInt;  // m3/s
			double rain_rate = (Parptr->RainTotalLoss - Parptr->RainLoss) / Parptr->MassInt;// m3/s
			double rain_depth_rate_per_cell = rain_rate / Parptr->sumNCells / Parptr->avgCellArea * 1000.0 * 3600;  //mm/h
			double infil_depth_rate_per_cell = infil_rate / Parptr->sumNCells / Parptr->avgCellArea * 1000.0 * 3600;  //mm/h
			double evap_depth_rate_per_cell = evap_rate / Parptr->sumNCells / Parptr->avgCellArea * 1000.0 * 3600;  //mm/h

			double interflow_gen_rate = 0.0;
			double interflow_runoff_rate = 0.0;
			double interflow_2ch_rate = 0.0;

			double interflow_gen_rate_lyr = 0.0;
			double interflow_runoff_rate_lyr = 0.0;
			double interflow_2ch_rate_lyr = 0.0;

			double subSurfaceLatFlow2Channel_rate = 0.0;
			double subSurfaceLatFlow2Surf_rate = 0.0;

			double interflow_2ch_depth_rate_per_cell = 0.0;
			if (Statesptr->use_interflow_singlelayer == ON)
			{
				interflow_gen_rate = (Parptr->InterflowGenTotal - Parptr->InterflowGen) / Parptr->MassInt;  // m3/s
				interflow_runoff_rate = (Parptr->InterflowRunoffTotal - Parptr->InterflowRunoff) / Parptr->MassInt;  // m3/s
				interflow_2ch_rate = (Parptr->Interflow2ChTotal - Parptr->Interflow2Ch) / Parptr->MassInt;  // m3/s
				interflow_2ch_depth_rate_per_cell = interflow_2ch_rate / Parptr->sumNCells / Parptr->avgCellArea * 1000.0 * 3600;  //mm/h
				Parptr->InterflowGen = Parptr->InterflowGenTotal;
				Parptr->InterflowRunoff = Parptr->InterflowRunoffTotal;
				Parptr->Interflow2Ch = Parptr->Interflow2ChTotal;
			}
			double perclation_rate_vol = 0.0;
			double perclation_rate_depth = 0.0;
			if (Statesptr->use_percolation_singlelayer == ON)
			{
				perclation_rate_vol = Parptr->PercolationVol / Parptr->gwTstep;  //m3/s
				perclation_rate_depth = Parptr->PercolationDepth / Parptr->gwTstep * 3600;   //mm/h
			}
			double multi_soilWaterDepthOFLyr = 0.0;
			double multi_soilMoistureOfLyr = 0.0;
			double multi_soilPercoVolOfLyr = 0.0;
			double multi_soilPercoDepOfLyr = 0.0;

			// record cumulative loss for next time.
			Parptr->InfilLoss = Parptr->InfilTotalLoss;
			Parptr->EvapLoss = Parptr->EvapTotalLoss;
			Parptr->RainLoss = Parptr->RainTotalLoss;


#ifdef _DEBUGxx
			fprintf(Fptr->mass_fp, "%-12.3" NUM_FMT" %-10.4" NUM_FMT" %-10.4" NUM_FMT" %-10ld %12.4" NUM_FMT" %12.4" NUM_FMT"  %-11.3" NUM_FMT" %-10.3" NUM_FMT" %-11.3" NUM_FMT" %12.4" NUM_FMT" %12.4" NUM_FMT" %12.4" NUM_FMT"\n",
				curr_time,
				delta_time,
				Solverptr->MinTstep,
				Solverptr->itCount,
				fix_small_negative(Solverptr->FArea),
				fix_small_negative(Solverptr->vol2),
				fix_small_negative(boundary_cond->Qin),
				fix_small_negative(Solverptr->Hds),
				fix_small_negative(boundary_cond->Qout),
				fix_small_negative(Solverptr->Qerror),
				fix_small_negative(Solverptr->Verror),
				fix_small_negative(Parptr->RainTotalLoss - (Parptr->InfilTotalLoss + Parptr->EvapTotalLoss)));
#else
			fprintf(Fptr->mass_fp, "%-12.3" NUM_FMT" %-10.4" NUM_FMT" %-10.4" NUM_FMT" %-10li %12.4e %12.4e  %-11.3" NUM_FMT" %-10.3" NUM_FMT" %-15.6" NUM_FMT" %12.4e %12.4e %12.4e       %12.4e %12.4e %12.4e %12.4e %12.4e %12.4e ",
				LFPContext->curr_time, delta_time, Solverptr->MinTstep, Solverptr->itCount, Solverptr->FArea, Solverptr->vol2, LFPContext->boundary_cond->Qin,
				Solverptr->Hds, LFPContext->boundary_cond->Qout, Solverptr->Qerror, Solverptr->Verror, Parptr->RainTotalLoss - (Parptr->InfilTotalLoss + Parptr->EvapTotalLoss),
				rain_rate, rain_depth_rate_per_cell, infil_rate, infil_depth_rate_per_cell, evap_rate, evap_depth_rate_per_cell);
			if (Statesptr->use_interflow_singlelayer == ON)
			{
				//Parptr->GroundWaterQ2RiverTotal = Parptr->GroundWaterQ2RiverTotal / Parptr->MassInt;
				fprintf(Fptr->mass_fp, " %12.3" NUM_FMT" %12.3" NUM_FMT" %16.3" NUM_FMT" %16.3" NUM_FMT, interflow_gen_rate, Parptr->InterflowRunoffTotal, interflow_runoff_rate, interflow_2ch_rate);
			}
			if (Statesptr->use_interflow_multilayer == ON)
			{
				// 每一层
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					// 每一时步壤中流产生的量
					interflow_gen_rate_lyr = (Parptr->multi_interflowGenVolOfLyr[lyr] - Parptr->multi_interflowGenVolOfLyr_Last[lyr]) / Parptr->MassInt;
					// 每一时步壤中流产生的地表径流进入河道的径流量
					interflow_2ch_rate_lyr = (Parptr->multi_interflow2ChVolOfLyr[lyr] - Parptr->multi_interflow2ChVolOfLyr_Last[lyr]) / Parptr->MassInt;
					// 每一时步由于壤中流导致的地表径流总量 的变化率
					interflow_runoff_rate_lyr = (Parptr->multi_interflowRunoffVolOfLyr[lyr] - Parptr->multi_interflowRunoffVolOfLyr_Last[lyr]) / Parptr->MassInt;
					fprintf(Fptr->mass_fp, "%18.5" NUM_FMT"%16.5" NUM_FMT"%18.5" NUM_FMT"%18.5" NUM_FMT, interflow_gen_rate_lyr, Parptr->multi_interflowRunoffVolOfLyr[lyr], interflow_runoff_rate_lyr, interflow_2ch_rate_lyr);
				}
				// 所有层总的
				interflow_gen_rate = (Parptr->InterflowGenTotal - Parptr->InterflowGen) / Parptr->MassInt;  // m3/s
				interflow_runoff_rate = (Parptr->InterflowRunoffTotal - Parptr->InterflowRunoff) / Parptr->MassInt;  // m3/s
				interflow_2ch_rate = (Parptr->Interflow2ChTotal - Parptr->Interflow2Ch) / Parptr->MassInt;  // m3/s
				fprintf(Fptr->mass_fp, "%16.3" NUM_FMT"%14.3" NUM_FMT"%16.3" NUM_FMT"%16.3" NUM_FMT, interflow_gen_rate, Parptr->InterflowRunoffTotal, interflow_runoff_rate, interflow_2ch_rate);
				Parptr->InterflowGen = Parptr->InterflowGenTotal;
				Parptr->InterflowRunoff = Parptr->InterflowRunoffTotal;
				Parptr->Interflow2Ch = Parptr->Interflow2ChTotal;
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					Parptr->multi_interflowGenVolOfLyr_Last[lyr] = Parptr->multi_interflowGenVolOfLyr[lyr];
					Parptr->multi_interflow2ChVolOfLyr_Last[lyr] = Parptr->multi_interflow2ChVolOfLyr[lyr];
					Parptr->multi_interflowRunoffVolOfLyr_Last[lyr] = Parptr->multi_interflowRunoffVolOfLyr[lyr];
				}
			}

			if (Statesptr->use_percolation_singlelayer == ON)
			{
				//Parptr->GroundWaterQ2RiverTotal = Parptr->GroundWaterQ2RiverTotal / Parptr->MassInt;
				fprintf(Fptr->mass_fp, " %12.3" NUM_FMT" %12.3" NUM_FMT" %12.3" NUM_FMT" %12.3" NUM_FMT, Parptr->soilMoisAvgPerCell, Parptr->soilWaterDepthAvgPerCell, perclation_rate_vol, perclation_rate_depth);
				Parptr->PercolationVol = 0.f;
				Parptr->PercolationDepth = 0.f;
			}
			if (Statesptr->use_percolation_multilayer == ON)
			{
				// 每一层
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					// 每一层的平均土壤水深
					multi_soilWaterDepthOFLyr = (Parptr->multi_soilWaterDepthOfLyr[lyr]) / Parptr->sumNCells;
					// 当前MassInt时步内，每一层内平均每个栅格渗漏水量的速率
					multi_soilPercoVolOfLyr = (Parptr->multi_soilPercoVolOfLyr[lyr] - Parptr->multi_soilPercoVolOfLyr_Last[lyr]) / Parptr->MassInt / Parptr->sumNCells;
					// 当前MassInt时步内，每一层渗漏水深的速率
					multi_soilPercoDepOfLyr = (Parptr->multi_soilPercoDepOfLyr[lyr] - Parptr->multi_soilPercoDepOfLyr_Last[lyr]) / Parptr->MassInt / Parptr->sumNCells;
					// 每一层的平均土壤湿度
					multi_soilMoistureOfLyr = (Parptr->multi_soilMoistureOfLyr[lyr]) / Parptr->sumNCells;
					fprintf(Fptr->mass_fp, "%15.3" NUM_FMT"%14.3" NUM_FMT"%16.3" NUM_FMT"%16.3" NUM_FMT"%14.3" NUM_FMT"%18.3" NUM_FMT,
						multi_soilMoistureOfLyr, multi_soilWaterDepthOFLyr, multi_soilPercoVolOfLyr, multi_soilPercoDepOfLyr, Parptr->multi_soilFcOfLyr[lyr], Parptr->multi_soilProsityOfLyr[lyr]);
				}
				multi_soilMoistureOfLyr = 0.0;
				multi_soilPercoVolOfLyr = 0.0;
				multi_soilPercoDepOfLyr = 0.0;
				multi_soilWaterDepthOFLyr = 0.0;
				multi_soilWaterDepthOFLyr = 0.0;

				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					Parptr->multi_soilPercoVolOfLyr_Last[lyr] = Parptr->multi_soilPercoVolOfLyr[lyr];
					Parptr->multi_soilPercoDepOfLyr_Last[lyr] = Parptr->multi_soilPercoDepOfLyr[lyr];
				}

			}

			if (Statesptr->use_groundwater == ON)
			{
				fprintf(Fptr->mass_fp, "%14.3" NUM_FMT"%14.3" NUM_FMT"%14.5" NUM_FMT"%14.5" NUM_FMT, Parptr->GwStorageVol, Parptr->GwStorageDepth, Parptr->sumGndQ2Rch, Parptr->gwQPerSgcCell);
				Parptr->PercolationVol = 0.f;
				Parptr->PercolationDepth = 0.f;
			}

			if (Statesptr->use_dhsvm == ON)
			{
				//if (curr_time - last_gw_time >= Parptr->gwTstep) {
				//	last_gw_time = curr_time;
				Parptr->subSurfaceLatFlow2Channel_rate = (Parptr->subSurfaceLatFlow2ChTotal - Parptr->subSurfaceLatFlowTotal2Ch_Last) / Parptr->MassInt;
				Parptr->subSurfaceLatFlowTotal2Ch_Last = Parptr->subSurfaceLatFlow2ChTotal;
				Parptr->subSurfaceLatFlow2Surf_rate = (Parptr->subSurfaceLatFlow2SurfTotal - Parptr->subSurfaceLatFlowTotal2Surf_Last) / Parptr->MassInt;
				Parptr->subSurfaceLatFlowTotal2Surf_Last = Parptr->subSurfaceLatFlow2SurfTotal;
				Parptr->subSurfacePerc2Surf_rate = (Parptr->subSurfacePerc2SurfTotal - Parptr->subSurfacePercTotal2Surf_Last) / Parptr->MassInt;
				Parptr->subSurfacePercTotal2Surf_Last = Parptr->subSurfacePerc2SurfTotal;
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					Parptr->multi_soilPercoDepOfLyr_rate[lyr] = (Parptr->multi_soilPercoDepOfLyr[lyr] - Parptr->multi_soilPercoDepOfLyr_Last[lyr]) * 3600.0 / Parptr->MassInt;
					//Parptr->multi_soilPercoDepOfLyr_rate[lyr] = (Parptr->multi_soilPercoDepOfLyr[lyr] - Parptr->multi_soilPercoDepOfLyr_Last[lyr])  / Parptr->MassInt;
					Parptr->multi_soilPercoDepOfLyr_Last[lyr] = Parptr->multi_soilPercoDepOfLyr[lyr];
				}
				//}

				Parptr->surfaceFlow2Ch_rate = (Parptr->surfaceFlow2ChTotal - Parptr->surfaceFlow2ChTotal_Last) / Parptr->MassInt;
				Parptr->surfaceFlow2ChTotal_Last = Parptr->surfaceFlow2ChTotal;

				Parptr->surfaceHydro2Ch_rate = (Parptr->surfaceHydro2ChTotal - Parptr->surfaceHydro2ChTotal_Last) / Parptr->MassInt;
				Parptr->surfaceHydro2ChTotal_Last = Parptr->surfaceHydro2ChTotal;

				fprintf(Fptr->mass_fp, " %17.3" NUM_FMT" %19.3" NUM_FMT" %19.3" NUM_FMT" %19.3" NUM_FMT" %19.3" NUM_FMT,
					Parptr->subSurfaceLatFlow2Channel_rate, Parptr->subSurfaceLatFlow2Surf_rate, Parptr->subSurfacePerc2Surf_rate, Parptr->surfaceFlow2Ch_rate, Parptr->surfaceHydro2Ch_rate);
				// 每一层
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					multi_soilWaterDepthOFLyr = (Parptr->multi_soilWaterDepthOfLyr[lyr]) / Parptr->sumNCells;
					multi_soilPercoDepOfLyr = (Parptr->multi_soilPercoDepOfLyr_rate[lyr]) / Parptr->sumNCells;
					fprintf(Fptr->mass_fp, "%18.3" NUM_FMT "%18.5" NUM_FMT, multi_soilWaterDepthOFLyr, multi_soilPercoDepOfLyr);
				}
			}

			if (Statesptr->use_green_ampt_singlelayer == ON)
			{
				Parptr->InfilRateGA = C(0.0);
			}
			fprintf(Fptr->mass_fp, "\n");
#endif

			fflush(Fptr->mass_fp); // force program to flush buffer to file - keeps file in sync with writes - user sometimes tracks progress through the file.
			if (Statesptr->DamMode == ON)
			{
				// Dam Output
				fprintf(Fptr->dam_fp, "%-12.3" NUM_FMT" %-10.4" NUM_FMT"", LFPContext->curr_time, delta_time);
				for (int i = 0; i < Damptr->NumDams; i++)
				{
					fprintf(Fptr->dam_fp, "%-10.3" NUM_FMT" %-10.3" NUM_FMT" %-10.3" NUM_FMT" %-10.3" NUM_FMT" %-10.3" NUM_FMT" %-10.4" NUM_FMT" %-10.4" NUM_FMT" %-10.4" NUM_FMT"\n", Damptr->DamArea[i], Damptr->DamVol[i], Damptr->DamVin[i], Damptr->InitialHeight[i], Damptr->DamTotalQ[i] * delta_time, Damptr->SpillQ[i], Damptr->DamOperationQ[i], C(0.0));
				}
				fflush(Fptr->dam_fp);
			}
			Solverptr->vol1 = Solverptr->vol2;
			Parptr->MassTotal += Parptr->MassInt;


			//stage output
			if (Statesptr->save_stages == ON)
			{
				fprintf(Fptr->stage_fp, "%12.3" NUM_FMT"", LFPContext->curr_time);
				for (int i = 0; i < Locptr->Nstages; i++)
				{
					if (Locptr->stage_check[i] == 1)
					{
						int grid_index = Locptr->stage_grid_x[i] + Locptr->stage_grid_y[i] * LFPContext->grid_cols_padded;
						fprintf(Fptr->stage_fp, "%10.4" NUM_FMT"", LFPContext->h_grid[grid_index] + LFPContext->SGC_BankFullHeight_grid[grid_index]);
					}
					else
						fprintf(Fptr->stage_fp, "-\t");
				}
				fprintf(Fptr->stage_fp, "\n");
				fflush(Fptr->stage_fp); // force program to flush buffer to file - keeps file in sync with writes - user sometimes tracks progress through the file.
				// added to export scalar velocity
				// velocity moved to do_update - between update q and update h
			}

			//virtual gauge output
			if (Statesptr->gsection == ON)
			{
				fprintf(Fptr->gau_fp, "%12.2" NUM_FMT"", LFPContext->curr_time); // print tiem to file
				for (int i = 0; i < Locptr->Ngauges; i++) // loop through each virtual gauge
				{
					// call discharge calculation function
					NUMERIC_TYPE discharge = CalcVirtualGauge(i, LFPContext->grid_cols_padded, LFPContext->Qx_grid, LFPContext->Qy_grid, Locptr);
					fprintf(Fptr->gau_fp, " %10.3" NUM_FMT"", discharge); // Print discharge to file
				}
				fprintf(Fptr->gau_fp, "\n"); // print end of line
				fflush(Fptr->gau_fp); // force program to flush buffer to file - keeps file in sync with writes - user sometimes tracks progress through the file.
			}

		}
		// xiaodw, for groundwater timestep calculation
		// export maximum depth interval
		if (Statesptr->maxint == ON && LFPContext->curr_time >= Parptr->maxintTotal)
		{
			// update h to be 'depth' above sub-grid-channel
			for (int j = 0; j < LFPContext->grid_rows; j++)
			{
				for (int i = 0; i < LFPContext->grid_cols; i++)
				{
					int index_padded = i + j * LFPContext->grid_cols_padded;
					int index = i + j * LFPContext->grid_cols;
					NUMERIC_TYPE temp = LFPContext->maxH_grid[index_padded] + LFPContext->SGC_BankFullHeight_grid[index_padded];
					// depth in channel or above flood plain = h + BankFullHeight (should not be negative)
					// note previous version just dumped the wd with no depth_thresh truncation
					if (temp <= LFPContext->depth_thresh)
						temp = C(0.0);
					LFPContext->tmp_grid1[index] = temp;
				}
			}
			write_grid(Fnameptr->resrootname, Parptr->maxintcount,
				NETCDF_IGNORE, ".wd_max",
				LFPContext->tmp_grid1, LFPContext->grid_cols, LFPContext->grid_rows, Parptr->blx, Parptr->bly, Parptr->dx, &Statesptr->output_params);


			// update h to be 'depth' above sub-grid-channel
			for (int j = 0; j < LFPContext->grid_rows; j++)
			{
				for (int i = 0; i < LFPContext->grid_cols; i++)
				{
					int index_padded = i + j * LFPContext->grid_cols_padded;
					int index = i + j * LFPContext->grid_cols;
					LFPContext->maxH_grid[index_padded] = C(0.0); // rests max depth to zero at save interval
				}
			}

			// update interval counter
			Parptr->maxintTotal += Parptr->maxint;
			Parptr->maxintcount += 1;

		}

		// Regular output
		if (LFPContext->curr_time >= Parptr->SaveTotal)
		{

			gettimeofday(&(LFPContext->timstr), NULL);
			double write_start_time = LFPContext->timstr.tv_sec + (LFPContext->timstr.tv_usec / 1000000.0);

			time(&Solverptr->time_check);

#if _PROFILE_MODE < 2			
			write_regular_output(Fnameptr->resrootname,
				LFPContext->grid_cols, LFPContext->grid_rows, LFPContext->grid_cols_padded,
				LFPContext->depth_thresh, LFPContext->curr_time,
				LFPContext->tmp_grid1, LFPContext->tmp_grid2, LFPContext->tmp_grid3,
				LFPContext->h_grid, LFPContext->dem_grid,
				LFPContext->Qx_grid, LFPContext->Qy_grid,
				LFPContext->Qx_old_grid, LFPContext->Qy_old_grid,
				LFPContext->Vx_grid, LFPContext->Vy_grid,
				LFPContext->SGC_BankFullHeight_grid, LFPContext->maxH_grid,
				LFPContext->sub_grid_layout_rows, LFPContext->sub_grid_state_rows,

				LFPContext->dx_col, LFPContext->dy_col,
				Solverptr, Statesptr, Parptr, Fnameptr, LFPContext->boundary_cond, SGCptr,
				&Statesptr->output_params,
				Parptr->SaveNo,
				Statesptr->save_depth, Statesptr->save_elev, Statesptr->save_Qs,
				Statesptr->voutput, Statesptr->SGCvoutput, Statesptr->save_glacier_thickness, Statesptr->save_snow_thickness, Statesptr->save_table_depth
			);
#endif
			// if regular output includes max reset max
			if (Statesptr->saveint_max == ON)
			{
				// update h to be 'depth' above sub-grid-channel
				for (int j = 0; j < LFPContext->grid_rows; j++)
				{
					for (int i = 0; i < LFPContext->grid_cols; i++)
					{
						int index_padded = i + j * LFPContext->grid_cols_padded;
						int index = i + j * LFPContext->grid_cols;
						LFPContext->maxH_grid[index_padded] = C(0.0); // rests max depth to zero at save interval
					}
				}
			}

			// update interval counter
			Parptr->SaveTotal += Parptr->SaveInt;
			Parptr->SaveNo += 1;

			gettimeofday(&(LFPContext->timstr), NULL);
			double write_end_time = LFPContext->timstr.tv_sec + (LFPContext->timstr.tv_usec / 1000000.0);
			LFPContext->total_write_time += (write_end_time - write_start_time);
		}

		// If requested on command line, check whether we should kill this simulation...
		if (Statesptr->killsim == ON)
		{
			//iteration time
			time(&Solverptr->time_check);
			Solverptr->itrn_time_now = Solverptr->itrn_time + (NUMERIC_TYPE)difftime(Solverptr->time_check, Solverptr->time_start);
			// check if we have reached the kill time
			if (Solverptr->itrn_time_now >= Parptr->killsim_time)
			{
				printf("Simulation kill time reached... ");
				LFPContext->stop_loop = ON;
				//break;
			}
		}

		if (Statesptr->use_dhsvm == ON)
		{
			if (LFPContext->curr_time - LFPContext->last_gw_time >= Parptr->gwTstep) {
				LFPContext->last_gw_time += Parptr->gwTstep;
			}
		}
		// xdw modify, 执行完所有更新和输出后，再更新当前时间
		if (LFPContext->curr_time > C(0.0))
			Solverptr->MinTstep = getmin(Solverptr->MinTstep, delta_time);
		LFPContext->curr_time += delta_time;
		Solverptr->t = LFPContext->curr_time;
		int intT = LFPContext->curr_time;
		if (intT % 36000 == 0)
		{
			cout << "current time: " << Solverptr->t / 3600 << "hour,    timestep: " << delta_time << endl;
		}
		Solverptr->itCount++;
		itCount = Solverptr->itCount;

	} //end omp single section
}


void Fast_Main(const int grid_cols, const int grid_rows, const int grid_cols_padded,
	NUMERIC_TYPE *h_grid, NUMERIC_TYPE *volume_grid,
	NUMERIC_TYPE *Qx_grid, NUMERIC_TYPE *Qy_grid, NUMERIC_TYPE *Qx_old_grid, NUMERIC_TYPE *Qy_old_grid,
	NUMERIC_TYPE *maxH_grid, NUMERIC_TYPE *maxHtm_grid, NUMERIC_TYPE *initHtm_grid, NUMERIC_TYPE *totalHtm_grid,
	NUMERIC_TYPE *maxVc_grid, NUMERIC_TYPE *maxVc_height_grid, NUMERIC_TYPE *maxHazard_grid,
	NUMERIC_TYPE *Vx_grid, NUMERIC_TYPE *Vy_grid, NUMERIC_TYPE *Vx_max_grid, NUMERIC_TYPE *Vy_max_grid,
	const NUMERIC_TYPE *dem_grid,
	const NUMERIC_TYPE *g_friction_sq_x_grid, const NUMERIC_TYPE *g_friction_sq_y_grid,
	const NUMERIC_TYPE *friction_x_grid, const NUMERIC_TYPE *friction_y_grid,
	const NUMERIC_TYPE *dx_col, const NUMERIC_TYPE *dy_col, const NUMERIC_TYPE *cell_area_col,
	const NUMERIC_TYPE *Fp_xwidth, const NUMERIC_TYPE *Fp_ywidth,

	const SubGridRowList * sub_grid_layout_rows,
	SubGridState * sub_grid_state_rows,
	const SubGridRowList * sub_grid_layout_blocks,
	SubGridState * sub_grid_state_blocks,

	const NUMERIC_TYPE * SGC_BankFullHeight_grid,

	TimeSeries * evap_time_series,
	NetCDFVariable * evap_grid,
	TimeSeries * rain_time_series,
	TimeSeries * temperature_time_series,
	NUMERIC_TYPE *rain_grid,
	const NUMERIC_TYPE *dist_infil_grid,

	WetDryRowBound* wet_dry_bounds,
	PointSourceRowList * ps_layout, BoundaryCondition * boundary_cond,
	WeirLayout * weirs_weirs, WeirLayout * weirs_bridges,
	RouteDynamicList * route_dynamic_list,
	const NUMERIC_TYPE *route_V_ratio_per_sec_qx, const NUMERIC_TYPE * route_V_ratio_per_sec_qy,

	SuperGridLinksList *Super_linksptr,
	Fnames *Fnameptr,
	Files *Fptr,
	Stage *Locptr,
	States *Statesptr,
	Pars *Parptr,
	Solver *Solverptr,
	Pois *Poisptr,
	Arrays *Arrptr,
	DamData *Damptr,
	SGCprams * SGCptr,
	LISFLOODFPContext* LFPContextPtr
#ifdef RESULT_CHECK
	Arrays * Arrptr, // only for compare results
	BoundCs * BCptr, // only for compare results
	ChannelSegmentType *ChannelSegments,
	vector<ChannelSegmentType> *ChannelSegmentsVecPtr,
#endif

)
{
	int tstep_counter = -1;   // start at -1 so that in first run through we calculate river
	int steadyCount = 0;
	NUMERIC_TYPE tstep_channel = C(0.0); // channel timestep
	NUMERIC_TYPE Previous_t;      // previous time channel was calculated
	NUMERIC_TYPE discharge = C(0.0); // value of discharge for virtual gauge output
	NUMERIC_TYPE loss; //temp variable to keep track of losses since last mass interval
	NUMERIC_TYPE Comp_time, Model_Comp_Ratio, Model_time_left, Est_Time_Tot, Est_Time_Fin;

	const NUMERIC_TYPE depth_thresh = Solverptr->DepthThresh;
	const NUMERIC_TYPE g = Solverptr->g;

	// tmp data for preparing data to write as output
	LFPContextPtr->tmp_grid1 = (NUMERIC_TYPE*)memory_allocate(sizeof(NUMERIC_TYPE) * LFPContextPtr->grid_cols_padded * (LFPContextPtr->grid_rows + 1));
	LFPContextPtr->tmp_grid2 = (NUMERIC_TYPE*)memory_allocate(sizeof(NUMERIC_TYPE) * LFPContextPtr->grid_cols_padded * (LFPContextPtr->grid_rows + 1));
	LFPContextPtr->tmp_grid3 = (NUMERIC_TYPE*)memory_allocate(sizeof(NUMERIC_TYPE) * LFPContextPtr->grid_cols_padded * (LFPContextPtr->grid_rows + 1));
	memset(LFPContextPtr->tmp_grid1, 0, sizeof(NUMERIC_TYPE) * LFPContextPtr->grid_rows * LFPContextPtr->grid_cols_padded);
	memset(LFPContextPtr->tmp_grid2, 0, sizeof(NUMERIC_TYPE) * LFPContextPtr->grid_rows * LFPContextPtr->grid_cols_padded);
	memset(LFPContextPtr->tmp_grid3, 0, sizeof(NUMERIC_TYPE) * LFPContextPtr->grid_rows * LFPContextPtr->grid_cols_padded);

	{
		NUMERIC_TYPE reduce_flood_area = C(0.0);
		NUMERIC_TYPE reduce_domain_volume = C(0.0);
#pragma omp parallel for default(shared) reduction ( + : reduce_flood_area, reduce_domain_volume) schedule(static)
		for (int block_index = 0; block_index < wet_dry_bounds->block_count; block_index++)
		{
			NUMERIC_TYPE block_flood_area, block_domain_volume;
			SGC2_DomainVolumeAndFloodArea_block(block_index, grid_cols, grid_rows, grid_cols_padded, depth_thresh,
				wet_dry_bounds,
				h_grid, cell_area_col, volume_grid, &block_flood_area, &block_domain_volume);
			reduce_flood_area += block_flood_area;   //总淹没面积
			reduce_domain_volume += block_domain_volume;  // 总淹没体积
		}
		Solverptr->vol1 = reduce_domain_volume;
	}

	// set previous time to one timestep backwards so that first river calcs uses Solverptr->Tstep for 1st iteration
	// this is because of the way the timestep is calculated as the time difference from the last time the river was run
	Previous_t = Solverptr->t - Solverptr->Tstep;

#if defined (__INTEL_COMPILER) && _PROFILE_MODE > 0
	printf("Intel profiler resume\n");
	__itt_resume();
#endif

	struct timeval timstr;      /* structure to hold elapsed time */
	double processing_start_time, processing_end_time;
	gettimeofday(&timstr, NULL);
	processing_start_time = timstr.tv_sec + (timstr.tv_usec / 1000000.0);
	time_t loop_start;
	time(&loop_start);

	// initialise the h wet_dry_bounds
	SGC2_InitHBounds(grid_cols, grid_rows, grid_cols_padded, depth_thresh, sub_grid_layout_rows,
		sub_grid_state_rows,
		cell_area_col,
		h_grid, volume_grid, wet_dry_bounds, SGCptr, dem_grid, Poisptr);
	SGC2_UpdateLoadBalance(grid_rows, grid_cols_padded, sub_grid_layout_rows, wet_dry_bounds);

	// initalise distributed rainfall
	DynamicRain<> dynamic_rain(Fnameptr->dynamicrainfilename, LFPContextPtr->verbose);

	NUMERIC_TYPE curr_time = Solverptr->t;

	int timestep_tripped = 0;
	int error_tripped = 0;

	int stop_loop = OFF;
	itCount = Solverptr->itCount;
	// xiaodw, for groundwater timestep calculation
	NUMERIC_TYPE last_gw_time = 0.f;
	NUMERIC_TYPE last_interflow_time = 0.f;

	// xiaodw, for distributed rainfall
	LFPContextPtr->last_rain_time = 0;
	if (Statesptr->rainfallmask)
	{
		LFPContextPtr->rain_begin_timestamp = getTimestampFromDateTime(Solverptr->rain_begin_time);
		LFPContextPtr->rain_end_timestamp = getTimestampFromDateTime(Solverptr->rain_end_time);
		LFPContextPtr->tifFileTimes = getTimesFromFiles(Fnameptr->rainTifFolder, TIF);
		LFPContextPtr->last_rain_time = LFPContextPtr->rain_begin_timestamp;
		LFPContextPtr->rainfall_no_padding = (NUMERIC_TYPE*)memory_allocate(sizeof(NUMERIC_TYPE) * Parptr->xsz * Parptr->ysz);
	}
	// xiaodw, for seims
	LFPContextPtr->seims_begin_timestamp = getTimestampFromDateTime(Solverptr->seims_begin_time);
	LFPContextPtr->seims_end_timestamp = getTimestampFromDateTime(Solverptr->seims_end_time);
	// xiaodw, SGC模式下的河道初始径流量
	if (Parptr->sgcStartH > 0)
	{
		for (int j = 0; j < Parptr->ysz; j++)
		{
			const int * sg_pair_grid_index_lookup = sub_grid_layout_blocks->flow_info.flow_pair.sg_cell_grid_index_lookup;
			const NUMERIC_TYPE * sg_pair_SGC_BankFullHeight = sub_grid_layout_blocks->flow_info.flow_pair.sg_cell_SGC_BankFullHeight;
			const NUMERIC_TYPE * sg_pair_SGC_width = sub_grid_layout_blocks->flow_info.flow_pair.sg_cell_SGC_width;
			const NUMERIC_TYPE * sg_pair_dem = sub_grid_layout_blocks->flow_info.flow_pair.sg_cell_dem; // sg_cell_dem就是sub grid河道对应的DEM
			const int * sg_pair_SGC_group = sub_grid_layout_blocks->flow_info.flow_pair.sg_cell_SGC_group;

			NUMERIC_TYPE * sg_flow_Q = sub_grid_state_blocks->sg_flow_Q;
			const NUMERIC_TYPE * sg_flow_effective_distance = sub_grid_layout_blocks->flow_info.sg_flow_effective_distance;
			const NUMERIC_TYPE * sg_flow_g_friction_sq = sub_grid_layout_blocks->flow_info.sg_flow_g_friction_sq;

			const int sg_row_start_index = j * sub_grid_layout_blocks->row_cols_padded;
			const int sg_row_pair_start_index = j * 2 * sub_grid_layout_blocks->row_cols_padded;
			const int flow_end = sub_grid_layout_blocks->flow_row_count[j];
#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
			__assume(sg_row_start_index % GRID_ALIGN_WIDTH == 0);
			__assume(sg_row_pair_start_index % GRID_ALIGN_WIDTH == 0);
#endif

#pragma ivdep
#pragma simd
			for (int flow_i = 0; flow_i < flow_end; flow_i++)
			{
				int flow_index = sg_row_start_index + flow_i;
				int flow_pair_index = sg_row_pair_start_index + 2 * flow_i;
				int flow_pair_index_next = flow_pair_index + 1;

				int grid_index0 = sg_pair_grid_index_lookup[flow_pair_index];
				int grid_index1 = sg_pair_grid_index_lookup[flow_pair_index_next]; // also the q index for this flow (in d4)

				h_grid[grid_index0] += Parptr->sgcStartH;

			}
		}

	}

	//****************************Finish all initialize************************

}
