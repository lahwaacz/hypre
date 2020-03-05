/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision$
 ***********************************************************************EHEADER*/

#ifndef hypre_SSAMG_HEADER
#define hypre_SSAMG_HEADER

/*--------------------------------------------------------------------------
 * hypre_SSAMGData
 *
 * Notes:
 *        1) The value of active_l can vary across parts. We are not using it
 *           for load balance reasons.
 *--------------------------------------------------------------------------*/

typedef struct hypre_SSAMGData_struct
{
   hypre_MPI_Comm          comm;
   HYPRE_Int               nparts;

   HYPRE_Real              tol;
   HYPRE_Int               max_iter;
   HYPRE_Int               rel_change;
   HYPRE_Int               zero_guess;
   HYPRE_Int               max_levels; /* max_level <= 0 means no limit */

   HYPRE_Int               relax_type;
   HYPRE_Real              relax_weight;   /* relax weight */
   HYPRE_Int               num_pre_relax;  /* number of pre relaxation sweeps */
   HYPRE_Int               num_post_relax; /* number of post relaxation sweeps */
   HYPRE_Int               num_levels;     /* number of levels of the multigrid hierarchy */

   HYPRE_Int             **cdir_l;   /* (nlevels x nparts) array: coarsening directions */
   HYPRE_Real            **dxyz;     /* (nparts x 3) array: used to determine cdir */
   hypre_SStructGrid     **grid_l;

   /* work matrices and vectors */
   hypre_SStructMatrix   **A_l;
   hypre_SStructMatrix   **P_l;
   hypre_SStructMatrix   **RT_l;
   hypre_SStructVector   **b_l;
   hypre_SStructVector   **x_l;
   hypre_SStructVector   **r_l;
   hypre_SStructVector   **e_l;
   hypre_SStructVector   **tx_l;

   /* data structures for performing relaxation, interpolation,
      restriction and matrix-vector multiplication */
   void                  **relax_data_l;
   void                  **matvec_data_l;
   void                  **restrict_data_l;
   void                  **interp_data_l;

   /* log info (always logged) */
   HYPRE_Int               num_iterations;
   HYPRE_Int               time_index;
   HYPRE_Int               print_level;

   /* additional log info (logged when `logging' > 0) */
   HYPRE_Int               logging;
   HYPRE_Real             *norms;
   HYPRE_Real             *rel_norms;

} hypre_SSAMGData;

/*--------------------------------------------------------------------------
 * Accessor functions for the hypre_SSAMGData structure
 *--------------------------------------------------------------------------*/

#define hypre_SSAMGDataComm(ssamg_data)          ((ssamg_data) -> comm)
#define hypre_SSAMGDataNparts(ssamg_data)        ((ssamg_data) -> nparts)
#define hypre_SSAMGDataTol(ssamg_data)           ((ssamg_data) -> tol)
#define hypre_SSAMGDataNumLevels(ssamg_data)     ((ssamg_data) -> num_levels)
#define hypre_SSAMGDataMaxLevels(ssamg_data)     ((ssamg_data) -> max_levels)
#define hypre_SSAMGDataMaxIter(ssamg_data)       ((ssamg_data) -> max_iter)
#define hypre_SSAMGDataRelChange(ssamg_data)     ((ssamg_data) -> rel_change)
#define hypre_SSAMGDataZeroGuess(ssamg_data)     ((ssamg_data) -> zero_guess)
#define hypre_SSAMGDataNumIterations(ssamg_data) ((ssamg_data) -> num_iterations)
#define hypre_SSAMGDataRelaxType(ssamg_data)     ((ssamg_data) -> relax_type)
#define hypre_SSAMGDataRelaxWeight(ssamg_data)   ((ssamg_data) -> relax_weight)
#define hypre_SSAMGDataNumPreRelax(ssamg_data)   ((ssamg_data) -> num_pre_relax)
#define hypre_SSAMGDataNumPosRelax(ssamg_data)   ((ssamg_data) -> num_post_relax)
#define hypre_SSAMGDataTimeIndex(ssamg_data)     ((ssamg_data) -> time_index)
#define hypre_SSAMGDataPrintLevel(ssamg_data)    ((ssamg_data) -> print_level)
#define hypre_SSAMGDataLogging(ssamg_data)       ((ssamg_data) -> logging)
#define hypre_SSAMGDataDxyz(ssamg_data)          ((ssamg_data) -> dxyz)
#define hypre_SSAMGDataGridl(ssamg_data)         ((ssamg_data) -> grid_l)
#define hypre_SSAMGDataCdir(ssamg_data)          ((ssamg_data) -> cdir_l)
#define hypre_SSAMGDataNorms(ssamg_data)         ((ssamg_data) -> norms)
#define hypre_SSAMGDataRelNorms(ssamg_data)      ((ssamg_data) -> rel_norms)

#endif