#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>
#include <DeriveHLLHCJES/calibration2016.h>
#include <DeriveHLLHCJES/TowerSlide.h>
#include <DeriveHLLHCJES/CustomRho.h>

#include "AsgTools/SgTEvent.h"
#include "AsgTools/AsgTool.h"

#include "xAODJet/JetContainer.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODEventShape/EventShape.h"
#include "xAODTracking/VertexContainer.h"
#include "xAODTruth/TruthVertexContainer.h"
#include "JetSelectorTools/JetCleaningTool.h"
#include "xAODTruth/TruthEventContainer.h"

#include "JetRec/PseudoJetGetter.h"
#include "JetRec/JetFromPseudojet.h"
#include "JetRec/JetFinder.h"
#include "JetRec/JetSplitter.h"
#include "JetRec/JetRecTool.h"
#include "JetRec/JetDumper.h"
#include "JetRec/JetToolRunner.h"

#include "JetInterface/IJetModifier.h"
#include "xAODRootAccess/tools/Message.h"

//JetCalibrationTool
#include "JetCalibTools/JetCalibrationTool.h"

#include "TF1.h"

#include <stdlib.h>     // atof - converts string to double
#include <typeinfo>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

//MIGHT HAVE TO REMOVE THESE

#include "xAODCaloEvent/CaloClusterContainer.h"
#include "xAODCaloEvent/CaloTowerContainer.h"
#include "xAODCaloEvent/CaloTowerAuxContainer.h"

//#include "CustomPileupStep/CustomPileupStep.h"

#define GEV .001
#define CUT_SIZE 10

#define DOUBLE_CHECK( CONTEXT, EXP1, EXP2 )		    \
  do {                                                      \
    if( ! EXP1.isSuccess() ) {if(! EXP2.isSuccess()){	    \
  Error( CONTEXT,                                           \
    XAOD_MESSAGE( "Failed to execute: %s" ),                \
	 #EXP2 );                                           \
  return EL::StatusCode::FAILURE;                           \
      }}						    \
  } while( false );

#define EL_RETURN_CHECK( CONTEXT, EXP )         \
  do {                                          \
  if( ! EXP.isSuccess() ) {                     \
  Error( CONTEXT,                               \
    XAOD_MESSAGE( "Failed to execute: %s" ),    \
         #EXP );                                \
  return EL::StatusCode::FAILURE;               \
  }                                             \
  } while( false )
using namespace fastjet;
using namespace std;
using asg::ToolStore;


//////////////////////////////////////////////////ddobre::19.02.17-20.02.17
// GLOBAL VARIABLE INITIALIZATION {

fastjet::JetAlgorithm recomb_algorithms[] = {kt_algorithm, cambridge_algorithm, antikt_algorithm};
fastjet::JetAlgorithm recomb_algo;

//
////////////////////////////////////////////////// }


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    PRELIMINARY SETUP
//
/////////////////////////


// this is needed to distribute the algorithm to the workers
ClassImp(calibration2016)

calibration2016 :: calibration2016 (){}

calibration2016 :: calibration2016 (string config, string JESconfigfile = "JES_MC15Prerecommendation_April2015.config")
{
    m_config = config;

    m_isCollisions = false;
    JES_config_file = JESconfigfile;
} //

EL::StatusCode calibration2016 :: setupJob (EL::Job& job)
{
    job.useXAOD ();

    // let's initialize the algorithm to use the xAODRootAccess package
    xAOD::Init( "calibration2016" ).ignore(); // call before opening first file

    return EL::StatusCode::SUCCESS;
} //

EL::StatusCode calibration2016 :: histInitialize ()
{
    return EL::StatusCode::SUCCESS;
} //

EL::StatusCode calibration2016 :: fileExecute ()
{
    return EL::StatusCode::SUCCESS;
} //

EL::StatusCode calibration2016 :: changeInput (bool firstFile)
{
    return EL::StatusCode::SUCCESS;
} //


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    INITIALIZE
//
/////////////////////////


EL::StatusCode calibration2016 :: initialize ()
{
    Info("configure()", "User configuration read from : %s \n", m_config.c_str());

    m_config = gSystem->ExpandPathName( m_config.c_str() );
    TEnv* m_settings = new TEnv(m_config.c_str());
    if( !m_settings ) {
        Error("calibration2016::initiaize()", "Failed to read config file!");
        Error("calibration2016::initiaize()", "config name : %s",m_config.c_str());
    }

    t = clock();
    m_eventCounter = 0;

    m_event = wk()->xaodEvent();
    m_store = wk()->xaodStore();


    //////////////////////////////////////////////////ddobre::18.02.17
    // FASTJET - INITIALIZE {

    do_reclustering = m_settings->GetValue("Do_Reclustering", false);

    if (do_reclustering) {
      jet_R = m_settings->GetValue("Jet_R",1.0);
      recomb_algo = recomb_algorithms[m_settings->GetValue("Recomb_algorithm", 2 )];

      std::cout << "========================= JET RECONSTRUCTION PARAMETERS =========================" << std::endl;
      std::cout << "" << std::endl;
      std::cout << "initialize(): Jet R used for jet reconstruction = " << jet_R << std::endl;
      std::cout << "initialize(): Jet recombination algorithm = " <<  recomb_algo << std::endl;
      std::cout << "                              --> 0 = kt, 1 = CA, 2 = antikt" << std::endl;
      std::cout << "=================================================================================" << std::endl;
    }

    ///////////////////////// }

    //////////////////////////////////////////////////ddobre::20.02.17-02.03.17
    // ALTERNATE RHOS - INITIALIZE {

    //store ranges
    m_Strip_DeltaMax = VectorizeD(m_settings->GetValue("Strip_DeltaMax",""));
    m_Circle_DeltaMax = VectorizeD(m_settings->GetValue("Circular_DeltaMax",""));
    m_Doughnut_DeltaMin = VectorizeD(m_settings->GetValue("Doughnut_DeltaMin",""));
    m_Doughnut_DeltaMax = VectorizeD(m_settings->GetValue("Doughnut_DeltaMax",""));

    //create string vectors of TTree names corresponding to ranges (value * jet_R from selected jets)
    for( int i = 0; i < m_Strip_DeltaMax.size(); ++i ){
      std::stringstream string_temp;
      string_temp << "RHOSTRIP_" << m_Strip_DeltaMax[i] << "R";
      RHOSTRIP_ranges.push_back(string_temp.str());
    }

    for( int i = 0; i < m_Circle_DeltaMax.size(); ++i ){
      std::stringstream string_temp;
      string_temp << "RHOCIRCLE_" << m_Circle_DeltaMax[i] << "R";
      RHOCIRCLE_ranges.push_back(string_temp.str());
    }

    for( int i = 0; i < m_Doughnut_DeltaMax.size() && i < m_Doughnut_DeltaMin.size() ; ++i ){
      std::stringstream string_temp;
      string_temp << "RHODOUGHNUT_" << m_Doughnut_DeltaMin[i] << "_" << m_Doughnut_DeltaMax[i] << "R";
      RHODOUGHNUT_ranges.push_back(string_temp.str());
    }

    ///////////////////////// }

    //////////////////////////////////////////////////ddobre::02.03.17
    // debugging {

    // std::cout << "----------m_Strip_DeltaMax = " << std::endl;
    // for (int i = 0; i < RHOSTRIP_ranges.size(); ++i) {
    //   std::cout << RHOSTRIP_ranges[i] << std::endl;
    //   std::cout << RHOCIRCLE_ranges[i] << std::endl;
    //   std::cout << RHODOUGHNUT_ranges[i] << std::endl;
    // }

    ///////////////////////// }

    std::cout << "" << std::endl;

    //////////
    m_jetAlgos = Vectorize(m_settings->GetValue("JetAlgos",""));
    m_Postfix = m_settings->GetValue("Postfix","");
    std::cout<<"PostFix: " << m_Postfix << std::endl;

    m_TruthjetAlgos = Vectorize(m_settings->GetValue("TruthJetAlgos",""));
    m_eventShapes = Vectorize(m_settings->GetValue("EventShapes",""));
    DoPUCut = m_settings->GetValue("DoPUCut",1);
    DoUnmatch = m_settings->GetValue("DoUnmatch",0);

    DoTowers = m_settings->GetValue("DoTowers",0);
    DoGridTowers = m_settings->GetValue("DoGridTowers",0);

    //////////
    h_events = new TH1F("h_events"+m_Postfix,"total events", 10, 0, 10);
    h_dr_10Matching = new TH1F("h_dr_10Matching"+m_Postfix,"closest dr 10 jets", 200, 0, 2);
    h_matching_ineff_count = new TH1F("h_matching_ineff_count"+m_Postfix,"matching reco-truth inefficiency counts", 10, 0, 10);
    h_totalTjets_count = new TH1F("h_totalTjets_count"+m_Postfix,"total truth jets counts", 10, 0, 10);

    h_dr_2d_10Matching = new TH2F("h_dr_2d_10Matching"+m_Postfix,"closest dr 10 jets vs pT", 100,0,300,200, 0, 2);

    h_dr_10Matching->GetXaxis()->SetTitle("closest dr 1.0 reco-truth jets");
    h_dr_2d_10Matching->GetYaxis()->SetTitle("closest dr 1.0 reco-truth jets");
    h_dr_2d_10Matching->GetXaxis()->SetTitle("reco jet pT [GeV]");

    wk()->addOutput (h_events);
    wk()->addOutput (h_dr_10Matching);
    wk()->addOutput (h_dr_2d_10Matching);
    wk()->addOutput (h_matching_ineff_count);
    wk()->addOutput (h_totalTjets_count);

    ////////// Creating a tree for each algorithm you are running
    for (int algo=0;algo<m_jetAlgos.size();++algo) {
      Str jetAlgo  = m_jetAlgos[algo];

      Str treeName=jetAlgo+m_Postfix;
      TTree* m_t = new TTree(treeName,"Tree for NI calibration");
      m_trees[treeName] = m_t;

      wk()->addOutput (m_trees[treeName]);

      m_t->Branch("EventWeight",&m_EVTvalues["EventWeight"],"EventWeight/F");

      //////////////////////////////////////////////////
      //
      //  Defining what you're running
      //  --> This is controlled by config files!
      //
      ///////////////

    	if ( m_settings->GetValue("iso_corr1",0) ) m_t->Branch("iso_corr1","vector<float>",&m_values["iso_corr1"]);
    	if ( m_settings->GetValue("iso_true",0) ) m_t->Branch("iso_true","vector<float>",&m_values["iso_true"]);
    	//m_t->Branch("pv_cut","vector<float>",&m_values["pv_cut"]);
    	if ( m_settings->GetValue("pv_cut",0) ) m_t->Branch("pv_cut",&m_EVTvalues["pv_cut"],"pv_cut/F");

    	// REMINDER: Only need to comment/uncomment to add remove here

    	if ( m_settings->GetValue("pt_orig1",0) ) m_t->Branch("pt_orig1","vector<float>",&m_values["pt_orig1"]);
    	if ( m_settings->GetValue("E_orig1",0) ) m_t->Branch("E_orig1","vector<float>",&m_values["E_orig1"]);
     	if ( m_settings->GetValue("m_orig1",0) ) m_t->Branch("m_orig1","vector<float>",&m_values["m_orig1"]);
     	if ( m_settings->GetValue("eta_orig1",0) ) m_t->Branch("eta_orig1","vector<float>",&m_values["eta_orig1"]);
      if ( m_settings->GetValue("phi_orig1",0) ) m_t->Branch("phi_orig1","vector<float>",&m_values["phi_orig1"]);

    	if ( m_settings->GetValue("pt_corr1",0) ) m_t->Branch("pt_corr1","vector<float>",&m_values["pt_corr1"]);
    	if ( m_settings->GetValue("E_corr1",0) ) m_t->Branch("E_corr1","vector<float>",&m_values["E_corr1"]);
      if ( m_settings->GetValue("m_corr1",0) ) m_t->Branch("m_corr1","vector<float>",&m_values["m_corr1"]);
      if ( m_settings->GetValue("eta_corr1",0) ) m_t->Branch("eta_corr1","vector<float>",&m_values["eta_corr1"]);
      if ( m_settings->GetValue("phi_corr1",0) ) m_t->Branch("phi_corr1","vector<float>",&m_values["phi_corr1"]);

      if ( m_settings->GetValue("pt_corr2",0) ) m_t->Branch("pt_corr2","vector<float>",&m_values["pt_corr2"]);
      if ( m_settings->GetValue("E_corr2",0) ) m_t->Branch("E_corr2","vector<float>",&m_values["E_corr2"]);
      if ( m_settings->GetValue("m_corr2",0) ) m_t->Branch("m_corr2","vector<float>",&m_values["m_corr2"]);
      if ( m_settings->GetValue("eta_corr2",0) ) m_t->Branch("eta_corr2","vector<float>",&m_values["eta_corr2"]);
      if ( m_settings->GetValue("phi_corr2",0) ) m_t->Branch("phi_corr2","vector<float>",&m_values["phi_corr2"]);

      if ( m_settings->GetValue("pt_calo",0) ) m_t->Branch("pt_calo","vector<float>",&m_values["pt_calo"]);
      if ( m_settings->GetValue("E_calo",0) ) m_t->Branch("E_calo","vector<float>",&m_values["E_calo"]);
      if ( m_settings->GetValue("m_calo",0) ) m_t->Branch("m_calo","vector<float>",&m_values["m_calo"]);
      if ( m_settings->GetValue("eta_calo",0) ) m_t->Branch("eta_calo","vector<float>",&m_values["eta_calo"]);
      if ( m_settings->GetValue("phi_calo",0) ) m_t->Branch("phi_calo","vector<float>",&m_values["phi_calo"]);

    	if ( m_settings->GetValue("tower_rho",0) ) m_t->Branch("tower_rho","vector<float>",&m_values["tower_rho"]);
      if ( m_settings->GetValue("tgrid_rho",0) ) m_t->Branch("tgrid_rho","vector<float>",&m_values["tgrid_rho"]);

      if ( m_settings->GetValue("pt_true",0) ) m_t->Branch("pt_true","vector<float>",&m_values["pt_true"]);
      if ( m_settings->GetValue("E_true",0) ) m_t->Branch("E_true","vector<float>",&m_values["E_true"]);
      if ( m_settings->GetValue("m_true",0) ) m_t->Branch("m_true","vector<float>",&m_values["m_true"]);
      if ( m_settings->GetValue("eta_true",0) ) m_t->Branch("eta_true","vector<float>",&m_values["eta_true"]);
      if ( m_settings->GetValue("phi_true",0) ) m_t->Branch("phi_true","vector<float>",&m_values["phi_true"]);

    	if ( m_settings->GetValue("area",0) ) m_t->Branch("area","vector<float>",&m_values["area"]);

      if ( m_settings->GetValue("index",0) ) m_t->Branch("index","vector<int>",&m_Intvalues["index"]);

      if ( m_settings->GetValue("mu",0) ) m_t->Branch("mu",&m_EVTvalues["mu"],"mu/F");
      if ( m_settings->GetValue("NPV",0) ) m_t->Branch("NPV",&m_EVTvalues["NPV"],"NPV/F");
      if ( m_settings->GetValue("RHO",0) ) m_t->Branch("RHO",&m_EVTvalues["RHO"],"RHO/F");
      if ( m_settings->GetValue("RHOTGRID",0) ) m_t->Branch("RHOTGRID",&m_EVTvalues["RHOTGRID"],"RHOTGRID/F");
      if ( m_settings->GetValue("RHOGRID",0) ) m_t->Branch("RHOGRID",&m_EVTvalues["RHOGRID"],"RHOGRID/F");

      //////////////////////////////////////////////////ddobre::20.02.17-02.03.17
      // Alternate rhos {

      //global rho (one rho per event)
      if ( m_settings->GetValue("GLOBAL_RHO",0) ) m_t->Branch("GLOBAL_RHO",&m_EVTvalues["GLOBAL_RHO"],"GLOBAL_RHO/F");

      //strip domain (one rho per jet per event)
      if ( m_settings->GetValue("RHOSTRIP",0) ){
        for ( int i = 0; i < RHOSTRIP_ranges.size(); ++i) {
          m_t->Branch(RHOSTRIP_ranges[i],"vector<float >",&m_values[RHOSTRIP_ranges[i]]);
        }
      }

      //circle domain (one rho per jet per event)
      if ( m_settings->GetValue("RHOCIRCLE",0) ){
        for ( int i = 0; i < RHOCIRCLE_ranges.size(); ++i) {
          m_t->Branch(RHOCIRCLE_ranges[i],"vector<float>",&m_values[RHOCIRCLE_ranges[i]]);
        }
      }

      //doughnut domain (one rho per jet per event)
      if ( m_settings->GetValue("RHODOUGHNUT",0) ){
        for ( int i = 0; i < RHODOUGHNUT_ranges.size(); ++i) {
          m_t->Branch(RHODOUGHNUT_ranges[i],"vector<float>",&m_values[RHODOUGHNUT_ranges[i]]);
        }
      }

      ///////////////////////// }

      if ( m_settings->GetValue("SIGMATGRID",0) ) m_t->Branch("SIGMATGRID",&m_EVTvalues["SIGMATGRID"],"SIGMATGRID/F");
      if ( m_settings->GetValue("SIGMAGRID",0) ) m_t->Branch("SIGMAGRID",&m_EVTvalues["SIGMAGRID"],"SIGMAGRID/F");

      //////////////////////////////////////////////////ddobre::28.02.17
      // antikt jets & testing {

//      m_t->Branch("pT_KTjets","vector<float>",&m_values["pT_KTjets"]);
//      m_t->Branch("area_KTjets","vector<float>",&m_values["area_KTjets"]);
//      m_t->Branch("pT_AntiKT4jets","vector<float>",&m_values["pT_AntiKT4jets"]);
      m_t->Branch("reco_jets_pt","vector<float>",&m_values["reco_jets_pt"]);

      ///////////////////////// }
    }

    // variable initalization
    m_matchingCut  =  m_settings->GetValue("MatchingCut",-99.0);
    m_recoIsoDR    =  m_settings->GetValue("RecoIsolationDR",-99.0);
    m_recoIsoPtCut =  m_settings->GetValue("RecoIsolationPtCut",-99.0);
    m_trueIsoDR    =  m_settings->GetValue("TruthIsolationDR",-99.0);
    m_trueIsoPtCut =  m_settings->GetValue("TruthIsolationPtCut",-99.0);
    m_pTRatioMin   =  m_settings->GetValue("MinPtRatioCut",-99.0);
    m_pTRatioMax   =  m_settings->GetValue("MaxPtRatioCut",-99.0);

    m_weightFlag = m_settings->GetValue("WeightFlag",false);

    printf("\nSELECTION:\n==============\n");
    printf("\n  Truth-Reco matching cut: DR < %.2f\n",m_matchingCut);
    if (m_recoIsoDR>0) {
        printf("\n  Reco jets required to be isolated:\n  No other reco jets with pT > %.2f GeV within DR of %.2f\n",
                m_recoIsoPtCut, m_recoIsoDR);
    } else printf("\n  No reco jet isolation required\n");
    if (m_trueIsoDR>0) {
        printf("\n  Truth jets required to be isolated:\n  No other truth jets with pT > %.2f GeV within DR of %.2f\n",
                m_trueIsoPtCut, m_trueIsoDR);
    } else printf("\n  No truth jet isolation required\n");

    printf("\n==============\n");


    m_ApplyJES =  m_settings->GetValue("ApplyJES", false);


    // Initialize JetCalibTool
    const std::string name_to_JetCalibTools_LC = "LC";
    const std::string name_to_JetCalibTools_EM = "EM";
    const std::string name_to_JetCalibTools_PF = "PF";

    TString calibSeq_LC_1 = "JetArea_Residual_Origin";
    TString calibSeq_LC_2 = "JetArea_Residual_Origin_EtaJES";
    TString calibSeq_EM_1 = "JetArea_Residual_Origin";
    TString calibSeq_EM_2 = "JetArea_Residual_Origin_EtaJES";
    TString calibSeq_PF_1 = "JetArea_Residual_Origin";
    TString calibSeq_PF_2 = "JetArea_Residual_Origin_EtaJES";

    myJES_akt4LC_1 = new JetCalibrationTool(name_to_JetCalibTools_LC,"AntiKt4LCTopo", JES_config_file, calibSeq_LC_1, m_isCollisions);
    myJES_akt4LC_2 = new JetCalibrationTool(name_to_JetCalibTools_LC,"AntiKt4LCTopo", JES_config_file, calibSeq_LC_2, m_isCollisions);
    myJES_akt4EM_1 = new JetCalibrationTool(name_to_JetCalibTools_EM,"AntiKt4EMTopo", JES_config_file, calibSeq_EM_1, m_isCollisions);
    myJES_akt4EM_2 = new JetCalibrationTool(name_to_JetCalibTools_EM,"AntiKt4EMTopo", JES_config_file, calibSeq_EM_2, m_isCollisions);
    myJES_akt4PF_1 = new JetCalibrationTool(name_to_JetCalibTools_PF,"AntiKt4EMTopo", JES_config_file, calibSeq_PF_1, m_isCollisions);
    myJES_akt4PF_2 = new JetCalibrationTool(name_to_JetCalibTools_PF,"AntiKt4EMTopo", JES_config_file, calibSeq_PF_2, m_isCollisions);

    EL_RETURN_CHECK("initialize()",myJES_akt4LC_1->initializeTool(name_to_JetCalibTools_LC));
    EL_RETURN_CHECK("initialize()",myJES_akt4LC_2->initializeTool(name_to_JetCalibTools_LC));
    EL_RETURN_CHECK("initialize()",myJES_akt4EM_1->initializeTool(name_to_JetCalibTools_EM));
    EL_RETURN_CHECK("initialize()",myJES_akt4EM_2->initializeTool(name_to_JetCalibTools_EM));
    EL_RETURN_CHECK("initialize()",myJES_akt4PF_1->initializeTool(name_to_JetCalibTools_PF));
    EL_RETURN_CHECK("initialize()",myJES_akt4PF_2->initializeTool(name_to_JetCalibTools_PF));

    return EL::StatusCode::SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    EXECUTE
//
/////////////////////////


// Eventloop algorithm here
EL::StatusCode calibration2016 :: execute ()
{

  // bookkeeping
  if( (m_eventCounter % 1000) == 0 ) Info("execute()", "Event number = %i", m_eventCounter );
  m_eventCounter++;
  h_events->Fill(0);

  // sanity check
  const xAOD::EventInfo* eventInfo = 0;
  if( ! m_event->retrieve( eventInfo, "EventInfo").isSuccess() ){
    Error("execute()", "Failed to retrieve event info collection. Exiting." );
    return EL::StatusCode::FAILURE;
  }

  float ev_weight=0;
  ev_weight = eventInfo->mcEventWeight();
  ControlCut("Event",0,"ev");

  //////////////// TOWERS
  // Not being used yet - will have to deal with when using Mike's method
  const xAOD::CaloTowerContainer *tc = 0;
  //TowerSlide ts(DoGridTowers);
  if(DoTowers){
    //EL_RETURN_CHECK("execute()", m_event->retrieve(tc, "CellTowers"));
    //ts.OncePerEvent(tc);
  }

  ///// Vertex PU cut ////////////////////////////////////////////////////////////////////////////////////

  // Kate - vertex stuff
  const xAOD::VertexContainer* RecovertexContainer = 0;
  if( ! m_event->retrieve( RecovertexContainer, "PrimaryVertices").isSuccess() ){
    Error("execute()", "Failed to retrieve primary vertices collection. Exiting." );
    return EL::StatusCode::FAILURE;
  }
  auto zRecoVertex = RecovertexContainer->at(0)->z();
  xAOD::VertexContainer::const_iterator vtx_itr = RecovertexContainer->begin();
  xAOD::VertexContainer::const_iterator vtx_end = RecovertexContainer->end();

  /*
    int index(0);
    int reco_index(0);
    for( ; vtx_itr != vtx_end; ++vtx_itr) {

    if ((*vtx_itr)->vertexType()==1) reco_index=index;

    index++;
    }

    cout<<"reco index: "<<reco_index<<" "<<RecovertexContainer->at(reco_index)->vertexType()<<" "<<RecovertexContainer->at(0)->vertexType()<<endl;
  */

  // Truth Vertex
  const xAOD::TruthVertexContainer* truthVertex = 0;
  if( ! m_event->retrieve( truthVertex, "TruthVertices").isSuccess() ){
    Error("execute()", "Failed to retrieve Truth Vertex container. Exiting." );
    return EL::StatusCode::FAILURE;
  }

  xAOD::TruthVertexContainer::const_iterator Tvtx_itr = truthVertex->begin();
  xAOD::TruthVertexContainer::const_iterator Tvtx_end = truthVertex->end();

  int Tindex(0);
  int truth_index(0);
  int barcode(-999);

  for( ; Tvtx_itr != Tvtx_end; ++Tvtx_itr) {
    if ((*Tvtx_itr)->barcode() < 0 && (*Tvtx_itr)->barcode()>barcode) {
      barcode = (*Tvtx_itr)->barcode();
      truth_index=Tindex;
    }
    Tindex++;
  }

  auto zTruthVertex = truthVertex->at(truth_index)->z();
  float deltaPVz = zRecoVertex-zTruthVertex;

  if(fabs(deltaPVz)>0.2 && DoPUCut) return EL::StatusCode::SUCCESS; // Skip event!

  ControlCut("Event",1,"ev");

  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  //Loop over jet collections
  for (int algo=0;algo<m_jetAlgos.size();++algo) {

    //std::cout << "execute(): jet algo = " << m_jetAlgos[algo] << std::endl;

    Str eventShape = m_eventShapes[algo];
    Str jetAlgo    = m_jetAlgos[algo];        // AntiKt4LCTopoJets AntiKt4EMTopoJets in standard case
    Str truthAlgo  = m_TruthjetAlgos[algo];

    //std::cout<<"JetAlgo: "<<jetAlgo<<"\t"<<truthAlgo<<std::endl;

    const xAOD::EventShape* eventShapes = 0;
    if( ! m_event->retrieve( eventShapes, (string)eventShape).isSuccess() ){
      Error("execute()", "Failed to retrieve eventshape. Exiting." );
      return EL::StatusCode::FAILURE;
    }
    // grabs the precalculated rho from the TTree
    double rho;
    eventShapes->getDensity( xAOD::EventShape::Density, rho );
    rho*=0.001;
    //

    // We're doing AntiKt4LCTopoJets and AntiKt4EMTopoJets
    xAOD::CaloCluster::State state = jetAlgo.Contains("LC") ? xAOD::CaloCluster::CALIBRATED : xAOD::CaloCluster::UNCALIBRATED;

    const xAOD::CaloClusterContainer* clusters = 0;
    EL_RETURN_CHECK("execute()", m_event->retrieve(clusters, "CaloCalTopoClusters") );

    const xAOD::CaloTowerContainer *tc = 0;
    //EL_RETURN_CHECK("execute()", m_event->retrieve(tc, "CellTowers"));

    float etalow = 3.2;
    float etahigh = 4.2;

    CustomRho customrho(state);

    float sigmagrid, sigmatgrid;
    float rhogrid = customrho.Grid(clusters, etalow, etahigh, 0.8, sigmagrid); // float rhogrid = 1
    float rhotgrid = 1.; // float rhotgrid = customrho.Grid(tc, etalow, etahigh, 0.5, sigmatgrid);

    //const xAOD::CaloClusterContainer* clusters = 0;
    //if(m_RemakeJets){
    //EL_RETURN_CHECK("execute()", m_event->retrieve(clusters, "CaloCalTopoClusters") );
    //}

    // get reco jet container
    const xAOD::JetContainer* jets = 0;
    DOUBLE_CHECK("execute()",m_event->retrieve( jets, (string)jetAlgo),m_store->retrieve( jets, (string)jetAlgo));
    /*
    if ( !m_event->retrieve( jets, (string)jetAlgo).isSuccess() ){
      Error("execute()", "Failed to retrieve JetAlgo container. Exiting.");

      return EL::StatusCode::FAILURE;
    }
    */


    xAOD::JetContainer::const_iterator jet_itr = jets->begin();
    xAOD::JetContainer::const_iterator jet_end = jets->end();

    // get truth jet container
    const xAOD::JetContainer* truth_jets = 0;

    if ( !m_event->retrieve( truth_jets, (string)truthAlgo).isSuccess() ){
      Error("execute()", "Failed to retrieve TruthAlgo container. Exiting.");

      return EL::StatusCode::FAILURE;
    }

    xAOD::JetContainer::const_iterator truth_jet_itr = truth_jets->begin();
    xAOD::JetContainer::const_iterator truth_jet_end = truth_jets->end();

    // Temporary variables definition

    JetV recoJetsCorr2;
    JetV recoJetsCorr1;
    JetV recoJets_calo;
    JetV truthJets;
    JetV recoJetsArea;
    JetV recoJetsOrig1;

    vector<float> iso_corr1;
    vector<float> iso_true;

    vector<float> E_corr1;
    vector<float> pt_corr1;
    vector<float> area;
    vector<float> m_corr1;
    vector<float> eta_corr1;
    vector<float> phi_corr1;

    vector<float> E_orig1;
    vector<float> pt_orig1;
    vector<float> m_orig1;
    vector<float> eta_orig1;
    vector<float> phi_orig1;

    vector<float> E_corr2;
    vector<float> pt_corr2;
    vector<float> m_corr2;
    vector<float> eta_corr2;
    vector<float> phi_corr2;

    vector<float> E_calo;
    vector<float> pt_calo;
    vector<float> m_calo;
    vector<float> eta_calo;
    vector<float> phi_calo;

    vector<float> tower_rho;
    vector<float> tgrid_rho;

    vector<float> E_true;
    vector<float> pt_true;
    vector<float> m_true;
    vector<float> eta_true;
    vector<float> phi_true;

    vector<int> Vindex;

    // get mu
    float mu= eventInfo->averageInteractionsPerCrossing();
    //get NPV
    float NPV_corrected(0.0);

    // get vertex container
    const xAOD::VertexContainer* vertexContainer = 0;

    if( ! m_event->retrieve( vertexContainer, "PrimaryVertices").isSuccess() ){
      Error("execute()", "Failed to retrieve primary vertices collection. Exiting." );
      return EL::StatusCode::FAILURE;
    }

    xAOD::VertexContainer::const_iterator vtx_itr = vertexContainer->begin();
    xAOD::VertexContainer::const_iterator vtx_end = vertexContainer->end();

    int NPV=0;

    for( ; vtx_itr != vtx_end; ++vtx_itr ) {

      if ((*vtx_itr)->nTrackParticles()>=2)
	NPV++;
    }

    NPV_corrected = (float) NPV;

    //////////////////////////////////////////////////
    //
    // Get jets from jetalgos
    //

    //////////////////////////////////////////////////ddobre::06.03.17
    // Storing jets for local rhos {
    // Loops over this twice? first time both go through the else statement, 2nd time they go through their respective if cases

    // -> jet container vector
    std::vector<fastjet::PseudoJet> JCV;
    std::vector<float> reco_jets_pt_vec;
//debugging
//    std::cout << " ----> " << jetAlgo << std::endl;

    ///////////////////////// }

    for( ; jet_itr != jet_end; ++jet_itr ) {


      ControlCut(jetAlgo,0,"jet");

      float rawE = (*jet_itr)->e();
      float rawM = (*jet_itr)->m();

      if (rawM > rawE) continue; //skip unphysical jet

      ControlCut(jetAlgo,1,"jet");

      //////////////////////////////////////////////////ddobre::06.03.17
      // storing jet info for local rhos {

      fastjet::PseudoJet temporaryJet( (*jet_itr)->px()*GEV, (*jet_itr)->py()*GEV, (*jet_itr)->pz()*GEV, (*jet_itr)->e()*GEV );
      JCV.push_back(temporaryJet);
      reco_jets_pt_vec.push_back( (*jet_itr)->pt()*GEV);

      ///////////////////////// }

      // Apply cleanning tool
      //			if( !m_jetCleaning->accept( **jet_itr )) {
      //				continue; //only keep good clean jets
      //			}

      ControlCut(jetAlgo,2,"jet");

      //get reco values from a given scale

      float pt=0;
      float eta=0;
      float phi=0;
      float e=0;


      pt = (*jet_itr)->jetP4(xAOD::JetConstitScaleMomentum).Pt()/1000;
      eta = (*jet_itr)->jetP4(xAOD::JetConstitScaleMomentum).Eta();
      phi = (*jet_itr)->jetP4(xAOD::JetConstitScaleMomentum).Phi();
      e = (*jet_itr)->jetP4(xAOD::JetConstitScaleMomentum).E()/1000;

      xAOD::JetFourMom_t a;
      (*jet_itr)->getAttribute("ActiveArea4vec", a);
      TLorentzVector jet_area; //SA
      jet_area.SetPxPyPzE(a.Px(),a.Py(),a.Pz(),a.E());

      xAOD::JetFourMom_t O;
      (*jet_itr)->getAttribute<xAOD::JetFourMom_t>("JetOriginConstitScaleMomentum",O);
      TLorentzVector jet_orig; //SA
      jet_orig.SetPxPyPzE(O.Px()*.001,O.Py()*.001,O.Pz()*.001,O.E()*.001);



      TLorentzVector jet_calo;
      jet_calo.SetPtEtaPhiE(pt,eta,phi,e);


      TLorentzVector jet_corr1;
      TLorentzVector jet_corr2;

      //std::cout<<"applyjes?"<<m_ApplyJES<<" "<<jetAlgo<<std::endl;//SA
      if (m_ApplyJES !=0 && jetAlgo.Contains("AntiKt4LCTopoJets")) {

	      xAOD::Jet * jet_1 = 0;
	      myJES_akt4LC_1->calibratedCopy(**jet_itr,jet_1); //make a calibrated copy
	      //std::cout<<"applyjes?"<<m_ApplyJES<<" "<<jetAlgo<<" "<<jet_1->pt()<<std::endl;//SA

	      jet_corr1.SetPtEtaPhiE(jet_1->pt()*0.001, jet_1->eta(), jet_1->phi(), jet_1->e()*0.001);
	      delete jet_1;

	      xAOD::Jet * jet_2 = 0;
	      myJES_akt4LC_2->calibratedCopy(**jet_itr,jet_2); //make a calibrated copy
	      //std::cout<<"applyjes2?"<<m_ApplyJES<<" "<<jetAlgo<<" "<<jet_2->pt()<<std::endl;//SA
	      jet_corr2.SetPtEtaPhiE(jet_2->pt()*0.001, jet_2->eta(), jet_2->phi(), jet_2->e()*0.001);
	      delete jet_2;

      }

      else if (m_ApplyJES !=0 && jetAlgo.Contains("AntiKt4EMTopoJets")) {

	      xAOD::Jet * jet_1 = 0;
	      myJES_akt4EM_1->calibratedCopy(**jet_itr,jet_1); //make a calibrated copy
	      jet_corr1.SetPtEtaPhiE(jet_1->pt()*0.001, jet_1->eta(), jet_1->phi(), jet_1->e()*0.001);
	      delete jet_1;

	      xAOD::Jet * jet_2 = 0;
	      myJES_akt4EM_2->calibratedCopy(**jet_itr,jet_2); //make a calibrated copy
	      jet_corr2.SetPtEtaPhiE(jet_2->pt()*0.001, jet_2->eta(), jet_2->phi(), jet_2->e()*0.001);
	      delete jet_2;

      }

      else {

	      jet_corr1.SetPtEtaPhiE(pt,eta,phi,e);
	      jet_corr2.SetPtEtaPhiE(pt,eta,phi,e);
      }

      recoJetsCorr2.push_back(jet_corr2);
      recoJetsCorr1.push_back(jet_corr1);
      recoJets_calo.push_back(jet_calo);
      recoJetsArea.push_back(jet_area);
      recoJetsOrig1.push_back(jet_orig);

      //std::cout<<"loop "<<std::endl;

    }// end for loop over jets


    //loop in TruthJets

    for( ; truth_jet_itr != truth_jet_end; ++truth_jet_itr ) {
      //std::cout<<"loop truth"<<std::endl;


      float pt = (*truth_jet_itr)->pt()/1000;
      float eta = (*truth_jet_itr)->eta();
      float phi = (*truth_jet_itr)->phi();
      float e = (*truth_jet_itr)->e()/1000;

      if (pt<7) continue; //FIXME

      TLorentzVector jet;
      jet.SetPtEtaPhiE(pt,eta,phi,e);

      truthJets.push_back(jet);

    }// end for loop over jets

    int NtruthJets = truthJets.size();
    int NrecoJets = recoJetsCorr1.size();


    if (m_weightFlag){
      float avgRecoPt;

      if ((NrecoJets!=0 && NtruthJets!=0)){
	/*
	  float sum = 0;
	  for (int a=0; a<NrecoJets; a++){
	  sum += recoJetsCorr1[a].Pt();
	  }
	  avgRecoPt = sum/NrecoJets;
	*/

	float leadRecoPt = recoJetsCorr1[0].Pt();
	int ind=0;

	//Get leading reco jet
	for (int b=1; b<NrecoJets; b++) {
	  if (recoJetsCorr1[b].Pt()>leadRecoPt){
	    leadRecoPt = recoJetsCorr1[b].Pt();
	    ind=b;
	  }
	}

	//Get subleading reco jet
	float subleadRecoPt = recoJetsCorr1[1].Pt();
	for (int b=1; b<NrecoJets; b++) {
	  if ((b != ind) && recoJetsCorr1[b].Pt()>subleadRecoPt){
	    subleadRecoPt = recoJetsCorr1[b].Pt();
	  }
	}

	//Get leading truth jet
	float leadTruthPt = truthJets[0].Pt();
	for (int b=1; b<NtruthJets; b++) {
	  if (truthJets[b].Pt()>leadTruthPt){
	    leadTruthPt = truthJets[b].Pt();
	  }
	}

	avgRecoPt = (leadRecoPt+subleadRecoPt)/2;

	//Skip events affected by pile-up jets
  //SOME FORM OF pT cut ----------------
	float qualityPtRatio = avgRecoPt/leadTruthPt;
	if ((m_pTRatioMin > 0 || m_pTRatioMax > 0) && ( (qualityPtRatio<m_pTRatioMin) || (qualityPtRatio>m_pTRatioMax) )){
	  //  cout << "Skipping event which fails Pt ratio cut" << endl;
	  //  cout << "avgRecoPt: " << avgRecoPt << " and leadTruthPt: " << leadTruthPt << endl;
	  //  cout << "weight: " << w << " and ratio: " << qualityPtRatio << endl;
	  return EL::StatusCode::SUCCESS; //skip event
	}
      }
    }

    ControlCut(jetAlgo,2,"jet");


    if(DoUnmatch){
      //std::cout<<"dounmatch"<<std::endl;
      for (int ti=0;ti<NrecoJets;++ti) {
	      //Jet truthJet = recoJetsCorr1[ti];
	      //Jet truthJet = truthJets[ti];

	      ControlCut(jetAlgo, 0,"truthjet");

	      // Let's find the matching reco jet
	      Jet recoJet;
	      int index(0);
	      int Nmatches = TruthMatch_index(recoJetsCorr1[ti],truthJets,recoJet,index);

	      // skip truth jets that don't match any reco jets
	      if (Nmatches!=0) continue;

	      ControlCut(jetAlgo, 1,"truthjet");

	      //double DRminReco = DRmin(recoJet,recoJetsCorr1,m_recoIsoPtCut);
	      //if ( m_recoIsoDR > 0 && DRminReco < m_recoIsoDR ) continue;

	      ControlCut(jetAlgo, 2,"truthjet");

	      //double DRminTruth = DRmin(truthJet,truthJets,m_trueIsoPtCut);
	      //if ( m_trueIsoDR > 0 && DRminTruth < m_trueIsoDR ) continue;

	      ControlCut(jetAlgo, 3,"truthjet");

	      //Storing variables
	      //iso_corr1.push_back(DRminReco);
	      //iso_true.push_back(DRminTruth);
	      //pv_cut.push_back();

	      E_calo.push_back(recoJets_calo[ti].E());
	      m_calo.push_back(recoJets_calo[ti].M());
	      eta_calo.push_back(recoJets_calo[ti].Eta());
	      phi_calo.push_back(recoJets_calo[ti].Phi());
	      pt_calo.push_back(recoJets_calo[ti].Pt());

	      //if(DoTowers) tower_rho.push_back(ts.GetRho(recoJets_calo[ti].Eta()));
	      //if(DoGridTowers) tgrid_rho.push_back(ts.GetGridRho(recoJets_calo[ti].Eta()));

	      E_orig1.push_back(recoJetsOrig1[ti].E());
	      m_orig1.push_back(recoJetsOrig1[ti].M());
	      eta_orig1.push_back(recoJetsOrig1[ti].Eta());
	      phi_orig1.push_back(recoJetsOrig1[ti].Phi());
	      pt_orig1.push_back(recoJetsOrig1[ti].Pt());

	      E_corr1.push_back(recoJetsCorr1[ti].E());
	      m_corr1.push_back(recoJetsCorr1[ti].M());
	      eta_corr1.push_back(recoJetsCorr1[ti].Eta());
	      phi_corr1.push_back(recoJetsCorr1[ti].Phi());
	      pt_corr1.push_back(recoJetsCorr1[ti].Pt());
	      area.push_back(recoJetsArea[ti].Pt());

	      E_corr2.push_back(recoJetsCorr2[ti].E());
	      m_corr2.push_back(recoJetsCorr2[ti].M());
	      eta_corr2.push_back(recoJetsCorr2[ti].Eta());
	      phi_corr2.push_back(recoJetsCorr2[ti].Phi());
	      pt_corr2.push_back(recoJetsCorr2[ti].Pt());

	      //Vindex.push_back(index);
      } // end loop in jets
    }else{
      for (int ti=0;ti<NtruthJets;++ti) {
	      Jet truthJet = truthJets[ti];

	      ControlCut(jetAlgo, 0,"truthjet");

	      // Let's find the matching reco jet
	      Jet recoJet;
	      int index(0);
	      int Nmatches = TruthMatch_index(truthJets[ti],recoJetsCorr1,recoJet,index);

	      // skip truth jets that don't match any reco jets
	      if (Nmatches==0) continue;

	      ControlCut(jetAlgo, 1,"truthjet");

	      double DRminReco = DRmin(recoJet,recoJetsCorr1,m_recoIsoPtCut);
	      if ( m_recoIsoDR > 0 && DRminReco < m_recoIsoDR ) continue;

	      ControlCut(jetAlgo, 2,"truthjet");

	      double DRminTruth = DRmin(truthJet,truthJets,m_trueIsoPtCut);
	      if ( m_trueIsoDR > 0 && DRminTruth < m_trueIsoDR ) continue;

	      ControlCut(jetAlgo, 3,"truthjet");

	      //Storing variables
	      iso_corr1.push_back(DRminReco);
	      iso_true.push_back(DRminTruth);
	      //pv_cut.push_back();

	      E_true.push_back(truthJet.E());
	      eta_true.push_back(truthJet.Eta());
	      phi_true.push_back(truthJet.Phi());
	      m_true.push_back(truthJet.M());
	      pt_true.push_back(truthJet.Pt());

	      E_calo.push_back(recoJets_calo[index].E());
	      m_calo.push_back(recoJets_calo[index].M());
	      eta_calo.push_back(recoJets_calo[index].Eta());
	      phi_calo.push_back(recoJets_calo[index].Phi());
	      pt_calo.push_back(recoJets_calo[index].Pt());

	      //if(DoTowers) tower_rho.push_back(ts.GetRho(recoJets_calo[index].Eta()));
	      //if(DoGridTowers) tgrid_rho.push_back(ts.GetGridRho(recoJets_calo[index].Eta()));

	      E_orig1.push_back(recoJetsOrig1[index].E());
	      m_orig1.push_back(recoJetsOrig1[index].M());
	      eta_orig1.push_back(recoJetsOrig1[index].Eta());
	      phi_orig1.push_back(recoJetsOrig1[index].Phi());
	      pt_orig1.push_back(recoJetsOrig1[index].Pt());

	      E_corr1.push_back(recoJetsCorr1[index].E());
	      m_corr1.push_back(recoJetsCorr1[index].M());
	      eta_corr1.push_back(recoJetsCorr1[index].Eta());
	      phi_corr1.push_back(recoJetsCorr1[index].Phi());
	      pt_corr1.push_back(recoJetsCorr1[index].Pt());
	      area.push_back(recoJetsArea[index].Pt());

	      E_corr2.push_back(recoJetsCorr2[index].E());
	      m_corr2.push_back(recoJetsCorr2[index].M());
	      eta_corr2.push_back(recoJetsCorr2[index].Eta());
	      phi_corr2.push_back(recoJetsCorr2[index].Phi());
	      pt_corr2.push_back(recoJetsCorr2[index].Pt());

	      Vindex.push_back(index);
      } // end loop in jets
    }

    //////////////////////////////////////////////////ddobre::18.02.17--28.02.17
    // FASTJET::RECLUSTERING AND ALTERNATE RHOS {

    //initialize pseudojet vector and associated jet areas - for KT clustring
    vector<PseudoJet>* reclustered_jets;
    vector<float> KTjetArea;
    vector<float> KTjetpT;

    //////////////////////////////////////////////////
    // antikt {
//    vector<PseudoJet>* antikt4_reclustered_jets;
//    vector<float> antikt4_jetpT;
    ///////////////////////// }

    //initialize the rhos you want
    float global_Rho;
    std::vector<std::vector<float>> rhostrip_vec_perRange;
    std::vector<std::vector<float>> rhocircle_vec_perRange;
    std::vector<std::vector<float>> rhodoughnut_vec_perRange;

    //initialize area definition to derive jet areas
    fastjet::AreaDefinition area_def;
    area_def = fastjet::AreaDefinition(fastjet::active_area); //, fastjet::GhostedAreaSpec(6,1,0.0005)); // use this for more fine area calculations

    if (do_reclustering) {

      //////////////////////////////////////////////////
      // antikt {
//      fastjet::JetDefinition antikt4_jet_Def( antikt_algorithm, 0.4);
      ///////////////////////// }

      //define algo and radius, then your vector of input particles used in reclustering
      fastjet::JetDefinition jet_def(recomb_algo, jet_R);
      std::vector<fastjet::PseudoJet> input_particles;

      //loop over the cluster, extract particles from CaloCalTopoClusters
      for(int c = 0; c < clusters->size(); ++c) {
        if((clusters->at(c)->p4(state)).Pt()*GEV < 0 || (clusters->at(c)->p4(state)).E()*GEV < 0) continue;
        fastjet::PseudoJet PJ((clusters->at(c)->p4(state)).Px()*GEV, (clusters->at(c)->p4(state)).Py()*GEV, (clusters->at(c)->p4(state)).Pz()*GEV, (clusters->at(c)->p4(state)).E()*GEV);
        input_particles.push_back(PJ);
      }

      //////////////////////////////////////////////////
      // antikt {
//      fastjet::ClusterSequence antikt4_clust_seq(input_particles, antikt4_jet_Def);
//      vector<PseudoJet> antikt4_sorted_jets_temp = sorted_by_pt(antikt4_clust_seq.inclusive_jets());
//      antikt4_reclustered_jets = &antikt4_sorted_jets_temp;
//      for(int j = 0; j < antikt4_sorted_jets_temp.size(); ++j) {
//        if (antikt4_reclustered_jets->at(j).pt() > 4.5) antikt4_jetpT.push_back(antikt4_reclustered_jets->at(j).pt()); }
      ///////////////////////// }

      //recluster the extracted particles with associated jet definition and ghost area definition
      fastjet::ClusterSequenceArea clust_seq(input_particles, jet_def, area_def);

      //extract reclustered jets and write to a vector -----> CHECK IF YOU CAN JUST DO >> &clust_seq.inclusive_jets()
      vector<PseudoJet> sorted_jets_temp = clust_seq.inclusive_jets();
      reclustered_jets = &sorted_jets_temp;

      //fill up area and pt vectors for easier access later on
      for(int j = 0; j < sorted_jets_temp.size(); ++j) {
        KTjetArea.push_back(reclustered_jets->at(j).area());
        if (reclustered_jets->at(j).pt() > 4.5 ) KTjetpT.push_back(reclustered_jets->at(j).pt());
      }

      //////////////////////////////////////////////////ddobre::03.03.17
      // debugging - printing {

      // std::cout << "Clustering with " << jet_def.description() << std::endl;
      // // print the reclustered_jets
      // std::cout <<   "        pt  y  phi  area" << std::endl;
      // for (unsigned i = 0; i < reclustered_jets->size(); i++) {
      //   std::cout << "jet " << i << ": "<< reclustered_jets->at(i).pt() <<" "<< reclustered_jets->at(i).rap() <<" "<< reclustered_jets->at(i).phi() <<" "<< reclustered_jets->at(i).area() <<std::endl;
      // }

      ///////////////////////// }

      //////////////////////////////////////////////////
      // GLOBAL RHO
      // Use median of set of pt/area with |eta| < 2.0  -  Found in CustomRho.cxx
      global_Rho = customrho.GlobalDomain_rho(reclustered_jets, KTjetArea);

      //////////////////////////////////////////////////
      // STRIP DOMAIN RHO ALG
      // median of set of jets within specified range DeltaMax*jet_R  - in CustomRho.cxx
      // feed in jets from jet container vector

      for(int pos = 0; pos < m_Strip_DeltaMax.size() ; ++pos){

        std::vector<float> strip_Rho_vec;
        for(int j = 0; j < JCV.size(); ++j){
          float strip_Rho = customrho.StripDomain_rho(reclustered_jets, KTjetArea, m_Strip_DeltaMax[pos], JCV[j].rapidity() , JCV[j].phi(), jet_R);
          strip_Rho_vec.push_back(strip_Rho);
        }
        rhostrip_vec_perRange.push_back(strip_Rho_vec);
      }


      //////////////////////////////////////////////////
      // CIRCLE DOMAIN RHO ALG
      // median of set of jets within specified range DeltaMax*jet_R  - in CustomRho.cxx

      for(int pos = 0; pos < m_Circle_DeltaMax.size() ; ++pos){

        std::vector<float> circle_Rho_vec;
        for(int j = 0; j < JCV.size(); ++j){
          float circle_Rho = customrho.CircleDomain_rho(reclustered_jets, KTjetArea, m_Circle_DeltaMax[pos], JCV[j].rapidity(), JCV[j].phi(), jet_R);
          circle_Rho_vec.push_back(circle_Rho);
        }
        rhocircle_vec_perRange.push_back(circle_Rho_vec);
      }


      //////////////////////////////////////////////////
      // DOUGHNUT DOMAIN RHO ALG
      // median of set of jets within specified range DeltaMax*jet_R  - in CustomRho.cxx

      for(int pos = 0; pos < m_Doughnut_DeltaMax.size() ; ++pos){

        std::vector<float> doughnut_Rho_vec;
        for(int j = 0; j < JCV.size(); ++j){
          float doughnut_Rho = customrho.DoughnutDomain_rho(reclustered_jets, KTjetArea, m_Doughnut_DeltaMin[pos], m_Doughnut_DeltaMax[pos], JCV[j].rapidity(), JCV[j].phi(), jet_R);
          doughnut_Rho_vec.push_back(doughnut_Rho);
        }
        rhodoughnut_vec_perRange.push_back(doughnut_Rho_vec);
      }
    }
    // END }
    ///////////////////////////////////////////////////


    //std::cout<<"next"<<std::endl;

    Str treeName=jetAlgo+m_Postfix;

    TTree *t = m_trees[treeName];

    if (t==NULL) {printf("\nERROR:\n  %s\n\n",TString("Can't find tree "+treeName+" for jet algo ").Data()); abort();}

    m_EVTvalues["EventWeight"]=ev_weight;

    m_values["iso_corr1"] = iso_corr1;
    m_values["iso_true"] = iso_true;
    m_EVTvalues["pv_cut"] = fabs(deltaPVz);

    m_values["E_true"]=E_true;
    m_values["eta_true"]=eta_true;
    m_values["phi_true"]=phi_true;
    m_values["m_true"]=m_true;
    m_values["pt_true"]=pt_true;

    m_values["E_orig1"]=E_orig1;
    m_values["m_orig1"]=m_orig1;
    m_values["eta_orig1"]=eta_orig1;
    m_values["phi_orig1"]=phi_orig1;
    m_values["pt_orig1"]=pt_orig1;

    m_values["E_calo"]=E_calo;
    m_values["m_calo"]=m_calo;
    m_values["eta_calo"]=eta_calo;
    m_values["phi_calo"]=phi_calo;
    m_values["pt_calo"]=pt_calo;

    m_values["tower_rho"]=tower_rho;
    m_values["tgrid_rho"]=tgrid_rho;

    m_values["E_corr1"]=E_corr1;
    m_values["eta_corr1"]=eta_corr1;
    m_values["phi_corr1"]=phi_corr1;
    m_values["m_corr1"]=m_corr1;
    m_values["pt_corr1"]=pt_corr1;
    m_values["area"]=area;

    m_values["E_corr2"]=E_corr2;
    m_values["eta_corr2"]=eta_corr2;
    m_values["phi_corr2"]=phi_corr2;
    m_values["m_corr2"]=m_corr2;
    m_values["pt_corr2"]=pt_corr2;

    m_Intvalues["index"]=Vindex;
    m_EVTvalues["mu"]=mu;
    m_EVTvalues["NPV"]=NPV_corrected;
    m_EVTvalues["RHO"]=(float)rho;

    m_EVTvalues["RHOGRID"]=rhogrid;
    m_EVTvalues["RHOTGRID"]=rhotgrid;

    m_EVTvalues["GLOBAL_RHO"]=global_Rho;

    m_EVTvalues["SIGMAGRID"]=sigmagrid;
    m_EVTvalues["SIGMATGRID"]=sigmatgrid;

    //////////////////////////////////////////////////ddobre::20.02.17-03.03.17
    // antikt {

//    m_values["pT_KTjets"]=KTjetpT;
//    m_values["area_KTjets"]=KTjetArea;
//    m_values["pT_AntiKT4jets"]=antikt4_jetpT;
    m_values["reco_jets_pt"]=reco_jets_pt_vec;

    ///////////////////////// }

    //strip domain (one rho per jet per event)
    for ( int i = 0; i < RHOSTRIP_ranges.size(); ++i) {
      m_values[RHOSTRIP_ranges[i]]=rhostrip_vec_perRange[i];
    }

    //circle domain (one rho per jet per event)
    for ( int i = 0; i < RHOCIRCLE_ranges.size(); ++i) {
      m_values[RHOCIRCLE_ranges[i]]=rhocircle_vec_perRange[i];
    }

    //doughnut domain (one rho per jet per event)
    for ( int i = 0; i < RHODOUGHNUT_ranges.size(); ++i) {
      m_values[RHODOUGHNUT_ranges[i]]=rhodoughnut_vec_perRange[i];
    }

    t->Fill();

    //std::cout<<"jet algo out"<<std::endl;
  }//for each jet algo

  //std::cout<<"out"<<std::endl;

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode calibration2016 :: postExecute ()
{
    return EL::StatusCode::SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    FINALIZE
//
/////////////////////////


EL::StatusCode calibration2016 :: finalize ()
{
  if( myJES_akt4LC_1 ) {
    delete myJES_akt4LC_1;
    myJES_akt4LC_1 = 0;
  }

  if( myJES_akt4LC_2 ) {
    delete myJES_akt4LC_2;
    myJES_akt4LC_2 = 0;
  }


  if( myJES_akt4EM_1 ) {
    delete myJES_akt4EM_1;
    myJES_akt4EM_1 = 0;
  }

  if( myJES_akt4EM_2 ) {
    delete myJES_akt4EM_2;
    myJES_akt4EM_2 = 0;
  }

  if( myJES_akt4PF_1 ) {
    delete myJES_akt4PF_1;
    myJES_akt4PF_1 = 0;
  }

  if( myJES_akt4PF_2 ) {
    delete myJES_akt4PF_2;
    myJES_akt4PF_2 = 0;
  }

  t = clock() - t;
  printf ("It took me %d clicks (%f seconds).\n",t,((float)t)/CLOCKS_PER_SEC);

  //high_rho_range.close();
  //norm_rho_range.close();


  return EL::StatusCode::SUCCESS;
}



EL::StatusCode calibration2016 :: histFinalize ()
{
    return EL::StatusCode::SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//
//    ADDITIONAL FUNCTIONS
//
/////////////////////////


void calibration2016 :: ControlCut(TString jetAlgo, int i, TString category) {
  if(i < 0 || i > CUT_SIZE -1) return;
  if(m_cuts.count(jetAlgo+"_"+category) == 0){
    m_cuts[jetAlgo+"_"+category] = new TH1F("CUTFLOW_"+jetAlgo+"_"+category+m_Postfix,jetAlgo+"_"+category+" cutFlow", CUT_SIZE, -.5, (float)CUT_SIZE - .5);
    if(category==TString("ev")){
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetTitle("Event cutFlow for LC jets");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(1,"No cut");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(2,"PV PU cut");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(3,"Avg pT cut");
    }else if(category==TString("jet")){
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetTitle("Jet cutFlow for EM jets");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(1,"No cut");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(2,"Unphysical jet");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(3,"Cleaning tool");
    }else if(category==TString("truthjet")){
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetTitle("Truth-Reco cutFlow for EM jets");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(1,"No cut");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(2,"Matching");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(3,"Reco iso");
      m_cuts[jetAlgo+"_"+category]->GetXaxis()->SetBinLabel(4,"Truth iso");
    }
    wk()->addOutput(m_cuts[jetAlgo+"_"+category]);
  }
  m_cuts[jetAlgo+"_"+category]->Fill(i);
}

int calibration2016 :: TruthMatch(Jet myjet, JetV jets, Jet &matchingJet) {
    int Nmatches=0;
    for (uint ji=0;ji<jets.size();++ji)
        if (myjet.DeltaR(jets[ji])<m_matchingCut) {
            if (Nmatches==0) matchingJet=jets[ji];
            ++Nmatches;
        }
    return Nmatches;
}

int calibration2016 :: TruthMatch_index(Jet myjet, JetV jets, Jet &matchingJet, int &index) {
    int Nmatches=0;

    double drmin = 999;
    int Min_index=-1;

    h_totalTjets_count->Fill(0);

    for (uint ji=0;ji<jets.size();++ji) {

        double dr = myjet.DeltaR(jets[ji]);

        if (dr < m_matchingCut) ++Nmatches;

        //find minimum:
        if (dr < drmin) {
            drmin = dr;
            Min_index = ji;
        }
    }

    if (jets.size()==0) {
        h_matching_ineff_count->Fill(0);
    }

    if (drmin<m_matchingCut) {
        matchingJet=jets[Min_index]; index=Min_index;
    }

    h_dr_10Matching->Fill(drmin);
    h_dr_2d_10Matching->Fill(myjet.Pt(),drmin);

    return Nmatches;
}


double calibration2016 :: DRmin(Jet myjet, JetV jets, double PtMin) {
    double DRmin=9999;
    for (uint ji=0;ji<jets.size();++ji) {
        if (PtMin>0&&jets[ji].Pt()<PtMin) continue;
        double DR=myjet.DeltaR(jets[ji]);
        if ( DR>0.001 && DR<DRmin ) DRmin=DR;
    }
    return DRmin;
}

VecD calibration2016 :: VectorizeD(Str str) {
    VecD result;
    TObjArray *strings = str.Tokenize(" ");
    if (strings->GetEntries()==0) return result;
    TIter istr(strings);
    while (TObjString* os=(TObjString*)istr())
      result.push_back(atof(os->GetString()));

    return result;
}
StrV calibration2016 :: Vectorize(Str str) {
    StrV result;
    TObjArray *strings = str.Tokenize(" ");
    if (strings->GetEntries()==0) return result;
    TIter istr(strings);
    while (TObjString* os=(TObjString*)istr())
      result.push_back(os->GetString());

    return result;
}