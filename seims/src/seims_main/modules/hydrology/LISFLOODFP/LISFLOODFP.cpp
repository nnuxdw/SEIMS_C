#include "LISFLOODFP.h"


#include <map>
#include <set> 
#include "text.h"

LISFLOODFP::LISFLOODFP() :
	m_dt(-1), counter(0), m_subbsnID(nullptr),Arrptr(nullptr), FpsPtr(nullptr), Fnameptr(nullptr), Statesptr(nullptr), Parptr(nullptr), Solverptr(nullptr), Poisptr(nullptr), BCptr(nullptr),
	Stageptr(nullptr), SGCptr(nullptr), Damptr(nullptr), tmpFileNamePtr(nullptr), tmpSysCmdPtr(nullptr) {

}

LISFLOODFP::~LISFLOODFP() {
	LisFloodFP_Finilize(Solverptr, Arrptr, Fnameptr, FpsPtr, Statesptr, Parptr,  LFPContextPtr,  tmpFileNamePtr);

}

void LISFLOODFP::SetValue(const char* key, const float value) {
	string sk(key);
	if (StringMatch(sk, Tag_TimeStep)) m_dt = CVT_INT(value);
	else if (StringMatch(sk, Tag_StartTime)) seims_start_time = CVT_INT(value);
	else if (StringMatch(sk, Tag_CellSize)) m_nCells = CVT_INT(value);
	//else if (StringMatch(sk, Tag_CellWidth)) m_CellWth = value;
	else if (StringMatch(sk, VAR_SUBBSNID_NUM)) m_nSubbsns = CVT_INT(value);
	//else if (StringMatch(sk, Tag_SubbasinId)) m_inputSubbsnID = CVT_INT(value);
	else {
		throw ModelException(MID_LISFLOODFP, "SetValue", "Parameter " + sk + " does not exist.");
	}
}

void LISFLOODFP::Set1DData(const char* key, const int n, float* data) {
	
	string sk(key);

	if (StringMatch(sk, VAR_AHRU)) {
		CheckInputSize(MID_LISFLOODFP, key, n, m_nCells);
		//m_handArea = data;
	}
	else if (StringMatch(sk, VAR_SUBBSN)) m_subbsnID = data;
	//else if (StringMatch(sk, VAR_BKST)) m_bankSto = data;	
	else {
		throw ModelException(MID_LISFLOODFP, "Set1DData", "Parameter " + sk + " does not exist.");
	}
}

void LISFLOODFP::Set2DData(const char* key, const int n, const int col, float** data) {
    string sk(key);
    //if (!CheckInputSize2D("IO_TEST", key, n, col, m_nCells, m_maxSoilLyrs)) return;
    //if (StringMatch(sk, VAR_CONDUCT)) {
    //    m_raster2D = data;
    //}
}


void LISFLOODFP::SetReaches(clsReaches* reaches) {

	if (nullptr == reaches) {
		throw ModelException(MID_LISFLOODFP, "SetReaches", "The reaches input can not to be NULL.");
	}

	
}

bool LISFLOODFP::CheckInputData() {
    /// m_date is protected variable member in base class SimulationModule.
    //CHECK_POSITIVE("IO_TEST", m_date);
    //CHECK_POSITIVE("IO_TEST", m_nCells);
    //CHECK_POINTER("IO_TEST", m_raster1D);
    //CHECK_POINTER("IO_TEST", m_raster2D);
    //CHECK_POINTER("IO_TEST", m_nSoilLyrs);
    return true;
}

void LISFLOODFP::InitialOutputs() {
	//CHECK_POSITIVE(MID_LISFLOODFP, m_nreach);
	//CHECK_POSITIVE(MID_LISFLOODFP, m_nCells);
	char* argv[] = {
		(char*)"-v",
		(char*)"F:\\BasinFloodData\\BasinFloodData1726898305348_250m\\prepdata\\Basin\\lisfloodfp\\cali_test\\test-seims.par"
		};
	int argc = 2;

	memset(&Raster, 0, sizeof(Arrays));
	memset(&Fps, 0, sizeof(Files));
	memset(&ParFp, 0, sizeof(Fnames));
	memset(&SimStates, 0, sizeof(States));
	memset(&Params, 0, sizeof(Pars));
	memset(&ParSolver, 0, sizeof(Solver));
	memset(&PoisHandler, 0, sizeof(Pois));
	memset(&Bounds, 0, sizeof(BoundCs));
	memset(&OutLocs, 0, sizeof(Stage));
	memset(&SGCchanprams, 0, sizeof(SGCprams));
	memset(&DamDataprams, 0, sizeof(DamData));
	memset(&LFPContext, 0, sizeof(LISFLOODFPContext));
	
	
	Arrptr = &Raster;
	FpsPtr = &Fps;
	Fnameptr = &ParFp;
	Statesptr = &SimStates;
	Parptr = &Params;
	Solverptr = &ParSolver;
	Poisptr = &PoisHandler;
	BCptr = &Bounds;
	Stageptr = &OutLocs;
	SGCptr = &SGCchanprams;
	Damptr = &DamDataprams;
	LFPContextPtr = &LFPContext;
	tmpFileNamePtr = new char[255];
	tmpSysCmdPtr = new char[255];
	//Super_linksptr = new SuperGridLinksList();

	LisFloodFP_Initilize(argc, argv,Arrptr, FpsPtr, Fnameptr, Statesptr, Parptr, Solverptr, Poisptr, BCptr,Stageptr, SGCptr, Damptr,ChannelSegmentsVecPtr, LFPContextPtr, Super_linksptr, tmpFileNamePtr, tmpSysCmdPtr);

	//LisFloodFP_Initilize(argc, argv,Arrptr, FpsPtr, Fnameptr, Statesptr, Parptr, Solverptr, Poisptr, BCptr,Stageptr, SGCptr, Damptr,ChannelSegmentsVecPtr, LFPContextPtr, Super_linksptr, tmpFileNamePtr, tmpSysCmdPtr);

	LisFloodFP_Initilize(argc, argv, Arrptr, FpsPtr, Fnameptr, Statesptr, Parptr, Solverptr, Poisptr, BCptr, Stageptr, SGCptr, Damptr, ChannelSegmentsVecPtr, LFPContextPtr,  tmpFileNamePtr, tmpSysCmdPtr);

	
}

int LISFLOODFP::Execute() {
    /// Initialize output variables
    //if (nullptr == m_output1Draster) Initialize1DArray(m_nCells, m_output1Draster, 0.f);

    //if (nullptr == m_output2Draster) Initialize2DArray(m_nCells, m_maxSoilLyrs, m_output2Draster, NODATA_VALUE);

	//check the data
	CheckInputData();
	if (nullptr == Arrptr)
	{
		InitialOutputs();
	}
	counter++;
	int seims_cur_step_start_timestamp = LFPContextPtr->seims_begin_timestamp + (counter - 1) * m_dt;
	int seims_cur_step_end_timestamp = LFPContextPtr->seims_end_timestamp + counter * m_dt;
	int current_timestamp;
	if (seims_cur_step_start_timestamp < LFPContextPtr->rain_end_timestamp && seims_cur_step_end_timestamp > LFPContextPtr->rain_begin_timestamp)
	{
		updateCurrentTimestamp(&current_timestamp, LFPContextPtr);
		// The simulation starts at the begin time of lisfloodfp, but it must end at the end of a SEIMS time step.
		// This means lisfloodfp may run slightly longer than expected if its end time is not aligned with SEIMS.
		while (current_timestamp > seims_cur_step_start_timestamp && current_timestamp < seims_cur_step_end_timestamp) {
			Fast_RunStep(Arrptr, FpsPtr, Fnameptr, Statesptr, Parptr, Solverptr, Poisptr, SGCptr, Damptr, Locptr, LFPContextPtr, Super_linksptr);
			updateCurrentTimestamp(&current_timestamp, LFPContextPtr);
		}
	}
    return 0;
}

void LISFLOODFP::Get1DData(const char* key, int* n, float** data) {
    string sk(key);
	//if (StringMatch(sk, VAR_LISFLOODFP_WTRDEP)) {
	//*data = m_handWtrDep;
	//*n = m_nCells;
	//}
}

void LISFLOODFP::Get2DData(const char* key, int* n, int* col, float*** data) {
    string sk(key);
    //if (StringMatch(sk, "K_M")) {
    //    *data = this->m_output2Draster;
    //    *n = this->m_nCells;
    //    *col = this->m_maxSoilLyrs;
    //}
}

void LISFLOODFP::InitializeLisfloodFP() {

}

void LISFLOODFP::RunCalculation() {

}


void updateCurrentTimestamp(int * current_timestamp, LISFLOODFPContext* LFPContextPtr) {
	*current_timestamp = LFPContextPtr->curr_time + LFPContextPtr->rain_begin_timestamp;
}




