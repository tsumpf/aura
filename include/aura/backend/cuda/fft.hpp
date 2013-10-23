#ifndef AURA_BACKEND_CUDA_FFT_HPP
#define AURA_BACKEND_CUDA_FFT_HPP

#include <boost/move/move.hpp>
#include <cuda.h>
#include <cufft.h>
#include <aura/backend/cuda/call.hpp>
#include <aura/backend/cuda/memory.hpp>
#include <aura/detail/svec.hpp>
#include <aura/backend/cuda/device.hpp>

namespace aura {
namespace backend_detail {
namespace cuda {

typedef int fft_size;
typedef svec<fft_size, 3> fft_dim;
typedef svec<fft_size, 3> fft_embed;

/**
 * fft class
 */
class fft {

private:
  BOOST_MOVABLE_BUT_NOT_COPYABLE(fft)

public:
 
  enum type {
    r2c,  // real to complex  
    c2r,  // complex to real 
    c2c,  // complex to complex  
    d2z,  // double to double-complex 
    z2d,  // double-complex to double 
    z2z   // double-complex to double-complex
  };

  enum direction {
    fwd = CUFFT_FORWARD,
    inv = CUFFT_INVERSE
  };

  /**
   * create empty fft object without device and stream
   */
  inline explicit fft() : empty_(true) { 
  }

  /**
   * create fft 
   *
   * @param d device to create fft for
   */
  inline explicit fft(device & d, const fft_dim & dim, const fft::type & type,
    std::size_t batch = 1, 
    const fft_embed & iembed = fft_embed(),
    std::size_t istride = 1, std::size_t idist = 0,
    const fft_embed & oembed = fft_embed(),
    std::size_t ostride = 1, std::size_t odist = 0) : 
    device_(&d), type_(type), empty_(false)
  {
    device_->set();
    AURA_CUFFT_SAFE_CALL(
      cufftPlanMany(
        &handle_, 
        dim.size(), 
        const_cast<int*>(&dim[0]),
        0 == iembed.size() ? NULL : const_cast<int*>(&iembed[0]),
        istride, 
        0 == idist ? product(dim) : idist,
        0 == oembed.size() ? NULL : const_cast<int*>(&oembed[0]),
        ostride, 
        0 == odist ? product(dim) : odist,
        map_type(type), 
        batch 
      )
    ); 
    device_->unset(); 
  }

  /**
   * move constructor, move fft information here, invalidate other
   *
   * @param f fft to move here
   */
  fft(BOOST_RV_REF(fft) f) :
    device_(f.device_), handle_(f.handle_), type_(f.type_), empty_(false)
  { 
    f.empty_ = true;
  }

  /**
   * move assignment, move fft information here, invalidate other
   *
   * @param f fft to move here
   */
  fft& operator=(BOOST_RV_REF(fft) f)
  {
    device_ = f.device_;
    handle_ = f.handle_;
    type_ = f.type_;
    empty_ = 0;
    f.empty_ = 1; 
    return *this;
  }

  /**
   * destroy fft
   */
  inline ~fft() {
    if(empty_) {
      return;
    }
    device_->set();
    AURA_CUFFT_SAFE_CALL(cufftDestroy(handle_));
    device_->unset();
  }

  /**
   * set feed
   */
  void set_feed(const feed & f) {
    AURA_CUFFT_SAFE_CALL(cufftSetStream(handle_, f.get_stream()));
  }

  /**
   * return fft handle
   */
  const cufftHandle & get_handle() const {
    return handle_;
  }
  
  /**
   * return fft type
   */
  const type & get_type() const {
    return type_;
  }

  /// map fft type to cufftType
  cufftType map_type(fft::type type) {
    switch(type) {
      case r2c: 
        return CUFFT_R2C;
      case c2r: 
        return CUFFT_C2R;
      case c2c: 
        return CUFFT_C2C;
      case d2z: 
        return CUFFT_D2Z;
      case z2d: 
        return CUFFT_Z2D;
      case z2z: 
        return CUFFT_Z2Z;
      default:
        return (cufftType)0;
    }
  }

protected:
  /// device handle
  device * device_;
  
private:
  /// fft handle
  cufftHandle handle_;

  /// fft type
  type type_;

  /// empty marker
  int empty_;

  // give free functions access to device
  friend void fft_forward(memory & dst, memory & src, 
    fft & plan, const feed & f);
  friend void fft_inverse(memory & dst, memory & src, 
    fft & plan, const feed & f);

};

/// initialize fft library
inline void fft_initialize() {
}
/// finalize fft library and release all associated resources
inline void fft_terminate() {
}

/**
 * @brief calculate forward fourier transform
 * 
 * @param dst pointer to result of fourier transform
 * @param src pointer to input of fourier transform
 * @param plan that is used to calculate the fourier transform
 * @param f feed the fourier transform should be calculated in
 */
inline void fft_forward(memory & dst, memory & src, 
  fft & plan, const feed & f) {
  plan.device_->set();
  plan.set_feed(f);
  switch(plan.get_type()) {
    case fft::type::r2c: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecR2C(
          plan.get_handle(), (cufftReal*)src, (cufftComplex*)dst)
      );
      break;
    }
    case fft::type::c2r: {
      assert(false); // FIXME 
      break;
    }
    case fft::type::c2c: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecC2C(
          plan.get_handle(), 
          (cufftComplex*)src, 
          (cufftComplex*)dst, 
          fft::direction::fwd)
      );
      break;
    }
    case fft::type::d2z: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecD2Z(
          plan.get_handle(), (cufftDoubleReal*)src, (cufftDoubleComplex*)dst)
      );
      break;
    }
    case fft::type::z2d: {
      assert(false); // FIXME 
      break;
    }
    case fft::type::z2z: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecZ2Z(
          plan.get_handle(), 
          (cufftDoubleComplex*)src, 
          (cufftDoubleComplex*)dst,
          fft::direction::fwd)
      );
      break;
    }    
  }
  plan.device_->unset();
}


/**
 * @brief calculate forward fourier transform
 * 
 * @param dst pointer to result of fourier transform
 * @param src pointer to input of fourier transform
 * @param plan that is used to calculate the fourier transform
 * @param f feed the fourier transform should be calculated in
 */
inline void fft_inverse(memory & dst, memory & src, 
  fft & plan, const feed & f) {
  plan.device_->set();
  plan.set_feed(f);
  switch(plan.get_type()) {
    case fft::type::r2c: {
      assert(false); // FIXME
      break;
    }
    case fft::type::c2r: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecC2R(
          plan.get_handle(), (cufftComplex*)src, (cufftReal*)dst)
      );
      break;
    }
    case fft::type::c2c: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecC2C(
          plan.get_handle(), 
          (cufftComplex*)src, 
          (cufftComplex*)dst, 
          fft::direction::inv)
      );
      break;
    }
    case fft::type::d2z: {
      assert(false); // FIXME
      break;
    }
    case fft::type::z2d: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecZ2D(
          plan.get_handle(), (cufftDoubleComplex*)src, (cufftDoubleReal*)dst)
      );
      break;
    }
    case fft::type::z2z: {
      AURA_CUFFT_SAFE_CALL(
        cufftExecZ2Z(
          plan.get_handle(), 
          (cufftDoubleComplex*)src, 
          (cufftDoubleComplex*)dst,
          fft::direction::inv)
      );
      break;
    }    
  }
  plan.device_->unset();
}
} // cuda
} // backend_detail
} // aura

#endif // AURA_BACKEND_CUDA_FFT_HPP

