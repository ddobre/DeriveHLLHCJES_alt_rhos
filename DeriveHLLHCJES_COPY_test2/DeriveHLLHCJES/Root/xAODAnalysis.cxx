#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>
#include <DeriveHLLHCJES/xAODAnalysis.h>
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

#include <iostream>

//MIGHT HAVE TO REMOVE THESE 

#include "xAODCaloEvent/CaloClusterContainer.h"
#include "xAODCaloEvent/CaloTowerContainer.h"
#include "xAODCaloEvent/CaloTowerAuxContainer.h"

//#include "CustomPileupStep/CustomPileupStep.h"


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

using namespace std;
using asg::ToolStore;

// this is needed to distribute the algorithm to the workers
ClassImp(xAODAnalysis)

xAODAnalysis :: xAODAnalysis (){}


xAODAnalysis :: xAODAnalysis (string config, string JESconfigfile = "JES_MC15Prerecommendation_April2015.config")
{
    m_config = config;

    m_isCollisions = false;
    JES_config_file = JESconfigfile;
}



EL::StatusCode xAODAnalysis :: setupJob (EL::Job& job)
{
    job.useXAOD ();

    // let's initialize the algorithm to use the xAODRootAccess package
    xAOD::Init( "xAODAnalysis" ).ignore(); // call before opening first file

    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: histInitialize ()
{
    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: fileExecute ()
{
    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: changeInput (bool firstFile)
{
    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: initialize ()
{
    Info("configure()", "User configuration read from : %s \n", m_config.c_str());

    m_config = gSystem->ExpandPathName( m_config.c_str() );
    TEnv* m_settings = new TEnv(m_config.c_str());
    if( !m_settings ) {
        Error("xAODAnalysis::initiaize()", "Failed to read config file!");
        Error("xAODAnalysis::initiaize()", "config name : %s",m_config.c_str());
    }

    t = clock();

    m_eventCounter = 0;

    m_event = wk()->xaodEvent();
    m_store = wk()->xaodStore();

    /////////////
    m_jetAlgos = Vectorize(m_settings->GetValue("JetAlgos",""));
    m_Postfix = m_settings->GetValue("Postfix","");
    std::cout<<"PostFix: " << m_Postfix << std::endl;

    m_TruthjetAlgos = Vectorize(m_settings->GetValue("TruthJetAlgos",""));
    m_eventShapes = Vectorize(m_settings->GetValue("EventShapes",""));
    DoPUCut = m_settings->GetValue("DoPUCut",1);
    DoUnmatch = m_settings->GetValue("DoUnmatch",0);

    DoTowers = m_settings->GetValue("DoTowers",0);
		DoTowers = false;
    DoGridTowers = m_settings->GetValue("DoGridTowers",0);
		DoGridTowers = false;
    //////////
    h_events = new TH1F("h_events"+m_Postfix,"total events", 10, 0, 10); 
    h_ev_LC_cutFlow = new TH1F("h_ev_LC_cutFlow"+m_Postfix,"event LC cutFlow", 10, 0, 10); 
    h_ev_EM_cutFlow = new TH1F("h_ev_EM_cutFlow"+m_Postfix,"event EM cutFlow", 10, 0, 10); 
    h_ev_10_cutFlow = new TH1F("h_ev_10_cutFlow"+m_Postfix,"event 1.0 jets cutFlow", 10, 0, 10); 
    h_jet_EM_cutFlow = new TH1F("h_jet_EM_cutFlow"+m_Postfix,"jet EM cutFlow", 10, 0, 10); 
    h_jet_LC_cutFlow = new TH1F("h_jet_LC_cutFlow"+m_Postfix,"jet LC cutFlow", 10, 0, 10); 
    h_10jet_cutFlow = new TH1F("h_10jet_cutFlow"+m_Postfix,"1.0 jet cutFlow", 10, 0, 10); 
    h_jet_T_LC_cutFlow = new TH1F("h_jet_T_LC_cutFlow"+m_Postfix,"truth jet cutFlow for LC jets", 10, 0, 10); 
    h_jet_T_EM_cutFlow = new TH1F("h_jet_T_EM_cutFlow"+m_Postfix,"truth jet cutFlow for EM jets", 10, 0, 10); 
    h_10jet_T_cutFlow = new TH1F("h_10jet_T_cutFlow"+m_Postfix,"truth jet cutFlow for 1.0 jets", 10, 0, 10); 

    h_dr_10Matching = new TH1F("h_dr_10Matching"+m_Postfix,"closest dr 10 jets", 200, 0, 2); 
    h_matching_ineff_count = new TH1F("h_matching_ineff_count"+m_Postfix,"matching reco-truth inefficiency counts", 10, 0, 10); 
    h_totalTjets_count = new TH1F("h_totalTjets_count"+m_Postfix,"total truth jets counts", 10, 0, 10); 

    h_dr_2d_10Matching = new TH2F("h_dr_2d_10Matching"+m_Postfix,"closest dr 10 jets vs pT", 100,0,300,200, 0, 2); 

    h_dr_10Matching->GetXaxis()->SetTitle("closest dr 1.0 reco-truth jets");
    h_dr_2d_10Matching->GetYaxis()->SetTitle("closest dr 1.0 reco-truth jets");
    h_dr_2d_10Matching->GetXaxis()->SetTitle("reco jet pT [GeV]");

    h_ev_LC_cutFlow->GetXaxis()->SetTitle("Event cutFlow for LC jets");
    h_ev_LC_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_ev_LC_cutFlow->GetXaxis()->SetBinLabel(2,"PV PU cut");
    h_ev_LC_cutFlow->GetXaxis()->SetBinLabel(3,"Avg pT cut");

    h_ev_EM_cutFlow->GetXaxis()->SetTitle("Event cutFlow for EM jets");
    h_ev_EM_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_ev_EM_cutFlow->GetXaxis()->SetBinLabel(2,"PV PU cut");
    h_ev_EM_cutFlow->GetXaxis()->SetBinLabel(3,"Avg pT cut");

    h_ev_10_cutFlow->GetXaxis()->SetTitle("Event cutFlow for 1.0 jets");
    h_ev_10_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_ev_10_cutFlow->GetXaxis()->SetBinLabel(2,"PV PU cut");
    h_ev_10_cutFlow->GetXaxis()->SetBinLabel(3,"Avg pT cut");

    h_jet_EM_cutFlow->GetXaxis()->SetTitle("Jet cutFlow for EM jets");
    h_jet_EM_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_jet_EM_cutFlow->GetXaxis()->SetBinLabel(2,"Unphysical jet");
    h_jet_EM_cutFlow->GetXaxis()->SetBinLabel(3,"Cleaning tool");

    h_jet_LC_cutFlow->GetXaxis()->SetTitle("Jet cutFlow for LC jets");
    h_jet_LC_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_jet_LC_cutFlow->GetXaxis()->SetBinLabel(2,"Unphysical jet");
    h_jet_LC_cutFlow->GetXaxis()->SetBinLabel(3,"Cleaning tool");

    h_10jet_cutFlow->GetXaxis()->SetTitle("Jet cutFlow for 1.0 jets");
    h_10jet_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_10jet_cutFlow->GetXaxis()->SetBinLabel(2,"Unphysical jet");
    h_10jet_cutFlow->GetXaxis()->SetBinLabel(3,"Cleaning tool");

    h_jet_T_EM_cutFlow->GetXaxis()->SetTitle("Truth-Reco cutFlow for EM jets");
    h_jet_T_EM_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_jet_T_EM_cutFlow->GetXaxis()->SetBinLabel(2,"Matching");
    h_jet_T_EM_cutFlow->GetXaxis()->SetBinLabel(3,"Reco iso");
    h_jet_T_EM_cutFlow->GetXaxis()->SetBinLabel(4,"Truth iso");

    h_jet_T_LC_cutFlow->GetXaxis()->SetTitle("Truth-Reco cutFlow for LC jets");
    h_jet_T_LC_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_jet_T_LC_cutFlow->GetXaxis()->SetBinLabel(2,"Matching");
    h_jet_T_LC_cutFlow->GetXaxis()->SetBinLabel(3,"Reco iso");
    h_jet_T_LC_cutFlow->GetXaxis()->SetBinLabel(4,"Truth iso");

    h_10jet_T_cutFlow->GetXaxis()->SetTitle("Truth-Reco cutFlow for 1.0 jets");
    h_10jet_T_cutFlow->GetXaxis()->SetBinLabel(1,"No cut");
    h_10jet_T_cutFlow->GetXaxis()->SetBinLabel(2,"Matching");
    h_10jet_T_cutFlow->GetXaxis()->SetBinLabel(3,"Reco iso");
    h_10jet_T_cutFlow->GetXaxis()->SetBinLabel(4,"Truth iso");

    wk()->addOutput (h_events);
    wk()->addOutput (h_ev_LC_cutFlow);
    wk()->addOutput (h_ev_EM_cutFlow);
    wk()->addOutput (h_ev_10_cutFlow);
    wk()->addOutput (h_jet_LC_cutFlow);
    wk()->addOutput (h_jet_EM_cutFlow);
    wk()->addOutput (h_10jet_cutFlow);
    wk()->addOutput (h_jet_T_EM_cutFlow);
    wk()->addOutput (h_jet_T_LC_cutFlow);
    wk()->addOutput (h_10jet_T_cutFlow);
    wk()->addOutput (h_dr_10Matching);
    wk()->addOutput (h_dr_2d_10Matching);
    wk()->addOutput (h_matching_ineff_count);
    wk()->addOutput (h_totalTjets_count);

    //////////
    for (int algo=0;algo<m_jetAlgos.size();++algo) {
        Str jetAlgo  = m_jetAlgos[algo];
	
        Str treeName=jetAlgo+m_Postfix;
        TTree* m_t = new TTree(treeName,"Tree for NI calibration");
        m_trees[treeName] = m_t;

        wk()->addOutput (m_trees[treeName]);

        m_t->Branch("EventWeight",&m_EVTvalues["EventWeight"],"EventWeight/F");
        
	if ( m_settings->GetValue("iso_corr1",0) ) m_t->Branch("iso_corr1","vector<float>",&m_values["iso_corr1"]);
	if ( m_settings->GetValue("iso_true",0) ) m_t->Branch("iso_true","vector<float>",&m_values["iso_true"]);
	//m_t->Branch("pv_cut","vector<float>",&m_values["pv_cut"]);
	if ( m_settings->GetValue("pv_cut",0) ) m_t->Branch("pv_cut",&m_EVTvalues["pv_cut"],"pv_cut/F");
	
	////////////REMINDER: Only need to comment/uncomment to add remove here
	
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
	if ( m_settings->GetValue("SIGMATGRID",0) ) m_t->Branch("SIGMATGRID",&m_EVTvalues["SIGMATGRID"],"SIGMATGRID/F");
        if ( m_settings->GetValue("SIGMAGRID",0) ) m_t->Branch("SIGMAGRID",&m_EVTvalues["SIGMAGRID"],"SIGMAGRID/F");
    
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
    const std::string name_to_JetCalibTools_LC = "LC"; //string describing the current thread, for logging
    const std::string name_to_JetCalibTools_EM = "EM"; //string describing the current thread, for logging
    TString calibSeq_LC_1 = "JetArea_Residual_Origin"; //String describing the calibration sequence to apply (see below)
    TString calibSeq_LC_2 = "JetArea_Residual_Origin_EtaJES"; //String describing the calibration sequence to apply (see below)
    TString calibSeq_EM_1 = "JetArea_Residual_Origin"; //String describing the calibration sequence to apply (see below)
    TString calibSeq_EM_2 = "JetArea_Residual_Origin_EtaJES"; //String describing the calibration sequence to apply (see below)
    myJES_akt4LC_1 = new JetCalibrationTool(name_to_JetCalibTools_LC,"AntiKt4LCTopo", JES_config_file, calibSeq_LC_1, m_isCollisions);
    myJES_akt4LC_2 = new JetCalibrationTool(name_to_JetCalibTools_LC,"AntiKt4LCTopo", JES_config_file, calibSeq_LC_2, m_isCollisions);
    myJES_akt4EM_1 = new JetCalibrationTool(name_to_JetCalibTools_EM,"AntiKt4EMTopo", JES_config_file, calibSeq_EM_1, m_isCollisions);
    myJES_akt4EM_2 = new JetCalibrationTool(name_to_JetCalibTools_EM,"AntiKt4EMTopo", JES_config_file, calibSeq_EM_2, m_isCollisions);

    EL_RETURN_CHECK("initialize()",myJES_akt4LC_1->initializeTool(name_to_JetCalibTools_LC));
    EL_RETURN_CHECK("initialize()",myJES_akt4LC_2->initializeTool(name_to_JetCalibTools_LC));
    EL_RETURN_CHECK("initialize()",myJES_akt4EM_1->initializeTool(name_to_JetCalibTools_EM));
    EL_RETURN_CHECK("initialize()",myJES_akt4EM_2->initializeTool(name_to_JetCalibTools_EM));
    
    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: execute ()
{
  
  //xAOD::TStore* store = wk()->xaodStore();
  //xAOD::TEvent* event = wk()->xaodEvent();
  
    // Here you do everything that needs to be done on every single
    // events, e.g. read input variables, apply cuts, and fill
    // histograms and trees.  This is where most of your actual analysis
    // code will go.
  ////////////
  /*
  jrun->execute();
  
  const xAOD::JetContainer* myjets = 0;
  if ( !m_store->retrieve( myjets, "MyAntiKt4LCTopoJets").isSuccess() ){ 
    Error("execute()", "Failed to retrieve My container. Exiting.");  
    return EL::StatusCode::FAILURE;
  }		

  
  xAOD::JetContainer::const_iterator myjet_itr = myjets->begin();
  xAOD::JetContainer::const_iterator myjet_end = myjets->end();
  for( ; myjet_itr != myjet_end; ++myjet_itr ) {
    //float pt=0;
    //std::cout<<"1"<<std::endl;
    float rawE = (*myjet_itr)->e();
    //pt = (*myjet_itr)->jetP4(xAOD::JetConstitScaleMomentum).Pt()/1000;
    //std::cout<<"rawE: "<<rawE<<std::endl;
  }
  */
  //////////////////
  
  if( (m_eventCounter % 10) ==0 ) Info("execute()", "Event number = %i", m_eventCounter );
  m_eventCounter++;
  
  h_events->Fill(0);
  
  const xAOD::EventInfo* eventInfo = 0;
  if( ! m_event->retrieve( eventInfo, "EventInfo").isSuccess() ){
    Error("execute()", "Failed to retrieve event info collection. Exiting." );
    return EL::StatusCode::FAILURE;
  }


  float ev_weight=0;

  ev_weight = eventInfo->mcEventWeight();

  ev_ControlCut("",0);
  
  ////////////////TOWERS
  const xAOD::CaloTowerContainer *tc = 0;
  TowerSlide ts;(DoGridTowers);
	DoTowers = false;
  if(DoTowers){
  	ts = TowerSlide(DoGridTowers);
    EL_RETURN_CHECK("execute()", m_event->retrieve(tc, "CellTowers"));
    ts.OncePerEvent(tc);
  }

  
  ///// Vertex PU cut ////////////////////////////////////////////////////////////////////////////////////

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

    
  ev_ControlCut("",1);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////

  std::cout<<"!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!- LOOPING OVER JET COLLECTIONS NOW vvvvvv "<<std::endl;


  //Loop over jet collections
  for (int algo=0;algo<m_jetAlgos.size();++algo) {

    Str eventShape = m_eventShapes[algo];
    Str jetAlgo    = m_jetAlgos[algo];
    Str truthAlgo  = m_TruthjetAlgos[algo];

    

    std::cout<<"!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!-!- JET COLLECTIONS"<<std::endl;
    std::cout<<"JetAlgo: "<<jetAlgo<<"\t"<<truthAlgo<<std::endl;

    const xAOD::EventShape* eventShapes = 0;
    if( ! m_event->retrieve( eventShapes, (string)eventShape).isSuccess() ){
      Error("execute()", "Failed to retrieve eventshape. Exiting." );
      return EL::StatusCode::FAILURE;
    }
    double rho;
    eventShapes->getDensity( xAOD::EventShape::Density, rho );
    rho*=0.001;
    //

    xAOD::CaloCluster::State state = jetAlgo.Contains("LC") ? xAOD::CaloCluster::CALIBRATED : xAOD::CaloCluster::UNCALIBRATED;
    
    const xAOD::CaloClusterContainer* clusters = 0;
    EL_RETURN_CHECK("execute()", m_event->retrieve(clusters, "CaloCalTopoClusters") );

    std::cout<<clusters<<std::endl;
    
    const xAOD::CaloTowerContainer *tc = 0;

  if(DoTowers){
    EL_RETURN_CHECK("execute()", m_event->retrieve(tc, "CellTowers"));
  }
  //  EL_RETURN_CHECK("execute()", m_event->retrieve(tc, "CellTowers"));


    float etalow = 3.2;
    float etahigh = 4.2;

    //Notes:
    //float CustomRho :: Grid(const xAOD::CaloTowerContainer* towers, float etalow, float etahigh, float r, float &sigma

    CustomRho customrho(state);
    float sigmagrid, sigmatgrid;
    float rhogrid = customrho.Grid(clusters, etalow, etahigh, 0.8, sigmagrid); 

		float rhotgrid = 0;
  	if(DoTowers){
    	rhotgrid = customrho.Grid(tc, etalow, etahigh, 0.5, sigmatgrid);
		}
    //

    //HERE
    //const xAOD::CaloClusterContainer* clusters = 0;
    //if(m_RemakeJets){
    //EL_RETURN_CHECK("execute()", m_event->retrieve(clusters, "CaloCalTopoClusters") );
    //}

    // get reco jet container
    const xAOD::JetContainer* jets = 0;
    DOUBLE_CHECK("execute()",m_event->retrieve( jets, (string)jetAlgo),m_store->retrieve( jets, (string)jetAlgo));
		//std::cout << jetAlgo << "\t" << jets->size() << std::endl;
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

    //loop in jets

    for( ; jet_itr != jet_end; ++jet_itr ) {

      jet_ControlCut(jetAlgo,0);

      float rawE = (*jet_itr)->e();
      float rawM = (*jet_itr)->m();

      if (rawM > rawE) continue; //skip unphysical jet

      jet_ControlCut(jetAlgo,1);

      // Apply cleanning tool
      //			if( !m_jetCleaning->accept( **jet_itr )) {
      //				continue; //only keep good clean jets
      //			}

      jet_ControlCut(jetAlgo,2);

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

      std::cout<<"applyjes?"<<m_ApplyJES<<" "<<jetAlgo<<std::endl;//SA
      if (m_ApplyJES !=0 && jetAlgo.Contains("AntiKt4LCTopoJets")) {

	xAOD::Jet * jet_1 = 0;
	myJES_akt4LC_1->calibratedCopy(**jet_itr,jet_1); //make a calibrated copy
	std::cout<<"applyjes?"<<m_ApplyJES<<" "<<jetAlgo<<" "<<jet_1->pt()<<std::endl;//SA
        
	jet_corr1.SetPtEtaPhiE(jet_1->pt()*0.001, jet_1->eta(), jet_1->phi(), jet_1->e()*0.001);
	delete jet_1;

	xAOD::Jet * jet_2 = 0;
	myJES_akt4LC_2->calibratedCopy(**jet_itr,jet_2); //make a calibrated copy
	std::cout<<"applyjes2?"<<m_ApplyJES<<" "<<jetAlgo<<" "<<jet_2->pt()<<std::endl;//SA
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
            
	    

    }// end for loop over jets


    //loop in TruthJets

    for( ; truth_jet_itr != truth_jet_end; ++truth_jet_itr ) {


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

	float qualityPtRatio = avgRecoPt/leadTruthPt; 
	if ((m_pTRatioMin > 0 || m_pTRatioMax > 0) && ( (qualityPtRatio<m_pTRatioMin) || (qualityPtRatio>m_pTRatioMax) )){
	  //  cout << "Skipping event which fails Pt ratio cut" << endl;
	  //  cout << "avgRecoPt: " << avgRecoPt << " and leadTruthPt: " << leadTruthPt << endl;  
	  //  cout << "weight: " << w << " and ratio: " << qualityPtRatio << endl; 
	  return EL::StatusCode::SUCCESS; //skip event 
	}
      }
    }

    ev_ControlCut(jetAlgo,2);


    if(DoUnmatch){
      for (int ti=0;ti<NrecoJets;++ti) {
	//Jet truthJet = recoJetsCorr1[ti];
	//Jet truthJet = truthJets[ti];
	
	jet_T_ControlCut(jetAlgo, 0);
	
	// Let's find the matching reco jet
	Jet recoJet; 
	int index(0);
	int Nmatches = TruthMatch_index(recoJetsCorr1[ti],truthJets,recoJet,index);
	
	// skip truth jets that don't match any reco jets
	if (Nmatches!=0) continue; 
	
	jet_T_ControlCut(jetAlgo, 1);
	
	//double DRminReco = DRmin(recoJet,recoJetsCorr1,m_recoIsoPtCut);
	//if ( m_recoIsoDR > 0 && DRminReco < m_recoIsoDR ) continue;
	
	jet_T_ControlCut(jetAlgo, 2);
      
	//double DRminTruth = DRmin(truthJet,truthJets,m_trueIsoPtCut);
	//if ( m_trueIsoDR > 0 && DRminTruth < m_trueIsoDR ) continue;
	
	jet_T_ControlCut(jetAlgo, 3);
	
	//Storing variables
	//iso_corr1.push_back(DRminReco);
	//iso_true.push_back(DRminTruth);
	//pv_cut.push_back();
	
	////CANT USE THESE FOR UNMATCH
	//E_true.push_back(truthJet.E());
	//eta_true.push_back(truthJet.Eta());
	//phi_true.push_back(truthJet.Phi());
	//m_true.push_back(truthJet.M());
	//pt_true.push_back(truthJet.Pt());

	E_calo.push_back(recoJets_calo[ti].E());
	m_calo.push_back(recoJets_calo[ti].M());
	eta_calo.push_back(recoJets_calo[ti].Eta());
	phi_calo.push_back(recoJets_calo[ti].Phi());
	pt_calo.push_back(recoJets_calo[ti].Pt());
	
	if(DoTowers) tower_rho.push_back(ts.GetRho(recoJets_calo[ti].Eta()));
	if(DoGridTowers) tgrid_rho.push_back(ts.GetGridRho(recoJets_calo[ti].Eta()));
	
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

	jet_T_ControlCut(jetAlgo, 0);

	// Let's find the matching reco jet
	Jet recoJet; 
	int index(0);
	int Nmatches = TruthMatch_index(truthJets[ti],recoJetsCorr1,recoJet,index);

	// skip truth jets that don't match any reco jets
	if (Nmatches==0) continue; 

	jet_T_ControlCut(jetAlgo, 1);
	    
	double DRminReco = DRmin(recoJet,recoJetsCorr1,m_recoIsoPtCut);
	if ( m_recoIsoDR > 0 && DRminReco < m_recoIsoDR ) continue;
      
	jet_T_ControlCut(jetAlgo, 2);
      
	double DRminTruth = DRmin(truthJet,truthJets,m_trueIsoPtCut);
	if ( m_trueIsoDR > 0 && DRminTruth < m_trueIsoDR ) continue;
      
	jet_T_ControlCut(jetAlgo, 3);

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

	if(DoTowers) tower_rho.push_back(ts.GetRho(recoJets_calo[index].Eta()));
	if(DoGridTowers) tgrid_rho.push_back(ts.GetGridRho(recoJets_calo[index].Eta()));
	
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

    Str treeName=jetAlgo+m_Postfix;

    TTree *t = m_trees[treeName];

    if (t==NULL) error("Can't find tree "+treeName+" for jet algo ");


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

    m_EVTvalues["SIGMAGRID"]=sigmagrid;
    m_EVTvalues["SIGMATGRID"]=sigmatgrid;

    t->Fill();


  }//for each jet algo

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: postExecute ()
{
    // Here you do everything that needs to be done after the main event
    // processing.  This is typically very rare, particularly in user
    // code.  It is mainly used in implementing the NTupleSvc.
    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: finalize ()
{
    // This method is the mirror image of initialize(), meaning it gets
    // called after the last event has been processed on the worker node
    // and allows you to finish up any objects you created in
    // initialize() before they are written to disk.  This is actually
    // fairly rare, since this happens separately for each worker node.
    // Most of the time you want to do your post-processing on the
    // submission node after all your histogram outputs have been
    // merged.  This is different from histFinalize() in that it only
    // gets called on worker nodes that processed input events.



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

    t = clock() - t;
    printf ("It took me %d clicks (%f seconds).\n",t,((float)t)/CLOCKS_PER_SEC);

    return EL::StatusCode::SUCCESS;
}



EL::StatusCode xAODAnalysis :: histFinalize ()
{
    // This method is the mirror image of histInitialize(), meaning it
    // gets called after the last event has been processed on the worker
    // node and allows you to finish up any objects you created in
    // histInitialize() before they are written to disk.  This is
    // actually fairly rare, since this happens separately for each
    // worker node.  Most of the time you want to do your
    // post-processing on the submission node after all your histogram
    // outputs have been merged.  This is different from finalize() in
    // that it gets called on all worker nodes regardless of whether
    // they processed input events.
    return EL::StatusCode::SUCCESS;
}


void xAODAnalysis :: ev_ControlCut(TString jetAlgo, int i) {

    if (jetAlgo.Contains("AntiKt4LCTopoJets")) ev_LC_cutFlow(i);
    if (jetAlgo.Contains("AntiKt4EMTopoJets")) ev_EM_cutFlow(i);
    if (jetAlgo.Contains("AntiKt10LCTopoTrimmedPtFrac5SmallR20Jets")) ev_10_cutFlow(i);
    if (jetAlgo.IsNull()) {
        ev_EM_cutFlow(i);
        ev_LC_cutFlow(i);
        ev_10_cutFlow(i);
    }
}


void xAODAnalysis :: jet_ControlCut(TString jetAlgo, int i) {

    if (jetAlgo.Contains("AntiKt4LCTopoJets")) jet_LC_cutFlow(i);
    if (jetAlgo.Contains("AntiKt4EMTopoJets")) jet_EM_cutFlow(i);
    if (jetAlgo.Contains("AntiKt10LCTopoTrimmedPtFrac5SmallR20Jets")) Fatjet_cutFlow(i);

}

void xAODAnalysis :: jet_T_ControlCut(TString jetAlgo, int i) {

    if (jetAlgo.Contains("AntiKt4LCTopoJets"))  jet_Truth_LC_cutFlow(i);
    if (jetAlgo.Contains("AntiKt4EMTopoJets"))  jet_Truth_EM_cutFlow(i);
    if (jetAlgo.Contains("AntiKt10LCTopoTrimmedPtFrac5SmallR20Jets"))  Fatjet_Truth_cutFlow(i);

}

void xAODAnalysis::ev_LC_cutFlow (int i) {

    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_ev_LC_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_ev_LC_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_ev_LC_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_ev_LC_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_ev_LC_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_ev_LC_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_ev_LC_cutFlow->Fill(6);
            break;
    }
}

void xAODAnalysis::ev_EM_cutFlow (int i) {

    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_ev_EM_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_ev_EM_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_ev_EM_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_ev_EM_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_ev_EM_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_ev_EM_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_ev_EM_cutFlow->Fill(6);
            break;
    }
}

void xAODAnalysis::ev_10_cutFlow (int i) {

    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_ev_10_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_ev_10_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_ev_10_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_ev_10_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_ev_10_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_ev_10_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_ev_10_cutFlow->Fill(6);
            break;
    }
}

void xAODAnalysis::jet_LC_cutFlow (int i) {

    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_jet_LC_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_jet_LC_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_jet_LC_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_jet_LC_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_jet_LC_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_LC_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_LC_cutFlow->Fill(6);
            break;
    }
}

void xAODAnalysis::jet_EM_cutFlow (int i) {

    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_jet_EM_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_jet_EM_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_jet_EM_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_jet_EM_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_jet_EM_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_EM_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_EM_cutFlow->Fill(6);
            break;
    }
}

void xAODAnalysis::Fatjet_cutFlow (int i) {

    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_10jet_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_10jet_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_10jet_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_10jet_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_10jet_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_10jet_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_10jet_cutFlow->Fill(6);
            break;
    }
}

void xAODAnalysis::jet_Truth_EM_cutFlow (int i) {


    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_jet_T_EM_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_jet_T_EM_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_jet_T_EM_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_jet_T_EM_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_jet_T_EM_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_EM_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_EM_cutFlow->Fill(6);
            break;

        case 7: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_EM_cutFlow->Fill(7);
            break;

        case 8: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_EM_cutFlow->Fill(8);
            break;


    }
}


void xAODAnalysis::jet_Truth_LC_cutFlow (int i) {


    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_jet_T_LC_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_jet_T_LC_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_jet_T_LC_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_jet_T_LC_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_jet_T_LC_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_LC_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_LC_cutFlow->Fill(6);
            break;

        case 7: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_LC_cutFlow->Fill(7);
            break;

        case 8: 
            //	ATH_MSG_INFO("passing cut 5");
            h_jet_T_LC_cutFlow->Fill(8);
            break;

    }
}

void xAODAnalysis::Fatjet_Truth_cutFlow (int i) {


    switch (i) {
        case 0:
            //	ATH_MSG_INFO("passing cut 0");
            h_10jet_T_cutFlow->Fill(0);
            break;
        case 1:
            //	ATH_MSG_INFO("passing cut 1");
            h_10jet_T_cutFlow->Fill(1);
            break;
        case 2: 
            //	ATH_MSG_INFO("passing cut 2");
            h_10jet_T_cutFlow->Fill(2);
            break;
        case 3: 
            //	ATH_MSG_INFO("passing cut 3");
            h_10jet_T_cutFlow->Fill(3);
            break;
        case 4: 
            //	ATH_MSG_INFO("passing cut 4");
            h_10jet_T_cutFlow->Fill(4);
            break;
        case 5: 
            //	ATH_MSG_INFO("passing cut 5");
            h_10jet_T_cutFlow->Fill(5);
            break;

        case 6: 
            //	ATH_MSG_INFO("passing cut 5");
            h_10jet_T_cutFlow->Fill(6);
            break;

        case 7: 
            //	ATH_MSG_INFO("passing cut 5");
            h_10jet_T_cutFlow->Fill(7);
            break;

        case 8: 
            //	ATH_MSG_INFO("passing cut 5");
            h_10jet_T_cutFlow->Fill(8);
            break;

    }
}

int xAODAnalysis :: TruthMatch(Jet myjet, JetV jets, Jet &matchingJet) {
    int Nmatches=0;
    for (uint ji=0;ji<jets.size();++ji)
        if (myjet.DeltaR(jets[ji])<m_matchingCut) {
            if (Nmatches==0) matchingJet=jets[ji];
            ++Nmatches;
        }
    return Nmatches;
}

int xAODAnalysis :: TruthMatch_index(Jet myjet, JetV jets, Jet &matchingJet, int &index) {
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


double xAODAnalysis :: DRmin(Jet myjet, JetV jets, double PtMin) {
    double DRmin=9999;
    for (uint ji=0;ji<jets.size();++ji) {
        if (PtMin>0&&jets[ji].Pt()<PtMin) continue;
        double DR=myjet.DeltaR(jets[ji]);
        if ( DR>0.001 && DR<DRmin ) DRmin=DR;
    }
    return DRmin;
}

StrV xAODAnalysis :: Vectorize(Str str) {
    StrV result;
    TObjArray *strings = str.Tokenize(" ");
    if (strings->GetEntries()==0) return result;
    TIter istr(strings);
    while (TObjString* os=(TObjString*)istr())
        add(result,os->GetString());

    return result;
}


