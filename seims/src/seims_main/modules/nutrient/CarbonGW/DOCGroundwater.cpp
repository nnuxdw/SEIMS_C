#include "DOCGroundwater.h"

#include "text.h"

DOCGroundwater::DOCGroundwater():
	//input
	m_nSubbsns(-1),  m_nCells(-1),curBasinArea(nullptr),
	m_subbsnID(nullptr),m_soilPerco(nullptr), m_hlife_docgw(NODATA_VALUE),
	m_subbasinsInfo(nullptr),m_dp_co(NODATA_VALUE),m_area(nullptr),m_soilPercoDIC(nullptr),
	
	//output
	m_gw_DOCSto(nullptr), m_gw_DOCconc(nullptr),  m_Deepgrndwtr_DOC(nullptr),m_recharge1(nullptr),
	m_gwDOCtoCH(nullptr),m_maxSoilLyrs(-1),gw_delay(NODATA_VALUE),gw_delay_1d(nullptr),m_hlife_docgw_1d(nullptr),
	m_gw_DICSto(nullptr),m_Deepgrndwtr_DIC(nullptr),m_recharge2(nullptr),m_gwDICtoCH(nullptr)
{
}

DOCGroundwater::~DOCGroundwater() {
	if (curBasinArea != nullptr) Release1DArray(curBasinArea);
	if (m_recharge1 != nullptr) Release1DArray(m_recharge1);
	if (m_gw_DOCSto != nullptr) Release1DArray(m_gw_DOCSto);
	if (m_Deepgrndwtr_DOC != nullptr) Release1DArray(m_Deepgrndwtr_DOC);
	if (m_gwDOCtoCH != nullptr) Release1DArray(m_gwDOCtoCH);
	if (m_gwDICtoCH != nullptr) Release1DArray(m_gwDICtoCH);
	if (m_recharge2 != nullptr) Release1DArray(m_recharge2);
	if (m_gw_DICSto != nullptr) Release1DArray(m_gw_DICSto);
	if (m_Deepgrndwtr_DIC != nullptr) Release1DArray(m_Deepgrndwtr_DIC);
	if (m_gw_DOCconc != nullptr) Release1DArray(m_gw_DOCconc);

}

void DOCGroundwater::SetSubbasins(clsSubbasins* subbasins) {
	if (nullptr == m_subbasinsInfo) {
		m_subbasinsInfo = subbasins;
		m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
	}
}

bool DOCGroundwater::CheckInputData() {
	CHECK_POSITIVE(MID_CarbonGW, m_nSubbsns);
	CHECK_POSITIVE(MID_CarbonGW, m_nCells);
    return true;
}

void DOCGroundwater::SetValue(const char* key, const float value) {
    string sk(key);
	if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);

	if (StringMatch(sk, VAR_DF_COEF)) m_dp_co = value;
	if (StringMatch(sk, VAR_HLDOCGW)) m_hlife_docgw = value;
	if (StringMatch(sk, "gw_delay")) gw_delay = value;	

	if (StringMatch(sk, VAR_KDOC)) m_kdoc = value;
}

void DOCGroundwater::Set1DData(const char* key, const int n, float* data) {
	string sk(key);  
	if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
	if (StringMatch(sk, VAR_AHRU)) m_area = data;
	if (StringMatch(sk, VAR_PERC_LOWEST_DOC)) m_soilPerco = data;
	if (StringMatch(sk, VAR_PERC_LOWEST_DIC)) m_soilPercoDIC = data;
	if (StringMatch(sk, "gw_delay_1d")) gw_delay_1d = data;
	if (StringMatch(sk, "hlife_docgw_1d")) m_hlife_docgw_1d = data;


}

void DOCGroundwater::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
	string sk(key);
	CheckInputSize2D(MID_NUTR_TF, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
}

void DOCGroundwater::InitialOutputs() {
	CHECK_POSITIVE(MID_CarbonGW, m_nSubbsns);
	CHECK_POSITIVE(MID_CarbonGW, m_nCells);
	if (curBasinArea == nullptr) {
		Initialize1DArray(m_nSubbsns + 1, curBasinArea, 0.f);
		for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
		int subID = *it;
		Subbasin* curSub = m_subbasinsInfo->GetSubbasinByID(subID);
		// get percolation from the bottom soil layer at the subbasin scale
		int curCellsNum = curSub->GetCellCount();
		int* curCells = curSub->GetCells();
			for (int i = 0; i < curCellsNum; i++) {
				curBasinArea[subID] += m_area[curCells[i]];
			}
		}
	}
	if (m_recharge1 == nullptr) Initialize1DArray(m_nSubbsns + 1, m_recharge1, 0.f);
	if (m_Deepgrndwtr_DOC == nullptr)Initialize1DArray(m_nSubbsns + 1, m_Deepgrndwtr_DOC, 0.f);
	if (m_gwDOCtoCH == nullptr)Initialize1DArray(m_nSubbsns + 1, m_gwDOCtoCH, 0.f);
	if (m_gw_DOCSto == nullptr)Initialize1DArray(m_nSubbsns + 1, m_gw_DOCSto, 0.f);
	if (m_gw_DOCconc == nullptr)Initialize1DArray(m_nSubbsns + 1, m_gw_DOCconc, 0.f);
	if (m_recharge2 == nullptr) Initialize1DArray(m_nSubbsns + 1, m_recharge2, 0.f);
	if (m_Deepgrndwtr_DIC == nullptr)Initialize1DArray(m_nSubbsns + 1, m_Deepgrndwtr_DIC, 0.f);
	if (m_gwDICtoCH == nullptr)Initialize1DArray(m_nSubbsns + 1, m_gwDICtoCH, 0.f);
	if (m_gw_DICSto == nullptr)Initialize1DArray(m_nSubbsns + 1, m_gw_DICSto, 0.f);

}

int DOCGroundwater::Execute() {
    CheckInputData();
    InitialOutputs();
	for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
		int subID = *it;
		m_gw_DOCconc[subID] = 0.f;

		Subbasin* curSub = m_subbasinsInfo->GetSubbasinByID(subID);
		// get percolation from the bottom soil layer at the subbasin scale
		int curCellsNum = curSub->GetCellCount();
		int* curCells = curSub->GetCells();
		float perco = 0.f;
		//float perco_dic = 0.f;
		float rchrg1 = 0.f;
		float rchrg2 = 0.f;
		float rep = 0.f;
		//if (curSub->GetPerco() > 0.f) rchrg1 = m_recharge1[subID];
		if (curSub->GetEg() > 0.f) rep = curSub->GetPerco();
		for (int i = 0; i < curCellsNum; i++) {
            int index = curCells[i];
            float tmp_perc = m_soilPerco[index];
			//float tmp_perc_dic = m_soilPercoDIC[index];
            if (tmp_perc > 0) {
                perco += tmp_perc * (m_area[index] / curBasinArea[subID]);
				
			} else {
                m_soilPerco[index] = 0.f;
            }
			// if (tmp_perc_dic > 0) {
            //     perco_dic += tmp_perc_dic * (m_area[index] / curBasinArea[subID]);
				
			// } else {
            //     m_soilPercoDIC[index] = 0.f;
            // }
        }
		rchrg1 = m_recharge1[subID];
		//rchrg2 = m_recharge2[subID];
        m_recharge1[subID] = 0.f;
		//m_recharge2[subID] = 0.f;
        float gw_delaye = exp(-1./(gw_delay));
        //float gw_delaye = exp(-1./(gw_delay_1d[subID]));
        m_recharge1[subID] = (1.- gw_delaye) * perco + gw_delaye * rchrg1;
		//m_recharge2[subID] = (1.- gw_delaye) * perco_dic + gw_delaye * rchrg2;
        perco = m_recharge1[subID];
		//perco_dic = m_recharge2[subID];
		float ratio2gw = 1.f;
        perco *= ratio2gw;
        float percoDeep = perco * m_dp_co; ///< deep percolation
		//float percoDeep_dic = perco_dic * m_dp_co; ///< deep percolation

        m_gw_DOCSto[subID] += (perco - percoDeep);   //kg/ha
		//m_gw_DICSto[subID] += (perco_dic - percoDeep_dic);   //kg/ha
		
		float m_deepWaterDepth = curSub->GetGw();
		float m_RG = curSub->GetRg();
		float m_EG = curSub->GetEg();
		float xx = (m_deepWaterDepth + m_RG);
		//float xx2 = (m_deepWaterDepth + m_RG);
		if (xx > 0.f) {
			xx = m_gw_DOCSto[subID] / (m_deepWaterDepth + m_RG + m_EG);
			//xx2 = m_gw_DICSto[subID] / (m_deepWaterDepth + m_RG);
		}
		else {
			xx = 0.f;
			//xx2 = 0.f;
		}

		if (xx < 1.e-6f) {
			xx = 0.f;
			//xx2 = 0.f;
		}
		m_Deepgrndwtr_DOC[subID] = 0.f;
		m_Deepgrndwtr_DOC[subID] = xx * m_RG;
		//m_Deepgrndwtr_DIC[subID] = 0.f;
		//m_Deepgrndwtr_DIC[subID] = xx2 * m_RG;
		//subtract DOC transport losses from the shallow aquifer
		m_gw_DOCSto[subID] = m_gw_DOCSto[subID] - m_Deepgrndwtr_DOC[subID];
		m_gw_DOCSto[subID] = Max(0.f, m_gw_DOCSto[subID]);
		//m_gw_DICSto[subID] = m_gw_DICSto[subID] - m_Deepgrndwtr_DIC[subID];
		//m_gw_DICSto[subID] = Max(0.f, m_gw_DICSto[subID]);

		//compute DOC reaction losses in the groundwater
		float temp_doc = m_gw_DOCSto[subID];
		m_gw_DOCSto[subID] = m_gw_DOCSto[subID] * exp(-0.693f /m_hlife_docgw);
		//m_gw_DOCSto[subID] = m_gw_DOCSto[subID] * exp(-0.693f /m_hlife_docgw_1d[subID]);
		m_gw_DOCSto[subID] = Max(0.f, m_gw_DOCSto[subID]);
		//m_gw_DICSto[subID] += temp_doc - m_gw_DOCSto[subID];
		//m_gw_DICSto[subID] = Max(0.f, m_gw_DICSto[subID]);

	}

//#pragma omp parallel 不要并行
    {
        float* tmp_dDOCtoCH = new(nothrow) float[m_nSubbsns + 1];
		//float* tmp_dDICtoCH = new(nothrow) float[m_nSubbsns + 1];
        for (int i = 0; i <= m_nSubbsns; i++) {
            tmp_dDOCtoCH[i] = 0.f;
			m_gwDOCtoCH[i] = 0.f;
			//tmp_dDICtoCH[i] = 0.f;
			//m_gwDICtoCH[i] = 0.f;
        }
        for (int i = 1; i < m_nSubbsns; i++) {
            m_gwDOCtoCH[i] += m_Deepgrndwtr_DOC[i] * curBasinArea[i] * 0.0001f;  //kg
			//m_gwDICtoCH[i] += m_Deepgrndwtr_DIC[i] * curBasinArea[i] * 0.0001f;  //kg
        }

		delete[] tmp_dDOCtoCH;
		//delete[] tmp_dDICtoCH;
        //tmp_dDICtoCH = nullptr;
        tmp_dDOCtoCH = nullptr;

    } /* END of #pragma omp parallel */
    // sum all the subbasins and put the sum value in the zero-index of the array
    for (int i = 1; i < m_nSubbsns + 1; i++) {
        m_gwDOCtoCH[0] += m_gwDOCtoCH[i];   //units: kg
		//m_gwDICtoCH[0] += m_gwDICtoCH[i];   //units: kg
    }
		return 0;
}

void DOCGroundwater::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);
	if   (StringMatch(sk, "gw_RDOCtoCH")) {
		*data = m_gwDOCtoCH;
		*n = m_nSubbsns + 1;
	}
	if   (StringMatch(sk, "gw_DOCsto")) {
		*data = m_gw_DOCSto;
		*n = m_nSubbsns + 1;
	}
	// if   (StringMatch(sk, "gw_DICtoCH")) {
	// 	*data = m_gwDICtoCH;
	// 	*n = m_nSubbsns + 1;
	// }
   }


void DOCGroundwater::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string sk(key);
}