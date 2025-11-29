/*
#####################################################################################
LISFLOOD-FP flood inundation model
#####################################################################################

� copyright Bristol University Hydrology Research Group 2008

webpage -	http://www.ggy.bris.ac.uk/research/hydrology/models/lisflood
contact -	Professor Paul Bates, email: paul.bates@Bristol.ac.uk,
Tel: +44-117-928-9108, Fax: +44-117-928-7878

*/


#include "lisflood.h"
#include "VersionHistory.h"
#include "lisflood2/lisflood_processing.h"
#include "utility.h"
#include "sgc.h"
#include "swe/fv1.h"
#include "swe/dg2.h"
#ifdef CUDA
#include "cuda/acc/cuda_acc_simulate.cuh"													
#include "cuda/fv1/cuda_fv1_simulate.cuh"
#include "cuda/dg2/cuda_dg2_simulate.cuh"
#include "cuda/fv2/cuda_fv2_simulate.cuh"
#include "cuda/acc_nugrid/cuda_acc_nugrid_simulate.cuh"
#endif

#include "lisflood2/file_tool.h"

#include "time_tool.h"

//#include "lisflood2/lis2_output.h"
//#include "lisflood2/lisflood_processing.h"

//---------------------------------------------------------------------------
void printversion(int verbose)
// printout header with program and version number
{
	printf("***************************\n");
	printf(" BASINFLOOD version %d.%d.%d (%s)\n", LF_VersionMajor, LF_VersionMinor, LF_VersionInc, NUMERIC_TYPE_NAME);
	if (verbose == ON)
	{
#if defined (__INTEL_COMPILER)
		printf("Intel Compiler version: %d\n", __INTEL_COMPILER);

		//https://software.intel.com/en-us/node/514528
		printf("CPU instructions used:");
#if defined (__AVX2__)
		printf(" AVX2");
#endif
#if defined (__AVX__)
		printf(" AVX");
#endif
#if defined (__SSE4_2__)
		printf(" SSE_4.2");
#endif
#if defined (__SSE4_1__)
		printf(" SSE_4.1");
#endif
#if defined (__SSE3__)
		printf(" SSE3");
#endif
#if defined (__SSE2__)
		printf(" SSE2");
#endif
#if defined (__SSE__)
		printf(" SSE");
#endif
		printf("\n");

#endif
	}

#if defined (CUDA)
	printf("CUDA supported\n");
#endif
#if defined (_PROFILE_MODE) && _PROFILE_MODE > 0
	printf("Profile Mode Enabled: %d\n", _PROFILE_MODE);
#endif
#if defined (_SGM_BY_BLOCKS) && _SGM_BY_BLOCKS > 0
	printf("_SGM_BY_BLOCKS: %d\n", _SGM_BY_BLOCKS);
#endif
#if defined (_BALANCE_TYPE) && _BALANCE_TYPE > 0
	printf("_BALANCE_TYPE: %d\n", _BALANCE_TYPE);
#endif
#if defined (_ONLY_RECT) && _ONLY_RECT == 1
	printf("Rectangular channels only.\n");
#endif
#if defined (_DISABLE_WET_DRY) && _DISABLE_WET_DRY == 1
	printf("_DISABLE_WET_DRY.\n");
#endif
#if defined (_CALCULATE_Q_MODE) && (_CALCULATE_Q_MODE != 0)
	printf("_CALCULATE_Q_MODE %d.\n", _CALCULATE_Q_MODE);
#endif

	printf("***************************\n\n");
}

//-------------------DataTypes.cpp-------------------------
void AllocateWetDryRowBound(int row_count, int block_count, WetDryRowBound * wet_dry_bounds)
{
	wet_dry_bounds->fp_h = (IndexRange*)memory_allocate(row_count * sizeof(IndexRange));
	wet_dry_bounds->fp_h_prev = (IndexRange*)memory_allocate(row_count * sizeof(IndexRange));
	wet_dry_bounds->fp_vol = (IndexRange*)memory_allocate(row_count * sizeof(IndexRange));
	wet_dry_bounds->dem_data = (IndexRange*)memory_allocate(row_count * sizeof(IndexRange));


	wet_dry_bounds->block_count = block_count;
	wet_dry_bounds->block_row_bounds = (IndexRange*)memory_allocate(sizeof(IndexRange*) * block_count);

	for (int block_index = 0; block_index < block_count; block_index++)
	{
		wet_dry_bounds->block_row_bounds[block_index].start = -1;
		wet_dry_bounds->block_row_bounds[block_index].end = -1;
	}
}

void AllocateSubGridCellInfo(int cell_count, SubGridCellInfo * sub_grid_cell_info)
{
	sub_grid_cell_info->cell_count = cell_count;

	sub_grid_cell_info->sg_cell_x = (int*)memory_allocate(cell_count * sizeof(int));
	sub_grid_cell_info->sg_cell_y = (int*)memory_allocate(cell_count * sizeof(int));
	sub_grid_cell_info->sg_cell_grid_index_lookup = (int*)memory_allocate(cell_count * sizeof(int));

	sub_grid_cell_info->sg_cell_cell_area = (NUMERIC_TYPE*)memory_allocate(cell_count * sizeof(NUMERIC_TYPE));
	sub_grid_cell_info->sg_cell_dem = (NUMERIC_TYPE*)memory_allocate(cell_count * sizeof(NUMERIC_TYPE));

	sub_grid_cell_info->sg_cell_SGC_width = (NUMERIC_TYPE*)memory_allocate(cell_count * sizeof(NUMERIC_TYPE));
	sub_grid_cell_info->sg_cell_SGC_c = (NUMERIC_TYPE*)memory_allocate(cell_count * sizeof(NUMERIC_TYPE));
	sub_grid_cell_info->sg_cell_SGC_BankFullHeight = (NUMERIC_TYPE*)memory_allocate(cell_count * sizeof(NUMERIC_TYPE));
	sub_grid_cell_info->sg_cell_SGC_BankFullVolume = (NUMERIC_TYPE*)memory_allocate(cell_count * sizeof(NUMERIC_TYPE));

	sub_grid_cell_info->sg_cell_SGC_group = (int*)memory_allocate(cell_count * sizeof(int));
	sub_grid_cell_info->sg_cell_SGC_is_large = (int*)memory_allocate(cell_count * sizeof(int));
}

void ZeroSubGridCellInfo(SubGridCellInfo * sub_grid_cell_info, int cell_index)
{
	sub_grid_cell_info->sg_cell_x[cell_index] = -1;
	sub_grid_cell_info->sg_cell_y[cell_index] = -1;
	sub_grid_cell_info->sg_cell_grid_index_lookup[cell_index] = -1;

	sub_grid_cell_info->sg_cell_cell_area[cell_index] = C(-1.0);
	sub_grid_cell_info->sg_cell_dem[cell_index] = C(-1.0);

	sub_grid_cell_info->sg_cell_SGC_width[cell_index] = C(-1.0);
	sub_grid_cell_info->sg_cell_SGC_BankFullHeight[cell_index] = C(-1.0);
	sub_grid_cell_info->sg_cell_SGC_BankFullVolume[cell_index] = C(-1.0);
	sub_grid_cell_info->sg_cell_SGC_c[cell_index] = C(-1.0);

	sub_grid_cell_info->sg_cell_SGC_group[cell_index] = -1;
	sub_grid_cell_info->sg_cell_SGC_is_large[cell_index] = -1;
}

void AllocateWaterSource(int count, WaterSource * waterSource)
{
	waterSource->count = count;

	waterSource->Ident = (ESourceType*)memory_allocate(count * sizeof(ESourceType));
	waterSource->Val = (NUMERIC_TYPE*)memory_allocate(count * sizeof(NUMERIC_TYPE));
	waterSource->Name = (char**)malloc(count * sizeof(char*));
	for (int i = 0; i < count; ++i) {
		waterSource->Name[i] = (char*)malloc(80 * sizeof(char));
	}
	waterSource->timeSeries = (TimeSeries**)memory_allocate(count * sizeof(TimeSeries*));
	waterSource->Q_FP_old = (NUMERIC_TYPE*)memory_allocate(count * sizeof(NUMERIC_TYPE));
	waterSource->Q_SG_old = (NUMERIC_TYPE*)memory_allocate(count * sizeof(NUMERIC_TYPE));
	waterSource->g_friction_squared_FP = (NUMERIC_TYPE*)memory_allocate(count * sizeof(NUMERIC_TYPE));
	waterSource->g_friction_squared_SG = (NUMERIC_TYPE*)memory_allocate(count * sizeof(NUMERIC_TYPE));

	AllocateSubGridCellInfo(count, &waterSource->ws_cell);
}


void AllocateWeir(int count, WeirLayout * weirs)
{
	weirs->weir_index_qx = (int*)memory_allocate(count * sizeof(int));
	weirs->weir_index_qy = (int*)memory_allocate(count * sizeof(int));

	int weir_count = weirs->weir_count;

	weirs->Weir_Q_old_SG = (NUMERIC_TYPE*)memory_allocate(weir_count * sizeof(NUMERIC_TYPE));
	weirs->Weir_grid_index = (int*)memory_allocate(weir_count * sizeof(int));
	weirs->Weir_g_friction_sq = (NUMERIC_TYPE*)memory_allocate(weir_count * sizeof(NUMERIC_TYPE));

	weirs->Weir_pair_stream_flow_index = (int*)memory_allocate(2 * weir_count * sizeof(int));
	AllocateSubGridCellInfo(2 * weir_count, &weirs->cell_pair);
}


void AllocateRoutingDynamicList(int rows, int grid_cols_padded, RouteDynamicList * route_dynamic_list)
{
	route_dynamic_list->row_route_qx_count = (int*)memory_allocate(rows * sizeof(int));
	route_dynamic_list->row_route_qy_count = (int*)memory_allocate(rows * sizeof(int));
	route_dynamic_list->route_list_i_lookup_qx = (int*)memory_allocate(grid_cols_padded * rows * sizeof(int));
	route_dynamic_list->route_list_i_lookup_qy = (int*)memory_allocate(grid_cols_padded * rows * sizeof(int));
}

//-------------------lisflood.cpp-------------------------
static long total_allocated = 0;
static long total_legacy_allocated = 0;

NUMERIC_TYPE* memory_allocate_zero_numeric_legacy(size_t size)
{
	NUMERIC_TYPE* memory = new NUMERIC_TYPE[size]();

	if (memory == NULL)
	{
		printf("memory allocation failed %ldMB, legacy %ld, total %ld\n", total_allocated / 1024 / 1024, total_legacy_allocated / 1024 / 1024, (total_allocated + total_legacy_allocated) / 1024 / 1024);
		exit(-1);
	}
	total_legacy_allocated += size * sizeof(NUMERIC_TYPE);
	return memory;
}

NUMERIC_TYPE* memory_allocate_numeric_legacy(size_t size)
{
	NUMERIC_TYPE* memory = new NUMERIC_TYPE[size];

	if (memory == NULL)
	{
		printf("memory allocation failed %ldMB, legacy %ld, total %ld\n", total_allocated / 1024 / 1024, total_legacy_allocated / 1024 / 1024, (total_allocated + total_legacy_allocated) / 1024 / 1024);
		exit(-1);
	}
	total_legacy_allocated += size * sizeof(NUMERIC_TYPE);
	return memory;
}

void memory_free_legacy(int** memory)
{
	if (*memory != NULL)
	{
		total_legacy_allocated -= sizeof *memory;

		delete[] * memory;
		*memory = NULL;
	}
}

void memory_free_legacy(NUMERIC_TYPE** memory)
{
	if (*memory != NULL)
	{
		total_legacy_allocated -= sizeof *memory;

		delete[] * memory;
		*memory = NULL;
	}
}
///////******************xiaodw, 避免linux内存分配错误*****************
size_t align_up(size_t size, size_t alignment) {
	return (size + alignment - 1) & ~(alignment - 1);
}

void* memory_allocate_aligned(size_t size, size_t alignment)
{
	void* memory = NULL;

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	memory = _mm_malloc(size, alignment);  // Windows or Intel
#else
	size_t aligned_size = align_up(size, alignment);  // 自动补齐
	int result = posix_memalign(&memory, alignment, aligned_size);
	if (result != 0) memory = NULL;
#endif

	if (memory == NULL) {
		printf("memory allocation failed: requested %lu bytes (aligned to %lu)\n", size, alignment);
		exit(-1);
	}

	total_allocated += size;
	return memory;
}

void* memory_allocate(size_t size)
{
#if defined(_MSC_VER) || defined (__INTEL_COMPILER)
	void* memory = _mm_malloc(size, 64);
#else
	void* memory = NULL;
	posix_memalign(&memory, 64, size);
#endif
	if (memory == NULL)
	{
		printf("memory allocation failed %ldMB, legacy %ld, total %ld\n", total_allocated / 1024 / 1024, total_legacy_allocated / 1024 / 1024, (total_allocated + total_legacy_allocated) / 1024 / 1024);
		exit(-1);
	}
	total_allocated += size;
	return memory;
}

void memory_free(int** memory)
{
	memory_free((void**)memory);
}

void memory_free(NUMERIC_TYPE** memory)
{
	memory_free((void**)memory);
}

void memory_free(void** memory)
{
#if defined(_MSC_VER) || defined (__INTEL_COMPILER)
	_mm_free(*memory);
#else
	free(*memory);  // xiaodw, 正确释放 malloc 或 posix_memalign 分配的内存
	//posix_memalign((void**)&memory, sizeof(float) * 100* 128, 64);
#endif
	*memory = NULL;
}

void memory_free(void** memory, size_t size)
{
	memory_free(memory);
	total_allocated += size;
}



// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trimwhitespace(char *str)
{
	char *end;

	// Trim leading space
	while (isspace(*str)) str++;

	if (*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) end--;

	// Write new null terminator
	*(end + 1) = 0;

	return str;
}

void SetArrayValue(int* arr, int value, int length)
{
	for (int j = 0; j < length; j++)
	{
		arr[j] = value;
	}
}



void Fast_MainStart(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr,
	Stage *Locptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr,
	DamData *Damptr, LISFLOODFPContext* LFPContextPtr, SuperGridLinksList* Super_linksptr, LfpCouplingInfo * LfpCouplingInfoPtr)
{
	if (LFPContextPtr->verbose == ON)
	{
		printf("\nStarting time steps: ");
		fflush(stdout);
	}
	Solverptr->itrn_time_now = Solverptr->itrn_time;

	// Populating Tstep variables prior to start of simulation
	// 注意这里可以选择不同的时间步长策略
	if (Statesptr->adaptive_ts == ON)
	{
		if (Solverptr->t == 0)
		{
			Solverptr->Tstep = Solverptr->InitTstep;
			Solverptr->MinTstep = Solverptr->InitTstep;
		}
		if (LFPContextPtr->verbose == ON) printf("adaptive mode\n\n");
       		fflush(stdout);
	}
	else if (Statesptr->acceleration == ON)
	{
		if (Solverptr->t == 0)
		{
			Solverptr->Tstep = Solverptr->InitTstep;
			Solverptr->MinTstep = Solverptr->InitTstep;
		}
		if (LFPContextPtr->verbose == ON) printf("acceleration mode\n\n");
		fflush(stdout);
	}
	else if (Statesptr->Roe == ON)
	{
		if (Solverptr->t == 0)
		{
			Solverptr->Tstep = Solverptr->InitTstep;
			Solverptr->MinTstep = Solverptr->InitTstep;
		}
		if (LFPContextPtr->verbose == ON) printf("Roe mode\n\n");
		fflush(stdout);
	}
	else
	{
		if (Solverptr->t == 0)
		{
			Solverptr->Tstep = Solverptr->InitTstep;
			Solverptr->MinTstep = Solverptr->InitTstep;
		}
		if (LFPContextPtr->verbose == ON) printf("non-adaptive mode\n\n");
		fflush(stdout);
	}
	if (Statesptr->SGC == ON)
	{
		// because the SGC model calculates the time step in UpdateH rather than during calcFPflow it needs to initalise 
		// SGCtmpTstep, which would usually be calculated in UpdateH
		Solverptr->Tstep = Solverptr->InitTstep;
		CalcT(Parptr, Solverptr, Arrptr);
		Solverptr->SGCtmpTstep = Solverptr->Tstep;
		if (LFPContextPtr->verbose == ON) printf("SGC mode\n\n");
		fflush(stdout);
	}
	//	NUMERIC_TYPE init[16];
	//#pragma omp parallel for default(shared) schedule(static)
	//	for (int j = 0; j < 16; j++)
	//	{
	//		#pragma omp critical
	//		{
	//			init[j] = CBRT((NUMERIC_TYPE)j);
	//		}
	//
	//	}
	//	for (int j = 0; j < 16; j++)
	//	{
	//		printf("Init done: %" NUM_FMT"\n", init[j]);
	//	}
	//Fast_MainInit(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Locptr,
	//	ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr, DamData *Damptr, LISFLOODFPContext* LFPContextPtr)
	Fast_MainInit(Fnameptr, Fptr, Statesptr, Parptr, Solverptr, Poisptr, BCptr, Locptr, ChannelSegments, Arrptr, SGCptr, ChannelSegmentsVecPtr, Damptr, LFPContextPtr, Super_linksptr,LfpCouplingInfoPtr);
}


int LisFloodFP_Finilize(Solver *Solverptr, Arrays *Arrptr, Fnames *Fnameptr, Files* FpsPtr, States *Statesptr, Pars *Parptr, LISFLOODFPContext *LFPContext, char* tmpFileNamePtr) {

	gettimeofday(&(LFPContext->timstr), NULL);
	LFPContext->processing_end_time = LFPContext->timstr.tv_sec + (LFPContext->timstr.tv_usec / 1000000.0);
	double total_processing_time = (LFPContext->processing_end_time - LFPContext->processing_start_time) - LFPContext->total_write_time;

	printf("Elapsed processing time:\t\t\t%.6" NUM_FMT" (s) %ld\n", total_processing_time, Solverptr->itCount);
	printf("Output write time:\t\t\t%.6" NUM_FMT" (s) %d\n", LFPContext->total_write_time, Parptr->SaveNo);

	time_t loop_end;
	time(&loop_end);

	double seconds = difftime(loop_end, LFPContext->loop_start);
	printf("loop time %lf\n", seconds);

#if defined (__INTEL_COMPILER) && _PROFILE_MODE > 0
	__itt_pause();
#endif
#if _PROFILE_MODE > 1
	//force stop 
	exit(0);
#endif


	////output final output
	//WriteOutput(Fnameptr, LFPContext->grid_cols, LFPContext->grid_rows, LFPContext->grid_cols_padded,
	//	LFPContext->depth_thresh,
	//	LFPContext->tmp_grid1,
	//	LFPContext->initHtm_grid, LFPContext->totalHtm_grid, LFPContext->maxH_grid, LFPContext->maxHtm_grid,
	//	LFPContext->maxVc_grid, LFPContext->maxVc_height_grid, LFPContext->maxHazard_grid,
	//	LFPContext->Vx_max_grid, LFPContext->Vy_max_grid,
	//	LFPContext->dem_grid, LFPContext->SGC_BankFullHeight_grid,
	//	Statesptr, Parptr, &Statesptr->output_params);

#ifndef RESULT_CHECK

	memory_free_legacy(&Arrptr->Weir_Identx); //Weir_Identx converted to list, grid no longer needed
	memory_free_legacy(&Arrptr->Weir_Identy); //Weir_Identy converted to list, grid no longer needed

	// clean other memory that is no longer required after it has been copied/used
	memory_free_legacy(&Arrptr->FlowDir);
	memory_free_legacy(&Arrptr->RouteInt);
	memory_free_legacy(&Arrptr->H);
	//memory_free_legacy(& Arrptr->H);
	memory_free_legacy(&Arrptr->DEM);
	memory_free_legacy(&Arrptr->DEM);
	if (Arrptr->Manningsn != NULL)
		memory_free_legacy(&Arrptr->Manningsn);
	if (Arrptr->SGCManningsn != NULL)
		memory_free_legacy(&Arrptr->SGCManningsn);


	memory_free_legacy(&Arrptr->QxSGold);
	memory_free_legacy(&Arrptr->QySGold);

	memory_free_legacy(&Arrptr->SGCwidth);
	memory_free_legacy(&Arrptr->SGCz);
	memory_free_legacy(&Arrptr->SGCc);
	memory_free_legacy(&Arrptr->SGCbfH);
	memory_free_legacy(&Arrptr->SGCbfV);

	memory_free_legacy(&Arrptr->dx);
	memory_free_legacy(&Arrptr->dy);
	memory_free_legacy(&Arrptr->dA);

	// -----------------------xdw useless free memory --------------------
	//if (Statesptr->use_green_ampt_singlelayer == ON) {


	//}

	//if (Statesptr->use_interflow_singlelayer == ON) {

	//}

	//if (Statesptr->use_percolation_singlelayer == ON) {

	//	
	//}

	//if (Statesptr->use_percolation_multilayer == ON) {

	//}
#endif
	/*
	if (Statesptr->use_green_ampt_singlelayer == ON) {
		if (Parptr->capillarySuctionPD != NULL)
		{
			delete[] Parptr->capillarySuctionPD;
		}
		if (Parptr->accumuDepthPD != NULL)
		{
			delete[] Parptr->accumuDepthPD;
		}
		if (Parptr->soilMoisturePD != NULL)
		{
			delete[] Parptr->soilMoisturePD;
		}
		if (Parptr->infilPD != NULL)
		{
			delete[] Parptr->infilPD;
		}
		if (Parptr->infilCapacitySurplusPD != NULL)
		{
			delete[] Parptr->infilCapacitySurplusPD;
		}
		if (Parptr->ksPD != NULL)
		{
			delete[] Parptr->ksPD;
		}
		if (Parptr->initSoilMoisturePD != NULL)
		{
			delete[] Parptr->initSoilMoisturePD;
		}
		if (Parptr->porosityPD != NULL)
		{
			delete[] Parptr->porosityPD;
		}
		if (Parptr->clayPD != NULL)
		{
			delete[] Parptr->clayPD;
		}
		if (Parptr->sandPD != NULL)
		{
			delete[] Parptr->sandPD;
		}

		if (Parptr->rootDepthPD != NULL)
		{
			delete[] Parptr->rootDepthPD;
		}
		if (Parptr->soilWaterDepth != NULL)
		{
			delete[] Parptr->soilWaterDepth;
		}

		if (Parptr->capillarySuction != NULL)
		{
			delete[] Parptr->capillarySuction;
		}
		if (Parptr->accumuDepth != NULL)
		{
			delete[] Parptr->accumuDepth;
		}
		if (Parptr->infil != NULL)
		{
			delete[] Parptr->infil;
		}
		if (Parptr->ks != NULL)
		{
			delete[] Parptr->ks;
		}

		if (Parptr->infilCapacitySurplus != NULL)
		{
			delete[] Parptr->infilCapacitySurplus;
		}
		if (Parptr->initSoilMoisture != NULL)
		{
			delete[] Parptr->initSoilMoisture;
		}
		if (Parptr->porosity != NULL)
		{
			delete[] Parptr->porosity;
		}
		if (Parptr->clay != NULL)
		{
			delete[] Parptr->clay;
		}
		if (Parptr->sand != NULL)
		{
			delete[] Parptr->sand;
		}
		if (Parptr->rootDepth != NULL)
		{
			delete[] Parptr->rootDepth;
		}
	}
	if (Statesptr->use_percolation_singlelayer == ON) {
		if (Parptr->fieldCapacityPD != NULL)
		{
			delete[] Parptr->fieldCapacityPD;
		}
		if (Parptr->poreIndexPD != NULL)
		{
			delete[] Parptr->poreIndexPD;
		}
		if (Parptr->rechargePD != NULL)
		{
			delete[] Parptr->rechargePD;
		}
		if (Parptr->percolationPD != NULL)
		{
			delete[] Parptr->percolationPD;
		}
		if (Parptr->gwStoragePD != NULL)
		{
			delete[] Parptr->gwStoragePD;
		}
		if (Parptr->gndQ2RchPD != NULL)
		{
			delete[] Parptr->gndQ2RchPD;
		}

		if (Parptr->fieldCapacity != NULL)
		{
			delete[] Parptr->fieldCapacity;
		}
		if (Parptr->poreIndex != NULL)
		{
			delete[] Parptr->poreIndex;
		}
	}
	if (Statesptr->use_interflow_singlelayer == ON) {
		if (Parptr->interflowGenVolPD != NULL)
		{
			delete[] Parptr->interflowGenVolPD;
		}
		if (Parptr->interflow2ChVolPD != NULL)
		{
			delete[] Parptr->interflow2ChVolPD;
		}
		if (Parptr->interflowRunoffVolPD != NULL)
		{
			delete[] Parptr->interflowRunoffVolPD;
		}
		if (Parptr->slopePD != NULL)
		{
			delete[] Parptr->slopePD;
		}

		if (Parptr->slope != NULL)
		{
			delete[] Parptr->slope;
		}
	}
	if (Statesptr->use_percolation_multilayer == ON) {
		for (int i = 0; i < Parptr->multi_nSoilLyrs; i++)
		{
			delete[] Parptr->multi_soilPoreIndex[i];
			delete[] Parptr->multi_soilFc[i];
			delete[] Parptr->multi_soilPorosity[i];
			delete[] Parptr->multi_soilKs[i];
			delete[] Parptr->multi_soilThickness[i];
			delete[] Parptr->multi_soilInitMoisture[i];
		}
		delete[] Parptr->multi_soilPoreIndex;
		delete[] Parptr->multi_soilFc;
		delete[] Parptr->multi_soilPorosity;
		delete[] Parptr->multi_soilKs;
		delete[] Parptr->multi_soilThickness;
		delete[] Parptr->multi_soilInitMoisture;

		for (int i = 0; i < Parptr->multi_nSoilLyrs; i++)
		{
			delete[] Parptr->multi_soilPoreIndexPD[i];
			delete[] Parptr->multi_soilFcPD[i];
			delete[] Parptr->multi_soilPorosityPD[i];
			delete[] Parptr->multi_soilKsPD[i];
			delete[] Parptr->multi_soilThicknessPD[i];
			delete[] Parptr->multi_soilInitMoisturePD[i];

			delete[] Parptr->multi_soilPercoPD[i];
			delete[] Parptr->multi_soilWaterDepthPD[i];
			delete[] Parptr->multi_soilMoisturePD[i];
			delete[] Parptr->multi_soilInitMoisturePD[i];
			delete[] Parptr->multi_soilInitMoisturePD[i];
			delete[] Parptr->multi_soilInitMoisturePD[i];

		}
		delete[] Parptr->multi_soilPoreIndexPD;
		delete[] Parptr->multi_soilFcPD;
		delete[] Parptr->multi_soilPorosityPD;
		delete[] Parptr->multi_soilKsPD;
		delete[] Parptr->multi_soilThicknessPD;
		delete[] Parptr->multi_soilInitMoisturePD;

		delete[] Parptr->multi_soilPercoPD;
		delete[] Parptr->multi_soilWaterDepthPD;
		delete[] Parptr->multi_soilMoisturePD;
		delete[] Parptr->multi_soilInitMoisturePD;
		delete[] Parptr->multi_soilInitMoisturePD;
		delete[] Parptr->multi_soilInitMoisturePD;

		delete[] Parptr->multi_ksFactorVOfLyr;
		delete[] Parptr->multi_soilWtrStoPrfl;
		delete[] Parptr->multi_soilPoreIndexOfLyr;

	}
	if (Statesptr->use_interflow_multilayer == ON) {
		for (int i = 0; i < Parptr->multi_nSoilLyrs; i++)
		{
			delete[] Parptr->multi_interflowGenVolPD[i];
			delete[] Parptr->multi_interflow2ChVolPD[i];
			delete[] Parptr->multi_interflowRunoffVolPD[i];
		}
		delete[] Parptr->multi_interflowGenVolPD;
		delete[] Parptr->multi_interflow2ChVolPD;
		delete[] Parptr->multi_interflowRunoffVolPD;
		delete[] Parptr->multi_interflowCsValueOfLyr;
	}
	*/
	if (LFPContext->verbose == ON) printf("Finished.\n\n");

	time(&(Solverptr->time_finish));

	// get system time and echo for user
	if (LFPContext->verbose == ON) {
		time_t tf = time(0);
		tm timeF = *localtime(&tf);
		printf("\nFinish Date: %d/%d/%d \n", timeF.tm_mday, timeF.tm_mon + 1, timeF.tm_year + 1900);
		printf("Finish Time: %d:%d:%d \n\n", timeF.tm_hour, timeF.tm_min, timeF.tm_sec);
	}

	//iteration time
	Solverptr->itrn_time = Solverptr->itrn_time + (NUMERIC_TYPE)difftime(Solverptr->time_finish, Solverptr->time_start);
	if (LFPContext->verbose == ON) printf("\n  Total computation time: %.2" NUM_FMT" mins\n\n", (Solverptr->itrn_time / C(60.0)));

	if (Statesptr->logfile == ON)
	{
		freopen("CON", "w", stdout);
		printf("\nLisflood run finished see log file for run details");
	}

	if (Statesptr->save_stages == ON) fclose(FpsPtr->stage_fp);
	sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".stage");
	//if (Statesptr->call_gzip == ON) {
	//	sprintf(tmpSysCmdPtr, "%s%s", "gzip -9 -f ", *tmpFileNamePtr);
	//	system(tmpSysCmdPtr);
	//}

	if (FpsPtr->mass_fp != NULL) fclose(FpsPtr->mass_fp);
	sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".mass");
	//if (Statesptr->call_gzip == ON) {
	//	sprintf(tmpSysCmdPtr, "%s%s", "gzip -9 -f ", *tmpFileNamePtr);
	//	system(tmpSysCmdPtr);
	//}
	// FEOL Dam output
	if (Statesptr->DamMode == ON)
	{
		if (FpsPtr->dam_fp != NULL) fclose(FpsPtr->dam_fp);
		sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".dam");
	}
	return 1;
}

int LisFloodFP_Initilize(int argc, char *argv[], Arrays *Arrptr, Files* FpsPtr, Fnames *Fnameptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Stageptr, SGCprams *SGCptr, DamData *Damptr,
	vector<ChannelSegmentType> *ChannelSegmentsVecPtr, LISFLOODFPContext* LFPContextPtr, LfpCouplingInfo * LfpCouplingInfoPtr, SuperGridLinksList *Super_linksptr, char* tmpFileNamePtr, char* tmpSysCmdPtr)
{
	int i, chseg;
	FILE *tmp_fp;
	//char t1[255];
	NUMERIC_TYPE tmp;
	//char tmp_sys_com[255]; // temporary string to hold system command

	//Instances of Vectors
	vector<ChannelSegmentType> ChannelSegments; // CCS Contains the channel information for ALL rivers (indexed using RiversIndex vector below).
	//vector<ChannelSegmentType> *ChannelSegmentsVecPtr; // CCS
	ChannelSegmentsVecPtr = &ChannelSegments; // CCS

	vector<QID7_Store> QID7; //#CCS A temporary store for some ChannelSegments variables that cannot be written to the correct location during LoadRiver().
	vector<QID7_Store> *QID7_Vec_Ptr; // CCS
	QID7_Vec_Ptr = &QID7; // CCS

	vector<int> RiversIndex; // CCS Contains index values for ChannelSegments so we know where one river finishes and the next starts.
	vector<int> *RiversIndexVecPtr; // CCS
	RiversIndexVecPtr = &RiversIndex; // CCS

	// Define initial value for common simulation states (eg. verbose)
	Statesptr->output_params = OutputParams();

	// Define initial value for parameters
	Parptr->dx = C(10.0);
	Parptr->dy = C(10.0);
	Parptr->dA = C(100.0);
	Parptr->tlx = C(0.0);
	Parptr->tly = C(0.0);
	Parptr->blx = C(0.0);
	Parptr->bly = C(0.0);
	Parptr->FPn = C(0.06);
	Parptr->SaveInt = C(1000.0);
	Parptr->SaveTotal = C(0.0);
	Parptr->MassInt = C(100.0);
	Parptr->MassTotal = C(0.0);
	Parptr->SaveNo = 0;
	Parptr->op = C(100.0);
	Parptr->InfilLoss = C(0.0);
	//Parptr->InterflowGenLoss = C(0.0);
	Parptr->EvapLoss = C(0.0);
	Parptr->RainLoss = C(0.0);
	Parptr->InfilTotalLoss = C(0.0);
	Parptr->InterflowGenTotal = C(0.0);
	Parptr->InterflowRunoffTotal = C(0.0);
	Parptr->Interflow2ChTotal = C(0.0);
	Parptr->InterflowGen = C(0.0);
	Parptr->InterflowRunoff = C(0.0);
	Parptr->Interflow2Ch = C(0.0);
	Parptr->EvapTotalLoss = C(0.0);
	Parptr->RainTotalLoss = C(0.0);
	Parptr->checkfreq = CHKINTERVAL;  // set default checkpointing file output interval
	Parptr->nextcheck = C(0.0);
	Parptr->reset_timeinit_time = 0;
	Parptr->op_multinum = 0; // default to zero or can cause problems with checkpointing if multipass not used
	Parptr->ch_start_h = C(2.0); // default start water depth for channel
	Parptr->steadyQtol = C(0.0005); // tolerance for steady-state definition
	Parptr->steadyInt = C(1800.0); // interval at which to assess steady-state
	Parptr->steadyTotal = C(0.0);
	Parptr->SGC_p = C(0.78); // default for sub grid channel exponent
	Parptr->SGC_r = C(0.12); // default for sub grid channel multiplier (British rivers average Hey and Thorne (1986))
	Parptr->SGCchan_type = 1; // defines the type of channel used by the sub_grid model, default is rectangular channel.
	Parptr->SGC_s = C(2.0); // sub-grid channel parameter used for some of the channel types, parabolic channel default.
	Parptr->SGC_2 = C(0.0); // sub-grid channel parameter used for some of the channel types, meaningless default.
	Parptr->SGC_n = C(0.035); // sub-grid channel parameter used for some of the channel types, meaningless default.
	// xdw add, for saturation excess infiltration(m)
	Parptr->saturation_value = C(0.3);
	// xdw add, for snow and glacier melt
	Parptr->melt_temperature = C(0.0); // melt temperature threshold
	Parptr->FddSnow = C(0.0000578); // degree day for 1 second, 5/24/3600 (1~10), mm
	Parptr->FddGlacier = C(0.0001156); // degree day for 1 second, 5*2/24/3600 (1~10), mm
	Parptr->Frr = C(0.3); // correct factor to simulate liquid water refreezing, while temperature is below threshold.(0~1) 
	// xdw add, for groundwater percolation and recharge to river
	//Parptr->Percolation = C(0.0);
	//Parptr->GroundWaterQ2RiverTotal = C(0.0);
	//Parptr->GroundWaterStorageTotal = C(0.0);
	// xdw add, for greenampt
	Parptr->InfilRateGA = C(0.0);
	Parptr->AccumuDepthGA = C(0.0);
	Parptr->interflow_lagindex = C(0.0);
	// xdw add, for multilayer percolation
	Parptr->multi_curSoilLyr = C(0.0);
	Parptr->multi_nSoilLyrs = C(0.0);
	Parptr->multi_curSoilLyr = C(0.0);



	Parptr->Routing_Speed = C(0.1); // CCS default routing speed for shallow rainfall flows C(0.1) m/s
	Parptr->RouteInt = C(0.0); // CCS will be reasigned when FlowDirDEM function is called
	Parptr->RouteSfThresh = C(0.1); // CCS water surface slope at which routing takes over from shallow water equations when routing is enabled.
	Parptr->SGC_m = 1;  // JCN meander coefficient for sub-grid model.
	Parptr->SGC_a = -1; // JCN upstream area for sub-grid model.
	Parptr->min_dx = C(10.0); // CCS Holds min_dx value (needed for variable dimension lat-long grids)
	Parptr->min_dy = C(10.0); // CCS Holds min_dy value (needed for variable dimension lat-long grids)
	Parptr->min_dx_dy = C(10.0); // CCS Holds min of min_dx and min_dy values (needed for variable dimension lat-long grids)
	Parptr->max_Froude = C(10000.0); // maximum Froude allowed in model, set way higher than will ever occure by default (JCN)
	Parptr->maxint = C(99999999999.0); // maxinterval save time
	Parptr->maxintTotal = C(0.0); // maxinterval save time
	Parptr->maxintcount = 0; // number of max ints
	Parptr->limit_slopes = OFF;
	Parptr->output_precision = 3;
	Parptr->nodata_elevation = DEM_NO_DATA;
	Parptr->drain_nodata = OFF;
	//-----------------------------multi-layer perc, interflow
	Parptr->multi_nSoilLyrs = 0;


	// Define initial values for boundary conditions
	BCptr->Qin = C(0.0);
	BCptr->Qout = C(0.0);
	BCptr->VolInMT = C(0.0);
	BCptr->VolOutMT = C(0.0);

	// Define initial values for arrays
	Arrptr->Manningsn = NULL;
	Arrptr->SGCManningsn = NULL;

	// Define initial values for solver settings
	Solverptr->Sim_Time = C(3600.0);
	Solverptr->InitTstep = C(10.0);		// Maximum timestep
	Solverptr->Nit = 360;
	Solverptr->itCount = 0;
	Solverptr->t = C(0.0);
	Solverptr->g = C(9.8065500000000);
	Solverptr->divg = (1 / (2 * Solverptr->g));
	Solverptr->cfl = C(0.7);
	Solverptr->SolverAccuracy = C(1e-4);
	Solverptr->dynsw = 0; // Switch for full dynamic steady state (1) or diffusive steady state (0)
	Solverptr->DepthThresh = C(1e-3);
	Solverptr->MomentumThresh = C(1e-2);
	Solverptr->MaxHflow = C(10.0);
	Solverptr->Hds = C(0.0);
	Solverptr->Qerror = C(0.0);
	Solverptr->Verror = C(0.0);
	Solverptr->dhlin = C(0.01);
	Solverptr->htol = C(1.0);
	Solverptr->Qlimfact = C(1.0);
	Solverptr->itrn_time = C(0.0);
	Solverptr->itrn_time_now = C(0.0);
	Solverptr->ts_multiple = 1;  // default to x1 timestep decouple multiple
	Solverptr->SGCtmpTstep = 1; // JCN any number 
	Solverptr->theta = C(1.0); // GAMA (for q-centred numerical scheme), C(1.0)= semi-implicit version (Bates et al 2010);
	Solverptr->fricSolver2D = ON; //GAMA: uses the 2D friction scheme as default
	Solverptr->maxH = C(0.0);
	Solverptr->krivodonova_threshold = C(10.0);
	Solverptr->SpeedThresh = C(1e-6);
	Solverptr->epsilon = C(0.001); // adaptation
	Solverptr->L = 0; // adaptation

	// Define default values for SimStates instance of States
	Statesptr->diffusive = OFF;	// CCS added default state
	Statesptr->ChannelPresent = OFF;
	Statesptr->TribsPresent = ON;
	Statesptr->NCFS = ON;
	Statesptr->save_depth = ON;
	Statesptr->save_glacier_thickness = OFF;
	Statesptr->save_snow_thickness = OFF;
	Statesptr->save_elev = ON;
	Statesptr->save_vtk = ON;
	Statesptr->single_op = OFF;
	Statesptr->multi_op = OFF;
	Statesptr->calc_area = OFF;
	Statesptr->calc_meandepth = OFF;
	Statesptr->calc_volume = OFF;
	Statesptr->save_stages = OFF;
	Statesptr->adaptive_ts = ON;
	Statesptr->qlim = OFF; //TJF: Switch for qlim version, default is OFF
	Statesptr->acceleration = OFF; //PB: Switch for acceleration version, default is OFF
	Statesptr->debugmode = OFF;
	Statesptr->save_Qs = OFF;
	Statesptr->calc_infiltration = OFF;
	Statesptr->calc_distributed_infiltration = OFF;
	// xdw add, for green-ampt infiltration
	Statesptr->use_green_ampt_singlelayer = OFF;
	Statesptr->use_green_ampt_multilayer = OFF;
	// xdw add, for singlelayer percolation and interflow
	Statesptr->use_interflow_singlelayer = OFF;
	// xdw add, for multilayer percolation and interflow
	Statesptr->use_interflow_multilayer = OFF;
	Statesptr->use_percolation_multilayer = OFF;
	Statesptr->multi_soilPoreIndexFile = OFF;
	Statesptr->multi_soilPoreIndexOfLyr = OFF;


	Statesptr->call_gzip = OFF;
	Statesptr->alt_ascheader = OFF;
	Statesptr->checkpoint = OFF;
	Statesptr->checkfile = OFF;
	Statesptr->calc_evap = OFF;
	Statesptr->rainfall = OFF;
	Statesptr->use_temperature = OFF;
	Statesptr->rainfallmask = OFF;
	//Statesptr->use_distributed_rain = OFF;
	Statesptr->routing = OFF; //CCS: Switch for rainfall routing routine 
	Statesptr->reset_timeinit = OFF;
	Statesptr->profileoutput = OFF;
	Statesptr->porosity = OFF;
	Statesptr->weirs = OFF;
	Statesptr->save_Ts = OFF;
	Statesptr->save_QLs = OFF;
	Statesptr->startq = OFF;
	Statesptr->logfile = OFF;
	Statesptr->startfile = OFF;
	Statesptr->start_ch_h = OFF;
	Statesptr->comp_out = OFF;
	Statesptr->chainagecalc = ON;
	Statesptr->mint_hk = OFF;
	Statesptr->Roe = OFF;
	Statesptr->killsim = OFF;
	Statesptr->dhoverw = OFF;
	Statesptr->drychecking = ON;
	Statesptr->voutput = OFF;
	Statesptr->steadycheck = OFF;
	Statesptr->hazard = OFF;
	Statesptr->startq2d = OFF;
	Statesptr->Roe_slow = OFF;
	Statesptr->multiplerivers = OFF;
	Statesptr->SGC = OFF;
	Statesptr->SGCbed = OFF;
	Statesptr->SGClevee = OFF;
	Statesptr->SGCcat_area = OFF;
	Statesptr->SGCchangroup = OFF;
	Statesptr->SGCchanprams = OFF;
	Statesptr->SGCbfh_mode = OFF;
	Statesptr->SGCA_mode = OFF;
	Statesptr->binary_out = OFF;
	Statesptr->gsection = OFF;
	Statesptr->binarystartfile = OFF;
	Statesptr->startelev = OFF;
	Statesptr->latlong = OFF;
	Statesptr->dist_routing = OFF;
	Statesptr->SGCvoutput = OFF; // switch for sub-grid channel velocity output
	Statesptr->DamMode = OFF;
	Statesptr->DammaskRead = OFF;
	Statesptr->saveint_max = OFF;
	Statesptr->maxint = OFF;
	Statesptr->ChanMaskRead = OFF;
	Statesptr->LinkListRead = OFF;
	Statesptr->cuda = OFF;
	Statesptr->fv1 = OFF;
	Statesptr->fv2 = OFF;
	Statesptr->acc_nugrid = OFF;
	Statesptr->dg2 = OFF;
	Statesptr->dynamicrainfall = OFF;
	Statesptr->use_snow_glacier = OFF;
	Statesptr->use_temperature = OFF;
	Statesptr->save_in_tif = OFF;
	Statesptr->save_in_jpg = OFF;


	SGCptr->NSGCprams = 0;
	SGCptr->SGCbetahmin = C(0.2);

	/*default resrootname*/
	strcpy(Fnameptr->res_dirname, "");
	strcpy(Fnameptr->res_prefix, "res");

	int verbosemode = ReadVerboseMode(argc, argv);
	LFPContextPtr->verbose = verbosemode;
	printversion(verbosemode);

	ReadConfiguration(argc, argv, Fnameptr, Statesptr, Parptr, Solverptr,
		verbosemode);

	// use output folder if requested in parameter file or command line
	if (strlen(Fnameptr->res_dirname) > 0)
	{
		if (fexist(Fnameptr->res_dirname) == 0) // check if it doesn't exist
		{
			//create output folder
			sprintf(tmpSysCmdPtr, "%s%s", "mkdir ", Fnameptr->res_dirname);
			system(tmpSysCmdPtr);
		}
		//set the resroot to include the folder information
		sprintf(Fnameptr->resrootname, "%s" FILE_SEP"%s", Fnameptr->res_dirname, Fnameptr->res_prefix);
	}
	else
	{
		//set to res_prefix
		sprintf(Fnameptr->resrootname, "%s", Fnameptr->res_prefix);
	}

	// (MT) redirect all console output to logfile if requested
	if (Statesptr->logfile == ON)
	{
		sprintf(Fnameptr->logfilename, "%s%s", Fnameptr->resrootname, ".log");  //Default log filename
		printf("Redirecting all console output to %s\n\n", Fnameptr->logfilename);
		printf("Lisflood is running ......\n");
		freopen(Fnameptr->logfilename, "w", stdout); // redirect stdout to log file
		setvbuf(stdout, NULL, _IONBF, 0); // set buffer to zero so log file is always up to date
		printversion(verbosemode); // output version here as well (so we get on screen and in file)
	}


	if (strlen(Fnameptr->checkpointfilename) == 0)
		sprintf(Fnameptr->checkpointfilename, "%s%s", Fnameptr->resrootname, ".chkpnt");  //Default checkpoint filename

	if (Statesptr->checkpoint == ON && verbosemode == ON)
		printf("Running in checkpointing mode: frequency %" NUM_FMT" hours\n", Parptr->checkfreq);

	if (Statesptr->steadycheck == ON) {
		//for(i=1;i<argc-1;i++) if(!strcmp(argv[i],"-steadytol")) sscanf(argv[i+1],"%" NUM_FMT"",&Parptr->steadyQtol); // optional tolerance
		if (verbosemode == ON) printf("\nWARNING: simulation will stop on steady-state (tolerance: %.6" NUM_FMT"), or after %.1" NUM_FMT"s.\n", Parptr->steadyQtol, Solverptr->Sim_Time);
	}

	//code to load in alternative ASCII header for output files
	if (Statesptr->alt_ascheader == ON) {
		Parptr->ascheader = new char*[6];//6 lines in the file
		tmp_fp = fopen(Fnameptr->ascheaderfilename, "r");
		for (i = 0; i < 6; i++) {
			Parptr->ascheader[i] = new char[256];//255 characters per line
			fgets(Parptr->ascheader[i], 255, tmp_fp);
		}
		if (verbosemode == ON) printf("Using alternative ASCII header for output\n");
		fclose(tmp_fp);
	}


	// get system time and echo for user
	if (verbosemode == ON) {
		time_t ts = time(0);
		tm timeS = *localtime(&ts);
		printf("\nStart Date: %d/%d/%d \n", timeS.tm_mday, timeS.tm_mon + 1, timeS.tm_year + 1900);
		printf("Start Time: %d:%d:%d \n\n", timeS.tm_hour, timeS.tm_min, timeS.tm_sec);
	}



	LoadDEM(Fnameptr, Statesptr, Parptr, Arrptr, verbosemode);

#ifdef CUDA
	if (Statesptr->cuda == ON)
	{
		if (Statesptr->fv1 == ON)
		{
			lis::cuda::fv1::Simulation simulation;
			simulation.run(ParFp, SimStates, Params, ParSolver, verbosemode);
		}
		else if (Statesptr->fv2 == ON)
		{
			lis::cuda::fv2::Simulation simulation;
			simulation.run(ParFp, SimStates, Params, ParSolver, verbosemode);
		}
		else if (Statesptr->dg2 == ON)
		{
			lis::cuda::dg2::Simulation simulation;
			simulation.run(ParFp, SimStates, Params, ParSolver, verbosemode);
		}
		else if (Statesptr->acceleration == ON)
		{
			lis::cuda::acc::Simulation simulation;
			simulation.run(ParFp, SimStates, Params, ParSolver, verbosemode);
		}
		else if (Statesptr->acc_nugrid == ON)
		{
			lis::cuda::acc_nugrid::Simulation simulation;
			simulation.run(ParFp, SimStates, Params, ParSolver, verbosemode);
		}
		else
		{
			fprintf(stderr, "cuda only available for acc, fv1, fv2 and dg2 solvers\n");
			return -1;
		}
		return 0;
	}
#endif

	// Dammask needs to be read after LoadDEM and before SGC FEOL
	if (Statesptr->DamMode == ON)LoadDamPrams(Fnameptr, Statesptr, Parptr, Damptr, verbosemode); //FEOL
	Damptr->DamLoss = C(0.0); // To ensure dam loss is zero if no dams for mass balance! FEOL
	if (Statesptr->DammaskRead == ON)LoadDamMask(Fnameptr, Parptr, Arrptr, Damptr, verbosemode);
	CalcArrayDims(Statesptr, Parptr, Arrptr); // CCS populates dx, dy and dA arrays (calcs correct dimensions if using lat long grid) 

	// dhlin value calculated "on the fly" as a function of dx and gradient (C(0.0002)) from Cunge et al. 1980
	if (Statesptr->dhoverw == OFF) Solverptr->dhlin = Parptr->dx*C(0.0002);

	LoadRiverNetwork(Fnameptr, Statesptr, Parptr, ChannelSegmentsVecPtr, Arrptr, QID7_Vec_Ptr, RiversIndexVecPtr, verbosemode); // CCS
	if (Statesptr->ChannelPresent == OFF) ChannelSegments.resize(1); // temp fix to prevent visual studio debuger exiting on the next line (JCN)

	ChannelSegmentType *CSTypePtr = &ChannelSegments[0]; // CCS has to be defined after LoadRiverNetwork has completed.
	int *RiversIndexPtr = &RiversIndex[0];  // CCS has to be defined after LoadRiverNetwork has completed.

	if (QID7.size() != 0) // CCS If there are any tribs then we need to copy the terms from the temp store to the correct place.
	{
		QID7_Store *QID7Ptr = &QID7[0]; // CCS
		UpdateChannelsVector(Statesptr, CSTypePtr, QID7_Vec_Ptr, QID7Ptr, RiversIndexPtr); // CCS
	}

	//override river file friction if specified on command line
	for (i = 1; i < argc - 1; i++) if (!STRCMPi(argv[i], "-nch")) {
		sscanf(argv[i + 1], "%" NUM_FMT"", &tmp);
		if (verbosemode == ON) printf("Channel friction reset by command line: %" NUM_FMT"\n\n", tmp);
		for (chseg = 0; chseg < CSTypePtr->N_Channel_Segments; chseg++) for (i = 0; i < CSTypePtr[chseg].chsz; i++) CSTypePtr[chseg].ChanN[i] = tmp;
	}
	if (Statesptr->ChannelPresent == ON) SmoothBanks(Parptr, Solverptr, CSTypePtr, Arrptr, ChannelSegmentsVecPtr, verbosemode);

	if (Statesptr->SGC == ON) LoadSGC(Fnameptr, Parptr, Arrptr, Statesptr, verbosemode); // load sub grid channels
	if (Statesptr->SGC == ON && Statesptr->SGCchanprams == ON) LoadSGCChanPrams(Fnameptr, Statesptr, Parptr, SGCptr, verbosemode); // This loads the parameters for the SGC group information
	// 计算河道底面积、底面高程和河道单元体积
	if (Statesptr->SGC == ON) CalcSGCz(Fnameptr, Statesptr, Parptr, Arrptr, SGCptr, verbosemode);

	if (Statesptr->startfile == ON)
	{
		LoadStart(Fnameptr, Statesptr, Parptr, Arrptr, SGCptr, verbosemode);
		if (Statesptr->startq2d == ON)
		{
			LoadStartQ2D(Fnameptr, Parptr, Arrptr, verbosemode);
		}
	}
	if (Statesptr->binarystartfile == ON) LoadBinaryStart(Fnameptr, Statesptr, Parptr, Arrptr, SGCptr, verbosemode);

	LoadBCs_SEIMS(Fnameptr, Statesptr, Parptr, BCptr, verbosemode);


	LoadBCs(Fnameptr, Statesptr, Parptr, BCptr, verbosemode);



	LoadPOIs(Fnameptr, Statesptr, Parptr, Poisptr, verbosemode);
	LoadBCVar(Fnameptr, Statesptr, Parptr, BCptr, CSTypePtr, Arrptr, ChannelSegmentsVecPtr, verbosemode);
	LoadManningsn(Fnameptr, Parptr, Arrptr, verbosemode);
	LoadDistInfil(Fnameptr, Parptr, Arrptr, verbosemode);
	LoadSGCManningsn(Fnameptr, Parptr, Arrptr, verbosemode);
	// PFU add SGC dirn array
	LoadSGCdirn(Fnameptr, Parptr, Arrptr, verbosemode);
	LoadPor(Fnameptr, Statesptr, Parptr, Arrptr, verbosemode);
	LoadWeir(Fnameptr, Statesptr, Parptr, Arrptr, verbosemode);
	if (Statesptr->calc_evap == ON) LoadEvap(Fnameptr, Arrptr, verbosemode);
	if (Statesptr->rainfall == ON) {
		if (strlen(Fnameptr->rainfilename) != 0)
		{
			LoadRain(Fnameptr, Arrptr, verbosemode);
		}
		else if (strlen(Fnameptr->rain_csvfilename) != 0)
		{
			// rainfall value is in column 9
			LoadDataByCSV(Fnameptr->rain_csvfilename, &Arrptr->rain, verbosemode, 9);

			// convert rainfall rate from mm/hr to m/second
			for (i = 0; i < Arrptr->rain->count; i++)
				Arrptr->rain->value[i] /= (1000 * 3600);
		}
		else
		{
			printf("You have to specify rainfall or rainfall_csv in par file before using rainfall\n");
			exit(0);
		}
	}
	// xdw add, support temperature for glacier and snow melt
	if (Statesptr->use_temperature == ON) {
		if (strlen(Fnameptr->temperatureFile) != 0)
		{
			LoadTimeVaringTemperature(Fnameptr, Arrptr, verbosemode);
		}
		else if (strlen(Fnameptr->temperatureCsvFile) != 0)
		{
			// rainfall value is in column 9
			LoadDataByCSV(Fnameptr->temperatureCsvFile, &Arrptr->temperature, verbosemode, 2);
			for (int i = 0; i < 100; i++)
			{
				cout << Arrptr->temperature->time[i] << " " << Arrptr->temperature->value[i] << endl;
			}

		}
		else
		{
			printf("You have to specify temperature_file or temperature_csv in par file before using rainfall\n");
			exit(0);
		}
	}
	if (Statesptr->use_snow_glacier == ON) loadGlacierSnowProperties(Fnameptr, Parptr, verbosemode);

	if (Statesptr->rainfallmask == ON) LoadRainmask(Fnameptr, Parptr, Arrptr, Statesptr, verbosemode);
	if (Statesptr->save_stages == ON) LoadStages(Fnameptr, Statesptr, Parptr, Stageptr, verbosemode);
	if (Statesptr->gsection == ON) LoadGauges(Fnameptr, Statesptr, Parptr, Stageptr, verbosemode);


	//FEOL note this modifies the DEM! Changes DEM to DEM_NO_DATA where mask is negative
	if (Statesptr->routing == ON) // Call FlowDirDEM to generate flow direction map from DEM before main loop CCS
	{
		FlowDirDEM(Parptr, Arrptr, Statesptr, BCptr);
		if (verbosemode == ON) printf("Flow direction map generated from DEM\n\n");
	}

	// apply different starting methods for channel
	if (Statesptr->ChannelPresent == ON)
	{
		// calc initial steady state flows down channel
		CalcChannelStartQ(Statesptr, Parptr, Arrptr, CSTypePtr, RiversIndexVecPtr, RiversIndexPtr);

		if (Statesptr->startfile == ON)
		{
			// start file is specified. Do nothing, as starting H values for channel already read in from the startfile.
		}
		else if (Statesptr->startq == ON)
		{
			// Kinematic: Uses the kinematic initial solution to calculate H from Q
			// Diffusive: Uses diffusive steady state initial solution (default) or can use full dynamic steady state
			// initial if turned on using -dynsw on command line or "ch_dynamic" in the parameter file

			// use the flows to calculate a starting H
			SetChannelStartHfromQ(Statesptr, Parptr, Arrptr, CSTypePtr, Solverptr, RiversIndexVecPtr, RiversIndexPtr);
		}
		else
		{
			// set channel start H to default or user defined H
			SetChannelStartH(Statesptr, Parptr, Arrptr, CSTypePtr, RiversIndexVecPtr, RiversIndexPtr);
		}
	}
	// apply hot starting methods to SGC model
	if (Statesptr->startq == ON && Statesptr->SGC == ON)
	{
		SGC_hotstart(Statesptr, Parptr, Solverptr, Arrptr);
		if (verbosemode == ON) printf("\nStartq for SGC model implemented\n");
	}

	if (verbosemode == ON) if (Statesptr->calc_infiltration == ON) printf("Floodplain infiltration set at: %.10" NUM_FMT" ms-1\n\n", Parptr->InfilRate);

	//get multiple overpass timings from file
	if (Statesptr->multi_op == ON) {
		tmp_fp = fopen(Fnameptr->opfilename, "r");
		if (tmp_fp != NULL)
		{
			fscanf(tmp_fp, "%i", &Parptr->op_multinum);
			if (verbosemode == ON) printf("\nMultiple overpass files to be output: %d\n", Parptr->op_multinum);
			Parptr->op_multisteps = memory_allocate_numeric_legacy(Parptr->op_multinum);
			Parptr->op_multiswitch = new int[Parptr->op_multinum];
			for (i = 0; i < Parptr->op_multinum; i++) {
				if (fscanf(tmp_fp, "%" NUM_FMT"", &Parptr->op_multisteps[i]) != 1) // read in value and check if one value read in successfully
				{
					printf("\nWARNING: overpass file read error at line %i\n", i + 1);
					Parptr->op_multinum = i; // reset to number of values actually read in
					break;
				}
				Parptr->op_multiswitch[i] = 0;
				if (verbosemode == ON) printf("Overpass %d at %" NUM_FMT" seconds\n", i, Parptr->op_multisteps[i]);
			}
			fclose(tmp_fp);
		}
		else {
			Statesptr->multi_op = OFF;
			if (verbosemode == ON) printf("\nUnable to open multiple overpass output file: %s\n", Fnameptr->opfilename);
		}
	}

	//Load checkpointed data if this job has been restarted
	if (Statesptr->checkpoint == ON) {
		ReadCheckpoint(Fnameptr, Statesptr, Parptr, Solverptr, BCptr, CSTypePtr, Arrptr, verbosemode);
		if (verbosemode == ON) printf(" - checkpoint output file: %s\n", Fnameptr->checkpointfilename);
	}

	//mass balance
	sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".mass");
	if (Statesptr->checkpoint == ON && Solverptr->t > 0) { //if this is a checkpointed job, we only need to amend the .mass file
		FpsPtr->mass_fp = fopen(tmpFileNamePtr, "a");
	}
	else {
		FpsPtr->mass_fp = fopen(tmpFileNamePtr, "w");
	}
	if (FpsPtr->mass_fp != NULL)
	{
		if (Solverptr->t == 0) {
			fprintf(FpsPtr->mass_fp, "Time         Tstep      MinTstep   NumTsteps    Area         Vol         Qin         Hds           Qout          Qerror       Verror       Rain-(Inf+Evap)    Rain(m3/s)    Rain(mm/h)   Infil(m3/s)   Infil(mm/h)  Evap(m3/s)  Evap(mm/h)");
			if (Statesptr->use_interflow_singlelayer == ON)
			{
				fprintf(FpsPtr->mass_fp, " InterGenQ(m3/s)  InterRoff(m3)  InterRoff(m3/s) Inter2ChQ(m3/s)");
			}
			if (Statesptr->use_interflow_multilayer == ON)
			{
				// 每一层
				for (int lyrr = 0; lyrr < Parptr->multi_nSoilLyrs; lyrr++)
				{
					fprintf(FpsPtr->mass_fp, " InterGenQ_%d(m3/s) InterRoff_%d(m3) InterRoff_%d(m3/s) Inter2ChQ_%d(m3/s)", lyrr, lyrr, lyrr, lyrr);
				}
				// 所有层总的
				fprintf(FpsPtr->mass_fp, " InterGenQ(m3/s) InterRoff(m3) InterRoff(m3/s) Inter2ChQ(m3/s)");

			}
			if (Statesptr->use_percolation_singlelayer == ON)
			{
				fprintf(FpsPtr->mass_fp, " SoilMoi(Pct) SoilDep(mm) PercVol(m3/s) PercDep(mm/h) SoilFC(Pct) SoilProsty(Pct)");
			}
			if (Statesptr->use_percolation_multilayer == ON)
			{
				// 每一层
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					fprintf(FpsPtr->mass_fp, " SoilMoi_%d(Pct) SoilDep_%d(mm) PercVol_%d(m3/s) PercDep_%d(mm/h) SoilFC_%d(Pct) SoilProsty_%d(Pct)", lyr, lyr, lyr, lyr, lyr, lyr);
				}
				//fprintf(FpsPtr->mass_fp, " PercVol(m3/s) PercDep(mm/h)    GWSto(m3)    GWDep(mm)   GWQ2C(m3/s)  GWQ2C_PC(m3/s)");
			}
			if (Statesptr->use_groundwater == ON)
			{
				fprintf(FpsPtr->mass_fp, "     GWSto(m3)     GWDep(mm)     GWQ2C(m3/s)     GWQ2C_PC(m3/s)");
			}
			if (Statesptr->use_green_ampt_singlelayer == ON)
			{
				//fprintf(FpsPtr->mass_fp, "  Inifil      AccDep");
			}
			if (Statesptr->use_dhsvm == ON)
			{
				fprintf(FpsPtr->mass_fp, " SubFlow2Ch(m3/s)   SubFlow2Surf(m3/s)   SubPerc2Surf(m3/s)   SurfFlow2Ch(m3/s)   SurfHydro2Ch(m3/s)");
				for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
				{
					fprintf(FpsPtr->mass_fp, "      SoilDep_%d(cm)     Perc_%d(cm/h)", lyr, lyr);
				}
			}

			fprintf(FpsPtr->mass_fp, "\n");
		}
		else
		{
			// make a note in the mass file that this is a restart point - user can then edit the overlap out if they want a continuous mass file record.
			fprintf(FpsPtr->mass_fp, "####################################################### Checkpoint restart ########################################################\n");
			fprintf(FpsPtr->mass_fp, "Time         Tstep      MinTstep   NumTsteps    Area         Vol         Qin         Hds        Qout          Qerror       Verror       Rain-(Inf+Evap)\n");

			fflush(FpsPtr->mass_fp); // force program to flush buffer to file - keeps file in sync with writes - user sometimes tracks progress through the file.
		}
	}
	else
	{
		if (verbosemode == ON)
		{
			printf("Unable to open mass balance file: %s", tmpFileNamePtr);
			exit(0);
		}
	}
	// 打开poi输出文件
	if (Statesptr->save_poi == ON)
	{
		FpsPtr->pois_fp = new FILE*[Poisptr->num];
		for (int i = 0; i < Poisptr->num; i++)
		{
			sprintf(tmpFileNamePtr, "%s_%i%s", Fnameptr->resrootname, i + 1, ".pois");
			if (Solverptr->t > 0) {
				FpsPtr->pois_fp[i] = fopen(tmpFileNamePtr, "a");
			}
			else {
				FpsPtr->pois_fp[i] = fopen(tmpFileNamePtr, "w");
			}
			if (FpsPtr->pois_fp[i] != NULL)
			{
				if (Solverptr->t == 0)
				{
					const int grid_cols = Parptr->xsz;
					int grid_cols_padded = grid_cols + 1 + (64 / sizeof(NUMERIC_TYPE));
					grid_cols_padded += (GRID_ALIGN_WIDTH - (grid_cols_padded % GRID_ALIGN_WIDTH)) % GRID_ALIGN_WIDTH;
					fprintf(FpsPtr->pois_fp[i], "x:%i    y:%i    gridindex:%i\n", Poisptr->xpi[i], Poisptr->ypi[i], Poisptr->ypi[i] * grid_cols_padded + Poisptr->xpi[i]);
					if (Statesptr->use_snow_glacier == ON) {
						fprintf(FpsPtr->pois_fp[i], "Time         Tstep      T          Rain          Snow          Freeze        S-Melt        S-Thickness   G-Melt        G-Thickness   Infil         Soil-Wtr         Q             H             Vol           \n");
					}
					else if (Statesptr->use_dhsvm) {
						fprintf(FpsPtr->pois_fp[i], "Time         Tstep       Rain(mm)      Infil(mm)     InfilCh(mm)   Evap(mm)    Qx(mm)       Qy(mm)       Qch(mm)        H(mm)           Vol           SurfWD(mm)    LatFlowIn(mm)  LatFlowout(mm) ");
						// 每一层
						for (int lyr = 0; lyr < Parptr->multi_nSoilLyrs; lyr++)
						{
							fprintf(FpsPtr->pois_fp[i], "Perc_%d(mm)      LatFlowIn_%d(mm)   LatFlowOut_%d(mm)  SoilWtrDep_%d(mm)   SoilWtrDepDt_%d(mm)  ", lyr, lyr, lyr, lyr, lyr);
						}
						fprintf(FpsPtr->pois_fp[i], "\n");
					}
					else {
						fprintf(FpsPtr->pois_fp[i], "Time        Tstep         Rain     Infil         Soil-Wtr         Soil-Moi         Q            H             Vol\n");
					}
				}
				fflush(FpsPtr->pois_fp[i]);
			}
		}

	}


	// FEOL Dam Output file
	//mass balance
	if (Statesptr->DamMode == ON)
	{
		sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".dam");
		FpsPtr->dam_fp = fopen(tmpFileNamePtr, "w");

		if (FpsPtr->dam_fp != NULL)
		{
			if (Solverptr->t == 0) fprintf(FpsPtr->dam_fp, "*Time         *Tstep    Area         Vol         Vin         Hds        Vout          Qspill       Qoperation	Rain+Evap)\n");
			else
			{
				fprintf(FpsPtr->dam_fp, "*Time         *Tstep    Area         Vol         Vin         Hds        Vout          Qspill       Qoperation	Rain+Evap)\n");
				fflush(FpsPtr->dam_fp); // force program to flush buffer to file - keeps file in sync with writes - user sometimes tracks progress through the file.
			}
		}
		else
		{
			if (verbosemode == ON)
			{
				printf("Unable to open Dam output file: %s", tmpFileNamePtr);
				exit(0);
			}
		}
	}

	//stage output file
	if (Statesptr->save_stages == ON) {
		sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".stage");
		if (Statesptr->checkpoint == ON && Solverptr->t > 0) { //if this is a checkpointed job, we only need to amend the .stage file
			FpsPtr->stage_fp = fopen(tmpFileNamePtr, "a");
		}
		else {
			FpsPtr->stage_fp = fopen(tmpFileNamePtr, "w");
		}
		if (FpsPtr->stage_fp != NULL)
		{
			if (Solverptr->t == C(0.0) || Statesptr->checkpoint == OFF) // chnage to export z information if initial stage used (JCN)
			{
				fprintf(FpsPtr->stage_fp, "Stage output, depth (m). Stage locations from: %s\n\n", Fnameptr->stagefilename);
				fprintf(FpsPtr->stage_fp, "Stage information (stage,x,y,elev):\n");
				for (i = 0; i < Stageptr->Nstages; i++)
				{
					if (Statesptr->SGC == ON && Arrptr->SGCwidth[Stageptr->stage_grid_x[i] + Stageptr->stage_grid_y[i] * Parptr->xsz] > 0) // if a SUB GRID channel is present export the channel bed elevation
					{
						if (Stageptr->stage_check[i] == 1) fprintf(FpsPtr->stage_fp, "%d\t%.4" NUM_FMT"\t%.4" NUM_FMT"\t%.4" NUM_FMT"\n", i + 1, Stageptr->stage_loc_x[i], Stageptr->stage_loc_y[i], Arrptr->SGCz[Stageptr->stage_grid_x[i] + Stageptr->stage_grid_y[i] * Parptr->xsz]);
						else fprintf(FpsPtr->stage_fp, "%d\t%.4" NUM_FMT"\t%.4" NUM_FMT"\tn/a\n", i + 1, Stageptr->stage_loc_x[i], Stageptr->stage_loc_y[i]);
					}
					else
					{
						if (Stageptr->stage_check[i] == 1) fprintf(FpsPtr->stage_fp, "%d\t%.4" NUM_FMT"\t%.4" NUM_FMT"\t%.4" NUM_FMT"\n", i + 1, Stageptr->stage_loc_x[i], Stageptr->stage_loc_y[i], Arrptr->DEM[Stageptr->stage_grid_x[i] + Stageptr->stage_grid_y[i] * Parptr->xsz]);
						else fprintf(FpsPtr->stage_fp, "%d\t%.4" NUM_FMT"\t%.4" NUM_FMT"\tn/a\n", i + 1, Stageptr->stage_loc_x[i], Stageptr->stage_loc_y[i]);
					}
				}
				fprintf(FpsPtr->stage_fp, "\nOutput, depths:\n");
				fprintf(FpsPtr->stage_fp, "Time; stages 1 to %d\n", Stageptr->Nstages);
			}
			else
			{
				fprintf(FpsPtr->stage_fp, "####################################################### Checkpoint restart ########################################################\n");
				fflush(FpsPtr->stage_fp);
			}
		}
		else
		{
			if (verbosemode == ON) printf("Unable to open stage output file: %s", tmpFileNamePtr);
			Statesptr->save_stages = OFF;
		}

	}
	//velocity output file
	if (Statesptr->save_stages == ON && Statesptr->voutput_stage == ON)
	{
		sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".velocity");
		if (Statesptr->checkpoint == ON && Solverptr->t > 0) { //if this is a checkpointed job, we only need to amend the .stage file
			FpsPtr->vel_fp = fopen(tmpFileNamePtr, "a");
		}
		else {
			FpsPtr->vel_fp = fopen(tmpFileNamePtr, "w");
		}
		if (FpsPtr->vel_fp != NULL) {
			if (Solverptr->t == 0) {
				fprintf(FpsPtr->vel_fp, "Velocity output, velocity (ms-1). Velocity locations from: %s\n\n", Fnameptr->stagefilename);
				fprintf(FpsPtr->vel_fp, "Stage information (stage,x,y,elev):\n");
				for (i = 0; i < Stageptr->Nstages; i++) {
					if (Stageptr->stage_check[i] == 1) fprintf(FpsPtr->vel_fp, "%d\t%.4" NUM_FMT"\t%.4" NUM_FMT"\t%.4" NUM_FMT"\n", i + 1, Stageptr->stage_loc_x[i], Stageptr->stage_loc_y[i], Arrptr->DEM[Stageptr->stage_grid_x[i] + Stageptr->stage_grid_y[i] * Parptr->xsz]);
					else fprintf(FpsPtr->vel_fp, "%d\t%.4" NUM_FMT"\t%.4" NUM_FMT"\tn/a\n", i + 1, Stageptr->stage_loc_x[i], Stageptr->stage_loc_y[i]);
				}
				fprintf(FpsPtr->vel_fp, "\nOutput, depths:\n");
				fprintf(FpsPtr->vel_fp, "Time; velocities 1 to %d\n", Stageptr->Nstages);
			}
			else {
				fprintf(FpsPtr->vel_fp, "####################################################### Checkpoint restart ########################################################\n");
				fflush(FpsPtr->vel_fp);
			}
		}
		else {
			if (verbosemode == ON) printf("Unable to open velocity output file: %s", tmpFileNamePtr);
			Statesptr->save_stages = OFF;
		}
	}

	//velocity output file
	if (Statesptr->gsection == ON)
	{
		sprintf(tmpFileNamePtr, "%s%s", Fnameptr->resrootname, ".discharge");
		if (Statesptr->checkpoint == ON && Solverptr->t > 0) { //if this is a checkpointed job, we only need to amend the .stage file
			FpsPtr->gau_fp = fopen(tmpFileNamePtr, "a");
		}
		else {
			FpsPtr->gau_fp = fopen(tmpFileNamePtr, "w");
		}
		if (FpsPtr->gau_fp != NULL) {
			if (Solverptr->t == 0) {
				fprintf(FpsPtr->gau_fp, "Discharge output, discharge (m3s-1). Discharge locations from: %s\n\n", Fnameptr->gaugefilename);
				fprintf(FpsPtr->gau_fp, "Time; discharge 1 to %d\n", Stageptr->Ngauges);
			}
			else {
				fprintf(FpsPtr->gau_fp, "####################################################### Checkpoint restart ########################################################\n");
				fflush(FpsPtr->gau_fp);
			}
		}
		else {
			if (verbosemode == ON) printf("Unable to open discharge output file: %s", tmpFileNamePtr);
			Statesptr->gsection = OFF;
		}
	}

	if (Statesptr->maxdepthonly == ON)
	{
	}
	else
	{
		// output debug files (used DEM, channel mask seg mask) if required
		if (Statesptr->debugmode == ON)
			debugfileoutput(Fnameptr, Statesptr, Parptr, Arrptr);

		if (Statesptr->SGC == ON) // output base/bed DEM including channel depths for display purposes with water depth
			write_ascfile(Fnameptr->resrootname, -1, ".dem", Arrptr->SGCz, Arrptr->DEM, 0, Statesptr, Parptr);
		else  // Write out final DEM if not subgrid - includes 1D river channel and channel bank modifications
			write_ascfile(Fnameptr->resrootname, -1, ".dem", Arrptr->DEM, Arrptr->DEM, 0, Statesptr, Parptr);
	}

	//start simulation
	//选择不同的模拟器开始模拟
	time(&Solverptr->time_start);
	if (Statesptr->SGC == ON) // SGC output
	{
		// SGC模拟
		//Fast_MainStart(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Locptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr, DamData *Damptr, int verbose)

		Fast_MainStart(Fnameptr, FpsPtr, Statesptr, Parptr, Solverptr, Poisptr, BCptr, Stageptr, CSTypePtr, Arrptr, SGCptr, ChannelSegmentsVecPtr, Damptr, LFPContextPtr, Super_linksptr, LfpCouplingInfoPtr); //Damptr added by FEOL
	}
	else if (Statesptr->fv1 == ON)
	{
		fv1::solve(Fnameptr, FpsPtr, Statesptr, Parptr, Solverptr, BCptr,
			Stageptr, Arrptr, verbosemode);
	}
	else if (Statesptr->dg2 == ON)
	{
		dg2::solve(Fnameptr, FpsPtr, Statesptr, Parptr, Solverptr, BCptr,
			Stageptr, Arrptr, verbosemode);
	}
	else
	{
		IterateQ(Fnameptr, FpsPtr, Statesptr, Parptr, Solverptr, BCptr, Stageptr, CSTypePtr, Arrptr, SGCptr, RiversIndexVecPtr, RiversIndexPtr, ChannelSegmentsVecPtr, verbosemode);
	}



	return 0;
}

