#ifndef AURA_BACKEND_CUDA_CALL_HPP
#define AURA_BACKEND_CUDA_CALL_HPP

#include <stdio.h>

/// check if a call returned error and throw exception if it did
#define AURA_CUDA_SAFE_CALL(call) { \
  CUresult err = call; \
  if (err != CUDA_SUCCESS) { \
    printf("CUDA error %d at %s:%d\n", err, __FILE__, __LINE__ ); \
  } \
} \
/**/

/// check if a call returned error and throw exception if it did
#define AURA_CUFFT_SAFE_CALL(call) { \
  cufftResult err = call; \
  if (err != CUFFT_SUCCESS) { \
    printf("CUFFT error %d at %s:%d\n", err, __FILE__, __LINE__ ); \
  } \
} \
/**/


/// check for error and throw exception if true 
#define AURA_CUDA_CHECK_ERROR(err) { \
  if (err != CUDA_SUCCESS) { \
    printf("CUDA error %d at %s:%d\n", err, __FILE__, __LINE__ ); \
  } \
} \
/**/

#endif // AURA_BACKEND_CUDA_CALL_HPP

