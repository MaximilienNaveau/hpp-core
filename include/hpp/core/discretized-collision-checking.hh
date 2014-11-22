//
// Copyright (c) 2014 CNRS
// Authors: Florent Lamiraux
//
// This file is part of hpp-core
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
// hpp-core  If not, see
// <http://www.gnu.org/licenses/>.

#ifndef HPP_CORE_DISCRETIZED_COLLISION_CHECKING
# define HPP_CORE_DISCRETIZED_COLLISION_CHECKING

# include <hpp/core/collision-path-validation-report.hh>
# include <hpp/core/path-validation.hh>

namespace hpp {
  namespace core {
    /// Validation of path by collision checking at discretized parameter values
    ///
    /// Should be replaced soon by a better algorithm
    class HPP_CORE_DLLAPI DiscretizedCollisionChecking : public PathValidation
    {
    public:
      static DiscretizedCollisionCheckingPtr_t
      create (const DevicePtr_t& robot, const value_type& stepSize);
      /// Compute the largest valid interval starting from the path beginning
      ///
      /// \param path the path to check for validity,
      /// \param reverse if true check from the end,
      /// \retval the extracted valid part of the path, pointer to path if
      ///         path is valid.
      /// \retval report information about the validation process. The type
      ///         can be derived for specific implementation
      /// \return whether the whole path is valid.
      /// \precond validationReport should be a of type
      ///          CollisionPathValidationReport.
      virtual bool validate (const PathPtr_t& path, bool reverse,
			     PathPtr_t& validPart);

      /// Compute the largest valid interval starting from the path beginning
      ///
      /// \param path the path to check for validity,
      /// \param reverse if true check from the end,
      /// \retval the extracted valid part of the path, pointer to path if
      ///         path is valid.
      /// \retval report information about the validation process. The type
      ///         can be derived for specific implementation
      /// \return whether the whole path is valid.
      /// \retval validationReport information about the validation process:
      ///         which objects have been detected in collision and at which
      ///         parameter along the path.
      /// \precond validationReport should be a of type
      ///          CollisionPathValidationReport.
      virtual bool validate (const PathPtr_t& path, bool reverse,
			     PathPtr_t& validPart,
			     ValidationReport& validationReport);
      /// Add an obstacle
      /// \param object obstacle added
      virtual void addObstacle (const CollisionObjectPtr_t&);
    protected:
      DiscretizedCollisionChecking (const DevicePtr_t& robot,
				    const value_type& stepSize);
    private:
      DevicePtr_t robot_;
      CollisionValidationPtr_t collisionValidation_;
      value_type stepSize_;
      /// This member is used by the validate method that does not take a
      /// validation report as input to call the validate method that expects
      /// a validation report as input. This is not fully satisfactory, but
      /// I did not find a better solution.
      CollisionPathValidationReport unusedReport_;
    }; // class DiscretizedCollisionChecking
  } // namespace core
} // namespace hpp

#endif // HPP_CORE_DISCRETIZED_COLLISION_CHECKING
