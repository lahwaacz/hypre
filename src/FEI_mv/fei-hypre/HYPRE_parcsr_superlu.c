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
 *********************************************************************EHEADER*/

/******************************************************************************
 *
 * HYPRE_ParCSR_SuperLU interface
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "utilities/_hypre_utilities.h"
#include "HYPRE.h"
#include "parcsr_mv/_hypre_parcsr_mv.h"
#include "parcsr_ls/HYPRE_parcsr_ls.h"

#include "HYPRE_FEI.h"

/*---------------------------------------------------------------------------
 * SUPERLU include files
 *-------------------------------------------------------------------------*/

#ifdef HAVE_SUPERLU_20
#include "dsp_defs.h"
#include "superlu_util.h"

typedef struct HYPRE_SuperLU_Struct
{
   int          factorized_;
   int          *permR_;
   int          *permC_;
   SuperMatrix  SLU_Lmat;
   SuperMatrix  SLU_Umat;
   int          outputLevel_;
}
HYPRE_SuperLU;
#endif

#ifdef HAVE_SUPERLU
#include "slu_ddefs.h"
#include "slu_util.h"

typedef struct HYPRE_SuperLU_Struct
{
   int          factorized_;
   int          *permR_;
   int          *permC_;
   SuperMatrix  SLU_Lmat;
   SuperMatrix  SLU_Umat;
   int          outputLevel_;
}
HYPRE_SuperLU;
#endif

/***************************************************************************
 * HYPRE_ParCSR_SuperLUCreate - Return a SuperLU object "solver".  
 *--------------------------------------------------------------------------*/

int HYPRE_ParCSR_SuperLUCreate( MPI_Comm comm, HYPRE_Solver *solver )
{
#ifdef HAVE_SUPERLU
   int           nprocs;
   HYPRE_SuperLU *sluPtr;
   
   MPI_Comm_size(comm, &nprocs);
   if ( nprocs > 1 )
   {
      printf("HYPRE_ParCSR_SuperLUCreate ERROR - too many processors.\n");
      return -1;
   }
   sluPtr = hypre_TAlloc(HYPRE_SuperLU, 1, HYPRE_MEMORY_HOST);
   assert ( sluPtr != NULL );
   sluPtr->factorized_  = 0;
   sluPtr->permR_       = NULL;
   sluPtr->permC_       = NULL;
   sluPtr->outputLevel_ = 0;
   *solver = (HYPRE_Solver) sluPtr;
   return 0;
#else
   printf("HYPRE_ParCSR_SuperLUCreate ERROR - SuperLU not enabled.\n");
   *solver = (HYPRE_Solver) NULL;
   return -1;
#endif
}

/***************************************************************************
 * HYPRE_ParCSR_SuperLUDestroy - Destroy a SuperLU object.
 *--------------------------------------------------------------------------*/

int HYPRE_ParCSR_SuperLUDestroy( HYPRE_Solver solver )
{
#ifdef HAVE_SUPERLU
   HYPRE_SuperLU *sluPtr = (HYPRE_SuperLU *) solver;
   assert ( sluPtr != NULL );
   if ( sluPtr->permR_ != NULL ) free(sluPtr->permR_);
   if ( sluPtr->permC_ != NULL ) free(sluPtr->permC_);
   free(sluPtr);
   return 0;
#else
   printf("HYPRE_ParCSR_SuperLUDestroy ERROR - SuperLU not enabled.\n");
   *solver = (HYPRE_Solver) NULL;
   return -1;
#endif
}

/***************************************************************************
 * HYPRE_ParCSR_SuperLUSetOutputLevel - Set debug level 
 *--------------------------------------------------------------------------*/

int HYPRE_ParCSR_SuperLUSetOutputLevel(HYPRE_Solver solver, int level)
{
#ifdef HAVE_SUPERLU
   HYPRE_SuperLU *sluPtr = (HYPRE_SuperLU *) solver;
   assert ( sluPtr != NULL );
   sluPtr->outputLevel_ = level;
   return 0;
#else
   printf("HYPRE_ParCSR_SuperLUSetOutputLevel ERROR - SuperLU not enabled.\n");
   *solver = (HYPRE_Solver) NULL;
   return -1;
#endif
}

/***************************************************************************
 * HYPRE_ParCSR_SuperLUSetup - Set up function for SuperLU.
 *--------------------------------------------------------------------------*/

int HYPRE_ParCSR_SuperLUSetup(HYPRE_Solver solver, HYPRE_ParCSRMatrix A_csr,
                              HYPRE_ParVector b, HYPRE_ParVector x )
{
#ifdef HAVE_SUPERLU
   int    startRow, endRow, nrows, *partition, *AdiagI, *AdiagJ, nnz;
   int    irow, colNum, index, *cscI, *cscJ, jcol, *colLengs;
   int    *etree, permcSpec, lwork, panelSize, relax, info;
   double *AdiagA, *cscA, diagPivotThresh, dropTol;
   char              refact[1];
   hypre_CSRMatrix   *Adiag;
   HYPRE_SuperLU     *sluPtr;
   SuperMatrix       sluAmat, auxAmat;
   superlu_options_t slu_options;
   SuperLUStat_t     slu_stat;

   /* ---------------------------------------------------------------- */
   /* get matrix information                                           */
   /* ---------------------------------------------------------------- */

   sluPtr = (HYPRE_SuperLU *) solver;
   assert ( sluPtr != NULL );
   HYPRE_ParCSRMatrixGetRowPartitioning( A_csr, &partition );
   startRow = partition[0];
   endRow   = partition[1] - 1;
   nrows    = endRow - startRow + 1;
   free( partition );
   if ( startRow != 0 )
   {
      printf("HYPRE_ParCSR_SuperLUSetup ERROR - start row != 0.\n");
      return -1;
   }

   /* ---------------------------------------------------------------- */
   /* get hypre matrix                                                 */
   /* ---------------------------------------------------------------- */

   Adiag  = hypre_ParCSRMatrixDiag((hypre_ParCSRMatrix *) A_csr);
   AdiagI = hypre_CSRMatrixI(Adiag);
   AdiagJ = hypre_CSRMatrixJ(Adiag);
   AdiagA = hypre_CSRMatrixData(Adiag);
   nnz    = AdiagI[nrows];

   /* ---------------------------------------------------------------- */
   /* convert the csr matrix into csc matrix                           */
   /* ---------------------------------------------------------------- */

   colLengs = hypre_TAlloc(int, nrows , HYPRE_MEMORY_HOST);
   for ( irow = 0; irow < nrows; irow++ ) colLengs[irow] = 0;
   for ( irow = 0; irow < nrows; irow++ )
      for ( jcol = AdiagI[irow]; jcol < AdiagI[irow+1]; jcol++ )
         colLengs[AdiagJ[jcol]]++;
   cscJ = hypre_TAlloc(int,  (nrows+1) , HYPRE_MEMORY_HOST);
   cscI = hypre_TAlloc(int,  nnz , HYPRE_MEMORY_HOST);
   cscA = hypre_TAlloc(double,  nnz , HYPRE_MEMORY_HOST);
   cscJ[0] = 0;
   nnz = 0;
   for ( jcol = 1; jcol <= nrows; jcol++ )
   {
      nnz += colLengs[jcol-1];
      cscJ[jcol] = nnz;
   }
   for ( irow = 0; irow < nrows; irow++ )
   {
      for ( jcol = AdiagI[irow]; jcol < AdiagI[irow+1]; jcol++ )
      {
         colNum = AdiagJ[jcol];
         index  = cscJ[colNum]++;
         cscI[index] = irow;
         cscA[index] = AdiagA[jcol];
      }
   }
   cscJ[0] = 0;
   nnz = 0;
   for ( jcol = 1; jcol <= nrows; jcol++ )
   {
      nnz += colLengs[jcol-1];
      cscJ[jcol] = nnz;
   }
   free(colLengs);

   /* ---------------------------------------------------------------- */
   /* create SuperMatrix                                                */
   /* ---------------------------------------------------------------- */
                                                                                
   dCreate_CompCol_Matrix(&sluAmat,nrows,nrows,cscJ[nrows],cscA,cscI,
                          cscJ, SLU_NC, SLU_D, SLU_GE);
   etree   = hypre_TAlloc(int, nrows , HYPRE_MEMORY_HOST);
   sluPtr->permC_  = hypre_TAlloc(int, nrows , HYPRE_MEMORY_HOST);
   sluPtr->permR_  = hypre_TAlloc(int, nrows , HYPRE_MEMORY_HOST);
   permcSpec = 0;
   get_perm_c(permcSpec, &sluAmat, sluPtr->permC_);
   slu_options.Fact = DOFACT;
   slu_options.SymmetricMode = NO;
   sp_preorder(&slu_options, &sluAmat, sluPtr->permC_, etree, &auxAmat);
   diagPivotThresh = 1.0;
   dropTol = 0.0;
   panelSize = sp_ienv(1);
   relax = sp_ienv(2);
   StatInit(&slu_stat);
   lwork = 0;
   slu_options.ColPerm = MY_PERMC;
   slu_options.DiagPivotThresh = diagPivotThresh;

   dgstrf(&slu_options, &auxAmat, dropTol, relax, panelSize,
          etree, NULL, lwork, sluPtr->permC_, sluPtr->permR_,
          &(sluPtr->SLU_Lmat), &(sluPtr->SLU_Umat), &slu_stat, &info);
   Destroy_CompCol_Permuted(&auxAmat);
   Destroy_CompCol_Matrix(&sluAmat);
   free(etree);
   sluPtr->factorized_ = 1;
   StatFree(&slu_stat);
   return 0;
#else
   printf("HYPRE_ParCSR_SuperLUSetup ERROR - SuperLU not enabled.\n");
   *solver = (HYPRE_Solver) NULL;
   return -1;
#endif
}

/***************************************************************************
 * HYPRE_ParCSR_SuperLUSolve - Solve function for SuperLU.
 *--------------------------------------------------------------------------*/

int HYPRE_ParCSR_SuperLUSolve(HYPRE_Solver solver, HYPRE_ParCSRMatrix A,
                              HYPRE_ParVector b, HYPRE_ParVector x )
{
#ifdef HAVE_SUPERLU
   int    nrows, i, info;
   double *bData, *xData;
   SuperMatrix B;
   SuperLUStat_t slu_stat;
   trans_t       trans;
   HYPRE_SuperLU *sluPtr = (HYPRE_SuperLU *) solver;

   /* ---------------------------------------------------------------- */
   /* make sure setup has been called                                  */
   /* ---------------------------------------------------------------- */

   assert ( sluPtr != NULL );
   if ( ! (sluPtr->factorized_) )
   {
      printf("HYPRE_ParCSR_SuperLUSolve ERROR - not factorized yet.\n");
      return -1;
   }

   /* ---------------------------------------------------------------- */
   /* fetch right hand side and solution vector                        */
   /* ---------------------------------------------------------------- */

   xData = hypre_VectorData(hypre_ParVectorLocalVector((hypre_ParVector *)x));
   bData = hypre_VectorData(hypre_ParVectorLocalVector((hypre_ParVector *)b));
   nrows = hypre_ParVectorGlobalSize((hypre_ParVector *)x); 
   for (i = 0; i < nrows; i++) xData[i] = bData[i];

   /* ---------------------------------------------------------------- */
   /* solve                                                            */
   /* ---------------------------------------------------------------- */

   dCreate_Dense_Matrix(&B, nrows, 1, bData, nrows, SLU_DN, SLU_D,SLU_GE);

   /* -------------------------------------------------------------
    * solve the problem
    * -----------------------------------------------------------*/

   trans = NOTRANS;
   StatInit(&slu_stat);
   dgstrs (trans, &(sluPtr->SLU_Lmat), &(sluPtr->SLU_Umat), 
           sluPtr->permC_, sluPtr->permR_, &B, &slu_stat, &info);
   Destroy_SuperMatrix_Store(&B);
   StatFree(&slu_stat);
   return 0;
#else
   printf("HYPRE_ParCSR_SuperLUSolve ERROR - SuperLU not enabled.\n");
   *solver = (HYPRE_Solver) NULL;
   return -1;
#endif
}

