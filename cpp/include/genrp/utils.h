#ifndef _GENRP_UTILS_H_
#define _GENRP_UTILS_H_

#include <cmath>
#include <vector>
#include <Eigen/Core>

#include "genrp/poly.h"

namespace genrp {

template <typename T1, typename T2>
inline bool isclose (const T1& a, const T2& b) {
  using std::abs;
  return (abs(a - b) <= 1e-6);
}

template <typename T>
T _logsumexp (const T& a, const T& b) {
  return b + log(T(1.0) + exp(a - b));
}

template <typename Derived>
bool check_coefficients (
  const Eigen::DenseBase<Derived>& alpha_real,
  const Eigen::DenseBase<Derived>& beta_real,
  const Eigen::DenseBase<Derived>& alpha_complex_real,
  const Eigen::DenseBase<Derived>& alpha_complex_imag,
  const Eigen::DenseBase<Derived>& beta_complex_real,
  const Eigen::DenseBase<Derived>& beta_complex_imag
) {
  typedef typename Derived::Scalar T;
  typedef Eigen::Matrix<T, Eigen::Dynamic, 1> vector_t;

  if (alpha_real.rows() != beta_real.rows()) return false;
  if (alpha_complex_real.rows() != alpha_complex_imag.rows()) return false;
  if (alpha_complex_real.rows() != beta_complex_real.rows()) return false;
  if (alpha_complex_real.rows() != beta_complex_imag.rows()) return false;

  // Start by building the polynomials for each term.
  int n = alpha_real.rows() + alpha_complex_real.rows();
  std::vector<vector_t> num(n), denom(n);
  T a, b, c, d, c2, d2, w0;
  int k = 0;
  for (int i = 0; i < alpha_real.rows(); ++i, ++k) {
    a = alpha_real[i];
    c = beta_real[i];
    c2 = c*c;

    num[k].resize(2);
    num[k][0] = a*c;
    num[k][1] = a*c*c2;

    denom[k].resize(3);
    denom[k][0] = T(1.0);
    denom[k][1] = 2.0*c2;
    denom[k][2] = c2*c2;
  }

  for (int i = 0; i < alpha_complex_real.rows(); ++i, ++k) {
    a = alpha_complex_real[i];
    b = alpha_complex_imag[i];
    c = beta_complex_real[i];
    d = beta_complex_imag[i];
    c2 = c*c;
    d2 = d*d;
    w0 = c2 + d2;

    num[k].resize(2);
    num[k][0] = a*c - b*d;
    num[k][1] = (a*c + b*d) * w0;

    denom[k].resize(3);
    denom[k][0] = T(1.0);
    denom[k][1] = 2.0*(c2 - d2);
    denom[k][2] = w0 * w0;
  }

  // Compute the full numerator.
  vector_t poly = vector_t::Zero(1), tmp;
  for (int i = 0; i < n; ++i) {
    tmp = num[i];
    for (int j = 0; j < n; ++j) {
      if (i != j) tmp = polymul(tmp, denom[j]);
    }
    poly = polyadd(poly, tmp);
  }

  if (polyval(poly, 0.0) < 0.0) return false;

  // Count the number of roots.
  int nroots = polycountroots(poly);
  return (nroots == 0);
}

template <typename Derived>
typename Derived::Scalar get_kernel_value (
  const Eigen::DenseBase<Derived>& alpha_real,
  const Eigen::DenseBase<Derived>& beta_real,
  const Eigen::DenseBase<Derived>& alpha_complex_real,
  const Eigen::DenseBase<Derived>& alpha_complex_imag,
  const Eigen::DenseBase<Derived>& beta_complex_real,
  const Eigen::DenseBase<Derived>& beta_complex_imag,
  typename Derived::Scalar tau
) {
  using std::abs;
  typedef typename Derived::Scalar T;

  T t = abs(tau), k = T(0.0);

  for (int i = 0; i < alpha_real.rows(); ++i)
    k += alpha_real[i] * exp(-beta_real[i] * t);

  for (int i = 0; i < alpha_complex_real.rows(); ++i)
    k += exp(-beta_complex_real[i] * t) * (
      alpha_complex_real[i] * cos(beta_complex_imag[i] * t) +
      alpha_complex_imag[i] * sin(beta_complex_imag[i] * t)
    );

  return k;
}

template <typename Derived>
typename Derived::Scalar get_psd_value (
  const Eigen::DenseBase<Derived>& alpha_real,
  const Eigen::DenseBase<Derived>& beta_real,
  const Eigen::DenseBase<Derived>& alpha_complex_real,
  const Eigen::DenseBase<Derived>& alpha_complex_imag,
  const Eigen::DenseBase<Derived>& beta_complex_real,
  const Eigen::DenseBase<Derived>& beta_complex_imag,
  typename Derived::Scalar& omega
) {
  typedef typename Derived::Scalar T;
  T w2 = omega * omega, p = T(0.0), a, b, c, d, w02;
  for (int i = 0; i < alpha_real.rows(); ++i) {
    a = alpha_real[i];
    c = beta_real[i];
    p += a*c / (c*c + w2);
  }

  for (int i = 0; i < alpha_complex_real.rows(); ++i) {
    a = alpha_complex_real[i];
    b = alpha_complex_imag[i];
    c = beta_complex_real[i];
    d = beta_complex_imag[i];
    w02 = c*c+d*d;
    p += ((a*c+b*d)*w02+(a*c-b*d)*w2) / (w2*w2 + 2.0*(c*c-d*d)*w2+w02*w02);
  }

  return sqrt(2.0 / M_PI) * p;
}

template <typename T>
bool check_coefficients (
  size_t p_real,
  const T* const alpha_real,
  const T* const beta_real,
  size_t p_complex,
  const T* const alpha_complex_real,
  const T* const alpha_complex_imag,
  const T* const beta_complex_real,
  const T* const beta_complex_imag
) {
  typedef Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1> > vector_t;
  return check_coefficients(
    vector_t(alpha_real, p_real, 1),
    vector_t(beta_real, p_real, 1),
    vector_t(alpha_complex_real, p_complex, 1),
    vector_t(alpha_complex_imag, p_complex, 1),
    vector_t(beta_complex_real, p_complex, 1),
    vector_t(beta_complex_imag, p_complex, 1)
  );
}

template <typename T>
T get_kernel_value (
  size_t p_real,
  const T* const alpha_real,
  const T* const beta_real,
  size_t p_complex,
  const T* const alpha_complex_real,
  const T* const alpha_complex_imag,
  const T* const beta_complex_real,
  const T* const beta_complex_imag,
  T tau
) {
  typedef Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1> > vector_t;
  return get_kernel_value(
    vector_t(alpha_real, p_real, 1),
    vector_t(beta_real, p_real, 1),
    vector_t(alpha_complex_real, p_complex, 1),
    vector_t(alpha_complex_imag, p_complex, 1),
    vector_t(beta_complex_real, p_complex, 1),
    vector_t(beta_complex_imag, p_complex, 1),
    tau
  );
}

template <typename T>
T get_psd_value (
  size_t p_real,
  const T* const alpha_real,
  const T* const beta_real,
  size_t p_complex,
  const T* const alpha_complex_real,
  const T* const alpha_complex_imag,
  const T* const beta_complex_real,
  const T* const beta_complex_imag,
  T omega
) {
  typedef Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1> > vector_t;
  return get_psd_value(
    vector_t(alpha_real, p_real, 1),
    vector_t(beta_real, p_real, 1),
    vector_t(alpha_complex_real, p_complex, 1),
    vector_t(alpha_complex_imag, p_complex, 1),
    vector_t(beta_complex_real, p_complex, 1),
    vector_t(beta_complex_imag, p_complex, 1),
    omega
  );
}

};

#endif
