/*=========================================================================

   Program: ParaView
   Module:    pqObjectInspectorWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqObjectInspectorWidget.h"

// Qt includes
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QTabWidget>
#include <QApplication>
#include <QStyle>
#include <QStyleOption>

// vtk includes
#include "QVTKWidget.h"

// ParaView Server Manager includes

// ParaView includes
#include "pqApplicationCore.h"
#include "pqAutoGeneratedObjectPanel.h"
#include "pqClipPanel.h"
#include "pqContourPanel.h"
#include "pqCutPanel.h"
#include "pqExodusPanel.h"
#include "pqThresholdPanel.h"
#include "pqLoadedFormObjectPanel.h"
#include "pqServerManagerObserver.h"
#include "pqPropertyManager.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqStreamTracerPanel.h"

//-----------------------------------------------------------------------------
pqObjectInspectorWidget::pqObjectInspectorWidget(QWidget *p)
  : QWidget(p)
{
  this->setObjectName("ObjectInspectorWidget");

  this->ForceModified = false;
  this->CurrentPanel = 0;
  
  // main layout
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(0);

  QScrollArea*s = new QScrollArea();
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("ScrollArea");

  this->PanelArea = new QWidget;
  this->PanelArea->setSizePolicy(QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding);
  QVBoxLayout *panelLayout = new QVBoxLayout(this->PanelArea);
  panelLayout->setMargin(0);
  s->setWidget(this->PanelArea);
  this->PanelArea->setObjectName("PanelArea");

  QBoxLayout* buttonlayout = new QHBoxLayout();
  this->AcceptButton = new QPushButton(this);
  this->AcceptButton->setObjectName("Accept");
  this->AcceptButton->setText(tr("Accept"));
  this->AcceptButton->setIcon(QIcon(QPixmap(":/pqWidgets/pqUpdate16.png")));
  this->ResetButton = new QPushButton(this);
  this->ResetButton->setObjectName("Reset");
  this->ResetButton->setText(tr("Reset"));
  this->ResetButton->setIcon(QIcon(QPixmap(":/pqWidgets/pqCancel16.png")));
  buttonlayout->addStretch();
  buttonlayout->addWidget(this->AcceptButton);
  buttonlayout->addWidget(this->ResetButton);
  buttonlayout->addStretch();
  
  mainLayout->addLayout(buttonlayout);
  mainLayout->addWidget(s);

  this->connect(this->AcceptButton, SIGNAL(clicked()), SLOT(accept()));
  this->connect(this->ResetButton, SIGNAL(clicked()), SLOT(reset()));

  this->AcceptButton->setEnabled(false);
  this->ResetButton->setEnabled(false);


  this->connect(pqApplicationCore::instance()->getPipelineData(), 
                SIGNAL(sourceUnRegistered(vtkSMProxy*)),
                SLOT(removeProxy(vtkSMProxy*)));
 
}

//-----------------------------------------------------------------------------
pqObjectInspectorWidget::~pqObjectInspectorWidget()
{
  // delete all queued panels
  foreach(pqObjectPanel* p, this->PanelStore)
    {
    delete p;
    }
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::forceModified(bool status)
{
  this->ForceModified = status;
  this->canAccept(status);
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::canAccept(bool status)
{
  this->AcceptButton->setEnabled(status);
  this->ResetButton->setEnabled(status);
}

//-----------------------------------------------------------------------------
void pqObjectInspectorWidget::setProxy(vtkSMProxy *proxy)
{
  // do nothing if this proxy is already current
  if(this->CurrentPanel && this->CurrentPanel->proxy() == proxy)
    {
    return;
    }

  if (this->CurrentPanel)
    {
    this->PanelArea->layout()->takeAt(0);
    this->CurrentPanel->deselect();
    this->CurrentPanel->hide();
    this->CurrentPanel->setObjectName("");
    // We don't delete the panel, it's managed by this->PanelStore.
    // Any deletion of any panel must ensure that call pointers 
    // to the panel ie. this->CurrentPanel, this->QueuedPanels, and this->PanelStore
    // are updated so that we don't have any dangling pointers.
    }

  // we have a proxy with pending changes
  if(this->AcceptButton->isEnabled())
    {
    // save the panel
    if(this->CurrentPanel)
      {
      // QueuedPanels keeps track of all modified panels.
      this->QueuedPanels.insert(this->CurrentPanel->proxy(),
        this->CurrentPanel);
      }
    }

  this->CurrentPanel = NULL;

  // search for a custom form for this proxy with pending changes
  QMap<pqSMProxy, pqObjectPanel*>::iterator iter;
  iter = this->PanelStore.find(proxy);
  if(iter != this->PanelStore.end())
    {
    this->CurrentPanel = iter.value();
    }

  if(proxy && !this->CurrentPanel)
    {
    const QString xml_name = proxy->GetXMLName();
    
    if(xml_name == "Clip")
      {
      this->CurrentPanel = new pqClipPanel(0);
      }
    else if(xml_name == "Contour")
      {
      this->CurrentPanel = new pqContourPanel(0);
      }
    else if(xml_name == "Cut")
      {
      this->CurrentPanel = new pqCutPanel(0);
      }
    else if(xml_name == "ExodusReader")
      {
      this->CurrentPanel = new pqExodusPanel(0);
      }
    else if(xml_name == "StreamTracer")
      {
      this->CurrentPanel = new pqStreamTracerPanel(0);
      }
    else if(xml_name == "Threshold")
      {
      this->CurrentPanel = new pqThresholdPanel(0);
      }
    else
      {
      // try to find a custom form in our pqWidgets resources
      QString proxyui = QString(":/pqWidgets/") + QString(proxy->GetXMLName()) + QString(".ui");
      pqLoadedFormObjectPanel* panel = new pqLoadedFormObjectPanel(proxyui, NULL);
      if(!panel->isValid())
        {
        delete panel;
        panel = NULL;
        }
        this->CurrentPanel = panel;
      }
    }

  if(this->CurrentPanel == NULL)
    {
    this->CurrentPanel = new pqAutoGeneratedObjectPanel;
    }

  // the current auto panel always has the name "Editor"
  this->CurrentPanel->setObjectName("Editor");
  this->CurrentPanel->setProxy(proxy);
  
  QObject::connect(this->CurrentPanel->getPropertyManager(), 
    SIGNAL(canAcceptOrReject(bool)), this, SLOT(canAccept(bool)));

  QObject::connect(this->CurrentPanel, SIGNAL(canAcceptOrReject(bool)), 
                   this, SLOT(canAccept(bool)));
  
  this->PanelArea->layout()->addWidget(this->CurrentPanel);
  this->CurrentPanel->select();
  this->CurrentPanel->show();

  this->PanelStore[proxy] = this->CurrentPanel;
}

void pqObjectInspectorWidget::accept()
{
  emit this->preaccept();

  // accept all queued panels
  foreach(pqObjectPanel* p, this->QueuedPanels)
    {
    p->accept();
    }
  
  if (this->CurrentPanel)
    {
    this->CurrentPanel->accept();
    }
 
  pqApplicationCore::instance()->getActiveRenderModule()->forceRender();

  /* For now just render the active window, later we will
     render all the windows to which this source belongs.
  // cause the screen to update
  pqPipelineSource *source = 
    pqServerManagerModel::instance()->getPQSource(proxy);
  if(source)
    {
    // FIXME
    // source->getDisplay()->UpdateWindows();
    }
    */
  
  emit this->accepted();
  
  foreach(pqObjectPanel* p, this->QueuedPanels)
    {
    p->postAccept();
    }
  this->QueuedPanels.clear();
  
  if (this->CurrentPanel)
    {
    this->CurrentPanel->postAccept();
    }
  
  this->ForceModified = false;
  emit this->postaccept();
}

void pqObjectInspectorWidget::reset()
{
  emit this->prereject();

  // reset all queued panels
  foreach(pqObjectPanel* p, this->QueuedPanels)
    {
    p->reset();
    }
  this->QueuedPanels.clear();
  
  if(this->CurrentPanel)
    {
    this->CurrentPanel->reset();
    }

  if (this->ForceModified)
    {
    this->forceModified(true);
    }
  emit postreject();
}

QSize pqObjectInspectorWidget::sizeHint() const
{
  // return a size hint that would reasonably fit several properties
  ensurePolished();
  QFontMetrics fm(font());
  int h = 20 * (qMax(fm.lineSpacing(), 14));
  int w = fm.width('x') * 25;
  QStyleOptionFrame opt;
  opt.rect = rect();
  opt.palette = palette();
  opt.state = QStyle::State_None;
  return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).
                                    expandedTo(QApplication::globalStrut()), this));
}

void pqObjectInspectorWidget::removeProxy(vtkSMProxy* proxy)
{
  QMap<pqSMProxy, pqObjectPanel*>::iterator iter;
  iter = this->QueuedPanels.find(proxy);
  if(iter != this->QueuedPanels.end())
    {
    this->QueuedPanels.erase(iter);
    }

  if(this->CurrentPanel && this->CurrentPanel->proxy() == proxy)
    {
    this->CurrentPanel = NULL;
    }

  iter = this->PanelStore.find(proxy);
  if (iter != this->PanelStore.end())
    {
    delete iter.value();
    this->PanelStore.erase(iter);
    }
}

