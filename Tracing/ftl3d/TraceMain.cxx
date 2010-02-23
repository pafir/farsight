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

/**
 \brief Main function for tracing in 3D volume. The input image can be
 \author $ Author: Amit Mukherjee$
 \version $Revision 1.0 $
 \date May 05 2009
*/
// Copyright Farsight Toolkit 2009, Rensselaer Polytechnic institute Troy NY 12180.


#include "TraceMain.h"


TraceSEMain::TraceSEMain()
{
	//create main window gui and load files
	QWidget * mainWidget = new QWidget();
	this->setCentralWidget(	mainWidget);
	QHBoxLayout *mainLayout = new QHBoxLayout;
	//vetical box layout for the main central widget
	mainWidget->setLayout(mainLayout);
	//file actions
	this->createFileActions();
	mainLayout->addWidget(this->FileActions);
	//settings
	this->CreateSettingsLayout();
	mainLayout->addWidget(this->settingsBox);
}
void TraceSEMain::createFileActions()
{
	this->FileActions = new QGroupBox("Add a File to Trace");
	QGridLayout *fileActionLayout = new QGridLayout;

	this->InputFileNameLine = new QLabel();
	QPushButton * InFileButton = new QPushButton("Input file",this);
	connect(InFileButton, SIGNAL(clicked()), this, SLOT(GetInputFileName()));
	fileActionLayout->addWidget(this->InputFileNameLine, 0,0);
	fileActionLayout->addWidget(InFileButton, 0,1);

	this->OutputFileNameLine = new QLineEdit();
	QPushButton * OutFileButton = new QPushButton("Output file",this);
	connect(OutFileButton, SIGNAL(clicked()), this, SLOT(GetOutputFileName()));
	fileActionLayout->addWidget(this->OutputFileNameLine, 1,0);
	fileActionLayout->addWidget(OutFileButton, 1,1);
	
	QPushButton * AddButton = new QPushButton("Add new Files",this);
	connect(AddButton, SIGNAL(clicked()), this, SLOT(addFileToTrace()));
	fileActionLayout->addWidget(AddButton, 2,1);

	this->FileListWindow = new QTextEdit(this);
	this->FileListWindow->setReadOnly(true);
	this->FileListWindow->setLineWrapMode(QTextEdit::NoWrap);
	fileActionLayout->addWidget(this->FileListWindow, 3,0);

	//run
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
		| QDialogButtonBox::Close);
     connect(buttonBox, SIGNAL(accepted()), this, SLOT(runSETracing()));
     connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));
	 fileActionLayout->addWidget(buttonBox,4,0);
	this->FileActions->setLayout(fileActionLayout);
}
void TraceSEMain::GetInputFileName()
{
	this->InputFileNameLine->clear();
	QString NewImageFile = QFileDialog::getOpenFileName(this , "Load Image Data to Trace", ".",
		tr("Trace Image ( *.tiff *.tif )"));
	if (!NewImageFile.isEmpty())
	{
		this->newInput = NewImageFile;
		this->InputFileNameLine->setText(NewImageFile);
		QString temp = NewImageFile.section('.',0,-1);
		this->OutputFileNameLine->setText(temp.append("_SE.xml"));
		this->statusBar()->showMessage("filename:\t" + NewImageFile);
	}
}
void TraceSEMain::GetOutputFileName()
{
	QString NewOutputFile = QFileDialog::getOpenFileName(this , "Change output Trace filename", ".",
		tr("Trace file ( *.xml )"));
	if (!NewOutputFile.isEmpty())
	{
		this->OutputFileNameLine->setText(NewOutputFile);
	}
}
void TraceSEMain::addFileToTrace()
{
	this->newOutput = this->OutputFileNameLine->text();
	if(!(this->newInput.isEmpty()||this->newOutput.isEmpty()))
	{
		this->InputFileNames.push_back(this->newInput.toStdString());
		this->OutputFileNames.push_back(this->newOutput.toStdString());
		this->FileListWindow->append(this->newInput.section('/',-1) 
			+ "\t" + this->newOutput.section('/',-1));
		this->statusBar()->showMessage(QString::number(this->InputFileNames.size()) + " files to trace");
		this->InputFileNameLine->clear(); //shows nothing 
		this->OutputFileNameLine->clear();
	}
}
void TraceSEMain::CreateSettingsLayout()
{
	//create settings layout for tracing parameters 
	//to disable a user input comment out the add row
	this->settingsBox = new QGroupBox("Trace Settings");
	QFormLayout *settingsLayout = new QFormLayout;

	this->GetGridSpacing = new QSpinBox(this);
	this->GetGridSpacing->setRange(5,70);
	this->GetGridSpacing->setValue(15);
	settingsLayout->addRow("Grid Spacing", this->GetGridSpacing);

	this->GetStepRatio = new QDoubleSpinBox(this);
	this->GetStepRatio->setRange(0.1,1);
	this->GetStepRatio->setSingleStep(0.1);
	this->GetStepRatio->setValue(.5);
	settingsLayout->addRow("Step Ratio", this->GetStepRatio);

	this->GetAspectRatio= new QDoubleSpinBox(this);
	this->GetAspectRatio->setRange(1.5,3.0);
	this->GetAspectRatio->setSingleStep(.1);
	this->GetAspectRatio->setValue(2.5);
	settingsLayout->addRow("Max Model Aspect Ratio", this->GetAspectRatio);

	this->GetTHRESHOLD = new QDoubleSpinBox(this);
	this->GetTHRESHOLD->setRange(0,1);
	this->GetTHRESHOLD->setSingleStep(.1);
	this->GetTHRESHOLD->setValue(.5);
	settingsLayout->addRow("Threshold",this->GetTHRESHOLD);

	this->GetminContrast = new QDoubleSpinBox(this);
	this->GetminContrast->setRange(1,255);
	this->GetminContrast->setValue(3);
	settingsLayout->addRow("Min Contrast", this->GetminContrast);

	this->GetMaximumVesselWidth = new QDoubleSpinBox(this);
	this->GetMaximumVesselWidth->setRange(10,50);
	this->GetMaximumVesselWidth->setValue(20);
	settingsLayout->addRow("Maximum Vessel Width",this->GetMaximumVesselWidth);

	this->GetMinimumVesselLength = new QDoubleSpinBox(this);
	this->GetMinimumVesselLength->setRange(2,20);
	this->GetMinimumVesselLength->setValue(3);
	settingsLayout->addRow("Minimum Vessel Length", this->GetMinimumVesselLength);

	this->GetMinimumVesselWidth = new QDoubleSpinBox(this);
	this->GetMinimumVesselWidth->setRange(.5,9);
	this->GetMinimumVesselWidth->setValue(1);
	settingsLayout->addRow("Minimum Vessel Width", this->GetMinimumVesselWidth);

	this->GetFitIterations = new QSpinBox(this);
	this->GetFitIterations->setRange(50,150);
	this->GetFitIterations->setValue(50);
	settingsLayout->addRow("Fitting Iterations", this->GetFitIterations);

	this->GetStartTHRESHOLD = new QDoubleSpinBox(this);
	this->GetStartTHRESHOLD->setRange(0,1);
	this->GetStartTHRESHOLD->setSingleStep(.1);
	this->GetStartTHRESHOLD->setValue(.7);
	settingsLayout->addRow("Starting Threshold", this->GetStartTHRESHOLD);

	this->GetUseHessian = new QCheckBox(this);
	this->GetUseHessian->setText("Use Multi-scale Hessian filtering");
	settingsLayout->addRow(this->GetUseHessian);

	this->settingsBox->setLayout(settingsLayout);
}

bool TraceSEMain::runSETracing()
{
	this->addFileToTrace();
	this->numDataFiles = this->InputFileNames.size();
	if (this->numDataFiles <1)
	{
		QMessageBox msgBox;
		msgBox.setText("There are no files to trace");
		msgBox.exec();
		return false;
	}
	this->statusBar()->showMessage("Setting Parameters");
	this->GridSpacing = this->GetGridSpacing->value();
	this->StepRatio = this->GetStepRatio->value();
	this->AspectRatio = this->GetAspectRatio->value();
	this->THRESHOLD = this->GetTHRESHOLD->value();
	this->minContrast = this->GetminContrast->value();
	this->MaximumVesselWidth = this->GetMaximumVesselWidth->value();
	this->MinimumVesselLength = this->GetMinimumVesselLength->value();
	this->MinimumVesselWidth = this->GetMinimumVesselWidth->value();
	this->StartTHRESHOLD = this->GetStartTHRESHOLD->value();
	this->Spacing[0] = 1;
	this->Spacing[1] = 1;
	this->Spacing[2] = 1;
	if (this->GetUseHessian->isChecked())
	{
		this->UseHessian = 1;
	}
	else
	{
		this->UseHessian = 0;
	}
	for (unsigned int k = 0; k < this->numDataFiles; k++) 
	{
	this->statusBar()->showMessage("Tracing file " + QString::number((int)k) 
		+ " of " + QString::number(this->numDataFiles));
		/*std::cout << "Tracing " << k+1 <<" out of " << m_Config->getNumberOfDataFiles()
			<< " image: " << m_Config->getInputFileName(k) << std::endl<< std::endl;*/

		ImageType3D::Pointer image3D = ImageType3D::New();
		ImageType2D::Pointer MIPimage = ImageType2D::New();
		typedef itk::ImageFileReader<ImageType3D> ReaderType;
		ReaderType::GlobalWarningDisplayOff();
		ReaderType::Pointer reader = ReaderType::New();

		reader->SetFileName(  this->InputFileNames.at(k));
		image3D = (reader->GetOutput());
		image3D->Update();
		this->statusBar()->showMessage("Image read");
		//this->statusBar()->showMessage("Image of size " + QString::number(image3D->GetBufferedRegion().GetSize() )+ " read successfully " );

		ImageDenoise(image3D, this->UseHessian);
		ImageStatistics(image3D);	//This inverts the intensities (in most cases)
		this->statusBar()->showMessage("Image Statistics finished");
		/*try	{*/
			SeedContainer3D::Pointer m_Seeds = SeedContainer3D::New();
			m_Seeds->SetGridSpacing((long) this->GridSpacing );
			m_Seeds->Detect(image3D, MIPimage);
		this->statusBar()->showMessage("Seeds ");
			//At input: image3D is inverted input image, MIPimage is blank.
													//At output: image3D is untouched, MIP image is minimum projection
													//				AND seeds have been detected
			//WriteSeedImage(MIPimage, m_Seeds , m_Config->getOutputFileName(k));

			Seed2Seg::Pointer m_SS = Seed2Seg::New();
			m_SS->Configure(this->FitIterations, this->AspectRatio, this->MinimumVesselWidth,
				this->StartTHRESHOLD, this->minContrast);
		this->statusBar()->showMessage("Comupute Start Segments ");
			m_SS->ComuputeStartSegments(m_Seeds , image3D);
		this->statusBar()->showMessage("sort Start Segments ");
			m_SS->SortStartSegments();

			TraceContainer3D::Pointer m_Tracer = TraceContainer3D::New();
			m_Tracer->Configure(this->THRESHOLD, this->minContrast, this->StepRatio,
								 this->AspectRatio, this->Spacing);
		this->statusBar()->showMessage("Compute Trace");
			m_Tracer->ComputeTrace(image3D, m_SS) ;
			//m_Tracer->WriteTraceToTxtFile(m_Config->getOutputFileName(k));
		this->statusBar()->showMessage("write to file");
			m_Tracer->WriteTraceToXMLFile(this->OutputFileNames.at(k));

			m_Seeds = NULL;
			m_SS = NULL;
			m_Tracer = NULL;
			image3D = NULL;
			MIPimage = NULL;
		//}
		//catch (itk::ExceptionObject & err )	    {
		//	std::cout << "ExceptionObject caught !" << std::endl;
		//	std::cout << err << std::endl;
		//	return EXIT_FAILURE;
		//}
	}
	this->statusBar()->showMessage("done");
	return true;
}
//void ImageDenoise(ImageType3D::Pointer& vol)	{
void TraceSEMain::ImageDenoise(ImageType3D::Pointer& im3D, int hessianFlag)	{
	// use median filtering based denoising to get rid of impulsive noise
	std::cout << "Denoising Image..." << std::endl;
	typedef itk::MedianImageFilter<ImageType3D, ImageType3D> MedianFilterType;
	MedianFilterType::Pointer medfilt = MedianFilterType::New();
	medfilt->SetInput(im3D);
	ImageType3D::SizeType rad = { {1, 1, 1} };
	medfilt->SetRadius(rad);

	ImageType3D::Pointer vol = medfilt->GetOutput();
	vol->Update();

	if (hessianFlag == 1)	{
 		ImageType3D::PixelType sigmas[] = { 2.0f, 2.88f, 4.0f, 5.68f, 8.0f };
		ImageType3D::Pointer temp = ImageType3D::New();
		ImageType3D::Pointer maxF = ImageType3D::New();

		temp->SetRegions(vol->GetLargestPossibleRegion());
		temp->Allocate();
		temp->FillBuffer(0.0);
		maxF->SetRegions(vol->GetLargestPossibleRegion());
		maxF->Allocate();
		maxF->FillBuffer(0.0);


		typedef itk::SmoothingRecursiveGaussianImageFilter< ImageType3D , ImageType3D> GFilterType;
		GFilterType::Pointer gauss = GFilterType::New();

		for (unsigned int i = 0; i < 4; ++i)	{
			std::cout << "Performing 3D Line Filtering using Hessian at " << sigmas[i] ;
			gauss->SetInput( vol );
			gauss->SetSigma( sigmas[i] );
			gauss->SetNormalizeAcrossScale(true);
			ImageType3D::Pointer svol = gauss->GetOutput();
			svol->Update();

			std::cout << "...";
			//GetFeature( vol, temp, sigmas[i] );
			GetFeature( temp, svol );
			std::cout << "....done" << std::endl;
			UpdateMultiscale( temp, maxF);
		}
		im3D = maxF;
	}
	else {
		im3D = vol;
	}
	/*
	typedef itk::Image<unsigned char, 3> CharImageType3D;
	typedef itk::CastImageFilter< ImageType3D,CharImageType3D > CastType; 
	CastType::Pointer cast = CastType::New();
	cast->SetInput( im3D );

	typedef itk::ImageFileWriter<CharImageType3D> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetInput( cast->GetOutput() );
	writer->SetFileName( "C:/TestImages/Tracing/test.tif" );
	try
	{
		writer->Update();
	}	
	catch( itk::ExceptionObject & exp )
	{
		std::cerr << "Exception thrown while writing the file " << std::endl;
		std::cerr << exp << std::endl;
	}
	*/
}


void TraceSEMain::ImageStatistics(ImageType3D::Pointer & im3D)	{
	typedef itk::StatisticsImageFilter<ImageType3D>  StatisticsType;
	typedef itk::ImageRegionIterator<ImageType3D> IteratorType;

	StatisticsType::Pointer statistics = StatisticsType::New();
	statistics->SetInput(im3D );
	statistics->Update();
	float imin = statistics->GetMinimum();
	float imax = statistics->GetMaximum();
	float imean = statistics->GetMean();
	float istd = statistics->GetSigma();

	std::cout << "Input Image Statistics: min:"<< imin << " max:" << imax << " mean:"<< imean << " std:" << istd << std::endl;
	if ((imean - imin) < (imax - imean))	{
		std::cout << "Inverting intensities" << std::endl;
		IteratorType it(im3D, im3D->GetRequestedRegion());
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)	{
			float d = it.Get();
			float imax2 = imean+4*istd;
			d = (d > (imax2)) ? (imax2) : d;
			d = 1 - (d - imin)/(imax2 - imin);
			it.Set(255.0*d);
		}
	}
	else {
		IteratorType it(im3D, im3D->GetRequestedRegion());
			for (it.GoToBegin(); !it.IsAtEnd(); ++it)	{
			float d = it.Get();
			float imin2 = imean-4*istd;
			d = (d < (imin2)) ? (imin2) : d;
			d = (d - imin2)/(imax - imin2);
			it.Set(255.0*d);
		}
	}
	/*
	typedef itk::Image<unsigned char, 3> CharImageType3D;
	typedef itk::CastImageFilter< ImageType3D,CharImageType3D > CastType; 
	CastType::Pointer cast = CastType::New();
	cast->SetInput( im3D );

	typedef itk::ImageFileWriter<CharImageType3D> WriterType;
	WriterType::Pointer writer = WriterType::New();
	writer->SetInput( cast->GetOutput() );
	writer->SetFileName( "test.tif" );
	try
	{
		writer->Update();
	}	
	catch( itk::ExceptionObject & exp )
	{
		std::cerr << "Exception thrown while writing the file " << std::endl;
		std::cerr << exp << std::endl;
	}
	*/
}



void TraceSEMain::WriteSeedImage(ImageType2D::Pointer im, SeedContainer3D::Pointer seedCnt ,std::string filename)	{

  typedef itk::RGBPixel<unsigned char>   RGBPixelType;
  typedef itk::Image< RGBPixelType,  2 >    RGBImageType;
  typedef itk::ImageFileWriter< RGBImageType >  RGBWriterType;

  RGBImageType::Pointer RGBImage = RGBImageType::New();
  RGBImageType::IndexType start;
  start[0] = 0;
  start[1] = 0;

  RGBImageType::SizeType size = im->GetRequestedRegion().GetSize();;

  RGBImageType::RegionType region;
  region.SetSize( size );
  region.SetIndex( start );

  RGBImage->SetRegions( region );
  RGBImage->Allocate();


  itk::ImageRegionIterator<RGBImageType> iterRGB(RGBImage,RGBImage->GetBufferedRegion());
  itk::ImageRegionConstIterator<ImageType2D> iter(im,im->GetBufferedRegion());

  RGBPixelType newPixel;
  for (iter.GoToBegin(), iterRGB.GoToBegin(); !iter.IsAtEnd(); ++iter, ++iterRGB)	{

		newPixel.SetRed(static_cast<unsigned char>(iter.Get()));
		newPixel.SetGreen(static_cast<unsigned char>(iter.Get()));
	    newPixel.SetBlue(static_cast<unsigned char>(iter.Get()));
	  	iterRGB.Set(newPixel);

  }


  RGBPixelType pixelValue;
  RGBImageType::IndexType pixelIndex;

  std::cout << "Writing "<< seedCnt->getNumberOfSeeds() << " seeds";

  for (unsigned int i = 0; i< seedCnt->getNumberOfSeeds() ;i++){

	  Vect3 pos = seedCnt->getSeed(i)->getPosition();
	  pixelIndex[0] = static_cast<long int> (pos(0));
      pixelIndex[1] = static_cast<long int> (pos(1));

      pixelValue[2] = 0;
      pixelValue[1] = 255;
      pixelValue[0] = 0;

      RGBImage->SetPixel(pixelIndex, pixelValue);
  }

  //Printing the ith seed for verification

  RGBWriterType::Pointer writer = RGBWriterType::New();
  //writer->SetFileName( "Seed_Points.png" );
  writer->SetFileName( filename + "_Seed_Points.png" );

  writer->SetInput( RGBImage );
  try
    {
      writer->Update();
      std::cout <<"...done." << std::endl;
    }
  catch (itk::ExceptionObject & e)
    {
      std::cerr << "Exception in Writer: " << e << std::endl;
      exit(0);
    }
}


void TraceSEMain::UpdateMultiscale( ImageType3D::Pointer& temp, ImageType3D::Pointer& maxF)	{
	itk::ImageRegionIterator<ImageType3D> itt(temp, temp->GetBufferedRegion());
	itk::ImageRegionIterator<ImageType3D> itm(maxF, maxF->GetBufferedRegion());
	for (itt.GoToBegin(), itm.GoToBegin(); !itt.IsAtEnd(); ++itt, ++itm)	{
		itm.Set(vnl_math_max(itm.Get(), itt.Get()));
	}
}


void TraceSEMain::GetFeature( ImageType3D::Pointer& temp, ImageType3D::Pointer& svol)	{
		// set the diagonal terms in neighborhood iterator
	itk::Offset<3>
		xp =  { {2 ,  0 ,   0} },
		xn =  { {-2,  0,    0} },
		yp =  { {0,   2,	  0} },
		yn =  { {0,  -2,    0} },
		zp =  { {0,   0,    2} },
		zn =  { {0,   0,   -2} };
		//center = { {0, 0 , 0} };
	itk::Size<3> rad = {{1,1,1}};
	itk::NeighborhoodIterator<ImageType3D> nit(rad , svol, svol->GetBufferedRegion());
	itk::ImageRegionIterator<ImageType3D> it(svol, svol->GetBufferedRegion());
	itk::ImageRegionIterator<ImageType3D> itt(temp, temp->GetBufferedRegion());


	unsigned int
		xy1 =  17,
		xy2 =  9,
		xy3 =  15,
		xy4 =  11,

		yz1 =  25,
		yz2 =  1,
		yz3 =  19,
		yz4 =  7,

		xz1 =  23,
		xz2 =  3,
		xz3 =  21,
		xz4 =  5;


	typedef itk::FixedArray< double, 3 > EigenValuesArrayType;
	typedef itk::Matrix< double, 3, 3 > EigenVectorMatrixType;
	typedef itk::SymmetricSecondRankTensor<double,3> TensorType;

	itk::Size<3> sz = svol->GetBufferedRegion().GetSize();
	sz[0] = sz[0] - 3; sz[1] = sz[1] - 3; sz[2] = sz[2] - 3;

	it.GoToBegin();
	nit.GoToBegin();
	itt.GoToBegin();
	itk::Vector<float,3> sp = svol->GetSpacing();

	float alpha1 = 0.5;
	float alpha2 = 2;

	while(!nit.IsAtEnd())	{
		itk::Index<3> ndx = it.GetIndex();
		if ( (ndx[0]<2) || (ndx[1]<2) || (ndx[2]<2) || (ndx[0]>(int)sz[0])
                    || (ndx[1]>(int)sz[1]) || (ndx[2]>(int)sz[2]) )
      {
			++itt;
			++it;
			++nit;
			continue;
		  }

		TensorType h;
		h.Fill(0.0);
		h[0] = svol->GetPixel( ndx + xp ) + svol->GetPixel( ndx + xn ) - 2*nit.GetPixel( 13 );
		h[3] = svol->GetPixel( ndx + yp ) + svol->GetPixel( ndx + yn ) - 2*nit.GetPixel( 13 );
		h[5] = svol->GetPixel( ndx + zp ) + svol->GetPixel( ndx + zn ) - 2*nit.GetPixel( 13 );

		float p = 0.0f;

		if ( (h[0]+h[3]+h[5]) < 0)	{
			h[1] = nit.GetPixel(xy1) + nit.GetPixel(xy2) - nit.GetPixel(xy3) - nit.GetPixel(xy4);
			h[2] = nit.GetPixel(xz1) + nit.GetPixel(xz2) - nit.GetPixel(xz3) - nit.GetPixel(xz4);
			h[4] = nit.GetPixel(yz1) + nit.GetPixel(yz2) - nit.GetPixel(yz3) - nit.GetPixel(yz4);

			EigenValuesArrayType ev;
			EigenVectorMatrixType em;
			h.ComputeEigenAnalysis (ev, em);

			float temp;
			if(ev[0] > ev[1])	{
				temp = ev[0];
				ev[0] = ev[1];
				ev[1] = temp;
			}
			if(ev[1] > ev[2])	{
				temp = ev[1];
				ev[1] = ev[2];
				ev[2] = temp;
			}

			//Assign vesselness
			if (ev[1] < 0)	{
				//use -ev[1] as normalization, and ev[2] as the numerator
				float norm = 0;
				if(ev[2] > 0)	{
					norm = vnl_math_sqr(ev[2]/(alpha1*ev[1]));
				}
				else	{
					norm = vnl_math_sqr(ev[2]/(alpha2*ev[1]));
				}
				p = -1*ev[1]*vcl_exp(-0.5*norm);
			}
			else	{
				p = 0.0;
			}
		}
		itt.Set(p);
		++itt;
		++it;
		++nit;
	}
}

