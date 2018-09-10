// Copyright (c) 2016, LAAS-CNRS
// Authors: Pierre Fernbach (pierre.fernbach@laas.fr)
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

#include <hpp/util/debug.hh>
#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/configuration.hh>
#include <hpp/pinocchio/joint.hh>
#include <hpp/core/config-projector.hh>
#include <hpp/core/kinodynamic-path.hh>
#include <hpp/core/projection-error.hh>

namespace hpp {
  namespace core {
    KinodynamicPath::KinodynamicPath (const DevicePtr_t& device,
                                      ConfigurationIn_t init,
                                      ConfigurationIn_t end,
                                      value_type length, ConfigurationIn_t a1,ConfigurationIn_t t0, ConfigurationIn_t t1, ConfigurationIn_t tv, ConfigurationIn_t t2, ConfigurationIn_t vLim) :
      parent_t (device,init,end,length),
      a1_(a1),t0_(t0),t1_(t1),tv_(tv),t2_(t2),vLim_(vLim)
    {
      assert (device);
      assert (length >= 0);
      assert (!constraints ());
      hppDout(notice,"Create kinodynamic path with values : ");
      hppDout(notice,"a1 = "<<pinocchio::displayConfig(a1_));
      hppDout(notice,"t0 = "<<pinocchio::displayConfig(t0_));
      hppDout(notice,"t1 = "<<pinocchio::displayConfig(t1_));
      hppDout(notice,"tv = "<<pinocchio::displayConfig(tv_));
      hppDout(notice,"t2 = "<<pinocchio::displayConfig(t2_));
      hppDout(notice,"length = "<<length);
      hppDout(notice,"vLim = "<<pinocchio::displayConfig(vLim_));
      
      // for now, this class only deal with the translation part of a freeflyer :
      assert(a1.size()==3 && t0.size()==3 && t1.size()==3 && tv.size()==3 && t2.size()==3 && vLim.size()==3 && "Inputs vector of kinodynamicPath are not of size 3");
      for(size_t i = 0 ; i < 3 ; i++){
        assert(fabs(length - (t0[i] + t1[i] + tv[i] + t2[i])) < std::numeric_limits <float>::epsilon ()
               && "Kinodynamic path : length is not coherent with switch times at index "+i);
      }
      
    }
    
    KinodynamicPath::KinodynamicPath (const DevicePtr_t& device,
                                      ConfigurationIn_t init,
                                      ConfigurationIn_t end,
                                      value_type length, ConfigurationIn_t a1, ConfigurationIn_t t0, ConfigurationIn_t t1, ConfigurationIn_t tv, ConfigurationIn_t t2, ConfigurationIn_t vLim,
                                      ConstraintSetPtr_t constraints) :
      parent_t (device,init,end,length,constraints),
      a1_(a1),t0_(t0),t1_(t1),tv_(tv),t2_(t2),vLim_(vLim)
    {
      assert (device);
      assert (length >= 0);
      hppDout(notice,"Create kinodynamic path with constraints, with values : ");
      hppDout(notice,"a1 = "<<pinocchio::displayConfig(a1_));
      hppDout(notice,"t0 = "<<pinocchio::displayConfig(t0_));
      hppDout(notice,"t1 = "<<pinocchio::displayConfig(t1_));
      hppDout(notice,"tv = "<<pinocchio::displayConfig(tv_));
      hppDout(notice,"t2 = "<<pinocchio::displayConfig(t2_));
      hppDout(notice,"length = "<<length);
      hppDout(notice,"vLim = "<<pinocchio::displayConfig(vLim_));

      // for now, this class only deal with the translation part of a freeflyer :
      assert(a1.size()==3 && t0.size()==3 && t1.size()==3 && tv.size()==3 && t2.size()==3 && vLim.size()==3 && "Inputs vector of kinodynamicPath are not of size 3");
      for(size_t i = 0 ; i < 3 ; i++){
        assert(fabs(length - (t0[i] + t1[i] + tv[i] + t2[i])) < std::numeric_limits <float>::epsilon ()
               && "Kinodynamic path : length is not coherent with switch times");
      }

    }
    
    KinodynamicPath::KinodynamicPath (const KinodynamicPath& path) :
      parent_t (path),a1_(path.a1_),t0_(path.t0_),t1_(path.t1_),tv_(path.tv_),t2_(path.t2_),vLim_(path.vLim_)
    {
    }
    
    KinodynamicPath::KinodynamicPath (const KinodynamicPath& path,
                                      const ConstraintSetPtr_t& constraints) :
      parent_t (path, constraints),
      a1_(path.a1_),t0_(path.t0_),t1_(path.t1_),tv_(path.tv_),t2_(path.t2_),vLim_(path.vLim_)
    {
      assert (constraints->apply (initial_));
      assert (constraints->apply (end_));
      assert (constraints->isSatisfied (initial_));
      assert (constraints->isSatisfied (end_));
    }
    
    bool KinodynamicPath::impl_compute (ConfigurationOut_t result,
                                        value_type t) const
    {
      assert (result.size() == device()->configSize());
      
      if (t == timeRange ().first || timeRange ().second == 0) {
        result = initial_;
        return true;
      }
      if (t == timeRange ().second) {
        result = end_;
        return true;
      }
      
      size_type configSize = device()->configSize() - device()->extraConfigSpace().dimension ();      
      // const JointVector_t& jv (device()->getJointVector ());
      double v2,t2,t1,tv;
      size_type indexVel;
      size_type indexAcc;
      // straight path for all the joints, except the translations of the base :
      value_type u = t/timeRange ().second;
      if (timeRange ().second == 0)
        u = 0;

      pinocchio::interpolate (device_,initial_, end_, u, result);
      //hppDout(notice,"path : initial = "<<pinocchio::displayConfig(initial_));

      for(int id = 0 ; id < 3 ; id++){ // FIX ME : only work for freeflyer (translation part)
      //for (model::JointVector_t::const_iterator itJoint = jv.begin (); itJoint != jv.end (); itJoint++) {
        // size_type id = (*itJoint)->rankInConfiguration ();
        // size_type indexVel = (*itJoint)->rankInVelocity() + configSize;
        indexVel = id + configSize;
        indexAcc = id + configSize + 3;
        
        //hppDout(notice," PATH For joint :"<<id<<" ; t = "<<t);
       // hppDout(notice,"PATH for joint :"<<device()->getJointAtConfigRank(id)->name());
        if(device()->getJointAtConfigRank(id)->name() != "base_joint_SO3"){          
          
          //if((*itJoint)->configSize() >= 1){
          // 3 case (each segment of the trajectory) : 
          if(t <= t0_[id]){ // before first segment
            //hppDout(info,"before 1° segment");
            result[id] = initial_[id] + t*initial_[indexVel];
            result[indexVel] = initial_[indexVel];
            result[indexAcc] = 0;
          }
          else if(t <= (t0_[id] + t1_[id])){
            //hppDout(info,"on  1° segment");
            t1 = t - t0_[id];
            result[id] = 0.5*t1*t1*a1_[id] + t1*initial_[indexVel] + initial_[id] + t0_[id]*initial_[indexVel];
            result[indexVel] = t1*a1_[id] + initial_[indexVel];
            result[indexAcc] = a1_[id];
          }else if (t <= (t0_[id] + t1_[id] + tv_[id]) ){
            //hppDout(info,"on  constant velocity segment");
            tv = t - t0_[id] - t1_[id];
            result[id] = 0.5*t1_[id]*t1_[id]*a1_[id] + t1_[id]*initial_[indexVel] + initial_[id] + t0_[id]*initial_[indexVel] + (tv)*vLim_[id];
            result[indexVel] = vLim_[id];
            result[indexAcc] = 0.;

          }else if (t <= (t0_[id] + t1_[id] + tv_[id] +t2_[id]) ){
            //hppDout(info,"on  3° segment");
            t2 = t - tv_[id] - t1_[id] - t0_[id];
            if(tv_[id] > 0 )
              v2 = vLim_[id];
            else
              v2 = t1_[id]*a1_[id] + initial_[indexVel];
            result[id] = 0.5*t1_[id]*t1_[id]*a1_[id] + t1_[id]*initial_[indexVel] + initial_[id] + t0_[id]*initial_[indexVel] + tv_[id]*vLim_[id] - 0.5*t2*t2*a1_[id] + t2*v2;
            result[indexVel] = v2 - t2 * a1_[id];
            result[indexAcc] = -a1_[id];

          }else{ // after last segment : constant velocity (null ?)
            if(end_[indexVel] != 0 && (t - (t0_[id] + t1_[id] + tv_[id] +t2_[id])) > 0.00015 ){ // epsilon from rbprm-steering-kinodynamic
              hppDout(notice,"Should not happen : You should not have a constant velocity segment at the end if the velocity is not null");
              hppDout(notice,"index Joint = "<<id);
              hppDout(notice,"t = "<<t<<"  ;  length = "<<t0_[id] + t1_[id] + tv_[id] +t2_[id]);
            }
            result[id] = end_[id];
            result[indexVel] = end_[indexVel];
            result[indexAcc] = 0;
          }
        }// if not quaternion joint

     // }// if joint config size > 1
      
     }// for all joints
      
      return true;
    }
    
    PathPtr_t KinodynamicPath::extract (const interval_t& subInterval) const
    throw (projection_error)
    {
      assert(subInterval.first >=0 && subInterval.second >=0 && "Negative time values in extract path");
      assert(subInterval.first >=timeRange_.first && subInterval.second <= timeRange_.second && "Given interval not inside path interval");
      // Length is assumed to be proportional to interval range
      if(subInterval.first == timeRange().first && subInterval.second == timeRange().second){
        hppDout(notice,"Call extract with same interval");
        return weak_.lock();
      }
      value_type l = fabs (subInterval.second - subInterval.first);
      if(l<=0){
        hppDout(notice,"Call Extract with a length null");
        return PathPtr_t();
      }

      hppDout(notice,"%% EXTRACT PATH : path interval : "<<timeRange_.first<<" ; "<<timeRange_.second);
      hppDout(notice,"%% EXTRACT PATH : sub  interval : "<<subInterval.first<<" ; "<<subInterval.second);

      bool success;
      Configuration_t q1 ((*this) (subInterval.first, success));
      if (!success) throw projection_error
          ("Failed to apply constraints in KinodynamicPath::extract");
      Configuration_t q2 ((*this) (subInterval.second, success));
      if (!success) throw projection_error
          ("Failed to apply constraints in KinodynamicPath::extract");

      // set acceleration to 0 for initial and end config :
      size_type configSize = device()->configSize() - device()->extraConfigSpace().dimension ();
      q1[configSize+3] = 0.0;
      q1[configSize+4] = 0.0;
      q1[configSize+5] = 0.0;
      q2[configSize+3] = 0.0;
      q2[configSize+4] = 0.0;
      q2[configSize+5] = 0.0;
      hppDout(info,"from : ");
      hppDout(info,"q1 = "<<pinocchio::displayConfig(initial_));
      hppDout(info,"q2 = "<<pinocchio::displayConfig(end_));
      hppDout(info,"to : ");
      hppDout(info,"q1 = "<<pinocchio::displayConfig(q1));
      hppDout(info,"q2 = "<<pinocchio::displayConfig(q2));
      if (!success) throw projection_error
          ("Failed to apply constraints in KinodynamicPath::extract");

      if(subInterval.first > subInterval.second){  // reversed path
        hppDout(notice,"%% REVERSE PATH, not implemented yet !");
        std::cout<<"ERROR, you shouldn't call reverse() on a kinodynamic path"<<std::endl;
        return PathPtr_t();
      }
      double ti,tf,oldT0,oldT2,oldT1,oldTv;
      hppDout(notice,"%% subinterval PATH");
      // new timebounds
      Configuration_t t0(t0_);
      Configuration_t t1(t1_);
      Configuration_t t2(t2_);
      Configuration_t tv(tv_);
      Configuration_t a1(a1_);




      for(int i = 0 ; i < a1_.size() ; ++i){ // adjust times bounds
        ti = subInterval.first - timeRange().first;
        tf = timeRange().second - timeRange().second;
        t0[i] = t0_[i] - ti;
        if(t0[i] <= 0){
          t0[i] = 0;
          ti = ti - t0_[i];
          t1[i] = t1_[i] - ti;
          if(t1[i] <= 0){
            t1[i] = 0;
            ti = ti - t1_[i];
            tv[i] = tv_[i] - ti;
            if(tv[i] <= 0 ){
              tv[i] = 0;
              ti = ti - tv_[i];
              t2[i] = t2_[i] - ti;
              if(t2[i] < 0){
                t2[i] = 0;
                hppDout(notice,"Should not happen !");
              }
            }
          }
        }
        oldT0 = t0[i];
        oldT1 = t1[i];
        oldT2 = t2[i];
        oldTv = tv[i];
        t2[i] = oldT2 - tf;
        if(t2[i] <= 0 ){
          t2[i] = 0 ;
          tf = tf - oldT2;
          tv[i] = oldTv - tf;
          if(tv[i] <= 0){
            tv[i] = 0;
            tf = tf - oldTv;
            t1[i] = oldT1 - tf;
            if(t1[i] <= 0 ){
              t1[i] = 0;
              tf = tf - oldT1;
              t0[i] = oldT0 - tf;
              if(t0[i] < 0){
                t0[i] = 0;
                hppDout(notice,"Should not happen !");
              }
            }
          }
        }


      } // for all joints
      PathPtr_t result = KinodynamicPath::create (device_, q1, q2, l,a1,t0,t1,tv,t2,vLim_,
                                                  constraints ());
      return result;
    }
    
  } //   namespace core
} // namespace hpp

