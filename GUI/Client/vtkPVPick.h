/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPick.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPick - A special PVSource.
// .SECTION Description
// This class will set up defaults for thePVData.
// It will also create a special PointLabelDisplay.
// Both of these features should be specified in XML so we
// can get rid of this special case.


#ifndef __vtkPVPick_h
#define __vtkPVPick_h

#include "vtkPVSource.h"

class vtkSMPointLabelDisplayProxy;

class vtkCollection;
class vtkKWFrame;
class vtkKWLabel;
class vtkDataSetAttributes;
class vtkKWFrameWithLabel;
class vtkKWCheckButton;
class vtkKWThumbWheel;
class vtkPVArraySelection;
class vtkSMXYPlotDisplayProxy;
class vtkSMProxy;
class vtkTemporalPickObserver;
class vtkKWLoadSaveButton;

class VTK_EXPORT vtkPVPick : public vtkPVSource
{
public:
  static vtkPVPick* New();
  vtkTypeRevisionMacro(vtkPVPick, vtkPVSource);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  virtual void CreateProperties();

  // Description:
  // Called when the delete button is pressed.
  virtual void DeleteCallback();

  // Description:
  // Called by GUI to change Point Label appearance
  void PointLabelCheckCallback();
  void ChangePointLabelFontSize();

  // Description:
  // Refreshes GUI with current Point Label appearance
  void UpdatePointLabelCheck();
  void UpdatePointLabelFontSize();

  // Description:
  // Called when scalars are selected or deselected for the plot.
  void ArraySelectionInternalCallback();

  // Description:
  // Callback for the Save as comma separated values button.
  void SaveDialogCallback();

  // Description:
  // Access to the ShowXYPlotToggle from Tcl
  vtkGetObjectMacro(ShowXYPlotToggle, vtkKWCheckButton);

protected:
  vtkPVPick();
  ~vtkPVPick();

  vtkKWFrame *DataFrame;
  vtkCollection* LabelCollection;

  virtual void Select();
  void UpdateGUI();
  void ClearDataLabels();
  void InsertDataLabel(const char* label, vtkIdType idx,
                       vtkDataSetAttributes* attr, double* x=0);
  int LabelRow;

  // The real AcceptCallback method.
  virtual void AcceptCallbackInternal();  

  // Point label controls
  vtkKWFrameWithLabel *PointLabelFrame;
  vtkKWCheckButton *PointLabelCheck;
  vtkKWLabel       *PointLabelFontSizeLabel;
  vtkKWThumbWheel  *PointLabelFontSizeThumbWheel;

  // Added for temporal plot
  vtkKWFrameWithLabel *XYPlotFrame;
  vtkKWCheckButton *ShowXYPlotToggle;
  vtkPVArraySelection *ArraySelection;
  vtkSMXYPlotDisplayProxy* PlotDisplayProxy;
  char* PlotDisplayProxyName; 
  vtkSetStringMacro(PlotDisplayProxyName);
  vtkSMProxy* TemporalProbeProxy;
  char* TemporalProbeProxyName; 
  vtkSetStringMacro(TemporalProbeProxyName);
  vtkTemporalPickObserver *Observer;
  vtkKWLoadSaveButton *SaveButton;
  int LastPorC;
  int LastUseId;

private:
  vtkPVPick(const vtkPVPick&); // Not implemented
  void operator=(const vtkPVPick&); // Not implemented
};

#endif
