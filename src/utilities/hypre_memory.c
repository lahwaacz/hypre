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

/******************************************************************************
 *
 * Memory management utilities
 *
 *****************************************************************************/

#define HYPRE_USE_MANAGED_SCALABLE 1
#include "_hypre_utilities.h"
//#include "gpgpu.h"
//#include "hypre_nvtx.h"
//#include "gpuMem.h"
#ifdef HYPRE_USE_UMALLOC
#undef HYPRE_USE_UMALLOC
#endif
/******************************************************************************
 *
 * Standard routines
 *
 *****************************************************************************/

/*--------------------------------------------------------------------------
 * hypre_OutOfMemory
 *--------------------------------------------------------------------------*/

HYPRE_Int
hypre_OutOfMemory( size_t size )
{
   hypre_printf("Out of memory trying to allocate %d bytes\n", (HYPRE_Int) size);
   fflush(stdout);

   hypre_error(HYPRE_ERROR_MEMORY);

   return 0;
}

/*--------------------------------------------------------------------------
 * hypre_MAlloc
 *--------------------------------------------------------------------------*/

char *
 hypre_MAlloc( size_t size , HYPRE_Int location)
{
   void *ptr;

   if (size > 0)
   {
     PUSH_RANGE_PAYLOAD("MALLOC",2,size);
#ifdef HYPRE_USE_UMALLOC
      HYPRE_Int threadid = hypre_GetThreadID();
#ifdef HYPRE_USE_MANAGED
      printf("ERROR HYPRE_USE_UMALLOC AND HYPRE_USE_MANAGED are mutually exclusive\n");
#endif
      ptr = _umalloc_(size);
#elif HYPRE_USE_MANAGED
#ifdef HYPRE_USE_MANAGED_SCALABLE
      gpuErrchk( cudaMallocManaged(&ptr,size+sizeof(size_t)*MEM_PAD_LEN,CUDAMEMATTACHTYPE) );
      size_t *sp=(size_t*)ptr;
      *sp=size;
      ptr=(void*)(&sp[MEM_PAD_LEN]);
#else
      gpuErrchk( cudaMallocManaged(&ptr,size,CUDAMEMATTACHTYPE) );
      mempush(ptr,size,0);
#endif
#else
      ptr = malloc(size);
#endif

#if 1
      if (ptr == NULL)
      {
        hypre_OutOfMemory(size);
      }
#endif
      POP_RANGE;
   }
   else
   {
      ptr = NULL;
   }

   return (char*)ptr;
}

/*--------------------------------------------------------------------------
 * hypre_CAlloc
 *--------------------------------------------------------------------------*/

char *
 hypre_CAlloc( size_t count, 
              size_t elt_size , HYPRE_Int location)
{
   void   *ptr;
   size_t  size = count*elt_size;

   if (size > 0)
   {
     PUSH_RANGE_PAYLOAD("MALLOC",4,size);
#ifdef HYPRE_USE_UMALLOC
#ifdef HYPRE_USE_MANAGED
      printf("ERROR HYPRE_USE_UMALLOC AND HYPRE_USE_MANAGED are mutually exclusive\n");
#endif
      HYPRE_Int threadid = hypre_GetThreadID();

      ptr = _ucalloc_(count, elt_size);
#elif HYPRE_USE_MANAGED
#ifdef HYPRE_USE_MANAGED_SCALABLE
      ptr=(void*)hypre_MAlloc(size, HYPRE_MEMORY_HOST);
      memset(ptr,0,count*elt_size);
#else
      gpuErrchk( cudaMallocManaged(&ptr,size,CUDAMEMATTACHTYPE) );
      memset(ptr,0,count*elt_size);
      mempush(ptr,size,0);
#endif
#else
      ptr = calloc(count, elt_size);
#endif

#if 1
      if (ptr == NULL)
      {
        hypre_OutOfMemory(size);
      }
#endif
      POP_RANGE;
   }
   else
   {
      ptr = NULL;
   }

   return(char*) ptr;
}

#ifdef HYPRE_USE_MANAGED
size_t memsize(const void *ptr){
return ((size_t*)ptr)[-MEM_PAD_LEN];
}
#endif
/*--------------------------------------------------------------------------
 * hypre_ReAlloc
 *--------------------------------------------------------------------------*/

char *
 hypre_ReAlloc( char   *ptr, 
               size_t  size,
               HYPRE_Int location)
{
#ifdef HYPRE_USE_UMALLOC
   if (ptr == NULL)
   {
      ptr = hypre_MAlloc(size, HYPRE_MEMORY_HOST);
   }
   else if (size == 0)
   {
      hypre_Free(ptr, HYPRE_MEMORY_HOST);
   }
   else
   {
      HYPRE_Int threadid = hypre_GetThreadID();
      ptr = (char*)_urealloc_(ptr, size);
   }
#elif HYPRE_USE_MANAGED
   if (ptr == NULL)
   {

      ptr = hypre_MAlloc(size, HYPRE_MEMORY_HOST);
   }
   else if (size == 0)
   {
     hypre_Free(ptr, HYPRE_MEMORY_HOST);
     return NULL;
   }
   else
   {
     void *nptr = hypre_MAlloc(size, HYPRE_MEMORY_HOST);
#ifdef HYPRE_USE_MANAGED_SCALABLE
     size_t old_size=memsize((void*)ptr);
#else
     size_t old_size=mempush((void*)ptr,0,0);
#endif
     if (size>old_size)
       memcpy(nptr,ptr,old_size);
     else
       memcpy(nptr,ptr,size);
     hypre_Free(ptr, HYPRE_MEMORY_HOST);
     ptr=(char*) nptr;
   }
#else
   if (ptr == NULL)
   {
	   ptr = (char*)malloc(size);
   }
   else
   {
	   ptr = (char*)realloc(ptr, size);
   }
#endif

#if 1
   if ((ptr == NULL) && (size > 0))
   {
      hypre_OutOfMemory(size);
   }
#endif

   return ptr;
}


/*--------------------------------------------------------------------------
 * hypre_Free
 *--------------------------------------------------------------------------*/

void
 hypre_Free( char *ptr ,
            HYPRE_Int location)
{
   if (ptr)
   {
#ifdef HYPRE_USE_UMALLOC
      HYPRE_Int threadid = hypre_GetThreadID();

      _ufree_(ptr);
#elif HYPRE_USE_MANAGED
      //size_t size=mempush(ptr,0,0);
#ifdef HYPRE_USE_MANAGED_SCALABLE
      cudaSafeFree(ptr,MEM_PAD_LEN);
#else
      mempush(ptr,0,1);
      cudaSafeFree(ptr,0);
#endif
      //gpuErrchk(cudaFree((void*)ptr));
#else
      free(ptr);
#endif
   }
}

/*--------------------------------------------------------------------------
 * hypre_Memcpy
 *--------------------------------------------------------------------------*/

void
hypre_Memcpy( char *dst,
	      char *src,
	      size_t size,
	      HYPRE_Int locdst,
	      HYPRE_Int locsrc )
{
   if (src)
   {
	  if ( locdst==HYPRE_MEMORY_DEVICE && locsrc==HYPRE_MEMORY_DEVICE )
	  {
		 if (dst != src)
		 {
#if defined(HYPRE_USE_MANAGED)
			memcpy( dst, src, size);
#elif defined(HYPRE_MEMORY_GPU)
			cudaMemcpy( dst, src, size, cudaMemcpyDeviceToDevice);
#elif defined(HYPRE_USE_OMP45)
	
#else
			memcpy( dst, src, size);
#endif
		 }
		 else
		 {
			dst = src;
		 }
	  }
	  else if ( locdst==HYPRE_MEMORY_DEVICE && locsrc==HYPRE_MEMORY_HOST )
	  {
#if defined(HYPRE_USE_MANAGED)
		 memcpy( dst, src, size);
#elif defined(HYPRE_MEMORY_GPU)
		 cudaMemcpy( dst, src, size, cudaMemcpyHostToDevice);
#elif defined(HYPRE_USE_OMP45)
		 //hypre_omp45_offload(device_num, dst, src, type, 0, count, "update", "to");
#else
		 memcpy( dst, src, size);
#endif        
	  }
	  else if ( locdst==HYPRE_MEMORY_HOST && locsrc==HYPRE_MEMORY_DEVICE )
	  {
#if defined(HYPRE_USE_MANAGED)
		 memcpy( dst, src, size);
#elif defined(HYPRE_MEMORY_GPU)
		 cudaMemcpy( dst, src, size, cudaMemcpyDeviceToHost);
#elif defined(HYPRE_USE_OMP45)
		 //hypre_omp45_offload(device_num, dst, src, type, 0, count, "update", "from");
#else
		 memcpy( dst, src, size);
#endif
	  }
	  else if ( locdst==HYPRE_MEMORY_HOST && locsrc==HYPRE_MEMORY_HOST )
	  {
		  if (dst != src)
		  {
			 memcpy( dst, src, size);
		  }
		  else
		  {
			  dst = src;
		  }
	  }
	  else
	  {
		  hypre_printf("Wrong memory location.\n");
		  fflush(stdout);
		  hypre_error(HYPRE_ERROR_MEMORY);
	  }
   }
}

/*--------------------------------------------------------------------------
 * hypre_MemcpyAsync
 *--------------------------------------------------------------------------*/

void
hypre_MemcpyAsync( char *dst,
		   char *src,
		   size_t size,
		   HYPRE_Int locdst,
		   HYPRE_Int locsrc )
{
   if (src)
   {
     if ( locdst==HYPRE_MEMORY_DEVICE && locsrc==HYPRE_MEMORY_DEVICE )
     {
        if (dst != src)
        {
#if defined(HYPRE_USE_MANAGED)
	   cudaMemcpyAsync( dst, src, size, cudaMemcpyDefault); 
#elif defined(HYPRE_MEMORY_GPU)
	   cudaMemcpyAsync( dst, src, size, cudaMemcpyDeviceToDevice);
#elif defined(HYPRE_USE_OMP45)
	
#else
	   memcpy( dst, src, size);
#endif
	}
	else
        {
	  /* Prefetch the data to GPU */
#if defined(HYPRE_USE_MANAGED)
	   HYPRE_Int device = -1;
	   cudaGetDevice(&device);
	   cudaMemPrefetchAsync(x, size, device, NULL);
#endif	   
	}
     }
     else if ( locdst==HYPRE_MEMORY_DEVICE && locsrc==HYPRE_MEMORY_HOST )
     {
#if defined(HYPRE_USE_MANAGED)
        cudaMemcpyAsync( dst, src, size, cudaMemcpyDefault); 
#elif defined(HYPRE_MEMORY_GPU)
	cudaMemcpyAsync( dst, src, size, cudaMemcpyHostToDevice);
#elif defined(HYPRE_USE_OMP45)
	//hypre_omp45_offload(device_num, dst, src, type, 0, count, "update", "to");
#else
	memcpy( dst, src, size);
#endif        
     }
     else if ( locdst==HYPRE_MEMORY_HOST && locsrc==HYPRE_MEMORY_DEVICE )
     {
#if defined(HYPRE_USE_MANAGED)
        cudaMemcpyAsync( dst, src, size, cudaMemcpyDefault); 
#elif defined(HYPRE_MEMORY_GPU)
	cudaMemcpyAsync( dst, src, size, cudaMemcpyDeviceToHost);
#elif defined(HYPRE_USE_OMP45)
	//hypre_omp45_offload(device_num, dst, src, type, 0, count, "update", "from");
#else
	memcpy( dst, src, size);
#endif
     }
     else if ( locdst==HYPRE_MEMORY_HOST && locsrc==HYPRE_MEMORY_HOST )
     {
        if (dst != src)
        {
	   memcpy( dst, src, size);
	}
	else
	{
	   dst = src;
	}
     }
     else
     {
         hypre_printf("Wrong memory location. Only HYPRE_LOCATION_DEVICE and HYPRE_LOCATION_HOST are avaible\n");
	 fflush(stdout);
	 hypre_error(HYPRE_ERROR_MEMORY);
     }
   }
}

/*--------------------------------------------------------------------------
 * hypre_MAllocPinned
 *--------------------------------------------------------------------------*/

char *
hypre_MAllocPinned( size_t size )
{
   void *ptr;

   if (size > 0)
   {
     PUSH_RANGE_PAYLOAD("MALLOC",2,size);
#ifdef HYPRE_USE_UMALLOC
      HYPRE_Int threadid = hypre_GetThreadID();
#ifdef HYPRE_USE_MANAGED
      printf("ERROR HYPRE_USE_UMALLOC AND HYPRE_USE_MANAGED are mutually exclusive\n");
#endif
      ptr = _umalloc_(size);
#elif HYPRE_USE_MANAGED
#ifdef HYPRE_USE_MANAGED_SCALABLE
#ifdef HYPRE_GPU_USE_PINNED
      gpuErrchk( cudaHostAlloc(&ptr,size+sizeof(size_t)*MEM_PAD_LEN,cudaHostAllocMapped));
#else
      gpuErrchk( cudaMallocManaged(&ptr,size+sizeof(size_t)*MEM_PAD_LEN,CUDAMEMATTACHTYPE) );
#endif
      size_t *sp=(size_t*)ptr;
      *sp=size;
      ptr=(void*)(&sp[MEM_PAD_LEN]);
#else
      gpuErrchk( cudaMallocManaged(&ptr,size,CUDAMEMATTACHTYPE) );
      mempush(ptr,size,0);
#endif
#else
      ptr = malloc(size);
#endif

#if 1
      if (ptr == NULL)
      {
        hypre_OutOfMemory(size);
      }
#endif
      POP_RANGE;
   }
   else
   {
      ptr = NULL;
   }

   return (char*)ptr;
}
