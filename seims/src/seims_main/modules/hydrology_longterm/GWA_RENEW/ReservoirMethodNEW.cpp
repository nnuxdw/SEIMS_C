#include "ReservoirMethodNEW.h"

#include "text.h"

ReservoirMethodNEW::ReservoirMethodNEW() :
    m_dt(-1), m_nCells(-1), m_cellWth(NODATA_VALUE), m_maxSoilLyrs(-1),
    m_nSoilLyrs(nullptr), m_soilThk(nullptr),
    m_dp_co(NODATA_VALUE), m_Kg(NODATA_VALUE), m_Base_ex(NODATA_VALUE),
    m_soilPerco(nullptr), m_IntcpET(nullptr), m_deprStoET(nullptr),
    m_soilET(nullptr), m_actPltET(nullptr), m_pet(nullptr),
    m_revap(nullptr), m_GW0(NODATA_VALUE), m_GWMAX(NODATA_VALUE),/*, m_GWT0(NODATA_VALUE)*/
    m_petSubbsn(nullptr), m_gwSto(nullptr), m_gwTab(nullptr), m_slope(nullptr), m_soilWtrSto(nullptr),
    m_soilDepth(nullptr),
    m_VgroundwaterFromBankStorage(nullptr), m_T_Perco(nullptr),
    /// intermediate
    m_T_PerDep(nullptr), m_T_RG(nullptr),
    /// outputs
    m_T_QG(nullptr), m_T_Revap(nullptr), m_T_GWWB(nullptr),
    m_nSubbsns(-1), m_inputSubbsnID(-1), m_subbasinsInfo(nullptr),
    m_area(nullptr), curBasinArea(nullptr), gwSub(nullptr), QGSub(nullptr), m_surfRf(nullptr), m_potVol(nullptr), m_infil(nullptr), m_impoundTrig(nullptr),
	// xiaodw++
	m_GWMAX_1d(nullptr), m_Base_ex_1d(nullptr), m_Kg_1d(nullptr), gw_delay_1d(nullptr), m_hand_eavp(nullptr), m_handWtrDep(nullptr), m_chSto(nullptr)
{
}

ReservoirMethodNEW::~ReservoirMethodNEW() {
    if (m_T_Perco != nullptr) Release1DArray(m_T_Perco);
    if (m_T_PerDep != nullptr) Release1DArray(m_T_PerDep);
    if (m_revap != nullptr) Release1DArray(m_revap);
    if (m_T_Revap != nullptr) Release1DArray(m_T_Revap);
    if (m_T_RG != nullptr) Release1DArray(m_T_RG);
    if (m_T_QG != nullptr) Release1DArray(m_T_QG);
    if (m_petSubbsn != nullptr) Release1DArray(m_petSubbsn);
    if (m_gwSto != nullptr) Release1DArray(m_gwSto);
    if (m_T_GWWB != nullptr) Release2DArray(m_nSubbsns + 1, m_T_GWWB);
    if (curBasinArea != nullptr) Release1DArray(curBasinArea);
}

void ReservoirMethodNEW::InitialOutputs() {
    CHECK_POSITIVE(MID_GWA_RE, m_nSubbsns);
    int nLen = m_nSubbsns + 1;
    if (m_T_Perco == nullptr) Initialize1DArray(nLen, m_T_Perco, 0.f);
    if (m_T_Revap == nullptr) Initialize1DArray(nLen, m_T_Revap, 0.f);
    if (m_T_PerDep == nullptr) Initialize1DArray(nLen, m_T_PerDep, 0.f);
    if (m_T_RG == nullptr) Initialize1DArray(nLen, m_T_RG, 0.f);
    if (m_T_QG == nullptr) Initialize1DArray(nLen, m_T_QG, 0.f);
    if (m_petSubbsn == nullptr) Initialize1DArray(nLen, m_petSubbsn, 0.f);
    if (m_gwSto == nullptr) Initialize1DArray(nLen, m_gwSto, m_GW0);
    //if (m_gwTab == nullptr) Initialize1DArray(nLen, m_gwTab, m_GWT0);
    if (m_revap == nullptr) Initialize1DArray(m_nCells, m_revap, 0.f);
    if (m_T_GWWB == nullptr) Initialize2DArray(nLen, 6, m_T_GWWB, 0.f);
    if (curBasinArea == nullptr) Initialize1DArray(nLen, curBasinArea, 0.f);
    if (gwSub == nullptr) Initialize1DArray(m_nCells, gwSub, m_GW0);
    if (QGSub == nullptr) Initialize1DArray(m_nCells, QGSub, 0);

}

int ReservoirMethodNEW::Execute() {
    CheckInputData();
    InitialOutputs();
    //float QGConvert = 1.f * m_cellWth * m_cellWth / m_dt * 0.001f; // mm ==> m3/s
    float QGConvert = 1.f * 0.001f / m_dt;
    float total_area = 0.f;
    for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
        int subID = *it;
        Subbasin* curSub = m_subbasinsInfo->GetSubbasinByID(subID);
        // get percolation from the bottom soil layer at the subbasin scale
        int curCellsNum = curSub->GetCellCount();
        int* curCells = curSub->GetCells();
        float perco = 0.f;
        float fPET = 0.f;
        float revap = 0.f;
        //float curBasinArea = 0.f;
		curBasinArea[subID] = 0.f;
        for (int i = 0; i < curCellsNum; i++) {
            curBasinArea[subID] += m_area[curCells[i]];
        }
        //if (subID ==25)gwSub
        //{
        //    cout << endl;
        //}
        total_area += curBasinArea[subID];
        //#pragma omp parallel for reduction(+:perco, fPET, revap)
        for (int i = 0; i < curCellsNum; i++) {
            int index = curCells[i];
			int nly = CVT_INT(m_nSoilLyrs[index]);
			int last = nly - 1;

			// 先缓存“更新前”的量（用于算变化量 ）
			float gw_before = gwSub[index];
			float soil_before_last = m_soilWtrSto[index][last];

			// 如果你还想看每层变化量，就把每层都缓存下来
			vector<float> soil_before;
			soil_before.reserve(nly);
			for (int ly = 0; ly < nly; ++ly) soil_before.push_back(m_soilWtrSto[index][ly]);

            float tmp_perc = MAX(0, m_soilPerco[index][CVT_INT(m_nSoilLyrs[index]) - 1]);
            if (tmp_perc + gwSub[index] >= m_GWMAX_1d[subID])
            {
                float excessWater = tmp_perc + gwSub[index] - m_GWMAX_1d[subID];
                m_soilWtrSto[index][CVT_INT(m_nSoilLyrs[index]) - 1] += excessWater;
                tmp_perc = tmp_perc - excessWater;
                m_soilPerco[index][CVT_INT(m_nSoilLyrs[index]) - 1] = tmp_perc;
                /// for the last soil layer
                if (m_soilWtrSto[index][CVT_INT(m_nSoilLyrs[index]) - 1] - m_soilSat[index][CVT_INT(m_nSoilLyrs[index]) - 1] > 1.e-4f) {
                    float ul_excess = m_soilWtrSto[index][CVT_INT(m_nSoilLyrs[index]) - 1] - m_soilSat[index][CVT_INT(m_nSoilLyrs[index]) - 1];
                    m_soilWtrSto[index][CVT_INT(m_nSoilLyrs[index]) - 1] = m_soilSat[index][CVT_INT(m_nSoilLyrs[index]) - 1];
                    for (int ly = CVT_INT(m_nSoilLyrs[index]) - 2; ly >= 0; ly--) {
                        m_soilWtrSto[index][ly] += ul_excess;
                        m_soilPerco[index][ly] -= ul_excess;
                        if (m_soilWtrSto[index][ly] > m_soilSat[index][ly]) {
                            ul_excess = m_soilWtrSto[index][ly] - m_soilSat[index][ly];
                            m_soilWtrSto[index][ly] = m_soilSat[index][ly];
                        }
                        else {
                            ul_excess = 0.f;
                            break;
                        }
                        if (ly == 0 && ul_excess > 0.f) {
                            // add ul_excess to depressional storage and then to surfq
                            if (m_potVol != nullptr && FloatEqual(m_impoundTrig[index], 0.f)) {
                                m_potVol[index] += ul_excess;
                            }
							// xiaodw, add ul_excess to hand's inundation depth
							else if (m_handWtrDep[index] > 0.f) {
								m_chSto[subID] += ul_excess * m_area[index] * 0.001f;
							}
                           /* else if (!wascobSubarea[index].empty())
                            {
                                for (auto b : wascobSubarea[index])
                                {
                                    float water_m3 = ul_excess / 1e3f * b->m_drainageAreaSubareas[b->SubareaId];
                                    b->addWaterStorage(water_m3);
                                }
                            }*/
                            else
                            {
                                m_surfRf[index] += ul_excess;

                            }
                            m_infil[index] -= ul_excess;
                            if (m_infil[index] < 0)
                            {
                                m_infil[index] = 0;
                            }
                        }
                    }
                }
            }
       /*     if (!tiledrainSubarea[index].empty())
            {
                for (auto b : tiledrainSubarea[index])
                {
                    if (b->getTileDepth(m_date) > b->DepthToImperviableLayer - gwSub[index])
                    {
                        m_revap[index] = 0;
                    }
                    else
                    {
                        m_revap[index] = m_pet[index] - m_IntcpET[index] - m_deprStoET[index] - m_soilET[index] - m_actPltET[index];
                        m_revap[index] = Max(m_revap[index], 0.f);
                        m_revap[index] = m_revap[index] * m_gwSto[subID] / m_GWMAX;
                    }
                }
            }
            else
            {*/
                //m_revap[index] = m_pet[index] - m_IntcpET[index] - m_deprStoET[index] - m_soilET[index] - m_actPltET[index];
				m_revap[index] = m_pet[index] - m_IntcpET[index] - m_deprStoET[index] - m_soilET[index] - m_hand_eavp[index];
                m_revap[index] = Max(m_revap[index], 0.f);
                m_revap[index] = m_revap[index] * m_gwSto[subID] / m_GWMAX_1d[subID];
            //}
            float dGW = tmp_perc - m_revap[index];
            gwSub[index] = gwSub[index] + dGW;
            if (tmp_perc > 0) {
                //perco += tmp_perc;
                perco += tmp_perc * (m_area[index] / curBasinArea[subID]);
            }
            else {
                m_soilPerco[index][CVT_INT(m_nSoilLyrs[index]) - 1] = 0.f;
            }
            if (m_pet[index] > 0.f) {
                //fPET += m_pet[index];
                fPET += m_pet[index] * (m_area[index] / curBasinArea[subID]);
            }
            ////revap += m_revap[index];
            revap += m_revap[index] * (m_area[index] / curBasinArea[subID]);

			/// xiaodw++, output for debug
			#ifdef DEBUG_GWA_RENEW
			if (index == 15012) {
				float tmp_perc_last = m_soilPerco[index][last];      // 最下层向地下水渗漏（mm）——注意你前面可能会被改写
				float revap_mm = m_revap[index];                     // 地下水蒸发回补（mm）
				float dGW_mm = tmp_perc_last - revap_mm;             // 本步地下水变化量（mm）（按你的更新公式）
				float gw_after = gwSub[index];

				std::cout << std::fixed << std::setprecision(6);
				std::cout << "\n[GWA_RENEW] subID=" << subID
					<< " index=" << index
					<< " date=" << m_year << "-" << m_month << "-" << m_day
					<< " dt=" << m_dt
					<< "\n  nly=" << nly
					<< " area=" << m_area[index]
					<< " slopeCoef=" << curSub->GetSlopeCoef()
					<< "\n  --- Fluxes (mm) ---"
					<< "\n  perc_last(mm)=" << tmp_perc_last
					<< "  revap(mm)=" << revap_mm
					<< "  dGW(mm)=perc_last-revap=" << dGW_mm
					<< "\n  --- GW Storage (mm) ---"
					<< "\n  gw_before(mm)=" << gw_before
					<< "  gw_after(mm)=" << gw_after
					<< "  GWMAX_1d(mm)=" << m_GWMAX_1d[subID]
					<< "\n  --- Soil Water (mm) ---";

				// 每层土壤含水量 & 饱和含水量 & 变化量
				for (int ly = 0; ly < nly; ++ly) {
					float sw_before = soil_before[ly];
					float sw_after = m_soilWtrSto[index][ly];
					float dsw = sw_after - sw_before;
					float sat = m_soilSat[index][ly];

					std::cout << "\n  ly=" << ly
						<< "  soil_before=" << sw_before
						<< "  soil_after=" << sw_after
						<< "  dSoil=" << dsw
						<< "  sat=" << sat
						<< "  (after-sat)=" << (sw_after - sat);
				}

				// 额外：一些你这段里最容易导致异常的关键量
				std::cout << "\n  --- Key Vars ---"
					<< "\n  infiltr(mm)=" << m_infil[index]
					<< "  surfRf(mm)=" << m_surfRf[index]
					<< "  handWtrDep(m)=" << m_handWtrDep[index]
					<< "  hand_eavp(mm)=" << m_hand_eavp[index]
					<< "\n  pet=" << m_pet[index]
					<< "  IntcpET=" << m_IntcpET[index]
					<< "  deprStoET=" << m_deprStoET[index]
					<< "  soilET=" << m_soilET[index]
					<< "\n" << std::endl;
			}
#endif
        }
        // perco /= curCellsNum; // mean mm
        // fPET /= curCellsNum;
        // revap /= curCellsNum;
        /// percolated water ==> vadose zone ==> shallow aquifer ==> deep aquifer
        /// currently, for convenience, we assume a small portion of the percolated water
        /// will enter groundwater. By LJ. 2016-9-2
        float ratio2gw = 1.f;
        perco *= ratio2gw;
        float percoDeep = perco * m_dp_co; ///< deep percolation

        if (revap > m_gwSto[subID]) {
            for (int i = 0; i < curCellsNum; i++) {
                int index = 0;
                index = curCells[i];
                m_revap[index] *= m_gwSto[subID] / revap;
                cout << " m_revap[index] : " << m_revap[index] << " index: " << index
                    << " m_pet[index]: " << m_pet[index] << " m_IntcpET[index]: " << m_IntcpET[index] << " m_deprStoET[index]: " << m_deprStoET[index]
                    << " m_soilET[index]: " << m_soilET[index] << " m_actPltET[index]: " << m_actPltET[index] << endl;
                throw ModelException("ReservoirMethodNEW", "Execute", "蒸发过大");
            }
            revap = m_gwSto[subID];
        }

        // groundwater runoff (mm)
        float slopeCoef = curSub->GetSlopeCoef();
        float kg = m_Kg_1d[subID] * slopeCoef;
        float groundRunoff = kg * pow(m_gwSto[subID], m_Base_ex_1d[subID]); // mm
        //float groundQ = groundRunoff * curCellsNum * QGConvert;     // groundwater discharge (m3/s)
        float groundQ = groundRunoff * curBasinArea[subID] * QGConvert;
        //if (m_gwSto[subID] > m_GWMAX) {
        //    groundRunoff += m_gwSto[subID] - m_GWMAX;
        //    //groundQ = groundRunoff * curCellsNum * QGConvert; // groundwater discharge (m3/s)
        //    groundQ = groundRunoff * curBasinArea * QGConvert;
        //    m_gwSto[subID] = m_GWMAX;
        //}
        double totalVolume = 0;
        for (int i = 0; i < curCellsNum; i++) {//这里谨慎加并行，要不然容易一次一个结果
            int index = curCells[i];
            totalVolume += gwSub[index] * m_area[index];
			//cout << i << "  " << index << "  " << subID << "  " << totalVolume << endl;
			//cout.flush();
        }
        double dGWVolume = -(percoDeep + groundRunoff) * curBasinArea[subID];
        if (totalVolume == 0)
        {

        }
        else
        {
            for (int i = 0; i < curCellsNum; i++) {
                int index = curCells[i];
                QGSub[index] = groundRunoff * curBasinArea[subID] * (gwSub[index] * m_area[index]) / totalVolume / m_area[index];
                gwSub[index] += dGWVolume * (gwSub[index] * m_area[index]) / totalVolume / m_area[index];
                gwSub[index] = MAX(0, gwSub[index]);
            }
        }
        double groundStorage = 0;
		double groundStorageBefore = 0;
        //groundStorage += perco - revap - percoDeep - groundRunoff;
        for (int i = 0; i < curCellsNum; i++) {
            int index = curCells[i];
            groundStorage += gwSub[index] * m_area[index] / curBasinArea[subID];
        }
        //add the ground water from bank storage, 2011-3-14
        float gwBank = 0.f;
        // at the first time step m_VgroundwaterFromBankStorage is nullptr
        if (m_VgroundwaterFromBankStorage != nullptr) {
            gwBank = m_VgroundwaterFromBankStorage[subID];
        }
        groundStorage += gwBank / curSub->GetArea() * 1000.f;
        if (isnan(groundStorage))
        {
            cout << " m_year: " << m_year << " m_month: " << m_month << " m_day: " << m_day << " subID: " << subID << endl;
			cout.flush();
			throw ModelException("ReservoirMethodNEW", "EXECUTE", "地下水为nan");
        }
        groundStorage = Max(groundStorage, 0.f);
        for (int i = 0; i < curCellsNum; i++) {
            int index = curCells[i];
            gwSub[index] = groundStorage;
        }
        /**** Set values for current subbasin ****/
        curSub->SetPet(fPET);
        curSub->SetPerco(perco);
        curSub->SetPerde(percoDeep);
        curSub->SetEg(revap);
        curSub->SetRg(groundRunoff);
        curSub->SetQg(groundQ);
        curSub->SetGw(groundStorage);

        if (groundStorage != groundStorage) {
            std::ostringstream oss;
            oss << perco << "\t" << revap << "\t" << percoDeep << "\t" << groundRunoff << "\t" << m_gwSto[subID]
                << "\t" << m_Kg << "\t" << m_Base_ex << "\t" << slopeCoef << endl;
            throw ModelException("Subbasin", "setInputs", oss.str());
        }
#ifdef DEBUG_GWA_RENEW
		if (subID == 1171)
		{
			cout << "ID: " << subID <<
				", pet: " << std::fixed << setprecision(6) << fPET <<
				", perco: " << std::fixed << setprecision(6) << perco <<
				", percoDeep: " << std::fixed << setprecision(6) << percoDeep <<
				", revap: " << std::fixed << setprecision(6) << revap <<
				", groundRunoff: " << std::fixed << setprecision(6) << groundRunoff <<
				", groundQ: " << std::fixed << setprecision(6) << groundQ <<
				", gwStore: " << std::fixed << setprecision(6) << groundStorage << endl;
		}

#endif
        m_petSubbsn[subID] = curSub->GetPet();
        m_T_Perco[subID] = curSub->GetPerco();
        m_T_PerDep[subID] = curSub->GetPerde();
        m_T_Revap[subID] = curSub->GetEg();
        m_T_RG[subID] = curSub->GetRg(); //get rg of specific subbasin
        m_T_QG[subID] = curSub->GetQg(); //get qg of specific subbasin
        m_gwSto[subID] = curSub->GetGw();
    }
    m_petSubbsn[0] /= total_area;
    m_T_Perco[0] /= total_area;
    m_T_PerDep[0] /= total_area;
    m_T_Revap[0] /= total_area;
    m_T_RG[0] /= total_area;
    m_gwSto[0] /= total_area;
    // m_T_Perco[0] = m_subbasinsInfo->Subbasin2Basin(VAR_PERCO);
    // m_T_PerDep[0] = m_subbasinsInfo->Subbasin2Basin(VAR_PERDE);
    // m_T_Revap[0] = m_subbasinsInfo->Subbasin2Basin(VAR_REVAP);
    // m_T_RG[0] = m_subbasinsInfo->Subbasin2Basin(VAR_RG); // get rg of entire watershed
    // m_T_QG[0] = m_subbasinsInfo->Subbasin2Basin(VAR_QG); // get qg of entire watershed
    // m_gwSto[0] = m_subbasinsInfo->Subbasin2Basin(VAR_GW_Q);

    // output to GWWB, the sequence is coincident with the header information defined in PrintInfo.cpp, line 528.
    for (int i = 0; i <= m_nSubbsns; i++) {
        m_T_GWWB[i][0] = m_T_Perco[i];
        m_T_GWWB[i][1] = m_T_Revap[i];
        m_T_GWWB[i][2] = m_T_PerDep[i];
        m_T_GWWB[i][3] = m_T_RG[i];
        m_T_GWWB[i][4] = m_gwSto[i];
        m_T_GWWB[i][5] = m_T_QG[i];
    }

    // update soil moisture
    for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
        Subbasin* sub = m_subbasinsInfo->GetSubbasinByID(*it);
        int* cells = sub->GetCells();
        int nCells = sub->GetCellCount();
        int index = 0;
        //#pragma omp parallel for
        for (int i = 0; i < nCells; i++) {
            index = cells[i];
            m_soilWtrSto[cells[i]][CVT_INT(m_nSoilLyrs[cells[i]]) - 1] += m_revap[index];
        }
    }
    // TODO: Is it need to allocate revap to each soil layers??? By LJ
    return 0;
}



bool ReservoirMethodNEW::CheckInputData() {
    CHECK_POSITIVE(MID_GWA_RE, m_nCells);
    CHECK_POSITIVE(MID_GWA_RE, m_nSubbsns);
    CHECK_POSITIVE(MID_GWA_RE, m_cellWth);
    CHECK_POSITIVE(MID_GWA_RE, m_dt);
    CHECK_POSITIVE(MID_GWA_RE, m_maxSoilLyrs);
    CHECK_NODATA(MID_GWA_RE, m_dp_co);
    CHECK_NODATA(MID_GWA_RE, m_Kg);
    CHECK_NODATA(MID_GWA_RE, m_Base_ex);
    CHECK_POINTER(MID_GWA_RE, m_soilPerco);
    CHECK_POINTER(MID_GWA_RE, m_IntcpET);
    CHECK_POINTER(MID_GWA_RE, m_deprStoET);
    CHECK_POINTER(MID_GWA_RE, m_soilET);
    //CHECK_POINTER(MID_GWA_RE, m_actPltET);
    CHECK_POINTER(MID_GWA_RE, m_pet);
    CHECK_POINTER(MID_GWA_RE, m_slope);
    CHECK_POINTER(MID_GWA_RE, m_soilWtrSto);
    CHECK_POINTER(MID_GWA_RE, m_nSoilLyrs);
    CHECK_POINTER(MID_GWA_RE, m_soilThk);
    CHECK_POINTER(MID_GWA_RE, m_subbasinsInfo);
    return true;
}

// set value
void ReservoirMethodNEW::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, Tag_TimeStep)) m_dt = CVT_INT(value);
    else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
    else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
    else if (StringMatch(sk, Tag_CellWidth)) m_cellWth = value;
    else if (StringMatch(sk, VAR_KG)) m_Kg = value;
    else if (StringMatch(sk, VAR_Base_ex)) m_Base_ex = value;
    else if (StringMatch(sk, VAR_DF_COEF)) m_dp_co = value;
    else if (StringMatch(sk, VAR_GW0)) m_GW0 = value;
    //else if (StringMatch(sk, VAR_GWT0)) m_GWT0 = value;
    else if (StringMatch(sk, VAR_GWMAX)) m_GWMAX = value;
    else {
        throw ModelException(MID_GWA_RE, "SetValue", "Parameter " + sk + " does not exist in current module.");
    }
}

void ReservoirMethodNEW::Set1DData(const char* key, const int n, float* data) {
    string sk(key);
    if (StringMatch(sk, VAR_GWNEW)) {
        m_VgroundwaterFromBankStorage = data;
        return;
    }
    if (StringMatch(sk, VAR_ROOTDEPTH)) {
        m_rootDepth = data;
        return;
    }
    //check the input data
    //if (!CheckInputSize(MID_GWA_RE, key, n, m_nCells)) return;

    //set the value
    if (StringMatch(sk, VAR_INET)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_IntcpET = data;
    }
    else if (StringMatch(sk, VAR_DEET)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_deprStoET = data;
    }
    else if (StringMatch(sk, VAR_SOET)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_soilET = data;
    }
    else if (StringMatch(sk, VAR_AET_PLT)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_actPltET = data;
    }
    else if (StringMatch(sk, VAR_PET)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_pet = data;
    }
    else if (StringMatch(sk, VAR_SLOPE)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_slope = data;
    }
    else if (StringMatch(sk, VAR_SOILLAYERS)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_nSoilLyrs = data;
    } //ljj++
    else if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_area = data;
    }
    else if (StringMatch(sk, VAR_POT_VOL))
    {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_potVol = data;
    }
    else if (StringMatch(sk, VAR_SURU)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_surfRf = data;
    }
    else if (StringMatch(sk, VAR_IMPOUND_TRIG)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_impoundTrig = data;
    }
    else if (StringMatch(sk, VAR_INFIL)) {
		CheckInputSize(MID_GWA_RE, key, n, m_nCells);
        m_infil = data;
    }
	else if (StringMatch(sk, VAR_GWMAX_1D)) {
		m_GWMAX_1d = data;
	}
	else if (StringMatch(sk, "Base_ex_1d")) {
		m_Base_ex_1d = data;
	}
	else if (StringMatch(sk, "Kg_1d")) {
		m_Kg_1d = data;
	}
	else if (StringMatch(sk, "gw_delay_1d")) {
		gw_delay_1d = data;
	}
	else if (StringMatch(sk, VAR_CHST)) {
		// 注意这里按你的要求用的是 n - 1 和 m_nreach
		m_chSto = data;
	}
	else if (StringMatch(sk, VAR_HAND_EVAP)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_hand_eavp = data;
	}
	else if (StringMatch(sk, VAR_OL_HAND_WTRDEP)) {
		CheckInputSize(MID_SET_LM, key, n, m_nCells);
		m_handWtrDep = data;
	}
    //else if (StringMatch(sk, VAR_WASCOB))//WHC++
    //{
    //    wascobRaster = data;
    //}
    else {
        throw ModelException(MID_GWA_RE, "Set1DData", "Parameter " + sk + " does not exist in current module.");
    }
}

void ReservoirMethodNEW::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    string sk(key);
    CheckInputSize2D(MID_GWA_RE, key, nrows, ncols, m_nCells, m_maxSoilLyrs);

    if (StringMatch(sk, VAR_PERCO)) {
        m_soilPerco = data;
    }
    else if (StringMatch(sk, VAR_SOL_ST)) {
        m_soilWtrSto = data;
    }
    else if (StringMatch(sk, VAR_SOILDEPTH)) {
        m_soilDepth = data;
    }
    else if (StringMatch(sk, VAR_SOILTHICK)) {
        m_soilThk = data;
    }
    else if (StringMatch(sk, VAR_SOL_UL)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilSat = data;
    }
    else if (StringMatch(sk, VAR_FIELDCAP)) {
        m_fieldCap = data;
        return;
    }
    else if (StringMatch(sk, VAR_SOL_AWC)) {
        m_soilFC = data;
    }
    else if (StringMatch(sk, VAR_POROST)) {
        CheckInputSize2D(MID_SSR_DA, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
        m_soilPor = data;
    }
    else {
        throw ModelException(MID_GWA_RE, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void ReservoirMethodNEW::SetSubbasins(clsSubbasins* subbsns) {
    if (m_subbasinsInfo == nullptr) {
        m_subbasinsInfo = subbsns;
        // m_nSubbasins = m_subbasinsInfo->GetSubbasinNumber(); // Set in SetValue()! lj
        m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
    }
}
void ReservoirMethodNEW::SetScenario(Scenario* sce) {
    if (nullptr == sce) {
        throw ModelException(MID_GWA_RE, "SetScenario", "The Scenario data can not to be nullptr.");
    }
    //m_OutletFactory = sce->getBMPForOutletReach();
    //if (!m_OutletFactory.empty())
    //{
    //    //#pragma omp parallel for
    //    for (int i = 0; i < m_OutletFactory.size(); i++) {
    //        if (m_OutletFactory[i]->bmpId() == BMP_TYPE_WASCOB) {
    //            BMPWascobFactory* bmps = (BMPWascobFactory*)m_OutletFactory[i];
    //            wascob = bmps->GetOperations();
    //            for (int k = 0; k < m_nCells; k++)
    //            {
    //                for (auto b : wascob)
    //                {
    //                    for (auto wsc : b.second)
    //                    {
    //                        if (wsc->SubareaId == k)
    //                        {
    //                            wascobSubarea[k].push_back(wsc);
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //        if (m_OutletFactory[i]->bmpId() == BMP_TYPE_TILEDRAIN) {
    //            BMPTileDrainFactory* bmps = (BMPTileDrainFactory*)m_OutletFactory[i];
    //            tiledrain = bmps->GetOperations();
    //        }
    //    }
    //    for (int i = 0; i < m_OutletFactory.size(); i++)
    //    {
    //        if (m_OutletFactory[i]->bmpId() == BMP_TYPE_TILEDRAIN)
    //        {
    //            BMPTileDrainFactory* bmps = (BMPTileDrainFactory*)m_OutletFactory[i];
    //            soilPar = bmps->getclsTileDrain();
    //        }
    //    }
    //}
    //if (!m_subbasinIDs.empty())
    //{
    //    for (int i = 0; i < m_subbasinIDs.size(); i++)
    //    {
    //        int subId = m_subbasinIDs[i];
    //        for (auto b : tiledrain) {
    //            for (BMPTileDrain* tiledrain : b.second) {
    //                if (tiledrain->OutletReachId == subId)
    //                {
    //                    tiledrainReach[subId].push_back(tiledrain);
    //                }
    //            }
    //        }
    //    }
    //    for (auto b : tiledrain) {
    //        for (BMPTileDrain* tile : b.second) {
    //            tiledrainSubarea[tile->subareaId].push_back(tile);
    //            if (tiledrainSubarea[tile->subareaId].size() > 1)
    //            {
    //                throw ModelException("ReservoirMethodNEW", "SetScenario", "一个subarea只能对应一个tiledrain");
    //            }
    //        }
    //    }
    /*}*/
}

void ReservoirMethodNEW::Get1DData(const char* key, int* nrows, float** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_REVAP)) {
        *data = m_revap;
        *nrows = m_nCells;
    }
    else if (StringMatch(sk, VAR_RG)) {
        *data = m_T_RG;
        *nrows = m_nSubbsns + 1;
    }
    else if (StringMatch(sk, VAR_SBQG)) {
        *data = m_T_QG;
        *nrows = m_nSubbsns + 1;
    }
    else if (StringMatch(sk, VAR_SBGS)) {
        *data = m_gwSto;
        *nrows = m_nSubbsns + 1;
    }
    else if (StringMatch(sk, VAR_SBPET)) {
        *data = m_petSubbsn;
        *nrows = m_nSubbsns + 1;
    }
    else if (StringMatch(sk, VAR_GWSUBAREA)) {
        *data = gwSub;
        *nrows = m_nCells;
    }
	else if (StringMatch(sk, VAR_CHST)) {
		*data = m_chSto;
	}
    //else if (StringMatch(sk, VAR_SBQGSUBAREA)) {
    //    *data = QGSub;  
    //}
    else {
        throw ModelException(MID_GWA_RE, "Get1DData", "Parameter " + sk + " does not exist.");
    }
}

void ReservoirMethodNEW::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
    InitialOutputs();
    string sk(key);
    if (StringMatch(sk, VAR_GWWB)) {
        *data = m_T_GWWB;
        *nrows = m_nSubbsns + 1;
        *ncols = 6;
    }
    //else if (StringMatch(sk, VAR_PERCOWB)) {
    //    *data = m_soilPerco;
    //}
    else {
        throw ModelException(MID_GWA_RE, "Get2DData", "Parameter " + sk + " does not exist in current module.");
    }
}
