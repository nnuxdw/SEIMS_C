#include "wetland_SWAT.h"

#include "text.h"

WETLAND::WETLAND() :
    m_nCells(-1), m_cellArea(NODATA_VALUE), m_timestep(-1),
    m_nSoilLyrs(nullptr), m_maxSoilLyrs(-1), m_subbasin(nullptr), m_nSubbasins(-1),
    m_rteLyrs(nullptr), m_nRteLyrs(-1),m_flowOutIdxD8(nullptr),
    m_soilThick(nullptr), m_area(nullptr), m_flowInIdxD8(nullptr),
    m_soildepth(nullptr), m_pet(nullptr), m_surfaceRunoff(nullptr),
    m_surfqToCh(nullptr), m_IfluCbntoCH(nullptr), m_IfluInOrgnCbntoCH(nullptr),
    m_soilsediRPOC(nullptr), m_soilSurfCbn(nullptr), m_soilSurfInOrgnCbn(nullptr),m_soilsediLPOC(nullptr),
    m_latqToCh(nullptr),m_pcp(nullptr), m_soilPerco(nullptr),m_sublatDOC(nullptr),m_pNet(nullptr),
    m_rchID(nullptr),m_subSurfRfVol(nullptr),evwet(NODATA_VALUE),wetmxvol(NODATA_VALUE),wetnvol(NODATA_VALUE),
    wetk(NODATA_VALUE),wetlagtime(NODATA_VALUE),m_sublatDIC(nullptr),m_LPOCtoCH(nullptr),m_RPOCtoCH(nullptr),
    m_soilFrozenTemp(NODATA_VALUE),m_soilTemp(nullptr),
    wet_mxvol(nullptr),wet_nvol(nullptr),wet_nsa(nullptr),wet_mxsa(nullptr),bw1(nullptr),bw2(nullptr),
    m_wet_k(nullptr),m_subSurfRf(nullptr),m_wetland_wt(nullptr),m_subarea(nullptr), m_subbasinsInfo(nullptr),
    m_cellfr(nullptr), m_surfDOCtoCH(nullptr), m_surfDICtoCH(nullptr),m_wetland_oc(nullptr),
    ksr0(NODATA_VALUE),ksr1(NODATA_VALUE),ksr2(NODATA_VALUE),krem0(NODATA_VALUE),krem1(NODATA_VALUE),krem2(NODATA_VALUE),Cdoc(NODATA_VALUE),
    m_landCover(nullptr), m_WetVol(nullptr),m_wetdoccon(nullptr),
    m_WetDOC(nullptr),m_WetDIC(nullptr),m_WetRPOC(nullptr),m_WetLPOC(nullptr), m_wetarea(nullptr),
    m_mvsurfdoc(nullptr),m_mvsurfdic(nullptr),m_mvsurflpoc(nullptr),m_mvsurfrpoc(nullptr),
    m_SurfRfVol(nullptr),m_upstreamIfluCbntoCH(nullptr),m_upstreamIfluInOrgnCbntoCH(nullptr),
    m_soilPercoCbnLowest(nullptr),m_surf_leachdoc(nullptr),m_PercoCbn(nullptr),
    mean_lakedepth(nullptr),res_time(nullptr) {
}

WETLAND::~WETLAND() {
    if (m_WetVol != nullptr) Release1DArray(m_WetVol);
    if (wet_mxvol != nullptr) Release1DArray(wet_mxvol);
    if (wet_nvol != nullptr) Release1DArray(wet_nvol);
    if (wet_nsa != nullptr) Release1DArray(wet_nsa);
    if (wet_mxsa != nullptr) Release1DArray(wet_mxsa);
    if (bw1 != nullptr) Release1DArray(bw1);
    if (bw2 != nullptr) Release1DArray(bw2);
    if (m_wet_k != nullptr) Release1DArray(m_wet_k);
    if (m_subarea != nullptr) Release1DArray(m_subarea);
    if (m_wetarea != nullptr) Release1DArray(m_wetarea);
    if (m_cellfr != nullptr) Release1DArray(m_cellfr);
    if (m_wetland_wt != nullptr) Release2DArray(m_nSubbasins+1, m_wetland_wt);
    if (m_wetland_oc != nullptr) Release2DArray(m_nSubbasins+1, m_wetland_oc);
    if (m_WetDOC != nullptr) Release1DArray( m_WetDOC);
    if (m_WetDIC != nullptr) Release1DArray( m_WetDIC);
    if (m_WetRPOC != nullptr) Release1DArray( m_WetRPOC);
    if (m_WetLPOC != nullptr) Release1DArray( m_WetLPOC);
    if (m_upstreamIfluInOrgnCbntoCH != nullptr) Release1DArray(m_upstreamIfluInOrgnCbntoCH);
    if (m_upstreamIfluCbntoCH != nullptr) Release1DArray( m_upstreamIfluCbntoCH);
    if (m_SurfRfVol != nullptr) Release1DArray(m_SurfRfVol);
    if (m_mvsurfdoc != nullptr) Release1DArray(m_mvsurfdoc);
	if (m_mvsurfdic != nullptr) Release1DArray(m_mvsurfdic);
	if (m_mvsurflpoc != nullptr) Release1DArray(m_mvsurflpoc);
	if (m_mvsurfrpoc != nullptr) Release1DArray(m_mvsurfrpoc);
    if (m_wetdoccon != nullptr) Release1DArray(m_wetdoccon);
    if (m_surf_leachdoc != nullptr) Release1DArray(m_surf_leachdoc);
    if (m_PercoCbn != nullptr) Release1DArray(m_PercoCbn);
    if (m_IfluInOrgnCbntoCH != nullptr) Release1DArray(m_IfluInOrgnCbntoCH);
    // if (m_soilPercoCbnLowest != nullptr) Release1DArray(m_soilPercoCbnLowest);
}

bool WETLAND::CheckInputData() {
    CHECK_POSITIVE("WETLAND", m_nCells);
    CHECK_POSITIVE("WETLAND", m_maxSoilLyrs);
    CHECK_POSITIVE("WETLAND", m_nRteLyrs);

    CHECK_POINTER("WETLAND", m_landCover);
    CHECK_POINTER("WETLAND", m_subSurfRf);
    CHECK_POINTER("WETLAND", m_subSurfRfVol);
    CHECK_POINTER("WETLAND", m_sublatDOC);
    CHECK_POINTER("WETLAND", m_sublatDIC);
    CHECK_POINTER("WETLAND", m_RPOCtoCH);
    CHECK_POINTER("WETLAND", m_LPOCtoCH);
    CHECK_POINTER("WETLAND", m_surfDOCtoCH);
    CHECK_POINTER("WETLAND", m_surfDICtoCH);
    CHECK_POINTER("WETLAND", m_IfluCbntoCH);
    CHECK_POINTER("WETLAND", m_soilSurfCbn);

    CHECK_POINTER("WETLAND", m_rchID);
    CHECK_POINTER("WETLAND", m_flowOutIdxD8);
    CHECK_POINTER("WETLAND", m_pcp);
    CHECK_POINTER("WETLAND", m_pNet);
    CHECK_POINTER("WETLAND", m_soilPerco);
    CHECK_POSITIVE("WETLAND", evwet);
    CHECK_POSITIVE("WETLAND", wetk);
    CHECK_POSITIVE("WETLAND", wetlagtime);
	CHECK_POINTER("WETLAND", m_subbasinsInfo);
    CHECK_POINTER("WETLAND", m_flowInIdxD8);
    CHECK_POINTER("WETLAND", m_soilTemp);
    CHECK_NODATA("WETLAND", m_soilFrozenTemp);
    return true;
}

void WETLAND::SetValue(const char* key, const float value) {
    string sk(key);
    if (StringMatch(sk, Tag_TimeStep)) m_timestep = value;

    //ljj test for wetland
    else if (StringMatch(sk, "Wetmxvol")) wetmxvol = value;
    else if (StringMatch(sk, "Wetnvol")) wetnvol = value;
    else if (StringMatch(sk, "evwet")) evwet = value;
    else if (StringMatch(sk, "wetk")) wetk = value;
    else if (StringMatch(sk, "wetlag")) wetlagtime = value;
    else if (StringMatch(sk, VAR_T_SOIL)) m_soilFrozenTemp = value;
    else if (StringMatch(sk, "ksr0")) ksr0 = value;
    else if (StringMatch(sk, "ksr1")) ksr1 = value;
    else if (StringMatch(sk, "ksr2")) ksr2 = value;
    else if (StringMatch(sk, "krem0")) krem0 = value;
    else if (StringMatch(sk, "krem1")) krem1 = value;
    else if (StringMatch(sk, "krem2")) krem2 = value;
    else if (StringMatch(sk, "Cdoc")) Cdoc = value;
    else {
        throw ModelException("WETLAND", "SetValue", "Parameter " + sk + " does not exist.");
    }
}

void WETLAND::Set1DData(const char* key, const int n, float* data) {
    string sk(key);

    if (StringMatch(sk, VAR_SBOF)) {
        m_surfqToCh = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_SBIF)) {
        m_latqToCh = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_latRDOCtoCH)) {
        m_sublatDOC = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_latDICtoCH)) {
        m_sublatDIC = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_LPOCtoCH)) {
        m_LPOCtoCH = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_RPOCtoCH)) {
        m_RPOCtoCH = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_surfRDOCtoCH)) {
        m_surfDOCtoCH = data;
        m_nSubbasins = n - 1;
        return;
    }
    else if (StringMatch(sk, VAR_surfDICtoCH)){
        m_surfDICtoCH = data;
        m_nSubbasins = n - 1;
        return;
    }
    CheckInputSize("WETLAND", key, n, m_nCells);

    if (StringMatch(sk, VAR_SOTE)) m_soilTemp = data;
    else if (StringMatch(sk, VAR_SOILLAYERS)) m_nSoilLyrs = data;
    else if (StringMatch(sk, VAR_SUBBSN)) m_subbasin = data;
    else if (StringMatch(sk, VAR_PET)) m_pet = data;
    else if (StringMatch(sk, VAR_OLFLOW)) m_surfaceRunoff = data;
	else if (StringMatch(sk, "CELLAREA")) m_area = data;
    else if (StringMatch(sk, VAR_PERC_LOWEST_DOC)) m_soilPercoCbnLowest = data;
    else if (StringMatch(sk, VAR_LANDUSE)) m_landCover = data;
    else if (StringMatch(sk, VAR_STREAM_LINK)) m_rchID = data;
    else if (StringMatch(sk, VAR_NEPR)) m_pNet = data;
    else if (StringMatch(sk, VAR_PCP)) m_pcp = data;
    else if (StringMatch(sk, VAR_LATERAL_C)) m_IfluCbntoCH = data;
    //else if (StringMatch(sk, VAR_LATERAL_IC)) m_IfluInOrgnCbntoCH = data;
	else if (StringMatch(sk, VAR_SURF_DOC)) m_soilSurfCbn = data;
    else if (StringMatch(sk, VAR_SURF_DIC)) m_soilSurfInOrgnCbn = data;
	else if (StringMatch(sk, VAR_ENR_LPOC)) m_soilsediLPOC = data;
	else if (StringMatch(sk, VAR_ENR_RPOC)) m_soilsediRPOC = data;
    else if (StringMatch(sk, Tag_FLOWOUT_INDEX_D8)) m_flowOutIdxD8 = data;
    else if (StringMatch(sk, "hs_lakedepth")) mean_lakedepth = data;
    else if (StringMatch(sk, "res_time")) res_time = data;
    else {
        throw ModelException("WETLAND", "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void WETLAND::Set2DData(const char* key, const int n, const int col, float** data) {
    string sk(key);
    if (StringMatch(sk, Tag_ROUTING_LAYERS)) {
        CheckInputSize("WETLAND", key, n, m_nRteLyrs);
        m_nRteLyrs = n;
        m_rteLyrs = data;
        return;
    }
    if (StringMatch(sk, Tag_FLOWIN_INDEX_D8)) {
        CheckInputSize("WETLAND", key, n, m_nCells);
        m_flowInIdxD8 = data;
        return;
    }
    CheckInputSize2D("WETLAND", key, n, col, m_nCells, m_maxSoilLyrs);
    if (StringMatch(sk, VAR_CONDUCT)) m_ks = data;
    else if (StringMatch(sk, VAR_SOILTHICK)) m_soilThick = data;
    else if (StringMatch(sk, VAR_SOILDEPTH)) m_soildepth = data;

    else if (StringMatch(sk, VAR_SSRU)) m_subSurfRf = data;
    else if (StringMatch(sk, VAR_SSRUVOL)) m_subSurfRfVol = data;
    else if (StringMatch(sk, VAR_PERCO)) m_soilPerco = data;
    else {
        throw ModelException("WETLAND", "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void WETLAND::InitialOutputs() {
    CHECK_POSITIVE("WETLAND", m_nCells);

    if (m_WetDOC == nullptr) {
        Initialize1DArray(m_nCells, m_WetDOC, 0.f);
        for (int i = 0; i < m_nCells; i++) {

            m_WetDOC[i] = Cdoc*m_area[i] * wetnvol/1000;
        }
    }
    if (m_WetDIC == nullptr) Initialize1DArray(m_nCells, m_WetDIC, 0.f);
    if (m_WetRPOC == nullptr) Initialize1DArray(m_nCells, m_WetRPOC, 0.f);
    if (m_WetLPOC == nullptr) Initialize1DArray(m_nCells, m_WetLPOC, 0.f);

    if (m_SurfRfVol == nullptr) Initialize1DArray(m_nCells, m_SurfRfVol, 0.f);
    if (m_mvsurfdoc == nullptr)	Initialize1DArray(m_nCells, m_mvsurfdoc, 0.f);
	if (m_mvsurfdic == nullptr)	Initialize1DArray(m_nCells, m_mvsurfdic, 0.f);
	if (m_mvsurflpoc == nullptr) Initialize1DArray(m_nCells, m_mvsurflpoc, 0.f);
	if (m_mvsurfrpoc == nullptr) Initialize1DArray(m_nCells, m_mvsurfrpoc, 0.f);
    if (m_surf_leachdoc == nullptr) Initialize1DArray(m_nCells, m_surf_leachdoc, 0.f);
    if (m_IfluInOrgnCbntoCH == nullptr) Initialize1DArray( m_nCells, m_IfluInOrgnCbntoCH, 0.f);


    //if (m_soilPercoCbnLowest == nullptr) Initialize1DArray( m_nSubbasins + 1, m_soilPercoCbnLowest, 0.f);


    if (wet_nvol == nullptr) {
        Initialize1DArray(m_nCells, wet_nvol, 0.f);
        Initialize1DArray(m_nCells, wet_mxvol, 0.f);
        Initialize1DArray(m_nCells, wet_mxsa, 0.f);
        Initialize1DArray(m_nCells, wet_nsa, 0.f);
        for (int ilyr = 0; ilyr < m_nRteLyrs; ilyr++) {
        // There are not any flow relationship within each routing layer.
        // So parallelization can be done here.
            int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
                for (int icell = 1; icell <= ncells; icell++) {
                    int id = CVT_INT(m_rteLyrs[ilyr][icell]);

                    wet_nvol[id]  =  m_area[id]  * mean_lakedepth[id] * 0.5;
                    wet_mxvol[id] =  m_area[id]  * mean_lakedepth[id] * 2.0;
                    if (wet_mxvol[id] <= 0. || wet_mxvol[id] <= wet_nvol[id]) wet_mxvol[id] = 1.11 * wet_nvol[id];
                    if (wet_nvol[id] <= 0. || wet_mxvol[id] <= wet_nvol[id])  wet_nvol[id] = .9 * wet_mxvol[id];
                    wet_nsa[id] = .08 * wet_nvol[id];
                    wet_mxsa[id] = 1.5 * wet_nsa[id];
                    if(mean_lakedepth[id]<0) {
                        wet_nvol[id] = 0.f;
                        wet_mxvol[id] = 0.f;
                        wet_nsa[id] = 0.f;
                        wet_mxsa[id] = 0.f;
                    }
                    // wet_mxvol[id] = 10000. * wet_mxvol[id];
                    // wet_nvol[id] = 10000. * wet_nvol[id];
                    //cout<<m_nRteLyrs<<"   "<<wetnvol * (ilyr+1)<<endl;
                }
        }
    }
    if (m_WetVol == nullptr) {
        Initialize1DArray(m_nCells, m_WetVol, 0.f);
        for (int i = 0; i < m_nCells; i++) {
            if (m_WetVol[i] <= 0.) m_WetVol[i] = m_area[i] * mean_lakedepth[i] * 0.5;
            //m_WetVol[i] = 10000. * m_WetVol[i]* (m_area[i]* 1.e-4f);
        }
    }
    // if (wet_nsa == nullptr) Initialize1DArray(m_nCells, wet_nsa, 0.f);
    // if (wet_mxsa == nullptr) Initialize1DArray(m_nCells, wet_mxsa, 0.f);
    if (bw1 == nullptr) Initialize1DArray(m_nCells, bw1, 0.f);
    if (bw2 == nullptr) Initialize1DArray(m_nCells, bw2, 0.f);
    if (m_wet_k == nullptr) Initialize1DArray(m_nCells, m_wet_k, 0.f);
    if (m_wetdoccon == nullptr) Initialize1DArray(m_nCells, m_wetdoccon, 0.f);
    if (m_PercoCbn == nullptr) Initialize1DArray(m_nCells, m_PercoCbn, 0.f);

    int nLen = m_nSubbasins + 1;
	if (m_wetland_wt == nullptr) Initialize2DArray(nLen, 6, m_wetland_wt, 0.f);
    if (m_wetland_oc == nullptr) Initialize2DArray(nLen, 6, m_wetland_oc, 0.f);
    if (m_subarea == nullptr) {
		Initialize1DArray(nLen, m_subarea, 0.f);
        int index = 0;
		for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
			int subID = *it;
			Subbasin* sub = m_subbasinsInfo->GetSubbasinByID(*it);
        	int nCells = sub->GetCellCount();
            int* curCells = sub->GetCells();
       		for (int i = 0; i < nCells; i++) {
                index = curCells[i];
				m_subarea[subID] += m_area[index];
			}
		}
	}
    if (m_wetarea == nullptr) {
		Initialize1DArray(nLen, m_wetarea, 0.f);
        int index = 0;
		for (auto it = m_subbasinIDs.begin(); it != m_subbasinIDs.end(); ++it) {
			int subID = *it;
			Subbasin* sub = m_subbasinsInfo->GetSubbasinByID(*it);
        	int nCells = sub->GetCellCount();
            int* curCells = sub->GetCells();
       		for (int i = 0; i < nCells; i++) {
                index = curCells[i];
                //cout<<m_landCover[index]<<"   "<<index<<"   "<<m_ks[index][0]<<endl;
                //if (m_landCover[index] == 9 ||m_landCover[index] == 10||m_landCover[index] == 11)
                if (m_landCover[index] ==LANDUSE_ID_WATR && m_rchID[index]<=0.f){
				    m_wetarea[subID] += m_area[i];
				}
            }
		}
	}
    if (m_cellfr == nullptr) {
        Initialize1DArray(m_nCells, m_cellfr, 0.f);
		for (int i = 0; i < m_nCells; i++) {
			m_cellfr[i] = m_area[i] / m_subarea[CVT_INT(m_subbasin[i])]; //ljj,m_area is 1Ddata
		}
    }


    if (m_upstreamIfluCbntoCH == nullptr) Initialize1DArray(m_nCells, m_upstreamIfluCbntoCH, 0.f);
    if (m_upstreamIfluInOrgnCbntoCH == nullptr) Initialize1DArray(m_nCells, m_upstreamIfluInOrgnCbntoCH, 0.f);
}

void WETLAND::SetSubbasins(clsSubbasins* subbasins) {
	if (nullptr == m_subbasinsInfo) {
		m_subbasinsInfo = subbasins;
		// m_nSubbasins = m_subbasinsInfo->GetSubbasinNumber(); // Set in SetValue()
		m_subbasinIDs = m_subbasinsInfo->GetSubbasinIDs();
	}
}

int WETLAND::Execute() {

    CheckInputData();
    InitialOutputs();
    for (int i = 0; i < m_nCells; i++) {
        m_SurfRfVol[i] = 0.f;
		m_mvsurfdoc[i] = 0.f;
		m_mvsurfdic[i] = 0.f;
		m_mvsurflpoc[i] = 0.f;
		m_mvsurfrpoc[i] = 0.f;

    }
    for (int i = 0; i <= 5; i++) {
        m_wetland_wt[0][i] = 0.f;
        m_wetland_oc[0][i] = 0.f;
    }

    for (int ilyr = 0; ilyr < m_nRteLyrs; ilyr++) {
        // There are not any flow relationship within each routing layer.
        // So parallelization can be done here.
        int ncells = CVT_INT(m_rteLyrs[ilyr][0]);
        for (int icell = 1; icell <= ncells; icell++) {
            int id = CVT_INT(m_rteLyrs[ilyr][icell]);
            if(res_time[id]<0) continue;
            int nUpstream = CVT_INT(m_flowInIdxD8[id][0]);
            int id_downstream = CVT_INT(m_flowOutIdxD8[id]);
            //if this field is wetland, intercept the upstream water
            m_SurfRfVol[id] = m_surfaceRunoff[id]* 0.001f * m_area[id] ; //m3
            //if (m_landCover[id] == 9 ||m_landCover[id] == 10||m_landCover[id] == 11)
            if (m_landCover[id] ==LANDUSE_ID_WATR && m_rchID[id]<=0.f)
            {
                m_IfluCbntoCH[id] = 0.f;
                m_IfluInOrgnCbntoCH[id] = 0.f;
                for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
                    m_subSurfRf[id][j] = 0.f;
                    m_subSurfRfVol[id][j] = 0.f;
                }
            }
            for (int upIndex = 1; upIndex <= nUpstream; upIndex++) {
                // IMPORTANT!!! If the upstream cell is from another subbasin, CONTINUE to next upstream cell. By lj.
                int flowInID = CVT_INT(m_flowInIdxD8[id][upIndex]);
                if (CVT_INT(m_subbasin[flowInID]) != CVT_INT(m_subbasin[id])) { continue; }
                float init_surfRf = m_surfaceRunoff[id];
                float qUp = 0.f;
                float qDown =0.f;

                if (m_SurfRfVol[flowInID] > 0.f) {
                    qUp = m_SurfRfVol[flowInID];
                }
                if (qUp <= 0.f) qUp = 0.f;
                m_SurfRfVol[id] += qUp;

                //if (m_landCover[id] == 9 ||m_landCover[id] == 10||m_landCover[id] == 11)
                if (m_landCover[id] ==LANDUSE_ID_WATR && m_rchID[id]<=0.f) {
                for (int j = 0; j < CVT_INT(m_nSoilLyrs[id]); j++) {
                    if (m_subSurfRf[flowInID][j] > 0.f) {
                        m_subSurfRf[id][j] += m_subSurfRf[flowInID][j];
                    }
                    m_subSurfRfVol[id][j] += m_subSurfRf[flowInID][j] * 0.001f * m_area[flowInID];
                    m_subSurfRfVol[id][j] = Max(UTIL_ZERO, m_subSurfRfVol[id][j]);
                }
                    m_IfluCbntoCH[id] += m_IfluCbntoCH[flowInID]*m_area[flowInID]*0.0001;
                    m_IfluInOrgnCbntoCH[id] += m_IfluInOrgnCbntoCH[flowInID]*m_area[flowInID]*0.0001;
                }
            }
            m_mvsurfdoc[id] += m_soilSurfCbn[id] * m_area[id] * 0.0001f;  //m_soilSurfCbn是没有汇流之前，每个地块独立生成的DOC
	        m_mvsurfdic[id] += m_soilSurfInOrgnCbn[id] * m_area[id] * 0.0001f;
	        m_mvsurflpoc[id] += m_soilsediLPOC[id] * m_area[id] * 0.0001f;
	        m_mvsurfrpoc[id] += m_soilsediRPOC[id] * m_area[id] * 0.0001f;
            //if (m_landCover[id] == 9 ||m_landCover[id] == 10||m_landCover[id] == 11)
            if (m_landCover[id] ==LANDUSE_ID_WATR && m_rchID[id]<=0.f)
            {
                m_SurfRfVol[id] -= m_surfaceRunoff[id]* 0.001f * m_area[id] ; //m3, remove itself
                m_mvsurfdoc[id] -= m_soilSurfCbn[id] * m_area[id] * 0.0001f;
                WetlandSimulate(id);
                //cout<<id<<"    "<<m_SurfRfVol[id]<<"   "<<m_surfaceRunoff[id]<<endl;
            }
            //水量采用加上游，碳采用传递到下游，但是注意这里是做的分层运算，都是从上游到下游逐级计算
            if (id_downstream >= 0) {
			    m_mvsurfdoc[id_downstream] = m_mvsurfdoc[id] + m_soilSurfCbn[id_downstream]* 0.0001f * m_area[id_downstream];
			    m_mvsurfdic[id_downstream] = m_mvsurfdic[id] + m_soilSurfInOrgnCbn[id_downstream]* 0.0001f * m_area[id_downstream];
			    m_mvsurflpoc[id_downstream] = m_mvsurflpoc[id] + m_soilsediLPOC[id_downstream]* 0.0001f * m_area[id_downstream];
			    m_mvsurfrpoc[id_downstream] = m_mvsurfrpoc[id] + m_soilsediRPOC[id_downstream]* 0.0001f * m_area[id_downstream];
		    }

        }
    }

    /// reCalculate the surface runoff, sediment, nutrient etc. that into the channel
//#pragma omp parallel for
    for (int i = 0; i <= m_nSubbasins; i++) {
        m_surfqToCh[i] = 0.f;
        m_latqToCh[i] = 0.f;
        m_sublatDOC[i] = 0.f;
        m_sublatDIC[i] = 0.f;
        m_LPOCtoCH[i] = 0.f;
        m_RPOCtoCH[i] = 0.f;
        m_surfDICtoCH[i] = 0.f;
        m_surfDOCtoCH[i] = 0.f;
        //m_soilPercoCbnLowest[i] = 0.f;
    }
    // See https://github.com/lreis2415/SEIMS/issues/36 for more descriptions. By lj
//#pragma omp parallel
    {
        float* tmp_surfq2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_qiSubbsn = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_latc2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_latic2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_rpoc2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_lpoc2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_doc2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_dic2ch = new(nothrow) float[m_nSubbasins + 1];
        float* tmp_percodoc = new(nothrow) float[m_nSubbasins + 1];

        for (int i = 0; i <= m_nSubbasins; i++) {
            tmp_surfq2ch[i] = 0.f;
            tmp_qiSubbsn[i] = 0.f;
            tmp_latc2ch[i] = 0.f;
            tmp_latic2ch[i] = 0.f;
            tmp_rpoc2ch[i] = 0.f;
            tmp_lpoc2ch[i] = 0.f;
            tmp_doc2ch[i] = 0.f;
            tmp_dic2ch[i] = 0.f;
            tmp_percodoc[i] = 0.f;
        }
//#pragma omp for
        for (int i = 0; i < m_nCells; i++) {
            int subi = CVT_INT(m_subbasin[i]);
            m_wetdoccon[i] = m_mvsurfdoc[i];
            if (m_rchID[i] <= 0.f) continue;
            float qiAllLayers = 0.f;
            float qsSurf = 0.f;
            for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
                if ( m_subSurfRfVol[i][j] > UTIL_ZERO) {
                    qiAllLayers += m_subSurfRfVol[i][j]  / m_timestep; /// m^3/s
                }
            }
            if ( m_SurfRfVol[i] > UTIL_ZERO) {
                if (m_rchID[i] <= 0.f) continue;
                qsSurf += m_SurfRfVol[i]  / m_timestep; /// m^3/s
            }
            if (m_rchID[i] > 0) {
                tmp_qiSubbsn[CVT_INT(m_rchID[i])] += qiAllLayers;
                tmp_surfq2ch[CVT_INT(m_rchID[i])] += qsSurf;

			    tmp_latc2ch[CVT_INT(m_rchID[i])] += m_IfluCbntoCH[i] * m_area[i] * 0.0001f; //after transport
                tmp_latic2ch[CVT_INT(m_rchID[i])] += m_IfluInOrgnCbntoCH[i] * m_area[i] * 0.0001f; //after transport

                tmp_lpoc2ch[CVT_INT(m_rchID[i])] += m_mvsurflpoc[i];
				tmp_rpoc2ch[CVT_INT(m_rchID[i])] += m_mvsurfrpoc[i];
				tmp_dic2ch[CVT_INT(m_rchID[i])] += m_mvsurfdic[i] ;  //only the surface DIC
				tmp_doc2ch[CVT_INT(m_rchID[i])] += m_mvsurfdoc[i];
                //cout<<tmp_surfq2ch[CVT_INT(m_rchID[i])]* m_timestep<<",    "<<tmp_doc2ch[CVT_INT(m_rchID[i])] <<endl;
            }
        }
//#pragma omp for
        for (int i = 0; i < m_nCells; i++) {
            tmp_percodoc[CVT_INT(m_subbasin[i])] += m_PercoCbn[i]* m_area[i] * 0.0001f;
        }
//#pragma omp critical
        {
            for (int i = 1; i <= m_nSubbasins; i++) {
				m_surfqToCh[i] += tmp_surfq2ch[i] ;
                m_latqToCh[i] += tmp_qiSubbsn[i] ;
                m_surfDOCtoCH[i] += tmp_doc2ch[i];
                m_surfDICtoCH[i] += tmp_dic2ch[i];
                m_sublatDOC[i] += tmp_latc2ch[i];
                m_sublatDIC[i] += tmp_latic2ch[i];
                m_LPOCtoCH[i] += tmp_lpoc2ch[i];
                m_RPOCtoCH[i] += tmp_rpoc2ch[i];
                m_RPOCtoCH[i] += tmp_rpoc2ch[i];
                // m_soilPercoCbnLowest[i] += tmp_percodoc[i];
            }
        }
        delete[] tmp_surfq2ch;
        delete[] tmp_qiSubbsn;
        delete[] tmp_latc2ch;
        delete[] tmp_latic2ch;
        delete[] tmp_lpoc2ch;
        delete[] tmp_rpoc2ch;
        delete[] tmp_dic2ch;
        delete[] tmp_doc2ch;
        delete[] tmp_percodoc;
        tmp_surfq2ch = nullptr;
        tmp_qiSubbsn = nullptr;
        tmp_latc2ch = nullptr;
        tmp_latic2ch = nullptr;
        tmp_lpoc2ch = nullptr;
        tmp_rpoc2ch = nullptr;
        tmp_dic2ch = nullptr;
        tmp_doc2ch = nullptr;
        tmp_percodoc = nullptr;
    }

    for (int i = 1; i <= m_nSubbasins; i++) {
        m_surfqToCh[0] += m_surfqToCh[i];
        m_latqToCh[0] += m_latqToCh[i];
        m_sublatDOC[0] += m_sublatDOC[i];   //units: kg
        m_sublatDIC[0] += m_sublatDIC[i];   //units: kg
        m_LPOCtoCH[0] += m_LPOCtoCH[i];   //units: kg
        m_RPOCtoCH[0] += m_RPOCtoCH[i];   //units: kg
        m_surfDOCtoCH[0] += m_surfDOCtoCH[i];   //units: kg
        m_surfDICtoCH[0] += m_surfDICtoCH[i];   //units: kg

        // m_soilPercoCbnLowest[i] /=(m_subarea[i]* 0.0001f);//units: kg/ha
		// m_soilPercoCbnLowest[0] += m_soilPercoCbnLowest[i]; //units: kg/ha
    }
    return 0;
}

void WETLAND::WetlandSimulate(const int id) {
    //input
    // wet_nvol[id]  = m_area[id] * 0.0001 * wetnvol; //m --> 10^4 m^3
    // wet_mxvol[id] = m_area[id] * 0.0001 * wetmxvol;
    // if (wet_mxvol[id] <= 0. || wet_mxvol[id] <= wet_nvol[id]) wet_mxvol[id] = 1.11 * wet_nvol[id];
    // if (wet_nvol[id] <= 0. || wet_mxvol[id] <= wet_nvol[id])  wet_nvol[id] = .9 * wet_mxvol[id];
    // wet_nsa[id] = .08 * wet_nvol[id];
    // wet_mxsa[id] = 1.5 * wet_nsa[id];
    // wet_mxvol[id] = 10000. * wet_mxvol[id];
    // wet_nvol[id] = 10000. * wet_nvol[id];
    bw1[id] = 0.f;
    bw2[id] = 0.f;
    m_wet_k[id] = wetk; //calibration?

    float latq  = 0.f;

    /// initialize temporary variables
    float cnv = 0.f;
    cnv = m_area[id]* 1.e-3f; //m3 to mm

    for (int k = 0; k < CVT_INT(m_nSoilLyrs[id]); k++) {
        latq += m_subSurfRfVol[id][k];  //m3
    }

    //!! calculate shape parameters for surface area equation
    // float wetdif = 0.;
    // wetdif = wet_mxvol[id] - wet_nvol[id];
    // if ((wet_mxsa[id] - wet_nsa[id]) > 0.f && wetdif > 0.f) {
    //     float lnvol = 0.;
    //     lnvol = log10f(wet_mxvol[id]) - log10f(wet_nvol[id]);
    //     if (lnvol > 1.e-4) {
    //         bw2[id] = (log10f(wet_mxsa[id]) - log10f(wet_nsa[id])) / lnvol;
    //     }else{
    //         bw2[id] = (log10f(wet_mxsa[id]) - log10f(wet_nsa[id])) / 0.001;
    //     }
    //     if (bw2[id] > 0.9) bw2[id] = .9;
    //     bw1[id] = pow((wet_mxsa[id] / wet_mxvol[id]), bw2[id]);
    // }else{
    //     bw2[id] = .9;
    //     bw1[id] = pow((wet_nsa[id] / wet_nvol[id]), .9);
    // }


    //!! calculate water balance for day
    //float wetsa = 0.;
    //wetsa = bw1[id] * pow( m_WetVol[id], bw2[id]);
    //wetsa = m_area[id]* 1.e-4f;  //ha
    //float wetev = 10. * evwet * m_pet[id] * wetsa;
    float wetev = evwet * m_pet[id]*0.001f * m_area[id];
    float wetsep = m_wet_k[id]/1000.f * m_area[id];
    if ((wetev + wetsep) >= (m_WetVol[id]-wet_nvol[id])) {
        float rto = wetev/ (wetev + wetsep);
        wetev = rto * (m_WetVol[id]-wet_nvol[id]);
        wetsep = (1-rto)*  (m_WetVol[id]-wet_nvol[id]);
    }
    float wetpcp = m_pNet[id]*0.001f * m_area[id];
    if(m_soilTemp[id] < m_soilFrozenTemp) wetsep=0.f;  //ljj add
    //if(m_WetVol[id] <= wet_nvol[id])  wetsep=0.f;
    //!! calculate water flowing into wetland from HRU
    float wet_fr = 1.f;
    m_surfaceRunoff[id] = m_SurfRfVol[id] / (0.001f * m_area[id] );
    float wetflwi = m_SurfRfVol[id] + latq;   //change mm to m3

    float qdayi = m_surfaceRunoff[id];
    float latqi = latq;
    float qdayTmp = m_surfaceRunoff[id] * (1. - wet_fr);
    float lqdayTmp = latq * (1. - wet_fr);

    int routing =1;  //ljj switch
    if (routing > 0){
        float wetloss = qdayi - qdayTmp;
        float lwetloss = latqi - lqdayTmp;
        m_surfaceRunoff[id] -= wetloss;
        for (int k = 0; k < CVT_INT(m_nSoilLyrs[id]); k++) {
            //m_subSurfRf[id][k] -= lwetloss;
            m_subSurfRf[id][k] = 0.f; //lwetloss is 1D, cannot miues for 7 times
            m_subSurfRf[id][k] = Max(0.f, m_subSurfRf[id][k]);
            m_soilPerco[id][CVT_INT(m_nSoilLyrs[id]) - 1] = 0.f;
            m_subSurfRfVol[id][k] = m_subSurfRf[id][k] * 0.001f * m_area[id]; //m3
            m_subSurfRfVol[id][k] = Max(0.f, m_subSurfRfVol[id][k]);
        }
        // !--add carbon amount from HRU to wetland (kg)
        m_WetDOC[id]  += (m_mvsurfdoc[id] + m_IfluCbntoCH[id])* wet_fr;
        m_WetLPOC[id] += m_mvsurflpoc[id] * wet_fr;
        m_WetRPOC[id] += m_mvsurfrpoc[id] * wet_fr;
        m_WetDIC[id]  += (m_mvsurfdic[id] + m_IfluInOrgnCbntoCH[id]) * wet_fr;
        //if(id==6) {v
            m_wetland_oc[0][0] +=m_mvsurfdoc[id] + m_IfluCbntoCH[id];   //input from upstream
            m_wetland_wt[0][0] +=wetflwi;   //input from upstream
        //}
    } else{
        if(m_WetVol[id] <= wet_nvol[id])  wetsep=0.f;
    }
    //ljj++ K-DOC
    float DOCproduction =0.f;

    if(m_WetVol[id]>0.f){
        float fst_pd = 0.f;
        float Sdep = (m_WetVol[id])/m_area[id]*1000.0; //mm
        if (routing > 0){
            Sdep = (m_WetVol[id]+ (wetpcp + wetflwi - wetev))/m_area[id]*1000.0; //mm
            fst_pd = (wetpcp + wetflwi - wetev)/m_area[id]*1000.0 * (Cdoc-m_WetDOC[id] / m_WetVol[id] *1000.f );
        } else
        {
            Sdep = (m_WetVol[id]+ (wetpcp - wetev))/m_area[id]*1000.0; //mm
            fst_pd = (wetpcp  - wetev)/m_area[id]*1000.0 * (Cdoc-m_WetDOC[id] / m_WetVol[id] *1000.f);
        }
        if(m_WetVol[id]+ (wetpcp + wetflwi - wetev) < wet_mxvol[id]) {
            fst_pd = 0.f;
        }
        if(fst_pd < UTIL_ZERO) fst_pd = 0.f;

        float ksr = ksr0*1e-10 * exp(m_soilTemp[id]*ksr1)*pow(Sdep,ksr2);  //!!note:unit is different in vol and mass
        float krem = krem0*1e-10 *exp(m_soilTemp[id]*krem1)*pow(Sdep,krem2); //1e-10 is similar to K-DOC
        // float fstrel = fst_pd / ( m_WetVol[id]/m_area[id]*1000.0) * m_WetVol[id] *1000;
        // float release = ksr*Sdep/ ( m_WetVol[id]/m_area[id]*1000.0) * m_WetVol[id] *1000;
        //float removal = krem*(m_WetDOC[id] / m_WetVol[id] /1000.f)*Sdep/ ( m_WetVol[id]/m_area[id]*1000.0) * m_WetVol[id] *1000;
        float fstrel = fst_pd * m_area[id] *10e-6;    //mg/L to kg
        float release = ksr*Sdep* m_area[id] *10e-6;  //mg/L to kg
        float removal = krem*Sdep*(m_WetDOC[id] / m_WetVol[id] *1000.f)* m_area[id] *10e-6;  //mg/L to kg
        //float removal = krem*Sdep*((m_WetDOC[id]+fstrel+release) / (m_WetVol[id]+ (wetpcp + wetflwi - wetev)) *1000.f)* m_area[id] *10e-6;  //mg/L to kg


        fstrel = 0.f;
        //if(id==0) cout<<krem<<",   "<<ksr<<endl;
        removal = Max(removal,0.f);
        removal = Min(removal,m_WetDOC[id]+release+fstrel);
        DOCproduction = release - removal + fstrel;
        //if(fstrel>100) cout<<id<<",   "<< (wetpcp + wetflwi- wetsep - wetev)/m_area[id]*1000.0<<",   "<<fst_pd<<",   "<<fstrel<<",   "<<m_WetDOC[id] / m_WetVol[id] *1000.f<<endl;
        //if(id==0) {
            m_wetland_oc[0][1] +=DOCproduction;//kg, production
            m_wetland_oc[0][2] +=release;
        //}
    }

    m_WetDOC[id] += DOCproduction;
    // if(m_soilTemp[id] > 0) {
    //     m_WetDOC[id] += DOCproduction;
    // }else{
    //     DOCproduction = 0.f;
    // }
    m_WetDOC[id] = Max(m_WetDOC[id], 0.f);
    //ljj--
    //!-remove carbon amoount entering wetlands from HRU loadings (kg)
    if (routing > 0){
        m_mvsurfdoc[id] = m_mvsurfdoc[id] * (1. - wet_fr);
        m_soilSurfInOrgnCbn[id] = m_mvsurfdic[id] * (1. - wet_fr);
        m_IfluCbntoCH[id] = m_IfluCbntoCH[id] * (1. - wet_fr);
        m_IfluCbntoCH[id] = Max(m_IfluCbntoCH[id], 0.f);
        m_IfluInOrgnCbntoCH[id] = m_IfluInOrgnCbntoCH[id] * (1. - wet_fr);
        m_IfluInOrgnCbntoCH[id] = Max(m_IfluInOrgnCbntoCH[id], 0.f);
        //!! new water volume for day
        m_WetVol[id]= m_WetVol[id] - wetsep - wetev + wetpcp + wetflwi;
    }
    else{
        m_WetVol[id]= m_WetVol[id] - wetsep - wetev + wetpcp;  //no upstream water into wetland
    }

    m_WetVol[id] = Max(m_WetVol[id],0.f);
    if (m_WetVol[id] < 0.001) {
        wetsep = wetsep + m_WetVol[id];
        m_WetVol[id] = 0.;
        if (wetsep < 0.) {
            wetev = wetev + wetsep;
            wetsep = 0.;
        }
        m_WetDOC[id] = 0;
        m_WetDIC[id] = 0;
        m_WetRPOC[id] = 0;
        m_WetLPOC[id] = 0;
    }
    if (wetsep < 0.) {
        wetev = wetev + wetsep;
        wetsep = 0.;
    }
    //!Caculate initial carbon concentration in the wetland
    float rdoc_con=0.;
    float dic_con=0.;
    float rpoc_con=0.;
    float lpoc_con=0.;
    rdoc_con = m_WetDOC[id] /(m_WetVol[id]+wetsep)*1000.0;
    dic_con = m_WetDIC[id] /(m_WetVol[id]+wetsep)*1000.0;
    rpoc_con = m_WetRPOC[id] /(m_WetVol[id]+wetsep)*1000.0;
    lpoc_con = m_WetLPOC[id] /(m_WetVol[id]+wetsep)*1000.0;

    if(m_WetVol[id]<=1.f) {
        rdoc_con = 0;
        dic_con = 0;
        rpoc_con = 0;
        lpoc_con = 0;
    }

    //!! compute outflow if wetland water volume > 0
    float wetflwo;

    if (m_WetVol[id] <= wet_nvol[id]) {
        wetflwo = 0.;
    }else{
        if (m_WetVol[id] <= wet_mxvol[id]) {
            wetflwo = (m_WetVol[id]- wet_nvol[id]) / res_time[id];
            wetflwo = Min(wetflwo,m_WetVol[id]);
            m_WetVol[id] = m_WetVol[id] - wetflwo;
        }else{
            wetflwo = m_WetVol[id] - wet_mxvol[id];
            m_WetVol[id] = wet_mxvol[id];
        }
    }
    m_surfaceRunoff[id] = m_surfaceRunoff[id] + wetflwo / cnv;
    if (m_WetVol[id] < 1.e-6) m_WetVol[id] = 0.;
    if (m_surfaceRunoff[id] < 1.e-6) m_surfaceRunoff[id] = 0.;
    m_SurfRfVol[id] = m_surfaceRunoff[id] * cnv;  //mm to m3
    //cout<<id<<"   "<<m_WetVol[id]<<"   "<<m_surfaceRunoff[id]<<endl;
    float twlwet = wetsep / cnv;
    m_soilPerco[id][CVT_INT(m_nSoilLyrs[id]) - 1] = twlwet;  //replace rather than accumulate

    // //!calculating carbon amount leaving wetland (kg)
    m_mvsurfdoc[id] = m_mvsurfdoc[id] + rdoc_con * wetflwo/1000.0;
    m_mvsurflpoc[id] = m_mvsurflpoc[id] + lpoc_con * wetflwo/1000.0;
    m_mvsurfrpoc[id] = m_mvsurfrpoc[id] + rpoc_con * wetflwo/1000.0;
    m_mvsurfdic[id] = m_mvsurfdic[id] + dic_con * wetflwo/1000.0;
    m_PercoCbn[id]  = rdoc_con * wetsep /1000.0 / (0.0001f * m_area[id]);  //kg/ha
    m_soilPercoCbnLowest[id] = m_PercoCbn[id];

    if(wetflwo <= 0.f) {
        m_mvsurfdoc[id] = 0.f;
        m_mvsurflpoc[id] = 0.f;
        m_mvsurfrpoc[id] = 0.f;
        m_mvsurfdic[id] = 0.f;
    }
    // //!update carbon amount (kg) in wetland
    //cout<<werdoc_contflwo<<",  "<<rdoc_con * wetflwo/1000.0<<endl;
    m_WetDOC[id] =  m_WetDOC[id] - rdoc_con*wetflwo/1000.0 -  rdoc_con * wetsep /1000.0;
    m_WetDIC[id] =  m_WetDIC[id] - dic_con*wetflwo/1000.0  -  dic_con * wetsep /1000.0;
    m_WetLPOC[id] =  m_WetLPOC[id] - lpoc_con*wetflwo/1000.0;
    m_WetRPOC[id] =  m_WetRPOC[id] - rpoc_con*wetflwo/1000.0 ;
    if(m_WetDOC[id]<=0.f)  {
        m_WetDOC[id] = 0;
        m_WetDIC[id] = 0;
        m_WetLPOC[id] = 0;
        m_WetRPOC[id] = 0;
    }
    //cout<<id<<"   "<<rdoc_con<<endl;
    //if(id==0) {
    m_wetland_oc[0][3] +=m_WetDOC[id];
    m_wetland_oc[0][4] +=rdoc_con*wetflwo/1000.0; //mg, DOC load
    m_wetland_oc[0][5] +=rdoc_con*wetsep/1000.0; //mg, DOC seeps

    m_wetland_wt[0][1] +=(wetpcp - wetev); //m3
    m_wetland_wt[0][2] +=m_WetVol[id];
    m_wetland_wt[0][3] +=wetflwo;
    m_wetland_wt[0][4] +=wetsep;
    m_surf_leachdoc[id] = rdoc_con;
    //}

}


void WETLAND::Get1DData(const char* key, int* n, float** data) {
    InitialOutputs();
    string sk(key);

    if (StringMatch(sk, "wet_vol")) *data = m_WetVol;
    // else if (StringMatch(sk, VAR_PERC_LOWEST_DOC)) {
    //     *data = m_soilPercoCbnLowest;
    //     *n = m_nSubbasins + 1;
    //     return;
    // }
    if (StringMatch(sk, "wetdoccon")) *data = m_wetdoccon;
    if (StringMatch(sk, "surf_wetdoc")) *data = m_surf_leachdoc;

    else {
        throw ModelException("WETLAND", "Get1DData", "Parameter" + sk + "does not exist.");
    }
    *n = m_nCells;
}


void WETLAND::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string sk(key);
	if (StringMatch(sk, "wetland_wt")) {
		*data = m_wetland_wt;
		*nrows = m_nSubbasins + 1;
		*ncols = 6;
	}
    if (StringMatch(sk, "wetland_oc")) {
		*data = m_wetland_oc;
		*nrows = m_nSubbasins + 1;
		*ncols = 6;
	}
}
