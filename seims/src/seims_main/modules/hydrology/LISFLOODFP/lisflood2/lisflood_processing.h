#pragma once

#include "../lisflood.h"

//void Fast_MainStart(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr,Pois* Poisptr, BoundCs *BCptr, Stage *Locptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr, DamData *Damptr, int verbose);
void Fast_MainStart(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois* Poisptr, BoundCs *BCptr, Stage *Locptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr, DamData *Damptr, LISFLOODFPContext* LFPContextPtr);
//void Fast_MainInit(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Locptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr, DamData *Damptr, int verbose);
void Fast_MainInit(Fnames *Fnameptr, Files *Fptr, States *Statesptr, Pars *Parptr, Solver *Solverptr, Pois *Poisptr, BoundCs *BCptr, Stage *Locptr, ChannelSegmentType *ChannelSegments, Arrays *Arrptr, SGCprams *SGCptr, vector<ChannelSegmentType> *ChannelSegmentsVecPtr, DamData *Damptr, LISFLOODFPContext* LFPContextPtr);
