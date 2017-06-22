// Copyright (c) 2017, Joseph Mirabel
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//
// This file is part of hpp-core.
// hpp-core is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-core is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-core. If not, see <http://www.gnu.org/licenses/>.

#define BOOST_TEST_MODULE spline_path
#include <boost/test/included/unit_test.hpp>

#include <hpp/core/path/spline.hh>

#include <pinocchio/algorithm/joint-configuration.hpp>

#include <hpp/pinocchio/joint.hh>
#include <hpp/pinocchio/simple-device.hh>
#include <hpp/pinocchio/configuration.hh>
#include <hpp/pinocchio/liegroup.hh>

#include <hpp/core/problem.hh>
#include <hpp/core/steering-method-straight.hh>
#include <hpp/core/steering-method/spline.hh>

#define TOSTR( x ) static_cast< std::ostringstream & >( ( std::ostringstream() << x ) ).str()

using namespace hpp::core;
using namespace hpp::pinocchio;

DevicePtr_t createRobot ()
{
  DevicePtr_t robot = unittest::makeDevice(unittest::HumanoidRomeo);
  robot->controlComputation((Device::Computation_t) (Device::JOINT_POSITION | Device::JACOBIAN));
  robot->rootJoint()->lowerBound (0, -1);
  robot->rootJoint()->lowerBound (1, -1);
  robot->rootJoint()->lowerBound (2, -1);
  robot->rootJoint()->upperBound (0,  1);
  robot->rootJoint()->upperBound (1,  1);
  robot->rootJoint()->upperBound (2,  1);
  return robot;
}

typedef std::pair<value_type, value_type> Pair_t;

std::ostream& operator<< (std::ostream& os, const Pair_t& p) {
  os << "Pair " << p.first << ", " << p.second;
  return os;
}

void printAt(const PathPtr_t& p, ConfigurationOut_t q, value_type t)
{
  (*p)(q, t);
  std::cout << t << ":\t" << q.transpose() << std::endl;
}
void checkAt(const PathPtr_t orig, value_type to,
             const PathPtr_t extr, value_type te) {
  Configuration_t q1 (orig->outputSize()),
                  q2 (orig->outputSize());
  (*orig)(q1, to);
  (*extr)(q2, te);
  BOOST_CHECK_MESSAGE(q2.isApprox(q1),
      "\nPath 1: " << q1.head<10>().transpose() <<
      "\nPath 2: " << q2.head<10>().transpose()
      );
}

template <int SplineType> void compare_to_straight_path ()
{
  typedef path::Spline<SplineType, 1> path_t;
  typedef steeringMethod::Spline<SplineType, 1> SM_t;

  DevicePtr_t dev = createRobot();
  BOOST_REQUIRE (dev);
  Problem problem (dev);

  Configuration_t q1 (se3::randomConfiguration(dev->model()));
  Configuration_t q2 (se3::randomConfiguration(dev->model()));

  vector_t v (dev->numberDof());
  difference<LieGroupTpl> (dev, q2, q1, v);

  // create StraightPath
  PathPtr_t sp = (*problem.steeringMethod()) (q1, q2);
  // value_type length = sp->length();

  // Create linear spline
  typename SM_t::Ptr_t sm (SM_t::create (problem));
  PathPtr_t ls_abstract = (*sm) (q1, q2);
  typename path_t::Ptr_t ls = HPP_DYNAMIC_PTR_CAST(path_t, ls_abstract);

  /*
  typename path_t::Ptr_t ls = path_t::create (dev, interval_t(0, length), ConstraintSetPtr_t());
  ls->base (q1);
  typename path_t::ParameterMatrix_t ls_param = ls->parameters();
  ls_param.row(0).setZero();
  ls_param.row(1) = v;
  ls->parameters(ls_param);
  */

  BOOST_CHECK(sp->initial().isApprox(ls->initial()));
  BOOST_CHECK(sp->end().isApprox(ls->end()));

  const size_type N = 10;
  const value_type step1 = sp->length() / N;
  const value_type step2 = ls->length() / N;

  // Check that straight path and linear spline return the same result.
  for (size_type i = 0; i < N; ++i)
    checkAt (sp, value_type(i) * step1, ls, value_type(i) * step2);

  // Check that the velocities are equals
  vector_t v1 (dev->numberDof());
  vector_t v2 (dev->numberDof());
  for (size_type i = 0; i < N; ++i) {
    sp->derivative(v1, value_type(i) * step1, 1);
    ls->derivative(v2, value_type(i) * step2, 1);
    BOOST_CHECK_SMALL((v2 * step2 - v1 * step1).squaredNorm(), 1e-12);
  }

  // Check integral value
  BOOST_CHECK_EQUAL(ls->squaredNormIntegral(2), 0);
  BOOST_CHECK_CLOSE(ls->squaredNormIntegral(1), v.squaredNorm(), 1e-12);

  vector_t derivative (dev->numberDof() * 2);
  ls->squaredNormIntegralDerivative(1, derivative);
  switch (SplineType) {
    case path::CanonicalPolynomeBasis:
      BOOST_CHECK( derivative.head(dev->numberDof()).isZero());
      BOOST_CHECK_SMALL((derivative.tail(dev->numberDof()) - 2 * v).squaredNorm(), 1e-12);
      break;
    case path::BernsteinBasis:
      BOOST_CHECK_SMALL((derivative.head(dev->numberDof()) + 2 * v).squaredNorm(), 1e-12);
      BOOST_CHECK_SMALL((derivative.tail(dev->numberDof()) - 2 * v).squaredNorm(), 1e-12);
      break;
  } 
}

BOOST_AUTO_TEST_CASE (spline)
{
  compare_to_straight_path<path::CanonicalPolynomeBasis>();
  compare_to_straight_path<path::BernsteinBasis>();
}
