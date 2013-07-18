
#ifndef AURA_BACKEND_CUDA_DEVICE_HPP
#define AURA_BACKEND_CUDA_DEVICE_HPP


#include <boost/noncopyable.hpp>
#include <cuda.h>
#include <aura/backend/cuda/call.hpp>

namespace aura {
namespace backend_detail {
namespace cuda {

/**
 * device class
 *
 * every interaction with devices starts from this class
 */
class device : private boost::noncopyable {

public:
  /**
   * create device form ordinal, also creates a context
   *
   * @param ordinal device number
   */
  inline device(int ordinal) : pinned_(false) {
    AURA_CUDA_SAFE_CALL(cuDeviceGet(&device_, ordinal));
    AURA_CUDA_SAFE_CALL(cuCtxCreate(&context_, 0, device_));
  }

  /**
   * destroy device (context)
   */
  inline ~device() {
    AURA_CUDA_SAFE_CALL(cuCtxDestroy(context_));
  }
 
  /// make device active
  inline void set() const {
    AURA_CUDA_SAFE_CALL(cuCtxSetCurrent(context_));
  }
  
  /// undo make device active
  inline void unset() const {
    if(pinned_) {
      return;
    }
    AURA_CUDA_SAFE_CALL(cuCtxSetCurrent(NULL));
  }

  /**
   * pin 
   *
   * disable unset, device context stays associated with current thread
   * usefull for interoperability with other libraries that use a context
   * explicitly
   */
  inline void pin() {
    set();
    pinned_ = true;
  }
  
  /// unpin (reenable unset)
  inline void unpin() {
    pinned_ = false;
  } 

  /// access the device handle
  inline const CUdevice & get_device() const {
    return device_; 
  }
  
  /// access the context handle
  inline const CUcontext & get_context() const {
    return context_; 
  }

private:
  /// device handle
  CUdevice device_;
  /// context handle 
  CUcontext context_;
  /// flag indicating pinned or unpinned context
  bool pinned_;
};
  
/**
 * get number of devices available
 *
 * @return number of devices
 */
inline int device_get_count() {
  int num_devices;
  AURA_CUDA_SAFE_CALL(cuDeviceGetCount(&num_devices));
  return num_devices;
}


} // cuda
} // backend_detail
} // aura

#endif // AURA_BACKEND_CUDA_DEVICE_HPP

