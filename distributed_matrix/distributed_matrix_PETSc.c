/*BHEADER**********************************************************************
 * (c) 1997   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/
/******************************************************************************
 *
 * Member functions for hypre_DistributedMatrix class for PETSc storage scheme.
 *
 *****************************************************************************/

#include "./distributed_matrix.h"

/* Public headers and prototypes for PETSc matrix library */
#ifdef PETSC_AVAILABLE
#include "sles.h"
#endif

/*--------------------------------------------------------------------------
 * hypre_FreeDistributedMatrixPETSc
 *   Internal routine for freeing a matrix stored in PETSc form.
 *--------------------------------------------------------------------------*/

int 
hypre_FreeDistributedMatrixPETSc( hypre_DistributedMatrix *distributed_matrix )
{
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(distributed_matrix);

   MatDestroy( PETSc_matrix );
#endif

   return(0);
}

/*--------------------------------------------------------------------------
 * Optional routines that depend on underlying storage type
 *--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------
 * hypre_PrintDistributedMatrixPETSc
 *   Internal routine for printing a matrix stored in PETSc form.
 *--------------------------------------------------------------------------*/

int 
hypre_PrintDistributedMatrixPETSc( hypre_DistributedMatrix *matrix )
{
   int  ierr=0;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);

   ierr = MatView( PETSc_matrix, VIEWER_STDOUT_WORLD );
#endif
   return(ierr);
}

/*--------------------------------------------------------------------------
 * hypre_GetDistributedMatrixLocalRangePETSc
 *--------------------------------------------------------------------------*/

int 
hypre_GetDistributedMatrixLocalRangePETSc( hypre_DistributedMatrix *matrix,
                             int *start,
                             int *end )
{
   int ierr=0;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);
   MatType PETScType;

   if (!PETSc_matrix) return(-1);

   ierr = MatGetType( PETSc_matrix, &PETScType, NULL ); CHKERRA(ierr);

   if (PETScType != MATMPIAIJ) return(-1);

   ierr = MatGetOwnershipRange( PETSc_matrix, start, end ); CHKERRA(ierr);
#endif

   return(ierr);
}

/*--------------------------------------------------------------------------
 * hypre_GetDistributedMatrixRowPETSc
 *--------------------------------------------------------------------------*/

int 
hypre_GetDistributedMatrixRowPETSc( hypre_DistributedMatrix *matrix,
                             int row,
                             int *size,
                             int **col_ind,
                             double **values )
{
   int ierr;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);
   MatType PETScType;

   if (!PETSc_matrix) return(-1);

   ierr = MatGetType( PETSc_matrix, &PETScType, NULL ); CHKERRA(ierr);

   if (PETScType != MATMPIAIJ) return(-1);

   ierr = MatGetRow( PETSc_matrix, row, size, col_ind, values); CHKERRA(ierr);
#endif

   return(ierr);
}

/*--------------------------------------------------------------------------
 * hypre_RestoreDistributedMatrixRowPETSc
 *--------------------------------------------------------------------------*/

int 
hypre_RestoreDistributedMatrixRowPETSc( hypre_DistributedMatrix *matrix,
                             int row,
                             int *size,
                             int **col_ind,
                             double **values )
{
   int ierr;
#ifdef PETSC_AVAILABLE
   Mat PETSc_matrix = (Mat) hypre_DistributedMatrixLocalStorage(matrix);
   MatType PETScType;

   if (PETSc_matrix == NULL) return(-1);

   ierr = MatGetType( PETSc_matrix, &PETScType, NULL ); CHKERRA(ierr);

   if (PETScType != MATMPIAIJ) return(-1);

   ierr = MatRestoreRow( PETSc_matrix, row, size, col_ind, values); CHKERRA(ierr);
#endif

   return(ierr);
}
