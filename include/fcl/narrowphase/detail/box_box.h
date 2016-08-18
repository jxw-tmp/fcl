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

#ifndef FCL_NARROWPHASE_DETAIL_BOXBOX_H
#define FCL_NARROWPHASE_DETAIL_BOXBOX_H

#include <algorithm>
#include "fcl/collision_data.h"

namespace fcl
{

namespace details
{

template <typename S>
void lineClosestApproach(const Vector3<S>& pa, const Vector3<S>& ua,
                         const Vector3<S>& pb, const Vector3<S>& ub,
                         S* alpha, S* beta);

// find all the intersection points between the 2D rectangle with vertices
// at (+/-h[0],+/-h[1]) and the 2D quadrilateral with vertices (p[0],p[1]),
// (p[2],p[3]),(p[4],p[5]),(p[6],p[7]).
//
// the intersection points are returned as x,y pairs in the 'ret' array.
// the number of intersection points is returned by the function (this will
// be in the range 0 to 8).
template <typename S>
int intersectRectQuad2(S h[2], S p[8], S ret[16]);

// given n points in the plane (array p, of size 2*n), generate m points that
// best represent the whole set. the definition of 'best' here is not
// predetermined - the idea is to select points that give good box-box
// collision detection behavior. the chosen point indexes are returned in the
// array iret (of size m). 'i0' is always the first entry in the array.
// n must be in the range [1..8]. m must be in the range [1..n]. i0 must be
// in the range [0..n-1].
template <typename S>
void cullPoints2(int n, S p[], int m, int i0, int iret[]);

template <typename S, typename DerivedA, typename DerivedB>
int boxBox2(
    const Vector3<S>& side1,
    const Eigen::MatrixBase<DerivedA>& R1,
    const Eigen::MatrixBase<DerivedB>& T1,
    const Vector3<S>& side2,
    const Eigen::MatrixBase<DerivedA>& R2,
    const Eigen::MatrixBase<DerivedB>& T2,
    Vector3<S>& normal,
    S* depth,
    int* return_code,
    int maxc,
    std::vector<ContactPoint<S>>& contacts);

template <typename S>
int boxBox2(
    const Vector3<S>& side1,
    const Transform3<S>& tf1,
    const Vector3<S>& side2,
    const Transform3<S>& tf2,
    Vector3<S>& normal,
    S* depth,
    int* return_code,
    int maxc,
    std::vector<ContactPoint<S>>& contacts);

template <typename S>
bool boxBoxIntersect(const Box<S>& s1, const Transform3<S>& tf1,
                     const Box<S>& s2, const Transform3<S>& tf2,
                     std::vector<ContactPoint<S>>* contacts_);

//============================================================================//
//                                                                            //
//                              Implementations                               //
//                                                                            //
//============================================================================//

//==============================================================================
template <typename S>
void lineClosestApproach(const Vector3<S>& pa, const Vector3<S>& ua,
                         const Vector3<S>& pb, const Vector3<S>& ub,
                         S* alpha, S* beta)
{
  Vector3<S> p = pb - pa;
  S uaub = ua.dot(ub);
  S q1 = ua.dot(p);
  S q2 = -ub.dot(p);
  S d = 1 - uaub * uaub;
  if(d <= (S)(0.0001f))
  {
    *alpha = 0;
    *beta = 0;
  }
  else
  {
    d = 1 / d;
    *alpha = (q1 + uaub * q2) * d;
    *beta = (uaub * q1 + q2) * d;
  }
}

//==============================================================================
template <typename S>
int intersectRectQuad2(S h[2], S p[8], S ret[16])
{
  // q (and r) contain nq (and nr) coordinate points for the current (and
  // chopped) polygons
  int nq = 4, nr = 0;
  S buffer[16];
  S* q = p;
  S* r = ret;
  for(int dir = 0; dir <= 1; ++dir)
  {
    // direction notation: xy[0] = x axis, xy[1] = y axis
    for(int sign = -1; sign <= 1; sign += 2)
    {
      // chop q along the line xy[dir] = sign*h[dir]
      S* pq = q;
      S* pr = r;
      nr = 0;
      for(int i = nq; i > 0; --i)
      {
        // go through all points in q and all lines between adjacent points
        if(sign * pq[dir] < h[dir])
        {
          // this point is inside the chopping line
          pr[0] = pq[0];
          pr[1] = pq[1];
          pr += 2;
          nr++;
          if(nr & 8)
          {
            q = r;
            goto done;
          }
        }
        S* nextq = (i > 1) ? pq+2 : q;
        if((sign*pq[dir] < h[dir]) ^ (sign*nextq[dir] < h[dir]))
        {
          // this line crosses the chopping line
          pr[1-dir] = pq[1-dir] + (nextq[1-dir]-pq[1-dir]) /
            (nextq[dir]-pq[dir]) * (sign*h[dir]-pq[dir]);
          pr[dir] = sign*h[dir];
          pr += 2;
          nr++;
          if(nr & 8)
          {
            q = r;
            goto done;
          }
        }
        pq += 2;
      }
      q = r;
      r = (q == ret) ? buffer : ret;
      nq = nr;
    }
  }

 done:
  if(q != ret) memcpy(ret, q, nr*2*sizeof(S));
  return nr;
}

//==============================================================================
template <typename S>
void cullPoints2(int n, S p[], int m, int i0, int iret[])
{
  // compute the centroid of the polygon in cx,cy
  S a, cx, cy, q;
  switch(n)
  {
  case 1:
    cx = p[0];
    cy = p[1];
    break;
  case 2:
    cx = 0.5 * (p[0] + p[2]);
    cy = 0.5 * (p[1] + p[3]);
    break;
  default:
    a = 0;
    cx = 0;
    cy = 0;
    for(int i = 0; i < n-1; ++i)
    {
      q = p[i*2]*p[i*2+3] - p[i*2+2]*p[i*2+1];
      a += q;
      cx += q*(p[i*2]+p[i*2+2]);
      cy += q*(p[i*2+1]+p[i*2+3]);
    }
    q = p[n*2-2]*p[1] - p[0]*p[n*2-1];
    if(std::abs(a+q) > std::numeric_limits<S>::epsilon())
      a = 1/(3*(a+q));
    else
      a= 1e18f;

    cx = a*(cx + q*(p[n*2-2]+p[0]));
    cy = a*(cy + q*(p[n*2-1]+p[1]));
  }


  // compute the angle of each point w.r.t. the centroid
  S A[8];
  for(int i = 0; i < n; ++i)
    A[i] = atan2(p[i*2+1]-cy,p[i*2]-cx);

  // search for points that have angles closest to A[i0] + i*(2*pi/m).
  int avail[8];
  for(int i = 0; i < n; ++i) avail[i] = 1;
  avail[i0] = 0;
  iret[0] = i0;
  iret++;
  const S pi = constants<S>::pi();
  for(int j = 1; j < m; ++j)
  {
    a = j*(2*pi/m) + A[i0];
    if (a > pi) a -= 2*pi;
    S maxdiff= 1e9, diff;

    *iret = i0;	// iret is not allowed to keep this value, but it sometimes does, when diff=#QNAN0
    for(int i = 0; i < n; ++i)
    {
      if(avail[i])
      {
        diff = std::abs(A[i]-a);
        if(diff > pi) diff = 2*pi - diff;
        if(diff < maxdiff)
        {
          maxdiff = diff;
          *iret = i;
        }
      }
    }
    avail[*iret] = 0;
    iret++;
  }
}

//==============================================================================
template <typename S, typename DerivedA, typename DerivedB>
int boxBox2(
    const Vector3<S>& side1,
    const Eigen::MatrixBase<DerivedA>& R1,
    const Eigen::MatrixBase<DerivedB>& T1,
    const Vector3<S>& side2,
    const Eigen::MatrixBase<DerivedA>& R2,
    const Eigen::MatrixBase<DerivedB>& T2,
    Vector3<S>& normal,
    S* depth,
    int* return_code,
    int maxc,
    std::vector<ContactPoint<S>>& contacts)
{
  const S fudge_factor = S(1.05);
  Vector3<S> normalC;
  S s, s2, l;
  int invert_normal, code;

  Vector3<S> p = T2 - T1; // get vector from centers of box 1 to box 2, relative to box 1
  Vector3<S> pp = R1.transpose() * p; // get pp = p relative to body 1

  // get side lengths / 2
  Vector3<S> A = side1 * 0.5;
  Vector3<S> B = side2 * 0.5;

  // Rij is R1'*R2, i.e. the relative rotation between R1 and R2
  Matrix3<S> R = R1.transpose() * R2;
  Matrix3<S> Q = R.cwiseAbs();


  // for all 15 possible separating axes:
  //   * see if the axis separates the boxes. if so, return 0.
  //   * find the depth of the penetration along the separating axis (s2)
  //   * if this is the largest depth so far, record it.
  // the normal vector will be set to the separating axis with the smallest
  // depth. note: normalR is set to point to a column of R1 or R2 if that is
  // the smallest depth normal so far. otherwise normalR is 0 and normalC is
  // set to a vector relative to body 1. invert_normal is 1 if the sign of
  // the normal should be flipped.

  int best_col_id = -1;
  const Eigen::MatrixBase<DerivedA>* normalR = 0;
  S tmp = 0;

  s = - std::numeric_limits<S>::max();
  invert_normal = 0;
  code = 0;

  // separating axis = u1, u2, u3
  tmp = pp[0];
  s2 = std::abs(tmp) - (Q.row(0).dot(B) + A[0]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 0;
    normalR = &R1;
    invert_normal = (tmp < 0);
    code = 1;
  }

  tmp = pp[1];
  s2 = std::abs(tmp) - (Q.row(1).dot(B) + A[1]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 1;
    normalR = &R1;
    invert_normal = (tmp < 0);
    code = 2;
  }

  tmp = pp[2];
  s2 = std::abs(tmp) - (Q.row(2).dot(B) + A[2]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 2;
    normalR = &R1;
    invert_normal = (tmp < 0);
    code = 3;
  }

  // separating axis = v1, v2, v3
  tmp = R2.col(0).dot(p);
  s2 = std::abs(tmp) - (Q.col(0).dot(A) + B[0]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 0;
    normalR = &R2;
    invert_normal = (tmp < 0);
    code = 4;
  }

  tmp = R2.col(1).dot(p);
  s2 = std::abs(tmp) - (Q.col(1).dot(A) + B[1]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 1;
    normalR = &R2;
    invert_normal = (tmp < 0);
    code = 5;
  }

  tmp = R2.col(2).dot(p);
  s2 =  std::abs(tmp) - (Q.col(2).dot(A) + B[2]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 2;
    normalR = &R2;
    invert_normal = (tmp < 0);
    code = 6;
  }


  S fudge2(1.0e-6);
  Q.array() += fudge2;

  Vector3<S> n;
  S eps = std::numeric_limits<S>::epsilon();

  // separating axis = u1 x (v1,v2,v3)
  tmp = pp[2] * R(1, 0) - pp[1] * R(2, 0);
  s2 = std::abs(tmp) - (A[1] * Q(2, 0) + A[2] * Q(1, 0) + B[1] * Q(0, 2) + B[2] * Q(0, 1));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(0, -R(2, 0), R(1, 0));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 7;
    }
  }

  tmp = pp[2] * R(1, 1) - pp[1] * R(2, 1);
  s2 = std::abs(tmp) - (A[1] * Q(2, 1) + A[2] * Q(1, 1) + B[0] * Q(0, 2) + B[2] * Q(0, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(0, -R(2, 1), R(1, 1));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 8;
    }
  }

  tmp = pp[2] * R(1, 2) - pp[1] * R(2, 2);
  s2 = std::abs(tmp) - (A[1] * Q(2, 2) + A[2] * Q(1, 2) + B[0] * Q(0, 1) + B[1] * Q(0, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(0, -R(2, 2), R(1, 2));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 9;
    }
  }

  // separating axis = u2 x (v1,v2,v3)
  tmp = pp[0] * R(2, 0) - pp[2] * R(0, 0);
  s2 = std::abs(tmp) - (A[0] * Q(2, 0) + A[2] * Q(0, 0) + B[1] * Q(1, 2) + B[2] * Q(1, 1));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(R(2, 0), 0, -R(0, 0));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 10;
    }
  }

  tmp = pp[0] * R(2, 1) - pp[2] * R(0, 1);
  s2 = std::abs(tmp) - (A[0] * Q(2, 1) + A[2] * Q(0, 1) + B[0] * Q(1, 2) + B[2] * Q(1, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(R(2, 1), 0, -R(0, 1));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 11;
    }
  }

  tmp = pp[0] * R(2, 2) - pp[2] * R(0, 2);
  s2 = std::abs(tmp) - (A[0] * Q(2, 2) + A[2] * Q(0, 2) + B[0] * Q(1, 1) + B[1] * Q(1, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(R(2, 2), 0, -R(0, 2));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 12;
    }
  }

  // separating axis = u3 x (v1,v2,v3)
  tmp = pp[1] * R(0, 0) - pp[0] * R(1, 0);
  s2 = std::abs(tmp) - (A[0] * Q(1, 0) + A[1] * Q(0, 0) + B[1] * Q(2, 2) + B[2] * Q(2, 1));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(-R(1, 0), R(0, 0), 0);
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 13;
    }
  }

  tmp = pp[1] * R(0, 1) - pp[0] * R(1, 1);
  s2 = std::abs(tmp) - (A[0] * Q(1, 1) + A[1] * Q(0, 1) + B[0] * Q(2, 2) + B[2] * Q(2, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(-R(1, 1), R(0, 1), 0);
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 14;
    }
  }

  tmp = pp[1] * R(0, 2) - pp[0] * R(1, 2);
  s2 = std::abs(tmp) - (A[0] * Q(1, 2) + A[1] * Q(0, 2) + B[0] * Q(2, 1) + B[1] * Q(2, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(-R(1, 2), R(0, 2), 0);
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 15;
    }
  }



  if (!code) { *return_code = code; return 0; }

  // if we get to this point, the boxes interpenetrate. compute the normal
  // in global coordinates.
  if(best_col_id != -1)
    normal = normalR->col(best_col_id);
  else
    normal = R1 * normalC;

  if(invert_normal)
    normal = -normal;

  *depth = -s; // s is negative when the boxes are in collision

  // compute contact point(s)

  if(code > 6)
  {
    // an edge from box 1 touches an edge from box 2.
    // find a point pa on the intersecting edge of box 1
    Vector3<S> pa(T1);
    S sign;

    for(int j = 0; j < 3; ++j)
    {
      sign = (R1.col(j).dot(normal) > 0) ? 1 : -1;
      pa += R1.col(j) * (A[j] * sign);
    }

    // find a point pb on the intersecting edge of box 2
    Vector3<S> pb(T2);

    for(int j = 0; j < 3; ++j)
    {
      sign = (R2.col(j).dot(normal) > 0) ? -1 : 1;
      pb += R2.col(j) * (B[j] * sign);
    }

    S alpha, beta;
    Vector3<S> ua(R1.col((code-7)/3));
    Vector3<S> ub(R2.col((code-7)%3));

    lineClosestApproach(pa, ua, pb, ub, &alpha, &beta);
    pa += ua * alpha;
    pb += ub * beta;


    // Vector3<S> pointInWorld((pa + pb) * 0.5);
    // contacts.push_back(ContactPoint<S>(-normal, pointInWorld, -*depth));
    contacts.emplace_back(normal, pb, -*depth);
    *return_code = code;

    return 1;
  }

  // okay, we have a face-something intersection (because the separating
  // axis is perpendicular to a face). define face 'a' to be the reference
  // face (i.e. the normal vector is perpendicular to this) and face 'b' to be
  // the incident face (the closest face of the other box).

  const Eigen::MatrixBase<DerivedA> *Ra, *Rb;
  const Eigen::MatrixBase<DerivedB> *pa, *pb;
  const Vector3<S> *Sa, *Sb;

  if(code <= 3)
  {
    Ra = &R1;
    Rb = &R2;
    pa = &T1;
    pb = &T2;
    Sa = &A;
    Sb = &B;
  }
  else
  {
    Ra = &R2;
    Rb = &R1;
    pa = &T2;
    pb = &T1;
    Sa = &B;
    Sb = &A;
  }

  // nr = normal vector of reference face dotted with axes of incident box.
  // anr = absolute values of nr.
  Vector3<S> normal2, nr, anr;
  if(code <= 3)
    normal2 = normal;
  else
    normal2 = -normal;

  nr = Rb->transpose() * normal2;
  anr = nr.cwiseAbs();

  // find the largest compontent of anr: this corresponds to the normal
  // for the indident face. the other axis numbers of the indicent face
  // are stored in a1,a2.
  int lanr, a1, a2;
  if(anr[1] > anr[0])
  {
    if(anr[1] > anr[2])
    {
      a1 = 0;
      lanr = 1;
      a2 = 2;
    }
    else
    {
      a1 = 0;
      a2 = 1;
      lanr = 2;
    }
  }
  else
  {
    if(anr[0] > anr[2])
    {
      lanr = 0;
      a1 = 1;
      a2 = 2;
    }
    else
    {
      a1 = 0;
      a2 = 1;
      lanr = 2;
    }
  }

  // compute center point of incident face, in reference-face coordinates
  Vector3<S> center;
  if(nr[lanr] < 0)
    center = (*pb) - (*pa) + Rb->col(lanr) * ((*Sb)[lanr]);
  else
    center = (*pb) - (*pa) - Rb->col(lanr) * ((*Sb)[lanr]);

  // find the normal and non-normal axis numbers of the reference box
  int codeN, code1, code2;
  if(code <= 3)
    codeN = code-1;
  else codeN = code-4;

  if(codeN == 0)
  {
    code1 = 1;
    code2 = 2;
  }
  else if(codeN == 1)
  {
    code1 = 0;
    code2 = 2;
  }
  else
  {
    code1 = 0;
    code2 = 1;
  }

  // find the four corners of the incident face, in reference-face coordinates
  S quad[8]; // 2D coordinate of incident face (x,y pairs)
  S c1, c2, m11, m12, m21, m22;
  c1 = Ra->col(code1).dot(center);
  c2 = Ra->col(code2).dot(center);
  // optimize this? - we have already computed this data above, but it is not
  // stored in an easy-to-index format. for now it's quicker just to recompute
  // the four dot products.
  Vector3<S> tempRac = Ra->col(code1);
  m11 = Rb->col(a1).dot(tempRac);
  m12 = Rb->col(a2).dot(tempRac);
  tempRac = Ra->col(code2);
  m21 = Rb->col(a1).dot(tempRac);
  m22 = Rb->col(a2).dot(tempRac);

  S k1 = m11 * (*Sb)[a1];
  S k2 = m21 * (*Sb)[a1];
  S k3 = m12 * (*Sb)[a2];
  S k4 = m22 * (*Sb)[a2];
  quad[0] = c1 - k1 - k3;
  quad[1] = c2 - k2 - k4;
  quad[2] = c1 - k1 + k3;
  quad[3] = c2 - k2 + k4;
  quad[4] = c1 + k1 + k3;
  quad[5] = c2 + k2 + k4;
  quad[6] = c1 + k1 - k3;
  quad[7] = c2 + k2 - k4;

  // find the size of the reference face
  S rect[2];
  rect[0] = (*Sa)[code1];
  rect[1] = (*Sa)[code2];

  // intersect the incident and reference faces
  S ret[16];
  int n_intersect = intersectRectQuad2(rect, quad, ret);
  if(n_intersect < 1) { *return_code = code; return 0; } // this should never happen

  // convert the intersection points into reference-face coordinates,
  // and compute the contact position and depth for each point. only keep
  // those points that have a positive (penetrating) depth. delete points in
  // the 'ret' array as necessary so that 'point' and 'ret' correspond.
  Vector3<S> points[8]; // penetrating contact points
  S dep[8]; // depths for those points
  S det1 = 1.f/(m11*m22 - m12*m21);
  m11 *= det1;
  m12 *= det1;
  m21 *= det1;
  m22 *= det1;
  int cnum = 0;	// number of penetrating contact points found
  for(int j = 0; j < n_intersect; ++j)
  {
    S k1 =  m22*(ret[j*2]-c1) - m12*(ret[j*2+1]-c2);
    S k2 = -m21*(ret[j*2]-c1) + m11*(ret[j*2+1]-c2);
    points[cnum] = center + Rb->col(a1) * k1 + Rb->col(a2) * k2;
    dep[cnum] = (*Sa)[codeN] - normal2.dot(points[cnum]);
    if(dep[cnum] >= 0)
    {
      ret[cnum*2] = ret[j*2];
      ret[cnum*2+1] = ret[j*2+1];
      cnum++;
    }
  }
  if(cnum < 1) { *return_code = code; return 0; } // this should never happen

  // we can't generate more contacts than we actually have
  if(maxc > cnum) maxc = cnum;
  if(maxc < 1) maxc = 1;

  if(cnum <= maxc)
  {
    if(code<4)
    {
      // we have less contacts than we need, so we use them all
      for(int j = 0; j < cnum; ++j)
      {
        Vector3<S> pointInWorld = points[j] + (*pa);
        contacts.emplace_back(normal, pointInWorld, -dep[j]);
      }
    }
    else
    {
      // we have less contacts than we need, so we use them all
      for(int j = 0; j < cnum; ++j)
      {
        Vector3<S> pointInWorld = points[j] + (*pa) - normal * dep[j];
        contacts.emplace_back(normal, pointInWorld, -dep[j]);
      }
    }
  }
  else
  {
    // we have more contacts than are wanted, some of them must be culled.
    // find the deepest point, it is always the first contact.
    int i1 = 0;
    S maxdepth = dep[0];
    for(int i = 1; i < cnum; ++i)
    {
      if(dep[i] > maxdepth)
      {
        maxdepth = dep[i];
        i1 = i;
      }
    }

    int iret[8];
    cullPoints2(cnum, ret, maxc, i1, iret);

    for(int j = 0; j < maxc; ++j)
    {
      Vector3<S> posInWorld = points[iret[j]] + (*pa);
      if(code < 4)
        contacts.emplace_back(normal, posInWorld, -dep[iret[j]]);
      else
        contacts.emplace_back(normal, posInWorld - normal * dep[iret[j]], -dep[iret[j]]);
    }
    cnum = maxc;
  }

  *return_code = code;
  return cnum;
}

//==============================================================================
template <typename S>
int boxBox2(
    const Vector3<S>& side1,
    const Transform3<S>& tf1,
    const Vector3<S>& side2,
    const Transform3<S>& tf2,
    Vector3<S>& normal,
    S* depth,
    int* return_code,
    int maxc,
    std::vector<ContactPoint<S>>& contacts)
{
  const S fudge_factor = S(1.05);
  Vector3<S> normalC;

  const Vector3<S> p = tf2.translation() - tf1.translation(); // get vector from centers of box 1 to box 2, relative to box 1
  const Vector3<S> pp = tf1.linear().transpose() * p; // get pp = p relative to body 1

  // get side lengths / 2
  const Vector3<S> A = side1 * 0.5;
  const Vector3<S> B = side2 * 0.5;

  // Rij is R1'*R2, i.e. the relative rotation between R1 and R2
  const Matrix3<S> R = tf1.linear().transpose() * tf2.linear();
  Matrix3<S> Q = R.cwiseAbs();

  // for all 15 possible separating axes:
  //   * see if the axis separates the boxes. if so, return 0.
  //   * find the depth of the penetration along the separating axis (s2)
  //   * if this is the largest depth so far, record it.
  // the normal vector will be set to the separating axis with the smallest
  // depth. note: normalR is set to point to a column of R1 or R2 if that is
  // the smallest depth normal so far. otherwise normalR is 0 and normalC is
  // set to a vector relative to body 1. invert_normal is 1 if the sign of
  // the normal should be flipped.

  int best_col_id = -1;
  const Transform3<S>* normalT = nullptr;

  S s = - std::numeric_limits<S>::max();
  int invert_normal = 0;
  int code = 0;

  // separating axis = u1, u2, u3
  S tmp = pp[0];
  S s2 = std::abs(tmp) - (Q.row(0).dot(B) + A[0]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 0;
    normalT = &tf1;
    invert_normal = (tmp < 0);
    code = 1;
  }

  tmp = pp[1];
  s2 = std::abs(tmp) - (Q.row(1).dot(B) + A[1]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 1;
    normalT = &tf1;
    invert_normal = (tmp < 0);
    code = 2;
  }

  tmp = pp[2];
  s2 = std::abs(tmp) - (Q.row(2).dot(B) + A[2]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 2;
    normalT = &tf1;
    invert_normal = (tmp < 0);
    code = 3;
  }

  // separating axis = v1, v2, v3
  tmp = tf2.linear().col(0).dot(p);
  s2 = std::abs(tmp) - (Q.col(0).dot(A) + B[0]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 0;
    normalT = &tf2;
    invert_normal = (tmp < 0);
    code = 4;
  }

  tmp = tf2.linear().col(1).dot(p);
  s2 = std::abs(tmp) - (Q.col(1).dot(A) + B[1]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 1;
    normalT = &tf2;
    invert_normal = (tmp < 0);
    code = 5;
  }

  tmp = tf2.linear().col(2).dot(p);
  s2 =  std::abs(tmp) - (Q.col(2).dot(A) + B[2]);
  if(s2 > 0) { *return_code = 0; return 0; }
  if(s2 > s)
  {
    s = s2;
    best_col_id = 2;
    normalT = &tf2;
    invert_normal = (tmp < 0);
    code = 6;
  }


  S fudge2(1.0e-6);
  Q.array() += fudge2;

  Vector3<S> n;
  S eps = std::numeric_limits<S>::epsilon();

  // separating axis = u1 x (v1,v2,v3)
  tmp = pp[2] * R(1, 0) - pp[1] * R(2, 0);
  s2 = std::abs(tmp) - (A[1] * Q(2, 0) + A[2] * Q(1, 0) + B[1] * Q(0, 2) + B[2] * Q(0, 1));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(0, -R(2, 0), R(1, 0));
  S l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 7;
    }
  }

  tmp = pp[2] * R(1, 1) - pp[1] * R(2, 1);
  s2 = std::abs(tmp) - (A[1] * Q(2, 1) + A[2] * Q(1, 1) + B[0] * Q(0, 2) + B[2] * Q(0, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(0, -R(2, 1), R(1, 1));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 8;
    }
  }

  tmp = pp[2] * R(1, 2) - pp[1] * R(2, 2);
  s2 = std::abs(tmp) - (A[1] * Q(2, 2) + A[2] * Q(1, 2) + B[0] * Q(0, 1) + B[1] * Q(0, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(0, -R(2, 2), R(1, 2));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 9;
    }
  }

  // separating axis = u2 x (v1,v2,v3)
  tmp = pp[0] * R(2, 0) - pp[2] * R(0, 0);
  s2 = std::abs(tmp) - (A[0] * Q(2, 0) + A[2] * Q(0, 0) + B[1] * Q(1, 2) + B[2] * Q(1, 1));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(R(2, 0), 0, -R(0, 0));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 10;
    }
  }

  tmp = pp[0] * R(2, 1) - pp[2] * R(0, 1);
  s2 = std::abs(tmp) - (A[0] * Q(2, 1) + A[2] * Q(0, 1) + B[0] * Q(1, 2) + B[2] * Q(1, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(R(2, 1), 0, -R(0, 1));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 11;
    }
  }

  tmp = pp[0] * R(2, 2) - pp[2] * R(0, 2);
  s2 = std::abs(tmp) - (A[0] * Q(2, 2) + A[2] * Q(0, 2) + B[0] * Q(1, 1) + B[1] * Q(1, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(R(2, 2), 0, -R(0, 2));
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 12;
    }
  }

  // separating axis = u3 x (v1,v2,v3)
  tmp = pp[1] * R(0, 0) - pp[0] * R(1, 0);
  s2 = std::abs(tmp) - (A[0] * Q(1, 0) + A[1] * Q(0, 0) + B[1] * Q(2, 2) + B[2] * Q(2, 1));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(-R(1, 0), R(0, 0), 0);
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 13;
    }
  }

  tmp = pp[1] * R(0, 1) - pp[0] * R(1, 1);
  s2 = std::abs(tmp) - (A[0] * Q(1, 1) + A[1] * Q(0, 1) + B[0] * Q(2, 2) + B[2] * Q(2, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(-R(1, 1), R(0, 1), 0);
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 14;
    }
  }

  tmp = pp[1] * R(0, 2) - pp[0] * R(1, 2);
  s2 = std::abs(tmp) - (A[0] * Q(1, 2) + A[1] * Q(0, 2) + B[0] * Q(2, 1) + B[1] * Q(2, 0));
  if(s2 > 0) { *return_code = 0; return 0; }
  n = Vector3<S>(-R(1, 2), R(0, 2), 0);
  l = n.norm();
  if(l > eps)
  {
    s2 /= l;
    if(s2 * fudge_factor > s)
    {
      s = s2;
      best_col_id = -1;
      normalC = n / l;
      invert_normal = (tmp < 0);
      code = 15;
    }
  }



  if (!code) { *return_code = code; return 0; }

  // if we get to this point, the boxes interpenetrate. compute the normal
  // in global coordinates.
  if(best_col_id != -1)
    normal = normalT->linear().col(best_col_id);
  else
    normal = tf1.linear() * normalC;

  if(invert_normal)
    normal = -normal;

  *depth = -s; // s is negative when the boxes are in collision

  // compute contact point(s)

  if(code > 6)
  {
    // an edge from box 1 touches an edge from box 2.
    // find a point pa on the intersecting edge of box 1
    Vector3<S> pa(tf1.translation());
    S sign;

    for(int j = 0; j < 3; ++j)
    {
      sign = (tf1.linear().col(j).dot(normal) > 0) ? 1 : -1;
      pa += tf1.linear().col(j) * (A[j] * sign);
    }

    // find a point pb on the intersecting edge of box 2
    Vector3<S> pb(tf2.translation());

    for(int j = 0; j < 3; ++j)
    {
      sign = (tf2.linear().col(j).dot(normal) > 0) ? -1 : 1;
      pb += tf2.linear().col(j) * (B[j] * sign);
    }

    S alpha, beta;
    Vector3<S> ua(tf1.linear().col((code-7)/3));
    Vector3<S> ub(tf2.linear().col((code-7)%3));

    lineClosestApproach(pa, ua, pb, ub, &alpha, &beta);
    pa += ua * alpha;
    pb += ub * beta;


    // Vector3<S> pointInWorld((pa + pb) * 0.5);
    // contacts.push_back(ContactPoint<S>(-normal, pointInWorld, -*depth));
    contacts.emplace_back(normal, pb, -*depth);
    *return_code = code;

    return 1;
  }

  // okay, we have a face-something intersection (because the separating
  // axis is perpendicular to a face). define face 'a' to be the reference
  // face (i.e. the normal vector is perpendicular to this) and face 'b' to be
  // the incident face (the closest face of the other box).

  const Transform3<S> *Ta, *Tb;
  const Vector3<S> *Sa, *Sb;

  if(code <= 3)
  {
    Ta = &tf1;
    Tb = &tf2;
    Sa = &A;
    Sb = &B;
  }
  else
  {
    Ta = &tf2;
    Tb = &tf1;
    Sa = &B;
    Sb = &A;
  }

  // nr = normal vector of reference face dotted with axes of incident box.
  // anr = absolute values of nr.
  Vector3<S> normal2, nr, anr;
  if(code <= 3)
    normal2 = normal;
  else
    normal2 = -normal;

  nr = Tb->linear().transpose() * normal2;
  anr = nr.cwiseAbs();

  // find the largest compontent of anr: this corresponds to the normal
  // for the indident face. the other axis numbers of the indicent face
  // are stored in a1,a2.
  int lanr, a1, a2;
  if(anr[1] > anr[0])
  {
    if(anr[1] > anr[2])
    {
      a1 = 0;
      lanr = 1;
      a2 = 2;
    }
    else
    {
      a1 = 0;
      a2 = 1;
      lanr = 2;
    }
  }
  else
  {
    if(anr[0] > anr[2])
    {
      lanr = 0;
      a1 = 1;
      a2 = 2;
    }
    else
    {
      a1 = 0;
      a2 = 1;
      lanr = 2;
    }
  }

  // compute center point of incident face, in reference-face coordinates
  Vector3<S> center;
  if(nr[lanr] < 0)
    center = Tb->translation() - Ta->translation() + Tb->linear().col(lanr) * ((*Sb)[lanr]);
  else
    center = Tb->translation() - Ta->translation() - Tb->linear().col(lanr) * ((*Sb)[lanr]);

  // find the normal and non-normal axis numbers of the reference box
  int codeN, code1, code2;
  if(code <= 3)
    codeN = code-1;
  else codeN = code-4;

  if(codeN == 0)
  {
    code1 = 1;
    code2 = 2;
  }
  else if(codeN == 1)
  {
    code1 = 0;
    code2 = 2;
  }
  else
  {
    code1 = 0;
    code2 = 1;
  }

  // find the four corners of the incident face, in reference-face coordinates
  S quad[8]; // 2D coordinate of incident face (x,y pairs)
  S c1, c2, m11, m12, m21, m22;
  c1 = Ta->linear().col(code1).dot(center);
  c2 = Ta->linear().col(code2).dot(center);
  // optimize this? - we have already computed this data above, but it is not
  // stored in an easy-to-index format. for now it's quicker just to recompute
  // the four dot products.
  Vector3<S> tempRac = Ta->linear().col(code1);
  m11 = Tb->linear().col(a1).dot(tempRac);
  m12 = Tb->linear().col(a2).dot(tempRac);
  tempRac = Ta->linear().col(code2);
  m21 = Tb->linear().col(a1).dot(tempRac);
  m22 = Tb->linear().col(a2).dot(tempRac);

  S k1 = m11 * (*Sb)[a1];
  S k2 = m21 * (*Sb)[a1];
  S k3 = m12 * (*Sb)[a2];
  S k4 = m22 * (*Sb)[a2];
  quad[0] = c1 - k1 - k3;
  quad[1] = c2 - k2 - k4;
  quad[2] = c1 - k1 + k3;
  quad[3] = c2 - k2 + k4;
  quad[4] = c1 + k1 + k3;
  quad[5] = c2 + k2 + k4;
  quad[6] = c1 + k1 - k3;
  quad[7] = c2 + k2 - k4;

  // find the size of the reference face
  S rect[2];
  rect[0] = (*Sa)[code1];
  rect[1] = (*Sa)[code2];

  // intersect the incident and reference faces
  S ret[16];
  int n_intersect = intersectRectQuad2(rect, quad, ret);
  if(n_intersect < 1) { *return_code = code; return 0; } // this should never happen

  // convert the intersection points into reference-face coordinates,
  // and compute the contact position and depth for each point. only keep
  // those points that have a positive (penetrating) depth. delete points in
  // the 'ret' array as necessary so that 'point' and 'ret' correspond.
  Vector3<S> points[8]; // penetrating contact points
  S dep[8]; // depths for those points
  S det1 = 1.f/(m11*m22 - m12*m21);
  m11 *= det1;
  m12 *= det1;
  m21 *= det1;
  m22 *= det1;
  int cnum = 0;	// number of penetrating contact points found
  for(int j = 0; j < n_intersect; ++j)
  {
    S k1 =  m22*(ret[j*2]-c1) - m12*(ret[j*2+1]-c2);
    S k2 = -m21*(ret[j*2]-c1) + m11*(ret[j*2+1]-c2);
    points[cnum] = center + Tb->linear().col(a1) * k1 + Tb->linear().col(a2) * k2;
    dep[cnum] = (*Sa)[codeN] - normal2.dot(points[cnum]);
    if(dep[cnum] >= 0)
    {
      ret[cnum*2] = ret[j*2];
      ret[cnum*2+1] = ret[j*2+1];
      cnum++;
    }
  }
  if(cnum < 1) { *return_code = code; return 0; } // this should never happen

  // we can't generate more contacts than we actually have
  if(maxc > cnum) maxc = cnum;
  if(maxc < 1) maxc = 1;

  if(cnum <= maxc)
  {
    if(code<4)
    {
      // we have less contacts than we need, so we use them all
      for(int j = 0; j < cnum; ++j)
      {
        Vector3<S> pointInWorld = points[j] + Ta->translation();
        contacts.emplace_back(normal, pointInWorld, -dep[j]);
      }
    }
    else
    {
      // we have less contacts than we need, so we use them all
      for(int j = 0; j < cnum; ++j)
      {
        Vector3<S> pointInWorld = points[j] + Ta->translation() - normal * dep[j];
        contacts.emplace_back(normal, pointInWorld, -dep[j]);
      }
    }
  }
  else
  {
    // we have more contacts than are wanted, some of them must be culled.
    // find the deepest point, it is always the first contact.
    int i1 = 0;
    S maxdepth = dep[0];
    for(int i = 1; i < cnum; ++i)
    {
      if(dep[i] > maxdepth)
      {
        maxdepth = dep[i];
        i1 = i;
      }
    }

    int iret[8];
    cullPoints2(cnum, ret, maxc, i1, iret);

    for(int j = 0; j < maxc; ++j)
    {
      Vector3<S> posInWorld = points[iret[j]] + Ta->translation();
      if(code < 4)
        contacts.emplace_back(normal, posInWorld, -dep[iret[j]]);
      else
        contacts.emplace_back(normal, posInWorld - normal * dep[iret[j]], -dep[iret[j]]);
    }
    cnum = maxc;
  }

  *return_code = code;
  return cnum;
}

//==============================================================================
template <typename S>
bool boxBoxIntersect(const Box<S>& s1, const Transform3<S>& tf1,
                     const Box<S>& s2, const Transform3<S>& tf2,
                     std::vector<ContactPoint<S>>* contacts_)
{
  std::vector<ContactPoint<S>> contacts;
  int return_code;
  Vector3<S> normal;
  S depth;
  /* int cnum = */ boxBox2(s1.side, tf1,
                           s2.side, tf2,
                           normal, &depth, &return_code,
                           4, contacts);

  if(contacts_)
    *contacts_ = contacts;

  return return_code != 0;
}

} // namespace details

} // namespace fcl

#endif
