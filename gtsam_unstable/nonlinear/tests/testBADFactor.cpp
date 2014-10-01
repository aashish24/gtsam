/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file testBADFactor.cpp
 * @date September 18, 2014
 * @author Frank Dellaert
 * @author Paul Furgale
 * @brief unit tests for Block Automatic Differentiation
 */

#include <gtsam/slam/GeneralSFMFactor.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Cal3_S2.h>
#include <gtsam_unstable/nonlinear/BADFactor.h>
#include <gtsam/base/Testable.h>

#include <CppUnitLite/TestHarness.h>

using namespace std;
using namespace gtsam;

/* ************************************************************************* */
// Proposed naming convention
/* ************************************************************************* */
typedef Expression<Point2> Point2_;
typedef Expression<Point3> Point3_;
typedef Expression<Rot3> Rot3_;
typedef Expression<Pose3> Pose3_;
typedef Expression<Cal3_S2> Cal3_S2_;

/* ************************************************************************* */
// Functions that allow creating concise expressions
/* ************************************************************************* */
Point3_ transform_to(const Pose3_& x, const Point3_& p) {
  return Point3_(x, &Pose3::transform_to, p);
}

Point2_ project(const Point3_& p_cam) {
  return Point2_(PinholeCamera<Cal3_S2>::project_to_camera, p_cam);
}

template<class CAL>
Point2_ uncalibrate(const Expression<CAL>& K, const Point2_& xy_hat) {
  return Point2_(K, &CAL::uncalibrate, xy_hat);
}

/* ************************************************************************* */

TEST(BADFactor, test) {

  // Create some values
  Values values;
  values.insert(1, Pose3());
  values.insert(2, Point3(0, 0, 1));
  values.insert(3, Cal3_S2());

  // Create old-style factor to create expected value and derivatives
  Point2 measured(-17, 30);
  SharedNoiseModel model = noiseModel::Unit::Create(2);
  GeneralSFMFactor2<Cal3_S2> old(measured, model, 1, 2, 3);
  double expected_error = old.error(values);
  GaussianFactor::shared_ptr expected = old.linearize(values);

  // Test Constant expression
  Expression<int> c(0);

  // Create leaves
  Pose3_ x(1);
  Point3_ p(2);
  Cal3_S2_ K(3);

  // Create expression tree
  Point3_ p_cam(x, &Pose3::transform_to, p);
  Point2_ xy_hat(PinholeCamera<Cal3_S2>::project_to_camera, p_cam);
  Point2_ uv_hat(K, &Cal3_S2::uncalibrate, xy_hat);

  // Create factor and check value, dimension, linearization
  BADFactor<Point2> f(measured, uv_hat);
  EXPECT_DOUBLES_EQUAL(expected_error, f.error(values), 1e-9);
  EXPECT_LONGS_EQUAL(2, f.dim());
  boost::shared_ptr<GaussianFactor> gf = f.linearize(values);
  EXPECT( assert_equal(*expected, *gf, 1e-9));

  // Try concise version
  BADFactor<Point2> f2(measured, uncalibrate(K, project(transform_to(x, p))));
  EXPECT_DOUBLES_EQUAL(expected_error, f2.error(values), 1e-9);
  EXPECT_LONGS_EQUAL(2, f2.dim());
  boost::shared_ptr<GaussianFactor> gf2 = f2.linearize(values);
  EXPECT( assert_equal(*expected, *gf2, 1e-9));
}

/* ************************************************************************* */

TEST(BADFactor, compose) {

  // Create expression
  Rot3_ R1(1), R2(2);
  Rot3_ R3 = R1 * R2;

  // Create factor
  BADFactor<Rot3> f(Rot3(), R3);

  // Create some values
  Values values;
  values.insert(1, Rot3());
  values.insert(2, Rot3());

  // Check linearization
  JacobianFactor expected(1, eye(3), 2, eye(3), zero(3));
  boost::shared_ptr<GaussianFactor> gf = f.linearize(values);
  boost::shared_ptr<JacobianFactor> jf = //
      boost::dynamic_pointer_cast<JacobianFactor>(gf);
  EXPECT( assert_equal(expected, *jf,1e-9));
}

/* ************************************************************************* */
// Test compose with arguments referring to the same rotation
TEST(BADFactor, compose2) {

  // Create expression
  Rot3_ R1(1), R2(1);
  Rot3_ R3 = R1 * R2;

  // Create factor
  BADFactor<Rot3> f(Rot3(), R3);

  // Create some values
  Values values;
  values.insert(1, Rot3());

  // Check linearization
  JacobianFactor expected(1, 2 * eye(3), zero(3));
  boost::shared_ptr<GaussianFactor> gf = f.linearize(values);
  boost::shared_ptr<JacobianFactor> jf = //
      boost::dynamic_pointer_cast<JacobianFactor>(gf);
  EXPECT( assert_equal(expected, *jf,1e-9));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
