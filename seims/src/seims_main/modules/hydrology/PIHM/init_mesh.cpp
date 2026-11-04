#include "pihm.h"

void InitMesh(const meshtbl_struct *meshtbl, elem_struct elem[])
{
	int             i;

#if defined(_OPENMP)
# pragma omp parallel for
#endif
	for (i = 0; i < nelem; i++)
	{
		int             j;
		// index
		elem[i].ind = i + 1;
		// 初始化三角形的三个顶点、三个相邻三角形、相邻河道
		for (j = 0; j < NUM_EDGE; j++)
		{
			elem[i].node[j] = meshtbl->node[i][j];
			elem[i].nabr[j] = meshtbl->nabr[i][j];
			elem[i].nabr_river[j] = 0;  // initialize to 0
		}
	}
}
