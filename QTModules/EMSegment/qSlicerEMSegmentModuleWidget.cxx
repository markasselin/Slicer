// Qt includes
#include <QDebug>
#include <QPushButton>

// EMSegment QTModule includes
#include "qSlicerEMSegmentModuleWidget.h"
#include "ui_qSlicerEMSegmentModule.h"
#include "qSlicerEMSegmentWorkflowWidgetStep.h"

// CTK includes
#include <ctkWorkflowStackedWidget.h>
#include <ctkWorkflowButtonBoxWidget.h>
#include <ctkWorkflowWidgetStep.h>
#include <ctkWorkflow.h>

// EMSegment QTModule includes
#include "qSlicerEMSegmentDefineTaskStep.h"
#include "qSlicerEMSegmentDefineInputChannelsStep.h"
#include "qSlicerEMSegmentDefineAnatomicalTreeStep.h"
#include "qSlicerEMSegmentDefineAtlasStep.h"
#include "qSlicerEMSegmentEditRegistrationParametersStep.h"
#include "qSlicerEMSegmentDefinePreprocessingStep.h"
#include "qSlicerEMSegmentSpecifyIntensityDistributionStep.h"
#include "qSlicerEMSegmentEditNodeBasedParametersStep.h"
#include "qSlicerEMSegmentRunSegmentationStep.h"

// STD includes
#include <cstdlib>

// EMSegment/Logic includes
#include <vtkSlicerEMSegmentLogic.h>

//-----------------------------------------------------------------------------
class qSlicerEMSegmentModuleWidgetPrivate: public ctkPrivate<qSlicerEMSegmentModuleWidget>,
                                           public Ui_qSlicerEMSegmentModule
{
public:

  CTK_DECLARE_PUBLIC(qSlicerEMSegmentModuleWidget);
  qSlicerEMSegmentModuleWidgetPrivate();
  ~qSlicerEMSegmentModuleWidgetPrivate();

  vtkSlicerEMSegmentLogic* logic() const;

  ctkWorkflow*                    Workflow;
  ctkWorkflowStackedWidget *      WorkflowWidget;
   
};

//-----------------------------------------------------------------------------
// qSlicerEMSegmentModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerEMSegmentModuleWidgetPrivate::qSlicerEMSegmentModuleWidgetPrivate()
{
  this->Workflow = 0;
  this->WorkflowWidget = 0;
}

//-------------------------------------------------------------------------------
qSlicerEMSegmentModuleWidgetPrivate::~qSlicerEMSegmentModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerEmSegmentModuleWidget methods

//-----------------------------------------------------------------------------
// qSlicerEMSegmentModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
vtkSlicerEMSegmentLogic* qSlicerEMSegmentModuleWidgetPrivate::logic()const
{
  CTK_P(const qSlicerEMSegmentModuleWidget);
  return vtkSlicerEMSegmentLogic::SafeDownCast(p->logic());
}

//-----------------------------------------------------------------------------
// qSlicerEMSegmentModuleWidget methods

//-----------------------------------------------------------------------------
const QString qSlicerEMSegmentModuleWidget::SimpleMode = "simple";
const QString qSlicerEMSegmentModuleWidget::AdvancedMode = "advanced";

//-----------------------------------------------------------------------------
CTK_CONSTRUCTOR_1_ARG_CXX(qSlicerEMSegmentModuleWidget, QWidget*);

//-----------------------------------------------------------------------------
void qSlicerEMSegmentModuleWidget::setup()
{
  CTK_D(qSlicerEMSegmentModuleWidget);
  d->setupUi(this);

  // create the workflow and workflow widget if necessary
  d->Workflow = new ctkWorkflow;
  connect(d->Workflow, SIGNAL(currentStepChanged(ctkWorkflowStep*)),
          SLOT(onCurrentStepChanged(ctkWorkflowStep*)));

  d->WorkflowWidget = new ctkWorkflowStackedWidget(this);
  d->WorkflowWidget->setWorkflow(d->Workflow);
  d->gridLayout->addWidget(d->WorkflowWidget);

  QList<qSlicerEMSegmentWorkflowWidgetStep*> allSteps;

  // DefineTask Step is common to "simple" and "advanced" modes
  qSlicerEMSegmentWorkflowWidgetStep * defineTaskStep =
      new qSlicerEMSegmentDefineTaskStep(d->Workflow);
  allSteps << defineTaskStep;

  // Step specific to "advanced" mode
  qSlicerEMSegmentWorkflowWidgetStep * defineInputChannelsAdvancedStep =
      new qSlicerEMSegmentDefineInputChannelsStep(
          qSlicerEMSegmentDefineInputChannelsStep::Advanced, d->Workflow);
  allSteps << defineInputChannelsAdvancedStep;

  // Step specific to "simple" mode
  qSlicerEMSegmentWorkflowWidgetStep * defineInputChannelsSimpleStep =
      new qSlicerEMSegmentDefineInputChannelsStep(
          qSlicerEMSegmentDefineInputChannelsStep::Simple, d->Workflow);
  allSteps << defineInputChannelsSimpleStep;

  // The following steps are common to "simple" and "advanced" modes
  QList<qSlicerEMSegmentWorkflowWidgetStep*> commonSteps;
  commonSteps << new qSlicerEMSegmentDefineAnatomicalTreeStep(d->Workflow)
      << new qSlicerEMSegmentDefineAtlasStep(d->Workflow)
      << new qSlicerEMSegmentEditRegistrationParametersStep(d->Workflow)
      << new qSlicerEMSegmentDefinePreprocessingStep(d->Workflow)
      << new qSlicerEMSegmentSpecifyIntensityDistributionStep(d->Workflow)
      << new qSlicerEMSegmentEditNodeBasedParametersStep(d->Workflow);
  allSteps << commonSteps;

  qSlicerEMSegmentWorkflowWidgetStep * runSegmentationStep =
      new qSlicerEMSegmentRunSegmentationStep(d->Workflow);
  commonSteps << runSegmentationStep;
  allSteps << runSegmentationStep;

  // Initial step
  d->Workflow->setInitialStep(defineTaskStep);

  // Add transition from DefineTask step to DefineInputChannelsAdvanced step
  d->Workflow->addTransition(defineTaskStep, defineInputChannelsAdvancedStep, Self::AdvancedMode);

  // ... and from DefineInputChannelsAdvanced step to DefineAnatomicalTree step
  d->Workflow->addTransition(defineInputChannelsAdvancedStep, commonSteps.first());

  // Add transition from DefineTask step to DefineInputChannelsSimple step
  d->Workflow->addTransition(defineTaskStep, defineInputChannelsSimpleStep, Self::SimpleMode);

  // ... and from DefineInputChannelsAdvanced to DefineAnatomicalTree step
  d->Workflow->addTransition(defineInputChannelsSimpleStep, commonSteps.first());

  // Add transition associated to common steps
  for(int i = 0; i < commonSteps.size() - 1; ++i)
    {
    d->Workflow->addTransition(commonSteps.value(i),
                               commonSteps.value(i + 1));
    }

  // Assign MRML manager to all steps
  foreach(qSlicerEMSegmentWorkflowWidgetStep * step, allSteps)
    {
    step->setMRMLManager(d->logic()->GetMRMLManager());
    }

  d->WorkflowWidget->buttonBoxWidget()->setBackButtonDefaultText("");
  d->WorkflowWidget->buttonBoxWidget()->setNextButtonDefaultText("");

  d->Workflow->start();
}

//-----------------------------------------------------------------------------
void qSlicerEMSegmentModuleWidget::onCurrentStepChanged(ctkWorkflowStep* currentStep)
{
  CTK_D(qSlicerEMSegmentModuleWidget);

  // Default state
  d->WorkflowWidget->buttonBoxWidget()->setVisible(true);

  // Hide buttonBox if current step is "DefineTask"
  if (currentStep->id() == qSlicerEMSegmentDefineTaskStep::StepId)
    {
    d->WorkflowWidget->buttonBoxWidget()->setVisible(false);
    }
}

