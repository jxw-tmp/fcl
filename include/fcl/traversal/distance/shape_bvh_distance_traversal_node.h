/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011-2014, Willow Garage, Inc.
 *  Copyright (c) 2014-2016, Open Source Robotics Foundation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan */

#ifndef FCL_TRAVERSAL_SHAPEBVHDISTANCETRAVERSALNODE_H
#define FCL_TRAVERSAL_SHAPEBVHDISTANCETRAVERSALNODE_H

#include "fcl/traversal/traversal_node_base.h"
#include "fcl/traversal/distance/distance_traversal_node_base.h"
#include "fcl/BVH/BVH_model.h"

namespace fcl
{

/// @brief Traversal node for distance computation between shape and BVH
template<typename Shape, typename BV>
class ShapeBVHDistanceTraversalNode
    : public DistanceTraversalNodeBase<typename BV::S>
{
public:

  using S = typename BV::S;

  ShapeBVHDistanceTraversalNode();

  /// @brief Whether the BV node in the second BVH tree is leaf
  bool isSecondNodeLeaf(int b) const;

  /// @brief Obtain the left child of BV node in the second BVH
  int getSecondLeftChild(int b) const;

  /// @brief Obtain the right child of BV node in the second BVH
  int getSecondRightChild(int b) const;

  /// @brief BV culling test in one BVTT node
  S BVTesting(int b1, int b2) const;

  const Shape* model1;
  const BVHModel<BV>* model2;
  BV model1_bv;
  
  mutable int num_bv_tests;
  mutable int num_leaf_tests;
  mutable S query_time_seconds;
};

//============================================================================//
//                                                                            //
//                              Implementations                               //
//                                                                            //
//============================================================================//

//==============================================================================
template<typename Shape, typename BV>
ShapeBVHDistanceTraversalNode<Shape, BV>::ShapeBVHDistanceTraversalNode()
  : DistanceTraversalNodeBase<typename BV::S>()
{
  model1 = nullptr;
  model2 = nullptr;

  num_bv_tests = 0;
  num_leaf_tests = 0;
  query_time_seconds = 0.0;
}

//==============================================================================
template<typename Shape, typename BV>
bool ShapeBVHDistanceTraversalNode<Shape, BV>::isSecondNodeLeaf(int b) const
{
  return model2->getBV(b).isLeaf();
}

//==============================================================================
template<typename Shape, typename BV>
int ShapeBVHDistanceTraversalNode<Shape, BV>::getSecondLeftChild(int b) const
{
  return model2->getBV(b).leftChild();
}

//==============================================================================
template<typename Shape, typename BV>
int ShapeBVHDistanceTraversalNode<Shape, BV>::getSecondRightChild(int b) const
{
  return model2->getBV(b).rightChild();
}

//==============================================================================
template<typename Shape, typename BV>
typename BV::S
ShapeBVHDistanceTraversalNode<Shape, BV>::BVTesting(int b1, int b2) const
{
  return model1_bv.distance(model2->getBV(b2).bv);
}

} // namespace fcl

#endif
