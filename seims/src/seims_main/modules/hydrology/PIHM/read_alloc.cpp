#include "pihm.h"

void ReadAlloc(pihm_struct *pihm_strc, char pihm_dir[])
{
	char            proj[MAXSTRING];
	char           *token;
	char *nextToken; // 用于保存下一个子字符串的位置
	pihm_printf(VL_VERBOSE, "\nRead input files:\n");

	strcpy(proj, project);
	if (strstr(proj, ".") != 0)
	{

		//token = strtok(proj, ".");
		token = strtok_s(proj, ".", &nextToken);

		strcpy(proj, token);
	}
	else
	{
		strcpy(proj, project);
	}

	// Set file names of the input files
	sprintf(pihm_strc->filename.riv, "%s/input/%s/%s.riv", pihm_dir,proj, proj);
	sprintf(pihm_strc->filename.mesh, "%s/input/%s/%s.mesh", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.att, "%s/input/%s/%s.att", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.soil, "%s/input/%s/%s.soil", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.lc, "%s/input/vegprmt.tbl", pihm_dir);
	sprintf(pihm_strc->filename.meteo, "%s/input/%s/%s.meteo", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.lai, "%s/input/%s/%s.lai", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.bc, "%s/input/%s/%s.bc", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.para, "%s/input/%s/%s.para", pihm_dir, proj, proj);
	sprintf(pihm_strc->filename.calib, "%s/input/%s/%s.calib", pihm_dir, proj, project);
	sprintf(pihm_strc->filename.ic, "%s/input/%s/%s.ic", pihm_dir, proj, project);
	//sprintf(final_downstream_file, "%s/input/%s/final_downstream_file.txt", pihm_dir, proj);
#if defined(_DGW_)
	sprintf(pihm->filename.geol, "%s/input/%s/%s.geol", pihm_dir, proj, proj);
	sprintf(pihm->filename.bedrock, "%s/input/%s/%s.bedrock", pihm_dir, proj, proj);
#endif
#if defined(_NOAH_)
	sprintf(pihm->filename.lsm, "%s/input/%s/%s.lsm", pihm_dir, proj, proj);
	sprintf(pihm->filename.rad, "%s/input/%s/%s.rad", pihm_dir, proj, proj);
	sprintf(pihm->filename.ice, "%s/input/%s/%s.ice", pihm_dir, proj, proj);
#endif
#if defined(_CYCLES_)
	sprintf(pihm->filename.cycles, "%s/input/%s/%s.cycles", pihm_dir, proj, proj);
	sprintf(pihm->filename.soilinit, "%s/input/%s/%s.soilinit", pihm_dir, proj, proj);
	sprintf(pihm->filename.crop, "%s/input/%s/%s.crop", pihm_dir, proj, proj);
	sprintf(pihm->filename.cyclesic, "%s/input/%s/%s.cyclesic", pihm_dir, proj, proj);
#endif
#if defined(_BGC_)
	sprintf(pihm->filename.bgc, "%s/input/%s/%s.bgc", pihm_dir, proj, proj);
	sprintf(pihm->filename.bgcic, "%s/input/%s/%s.bgcic", pihm_dir, proj, proj);
#endif
#if defined(_RT_)
	sprintf(pihm->filename.chem, "%s/input/%s/%s.chem", pihm_dir, proj, proj);
	sprintf(pihm->filename.cini, "%s/input/%s/%s.cini", pihm_dir, proj, proj);
	sprintf(pihm->filename.cdbs, "%s/input/%s/%s.cdbs", pihm_dir, proj, proj);
	sprintf(pihm->filename.prep, "%s/input/%s/%s.prep", pihm_dir, proj, proj);
	sprintf(pihm->filename.rtic, "%s/input/%s/%s.rtic", pihm_dir, proj, proj);
#endif

	// Read river input file
	ReadRiver(pihm_strc->filename.riv, &pihm_strc->rivtbl, &pihm_strc->shptbl, &pihm_strc->matltbl, &pihm_strc->forc);

	// Read mesh structure input file
	ReadMesh(pihm_strc->filename.mesh, &pihm_strc->meshtbl);

	// Read attribute table input file
	ReadAtt(pihm_strc->filename.att, &pihm_strc->atttbl);

	// Read soil input file
	ReadSoil(pihm_strc->filename.soil, &pihm_strc->soiltbl);

	// Read land cover input file
	ReadLc(pihm_strc->filename.lc, &pihm_strc->lctbl);

	// Read meteorological forcing input file
	ReadMeteo(pihm_strc->filename.meteo, &pihm_strc->forc);

	// Read LAI input file
	ReadLai(pihm_strc->filename.lai, &pihm_strc->atttbl, &pihm_strc->forc);



	// Read source and sink input file
	pihm_strc->forc.nsource = 0;
#if NOT_YET_IMPLEMENTED
	ReadSS();
#endif

	// Read model control file
	ReadPara(pihm_strc->filename.para, &pihm_strc->ctrl);

	// Read calibration input file
	ReadCalib(pihm_strc->filename.calib, &pihm_strc->calib);

#if defined(_DGW_)
	// Read geology input file
	ReadGeol(pihm->filename.geol, &pihm->geoltbl);

	// Read bedrock control file
	ReadBedrock(pihm->filename.bedrock, &pihm->meshtbl, &pihm->atttbl, &pihm->ctrl);
#endif

#if defined(_NOAH_)
	// Read LSM input file
	ReadLsm(pihm->filename.lsm, &pihm->ctrl, &pihm->siteinfo, &pihm->noahtbl);

	if (pihm->ctrl.rad_mode == TOPO_SOL)
	{
		// Read radiation input file
		ReadRad(pihm->filename.rad, &pihm->forc);
	}
#endif

#if defined(_RT_)
	// Read RT input file
	ReadChem(pihm->filename.chem, pihm->filename.cdbs, pihm->chemtbl, pihm->kintbl, &pihm->rttbl, &pihm->forc,
		&pihm->ctrl);

	ReadCini(pihm->filename.cini, pihm->chemtbl, pihm->rttbl.num_stc, &pihm->atttbl, &pihm->chmictbl);

	if (pihm->forc.prcp_flag == 2)
	{
		ReadPrep(pihm->filename.prep, pihm->chemtbl, &pihm->rttbl, &pihm->forc);
	}
#endif

	// Read boundary condition input file
	// Boundary conditions might be needed by DGW and RT thus should be read in after reading bedrock and chemistry
	// input
#if defined(_RT_)
	ReadBc(pihm->filename.bc, &pihm->atttbl, pihm->chemtbl, &pihm->rttbl, &pihm->forc);
#else
	ReadBc(pihm_strc->filename.bc, &pihm_strc->atttbl, &pihm_strc->forc);
#endif

#if defined(_CYCLES_)
	// Read Cycles simulation control file
	ReadCyclesCtrl(pihm->filename.cycles, pihm->filename.co2, &pihm->agtbl, &pihm->ctrl, &pihm->co2ctrl);

	// Read soil initialization file
	ReadSoilInit(pihm->filename.soilinit, &pihm->soiltbl);

	// Read crop description file
	ReadCrop(pihm->filename.crop, pihm->croptbl);

	// Read operation files
	ReadMultOper(&pihm->agtbl, pihm->mgmttbl, pihm->croptbl);
#endif

#if defined(_BGC_)
	ReadBgc(pihm->filename.bgc, pihm->filename.co2, pihm->filename.ndep, &pihm->ctrl, &pihm->co2ctrl, &pihm->ndepctrl,
		&pihm->cninit);

	// Read Biome-BGC epc files
	ReadEpc(&pihm->epctbl);

	// Read Ndep file
	pihm->forc.ndep = (tsdata_struct *)malloc(sizeof(tsdata_struct));

	if (pihm->ndepctrl.varndep)
	{
		pihm->forc.nndep = 1;
		ReadAnnualFile(pihm->filename.ndep, &pihm->forc.ndep[0]);
	}
	else
	{
		pihm->forc.nndep = 0;
	}
#endif

#if defined(_BGC_) || defined(_CYCLES_)
	// Read CO2 file
	pihm->forc.co2 = (tsdata_struct *)malloc(sizeof(tsdata_struct));

	if (pihm->co2ctrl.varco2)
	{
		pihm->forc.nco2 = 1;
		ReadAnnualFile(pihm->filename.co2, &pihm->forc.co2[0]);
	}
	else
	{
		pihm->forc.nco2 = 0;
	}
#endif
}
