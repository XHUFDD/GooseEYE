/* =================================================================================================

(c - GPLv3) T.W.J. de Geus (Tom) | tom@geus.me | www.geus.me | github.com/tdegeus/GooseEYE

================================================================================================= */

#ifndef GOOSEEYE_ENSEMBLE_W2c_HPP
#define GOOSEEYE_ENSEMBLE_W2c_HPP

// =================================================================================================

#include "GooseEYE.h"

// =================================================================================================

namespace GooseEYE {

// =================================================================================================
// weighted 2-point correlation collapsed to cluster centres -- "master"
// =================================================================================================

void Ensemble::W2c(ArrI clus, ArrI cntr, ArrD f, ArrI fmask, std::string mode)
{
  // lock measure
  if ( mStat == Stat::Unset) mStat = Stat::W2c;

  // checks
  std::string name = "GooseEYE::Ensemble::W2c - ";
  if ( f.rank()  != mData.rank()  ) throw std::runtime_error(name+"rank inconsistent");
  if ( f.shape() != fmask.shape() ) throw std::runtime_error(name+"shape inconsistent");
  if ( f.shape() != clus .shape() ) throw std::runtime_error(name+"shape inconsistent");
  if ( f.shape() != cntr .shape() ) throw std::runtime_error(name+"shape inconsistent");

  // switch off bound-checks based on periodicity settings
  clus .setPeriodic(mPeriodic);
  cntr .setPeriodic(mPeriodic);
  f    .setPeriodic(mPeriodic);
  fmask.setPeriodic(mPeriodic);

  // change rank (to avoid failing assertions)
  f.chrank(3);

  // list of end-points of ROI-stamp used in path-based correlations (make 3-d to simply below)
  MatI stamp = stampPoints(3);

  // correlation
  for ( size_t ipnt = 0 ; ipnt < stamp.shape(0) ; ++ipnt )
  {
    // - voxel-path
    MatI pix = path({0,0,0}, {stamp(ipnt,0), stamp(ipnt,1), stamp(ipnt,2)}, mode);
    // - compute correlation
    for ( int h = mSkip[0] ; h < f.shape<int>(0)-mSkip[0] ; ++h ) {
      for ( int i = mSkip[1] ; i < f.shape<int>(1)-mSkip[1] ; ++i ) {
        for ( int j = mSkip[2] ; j < f.shape<int>(2)-mSkip[2] ; ++j ) {
          // -- use clusters centres as binary weight (skip zero weight)
          if ( cntr(h,i,j) ) {
            // -- store label
            int label = cntr(h,i,j);
            // -- proceed only when the centre is inside the cluster
            if ( clus(h,i,j) == label ) {
              // -- initialize counter
              int jpix = -1;
              // -- loop through the voxel-path
              for ( size_t ipix = 0 ; ipix < pix.shape(0) ; ++ipix ) {
                // -- get current voxel
                int dh = pix(ipix,0);
                int di = pix(ipix,1);
                int dj = pix(ipix,2);
                // -- loop through the voxel-path until the end of a cluster
                if ( clus(h+dh,i+di,j+dj) != label and jpix < 0 ) jpix = 0;
                // -- store: loop from the beginning of the path and store there
                if ( jpix >= 0 ) {
                  if ( ! fmask(h+dh,i+di,j+dj) ) {
                    mNorm(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += 1.;
                    mData(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += f(h+dh,i+di,j+dj);
                  }
                }
                // -- update counter
                jpix++;
              }
            }
          }
        }
      }
    }
  }
}

// =================================================================================================
// weighted 2-point correlation collapsed to cluster centres -- "slave": compare to "master"
// =================================================================================================

void Ensemble::W2c(ArrI clus, ArrI cntr, ArrI f, ArrI fmask, std::string mode)
{
  // lock measure
  if ( mStat == Stat::Unset) mStat = Stat::W2c;

  // checks
  std::string name = "GooseEYE::Ensemble::W2c - ";
  if ( f.rank()  != mData.rank()  ) throw std::runtime_error(name+"rank inconsistent");
  if ( f.shape() != fmask.shape() ) throw std::runtime_error(name+"shape inconsistent");
  if ( f.shape() != clus .shape() ) throw std::runtime_error(name+"shape inconsistent");
  if ( f.shape() != cntr .shape() ) throw std::runtime_error(name+"shape inconsistent");

  // switch off bound-checks based on periodicity settings
  clus .setPeriodic(mPeriodic);
  cntr .setPeriodic(mPeriodic);
  f    .setPeriodic(mPeriodic);
  fmask.setPeriodic(mPeriodic);

  // change rank (to avoid failing assertions)
  f.chrank(3);

  // list of end-points of ROI-stamp used in path-based correlations (make 3-d to simply below)
  MatI stamp = stampPoints(3);

  // correlation
  for ( size_t ipnt = 0 ; ipnt < stamp.shape(0) ; ++ipnt )
  {
    // - voxel-path
    MatI pix = path({0,0,0}, {stamp(ipnt,0), stamp(ipnt,1), stamp(ipnt,2)}, mode);
    // - compute correlation
    for ( int h = mSkip[0] ; h < f.shape<int>(0)-mSkip[0] ; ++h ) {
      for ( int i = mSkip[1] ; i < f.shape<int>(1)-mSkip[1] ; ++i ) {
        for ( int j = mSkip[2] ; j < f.shape<int>(2)-mSkip[2] ; ++j ) {
          // -- use clusters centres as binary weight (skip zero weight)
          if ( cntr(h,i,j) ) {
            // -- store label
            int label = cntr(h,i,j);
            // -- proceed only when the centre is inside the cluster
            if ( clus(h,i,j) == label ) {
              // -- initialize counter
              int jpix = -1;
              // -- loop through the voxel-path
              for ( size_t ipix = 0 ; ipix < pix.shape(0) ; ++ipix ) {
                // -- get current voxel
                int dh = pix(ipix,0);
                int di = pix(ipix,1);
                int dj = pix(ipix,2);
                // -- loop through the voxel-path until the end of a cluster
                if ( clus(h+dh,i+di,j+dj) != label and jpix < 0 ) jpix = 0;
                // -- store: loop from the beginning of the path and store there
                if ( jpix >= 0 ) {
                  if ( ! fmask(h+dh,i+di,j+dj) ) {
                    mNorm(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += 1.;
                    if ( f(h+dh,i+di,j+dj) )
                      mData(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += 1.;
                  }
                }
                // -- update counter
                jpix++;
              }
            }
          }
        }
      }
    }
  }
}

// =================================================================================================
// weighted 2-point correlation collapsed to cluster centres -- "slave": compare to "master"
// =================================================================================================

void Ensemble::W2c(ArrI clus, ArrI cntr, ArrD f, std::string mode)
{
  // lock measure
  if ( mStat == Stat::Unset) mStat = Stat::W2c;

  // checks
  std::string name = "GooseEYE::Ensemble::W2c - ";
  if ( f.rank()  != mData.rank()  ) throw std::runtime_error(name+"rank inconsistent");
  if ( f.shape() != clus .shape() ) throw std::runtime_error(name+"shape inconsistent");
  if ( f.shape() != cntr .shape() ) throw std::runtime_error(name+"shape inconsistent");

  // switch off bound-checks based on periodicity settings
  clus .setPeriodic(mPeriodic);
  cntr .setPeriodic(mPeriodic);
  f    .setPeriodic(mPeriodic);

  // change rank (to avoid failing assertions)
  f.chrank(3);

  // list of end-points of ROI-stamp used in path-based correlations (make 3-d to simply below)
  MatI stamp = stampPoints(3);

  // correlation
  for ( size_t ipnt = 0 ; ipnt < stamp.shape(0) ; ++ipnt )
  {
    // - voxel-path
    MatI pix = path({0,0,0}, {stamp(ipnt,0), stamp(ipnt,1), stamp(ipnt,2)}, mode);
    // - compute correlation
    for ( int h = mSkip[0] ; h < f.shape<int>(0)-mSkip[0] ; ++h ) {
      for ( int i = mSkip[1] ; i < f.shape<int>(1)-mSkip[1] ; ++i ) {
        for ( int j = mSkip[2] ; j < f.shape<int>(2)-mSkip[2] ; ++j ) {
          // -- use clusters centres as binary weight (skip zero weight)
          if ( cntr(h,i,j) ) {
            // -- store label
            int label = cntr(h,i,j);
            // -- proceed only when the centre is inside the cluster
            if ( clus(h,i,j) == label ) {
              // -- initialize counter
              int jpix = -1;
              // -- loop through the voxel-path
              for ( size_t ipix = 0 ; ipix < pix.shape(0) ; ++ipix ) {
                // -- get current voxel
                int dh = pix(ipix,0);
                int di = pix(ipix,1);
                int dj = pix(ipix,2);
                // -- loop through the voxel-path until the end of a cluster
                if ( clus(h+dh,i+di,j+dj) != label and jpix < 0 ) jpix = 0;
                // -- store: loop from the beginning of the path and store there
                if ( jpix >= 0 ) {
                  mNorm(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += 1.;
                  mData(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += f(h+dh,i+di,j+dj);
                }
                // -- update counter
                jpix++;
              }
            }
          }
        }
      }
    }
  }
}

// =================================================================================================
// weighted 2-point correlation collapsed to cluster centres -- "slave": compare to "master"
// =================================================================================================

void Ensemble::W2c(ArrI clus, ArrI cntr, ArrI f, std::string mode)
{
  // lock measure
  if ( mStat == Stat::Unset) mStat = Stat::W2c;

  // checks
  std::string name = "GooseEYE::Ensemble::W2c - ";
  if ( f.rank()  != mData.rank()  ) throw std::runtime_error(name+"rank inconsistent");
  if ( f.shape() != clus .shape() ) throw std::runtime_error(name+"shape inconsistent");
  if ( f.shape() != cntr .shape() ) throw std::runtime_error(name+"shape inconsistent");

  // switch off bound-checks based on periodicity settings
  clus .setPeriodic(mPeriodic);
  cntr .setPeriodic(mPeriodic);
  f    .setPeriodic(mPeriodic);

  // change rank (to avoid failing assertions)
  f.chrank(3);

  // list of end-points of ROI-stamp used in path-based correlations (make 3-d to simply below)
  MatI stamp = stampPoints(3);

  // correlation
  for ( size_t ipnt = 0 ; ipnt < stamp.shape(0) ; ++ipnt )
  {
    // - voxel-path
    MatI pix = path({0,0,0}, {stamp(ipnt,0), stamp(ipnt,1), stamp(ipnt,2)}, mode);
    // - compute correlation
    for ( int h = mSkip[0] ; h < f.shape<int>(0)-mSkip[0] ; ++h ) {
      for ( int i = mSkip[1] ; i < f.shape<int>(1)-mSkip[1] ; ++i ) {
        for ( int j = mSkip[2] ; j < f.shape<int>(2)-mSkip[2] ; ++j ) {
          // -- use clusters centres as binary weight (skip zero weight)
          if ( cntr(h,i,j) ) {
            // -- store label
            int label = cntr(h,i,j);
            // -- proceed only when the centre is inside the cluster
            if ( clus(h,i,j) == label ) {
              // -- initialize counter
              int jpix = -1;
              // -- loop through the voxel-path
              for ( size_t ipix = 0 ; ipix < pix.shape(0) ; ++ipix ) {
                // -- get current voxel
                int dh = pix(ipix,0);
                int di = pix(ipix,1);
                int dj = pix(ipix,2);
                // -- loop through the voxel-path until the end of a cluster
                if ( clus(h+dh,i+di,j+dj) != label and jpix < 0 ) jpix = 0;
                // -- store: loop from the beginning of the path and store there
                if ( jpix >= 0 ) {
                  mNorm(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += 1.;
                  if ( f(h+dh,i+di,j+dj) )
                    mData(mMid[0]+pix(jpix,0),mMid[1]+pix(jpix,1),mMid[1]+pix(jpix,2)) += 1.;
                }
                // -- update counter
                jpix++;
              }
            }
          }
        }
      }
    }
  }
}

// =================================================================================================
// wrapper functions
// =================================================================================================

void Ensemble::W2c_auto(ArrI w, ArrI f, std::string mode)
{
  ArrI clus, cntr;

  std::tie(clus, cntr) = clusterCenters(w, mPeriodic);

  W2c(clus, cntr, f, mode);
}

// -------------------------------------------------------------------------------------------------

void Ensemble::W2c_auto(ArrI w, ArrI f, ArrI fmask, std::string mode)
{
  ArrI clus, cntr;

  std::tie(clus, cntr) = clusterCenters(w, mPeriodic);

  W2c(clus, cntr, f, fmask, mode);
}

// -------------------------------------------------------------------------------------------------

void Ensemble::W2c_auto(ArrI w, ArrD f, std::string mode)
{
  ArrI clus, cntr;

  std::tie(clus, cntr) = clusterCenters(w, mPeriodic);

  W2c(clus, cntr, f, mode);
}

// -------------------------------------------------------------------------------------------------

void Ensemble::W2c_auto(ArrI w, ArrD f, ArrI fmask, std::string mode)
{
  ArrI clus, cntr;

  std::tie(clus, cntr) = clusterCenters(w, mPeriodic);

  W2c(clus, cntr, f, fmask, mode);
}

// =================================================================================================

} // namespace ...

// =================================================================================================

#endif
