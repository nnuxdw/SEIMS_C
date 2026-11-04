#include "glacier_deltah.h"
#include "text.h"
#include "utils_time.h"
GLA_DH::GLA_DH(void) : m_nCells(-1), m_nSubbsns(-1), m_tMean(NULL), m_subbsnID(NULL),
 m_iceStorage(NULL),   m_prec(NULL),
//m_gArea(NULL),m_albedo(NULL), m_iceStorage_init(NULL),isglacier(NULL),
m_t(NULL), m_snowStorage(NULL), m_glacQ(NULL), m_dGlacRunoff(NULL),  hlu_glacier_area(NULL), hlu_area(NULL), hlu_nonglacier_area(NULL),
m_sub_qfast(NULL), m_sub_qslow(NULL), m_sub_qg(NULL), m_sub_glacierq(NULL), m_gmb(NULL),
ss(NULL), sw(NULL), su(NULL), sf(NULL), sg(NULL), sfg(NULL), Qfgglacier(NULL), m_snomelt(NULL),
Qfg(NULL), Qf(NULL), Qs(NULL), Qm(NULL),  m_sr(NULL), m_landUse(NULL), sublimation(NULL), m_CellWidth(-1.f),

Qave(NULL), gmb_ave(NULL), m_nGlc(-1), m_nGlc_sum(-1),E(NULL), h(NULL), area(NULL),Qall(0), Qnogla(0), Q(0),
//parameter
Sumax(NODATA_VALUE), m_beta0(NODATA_VALUE), m_dd_Snow(NODATA_VALUE), m_tGmit(NODATA_VALUE), m_dd_glacierMelt(NODATA_VALUE), m_srf(NODATA_VALUE),
D(NODATA_VALUE),Kf(NODATA_VALUE),Ks(NODATA_VALUE),Kfg(NODATA_VALUE),Ce(NODATA_VALUE), Ca(NODATA_VALUE), Cg(NODATA_VALUE),


hscaled(NULL), ascaled(NULL), E_norm(NULL), Emax(NULL), Emin(NULL), dth(NULL), ascaled_sum(NULL), vscaled_sum(NULL), dtv(NULL), dtm(NULL),m_hluID(NULL),n1(-1),
m_csnow6(NODATA_VALUE), m_csnow12(NODATA_VALUE),m_snowTemp(NODATA_VALUE),sno_pack(NULL)

/// Plant operation related parameters
///m_glacierLookup(nullptr), m_glacierNum(-1)
{
	///
}
GLA_DH::~GLA_DH(void) {
	/// release map containers
	//if (!m_glacierLookupMap.empty()) {
	//	for (auto it = m_glacierLookupMap.begin(); it != m_glacierLookupMap.end();) {
	//		if (it->second != nullptr) {
	//			delete[] it->second;
	//			it->second = nullptr;
	//		}
	//		it->second = nullptr;
	//		m_glacierLookupMap.erase(it++);
	//	}
	//	m_glacierLookupMap.clear();
	//}
	if (this->m_snowStorage != NULL) Release1DArray(this->m_snowStorage);
	if (this->m_iceStorage != NULL) Release1DArray(this->m_iceStorage);
	if (this->m_glacQ != NULL) Release1DArray(this->m_glacQ);
	if (this->m_dGlacRunoff != NULL) Release2DArray(m_nCells, this->m_dGlacRunoff);

	if (this->m_sub_qfast != NULL) Release1DArray(this->m_sub_qfast);
	if (this->m_sub_qslow != NULL) Release1DArray(this->m_sub_qslow);
	if (this->m_sub_qg != NULL) Release1DArray(this->m_sub_qg);
	if (this->m_sub_glacierq != NULL) Release1DArray(this->m_sub_glacierq);
	if (this->m_gmb != NULL) Release1DArray(this->m_gmb);

	//ljj
	if (this->ss != NULL) Release1DArray(this->ss);
	if (this->sw != NULL) Release2DArray(m_nCells,this->sw);
	if (this->su != NULL) Release1DArray(this->su);
	if (this->sf != NULL) Release1DArray(this->sf);
	if (this->sg != NULL) Release2DArray(m_nCells, this->sg);
	if (this->sfg != NULL) Release2DArray(m_nCells, this->sfg);
	if (this->sublimation != NULL) Release2DArray(this->m_nCells, sublimation);
	if (this->sno_pack != NULL) Release1DArray(this->sno_pack);

	//if (this->Qfgglacier != NULL) Release2DArray(m_nCells, this->Qfgglacier);
	if (this->Qf != NULL) Release1DArray(this->Qf);
	if (this->Qs != NULL) Release1DArray(this->Qs);
	if (this->Qm != NULL) Release1DArray(this->Qm);
	if (this->m_snomelt != NULL) Release2DArray(this->m_nCells, m_snomelt);

	if (this->Qfg != NULL) Release1DArray(this->Qfg);
	if (this->Qave != NULL)Release1DArray(this->Qave);
	if (this->gmb_ave != NULL) Release1DArray(this->gmb_ave);

	if (this->hscaled != NULL) Release2DArray(m_nGlc, this->hscaled);
	if (this->ascaled != NULL) Release2DArray(m_nGlc, this->ascaled);
}

bool GLA_DH::CheckInputData(void) {
	CHECK_POSITIVE("MID_DEGREEDAYMELT", this->m_nCells);
	CHECK_POSITIVE("MID_DEGREEDAYMELT", this->m_nGlc_sum);
	CHECK_NODATA("MID_DEGREEDAYMELT", this->m_srf);
	CHECK_NODATA("MID_DEGREEDAYMELT", this->m_dd_glacierMelt);
	CHECK_NODATA("MID_DEGREEDAYMELT", this->Kfg);
	CHECK_NODATA("MID_DEGREEDAYMELT", this->Ca);
	CHECK_NODATA("MID_DEGREEDAYMELT", this->Cg);
	//CHECK_NODATA(MID_DEGREEDAYMELT, this->m_CellWidth);
	//CHECK_DATA(MID_DEGREEDAYMELT, this->m_iceStorage_init);
	//CHECK_POINTER(MID_DEGREEDAYMELT, this->m_albedo);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_sr);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_tMean);
	CHECK_POINTER("MID_DEGREEDAYMELT", this->m_prec);
	CHECK_POINTER("MID_DEGREEDAYMELT", hlu_glacier_area);
	CHECK_POINTER("MID_DEGREEDAYMELT", hlu_area);
	//CHECK_POINTER(MID_DEGREEDAYMELT, this->m_hluID);
	//CHECK_POINTER(MID_DEGREEDAYMELT, this->m_landUse);
	//CHECK_POINTER(MID_DEGREEDAYMELT, this->m_subbsnID);
	//CHECK_POINTER(MID_DEGREEDAYMELT, this->m_iceStorage_init);
    //if (this->m_nCells <= 0)  throw ModelException(MID_DEGREEDAYMELT , "CheckInputData", "the number of the glaciers can not be less than zero.");
	//if (this->m_t <= 0)  throw ModelException(MID_DEGREEDAYMELT , "CheckInputData", "the day of year can not be less than zero.");
	//if (this->m_gArea <= 0)  throw ModelException(MID_DEGREEDAYMELT, "CheckInputData", "You have not set the area of glacier.");
    return true;
}

void GLA_DH:: InitialOutputs() {
	CHECK_POSITIVE("MID_DEGREEDAYMELT", m_nCells);
    if (m_iceStorage == NULL) Initialize1DArray(m_nCells, m_iceStorage, 0.F);
	if (m_snowStorage == NULL) Initialize1DArray(m_nCells, m_snowStorage, 0.f);
	if (m_glacQ == NULL) Initialize1DArray(m_nCells, m_glacQ, 0.f);
	if (m_dGlacRunoff == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, m_dGlacRunoff, 0.f);

	if (m_sub_qslow == NULL) Initialize1DArray(m_nSubbsns + 1, m_sub_qslow, 0.f);
	if (m_sub_qfast == NULL) Initialize1DArray(m_nSubbsns + 1, m_sub_qfast, 0.f);
	if (m_sub_qg == NULL) Initialize1DArray(m_nSubbsns + 1, m_sub_qg, 0.f);
	if (m_sub_glacierq == NULL) Initialize1DArray(m_nSubbsns + 1, m_sub_glacierq, 0.f);
	if (m_gmb == NULL) Initialize1DArray(m_nSubbsns + 1, m_gmb, 0.f);

	if (hlu_nonglacier_area == NULL) Initialize1DArray(m_nCells, hlu_nonglacier_area, 0.f);

	//ljj
	if (ss == NULL) Initialize1DArray(m_nCells, ss, 0.f);
	if (sw == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, sw, 0.f);
	if (su == NULL) Initialize1DArray(m_nCells, su, 0.f);
	if (sf == NULL) Initialize1DArray(m_nCells, sf, 0.f);
	if (sg == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, sg, 999999.f);
	if (sfg == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, sfg, 0.f);
	if (Qfgglacier == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, Qfgglacier, 0.f);
    if (Qf == NULL) Initialize1DArray(m_nCells, Qf, 0.f);
    if (Qs == NULL) Initialize1DArray(m_nCells, Qs, 0.f);
    if (Qm == NULL) Initialize1DArray(m_nCells, Qm, 0.f);
	if (Qfg == NULL) Initialize1DArray(m_nCells, Qfg, 0.f);
	if (m_snomelt == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, m_snomelt, 0.f);
	if (sublimation == NULL) Initialize2DArray(m_nCells, m_nGlc_sum, sublimation, 0.f);
	if (sno_pack == NULL) Initialize1DArray(m_nCells, sno_pack, 0.f);

	if (Qave == NULL) Initialize1DArray(m_nGlc_sum,  Qave, 0.f);
	if (gmb_ave == NULL) Initialize1DArray(m_nGlc_sum, gmb_ave, 0.f);
	///hparameter
	if (E_norm == NULL) Initialize1DArray(n1, E_norm, 0.f);
	if (dth == NULL) Initialize1DArray(n1, dth, 0.f);
	if (dtv == NULL) Initialize1DArray(m_nGlc_sum, dtv, 0.f);
	if (dtm == NULL) Initialize1DArray(m_nGlc_sum, dtm, 0.f);
	if (hscaled == NULL) Initialize2DArray(m_nGlc_sum, n1, hscaled, 0.f);
	if (ascaled == NULL) Initialize2DArray(m_nGlc_sum, n1, ascaled, 0.f);
	if (ascaled_sum == NULL) Initialize1DArray(m_nGlc_sum, ascaled_sum, 0.f);
	if (vscaled_sum == NULL) Initialize1DArray(m_nGlc_sum, vscaled_sum, 0.f);
}


bool GLA_DH::normal(const int i) {

	//  float E0 = 100; //todo
	//  float Ea = 0.f; //actual evaporation
	//  float Ru = 0.f; //exceeds the storage capacity and cannot be stored in Su
	//  su[i] += m_prec[i] - Ea;
	//  su[i] -= Ru;
	//  su[i] = Min(su[i], Sumax);
	//  Ru = (m_prec[i])*(1- pow(1- (su[i] / (1+m_beta0)/ Sumax),m_beta0));
	//  //cout << i << " " << Ru << " " << m_prec[i] << " " << su[i] << " " << m_beta0 << " " << Sumax << endl;
	//  Ea = E0 * su[i] / (Ce *Sumax);
	//  if (Ru < 0.f) Ru = 0.f;
	//  if(su[i] < 0.f) su[i] = 0.f;
	//  sf[i] += Ru * D;
	//  ss[i] += Ru * (1-D);

	//  sf[i] -= Qf[i];
	//  ss[i] -= Qs[i];
	// // cout << i <<" "<< Ru<<" "<<sf[i] << " " << ss[i] << " " << Qf[i] << " "<<Qs[i] << endl;
	//  if(sf[i] < 0.f) sf[i] = 0.f;
	//  if(ss[i] < 0.f) ss[i] = 0.f;

	//  Qf[i] = sf[i]/Kf;
	//  Qs[i] = ss[i]/Ks;
	//  //cout << i << " " << sf[i] << " " << ss[i] << " " << Qf[i] << " " << Qs[i] << endl;
	//  Qf[i] = Min(sf[i], Qf[i]);
	//  Qs[i] = Min(ss[i], Qs[i]);
	float Ps = 0.f; //snow
	float P1 = 0.f; //rain
	if (m_tMean[i] > m_snowTemp) { 
		P1 = m_prec[i];
	}
	else {
		Ps = m_prec[i];
	}
	float sinv = CVT_FLT(sin(2.f * PI / 365.f * (m_dayOfYear - 81.f)));
    float cmelt = (m_csnow6 + m_csnow12) * 0.5f + (m_csnow6 - m_csnow12) * 0.5f * sinv;
	float m_snomelt = 0.f;
	if (m_tMean[i] <= 0) {
		m_snomelt = 0.f;
	}else {
		m_snomelt = cmelt * m_tMean[i] * Ca;
	}
	sno_pack[i] += Ps;
	//Snow sublimation
	float sublimation = 0.0864*(-7.093*m_tMean[i] + 28.26) / (pow(m_tMean[i], 2) - 3.593*m_tMean[i] + 5.175);
	sno_pack[i] -= sublimation;
	if (sno_pack[i] < 0.f) sno_pack[i] = 0.f;
	m_snomelt = Min(m_snomelt, sno_pack[i]);
	sno_pack[i]-= m_snomelt;

	Qf[i] = m_snomelt + P1;
	Qs[i] = 0.f;
	return true;
}

bool GLA_DH::glacier_all(const int i,const int j) {
	float Ps = 0.f; //snow
	float P1 = 0.f; //rain

	//Gao-2020-Stepwise modeling and the importance of internal variables validation to test model realism in a data scarce glacier basin
	//eq(2)
	if (m_tMean[i] > m_snowTemp) { 
		P1 = m_prec[i];
	}
	else {
		Ps = m_prec[i];
	}
	//float m_albedo = 0.5f;
	//eq(4)
	float sinv = CVT_FLT(sin(2.f * PI / 365.f * (m_dayOfYear - 81.f)));
    float cmelt = (m_csnow6 + m_csnow12) * 0.5f + (m_csnow6 - m_csnow12) * 0.5f * sinv;
	if (m_tMean[i] <= 0) {
		m_snomelt[i][j] = 0.f;
	}
	else {
		//m_snomelt[i,j] = m_dd_Snow * m_tMean[i] + m_srf * (1 - m_albedo[i]) * m_sr[i];  //m_dd_Snow = degree-day factor 
		m_snomelt[i][j] = cmelt * m_tMean[i] * Ca;
	}

	sw[i][j] += Ps;
	//Snow sublimation
	sublimation[i][j] = 0.0864*(-7.093*m_tMean[i] + 28.26) / (pow(m_tMean[i], 2) - 3.593*m_tMean[i] + 5.175);
	sw[i][j] -= sublimation[i][j];
	if (sw[i][j] < 0.f) sw[i][j] = 0.f;
	m_snomelt[i][j] = Min(m_snomelt[i][j], sw[i][j]);
	sw[i][j] -= m_snomelt[i][j];

	if (m_tMean[i] > 0 && sw[i][j] == 0) {
		//m_dGlacRunoff[i,j] = m_dd_glacierMelt * m_tMean[i] + m_srf * (1 - m_albedo[i]) * m_sr[i];
		m_dGlacRunoff[i][j] = m_dd_Snow * Cg * Ca * m_tMean[i];
	}
	if (m_tMean[i] <= 0 || sw[i][j] > 0) {
		m_dGlacRunoff[i][j] = 0.f;
	}

	m_dGlacRunoff[i][j] = Min(m_dGlacRunoff[i][j], sg[i][j]);
	sg[i][j] -= m_dGlacRunoff[i][j];
	if (sg[i][j] < 0.f) sg[i][j] = 0.f;
	m_dGlacRunoff[i][j] += m_snomelt[i][j];


	//sfg[i][j] += P1 + m_dGlacRunoff[i][j] - Qfgglacier[i][j];
	sfg[i][j] += P1 + m_dGlacRunoff[i][j];
	//if (i == 0) {
	//	cout << m_tMean[i]<<" "<< m_prec[i] << " " << P1 << " " << m_dGlacRunoff[i][0] << endl;
	//}
	if (sfg[i][j] < 0.f) sfg[i][j] = 0.f;
	Qfgglacier[i][j] = sfg[i][j] / Kfg;
	sfg[i][j] -= Qfgglacier[i][j] ;
	return true;
}

bool GLA_DH::hparameter(const int j) {

	float a, b, c, y;
    float aice = 850; //aice=850kg/m^3,�Ǳ����ܶ�
    //float Emax = 6099, Emin = 5283;
	
	dtm[j] = dtv[j+1] * aice;
    float fs, v, area_sum, tt;
    area_sum = 0.0;
	for (int i = 0; i < n1; i++) {
		area_sum = area_sum + area[j][i+1];
	}
		
	if (area_sum == 0) { return true; }
	if (area_sum < 5e6) { //5km^2
		a = -0.3;
		b = 0.6;
		c = 0.09;
		y = 2;
	}
	else if (area_sum > 20e6) {
		a = -0.02;
		b = 0.12;
		c = 0;
		y = 6;
	}
	else {
		a = -0.05;
		b = 0.19;
		c = 0.01;
		y = 4;
	}
	v = 0.0;
	for (int i = 0; i < n1; i++) {
		E_norm[i] = (Emax[j] - E[j][i+1]) / (Emax[j] - Emin[j]);
		//cout << i << " " << Emax[j] << " " << E[j][i+1] << " " << Emin[j] << " "<<area[j][i+1]<<endl;
		dth[i] = pow(E_norm[i] + a, y) + b * (E_norm[i] + a) + c;
		if (dth[i] > 1) {
			dth[i] = 0;
		}
		v = v + area[j][i+1] * dth[i];
	}
	fs = dtm[j] / v/ aice;
	//case1:
	for (int i = 0; i < n1; i++) {
		hscaled[j+1][i] = h[j][i+1]+ fs * dth[i];
		hscaled[j+1][i] = Max(hscaled[j+1][i], 0.f);
		if (hscaled[j+1][i] != 0)
		{
			ascaled[j+1][i] = area[j][i+1];
		}
		if (hscaled[j + 1][i] == 0)
		{
			ascaled[j + 1][i] = 0;
		}
		//ascaled[j][i]=area[i]*sqrt(hscaled[j][i]/h[i]);
		ascaled[j+1][i] = Max(ascaled[j+1][i], 0.f);
	}
	//case2:
	/*for (int i = 0; i < n1; i++) {
		hscaled[j+1][i] = h[i]+ fs * dth[i];
		hscaled[j+1][i] = Max(hscaled[j+1][i], 0.f);
		ascaled[j + 1][i] = area[i] * sqrt(hscaled[j + 1][i] / h[i]);
		ascaled[j + 1][i] = Max(ascaled[j + 1][i], 0.f);
		hscaled[j + 1][i] = hscaled[j + 1][i] / sqrt(hscaled[j + 1][i] / h[i]);
		hscaled[j + 1][i] = Max(hscaled[j + 1][i], 0.f);
	}*/
	//////////////////////////////////////////////////////
	for (int i = 0; i < n1; i++) {
		ascaled_sum[j+1] = ascaled_sum[j+1] + ascaled[j+1][i];
		vscaled_sum[j+1] = vscaled_sum[j+1] + ascaled[j+1][i] * hscaled[j+1][i];
	}
	///�ֵ���ˮ�ĵ�Ԫ
	for (int i = 0; i < m_nCells; i++) {
		if (m_hluID[i][j] != 100) {
			if (ascaled[j + 1][CVT_INT(m_hluID[i][j])] != 0)
			{
				//non_glacier[i][j] = hlu_glacier_area[i][j] - ascaled[j][i] * hlu_glacier_area[i][j] / area[CVT_INT(m_hluID[i])][j];
				hlu_glacier_area[i][j + 1] = ascaled[j + 1][CVT_INT(m_hluID[i][j])] * hlu_glacier_area[i][j + 1] / area[j][CVT_INT(m_hluID[i][j])+1];
			}
			else {
				hlu_glacier_area[i][j + 1] = 0.f;
			}
		}
	}
	for (int i = 0; i < n1; i++) {
		area[j][i+1] = ascaled[j+1][i];
		h[j][i+1] = hscaled[j+1][i];
	}
	//for (int i = 0; i < n1; i++)
	//{
		//cout << j << " " << i<< " " << vscaled_sum[j+1] << " " << hscaled[j + 1][i] << " " << hlu_glacier_area[i][j+1] << endl;
	//	cout << i << " " << area[i] << endl;
	//}
	return true;
}

int GLA_DH::Execute() {
	this->CheckInputData();
	this->InitialOutputs();
	
	//�Ǳ�����
	float non_qsum = 0.0;
	float glacier_area = 0.0;
	float glacier_area1 = 0.0;
	for (int i = 0; i < m_nCells; i++) {
		if(hlu_area[i] <=0) continue;
		glacier_area = glacier_area + hlu_area[i];///hlu_area[i]Ϊ��ʼ�����б����������
		glacier_area1 = glacier_area1 + hlu_glacier_area[i][0];///hlu_glacier_area[i][0]Ϊ���б����������(�ǲ��ϱ仯�ģ�
		hlu_nonglacier_area[i] = hlu_area[i] - hlu_glacier_area[i][0];
		if (hlu_nonglacier_area[i] > 0) {
			normal(i);
		}
		non_qsum += (Qf[i] + Qs[i])*hlu_nonglacier_area[i];
		//cout << i << " " << hlu_area[i] << endl;
	}
	if (glacier_area != glacier_area1) {
		Qnogla = non_qsum / (glacier_area - glacier_area1);
	}
	else {
		Qnogla = 0.f;
	}

	//������
	for (int j = 0; j < m_nGlc_sum; j++)
	{

		float gmb_t = 0.0;
		float Qt = 0.0;
		for (int i = 0; i < m_nCells; i++) {
			if (hlu_glacier_area[i][j]>0) {
				glacier_all(i, j);
			}
		}
		//cout << glacier_area << endl;
		for (int i = 0; i < m_nCells; i++) {
			Qt += Qfgglacier[i][j] * hlu_glacier_area[i][j];
			gmb_t += (m_prec[i] - Qfgglacier[i][j] - sublimation[i][j]) * hlu_glacier_area[i][j];
			//cout << j << " " << i << " " << m_prec[i] << " " << Qfg1[i][j] << " " << sublimation[i][j] <<" "<< hlu_glacier_area[i][j]<<" "<< gmb_t<<endl;
		}
		float singlearea = 0.f;
		for (int iw = 0; iw < m_nCells; iw++)
		{
			singlearea = singlearea + hlu_glacier_area[iw][j];
		}
		if (singlearea != 0) {
			Qave[j] = Qt / singlearea;
			gmb_ave[j] = gmb_t / singlearea;
		}
		else {
			Qave[j] = 0.f;
			gmb_ave[j] = 0.f;
		}
		dtv[j] = gmb_t;
	}

	for (int i = 0; i < m_nCells; i++) {
		if (hlu_area[i] != 0) {
			Qfg[i] = (Qfgglacier[i][0] * hlu_glacier_area[i][0] + (Qf[i] + Qs[i])*hlu_nonglacier_area[i]) / hlu_area[i];
		}
		else {
			Qfg[i] = 0.f;
		}
	//	cout << i << " " << Qfg[i] << endl;
	}


	//�Ǳ����������ӱ���������
	Qall =  (Qave[0] * glacier_area1 + non_qsum) / glacier_area;
	Q =  (Qave[0] * glacier_area1 + non_qsum)*0.001;

	for (int j = 0; j < m_nGlc_sum; j++)
	{
		dtv[j] = dtv[j] * 0.001;
	}
	for (int i = 0; i < m_nGlc_sum; i++) {
		ascaled_sum[i] = 0.f;
		vscaled_sum[i] = 0.f;
	}
	//for (int j = 0; j <n1; j++)
	//{
	//	cout<<j<<" "<<area[j]<<endl;
	//}
	for (int j = 0; j < m_nGlc; j++)
	{
		hparameter(j);
	}

	//for (int j = 0; j < n1; j++)
	//{
	//	cout << j << " " << area[j] << endl;
	//}

	for (int j = 1; j < m_nGlc_sum; j++)
	{
		ascaled_sum[0] = ascaled_sum[0] + ascaled_sum[j];
		vscaled_sum[0] = vscaled_sum[0] + vscaled_sum[j];
	}
	for (int i = 0; i < n1; i++)
	{
		ascaled[0][i] = 0.f;
		for (int j = 1; j < m_nGlc_sum; j++)
		{
			ascaled[0][i] = ascaled[0][i] + ascaled[j][i];
		}
		//cout << ascaled_sum[1]<<" "<< i << " " << ascaled[1][i]  << endl;
	}
	for (int i = 0; i < m_nCells; i++)
	{
		hlu_glacier_area[i][0] = 0.f;
		for (int j = 1; j < m_nGlc_sum; j++)
		{
			hlu_glacier_area[i][0] = hlu_glacier_area[i][0] + hlu_glacier_area[i][j];
		}
		//cout << i << " " << hlu_glacier_area[i][0] << " " << hlu_glacier_area[i][1] << endl;
	}


	return 0;
}

   

void GLA_DH::SetValue(const char *key, float data) {
    string s(key);
    //if (StringMatch(s, VAR_GLNU)) { this->m_nGlc = data; }
	if (StringMatch(s, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(data);
	else if (StringMatch(s, "GL_SRF")) this->m_srf = data;
	else if (StringMatch(s, "GL_FACTOR")) this->m_dd_glacierMelt = data;
	else if (StringMatch(s, Tag_CellWidth)) this->m_CellWidth = data;
	else if (StringMatch(s, "Kfg")) this->Kfg = data;
	else if (StringMatch(s, "Ca")) this->Ca = data;
	else if (StringMatch(s, "Cg")) this->Cg = data;
	else if (StringMatch(s, VAR_C_SNOW6)) m_csnow6 = data;
    else if (StringMatch(s, VAR_C_SNOW12)) m_csnow12 = data;
	else if (StringMatch(s, VAR_T_SNOW)) m_snowTemp = data;
	else if (StringMatch(s, "GL_SNDD")) this->m_dd_Snow = data;
	else
	{
		throw ModelException("MID_DEGREEDAYMELT", "SetValue", "Parameter " + s
            + " does not exist in current module. Please contact the module developer.");
    }
}   
//parameter and input 
void GLA_DH::Set1DData(const char *key, int n, float *data) {
    string s(key);
	if (StringMatch(s, VAR_LANDUSE)) m_landUse = data;
    else if (StringMatch(s, VAR_TMEAN)) {
		CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nCells);
		this->m_tMean = data;
	}
    else if (StringMatch(s, VAR_PCP)) {
		CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nCells);
		this->m_prec = data;
	}
	//else if (StringMatch(s, DataType_Albedo)) {
	//	CheckInputSize(MID_DEGREEDAYMELT, key, n, m_nCells);
	//	this->m_albedo = data;
	//}
	else if (StringMatch(s, DataType_SolarRadiation)) {
		CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nCells);
		this->m_sr = data;
	}else if (StringMatch(s, VAR_SUBBSN)) {
		CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nCells);
		this->m_subbsnID = data;
	}//else if (StringMatch(s, VAR_HLU_ID)) {
	 //	CheckInputSize(MID_DEGREEDAYMELT, key, n, m_nCells);
	 //	this->m_hluID = data;
	//}//else if (StringMatch(s, VAR_CELLAREA)) {
	//	CheckInputSize(MID_DEGREEDAYMELT, key, n, m_nCells);
	//	this->hlu_area = data;
	//}
	//else if (StringMatch(s, "ISGLACIER")) { this->isglacier = data;}
	else if(StringMatch(s, "HLU_AREA")) { this->hlu_area = data; }

	//else if (StringMatch(s, VAR_GL_AREA_h)) {
	//	CheckInputSize(MID_DEGREEDAYMELT, key, n, n1);
	//	this->area = data;
	//}else if (StringMatch(s, VAR_GL_THICKNESS)) {
	//	CheckInputSize(MID_DEGREEDAYMELT, key, n, n1);
	//	this->h = data;
	//}else if (StringMatch(s, VAR_GL_ELEVATION_AVE)) {
	//	CheckInputSize(MID_DEGREEDAYMELT, key, n, n1);
	//	this->E = data;
	//}
	else if (StringMatch(s, "Emax")) {
		CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nGlc);
		this->Emax = data;
	}
	else if (StringMatch(s, "Emin")) {
		CheckInputSize("MID_DEGREEDAYMELT", key, n, m_nGlc);
		this->Emin = data;
	}else {
		throw ModelException("MID_DEGREEDAYMELT", "Set1DData", "Parameter " + s + " does not exist.");
	}
}

void GLA_DH::Set2DData(const char* key, const int n, const int col, float** data) {
	string sk(key);
	/// lookup tables
	//if (StringMatch(sk, VAR_GLACIER_LOOKUP)) {
	//	m_glacierLookup = data;
	//	m_glacierNum = n;
	//	InitializeGlacierLookup();
	//	if (col != Glacier_PARAM_COUNT) {
	//		throw ModelException(MID_PLTMGT_SWAT, "ReadGlacierLookup", "The field number " + ValueToString(col) +
	//			"is not coincident with LANDUSE_PARAM_COUNT: " +
	//			ValueToString(Glacier_PARAM_COUNT));
	//	}
	//	return;
	//}
	/// 2D raster data

	if (StringMatch(sk, "GLACIER_AREA")) {
		CheckInputSize2D("MID_DEGREEDAYMELT", key, n, col, m_nCells, m_nGlc_sum);
		this->hlu_glacier_area = data;
	}
	
	if (StringMatch(sk, "HLU_ID")) {
		CheckInputSize2D("MID_DEGREEDAYMELT", key, n, col, m_nCells, m_nGlc);
		this->m_hluID = data;
	}

	if (StringMatch(sk, "AREA_H")) {
		CheckInputSize2D("MID_DEGREEDAYMELT", key, n, col , m_nGlc,n1);
		this->area = data;
	}
	else if (StringMatch(sk, "THICKNESS")) {
		CheckInputSize2D("MID_DEGREEDAYMELT", key, n, col, m_nGlc,n1);
		this->h = data;
	}
	else if (StringMatch(sk, "ELEVATION_AVE")) {
		CheckInputSize2D("MID_DEGREEDAYMELT", key, n, col,m_nGlc,n1);
		this->E = data;
	}
	//if(StringMatch(sk, VAR_GL_AREA)) { this->hlu_area = data; }
	///hparameter
	//CheckInputSize2D(MID_DEGREEDAYMELT, key, n, col, n1, m_nGlc);
	//if (StringMatch(sk, VAR_GL_AREA_h)) { this->area = data; }
	//if (StringMatch(sk, VAR_GL_THICKNESS)) { this->h = data; }
	//if (StringMatch(sk, VAR_GL_ELEVATION_AVE)) { this->E = data; }
	
}


//output
void GLA_DH::Get1DData(const char *key, int *n, float **data) {
	InitialOutputs();
	string s(key);
	//*n = m_nCells;
	if (StringMatch(s, "Qfg")) {
		*n = m_nCells;
		*data = this->Qfg;
	}
  

	//if (StringMatch(s, "Sw")) { *data = this->sw; }
	if (StringMatch(s, "Su")) { *data = this->su; *n = m_nCells;}
	if (StringMatch(s, "Sf")) { *data = this->sf; *n = m_nCells;}
    if (StringMatch(s, "Ss")) { *data = this->ss; *n = m_nCells;}
	//if (StringMatch(s, "Sg")) { *data = this->sg; }
	//if (StringMatch(s, "Sfg")) { *data = this->sfg; }
	//if (StringMatch(s, "m_snomelt")) { *data = this->m_snomelt; }
	//if (StringMatch(s, "m_dGlacRunoff")) { *data = this->m_dGlacRunoff; }

	

	//*n = m_nGlc_sum;
	if (StringMatch(s, "GMB")) { *data = this->gmb_ave; *n = m_nGlc_sum;}
	if (StringMatch(s, "Qm")) { *data = this->Qave; *n = m_nGlc_sum;}


	//*n = m_nGlc_sum;
	if (StringMatch(s, "GL_AREA_SUM")) { *data = this->ascaled_sum;  *n = m_nGlc_sum;}
	if (StringMatch(s, "GL_VOLUMN_SUM")) { *data = this->vscaled_sum;  *n = m_nGlc_sum;}

	

	// if (StringMatch(s, "T_times")) { *data = this->m_tMean; }
	// if (StringMatch(s, "P_times")) { *data = this->m_prec; }
	 
}
void GLA_DH::GetValue(const char* key, float* value) {
	 string s(key);
	 if (StringMatch(s, "Qnogla")) { *value = this->Qnogla; }
	 if (StringMatch(s, "Qall")) { *value = this->Qall; }
	 if (StringMatch(s, "Q")) { *value = this->Q; }
	
}

//void GLA_DH::InitializeGlacierLookup() {
	/// Check input data
//	if (m_glacierLookup == nullptr) {
//		throw ModelException(MID_PLTMGT_SWAT, "CheckInputData", "Glacier lookup array must not be nullptr");
//	}
//	if (m_glacierNum <= 0) {
//		throw ModelException(MID_PLTMGT_SWAT, "CheckInputData", "Glacier number must be greater than 0");
//	}
//	if (!m_glacierLookupMap.empty()) {
//		return;
//	}
//	for (int i = 0; i < m_glacierNum; i++) {
//#ifdef HAS_VARIADIC_TEMPLATES
//		m_glacierLookupMap.emplace(CVT_INT(m_glacierLookup[i][1]), m_glacierLookup[i]);
//#else
//		m_glacierLookupMap.insert(make_pair(CVT_INT(m_glacierLookup[i][1]), m_glacierLookup[i]));
//#endif
//	}
//}

void GLA_DH::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
	InitialOutputs();
	string s(key);
	*nrows = m_nGlc;
	*ncols = n1;
	if (StringMatch(s, "GL_AREA")) { *data = this->ascaled; }
	if (StringMatch(s, "GL_THICKNESS")) { *data = this->hscaled; }
}

