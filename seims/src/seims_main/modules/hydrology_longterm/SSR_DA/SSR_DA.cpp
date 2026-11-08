#include "SSR_DA.h"

#include "text.h"

SSR_DA::SSR_DA() :
    m_inputSubbsnID(-1), m_nCells(-1), m_CellWth(-1.f), m_maxSoilLyrs(-1), m_nSoilLyrs(nullptr),
    m_soilThk(nullptr),
    m_dt(-1), m_ki(NODATA_VALUE),
    m_soilFrozenTemp(NODATA_VALUE), m_slope(nullptr), m_ks(nullptr), m_soilSat(nullptr),
    m_poreIdx(nullptr),
    m_soilFC(nullptr), m_soilWP(nullptr),
    m_soilWtrSto(nullptr), m_soilWtrStoPrfl(nullptr), m_soilTemp(nullptr), m_chWidth(nullptr),
    m_rchID(nullptr), m_flowInIdxD8(nullptr), m_rteLyrs(nullptr),
    m_nRteLyrs(-1), m_nSubbsns(-1), m_subbsnID(nullptr),
    /// outputs
    m_subSurfRf(nullptr), m_subSurfRfVol(nullptr), m_ifluQ2Rch(nullptr),
    //ljj++
    m_area(nullptr),m_flowout_length(nullptr),m_soilTempprofile(nullptr),
    m_soilIceSto(nullptr),m_clay(nullptr),m_soilPor(nullptr),m_slplen(nullptr),m_TTlag(nullptr),
    m_cellFlow(nullptr),m_surfRf(nullptr),m_potVol(nullptr),m_infil(nullptr),m_impoundTrig(nullptr),
    m_soilPerco(nullptr),m_landUse(nullptr),m_soilAWC(nullptr),m_dis2Stream(nullptr),
    m_ki_1d(nullptr),m_soilFrozenTemp_1d(nullptr),
	//xdw++
	m_soilMoist_1(nullptr), m_soilMoist_5(nullptr), m_soilMoist_15(nullptr), m_soilMoist_30(nullptr),
	m_soilMoist_60(nullptr), m_soilMoist_100(nullptr), m_soilMoist_200(nullptr), m_soilMoist(nullptr),
	m_soilSat1(nullptr), m_soilSat5(nullptr), m_soilSat15(nullptr), m_soilSat30(nullptr),
	m_soilSat60(nullptr), m_soilSat100(nullptr), m_soilSat200(nullptr),
	m_soilAWC1(nullptr), m_soilAWC5(nullptr), m_soilAWC15(nullptr), m_soilAWC30(nullptr),
	m_soilAWC60(nullptr), m_soilAWC100(nullptr), m_soilAWC200(nullptr),
	m_soilMoistBfe(nullptr), m_soilWtrStoBfe(nullptr),
	m_ks1(nullptr), m_ks5(nullptr), m_ks15(nullptr), m_ks30(nullptr),
	m_ks60(nullptr), m_ks100(nullptr), m_ks200(nullptr)

     {
}

SSR_DA::~SSR_DA() {
    if (m_subSurfRf != nullptr) Release2DArray(m_nCells, m_subSurfRf);
    if (m_subSurfRfVol != nullptr) Release2DArray(m_nCells, m_subSurfRfVol);
    if (m_ifluQ2Rch != nullptr) Release1DArray(m_ifluQ2Rch);
    if (m_slplen != nullptr) Release1DArray(m_slplen);
    if (m_TTlag != nullptr) Release2DArray(m_nCells, m_TTlag);
    if (m_cellFlow != nullptr) Release2DArray(m_nCells, m_cellFlow);

	if (m_soilMoist_1 != nullptr) Release1DArray(m_soilMoist_1);
	if (m_soilMoist_5 != nullptr) Release1DArray(m_soilMoist_5);
	if (m_soilMoist_15 != nullptr) Release1DArray(m_soilMoist_15);
	if (m_soilMoist_30 != nullptr) Release1DArray(m_soilMoist_30);
	if (m_soilMoist_60 != nullptr) Release1DArray(m_soilMoist_60);
	if (m_soilMoist_100 != nullptr) Release1DArray(m_soilMoist_100);
	if (m_soilMoist_200 != nullptr) Release1DArray(m_soilMoist_200);
	if (m_soilMoist != nullptr) Release2DArray(m_nCells, m_soilMoist);
	if (m_soilSat1 != nullptr) Release1DArray(m_soilSat1);
	if (m_soilSat5 != nullptr) Release1DArray(m_soilSat5);
	if (m_soilSat15 != nullptr) Release1DArray(m_soilSat15);
	if (m_soilSat30 != nullptr) Release1DArray(m_soilSat30);
	if (m_soilSat60 != nullptr) Release1DArray(m_soilSat60);
	if (m_soilSat100 != nullptr) Release1DArray(m_soilSat100);
	if (m_soilSat200 != nullptr) Release1DArray(m_soilSat200);

	if (m_soilAWC1 != nullptr) Release1DArray(m_soilAWC1);
	if (m_soilAWC5 != nullptr) Release1DArray(m_soilAWC5);
	if (m_soilAWC15 != nullptr) Release1DArray(m_soilAWC15);
	if (m_soilAWC30 != nullptr) Release1DArray(m_soilAWC30);
	if (m_soilAWC60 != nullptr) Release1DArray(m_soilAWC60);
	if (m_soilAWC100 != nullptr) Release1DArray(m_soilAWC100);
	if (m_soilAWC200 != nullptr) Release1DArray(m_soilAWC200);
	if (m_soilMoistBfe != nullptr) Release2DArray(m_nCells, m_soilMoistBfe);
	if (m_soilWtrStoBfe != nullptr) Release2DArray(m_nCells, m_soilWtrStoBfe);
	
	if (m_ks1 != nullptr) Release1DArray(m_ks1);
	if (m_ks5 != nullptr) Release1DArray(m_ks5);
	if (m_ks15 != nullptr) Release1DArray(m_ks15);
	if (m_ks30 != nullptr) Release1DArray(m_ks30);
	if (m_ks60 != nullptr) Release1DArray(m_ks60);
	if (m_ks100 != nullptr) Release1DArray(m_ks100);
	if (m_ks200 != nullptr) Release1DArray(m_ks200);

}

bool SSR_DA::FlowInSoil(const int id) {

	// xdw++
	float surFlowOld = m_surfRf[id];
	int subbasinId = CVT_INT(m_subbsnID[id]);
    int frez =0;
    float s0 = Max(m_slope[id], 0.01f);
    // float flowWidth = m_CellWth;
    float flowWidth = m_flowout_length[id]; //ljj
    //ljj++ 对于不规则模拟单元，不应该跳过任何单元的计算，flowWidth必须大于0，湖泊除外

    if (m_flowout_length[id] <= 1 && m_landUse[id] !=18) flowWidth = sqrt(m_area[id]);  // subarea is not ajcent to the stream
    // there is no land in this cell
    //TODO ljj:23-10-13:对于河道的地块，流路宽度应该等于河道长度
    // if (m_rchID[id] > 0) {
    //     flowWidth -= m_chWidth[id];
    // }
    // initialize for current cell of current timestep
    for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
        m_subSurfRf[id][j] = 0.f;
        m_subSurfRfVol[id][j] = 0.f;
        m_soilPerco[id][j] = 0.f;
		m_soilMoist[id][j] = m_soilWtrSto[id][j] / m_soilSat[id][j];
		m_soilMoistBfe[id][j] = m_soilMoist[id][j];
		m_soilWtrStoBfe[id][j] = m_soilWtrSto[id][j];
    }
    /* Previous code. Update: In my view, if the flowWidth is less than 0, the subsurface flow
     * from the upstream cells should be added to stream cell directly, which will be summarized
     * for channel flow routing. By lj, 2018-4-12
    // return with initial values if flowWidth is less than 0
    if (flowWidth <= 0) return true;
    */
    // number of flow-in cells
    int nUpstream = CVT_INT(m_flowInIdxD8[id][0]);
    m_soilWtrStoPrfl[id] = 0.f; // update soil storage on profile


    for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
        float smOld = m_soilWtrSto[id][j];
        //sum the upstream subsurface flow
        float qUp = 0.f;    // mm
        float qUpVol = 0.f; // m^3
        // If no in cells flowin (i.e., nUpstream = 0), the for-loop will be ignored.
        for (int upIndex = 1; upIndex <= nUpstream; upIndex++) {
            int flowInID = CVT_INT(m_flowInIdxD8[id][upIndex]);
            // IMPORTANT!!! If the upstream cell is from another subbasin, CONTINUE to next upstream cell. By lj.
            if (CVT_INT(m_subbsnID[flowInID]) != CVT_INT(m_subbsnID[id])) { continue; }
            if (m_subSurfRf[flowInID][j] > 0.f) {
                qUp += m_subSurfRf[flowInID][j]*m_area[flowInID]/m_area[id]; // * m_flowInPercentage[id][upIndex]; // TODO: Consider MFD algorithms
                qUpVol += m_subSurfRfVol[flowInID][j];
            }
        }
        // add upstream water to the current cell
        if (qUp <= 0.f || qUpVol <= 0.f) {
            qUp = 0.f;
            qUpVol = 0.f;
        }
        //if(id<5 && j==0) cout<<m_soilAWC[id][j]<<endl;
        // if the flowWidth is less than 0, the subsurface flow from the upstream cells
        // should be added to stream cell directly, which will be summarized
        // for channel flow routing. By lj, 2018-4-12
        if (flowWidth <= 0.f || m_landUse[id] ==LANDUSE_ID_WATR) {
            m_subSurfRf[id][j] = qUp;
            m_subSurfRfVol[id][j] = qUpVol;
            continue;
        }
        if (m_soilWtrSto[id][j] != m_soilWtrSto[id][j] || m_soilWtrSto[id][j] < 0.f) {
            cout << "cell id: " << id << ", layer: " << j << ", moisture is less than zero: "
                    << m_soilWtrSto[id][j] << ", previous: " << smOld << ", qUp: " << qUp << ", depth:"
                    << m_soilThk[id][j] << endl;
            return false;
        }
        m_soilWtrSto[id][j] += qUp; // mm

        // if soil moisture is below the field capacity, no interflow will be generated
        //if (m_soilWtrSto[id][j] <= m_soilFC[id][j]) continue;
		m_soilMoist[id][j] = m_soilWtrSto[id][j] / m_soilSat[id][j];
        if (m_soilWtrSto[id][j] <= m_soilAWC[id][j]) continue;
        // Otherwise, calculate interflow:
        // for the upper two layers, soil may be frozen
        // also check if there are upstream inflow
        // if (j == 0 && m_soilTemp[id] <= m_soilFrozenTemp && qUp <= 0.f) {
        //     continue;
        // }
        //if (frez == 0 && m_soilTempprofile[id][j] <= m_soilFrozenTemp) {
		 // xiaodw comment, don't need soil temperature now
        //if (frez == 0 && m_soilTempprofile[id][j] <= m_soilFrozenTemp_1d[id]) {
        //    continue;
        //}

        float k = 0.f, maxSoilWater = 0.f, soilWater = 0.f, fcSoilWater = 0.f;
        soilWater = m_soilWtrSto[id][j];
        maxSoilWater = m_soilSat[id][j];

        //ljj++
        float WCND =  m_ks[id][j];
        if(frez==1) {
            maxSoilWater =m_soilPor[id][j]*m_soilThk[id][j] -  m_soilIceSto[id][j];//ljj++ frozen
            maxSoilWater  = Max(0.f,maxSoilWater);

            float FACTR = Max(0.01, (m_soilWtrSto[id][j])/maxSoilWater) ;
            FACTR = Min(FACTR,1.f);
            float BEXP = 2.91+0.159*m_clay[id][j]*0.01;
            float EXPON = 2.0*BEXP + 3.0 ;
            //WCND  = myDKSAT * FACTR ** EXPON
            float alpha = 3;
            float f_frozen=exp(-alpha*(1-Min(m_soilIceSto[id][j]/(m_soilPor[id][j]*m_soilThk[id][j]),1)));
            f_frozen = Max(0.f,f_frozen);
            f_frozen = f_frozen - exp(-alpha);
            WCND =  (1-f_frozen)*m_ks[id][j]* Min(1.0,pow(FACTR, EXPON));
            WCND = Min(WCND,m_ks[id][j]);
        }
        //

        //fcSoilWater = m_soilFC[id][j];
        fcSoilWater = m_soilAWC[id][j];
        //the moisture content can exceed the porosity in the way the algorithm is implemented
        if (m_soilWtrSto[id][j] > maxSoilWater) {
            k = m_ks[id][j];
            if(frez==1) k = WCND ;  //ljj
        } else {
            /// Using Clapp and Hornberger (1978) equation to calculate unsaturated hydraulic conductivity.
            float dcIndex = 2.f * m_poreIdx[id][j] + 3.f; // pore disconnectedness index
            k = m_ks[id][j] * pow(m_soilWtrSto[id][j] / maxSoilWater, dcIndex);
            if(frez==1) k = WCND ;  //ljj
            if (k <= UTIL_ZERO) k = 0.f;
            //cout << id << "\t" << j << "\t" << k << endl;
        }
        // 1. / 3600. = 0.0002777777777777778
        flowWidth = sqrt(m_area[id]);  // subarea is not ajcent to the stream
        //m_subSurfRf[id][j] =m_ki * s0 * k * m_dt * 0.0002777777777777778f * m_soilThk[id][j] * 0.001f / flowWidth;
        m_subSurfRf[id][j] =m_ki_1d[id] * s0 * k * m_dt * 0.0002777777777777778f * m_soilThk[id][j] * 0.001f / flowWidth;
        // the unit is mm

        // if (soilWater - m_subSurfRf[id][j] > maxSoilWater) {
        //     m_subSurfRf[id][j] = soilWater - maxSoilWater;
        // //}
        // } else
        if (soilWater - m_subSurfRf[id][j] < fcSoilWater) {
            m_subSurfRf[id][j] = soilWater - fcSoilWater;
        }
        m_subSurfRf[id][j] = Max(0.f, m_subSurfRf[id][j]);
        //ljj++
        //float sw_excess = m_soilWtrSto[id][j] - m_soilFC[id][j];
        float sw_excess = m_soilWtrSto[id][j] - m_soilAWC[id][j];
        // No movement if soil moisture is below field capacity
        if (sw_excess > 1.e-5f) {
            float maxPerc = maxSoilWater - fcSoilWater;
            if (maxPerc < 0.f) maxPerc = 0.1f;
            float tt = 3600.f * maxPerc / m_ks[id][j];                  // secs
            //if(frez==1) tt = 3600.f * maxPerc / WCND ;  //ljj++
            m_soilPerco[id][j] = sw_excess * (1.f - exp(-m_dt / tt)); // secs
            if (m_soilPerco[id][j] > maxPerc) {
                m_soilPerco[id][j] = maxPerc;
            }
            // if (soilWater - m_soilPerco[id][j] > maxSoilWater) {
            //     m_soilPerco[id][j] = soilWater - maxSoilWater;
            // }
        }
        float ss = Min(m_slplen[id],300.f);
        float xx = 10.4f * ss /(m_ks[id][j]);
        m_TTlag[id][j] = 1.f - exp(-1.f/xx);
        m_TTlag[id][j] = Min(m_TTlag[id][j],1.f);

        if((m_soilPerco[id][j] + m_subSurfRf[id][j]) > sw_excess) {
            float ratio = 0.;
            ratio = m_soilPerco[id][j] / (m_subSurfRf[id][j] + m_soilPerco[id][j]);
            m_soilPerco[id][j] = sw_excess * ratio;
            m_subSurfRf[id][j] = sw_excess * (1. - ratio);
        }

        m_soilWtrSto[id][j] -= m_soilPerco[id][j];
        m_soilWtrSto[id][j] -= m_subSurfRf[id][j];
		float soilWtrStoNexLyrOld = 0.0f;
		if (j < CVT_INT(m_nSoilLyrs[id]) - 1) soilWtrStoNexLyrOld = m_soilWtrSto[id][j + 1];
        if (j < CVT_INT(m_nSoilLyrs[id]) - 1) m_soilWtrSto[id][j + 1] += m_soilPerco[id][j];
        m_soilWtrSto[id][j] = Max(UTIL_ZERO, m_soilWtrSto[id][j]);
        m_cellFlow[id][j] += m_subSurfRf[id][j];
        m_subSurfRf[id][j] = m_cellFlow[id][j] *  m_TTlag[id][j];  //this time step
        m_cellFlow[id][j] -= m_subSurfRf[id][j];
        m_cellFlow[id][j] = Max(m_cellFlow[id][j],0.f);

       // m_subSurfRfVol[id][j] = m_subSurfRf[id][j] * 0.001f * m_CellWth * flowWidth; //m3
		m_subSurfRfVol[id][j] = m_subSurfRf[id][j] * 0.001f * m_area[id]; //ljj change for field
        m_subSurfRfVol[id][j] = Max(UTIL_ZERO, m_subSurfRfVol[id][j]);
        //Adjust the moisture content in the current layer, and the layer immediately below it

        // redistribute soil water if above field capacity (high water table), rewrite from sat_excess.f of SWAT
        //float qlyr = m_soilStorage[i][j];
        if (j < CVT_INT(m_nSoilLyrs[id]) - 1) {
            if (m_soilWtrSto[id][j] - m_soilSat[id][j] > 1.e-4f) {
                m_soilWtrSto[id][j + 1] += m_soilWtrSto[id][j] - m_soilSat[id][j];
                m_soilWtrSto[id][j] = m_soilSat[id][j];
            }
        } else {
            /// for the last soil layer
            if (m_soilWtrSto[id][j] - m_soilSat[id][j] > 1.e-4f) {
                float ul_excess = m_soilWtrSto[id][j] - m_soilSat[id][j];
                m_soilWtrSto[id][j] = m_soilSat[id][j];
                for (int ly = CVT_INT(m_nSoilLyrs[id]) - 2; ly >= 0; ly--) {
                    m_soilWtrSto[id][ly] += ul_excess;
                    if (m_soilWtrSto[id][ly] > m_soilSat[id][ly]) {
                        ul_excess = m_soilWtrSto[id][ly] - m_soilSat[id][ly];
                        m_soilWtrSto[id][ly] = m_soilSat[id][ly];
                    } else {
                        ul_excess = 0.f;
                        break;
                    }
                    if (ly == 0 && ul_excess > 0.f) {
                        // add ul_excess to depressional storage and then to surfq
                        if (m_potVol != nullptr && FloatEqual(m_impoundTrig[id], 0.f)) {
                            m_potVol[id] += ul_excess;
                        } else {
                            m_surfRf[id] += ul_excess;
                        }
                        m_infil[id] -= ul_excess;
                    }
                }
            }
        }
        m_soilWtrStoPrfl[id] += m_soilWtrSto[id][j];

		m_soilMoist[id][j] = m_soilWtrSto[id][j] / m_soilSat[id][j];

        if (m_soilWtrSto[id][j] != m_soilWtrSto[id][j] || m_soilWtrSto[id][j] < 0.f) {
            cout << "cell id: " << id << ", layer: " << j << ", moisture is less than zero: "
                    << m_soilWtrSto[id][j] << ", subsurface runoff: " << m_subSurfRf[id][j] << ", depth:"
                    << m_soilThk[id][j] << endl;
            return false;
        }


    }
	m_soilMoist_1[id] = m_soilWtrSto[id][0] / (m_soilSat[id][0]);
	m_soilMoist_5[id] = m_soilWtrSto[id][1] / m_soilSat[id][1];
	m_soilMoist_15[id] = m_soilWtrSto[id][2] / m_soilSat[id][2];
	m_soilMoist_30[id] = m_soilWtrSto[id][3] / m_soilSat[id][3];
	m_soilMoist_60[id] = m_soilWtrSto[id][4] / m_soilSat[id][4];
	m_soilMoist_100[id] = m_soilWtrSto[id][5] / m_soilSat[id][5];
	m_soilMoist_200[id] = m_soilWtrSto[id][6] / m_soilSat[id][6];

	m_soilSat1[id] = m_soilSat[id][0];
	m_soilSat5[id] = m_soilSat[id][1];
	m_soilSat15[id] = m_soilSat[id][2];
	m_soilSat30[id] = m_soilSat[id][3];
	m_soilSat60[id] = m_soilSat[id][4];
	m_soilSat100[id] = m_soilSat[id][5];
	m_soilSat200[id] = m_soilSat[id][6];

	m_soilAWC1[id] = m_soilAWC[id][0];
	m_soilAWC5[id] = m_soilAWC[id][1];
	m_soilAWC15[id] = m_soilAWC[id][2];
	m_soilAWC30[id] = m_soilAWC[id][3];
	m_soilAWC60[id] = m_soilAWC[id][4];
	m_soilAWC100[id] = m_soilAWC[id][5];
	m_soilAWC200[id] = m_soilAWC[id][6];

	m_ks1[id] = m_ks[id][0];
	m_ks5[id] = m_ks[id][1];
	m_ks15[id] = m_ks[id][2];
	m_ks30[id] = m_ks[id][3];
	m_ks60[id] = m_ks[id][4];
	m_ks100[id] = m_ks[id][5];
	m_ks200[id] = m_ks[id][6];


#ifdef DEBUG_SSR_DA
		{
			int SPECIFIED_SBID = 2;
			if (subbasinId == SPECIFIED_SBID) {
				cout << "Sbid: " << subbasinId << "   "
					<< " HandId: " << id << "   " << endl;
					for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
						cout 
							<< "Layer: " << j << "   "
							<< " MoisBfe=" << m_soilMoistBfe[id][j] << "   "
							<< " MoisAft=" << m_soilMoist[id][j] << "   "
							<< " WtrStoBfe=" << m_soilWtrStoBfe[id][j] << "   "
							<< " WtrStoAft=" << m_soilWtrSto[id][j] << "   "
							<< " Perco=" << m_soilPerco[id][j] << "   "
							<< " SubF= " << m_subSurfRf[id][j] << "   "
							<< " SAT=" << m_soilSat[id][j] << "   "
							<< " AWC=" << m_soilAWC[id][j] << "   "
							<< " WP=" << m_soilWP[id][j] << "   "
							<< " THICK=" << m_soilThk[id][j] << "   "
							<< endl;
					}
				
			}
		}
#endif
    return true;
}

int SSR_DA::Execute() {
    CheckInputData();
    InitialOutputs();
#ifdef DEBUG_SSR_DA
	{
		cout << "[SSR_DA]" << endl;
	}
#endif
    for (int ilyr = 0; ilyr < m_nRteLyrs; ilyr++) {
        // There are not any flow relationship within each routing layer.
        // So parallelization can be done here.
        int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
        // DO NOT THROW EXCEPTION IN OMP FOR LOOP, i.e., FlowInSoil(id) function.
        int errCount = 0;
#pragma omp parallel for reduction(+: errCount)
        for (int icell = 1; icell <= ncells; icell++) {
            int id = CVT_INT(m_rteLyrs[ilyr][icell]);
            if (!FlowInSoil(id)) errCount++;

        }
        if (errCount > 0) {
            throw ModelException(MID_SSR_DA, "Execute:FlowInSoil",
                                 "Please check the error message for more information");
        }
    }
    for (int i = 0; i <= m_nSubbsns; i++) {
        m_ifluQ2Rch[i] = 0.f;
    }
    /// using openmp for reduction an array should be paid much more attention.
    /// here is a solution. https://stackoverflow.com/questions/20413995/reducing-on-array-in-openmp
    /// #pragma omp parallel for reduction(+:myArray[:6]) is supported with OpenMP 4.5.
    /// However, MSVC 2010-2015 are using OpenMP 2.0.
    /// Added by lj, 2017-8-23
#pragma omp parallel
    {
        float* tmp_qiSubbsn = new(nothrow) float[m_nSubbsns + 1];
        for (int i = 0; i <= m_nSubbsns; i++) {
            tmp_qiSubbsn[i] = 0.f;
        }
#pragma omp for
        for (int i = 0; i < m_nCells; i++) {
            if (m_rchID[i] <= 0.f) continue;
            float qiAllLayers = 0.f;
            for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
                if (m_subSurfRfVol[i][j] > UTIL_ZERO) {
                    qiAllLayers += m_subSurfRfVol[i][j] / m_dt; /// m^3/s
                }
            }
            tmp_qiSubbsn[CVT_INT(m_rchID[i])] += qiAllLayers;
        }
#pragma omp critical
        {
            for (int i = 1; i <= m_nSubbsns; i++) {
                m_ifluQ2Rch[i] += tmp_qiSubbsn[i];
            }
        }
        delete[] tmp_qiSubbsn;
        tmp_qiSubbsn = nullptr;
    } /* END of #pragma omp parallel */

    for (int i = 1; i <= m_nSubbsns; i++) {
        m_ifluQ2Rch[0] += m_ifluQ2Rch[i];
    }
    return 0;
}

void SSR_DA::SetValue(const char* key, const float value) {
    string s(key);
    if (StringMatch(s, VAR_T_SOIL)) {
        m_soilFrozenTemp = value;
    } else if (StringMatch(s, VAR_KI)) {
        m_ki = value;
    } else if (StringMatch(s, VAR_SUBBSNID_NUM)) {
        m_nSubbsns = CVT_INT(value);
    } else if (StringMatch(s, Tag_SubbasinId)) {
        m_inputSubbsnID = CVT_INT(value);
    } else if (StringMatch(s, Tag_CellWidth)) {
        m_CellWth = value;
    } else if (StringMatch(s, Tag_TimeStep)) {
        m_dt = CVT_INT(value);
    } else {
        throw ModelException(MID_SSR_DA, "SetValue", "Parameter " + s + " does not exist.");
    }
}

void SSR_DA::Set1DData(const char* key, const int nrows, float* data) {
    string s(key);
    CheckInputSize(MID_SSR_DA, key, nrows, m_nCells);
    if (StringMatch(s, VAR_SLOPE)) {
        m_slope = data;
    }  else if (StringMatch(s, VAR_STREAM_LINK)) {
        m_rchID = data;
    } else if (StringMatch(s, VAR_SOTE)) {
        m_soilTemp = data;
    } else if (StringMatch(s, VAR_SUBBSN)) {
        m_subbsnID = data;
    } else if (StringMatch(s, VAR_SOILLAYERS)) {
        m_nSoilLyrs = data;
    } else if (StringMatch(s, VAR_SOL_SW)) {
        m_soilWtrStoPrfl = data;
    }
    //ljj++
    else if (StringMatch(s, VAR_AHRU)) m_area = data;
    else if (StringMatch(s, VAR_FLOWOUT_LEN)) m_flowout_length= data;
    else if (StringMatch(s, VAR_POT_VOL)) m_potVol = data;
    else if (StringMatch(s, VAR_SURU)) m_surfRf = data;
    else if (StringMatch(s, VAR_IMPOUND_TRIG)) m_impoundTrig = data;
    else if (StringMatch(s, VAR_INFIL)) m_infil = data;
    else if (StringMatch(s, VAR_LANDUSE)) m_landUse = data;
    else if (StringMatch(s, VAR_DISTSTREAM)) m_dis2Stream = data;
    else if (StringMatch(s, "Ki_1d")) m_ki_1d = data;
    else if (StringMatch(s, "t_soil_1d")) m_soilFrozenTemp_1d = data;

    else {
        throw ModelException(MID_SSR_DA, "Set1DData", "Parameter " + s + " does not exist.");
    }
}

void SSR_DA::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    string sk(key);
    if (StringMatch(sk, VAR_SOILTHICK)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilThk = data;
    } else if (StringMatch(sk, VAR_CONDUCT)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_ks = data;
    } else if (StringMatch(sk, VAR_SOL_UL)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilSat = data;
    } else if (StringMatch(sk, VAR_SOL_AWC)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        //m_soilFC = data;
        m_soilAWC = data;
    }else if (StringMatch(sk, VAR_FIELDCAP)) {
		CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
		m_soilFC = data;
	}
	else if (StringMatch(sk, VAR_SOL_WPMM)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilWP = data;
    } else if (StringMatch(sk, VAR_POREIDX)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_poreIdx = data;
    } else if (StringMatch(sk, VAR_SOL_ST)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilWtrSto = data;
    } else if (StringMatch(sk, Tag_ROUTING_LAYERS)) {
        CheckInputSize(MID_SSR_DA, key, nrows, m_nRteLyrs);
        m_rteLyrs = data;
    } else if (StringMatch(sk, Tag_FLOWIN_INDEX_D8)) {
        CheckInputSize(MID_SSR_DA, key, nrows, m_nCells);
        m_flowInIdxD8 = data;
    }
    //ljj++
    else if (StringMatch(sk, VAR_SOILT)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilTempprofile = data;
    }
    else if (StringMatch(sk, VAR_SOLICE)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilIceSto = data;
    }
    else if (StringMatch(sk, "clay")) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_clay = data;
    }
    else if (StringMatch(sk, VAR_POROST)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilPor = data;
    }
    else if (StringMatch(sk, VAR_PERCO)) m_soilPerco = data;
    else {
        throw ModelException(MID_SSR_DA, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void SSR_DA::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
	if (StringMatch(sk, VAR_SBIF)) {
		*data = m_ifluQ2Rch;
		*n = m_nSubbsns + 1;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST1)) {
		*n = m_nCells;
		*data = m_soilMoist_1;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST5)) {
		*n = m_nCells;
		*data = m_soilMoist_5;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST15)) {
		*n = m_nCells;
		*data = m_soilMoist_15;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST30)) {
		*n = m_nCells;
		*data = m_soilMoist_30;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST60)) {
		*n = m_nCells;
		*data = m_soilMoist_60;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST100)) {
		*n = m_nCells;
		*data = m_soilMoist_100;
	}
	else if (StringMatch(sk, VAR_SOL_MOIST200)) {
		*n = m_nCells;
		*data = m_soilMoist_200;
	}
	///////////////////////
	else if (StringMatch(sk, VAR_SOL_SAT1)) {
		*n = m_nCells;
		*data = m_soilSat1;
	}
	else if (StringMatch(sk, VAR_SOL_SAT5)) {
		*n = m_nCells;
		*data = m_soilSat5;
	}
	else if (StringMatch(sk, VAR_SOL_SAT15)) {
		*n = m_nCells;
		*data = m_soilSat15;
	}
	else if (StringMatch(sk, VAR_SOL_SAT30)) {
		*n = m_nCells;
		*data = m_soilSat30;
	}
	else if (StringMatch(sk, VAR_SOL_SAT60)) {
		*n = m_nCells;
		*data = m_soilSat60;
	}
	else if (StringMatch(sk, VAR_SOL_SAT100)) {
		*n = m_nCells;
		*data = m_soilSat100;
	}
	else if (StringMatch(sk, VAR_SOL_SAT200)) {
		*n = m_nCells;
		*data = m_soilSat200;
	}
	//////////////
	else if (StringMatch(sk, VAR_SOL_AWC1)) {
		*n = m_nCells;
		*data = m_soilAWC1;
	}
	else if (StringMatch(sk, VAR_SOL_AWC5)) {
		*n = m_nCells;
		*data = m_soilAWC5;
	}
	else if (StringMatch(sk, VAR_SOL_AWC15)) {
		*n = m_nCells;
		*data = m_soilAWC15;
	}
	else if (StringMatch(sk, VAR_SOL_AWC30)) {
		*n = m_nCells;
		*data = m_soilAWC30;
	}
	else if (StringMatch(sk, VAR_SOL_AWC60)) {
		*n = m_nCells;
		*data = m_soilAWC60;
	}
	else if (StringMatch(sk, VAR_SOL_AWC100)) {
		*n = m_nCells;
		*data = m_soilAWC100;
	}
	else if (StringMatch(sk, VAR_SOL_AWC200)) {
		*n = m_nCells;
		*data = m_soilAWC200;
	}

	else if (StringMatch(sk, VAR_KS1)) {
		*n = m_nCells;
		*data = m_ks1;
	}
	else if (StringMatch(sk, VAR_KS5)) {
		*n = m_nCells;
		*data = m_ks5;
	}
	else if (StringMatch(sk, VAR_KS15)) {
		*n = m_nCells;
		*data = m_ks15;
	}
	else if (StringMatch(sk, VAR_KS30)) {
		*n = m_nCells;
		*data = m_ks30;
	}
	else if (StringMatch(sk, VAR_KS60)) {
		*n = m_nCells;
		*data = m_ks60;
	}
	else if (StringMatch(sk, VAR_KS100)) {
		*n = m_nCells;
		*data = m_ks100;
	}
	else if (StringMatch(sk, VAR_KS200)) {
		*n = m_nCells;
		*data = m_ks200;
	}
	

    else {
        throw ModelException(MID_SSR_DA, "Get1DData", "Result " + sk + " does not exist.");
    }

}

void SSR_DA::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
    InitialOutputs();
    string sk(key);
    *nrows = m_nCells;
    *ncols = m_maxSoilLyrs;

    if (StringMatch(sk, VAR_SSRU)) {
        *data = m_subSurfRf;
    } else if (StringMatch(sk, VAR_SSRUVOL)) {
        *data = m_subSurfRfVol;
    } else if (StringMatch(sk, VAR_SOL_ST)) {
		*data = m_soilWtrSto;
	}else if (StringMatch(sk, VAR_PERCO)) {
		*data = m_soilPerco;
	}
	else {
        throw ModelException(MID_SSR_DA, "Get2DData", "Output " + sk + " does not exist.");
    }
}
void SSR_DA::SetReaches(clsReaches* reaches) {
	if (nullptr == reaches) {
		throw ModelException(MID_MUSK_CH_HAND, "SetReaches", "The reaches input can not to be NULL.");
	}
	m_nSubbsns = reaches->GetReachNumber();

	if (nullptr == m_chWidth) reaches->GetReachesSingleProperty(REACH_WIDTH, &m_chWidth);
}
bool SSR_DA::CheckInputData() {
    CHECK_NONNEGATIVE(MID_SSR_DA, m_inputSubbsnID);
    CHECK_POSITIVE(MID_SSR_DA, m_nCells);
    CHECK_POSITIVE(MID_SSR_DA, m_ki);
    CHECK_NODATA(MID_SSR_DA, m_soilFrozenTemp);
    CHECK_POSITIVE(MID_SSR_DA, m_dt);
    CHECK_POSITIVE(MID_SSR_DA, m_CellWth);
    CHECK_POSITIVE(MID_SSR_DA, m_nSubbsns);
    CHECK_POSITIVE(MID_SSR_DA, m_nRteLyrs);
    CHECK_POINTER(MID_SSR_DA, m_subbsnID);
    CHECK_POINTER(MID_SSR_DA, m_nSoilLyrs);
    CHECK_POINTER(MID_SSR_DA, m_soilThk);
    CHECK_POINTER(MID_SSR_DA, m_slope);
    CHECK_POINTER(MID_SSR_DA, m_poreIdx);
    CHECK_POINTER(MID_SSR_DA, m_ks);
    CHECK_POINTER(MID_SSR_DA, m_soilSat);
    //CHECK_POINTER(MID_SSR_DA, m_soilFC);
    CHECK_POINTER(MID_SSR_DA, m_soilWP);
    CHECK_POINTER(MID_SSR_DA, m_soilWtrSto);
    CHECK_POINTER(MID_SSR_DA, m_soilWtrStoPrfl);
	//CHECK_POINTER(MID_SSR_DA, m_soilTemp);     //  xiaodw comment, don't need soil temperature now
	//CHECK_POINTER(MID_SSR_DA, m_chWidth);
    CHECK_POINTER(MID_SSR_DA, m_rchID);
    CHECK_POINTER(MID_SSR_DA, m_flowInIdxD8);
    CHECK_POINTER(MID_SSR_DA, m_rteLyrs);
    return true;
}

void SSR_DA::InitialOutputs() {
    CHECK_POSITIVE(MID_SSR_DA, m_nCells);
    CHECK_POSITIVE(MID_SSR_DA, m_nSubbsns);
    if (nullptr == m_ifluQ2Rch) Initialize1DArray(m_nSubbsns + 1, m_ifluQ2Rch, 0.f);
    if (nullptr == m_subSurfRf) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_subSurfRf, 0.f);
    if (nullptr == m_subSurfRfVol) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_subSurfRfVol, 0.f);
    if (nullptr == m_TTlag) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_TTlag, 0.f);
    if (nullptr == m_cellFlow) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_cellFlow, 0.f);
	if (nullptr == m_soilMoist_1) Initialize1DArray(m_nCells, m_soilMoist_1, 0.f);
	if (nullptr == m_soilMoist_5) Initialize1DArray(m_nCells, m_soilMoist_5, 0.f);
	if (nullptr == m_soilMoist_15) Initialize1DArray(m_nCells, m_soilMoist_15, 0.f);
	if (nullptr == m_soilMoist_30) Initialize1DArray(m_nCells, m_soilMoist_30, 0.f);
	if (nullptr == m_soilMoist_60) Initialize1DArray(m_nCells, m_soilMoist_60, 0.f);
	if (nullptr == m_soilMoist_100) Initialize1DArray(m_nCells, m_soilMoist_100, 0.f);
	if (nullptr == m_soilMoist_200) Initialize1DArray(m_nCells, m_soilMoist_200, 0.f);
	if (nullptr == m_soilMoist) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilMoist, 0.f);
	if (nullptr == m_soilMoistBfe) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilMoistBfe, 0.f);
	if (nullptr == m_soilWtrStoBfe) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilWtrStoBfe, 0.f);

	if (nullptr == m_soilSat1) Initialize1DArray(m_nCells, m_soilSat1, 0.f);
	if (nullptr == m_soilSat5) Initialize1DArray(m_nCells, m_soilSat5, 0.f);
	if (nullptr == m_soilSat15) Initialize1DArray(m_nCells, m_soilSat15, 0.f);
	if (nullptr == m_soilSat30) Initialize1DArray(m_nCells, m_soilSat30, 0.f);
	if (nullptr == m_soilSat60) Initialize1DArray(m_nCells, m_soilSat60, 0.f);
	if (nullptr == m_soilSat100) Initialize1DArray(m_nCells, m_soilSat100, 0.f);
	if (nullptr == m_soilSat200) Initialize1DArray(m_nCells, m_soilSat200, 0.f);

	if (nullptr == m_soilAWC1) Initialize1DArray(m_nCells, m_soilAWC1, 0.f);
	if (nullptr == m_soilAWC5) Initialize1DArray(m_nCells, m_soilAWC5, 0.f);
	if (nullptr == m_soilAWC15) Initialize1DArray(m_nCells, m_soilAWC15, 0.f);
	if (nullptr == m_soilAWC30) Initialize1DArray(m_nCells, m_soilAWC30, 0.f);
	if (nullptr == m_soilAWC60) Initialize1DArray(m_nCells, m_soilAWC60, 0.f);
	if (nullptr == m_soilAWC100) Initialize1DArray(m_nCells, m_soilAWC100, 0.f);
	if (nullptr == m_soilAWC200) Initialize1DArray(m_nCells, m_soilAWC200, 0.f);

	if (nullptr == m_ks1) Initialize1DArray(m_nCells, m_ks1, 0.f);
	if (nullptr == m_ks5) Initialize1DArray(m_nCells, m_ks5, 0.f);
	if (nullptr == m_ks15) Initialize1DArray(m_nCells, m_ks15, 0.f);
	if (nullptr == m_ks30) Initialize1DArray(m_nCells, m_ks30, 0.f);
	if (nullptr == m_ks60) Initialize1DArray(m_nCells, m_ks60, 0.f);
	if (nullptr == m_ks100) Initialize1DArray(m_nCells, m_ks100, 0.f);
	if (nullptr == m_ks200) Initialize1DArray(m_nCells, m_ks200, 0.f);


    if (nullptr == m_slplen){
        Initialize1DArray(m_nCells, m_slplen, 30.f);
        for (int i = 0; i < m_nCells; i++) {
            m_slplen[i] = m_dis2Stream[i];
            m_slplen[i] = Min(m_slplen[i],300.f);
            m_slplen[i] = Max(m_slplen[i],1.f);
            //ljj++ consitent with SERO
            if(m_slope[i] <= 0.1)   m_slplen[i] = 61;
            if(m_slope[i] <= 0.2 && m_slope[i] > 0.1)   m_slplen[i] = 24;
            if(m_slope[i] > 0.2)   m_slplen[i] = 9.1;
        }
    }
}
