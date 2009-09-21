/*=========================================================================
Copyright 2009 Rensselaer Polytechnic Institute
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. 
=========================================================================*/

/*
  Farsight ToolKit 3D Viewer: 
  v1: implements render window functionality
    render window creation in "View3D::RenderWin"
    line objects mapped and actors created in "View3D::LineAct"
    actor added and window created in "View3D::addAct"
  v2: add picking functionality:
    access to observing a pick event is in "View3D::interact"
    the interacter calls for "View3D::PickPoint"
  v3: include contourFilter and rayCast renderers
  v4: converted to Qt, member names changed to fit "VTK style" more closely.
  v5: automated functions implemented structure in place for "PACE".
  v6: "ALISA" implemented
*/

#include "ftkGUI/PlotWindow.h"
#include "ftkGUI/HistoWindow.h"
#include "ftkGUI/TableWindow.h"

#include "itkImageFileReader.h"
#include "itkImageToVTKImageFilter.h"

#include <QAction>
#include <QtGui>
#include <QVTKWidget.h>

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellPicker.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkContourFilter.h"
#include "vtkCubeSource.h"
#include "vtkFloatArray.h"
#include "vtkGlyph3D.h"
#include "vtkImageData.h"
#include "vtkImageToStructuredPoints.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLODActor.h"
#include "vtkOpenGLVolumeTextureMapper3D.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlaybackRepresentation.h"
#include "vtkPlaybackWidget.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSliderRepresentation2D.h"
#include "vtkSliderWidget.h"
#include "vtkSphereSource.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include "TraceBit.h"
#include "TraceGap.h"
#include "TraceLine.h"
#include "TraceObject.h"
#include "TraceModel.h"
#include "MergeModel.h"
#include "View3DHelperClasses.h"
#include "View3D.h"

View3D::View3D(int argc, char **argv)
{
  unsigned int sz = 10;
  this->tobj = new TraceObject;
  this->undoBuff = new bufferType(sz);
  int num_loaded = 0;
  this->Volume=0;

  // load as many files as possible. Provide offset for differentiating types
  for(int counter=1; counter<argc; counter++)
    {
    int len = strlen(argv[counter]);
    if(strcmp(argv[counter]+len-3,"swc")==0)
      {
      printf("I detected swc\n");
      this->tobj->ReadFromSWCFile(argv[counter]);
       }
    else if (strcmp(argv[counter]+len-3,"xml")==0)
      {
      printf("I detected xml\n");
      this->tobj->ReadFromRPIXMLFile(argv[counter]);
      }
    else if( strcmp(argv[counter]+len-3,"tks")==0)
      {
      printf("I detected tks\n");
      this->tobj->ReadFromFeatureTracksFile(argv[counter],num_loaded);
      }
    else if( strcmp(argv[counter]+len-3,"tif")==0 ||
             strcmp(argv[counter]+len-4,"tiff")==0 ||
       strcmp(argv[counter]+len-3, "pic")==0||
       strcmp(argv[counter]+len-3, "PIC")==0)
      {
      printf("I detected a 3d image file\n");
      this->rayCast(argv[counter]);
      //this->AddVolumeSliders();
      }
    num_loaded++;
    }

  //The copy constructor does not appear to be working, the program segfaults on that line, code should work, if fixed
#if 0
  std::string orig = "Original";
  TraceObject newTobj(*(this->tobj));
  std::pair<std::string, TraceObject> newPair;
  newPair.first = orig;
  newPair.second = newTobj;
  undoBuff->Add(newPair);
#endif
  
this->QVTK = 0;
this->OpacitySlider = 0;
this->BrightnessSlider = 0;
this->tobj->gapTol = .5;
this->tobj->gapMax = 10;
this->smallLine = 5;
this->SelectColor =.1;
this->lineWidth= 2;

this->Initialize();
this->tobj->setSmallLineColor(.25);
this->tobj->setMergeLineColor(.4);
this->Ascending = Qt::AscendingOrder;
this->statusBar()->showMessage(tr("Ready"));

}
void View3D::LoadTraces()
{
	std::string traceFile;
	QString trace = QFileDialog::getOpenFileName(this , "Load Trace Data", ".",
		tr("TraceFile(*.xml *.swc"));
	if (!trace.isEmpty())
	{
		traceFile = trace.toStdString();
		if(trace.endsWith("swc"))
		{
			this->statusBar()->showMessage("Loading swc file");
			this->tobj->ReadFromSWCFile((char*)traceFile.c_str());
		}
		else if (trace.endsWith("xml"))
		{
			this->statusBar()->showMessage("Loading xml file");
			this->tobj->ReadFromRPIXMLFile((char*)traceFile.c_str());
		}
		this->UpdateLineActor();
		this->UpdateBranchActor();
		this->QVTK->GetRenderWindow()->Render();
		this->statusBar()->showMessage(tr("Update Tree Plots"));
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
	}	
	else
	{
		this->statusBar()->showMessage("Please select a Trace Data file");
	}
}
void View3D::LoadImageData()
{
	QString trace = QFileDialog::getOpenFileName(this , "Load Trace Image Data", ".",
		tr("Trace Image (*.tiff *.tif *.pic *.PIC"));
	if (!trace.isEmpty())
	{
		this->statusBar()->showMessage("Loading Image file");
		std::string traceFile = trace.toStdString();
		this->rayCast( (char*)traceFile.c_str());
		this->Renderer->AddActor(this->Volume);
		this->AddVolumeSliders();
		this->statusBar()->showMessage("Image File Rendered");
	}
	else
	{
		this->statusBar()->showMessage("Please select an Image file");
	}
}
View3D::~View3D()
{
  if(this->QVTK)
    {
    delete this->QVTK;
    }
  if(this->GapsPlotView)
    {
    delete this->GapsPlotView;
    }
  if(this->OpacitySlider)
    {
    this->OpacitySlider->Delete();
    }
  if(this->BrightnessSlider)
    {
    this->BrightnessSlider->Delete();
    }
  delete this->tobj;
  delete this->undoBuff;
  //the various Qt objects should be getting deleted by closeEvent and
  //parent/child relationships...
}

void View3D::Initialize()
{
  this->CreateGUIObjects();
  this->CreateLayout();
  this->CreateInteractorStyle();
  this->CreateActors();
  this->resize(640, 480);
  this->setWindowTitle(tr("Trace editor"));
  this->QVTK->GetRenderWindow()->Render();
  this->setupLinkedSpace();
}

void View3D::setupLinkedSpace()
{  
  this->tobj->Gaps.clear();
  this->GapsPlotView = NULL;
  this->TreePlot = NULL;
  this->histo = NULL;
  this->GapsTableView = new QTableView();
  this->TreeTable =new QTableView();
  this->MergeGaps = new MergeModel(this->tobj->Gaps);
  this->MergeGaps->setParent(this);
  if (this->tobj->FeatureHeaders.size() >=1)
  {
	  this->TreeModel = new TraceModel(this->tobj->GetTraceLines(), this->tobj->FeatureHeaders);
  }
  else
  {
	  this->TreeModel = new TraceModel(this->tobj->GetTraceLines());
  }
  this->TreeModel->setParent(this);
  this->ShowTreeData();
  this->connect(MergeGaps,SIGNAL(modelChanged()),this, SLOT(updateSelectionHighlights()));
  this->connect(this->MergeGaps->GetSelectionModel(), SIGNAL(selectionChanged(
	  const QItemSelection & , const QItemSelection &)), this, SLOT(updateSelectionHighlights()));

  this->connect(this->TreeModel, SIGNAL(modelChanged()),this, SLOT(updateTraceSelectionHighlights()));
  this->connect(this->TreeModel->GetSelectionModel(), SIGNAL(selectionChanged(
	  const QItemSelection & , const QItemSelection &)), this, SLOT(updateTraceSelectionHighlights()));
}

/*Set up the components of the interface */
void View3D::CreateGUIObjects()
{
  //Set up the main window's central widget
  this->CentralWidget = new QWidget(this);
  this->setCentralWidget(this->CentralWidget);

  //Set up a QVTK Widget for embedding a VTK render window in Qt.
  this->QVTK = new QVTKWidget(this->CentralWidget);
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->QVTK->GetRenderWindow()->AddRenderer(this->Renderer);

  //Set up the menu bar
  this->fileMenu = this->menuBar()->addMenu(tr("&File"));
  this->saveAction = fileMenu->addAction(tr("&Save as..."));
    connect(this->saveAction, SIGNAL(triggered()), this, SLOT(SaveToFile()));
  this->exitAction = fileMenu->addAction(tr("&Exit"));
  connect(this->exitAction, SIGNAL(triggered()), this, SLOT(close()));
  this->loadTraceAction = new QAction("Load Trace", this->CentralWidget);
    connect(this->loadTraceAction, SIGNAL(triggered()), this, SLOT(LoadTraces()));
    this->loadTraceAction->setStatusTip("Load traces from .xml or .swc file");
	this->fileMenu->addAction(this->loadTraceAction);
  this->loadTraceImage = new QAction("Load Image", this->CentralWidget);
	connect (this->loadTraceImage, SIGNAL(triggered()), this, SLOT(LoadImageData()));
	this->loadTraceImage->setStatusTip("Load an Image to RayCast Rendering");
	this->fileMenu->addAction(this->loadTraceImage);

  //Set up the buttons that the user will use to interact with this program. 
  this->ListButton = new QAction("List", this->CentralWidget);
	connect(this->ListButton, SIGNAL(triggered()), this, SLOT(ListSelections()));
	this->ListButton->setStatusTip("List all selections");
  this->ClearButton = new QAction("Clear", this->CentralWidget); 
	connect(this->ClearButton, SIGNAL(triggered()), this, SLOT(ClearSelection()));
	this->ClearButton->setStatusTip("Clear all selections");
  this->DeleteButton = new QAction("Delete", this->CentralWidget);
	connect(this->DeleteButton, SIGNAL(triggered()), this, SLOT(DeleteTraces()));
	this->DeleteButton->setStatusTip("Delete all selected traces");
  this->MergeButton = new QAction("Merge", this->CentralWidget);
	connect(this->MergeButton, SIGNAL(triggered()), this, SLOT(MergeTraces()));
	this->MergeButton->setStatusTip("Start Merge on selected traces");
  this->BranchButton = new QAction("Branch", this->CentralWidget);
	connect(this->BranchButton, SIGNAL(triggered()), this, SLOT(AddNewBranches()));
	this->BranchButton->setStatusTip("Add branches to trunk");
  this->SplitButton = new QAction("Split", this->CentralWidget); 
	connect(this->SplitButton, SIGNAL(triggered()), this, SLOT(SplitTraces()));
	this->SplitButton->setStatusTip("Split traces at point where selected");
  this->FlipButton = new QAction("Flip", this->CentralWidget);
	connect(this->FlipButton, SIGNAL(triggered()), this, SLOT(FlipTraces()));
	this->FlipButton->setStatusTip("Flip trace direction");
  this->WriteButton = new QAction("save as...", this->CentralWidget);
	connect(this->WriteButton, SIGNAL(triggered()), this, SLOT(SaveToFile()));
  this->SettingsButton = new QAction("Settings", this->CentralWidget);
	connect(this->SettingsButton, SIGNAL(triggered()), this,
		SLOT(ShowSettingsWindow()));
	this->SettingsButton->setStatusTip("edit the display and tolerance settings");
  this->AutomateButton = new QAction("Small Lines", this->CentralWidget);
	connect(this->AutomateButton, SIGNAL(triggered()), this, SLOT(SLine()));
	this->AutomateButton->setStatusTip("Automatic selection of all small lines");

  this->UndoButton = new QAction("&Undo", this->CentralWidget);  
	connect(this->UndoButton, SIGNAL(triggered()), this, SLOT(UndoAction()));
  this->RedoButton = new QAction("&Redo", this->CentralWidget);
	connect(this->RedoButton, SIGNAL(triggered()), this, SLOT(RedoAction()));
  //Setup the tolerance settings editing window
  this->SettingsWidget = new QWidget();
  QIntValidator *intValidator = new QIntValidator(1, 100, this->SettingsWidget);
  QDoubleValidator *doubleValidator =
    new QDoubleValidator(0, 100, 2, this->SettingsWidget);
  this->MaxGapField = new QLineEdit(this->SettingsWidget);
  this->MaxGapField->setValidator(intValidator);
  this->GapToleranceField = new QLineEdit(this->SettingsWidget);
  this->GapToleranceField->setValidator(intValidator);
  this->LineLengthField = new QLineEdit(this->SettingsWidget);
  this->LineLengthField->setValidator(doubleValidator);
  this->ColorValueField = new QLineEdit(this->SettingsWidget);
  this->ColorValueField->setValidator(doubleValidator);
  this->LineWidthField = new QLineEdit(this->SettingsWidget);
  this->LineWidthField->setValidator(doubleValidator);
  this->ApplySettingsButton = new QPushButton("&Apply", this->SettingsWidget);
  this->CancelSettingsButton = new QPushButton("&Cancel", this->SettingsWidget);
  connect(this->ApplySettingsButton, SIGNAL(clicked()), this,
    SLOT(ApplyNewSettings()));
  connect(this->CancelSettingsButton, SIGNAL(clicked()), this,
    SLOT(HideSettingsWindow())); 
//Loading soma data
  this->loadSoma = new QAction("Load Somas", this->CentralWidget);
   connect(this->loadSoma, SIGNAL(triggered()), this, SLOT(GetSomaFile()));
   this->fileMenu->addAction(this->loadSoma);
 
}

void View3D::CreateLayout()
{
  this->EditsToolBar = addToolBar(tr("Edit Toolbar"));
  /*this->EditsToolBar->addAction(this->saveAction);
  this->EditsToolBar->addAction(this->exitAction);
  this->EditsToolBar->addSeparator();*/
  this->EditsToolBar->addAction(this->AutomateButton);
  this->EditsToolBar->addAction(this->ListButton);
  this->EditsToolBar->addAction(this->ClearButton);
  this->EditsToolBar->addSeparator();
  this->EditsToolBar->addAction(this->DeleteButton);
  this->EditsToolBar->addAction(this->MergeButton);
  this->EditsToolBar->addAction(this->BranchButton);
  this->EditsToolBar->addAction(this->SplitButton);
  this->EditsToolBar->addAction(this->FlipButton);
  this->EditsToolBar->addSeparator();
  this->EditsToolBar->addAction(this->loadSoma);
  this->EditsToolBar->addAction(this->SettingsButton);

  QGridLayout *viewerLayout = new QGridLayout(this->CentralWidget);
  viewerLayout->addWidget(this->QVTK, 0, 0);
  //may add a tree view here
   //layout for the settings window
  QGridLayout *settingsLayout = new QGridLayout(this->SettingsWidget);
  QLabel *maxGapLabel = new QLabel(tr("Maximum gap length:"));
  settingsLayout->addWidget(maxGapLabel, 0, 0);
  settingsLayout->addWidget(this->MaxGapField, 0, 1);
  QLabel *gapToleranceLabel = new QLabel(tr("Gap length tolerance:"));
  settingsLayout->addWidget(gapToleranceLabel, 1, 0);
  settingsLayout->addWidget(this->GapToleranceField, 1, 1);
  QLabel *lineLengthLabel = new QLabel(tr("Small line length:"));
  settingsLayout->addWidget(lineLengthLabel, 2, 0);
  settingsLayout->addWidget(this->LineLengthField, 2, 1);
  QLabel *colorValueLabel = new QLabel(tr("Color value RGB scalar 0 to 1:"));
  settingsLayout->addWidget(colorValueLabel, 3, 0);
  settingsLayout->addWidget(this->ColorValueField, 3, 1);
  QLabel *lineWidthLabel = new QLabel(tr("Line width:"));
  settingsLayout->addWidget(lineWidthLabel, 4, 0);
  settingsLayout->addWidget(this->LineWidthField, 4, 1);
  settingsLayout->addWidget(this->ApplySettingsButton, 5, 0);
  settingsLayout->addWidget(this->CancelSettingsButton, 5, 1); 

  //Layout for the Load Soma Window
}

void View3D::CreateInteractorStyle()
{
  this->Interactor = this->QVTK->GetRenderWindow()->GetInteractor();
  //keep mouse command observers, but change the key ones
  this->keyPress = vtkSmartPointer<vtkCallbackCommand>::New();
  this->keyPress->SetCallback(HandleKeyPress);
  this->keyPress->SetClientData(this);

  this->Interactor->RemoveObservers(vtkCommand::KeyPressEvent);
  this->Interactor->RemoveObservers(vtkCommand::KeyReleaseEvent);
  this->Interactor->RemoveObservers(vtkCommand::CharEvent);
  this->Interactor->AddObserver(vtkCommand::KeyPressEvent, this->keyPress);

  //use trackball control for mouse commands
  vtkSmartPointer<vtkInteractorStyleTrackballCamera> style =
    vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  this->Interactor->SetInteractorStyle(style);
  this->CellPicker = vtkSmartPointer<vtkCellPicker>::New();
  this->CellPicker->SetTolerance(0.004);
  this->Interactor->SetPicker(this->CellPicker);
  this->isPicked = vtkSmartPointer<vtkCallbackCommand>::New();
  this->isPicked->SetCallback(PickCell);

  //isPicked caller allows observer to intepret click 
  this->isPicked->SetClientData(this);            
  this->Interactor->AddObserver(vtkCommand::RightButtonPressEvent,isPicked);
}

void View3D::CreateActors()
{
  this->LineActor = vtkSmartPointer<vtkActor>::New();
  this->LineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->UpdateLineActor();
  this->LineActor->SetPickable(1);
  this->Renderer->AddActor(this->LineActor);

  this->UpdateBranchActor();
  this->Renderer->AddActor(this->BranchActor);
 
  if(this->Volume!=NULL)
  {
    this->Renderer->AddVolume(this->Volume);
    this->AddVolumeSliders();
  }

  //this->Renderer->AddActor(this->VolumeActor);

  //sphere is used to mark the picks
  this->CreateSphereActor();
  Renderer->AddActor(this->SphereActor);
}

void View3D::CreateSphereActor()
{
  this->Sphere = vtkSmartPointer<vtkSphereSource>::New();
  this->Sphere->SetRadius(3);
  this->SphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->SphereMapper->SetInput(this->Sphere->GetOutput());
  this->SphereMapper->GlobalImmediateModeRenderingOn();

  this->SphereActor = vtkSmartPointer<vtkActor>::New();
  this->SphereActor->SetMapper(this->SphereMapper);
  this->SphereActor->GetProperty()->SetOpacity(.3);
  this->SphereActor->VisibilityOff();
  this->SphereActor->SetPickable(0);            //dont want to pick the sphere itself
}

/* update settings */
void View3D::ShowSettingsWindow()
{
  //make sure the values in the input fields are up-to-date
  this->MaxGapField->setText(QString::number(this->tobj->gapMax));
  this->GapToleranceField->setText(QString::number(this->tobj->gapTol));
  this->LineLengthField->setText(QString::number(this->smallLine));
  this->ColorValueField->setText(QString::number(this->SelectColor));
  this->LineWidthField->setText(QString::number(this->lineWidth));
  this->SettingsWidget->show();
}

void View3D::ApplyNewSettings()
{
  this->tobj->gapMax = this->MaxGapField->text().toInt();
  this->tobj->gapTol = this->GapToleranceField->text().toDouble();
  this->smallLine = this->LineLengthField->text().toFloat();
  this->SelectColor = this->ColorValueField->text().toDouble();
  this->lineWidth = this->LineWidthField->text().toFloat();
  //this->Rerender();
  this->SettingsWidget->hide();
  this->statusBar()->showMessage(tr("Applying new settings"),3000);
}

void View3D::HideSettingsWindow()
{
  this->SettingsWidget->hide();
}
bool View3D::setTol()
{
  bool change = false;
  char select=0;
  std::cout<<"Settings Configuration:\n gap (t)olerance:\t" <<this->tobj->gapTol
    <<"\ngap (m)ax:\t"<<this->tobj->gapMax
    <<"\n(s)mall line:\t"<< smallLine
    <<"\nselection (c)olor:\t"<<SelectColor
    <<"\nline (w)idth:\t"<<lineWidth
    <<"\n(e)nd edit settings\n";
  while (select !='e')
  {
    std::cout<< "select option:\t"; 
    std::cin>>select;
    switch(select)
    {
    case 'm':
      {
        int newMax;
        std::cout<< "maximum gap length\n";
        std::cin>>newMax;
        if (newMax!=this->tobj->gapMax)
        {
          this->tobj->gapMax=newMax;
          change= true;
        }
        break;
      }//end of 'm'
    case 't':
      {
        double newTol;
        std::cout<< "gap length tolerance\n";
        std::cin>>newTol;
        if (newTol!=this->tobj->gapTol)
        {
          this->tobj->gapTol=newTol;
          change= true;
        }
        break;
      }//end of 't'
    case 's':
      {
        float newSmall;
        std::cout<< "small line length\n";
        std::cin>>newSmall;
        if (newSmall!=smallLine)
        {
          smallLine=newSmall;
          change= true;
        }
        break;
      }// end of 's'
    case 'c':
      {
        double newColor;
        std::cout<< "color value RGB scalar 0 to 1\n";
        std::cin>>newColor;
        if (newColor!=SelectColor)
        {
          SelectColor=newColor;
          change= true;
        }
        break;
      }//end of 'c'
    case 'w':
      {
        float newWidth;
        std::cout<<"line Width\n";
        std::cin>>newWidth;
        if (newWidth!=lineWidth)
        {
          lineWidth=newWidth;
          change= true;
        }
        break;
      }
    }//end of switch
  }// end of while
  if (change== true)
  {std::cout<<"Settings Configuration are now:\n gap tollerance:\t" <<this->tobj->gapTol
    <<"\ngap max:\t"<<this->tobj->gapMax
    <<"\nsmall line:\t"<< smallLine
    <<"\nselection color:\t"<<SelectColor
    <<"\nline width:\t"<<lineWidth;
  }
  return change;
}
/*  picking */
void View3D::PickCell(vtkObject* caller, unsigned long event, void* clientdata, void* callerdata)
{ /*  PickPoint allows fot the point id and coordinates to be returned 
  as well as adding a marker on the last picked point
  R_click to select point on line  */
  //printf("Entered pickCell\n");
  View3D* view = (View3D*)clientdata;       //acess to view3d class so view-> is used to call functions

  int *pos = view->Interactor->GetEventPosition();
  view->Interactor->GetPicker()->Pick(pos[0],pos[1],0.0,view->Interactor->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
  vtkCellPicker *cell_picker = (vtkCellPicker *)view->Interactor->GetPicker();
  if (cell_picker->GetCellId() == -1) 
  {
    view->SphereActor->VisibilityOff();     //not working quite yet but sphere will move
  }
  else if(cell_picker->GetViewProp()!=NULL) 
  {
    //double selPt[3];
    double pickPos[3];
    view->CellPicker->GetPickPosition(pickPos);    //this is the coordinates of the pick
    //view->cell_picker->GetSelectionPoint(selPt);    //not so usefull but kept
    unsigned int cell_id = cell_picker->GetCellId();  
    view->SelectedTraceIDs.push_back(cell_id);
    TraceLine *tline = reinterpret_cast<TraceLine*>(view->tobj->hashc[cell_id]);
	if (view->tobj->Gaps.size() < 1)
	{
		view->TreeModel->SelectByIDs(tline->GetId());
		//view->HighlightSelected(tline, view->SelectColor);
		//tline->Getstats();              //prints the id and end coordinates to the command prompt 
		view->SphereActor->SetPosition(pickPos);    //sets the selector to new point
		view->SphereActor->VisibilityOn();      //deleteTrace can turn it off 
		view->poly_line_data->Modified();
	}
	else
	{ //int size = view->tobj->Gaps.size();
		int id = tline->GetId();
		view->MergeGaps->SelectbyTraceID(id);
		view->poly_line_data->Modified();
	}
    //update the head Qt view here too...
  }// end if pick
  view->QVTK->GetRenderWindow()->Render();             //update the render window
}

void View3D::updateTraceSelectionHighlights()
{
	this->UpdateLineActor();
	std::vector<TraceLine*> Selections = this->TreeModel->GetSelectedTraces();
	for (unsigned int i = 0; i < Selections.size(); i++)
	{
		this->HighlightSelected(Selections[i],this->SelectColor);
	}
	this->poly_line_data->Modified();
	this->QVTK->GetRenderWindow()->Render();
	this->statusBar()->showMessage(tr("Selected\t")
		+ QString::number(Selections.size()) +tr("\ttraces"));
}
void View3D::HighlightSelected(TraceLine* tline, double color)
{
  TraceLine::TraceBitsType::iterator iter = tline->GetTraceBitIteratorBegin();
  TraceLine::TraceBitsType::iterator iterend = tline->GetTraceBitIteratorEnd();
  if (color == -1)
  {
    color = tline->getTraceColor();
  }
  while(iter!=iterend)
  {
    //poly_line_data->GetPointData()->GetScalars()->SetTuple1(iter->marker,1/t);
    poly_line_data->GetPointData()->GetScalars()->SetTuple1(iter->marker,color);
    ++iter;
  }
  

}

void View3D::Rerender()
{
  this->statusBar()->showMessage(tr("Rerender Image"));
  this->SphereActor->VisibilityOff();
  this->SelectedTraceIDs.clear();
  this->MergeGaps->GetSelectionModel()->clearSelection();
  this->TreeModel->GetSelectionModel()->clearSelection();
  this->Renderer->RemoveActor(this->BranchActor);
  //this->Renderer->RemoveActor(this->VolumeActor);
  this->UpdateLineActor();
  this->UpdateBranchActor();
  this->Renderer->AddActor(this->BranchActor); 
  //this->Renderer->AddActor(this->VolumeActor);  
  this->QVTK->GetRenderWindow()->Render();
  this->statusBar()->showMessage(tr("Finished Rerendering Image"));
}

void View3D::UpdateLineActor()
{
  this->poly_line_data = this->tobj->GetVTKPolyData();
  this->LineMapper->SetInput(this->poly_line_data);
  this->LineActor->SetMapper(this->LineMapper);
  this->LineActor->GetProperty()->SetColor(0,1,0);
  this->LineActor->GetProperty()->SetPointSize(2);
  this->LineActor->GetProperty()->SetLineWidth(lineWidth);
}

void View3D::UpdateBranchActor()
{
  this->poly = tobj->generateBranchIllustrator();
  this->polymap = vtkSmartPointer<vtkPolyDataMapper>::New();
  polymap->SetInput(this->poly);
  this->BranchActor = vtkSmartPointer<vtkActor>::New();
  this->BranchActor->SetMapper(this->polymap);
  this->BranchActor->SetPickable(0);
  //this->AddPointsAsPoints(this->tobj->CollectTraceBits());
  //Renderer->AddActor(BranchActor);
  //BranchActor->Print(std::cout);

}
void View3D::AddPointsAsPoints(std::vector<TraceBit> vec)
{
  vtkSmartPointer<vtkCubeSource> cube_src = vtkSmartPointer<vtkCubeSource>::New();
  cube_src->SetBounds(-0.2,0.2,-0.2,0.2,-0.2,0.2);
  vtkSmartPointer<vtkPolyData> point_poly = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points=vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> cells=vtkSmartPointer<vtkCellArray>::New();
  for(unsigned int counter=0; counter<vec.size(); counter++)
  {
    int return_id = points->InsertNextPoint(vec[counter].x,vec[counter].y,vec[counter].z);
    cells->InsertNextCell(1);
    cells->InsertCellPoint(return_id);
  }
  printf("About to create poly\n");
  point_poly->SetPoints(points);
  point_poly->SetVerts(cells);
  vtkSmartPointer<vtkGlyph3D> glyphs = vtkSmartPointer<vtkGlyph3D>::New();
  glyphs->SetSource(cube_src->GetOutput());
  glyphs->SetInput(point_poly);
  vtkSmartPointer<vtkPolyDataMapper> cubemap = vtkSmartPointer<vtkPolyDataMapper>::New();
  cubemap->SetInput(glyphs->GetOutput());
  cubemap->GlobalImmediateModeRenderingOn();
  vtkSmartPointer<vtkActor> cubeact = vtkSmartPointer<vtkActor>::New();
  cubeact->SetMapper(cubemap);
  cubeact->SetPickable(0);
  cubeact->GetProperty()->SetPointSize(5);
  cubeact->GetProperty()->SetOpacity(.5);
  Renderer->AddActor(cubeact);

}

void View3D::HandleKeyPress(vtkObject* caller, unsigned long event,
                            void* clientdata, void* callerdata)
{
  View3D* view = (View3D*)clientdata;
  char key = view->Interactor->GetKeyCode();
  switch (key)
    {
    case 'l':
      view->ListSelections();
      break;

    case 'c':
      view->ClearSelection();
      break;

    case 'd':
    //view->tobj->Gaps.clear();
      view->DeleteTraces();
      break;
    
    case 'm':
      view->MergeTraces();
      break;
    
    case 's':
      view->SplitTraces();
      break;

    case 'f':
      view->FlipTraces();
      break;

    case 'w':
      view->SaveToFile();
    break;

    case 't':
      view->ShowSettingsWindow();
      break;

    case 'a':
      view->SLine();
      /*std::cout<<"select small lines\n";
      view->tobj->FindMinLines(view->smallLine);
      view->Rerender();*/
      break;

    case 'q':
      view->updateSelectionHighlights();
      break;

    case '-':
      if(view->SelectedTraceIDs.size()>=1)
        {
        TraceLine* tline=reinterpret_cast<TraceLine*>(
          view->tobj->hashc[view->SelectedTraceIDs[view->SelectedTraceIDs.size()-1]]);
        view->HighlightSelected(tline, tline->getTraceColor()-.25);
        view->SelectedTraceIDs.pop_back();
        cout<< " These lines: ";
        for (unsigned int i = 0; i < view->SelectedTraceIDs.size(); i++)
          {
          cout<<  "\t"<<view->SelectedTraceIDs[i];   
          } 
        cout<< " \t are selected \n" ;   
        }
      break;

	case 'b':
		view->AddNewBranches();
		break;

    default:
      break;
    }
}
/*  Actions   */
void View3D::SLine()
{
  int numLines, i;
  this->tobj->FindMinLines(this->smallLine);
  numLines= this->tobj->SmallLines.size();
  for (i=0; i<numLines; i++)
  {
	  this->TreeModel->SelectByIDs(this->tobj->SmallLines[i]);
  }
  QMessageBox Myquestion;
  Myquestion.setText("Number of selected small lines:  " 
    + QString::number(numLines));
  Myquestion.setInformativeText("Delete these small lines?" );
  Myquestion.setStandardButtons(QMessageBox::Yes|QMessageBox::No);
  Myquestion.setDefaultButton(QMessageBox::Yes);
  int ret = Myquestion.exec();
  switch (ret) 
  { 
  case QMessageBox::Yes:
  {
	this->DeleteTraces();
    this->tobj->SmallLines.clear();
  }
  break;
  case QMessageBox::No:
   {
     this->tobj->SmallLines.clear();
	 this->TreeModel->GetSelectionModel()->clearSelection();
   }
   break;
  }
  this->Rerender();
}


void View3D::ListSelections()
{
  QMessageBox *selectionInfo = new QMessageBox;
  QString listText;
  QString selectedText;
  if (this->SelectedTraceIDs.size()<= 0)
    {
    listText = tr("No traces selected");
    }
  else
    {
    listText += QString::number(this->SelectedTraceIDs.size()) + " lines are selected\n";
    for (unsigned int i = 0; i < this->SelectedTraceIDs.size(); i++)
      {
      listText += QString::number(this->SelectedTraceIDs[i]) + "\n";   
      } 
    }
  this->statusBar()->showMessage(listText);
  //selectionInfo->setText(listText);
  //selectionInfo->setDetailedText(selectedText);
  //selectionInfo->show();
}
void View3D::ShowTreeData()
{
	if (this->TreeTable)
	{
		this->TreeTable->close();
	}
	if (this->TreePlot)
	{
		this->TreePlot->close();
	}
	this->TreeTable->setModel(this->TreeModel->GetModel());
	this->TreeTable->setSelectionModel(this->TreeModel->GetSelectionModel());
	this->TreeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->TreeTable->update();
	this->TreeTable->show();

	this->TreePlot = new PlotWindow(this->TreeModel->GetSelectionModel());
	this->connect(this->TreePlot,SIGNAL(destroyed()),this, SLOT(DereferenceTreePlotView())); 
	this->TreePlot->show();
}
void View3D::DereferenceTreePlotView()
{
	this->TreePlot = 0;
}

void View3D::ClearSelection()
{
  QString selectText;

  if (this->TreeModel->GetSelecectedIDs().size() <= 0)
    {
    selectText=tr("Nothing Selected");
    }
  else
    {  
	this->TreeModel->GetSelectionModel()->Clear;
    this->SelectedTraceIDs.clear();
    selectText=tr("Cleared trace list");
    this->Rerender();
    }
  if(this->tobj->Gaps.size()>0)
  {
	  this->tobj->Gaps.clear();
	  if(this->GapsPlotView)
	  {
		  this->GapsPlotView->close();
      }
	  if (this->GapsTableView)
      {
		  this->GapsTableView->close();
      }
	selectText+=tr("Cleared gaps list");
  }
  this->statusBar()->showMessage(selectText, 4000);
}

/*  delete traces functions */
void View3D::DeleteTraces()
{
	unsigned int i;
	this->statusBar()->showMessage(tr("Deleting"));
	std::vector<TraceLine*> traceStructure = this->tobj->GetTraceLines(); 
	std::vector<TraceLine*> traceList;
	std::vector<int> IDList = this->TreeModel->GetSelecectedIDs();
	for ( i = 0; i< IDList.size(); i++)
	{
		bool found = false; 
		unsigned int j = 0;
		while ((found == false)&&(j < traceStructure.size()))
		{
			if (traceStructure[j]->GetId()==IDList[i])
			{
				traceList.push_back(traceStructure[j]);
				found= true;
			}
			else
			{
				j++;
			}
		}	
	}

	if (traceList.size() >=1)
	{
		for (i = 0; i < traceList.size(); i++)
		{
			this->poly_line_data->Modified();
			this->DeleteTrace(traceList[i]); 
		}
		this->Rerender();
		this->statusBar()->showMessage(tr("Update Tree Plots"));
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		this->statusBar()->showMessage(tr("Deleted\t") + QString::number(traceList.size()) + tr("\ttraces"));
	}
	else
	{
		this->statusBar()->showMessage(tr("Nothing to Delete \n"));
	}
}

void View3D::DeleteTrace(TraceLine *tline)
{
  std::vector<unsigned int> * vtk_cell_ids = tline->GetMarkers();

  vtkIdType ncells; vtkIdType *pts;
  for(unsigned int counter=0; counter<vtk_cell_ids->size(); counter++)
    {
    this->poly_line_data->GetCellPoints((vtkIdType)(*vtk_cell_ids)[counter],ncells,pts);
    pts[1]=pts[0];
    }
  std::vector<TraceLine*> *children = tline->GetBranchPointer();
  if(children->size()!=0)
    {
    for(unsigned int counter=0; counter<children->size(); counter++)
      {
      this->poly_line_data->GetCellPoints((*(*children)[counter]->GetMarkers())[0],ncells,pts);
      pts[1]=pts[0];
      this->tobj->GetTraceLinesPointer()->push_back((*children)[counter]);  
      (*children)[counter]->SetParent(NULL);
      }
    // remove the children now
    children->clear();
    }         //finds and removes children
  // remove from parent
  std::vector<TraceLine*>* siblings;
  if(tline->GetParent()!=NULL)
    {
    siblings=tline->GetParent()->GetBranchPointer();
    if(siblings->size()==2)
      {
      // its not a branch point anymore
      TraceLine *tother1;
      if(tline==(*siblings)[0])
        { 
        tother1 = (*siblings)[1];
        }
      else
        {
        tother1 = (*siblings)[0];
        }
      tother1->SetParent(NULL);
      siblings->clear();
      TraceLine::TraceBitsType::iterator iter1,iter2;
      iter1= tline->GetParent()->GetTraceBitIteratorEnd();
      iter2 = tother1->GetTraceBitIteratorBegin();
      iter1--;
    
      this->tobj->mergeTraces((*iter1).marker,(*iter2).marker);
      tline->SetParent(NULL);
      delete tline;
      return;
      }
    }
  else
    {
    siblings = this->tobj->GetTraceLinesPointer();
    }
  std::vector<TraceLine*>::iterator iter = siblings->begin();
  std::vector<TraceLine*>::iterator iterend = siblings->end();
  while(iter != iterend)
    {
    if(*iter== tline)
      {
      siblings->erase(iter);
      break;
      }
    ++iter;
    }
  tline->SetParent(NULL);
}

void View3D::AddNewBranches()
{
	TraceLine* trunk;
	std::vector <TraceLine*> newChildren;
	std::vector <TraceLine*> selected = this->TreeModel->GetSelectedTraces();
	if (selected.size() > 1)
	{
		trunk = selected[0];
		if (trunk->Orient(selected[1]))
		{
			this->tobj->ReverseSegment(trunk);
		}
		for (unsigned int i = 1; i < selected.size(); i ++)
		{
			if (!selected[i]->Orient(trunk))
			{
				this->tobj->ReverseSegment(selected[i]);
			}
			newChildren.push_back(selected[i]);
		}
		this->AddChildren(trunk, newChildren);
		this->Rerender();
		this->statusBar()->showMessage(tr("Update Tree Plots"));
		this->TreeModel->SetTraces(this->tobj->GetTraceLines());
		this->statusBar()->showMessage(tr("Branching complete"));
	}
}
void View3D::AddChildren(TraceLine *trunk, std::vector<TraceLine*> childTraces)
{
	for (unsigned int i = 0; i < childTraces.size(); i++)
	{
		if (childTraces[i]->GetParentID() == -1)
		{
			childTraces[i]->SetParent(trunk);
			trunk->AddBranch(childTraces[i]);
		}//end check/make connection 
	}//end child trace size loop
}
/*  merging functions */
void View3D::MergeTraces()
{
  if (this->tobj->Gaps.size() > 1)
    {
    if(this->GapsPlotView)
      {
      this->GapsPlotView->close();
      }
    if (this->GapsTableView)
      {
      this->GapsTableView->close();
      }
    this->MergeSelectedTraces();
    //this->Rerender();
    this->tobj->Gaps.clear();
    }
  else
    {
    int conflict = 0;
	std::vector<TraceLine*> traceList = this->TreeModel->GetSelectedTraces();
	if (traceList.size() >=1)
	{
		conflict = this->tobj->createGapLists(traceList);
     }
    else
    {
		conflict = this->tobj->createGapLists(this->tobj->GetTraceLines());
    }       
	if(conflict == -1)
    {
        //user aborted
        this->tobj->Gaps.clear();
        return;
    }
    unsigned int i; 
    QMessageBox MergeInfo;
    if (this->tobj->Gaps.size() > 1)
    {
      for (i=0;i<this->tobj->Gaps.size(); i++)
      {
        this->tobj->Gaps[i]->compID = i;
        //this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .25);
        //this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .25);
      }
      MergeInfo.setText("\nNumber of computed distances:\t" 
        + QString::number(this->tobj->Gaps.size())
        +"\nConflicts resolved:\t" + QString::number(conflict)
        +"\nEdit selection or press merge again");
      this->ShowMergeStats();
    }//end if this->tobj->Gaps size > 1
    else 
      {
      if (this->tobj->Gaps.size() ==1)
        {   
        tobj->mergeTraces(this->tobj->Gaps[0]->endPT1,this->tobj->Gaps[0]->endPT2);
        this->Rerender();
        MergeInfo.setText(this->myText + "\nOne Trace merged");
        }
      else
        {
        this->Rerender();
        MergeInfo.setText("\nNo merges possible, set higher tolerances\n"); 
        } 
      }   
    MergeInfo.exec();
    this->myText.clear();
    this->poly_line_data->Modified();
    this->QVTK->GetRenderWindow()->Render();
    }//end else size
}

void View3D::ShowMergeStats()
{
  this->MergeGaps->SetTraceGaps(this->tobj->Gaps);
  this->GapsTableView->setModel(this->MergeGaps->GetModel());
  this->GapsTableView->setSelectionModel(this->MergeGaps->GetSelectionModel()); 
  this->GapsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
 // this->GapsTableView->sortByColumn(8,Ascending);
  this->GapsTableView->update();
  this->GapsTableView->show();
  this->GapsTableView->horizontalHeader()->show();
  this->GapsPlotView = new PlotWindow(this->MergeGaps->GetSelectionModel());
  this->connect(this->GapsPlotView,SIGNAL(destroyed()),this, SLOT(DereferenceGapsPlotView()));
  this->GapsPlotView->show();
}

void View3D::DereferenceGapsPlotView()
{
  this->GapsPlotView = 0;
}

void View3D::updateSelectionHighlights()
{
  bool selected = false;
  int curID;
  std::vector<int> GapIDs = this->MergeGaps->GetSelectedGapIDs();
  for (unsigned int i = 0; i < this->tobj->Gaps.size(); i++)
  {
    curID = this->tobj->Gaps[i]->compID;
    selected = false;
    unsigned int j =0;
    while(!selected && j < GapIDs.size())
    {
      if ( curID == GapIDs[j])
      {
        selected = true;
      }
      else
      {
        j++;
      }
    }//end gapids size j
    if (selected == true)
    {
      this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .05);
      this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .05);
    }//end true
    else
    {
      this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .3);
      this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .3);
    }//end else true
  }//end for i
  this->poly_line_data->Modified();
  this->QVTK->GetRenderWindow()->Render();
  this->statusBar()->showMessage(tr("Done"));
}
void View3D::MergeSelectedTraces()
{
	this->statusBar()->showMessage(tr("Merging"));
  this->updateSelectionHighlights(); //if it didnt
  std::vector<int> GapIDs = this->MergeGaps->GetSelectedGapIDs();
  bool selected = false;
  int curID;
  QMessageBox MergeInfo;
  double currentAngle=0, aveCost=0;
  QPushButton *mergeAll;
  QPushButton *mergeNone;
  unsigned int i=0, j=0,mergeCount=0;
  MergeInfo.setText("Merge Function");
  if (GapIDs.size() > 1)
  {
    for (i = 0; i < this->tobj->Gaps.size(); i++)
    {
      curID = this->tobj->Gaps[i]->compID;
      selected = false; 
      j =0;
      while(!selected && j < GapIDs.size())
      {
        if ( curID == GapIDs[j])
        {
          selected = true;
        }
        else
        {
          j++;
        }
      }//end gapids size j
      if (selected == true)
      {
		  aveCost += this->tobj->Gaps[i]->cost;
        this->tobj->mergeTraces(this->tobj->Gaps[i]->endPT1,this->tobj->Gaps[i]->endPT2);
      }
    } 
    MergeInfo.setText("merged " + QString::number(GapIDs.size()) + " traces.");  
	this->dtext+="\nAverage Cost of Merge:\t" 
		+ QString::number( aveCost / GapIDs.size() );
	this->dtext+= "\nSum of Merge costs " + QString::number(aveCost);
	MergeInfo.setDetailedText(this->dtext);
    MergeInfo.exec();
  }
  else
  {
    for (i=0;i<this->tobj->Gaps.size(); i++)
    {
      //currentAngle=fabs(this->tobj->Gaps[i]->angle); 
	  if  (this->tobj->Gaps[i]->cost <=5)
      {
        this->dtext+= "\nTrace " + QString::number(this->tobj->Gaps[i]->Trace1->GetId());
        this->dtext+= " and "+ QString::number(this->tobj->Gaps[i]->Trace2->GetId() );
		this->dtext+="\tcost of:" + QString::number(this->tobj->Gaps[i]->cost); 
        this->HighlightSelected(this->tobj->Gaps[i]->Trace1, .125);
        this->HighlightSelected(this->tobj->Gaps[i]->Trace2, .125);
        this->candidateGaps.push_back( this->tobj->Gaps[i]);
	  } //end of cost<5
    }//end of for merge    
  if (this->candidateGaps.size()>=1)
  {      
      myText+="\nNumber of possible lines:\t" + QString::number(this->candidateGaps.size());
      mergeAll = MergeInfo.addButton("Merge All", QMessageBox::YesRole);
      mergeNone = MergeInfo.addButton("Merge None", QMessageBox::NoRole);
	  MergeInfo.setText(myText); 
	  MergeInfo.setDetailedText(this->dtext);
	  this->poly_line_data->Modified();
	  this->QVTK->GetRenderWindow()->Render();
	  MergeInfo.exec();    
	  if(MergeInfo.clickedButton()==mergeAll)
	  {
		  for (j=0; j<this->candidateGaps.size();j++)
		  {
			tobj->mergeTraces(this->candidateGaps[j]->endPT1,this->candidateGaps[j]->endPT2);
		  }
	  }
	  else if(MergeInfo.clickedButton()==mergeNone)
	  {
		  this->candidateGaps.clear();
		  this->statusBar()->showMessage("Merge Canceled");
	  }
    }   //end if graylist size
    else
    {
		this->statusBar()->showMessage("nothing to merge");
    }   //end else   
  }
  this->Rerender();
  this->tobj->Gaps.clear();
  this->candidateGaps.clear();
  this->myText.clear();
  this->dtext.clear();
  this->statusBar()->showMessage(tr("Update Tree Plots"));
  this->TreeModel->SetTraces(this->tobj->GetTraceLines());
  this->statusBar()->showMessage(tr("Done With Merge"));
}
/*  other trace modifiers */
void View3D::SplitTraces()
{
  if(this->SelectedTraceIDs.size()>=1)
    {
    std::unique(this->SelectedTraceIDs.begin(), this->SelectedTraceIDs.end());
    for(unsigned int i = 0; i < this->SelectedTraceIDs.size(); i++)
	{
	  this->tobj->splitTrace(this->SelectedTraceIDs[i]);
	} 
    this->Rerender();
	this->statusBar()->showMessage(tr("Update Tree Plots"));
	this->TreeModel->SetTraces(this->tobj->GetTraceLines());
    }
  else
    {
    this->statusBar()->showMessage(tr("Nothing to split"), 1000); 
    }
}

void View3D::FlipTraces()
{
  if(this->SelectedTraceIDs.size()>=1)
    {
    for(unsigned int i=0; i< this->SelectedTraceIDs.size(); i++)
      {
      this->tobj->ReverseSegment(reinterpret_cast<TraceLine*>(
        this->tobj->hashc[this->SelectedTraceIDs[i]]));
      }
    this->Rerender();
    }
}

void View3D::SaveToFile()
{
  //display a save file dialog
  QString fileName = QFileDialog::getSaveFileName(
    this,
    tr("Save File"),
    "",
    tr("SWC Images (*.swc);;VTK files (*.vtk)"));

  //if the user pressed cancel, bail out now.
  if(fileName.isNull())
    {
    return;
    }

  //make sure the user supplied an appropriate output file format
  if(!fileName.endsWith(".vtk") && !fileName.endsWith(".swc"))
    {
    QMessageBox::critical(this, tr("Unsupported file format"),
      tr("Trace editor only supports output to .swc and .vtk file formats"));
    this->SaveToFile();
    return;
    }

  if(fileName.endsWith(".swc"))
    {
    this->tobj->WriteToSWCFile(fileName.toStdString().c_str()); 
    }
  else if(fileName.endsWith(".vtk"))
    {
    this->tobj->WriteToVTKFile(fileName.toStdString().c_str()); 
    }
}


/*  Soma display stuff  */
void View3D::GetSomaFile()
{
	QString somaFile = QFileDialog::getOpenFileName(this , "Choose a Soma file to load", ".", 
	  tr("Image File (*.tiff *.tif *.pic *.PIC)"));
	if(!somaFile.isNull())
	{
		this->statusBar()->showMessage("Loading Soma Image");
		this->readImg(somaFile.toStdString());
		if(this->VolumeActor!=NULL)
		{
			this->Renderer->AddVolume(this->VolumeActor);
			this->QVTK->GetRenderWindow()->Render();
			this->statusBar()->showMessage("Somas Rendered");
		}
	}
}

void View3D::AddContourThresholdSliders()
{
  vtkSliderRepresentation2D *sliderRep2 =
    vtkSmartPointer<vtkSliderRepresentation2D>::New();
  sliderRep2->SetValue(0.8);
  sliderRep2->SetTitleText("Threshold");
  sliderRep2->GetPoint1Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep2->GetPoint1Coordinate()->SetValue(0.2,0.8);
  sliderRep2->GetPoint2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  sliderRep2->GetPoint2Coordinate()->SetValue(0.8,0.8);
  sliderRep2->SetSliderLength(0.02);
  sliderRep2->SetSliderWidth(0.03);
  sliderRep2->SetEndCapLength(0.01);
  sliderRep2->SetEndCapWidth(0.03);
  sliderRep2->SetTubeWidth(0.005);
  sliderRep2->SetMinimumValue(0.0);
  sliderRep2->SetMaximumValue(1.0);

  vtkSliderWidget *sliderWidget2 = vtkSmartPointer<vtkSliderWidget>::New();
  sliderWidget2->SetInteractor(Interactor);
  sliderWidget2->SetRepresentation(sliderRep2);
  sliderWidget2->SetAnimationModeToAnimate();

  vtkSlider2DCallbackContourThreshold *callback_contour =
    vtkSmartPointer<vtkSlider2DCallbackContourThreshold>::New();
  callback_contour->cfilter = this->ContourFilter;
  sliderWidget2->AddObserver(vtkCommand::InteractionEvent,callback_contour);
  sliderWidget2->EnabledOn();

}

void View3D::AddPlaybackWidget(char *filename)
{

  vtkSubRep *playbackrep = vtkSmartPointer<vtkSubRep>::New();
  playbackrep->slice_counter=0;
  playbackrep->GetPositionCoordinate()->SetCoordinateSystemToNormalizedDisplay();
  playbackrep->GetPositionCoordinate()->SetValue(0.2,0.1);
  playbackrep->GetPosition2Coordinate()->SetCoordinateSystemToNormalizedDisplay();
  playbackrep->GetPosition2Coordinate()->SetValue(0.8,0.1);
  

  vtkSmartPointer<vtkPlaybackWidget> pbwidget = vtkSmartPointer<vtkPlaybackWidget>::New();
  pbwidget->SetRepresentation(playbackrep);
  pbwidget->SetInteractor(Interactor);

  const unsigned int Dimension = 3;
  typedef unsigned char  PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  ReaderType::Pointer imreader = ReaderType::New();

  imreader->SetFileName(filename);
  imreader->Update();

  
  ImageType::SizeType size = imreader->GetOutput()->GetLargestPossibleRegion().GetSize();

  std::vector<vtkSmartPointer<vtkImageData> > &vtkimarray = playbackrep->im_pointer;
  vtkimarray.reserve(size[2]-1);
  printf("About to create vtkimarray contents in a loop\n");
  for(unsigned int counter=0; counter<size[2]-1; counter++)
  {
    vtkimarray.push_back(vtkSmartPointer<vtkImageData>::New());
    vtkimarray[counter]->SetScalarTypeToUnsignedChar();
    vtkimarray[counter]->SetDimensions(size[0],size[1],2);
    vtkimarray[counter]->SetNumberOfScalarComponents(1);
    vtkimarray[counter]->AllocateScalars();
    vtkimarray[counter]->SetSpacing(1/2.776,1/2.776,1);
    memcpy(vtkimarray[counter]->GetScalarPointer(),imreader->GetOutput()->GetBufferPointer()+counter*size[0]*size[1]*sizeof(unsigned char),size[0]*size[1]*2*sizeof(unsigned char));
  }

  printf("finished memcopy in playback widget\n");
  vtkPiecewiseFunction *opacityTransferFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  opacityTransferFunction->AddPoint(2,0.0);
  opacityTransferFunction->AddPoint(20,0.2);

  vtkColorTransferFunction *colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(20.0,1,0,0);

  
  vtkVolumeProperty *volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor(colorTransferFunction);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->SetInterpolationTypeToLinear();

  vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D> volumeMapper =
    vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D>::New();
  volumeMapper->SetSampleDistance(0.5);
  volumeMapper->SetInput(vtkimarray[playbackrep->slice_counter]);
  
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);
  volume->SetPickable(0);
  Renderer->AddVolume(volume);
  printf("Added volume in playback widget");
  this->Volume = volume;
  playbackrep->vmapper=volumeMapper;
  /*playbackrep->im_pointer = &vtkimarray;*/
  playbackrep->Print(std::cout);
  
  this->QVTK->GetRenderWindow()->Render();
}
void View3D::AddVolumeSliders()
{
  vtkSliderRepresentation2D *opacitySliderRep =
    vtkSliderRepresentation2D::New();
  opacitySliderRep->SetValue(0.1);
  opacitySliderRep->SetTitleText("Opacity");
  opacitySliderRep->GetPoint1Coordinate()->
    SetCoordinateSystemToNormalizedDisplay();
  opacitySliderRep->GetPoint1Coordinate()->SetValue(0.2,0.1);
  opacitySliderRep->GetPoint2Coordinate()->
    SetCoordinateSystemToNormalizedDisplay();
  opacitySliderRep->GetPoint2Coordinate()->SetValue(0.8,0.1);
  opacitySliderRep->SetSliderLength(0.02);
  opacitySliderRep->SetSliderWidth(0.03);
  opacitySliderRep->SetEndCapLength(0.01);
  opacitySliderRep->SetEndCapWidth(0.03);
  opacitySliderRep->SetTubeWidth(0.005);
  opacitySliderRep->SetMinimumValue(0.0);
  opacitySliderRep->SetMaximumValue(1.0);

  this->OpacitySlider = vtkSliderWidget::New();
  this->OpacitySlider->SetInteractor(this->Interactor);
  this->OpacitySlider->SetRepresentation(opacitySliderRep);
  this->OpacitySlider->SetAnimationModeToAnimate();

  vtkSlider2DCallbackOpacity *callback_opacity =
    vtkSlider2DCallbackOpacity::New();
  callback_opacity->volume = this->Volume;
  this->OpacitySlider->AddObserver(vtkCommand::InteractionEvent,callback_opacity);
  this->OpacitySlider->EnabledOn();
  opacitySliderRep->Delete();
  callback_opacity->Delete();
  

// slider 2

  vtkSliderRepresentation2D *brightnessSliderRep =
    vtkSliderRepresentation2D::New();
  brightnessSliderRep->SetValue(0.8);
  brightnessSliderRep->SetTitleText("Brightness");
  brightnessSliderRep->GetPoint1Coordinate()->
    SetCoordinateSystemToNormalizedDisplay();
  brightnessSliderRep->GetPoint1Coordinate()->SetValue(0.2,0.9);
  brightnessSliderRep->GetPoint2Coordinate()->
    SetCoordinateSystemToNormalizedDisplay();
  brightnessSliderRep->GetPoint2Coordinate()->SetValue(0.8,0.9);
  brightnessSliderRep->SetSliderLength(0.02);
  brightnessSliderRep->SetSliderWidth(0.03);
  brightnessSliderRep->SetEndCapLength(0.01);
  brightnessSliderRep->SetEndCapWidth(0.03);
  brightnessSliderRep->SetTubeWidth(0.005);
  brightnessSliderRep->SetMinimumValue(0.0);
  brightnessSliderRep->SetMaximumValue(1.0);

  this->BrightnessSlider = vtkSliderWidget::New();
  this->BrightnessSlider->SetInteractor(this->Interactor);
  this->BrightnessSlider->SetRepresentation(brightnessSliderRep);
  this->BrightnessSlider->SetAnimationModeToAnimate();

  vtkSlider2DCallbackBrightness *callback_brightness =
    vtkSlider2DCallbackBrightness::New();
  callback_brightness->volume = this->Volume;
  this->BrightnessSlider->AddObserver(vtkCommand::InteractionEvent,callback_brightness);
  this->BrightnessSlider->EnabledOn();
  brightnessSliderRep->Delete();
  callback_brightness->Delete();
}
 
void View3D::UndoAction()
{
	
	if(!(this->undoBuff->UndoOrRedo(0)))
	{
		return;
	}
	else
	{
		std::pair<std::string, TraceObject> undostate = this->undoBuff->getState();
		TraceObject newstate = undostate.second;
		*(this->tobj) = newstate;
		Rerender();
	}
	
}


void View3D::RedoAction()
{
	/*
	if(!(this->undoBuff->UndoOrRedo(1)))
	{
		return;
	}
	else
	{
		std::pair<std::string, TraceObject> undostate = this->undoBuff->getState();
		TraceObject newstate = undostate.second;
		*(this->tobj) = newstate;
		Rerender();
	}
	*/
}

void View3D::readImg(std::string sourceFile)
{
  
  //Set up the itk image reader
  const unsigned int Dimension = 3;
  typedef unsigned char  PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( sourceFile );

  //Test opening and reading the input file
  try
  {
    reader->Update();
    }
  catch( itk::ExceptionObject & exp )
    {
    std::cerr << "Exception thrown while reading the input file " << std::endl;
    std::cerr << exp << std::endl;
    //return EXIT_FAILURE;
    }
  std::cout << "Image Read " << std::endl;
  //Itk image to vtkImageData connector
  typedef itk::ImageToVTKImageFilter<ImageType> ConnectorType;
  ConnectorType::Pointer connector= ConnectorType::New();
  connector->SetInput( reader->GetOutput() );

  //Route vtkImageData to the contour filter
  this->ContourFilter = vtkSmartPointer<vtkContourFilter>::New();
  this->ContourFilter->SetInput(connector->GetOutput());
  this->ContourFilter->SetValue(0,10);            // this affects render

  this->ContourFilter->Update();

  //Route contour filter output to the mapper
  this->VolumeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->VolumeMapper->SetInput(this->ContourFilter->GetOutput());

  //Declare actor and set properties
  this->VolumeActor = vtkSmartPointer<vtkActor>::New();
  this->VolumeActor->SetMapper(this->VolumeMapper);

  //this->VolumeActor->GetProperty()->SetRepresentationToWireframe();
  this->VolumeActor->GetProperty()->SetOpacity(.5);
  this->VolumeActor->GetProperty()->SetColor(0.5,0.5,0.5);
  this->VolumeActor->SetPickable(0);

  this->statusBar()->showMessage("Created Contour Image");
}


void View3D::rayCast(char *raySource)
{
  const unsigned int Dimension = 3;
  typedef unsigned char  PixelType;
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  ReaderType::Pointer i2spReader = ReaderType::New();
  i2spReader->SetFileName( raySource );
  try
  {
    i2spReader->Update();
    }
  catch( itk::ExceptionObject & exp )
    {
    std::cerr << "Exception thrown while reading the input file " << std::endl;
    std::cerr << exp << std::endl;
    //return EXIT_FAILURE;
    }
  std::cout << "Image Read " << std::endl;
//itk-vtk connector
  typedef itk::ImageToVTKImageFilter<ImageType> ConnectorType;
  ConnectorType::Pointer connector= ConnectorType::New();
  connector->SetInput( i2spReader->GetOutput() );
  vtkSmartPointer<vtkImageToStructuredPoints> i2sp =
    vtkSmartPointer<vtkImageToStructuredPoints>::New();
  i2sp->SetInput(connector->GetOutput());
  
  ImageType::SizeType size = i2spReader->GetOutput()->GetLargestPossibleRegion().GetSize();
  vtkSmartPointer<vtkImageData> vtkim = vtkSmartPointer<vtkImageData>::New();
  vtkim->SetScalarTypeToUnsignedChar();
  vtkim->SetDimensions(size[0],size[1],size[2]);
  vtkim->SetNumberOfScalarComponents(1);
  vtkim->AllocateScalars();

  memcpy(vtkim->GetScalarPointer(),i2spReader->GetOutput()->GetBufferPointer(),size[0]*size[1]*size[2]*sizeof(unsigned char));

// Create transfer mapping scalar value to opacity
  vtkSmartPointer<vtkPiecewiseFunction> opacityTransferFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();

  opacityTransferFunction->AddPoint(2,0.0);
  opacityTransferFunction->AddPoint(50,0.1);
 // opacityTransferFunction->AddPoint(40,0.1);
  // Create transfer mapping scalar value to color
  // Play around with the values in the following lines to better vizualize data
  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
    colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(50.0,1,0,0);

  // The property describes how the data will look
  vtkSmartPointer<vtkVolumeProperty> volumeProperty =
    vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
  //  volumeProperty->ShadeOn();
    volumeProperty->SetInterpolationTypeToLinear();

  vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D> volumeMapper =
    vtkSmartPointer<vtkOpenGLVolumeTextureMapper3D>::New();
  volumeMapper->SetSampleDistance(0.75);
  volumeMapper->SetInput(vtkim);

  // The volume holds the mapper and the property and
  // can be used to position/orient the volume
  vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);
  volume->SetPickable(0);
//  Renderer->AddVolume(volume);
  this->Volume = volume;
 // this->QVTK->GetRenderWindow()->Render();
  std::cout << "RayCast generated \n";
}

void View3D::closeEvent(QCloseEvent *event)
{
  if(this->GapsPlotView)
    {
    this->GapsPlotView->close();
    }
  if(this->histo)
    {
    this->histo->close();
    }
  if(this->GapsTableView)
    {
    this->GapsTableView->close();
    }
  if(this->TreeTable)
    {
    this->TreeTable->close();
    }
  if(this->TreePlot)
  {
	  this->TreePlot->close();
  }
  event->accept();
}

