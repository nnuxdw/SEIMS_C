#include "PER_STR.h"

#include "text.h"

PER_STR::PER_STR() :
    m_maxSoilLyrs(-1), m_nSoilLyrs(nullptr), m_soilThk(nullptr), m_dt(-1),
    m_nCells(-1), m_soilFrozenTemp(NODATA_VALUE), m_ks(nullptr),
    m_soilSat(nullptr), m_soilFC(nullptr),m_soilAWC(nullptr),
    m_soilWtrSto(nullptr), m_soilWtrStoPrfl(nullptr), m_soilTemp(nullptr), m_infil(nullptr),
    m_surfRf(nullptr), m_potVol(nullptr), m_impoundTrig(nullptr),
    m_soilPerco(nullptr),m_soilTempprofile(nullptr),m_soilWP(nullptr),
    m_soilIceSto(nullptr),m_clay(nullptr),m_soilPor(nullptr) {
}

PER_STR::~PER_STR() {
    if (m_soilPerco != nullptr) Release2DArray(m_nCells, m_soilPerco);
}

void PER_STR::InitialOutputs() {
    CHECK_POSITIVE(MID_PER_STR, m_nCells);
    if (nullptr == m_soilPerco) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_soilPerco, 0.f);
}

int PER_STR::Execute() {
    int frez =1;
    CheckInputData();
    InitialOutputs();
    // Perocolation move to SSR_DA module
// #pragma omp parallel for
//     for (int i = 0; i < m_nCells; i++) {
//         // Note that, infiltration, pothole seepage, irrigation etc. have been added to
//         // the first soil layer in other modules. By LJ
//         float excessWater = 0.f, maxSoilWater = 0.f, fcSoilWater = 0.f;
//         for (int j = 0; j < CVT_INT(m_nSoilLyrs[i]); j++) {
//             excessWater = 0.f;
//             maxSoilWater = m_soilSat[i][j];
////             fcSoilWater = m_soilFC[i][j];
//             fcSoilWater = m_soilAWC[i][j];
//             // determine gravity drained water in layer
//             excessWater += m_soilWtrSto[i][j] - fcSoilWater;
//             //if (i == 100)
//             //	cout<<"lyr: "<<j<<", soil storage: "<<m_soilStorage[i][j]<<", fc: "<<fcSoilWater<<", excess: "<<excessWater<<endl;
//             // for the upper two layers, soil may be frozen
//             // if (j == 0 && m_soilTemp[i] <= m_soilFrozenTemp) {
//             //     continue;
//             // }
//             //ljj++
//             if (frez = 0 && j == 0 && m_soilTempprofile[i][j] <= 0.f) {
//                 continue;
//             }

//             //ljj++
//             float WCND =  m_ks[i][j];
//             if(frez==1) {
//                 maxSoilWater = m_soilPor[i][j] * m_soilThk[i][j]-  m_soilIceSto[i][j];
//                 maxSoilWater  = Max(0.f,maxSoilWater);

//                 float FACTR = Max(0.01, (m_soilWtrSto[i][j])/maxSoilWater) ;
//                 FACTR = Min(FACTR,1.f);
//                 float BEXP = 2.91+0.159*m_clay[i][j]*0.01;
//                 float EXPON = 2.0*BEXP + 3.0 ;
//                 //WCND  = myDKSAT * FACTR ** EXPON
//                 float alpha = 3;
//                 float f_frozen=exp(-alpha*(1-Min(m_soilIceSto[i][j]/(m_soilPor[i][j]*m_soilThk[i][j]),1)));
//                 f_frozen = Max(0.f,f_frozen);
//                 f_frozen = f_frozen - exp(-alpha); 
//                 float WCND =  (1-f_frozen)*m_ks[i][j] * Min(1.0,pow(FACTR, EXPON))*3600;
//                 WCND = Min(WCND,m_ks[i][j]);
//             }

//             m_soilPerco[i][j] = 0.f;
//             // No movement if soil moisture is below field capacity
//             if (excessWater > 1.e-5f) {
//                 float maxPerc = maxSoilWater - fcSoilWater;
//                 if (maxPerc < 0.f) maxPerc = 0.1f;
//                 float tt = 3600.f * maxPerc / m_ks[i][j];                  // secs
//                 if(frez==1) tt = 3600.f * maxPerc / WCND ;  //ljj++
//                 m_soilPerco[i][j] = excessWater * (1.f - exp(-m_dt / tt)); // secs

//                 if (m_soilPerco[i][j] > maxPerc) {
//                     m_soilPerco[i][j] = maxPerc;
//                 }
//                 //Adjust the moisture content in the current layer, and the layer immediately below it
//                 m_soilWtrSto[i][j] -= m_soilPerco[i][j];
//                 excessWater -= m_soilPerco[i][j];
//                 m_soilWtrSto[i][j] = Max(UTIL_ZERO, m_soilWtrSto[i][j]);
//                 // redistribute soil water if above field capacity (high water table), rewrite from sat_excess.f of SWAT
//                 //float qlyr = m_soilStorage[i][j];
//                 if (j < CVT_INT(m_nSoilLyrs[i]) - 1) {
//                     m_soilWtrSto[i][j + 1] += m_soilPerco[i][j];
//                     if (m_soilWtrSto[i][j] - m_soilSat[i][j] > 1.e-4f) {
//                         m_soilWtrSto[i][j + 1] += m_soilWtrSto[i][j] - m_soilSat[i][j];
//                         m_soilWtrSto[i][j] = m_soilSat[i][j];
//                     }
//                 } else {
//                     /// for the last soil layer
//                     if (m_soilWtrSto[i][j] - m_soilSat[i][j] > 1.e-4f) {
//                         float ul_excess = m_soilWtrSto[i][j] - m_soilSat[i][j];
//                         m_soilWtrSto[i][j] = m_soilSat[i][j];
//                         for (int ly = CVT_INT(m_nSoilLyrs[i]) - 2; ly >= 0; ly--) {
//                             m_soilWtrSto[i][ly] += ul_excess;
//                             if (m_soilWtrSto[i][ly] > m_soilSat[i][ly]) {
//                                 ul_excess = m_soilWtrSto[i][ly] - m_soilSat[i][ly];
//                                 m_soilWtrSto[i][ly] = m_soilSat[i][ly];
//                             } else {
//                                 ul_excess = 0.f;
//                                 break;
//                             }
//                             if (ly == 0 && ul_excess > 0.f) {
//                                 // add ul_excess to depressional storage and then to surfq
//                                 if (m_potVol != nullptr && FloatEqual(m_impoundTrig[i], 0.f)) {
//                                     m_potVol[i] += ul_excess;
//                                 } else {
//                                     m_surfRf[i] += ul_excess;
//                                 }
//                                 m_infil[i] -= ul_excess;
//                             }
//                         }
//                     }
//                 }
//             } else {
//                 m_soilPerco[i][j] = 0.f;
//             }
//         }
//         /// update soil profile water
//         m_soilWtrStoPrfl[i] = 0.f;
//         for (int ly = 0; ly < CVT_INT(m_nSoilLyrs[i]); ly++) {
//             m_soilWtrStoPrfl[i] += m_soilWtrSto[i][ly];
//         }
//     }
    // DEBUG
    //cout << "PER_STR, cell id 14377, m_soilStorage: ";
    //for (int i = 0; i < (int)m_soilLayers[14377]; i++)
    //    cout << m_soilStorage[14377][i] << ", ";
    //cout << endl;
    // END OF DEBUG
    return 0;
}

void PER_STR::Get2DData(const char* key, int* nrows, int* ncols, float*** data) {
    InitialOutputs();
    string sk(key);
    *nrows = m_nCells;
    *ncols = m_maxSoilLyrs;
    if (StringMatch(sk, VAR_PERCO)) *data = m_soilPerco;
    else {
        throw ModelException(MID_PER_STR, "Get2DData", "Output " + sk + " does not exist.");
    }
}

void PER_STR::Set1DData(const char* key, const int nrows, float* data) {
    CheckInputSize(MID_PER_STR, key, nrows, m_nCells);
    string sk(key);
    if (StringMatch(sk, VAR_SOTE)) m_soilTemp = data;
    else if (StringMatch(sk, VAR_INFIL)) m_infil = data;
    else if (StringMatch(sk, VAR_SOILLAYERS)) m_nSoilLyrs = data;
    else if (StringMatch(sk, VAR_SOL_SW)) m_soilWtrStoPrfl = data;
    else if (StringMatch(sk, VAR_POT_VOL)) m_potVol = data;
    else if (StringMatch(sk, VAR_SURU)) m_surfRf = data;
    else if (StringMatch(sk, VAR_IMPOUND_TRIG)) m_impoundTrig = data;
    else {
        throw ModelException(MID_PER_STR, "Set1DData", "Parameter " + sk + " does not exist.");
    }
}

void PER_STR::Set2DData(const char* key, const int nrows, const int ncols, float** data) {
    CheckInputSize2D(MID_PER_STR, key, nrows, ncols, m_nCells, m_maxSoilLyrs);
    string sk(key);
    if (StringMatch(sk, VAR_CONDUCT)) m_ks = data;
    else if (StringMatch(sk, VAR_SOILTHICK)) m_soilThk = data;
    else if (StringMatch(sk, VAR_SOL_UL)) m_soilSat = data;
    else if (StringMatch(sk, VAR_SOL_AWC)) m_soilAWC = data; //m_soilFC = data;
    else if (StringMatch(sk, VAR_SOL_ST)) m_soilWtrSto = data;
    //ljj++
    else if (StringMatch(sk, VAR_SOL_WPMM)) m_soilWP = data;
    else if (StringMatch(sk, VAR_SOILT)) m_soilTempprofile = data;
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
    else {
        throw ModelException(MID_PER_STR, "Set2DData", "Parameter " + sk + " does not exist.");
    }
}

void PER_STR::SetValue(const char* key, const float value) {
    string s(key);
    if (StringMatch(s, Tag_TimeStep)) m_dt = CVT_INT(value);
    else if (StringMatch(s, VAR_T_SOIL)) m_soilFrozenTemp = value;
    else {
        throw ModelException(MID_PER_STR, "SetValue",
                             "Parameter " + s + " does not exist in current module.");
    }
}

bool PER_STR::CheckInputData() {
    CHECK_POSITIVE(MID_PER_STR, m_date);
    CHECK_POSITIVE(MID_PER_STR, m_nCells);
    CHECK_POSITIVE(MID_PER_STR, m_dt);
    CHECK_POINTER(MID_PER_STR, m_ks);
    CHECK_POINTER(MID_PER_STR, m_soilSat);
    CHECK_NODATA(MID_PER_STR, m_soilFrozenTemp);
    //CHECK_POINTER(MID_PER_STR, m_soilFC);
    CHECK_POINTER(MID_PER_STR, m_soilWtrSto);
    CHECK_POINTER(MID_PER_STR, m_soilWtrStoPrfl);
    CHECK_POINTER(MID_PER_STR, m_soilThk);
    CHECK_POINTER(MID_PER_STR, m_soilTemp);
    CHECK_POINTER(MID_PER_STR, m_infil);
    return true;
}
