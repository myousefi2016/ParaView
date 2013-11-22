/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef IntersectionSetColorMapper_h
#define IntersectionSetColorMapper_h

#include "IntersectionSet.h"
#include "SQMacros.h"

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>

#include "vtkIntArray.h"

/**
/// Class that manages color assignement operation for insterection sets.
Class that manages color assignement operation for insterection sets. This class
assigns an intersection based on the intersection set surface ids and the number of
surfaces. Two sets, S1 and S2, are equivalent (and tehrefor are assigned the same
color) when S1=(a b) and S2=(b a).
*/
class IntersectionSetColorMapper
{
public:
  /// Construct in an invalid state.
  IntersectionSetColorMapper() : NSurfaces(0)
    {
    #ifdef SQTK_WITHOUT_MPI
    sqErrorMacro(
      std::cerr,
      << "This class requires MPI however it was built without MPI.");
    #endif
    };
  /// Copy construct.
  IntersectionSetColorMapper(const IntersectionSetColorMapper &other){
    *this=other;
    }
  /// Assignment.
  const IntersectionSetColorMapper &operator=(const IntersectionSetColorMapper &other){
    if (this!=&other)
      {
      this->NSurfaces=other.NSurfaces;
      this->Colors=other.Colors;
      }
    return *this;
    }
  /// Set the number of surfaces, this allocates space for the colors.
  /// and appropriately initializes them.
  void SetNumberOfSurfaces(int nSurfaces){
    this->BuildColorMap(nSurfaces);
    }
  /// Allocates space for the color map and appropriately initializes
  /// them as a function of surface pairs. A legend is also generated
  /// if a the surfaceName vector is provided these are used in the
  /// legend.
  void BuildColorMap(int nSurfaces){
    std::vector<std::string> names(nSurfaces);
    for (int i=0; i<nSurfaces; ++i)
      {
      std::ostringstream os;
      os << i;
      names[i]=os.str();
      }
    this->BuildColorMap(nSurfaces,names);
    }
  void BuildColorMap(int nSurfaces, std::vector<std::string> &names){
    // Store the color values in the upper triangle of a nxn matrix.
    // Note that we only need n=sum_{i=1}^{n+1}i colors, but using the
    // matrix sinmplifies look ups.
    this->NSurfaces=nSurfaces;
    const int nColors=(nSurfaces+1)*(nSurfaces+1);
    this->Colors.clear();
    this->Colors.resize(nColors,-1);
    this->ColorsUsed.resize(nColors,0);
    this->ColorLegend.resize(nColors);
    int color=0;
    for (int j=0; j<nSurfaces+1; ++j)
      {
      for (int i=j; i<nSurfaces+1; ++i)
        {
        // set the color entry
        int x=std::max(i,j);
        int y=std::min(i,j);
        int idx=x+(nSurfaces+1)*y;
        this->Colors[idx]=color;
        ++color;
        // set its legend entry
        std::ostringstream os;
        os << "(" << names[x] << ", " << names[y] << ")";
        this->ColorLegend[idx]=os.str();
        }
      }
    }
  /// Get color associated with an intersection set.
  int LookupColor(const IntersectionSet &iset){
    int s1=iset.GetForwardIntersectionId()+1;
    int s2=iset.GetBackwardIntersectionId()+1;
    return this->LookupColor(s1,s2);
    }
  /// Get color associated with 2 surfaces.
  int LookupColor(const int s1, const int s2){
    int x=std::max(s1,s2);
    int y=std::min(s1,s2);
    int idx=x+(this->NSurfaces+1)*y;
    this->ColorsUsed[idx]=1;
    return this->Colors[idx];
    }
  /// reduce the number of colors to those which are used.
  void SqueezeColorMap(vtkIntArray *scalar){
    #ifdef SQTK_WITHOUT_MPI
    (void)scalar;
    #else
    int procId=0;
    MPI_Comm_rank(MPI_COMM_WORLD,&procId);
    // Walk the color map, for any used color replace it with
    // the number of colors used thus far. This will reduce
    // the color map to only the used colors.
    vtkIdType nCells=scalar->GetNumberOfTuples();
    int *pScalar=scalar->GetPointer(0);
    int nUsed=0;
    for (int j=0; j<this->NSurfaces+1; ++j)
      {
      for (int i=j; i<this->NSurfaces+1; ++i)
        {
        int x=std::max(i,j);
        int y=std::min(i,j);
        int idx=x+(this->NSurfaces+1)*y;
        int color=this->Colors[idx];
        int colorUsed=0;
        MPI_Allreduce(&this->ColorsUsed[idx],&colorUsed,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
        if (colorUsed)
          {
          // print the lengend.
          if (procId==0)
            {
            std::cerr << this->ColorLegend[idx] << "->" << nUsed << std::endl;
            }
          for (vtkIdType q=0; q<nCells; ++q)
            {
            // search and replace the old value with the new.
            if (pScalar[q]==color)
              {
              pScalar[q]=nUsed;
              }
            }
          // next new color.
          ++nUsed;
          }
        }
      }
    #endif
    }
  /// Process 0 send the used colors in the color map to the terminal.
  void PrintUsed(){
    #ifndef SQTK_WITHOUT_MPI
    int procId=0;
    MPI_Comm_rank(MPI_COMM_WORLD,&procId);
    for (int j=0; j<this->NSurfaces+1; ++j)
      {
      for (int i=j; i<this->NSurfaces+1; ++i)
        {
        int x=std::max(i,j);
        int y=std::min(i,j);
        int idx=x+(this->NSurfaces+1)*y;
        int colorUsed=0;
        MPI_Allreduce(&this->ColorsUsed[idx],&colorUsed,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
        if (colorUsed)
          {
          // print the lengend.
          if (procId==0)
            {
            std::cerr << this->ColorLegend[idx] << "->" << this->Colors[idx] << std::endl;
            }
          }
        }
      }
    #endif
    }
  /// Process 0 send the color map to the terminal.
  void PrintAll()
    {
    #ifndef SQTK_WITHOUT_MPI
    int procId=0;
    MPI_Comm_rank(MPI_COMM_WORLD,&procId);
    if (procId!=0)
      {
      return;
      }
    for (int j=0; j<this->NSurfaces+1; ++j)
      {
      for (int i=j; i<this->NSurfaces+1; ++i)
        {
        int x=std::max(i,j);
        int y=std::min(i,j);
        int idx=x+(this->NSurfaces+1)*y;
        // print the lengend.
        std::cerr << this->ColorLegend[idx] << "->" << this->Colors[idx] << std::endl;
        }
      }
    #endif
    }

private:
  int NSurfaces;
  std::vector<int> Colors;
  std::vector<int> ColorsUsed;
  std::vector<std::string> ColorLegend;
};

#endif
