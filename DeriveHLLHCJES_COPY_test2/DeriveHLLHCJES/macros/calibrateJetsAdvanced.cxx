//#include "CalibrationTools.h"
#include "ScriptEtaJESCorrection.h"

void calibrateJetsAdvanced(const char* tag = "C0_", const char* singleRtag = "", const char* customMain = "", const char* ver=""){
  //gROOT->ProcessLine(".L ScriptEtaJESCorrection.h");

  TString downloadC("Download.config"); //used for organizing the local work
  TString localC("Local.config"); //used for custom tree variables

  TString customMainS(customMain);
  TString mainC("");
  if(customMainS.EqualTo("")) mainC="Main.config"; //used as a template (to do--use to force a calibration)
  else mainC = customMain;
  std::cout<<"customMain "<<customMain<<std::endl;
  
  std::cout<<"mainC "<<mainC<<std::endl;
  
  TEnv* downloadE = new TEnv(downloadC);
  std::vector<TString> Rtags;
  TString singleRtagS(singleRtag);
  if(singleRtagS==""){
    Rtags = Vectorize(downloadE->GetValue("Rtags",""));
    std::cout<<"vec: "<<downloadE->GetValue("Rtags","")<<std::endl;
  }else{
    Rtags.push_back(singleRtagS);
    std::cout<<"single: "<<singleRtagS<<std::endl;
  }
  
  TEnv* localE = new TEnv(localC); //directing the local calibration
  //std::vector<TString> JetAlgos = Vectorize(localE->GetValue("JetAlgos",""));
  
  TEnv* mainE = new TEnv(mainC);
  //TString jesS = mainE->GetValue("AbsoluteJES.CalibFile","JES.config");
  //TEnv* jesE = new TEnv(jesS);
  //TString residualS = mainE->GetValue("ResidualOffset.CalibFile","Residual.config");
  //std::cout<<"Residual Calib file: "<<residualS<<std::endl;
  //TEnv* residualE = new TEnv(residualS);
  //set values
  //--Control Output
  bool doUncalibrated = localE->GetValue("doUncalibrated",0);
  bool doArea = localE->GetValue("doArea",0);
  bool doAreaResidual = localE->GetValue("doAreaResidual",0);
  bool doAreaResidualConst = localE->GetValue("doAreaResidualConst",0);
  bool doAreaResidualJES = localE->GetValue("doAreaResidualJES",0);
  bool doAreaResidualConstJES = localE->GetValue("doAreaResidualConstJES",0);
  bool useOriginCorrection = localE->GetValue("useOriginCorrection",1);

  TString verS(ver);
  TString Version = downloadE->GetValue("Version","VERSION");
  

  TString OutputFolder = localE->GetValue("OutputFolder","/tmp/alkire/CHOOSEANOUTPUTFOLDER");
  if(verS!=""){ Version = verS; OutputFolder = "/tmp/alkire/"+Version;}
  TString CalibFolder = localE->GetValue("CalibFolder","/tmp/alkire/CHOOSEANOUTPUTFOLDER");
  TString InnerFolder = localE->GetValue("InnerFolder","HLLHC");
  TString DateString = localE->GetValue("DateString","Aug2016");
    
  //TString OutputFolder = localE->GetValue("OutputFolder","/tmp/alkire/CHOOSEANOUTPUTFOLDER");
  std::cout<<"Making various directories if they don't already exist."<<std::endl;
  system(TString("mkdir "+OutputFolder+"/calibrated").Data());
  system(TString("mkdir "+CalibFolder).Data());
  system(TString("mkdir "+CalibFolder+"/CalibrationFactors").Data());
  system(TString("mkdir "+CalibFolder+"/CalibrationConfigs").Data());  
  system(TString("mkdir "+CalibFolder+"/CalibrationFactors/"+InnerFolder).Data());
  system(TString("mkdir "+CalibFolder+"/CalibrationConfigs/"+InnerFolder).Data());  
  
  //. TString OutputFile = localE->GetValue("OutputFile","CHOOSEANOUTPUTFILE.root");
  //. TFile * OutputFileF = new TFile(OutputFolder+"/calibrated/"+OutputFile,"recreate");
  
  //--Pile-up and Residual
  
  //double DefaultMuRef = mainE->GetValue("OffsetCorrection.DefaultMuRef", 0.);
  
  //double DefaultNPVRef = mainE->GetValue("OffsetCorrection.DefaultNPVRef", 0.);
  //double MuScaleFactor = mainE->GetValue("MuScaleFactor", 1.0);

  //TString ResidualOffsetName = mainE->GetValue("ResidualOffsetCorrection.Name","UPGRIDINV12r7486");
  //std::vector<double> AbsEtaBins = VectorizeD(mainE->GetValue(ResidualOffsetName+".AbsEtaBins","0 4.5"));
  //TAxis * AbsEtaBinsAxis = new TAxis(AbsEtaBins.size()-1,&AbsEtaBins[0]);

  //--JES
  
  //--Calibration
  std::vector<TString> JetAlgos = Vectorize(localE->GetValue("JetAlgos","AntiKt4LCTopoJets AntiKt4EMTopoJets"));
  
  localE->Print();

  TString tagS = TString(tag);
  //TString Version = downloadE->GetValue("Version","VERSION");
  

  //Various PU subtraction procedures
  //std::vector<TString> rhos = Vectorize(TString(listOfRhos));
  

    
  //rtag loop over files
  for(int r = 0; r < Rtags.size(); ++r){
    //TString OutputFile = localE->GetValue("OutputFile","CHOOSEANOUTPUTFILE.root");
    TFile * OutputFileF = new TFile(OutputFolder+"/calibrated/"+tagS+Rtags.at(r)+".root","recreate");

    double DefaultMuRef = mainE->GetValue("OffsetCorrection.DefaultMuRef", 0.);
    
    double DefaultNPVRef = mainE->GetValue("OffsetCorrection.DefaultNPVRef", 0.);
    double MuScaleFactor = mainE->GetValue("MuScaleFactor", 1.0);

    //TString ResidualOffsetName = mainE->GetValue("ResidualOffsetCorrection.Name","UPGRIDINV12r7486");
    TString ResidualOffsetFileName(Version+Rtags.at(r));
    
    std::cout<<"Opening: "<<OutputFolder+"/residual/"+ResidualOffsetFileName+".config"<<std::endl;
    TEnv* residualE = new TEnv(OutputFolder+"/residual/"+ResidualOffsetFileName+".config");
    residualE->Print();
    //////

    TString InputFile = OutputFolder+"/"+Rtags.at(r)+".root"; //localE->GetValue("InputFile","CHOOSE AN INPUT FILE");
    //TDirectory* whydoesrootsucksomuch = gDirectory;
    TFile *InputFileF = new TFile(InputFile);
    //whydoesrootsucksomuch->cd();
    
    TString CaloPt = localE->GetValue("CaloPt","");
    std::vector<float>* pt; 
    TString CaloEta = localE->GetValue("CaloEta","");
    std::vector<float>* eta;
    TString CaloPhi = localE->GetValue("CaloPhi","");
    std::vector<float>* phi;
    TString CaloE = localE->GetValue("CaloE",""); std::cout<<CaloE<<std::endl;
    std::vector<float>* e; 
    TString CaloM = localE->GetValue("CaloM","");
    std::vector<float>* m;
    
    TString TruePt = localE->GetValue("TruePt","");
    std::vector<float>* tpt; 
    TString TrueEta = localE->GetValue("TrueEta","");
    std::vector<float>* teta;
    TString TruePhi = localE->GetValue("TruePhi","");
    std::vector<float>* tphi;
    TString TrueE = localE->GetValue("TrueE",""); std::cout<<TrueE<<std::endl;
    std::vector<float>* te; 
    TString TrueM = localE->GetValue("TrueM","");
    std::vector<float>* tm;
    
    TString OrigPt = localE->GetValue("OrigPt","");
    std::vector<float>* opt; 
    TString OrigEta = localE->GetValue("OrigEta","");
    std::vector<float>* oeta;
    TString OrigPhi = localE->GetValue("OrigPhi","");
    std::vector<float>* ophi;
    TString OrigE = localE->GetValue("OrigE",""); std::cout<<OrigE<<std::endl;
    std::vector<float>* oe; 
    TString OrigM = localE->GetValue("OrigM","");
    std::vector<float>* om;
  
    TString CaloA = localE->GetValue("CaloA","");
    std::vector<float>* a;
    TString Mu = localE->GetValue("Mu","");
    float mu;
    TString NPV = localE->GetValue("NPV","");
    float npv;

    std::vector<TString> Rhos = Vectorize(localE->GetValue("Rho","RHO"));
    std::vector<TString> Tags = Vectorize(localE->GetValue("Tag","NONE"));
    std::vector<TString> JetRhos = Vectorize(localE->GetValue("JetRho",""));
    std::vector<TString> JetTags = Vectorize(localE->GetValue("JetTag",""));
    ReplaceNONEs(Rhos);
    ReplaceNONEs(Tags);
    
    //float rho;
    TString EventWeight = localE->GetValue("EventWeight","EW");
    float w;

    Long64_t Nevents = localE->GetValue("Nevents",0);
    Long64_t Offset = localE->GetValue("Offset",0);

    //OutputFileF->cd();
    std::cout<<".."<<std::endl;
  
    for(int t = 0; t < JetAlgos.size(); ++t){
      std::cout<<"."<<t<<std::endl;
  
      //ScriptEtaJESCorrection * sjes; 
      //if(doAreaResidualConstJES){
      //sjes = new ScriptEtaJESCorrection( mainE ,JetAlgos.at(t));
      //}

      //TString ResidualOffsetName(Version+Rtags.at(r)+JetAlgos.at(t));
      //std::cout<<"ResidualOffsetName: "<<ResidualOffsetName<<std::endl;
      //std::vector<double> AbsEtaBins = VectorizeD(residualE->GetValue(ResidualOffsetName+".AbsEtaBins","0 4.5"));
      //TAxis * AbsEtaBinsAxis = new TAxis(AbsEtaBins.size()-1,&AbsEtaBins[0]);
      
      //std::vector<double> NPVTerm = VectorizeD(residualE->GetValue(ResidualOffsetName+".NPVTerm."+JetAlgos.at(t),"0 0 0 0 0")); 
      //std::vector<double> MuTerm = VectorizeD(residualE->GetValue(ResidualOffsetName+".MuTerm."+JetAlgos.at(t),"0 0 0 0 0")); 
      
      TTree * tree = (TTree*)InputFileF->Get(JetAlgos.at(t));
      tree->GetListOfBranches()->Print();  
      Long64_t Nentry = Nevents > 0 ? Nevents : tree->GetEntries();
    
      //tree->SetBranchStatus("*",0);
      std::cout<<"Nentry: "<<Nentry<<std::endl;
      if(CaloPt!=""){pt=0; tree->SetBranchAddress(CaloPt,&pt);}
      if(CaloEta!=""){eta=0; tree->SetBranchAddress(CaloEta,&eta);}
      if(CaloPhi!=""){phi=0; tree->SetBranchAddress(CaloPhi,&phi);}
      if(CaloE!=""){e=0; tree->SetBranchAddress(CaloE,&e);}  
      if(CaloM!=""){m=0; tree->SetBranchAddress(CaloM,&m);}
      if(CaloA!=""){a=0; tree->SetBranchAddress(CaloA,&a);}
    
      if(TruePt!=""){tpt=0; tree->SetBranchAddress(TruePt,&tpt);}
      if(TrueEta!=""){teta=0; tree->SetBranchAddress(TrueEta,&teta);}
      if(TruePhi!=""){tphi=0; tree->SetBranchAddress(TruePhi,&tphi);}
      if(TrueE!=""){te=0; tree->SetBranchAddress(TrueE,&te);}  
      if(TrueM!=""){tm=0; tree->SetBranchAddress(TrueM,&tm);}
    
      if(Mu!="") tree->SetBranchAddress(Mu,&mu);
      if(NPV!="") tree->SetBranchAddress(NPV,&npv);
      //if(Rho!="") tree->SetBranchAddress(Rho,&rho);
      if(EventWeight!="") tree->SetBranchAddress(EventWeight,&w);
      
      if(OrigPt!=""){opt=0; tree->SetBranchAddress(OrigPt,&opt);}
      if(OrigEta!=""){oeta=0; tree->SetBranchAddress(OrigEta,&oeta);}
      if(OrigPhi!=""){ophi=0; tree->SetBranchAddress(OrigPhi,&ophi);}
      if(OrigE!=""){oe=0; tree->SetBranchAddress(OrigE,&oe);}  
      if(OrigM!=""){om=0; tree->SetBranchAddress(OrigM,&om);}
      
      OutputFileF->cd();
      //TTree* UncalibratedT, AreaT, AreaResidualT, AreaResidualJEST;
      std::cout<<"."<<t<<std::endl;
  
      
      TTree* event = new TTree("Event"+JetAlgos.at(t),"Event variables for "+JetAlgos.at(t));
      event->Branch("mu",&mu,"mu/F");
      event->Branch("npv",&npv,"npv/F");
      //event->Branch("rho",&rho,"rho/F");
      event->Branch("w",&w,"w/F");
    
      LoTree* UncalibratedL = 0;
      std::vector<LoTree*> AreaL;
      std::vector<LoTree*> AreaResidualL;
      std::vector<LoTree*> AreaResidualConstL;
      std::vector<LoTree*> AreaResidualJESL;
      std::vector<LoTree*> AreaResidualConstJESL;
      if(doUncalibrated) UncalibratedL = new LoTree(new TTree(JetAlgos.at(t)+"Uncalibrated",JetAlgos.at(t)+"Uncalibrated"));
      
      float rho[Rhos.size()];
      std::vector<float>* jetrho[JetRhos.size()];
      for(int i = 0 ; i < JetRhos.size(); ++i) jetrho[i] = 0;
      /*
      std::vector< std::vector<float> * >  jetrho;
      for(int i = 0; i < JetRhos.size(); ++i){
	std::vector<float>* jr;
	jetrho->push_back
      */
      std::vector<TAxis *> AbsEtaBinsAxis;
      std::vector<TAxis *> ConstTermAbsEtaBinsAxis;
      std::vector< std::vector<double> > NPVTerm;
      std::vector< std::vector<double> > MuTerm;
      std::vector< std::vector<double> > ConstTerm;
      std::vector< ScriptEtaJESCorrection * > JESCorrection;
      
      for(int i = 0; i < Rhos.size(); ++i){
	if(doArea) AreaL.push_back(new LoTree(new TTree(JetAlgos.at(t)+Tags.at(i)+"Area",JetAlgos.at(t)+Tags.at(i)+"Area")));    
	if(doAreaResidual) AreaResidualL.push_back(new LoTree(new TTree(JetAlgos.at(t)+Tags.at(i)+"AreaResidual",JetAlgos.at(t)+Tags.at(i)+"AreaResidual")));
	if(doAreaResidualConst) AreaResidualConstL.push_back(new LoTree(new TTree(JetAlgos.at(t)+Tags.at(i)+"AreaResidualConst",JetAlgos.at(t)+Tags.at(i)+"AreaResidualConst")));
	if(doAreaResidualJES) AreaResidualJESL.push_back(new LoTree(new TTree(JetAlgos.at(t)+Tags.at(i)+"AreaResidualJES",JetAlgos.at(t)+Tags.at(i)+"AreaResidualJES")));
	if(doAreaResidualConstJES){
	  //std::cout<<"#"<<i<<std::endl;
  	  AreaResidualConstJESL.push_back(new LoTree(new TTree(JetAlgos.at(t)+Tags.at(i)+"AreaResidualConstJES",JetAlgos.at(t)+Tags.at(i)+"AreaResidualConstJES")));
	  
	  //std::cout<<"#"<<i<<std::endl;
	  JESCorrection.push_back(new ScriptEtaJESCorrection(mainE,JetAlgos.at(t)));
	  //std::cout<<"#"<<i<<std::endl;
	  //
	  TString JetAlgoSansJets(JetAlgos.at(t));
	  JetAlgoSansJets.ReplaceAll("Jets","");
	  //JetAlgoSansJets.ReplaceAll("My","");
	  std::cout<<"Creating calib file from: "<< OutputFolder+"/jes/"+Rtags.at(r)+JetAlgos.at(t)+Tags.at(i)+"AreaResidualConst_EtaJES_consts.py " << std::endl;
	  system("ls");
	  system("source ./calib_fileHLLHC.sh "+OutputFolder+"/jes/"+Rtags.at(r)+JetAlgos.at(t)+Tags.at(i)+"AreaResidualConst_EtaJES_consts.py "+JetAlgoSansJets+" > "+CalibFolder+"/CalibrationFactors/"+InnerFolder+"/MC15_JES_"+InnerFolder+"_"+Rtags.at(r)+"_"+Tags.at(i)+"_"+DateString+"_"+JetAlgoSansJets+".config");
	  //
	  TEnv* jesE = new TEnv(CalibFolder+"/CalibrationFactors/"+InnerFolder+"/MC15_JES_"+InnerFolder+"_"+Rtags.at(r)+"_"+Tags.at(i)+"_"+DateString+"_"+JetAlgoSansJets+".config");
	  jesE->Print();
	  JESCorrection.at(i)->initializeTool(jesE);
	}
	if(Rhos.at(i)!="0") {
	  tree->SetBranchAddress(Rhos.at(i),&rho[i]);
	  event->Branch(Rhos.at(i),&rho[i],Rhos.at(i)+"/F");
	}
	else rho[i] = 0;
	
	TString ResidualOffsetName(Tags.at(i)+Version+Rtags.at(r)+JetAlgos.at(t));
	//std::cout<<"ResidualOffsetName: "<<ResidualOffsetName<<std::endl;
	std::vector<double> AbsEtaBins = VectorizeD(residualE->GetValue(ResidualOffsetName+".AbsEtaBins","0 4.5"));
	AbsEtaBinsAxis.push_back(new TAxis(AbsEtaBins.size()-1,&AbsEtaBins[0]));
	
	std::vector<double> ConstTermAbsEtaBins = VectorizeD(residualE->GetValue(ResidualOffsetName+".ConstTermAbsEtaBins","0 4.5"));
	ConstTermAbsEtaBinsAxis.push_back(new TAxis(ConstTermAbsEtaBins.size()-1,&ConstTermAbsEtaBins[0]));//std::cout<<"NPVTerm: "<<ResidualOffsetName+".NPVTerm."+JetAlgos.at(t);
	NPVTerm.push_back( VectorizeD(residualE->GetValue(ResidualOffsetName+".NPVTerm."+JetAlgos.at(t),"0 0 0 0 0"))); 
	//std::cout<<residualE->GetValue(ResidualOffsetName+".NPVTerm."+JetAlgos.at(t),"0 0 0 0 0")<<std::endl;
	MuTerm.push_back( VectorizeD(residualE->GetValue(ResidualOffsetName+".MuTerm."+JetAlgos.at(t),"0 0 0 0 0"))); 
	ConstTerm.push_back( VectorizeD(residualE->GetValue(ResidualOffsetName+".ConstTerm."+JetAlgos.at(t),"0 0 0 0 0"))); 
      }
      //
      std::vector<TAxis *> jetAbsEtaBinsAxis;
      std::vector<TAxis *> jetConstTermAbsEtaBinsAxis;
      std::vector< std::vector<double> > jetNPVTerm;
      std::vector< std::vector<double> > jetMuTerm;
      std::vector< std::vector<double> > jetConstTerm;
      //std::cout<<"!! "<<std::endl;
	
      for(int i = 0; i < JetRhos.size(); ++i){
	std::cout<<"...&&"<<i<<std::endl;
  
	if(doArea) AreaL.push_back(new LoTree(new TTree(JetAlgos.at(t)+JetTags.at(i)+"Area",JetAlgos.at(t)+JetTags.at(i)+"Area")));    
	if(doAreaResidual) AreaResidualL.push_back(new LoTree(new TTree(JetAlgos.at(t)+JetTags.at(i)+"AreaResidual",JetAlgos.at(t)+JetTags.at(i)+"AreaResidual")));
	if(doAreaResidualConst) AreaResidualConstL.push_back(new LoTree(new TTree(JetAlgos.at(t)+JetTags.at(i)+"AreaResidualConst",JetAlgos.at(t)+JetTags.at(i)+"AreaResidualConst")));
	if(doAreaResidualJES) AreaResidualJESL.push_back(new LoTree(new TTree(JetAlgos.at(t)+JetTags.at(i)+"AreaResidualJES",JetAlgos.at(t)+JetTags.at(i)+"AreaResidualJES")));
	if(doAreaResidualConstJES){
	  std::cout<<"#"<<i<<std::endl;
	  AreaResidualConstJESL.push_back(new LoTree(new TTree(JetAlgos.at(t)+JetTags.at(i)+"AreaResidualConstJES",JetAlgos.at(t)+JetTags.at(i)+"AreaResidualConstJES")));
	  
	  std::cout<<"#"<<i<<std::endl;
	  JESCorrection.push_back(new ScriptEtaJESCorrection(mainE,JetAlgos.at(t)));
	  std::cout<<"#"<<i<<std::endl;
	  //
	  TString JetAlgoSansJets(JetAlgos.at(t));
	  JetAlgoSansJets.ReplaceAll("Jets","");
	  //JetAlgoSansJets.ReplaceAll("My","");
	  std::cout<<"Creating calib file from: "<< OutputFolder+"/jes/"+Rtags.at(r)+JetAlgos.at(t)+Tags.at(i)+"AreaResidualConst_EtaJES_consts.py " << std::endl;
	  system("ls");
	  std::cout<<"#"<<i<<std::endl;
	  system("source ./calib_fileHLLHC.sh "+OutputFolder+"/jes/"+Rtags.at(r)+JetAlgos.at(t)+JetTags.at(i)+"AreaResidualConst_EtaJES_consts.py "+JetAlgoSansJets+" > "+CalibFolder+"/CalibrationFactors/"+InnerFolder+"/MC15_JES_"+InnerFolder+"_"+Rtags.at(r)+"_"+JetTags.at(i)+"_"+DateString+"_"+JetAlgoSansJets+".config");
	  std::cout<<"#"<<i<<std::endl;
	  //
	  TEnv* jesE = new TEnv(CalibFolder+"/CalibrationFactors/"+InnerFolder+"/MC15_JES_"+InnerFolder+"_"+Rtags.at(r)+"_"+JetTags.at(i)+"_"+DateString+"_"+JetAlgoSansJets+".config");
	  jesE->Print();
	  JESCorrection.at(i)->initializeTool(jesE);
	  std::cout<<"#"<<i<<std::endl;
	
	}
	std::cout<<"###"<<i<<std::endl;
	  
	tree->SetBranchAddress(JetRhos.at(i),&jetrho[i]);
	//event->Branch(JetRhos.at(i),&jetrho[i],JetRhos.at(i)+"/F");
	
	//std::cout<<"!!! "<<std::endl;
	
	TString ResidualOffsetName(JetTags.at(i)+Version+Rtags.at(r)+JetAlgos.at(t));
	//std::cout<<"ResidualOffsetName: "<<ResidualOffsetName<<std::endl;
	std::vector<double> AbsEtaBins = VectorizeD(residualE->GetValue(ResidualOffsetName+".AbsEtaBins","0 4.5"));
	jetAbsEtaBinsAxis.push_back(new TAxis(AbsEtaBins.size()-1,&AbsEtaBins[0]));
	
	std::vector<double> ConstTermAbsEtaBins = VectorizeD(residualE->GetValue(ResidualOffsetName+".ConstTermAbsEtaBins","0 4.5"));
	jetConstTermAbsEtaBinsAxis.push_back(new TAxis(ConstTermAbsEtaBins.size()-1,&ConstTermAbsEtaBins[0]));
	
	//std::cout<<"!!!! "<<std::endl;
	
	jetNPVTerm.push_back( VectorizeD(residualE->GetValue(ResidualOffsetName+".NPVTerm."+JetAlgos.at(t),"0 0 0 0 0"))); 
	//std::cout<<"!!!!! "<<std::endl;
	jetMuTerm.push_back( VectorizeD(residualE->GetValue(ResidualOffsetName+".MuTerm."+JetAlgos.at(t),"0 0 0 0 0"))); 
	jetConstTerm.push_back( VectorizeD(residualE->GetValue(ResidualOffsetName+".ConstTerm."+JetAlgos.at(t),"0 0 0 0 0"))); 
	std::cout<<"!!!!!! "<<std::endl;
	
      }
      //



      for(Long64_t i = 0; i < Nentry; ++i){
	//std::cout<<"...."<<i<<std::endl;
	  
	tree->GetEntry(i+Offset);
	//std::cout<<"Nentry: "<<i<<" "<<e->size()<<std::endl;
	for(int j = 0; j < e->size(); ++j){
	  //std::cout<<"....."<<j<<std::endl;
  
	  TLorentzVector jet_calo;
	  TLorentzVector jet_orig;
	  TLorentzVector jet_area;
	  TLorentzVector jet_residual;
	  TLorentzVector jet_JES;
	
	  jet_calo.SetPtEtaPhiE(pt->at(j),eta->at(j),phi->at(j),e->at(j));
	  if(OrigE!="") jet_orig.SetPtEtaPhiE(opt->at(j),oeta->at(j),ophi->at(j),oe->at(j));
	
	  if(doUncalibrated){
	    if(CaloPt!="") UncalibratedL->pt.push_back(pt->at(j));
	    if(CaloEta!="") UncalibratedL->eta.push_back(eta->at(j));
	    if(CaloPhi!="") UncalibratedL->phi.push_back(phi->at(j));
	    if(CaloE!="") UncalibratedL->e.push_back(e->at(j));
	    if(CaloM!="") UncalibratedL->m.push_back(m->at(j));

	    if(TruePt!="") UncalibratedL->tpt.push_back(tpt->at(j));
	    if(TrueEta!="") UncalibratedL->teta.push_back(teta->at(j));
	    if(TruePhi!="") UncalibratedL->tphi.push_back(tphi->at(j));
	    if(TrueE!="") UncalibratedL->te.push_back(te->at(j));
	    if(TrueM!="") UncalibratedL->tm.push_back(tm->at(j));

	    if(EventWeight!="") UncalibratedL->EW = w;
	  }
	  
	  for(int r = 0; r < Rhos.size(); ++r){
	    //std::cout<<"......"<<r<<std::endl;
  
	    if(doArea){//add rho
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),rho[r],0.,true);
	      AreaL.at(r)->pt.push_back(area_pt);
	      AreaL.at(r)->eta.push_back(area_eta);
	      AreaL.at(r)->phi.push_back(area_phi);
	      AreaL.at(r)->e.push_back(area_e);
	      AreaL.at(r)->m.push_back(jet_area.M());

	      if(TruePt!="") AreaL.at(r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaL.at(r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaL.at(r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaL.at(r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaL.at(r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaL.at(r)->EW = w;
	    }
	    //std::cout<<"*....."<<r<<std::endl;
  	    if(doAreaResidual){//add rho
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      //std::cout<<": "<< MuTerm.at(r) << " " << NPVTerm.at(r)<<std::endl;
	      //AbsEtaBinsAxis.at(r)->Print();
	      float residualoffset = GetResidualOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),mu,npv,DefaultMuRef,DefaultNPVRef,MuTerm.at(r),NPVTerm.at(r),AbsEtaBinsAxis.at(r));
	      //if(i> 2000 && i < 2100) std::cout<<fabs(jet_calo.Eta())<<" "<<mu<<" "<<npv<<" "<<DefaultMuRef<<" "<<DefaultNPVRef<<" "<<MuTerm.at(1)<<" "<<NPVTerm.at(1)<<" "<<residualoffset<<std::endl;
	      //std::cout<<"residualoffset: " << residualoffset << std::endl;
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),rho[r],residualoffset,true);
	      AreaResidualL.at(r)->pt.push_back(area_pt);
	      AreaResidualL.at(r)->eta.push_back(area_eta);
	      AreaResidualL.at(r)->phi.push_back(area_phi);
	      AreaResidualL.at(r)->e.push_back(area_e);
	      AreaResidualL.at(r)->m.push_back(jet_area.M());

	      if(TruePt!="") AreaResidualL.at(r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualL.at(r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualL.at(r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualL.at(r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualL.at(r)->tm.push_back(tm->at(j));
	      
	      if(EventWeight!="") AreaResidualL.at(r)->EW = w;
	    }
	    //std::cout<<"*....."<<r<<std::endl;
  	    if(doAreaResidualConst){//add rho
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      //std::cout<<": "<< MuTerm.at(r) << " " << NPVTerm.at(r)<<std::endl;
	      //AbsEtaBinsAxis.at(r)->Print();
	      float residualoffset = GetResidualOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),mu,npv,DefaultMuRef,DefaultNPVRef,MuTerm.at(r),NPVTerm.at(r),AbsEtaBinsAxis.at(r));
	      float constoffset = GetConstOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),ConstTerm.at(r),ConstTermAbsEtaBinsAxis.at(r));
	      //if(i> 2000 && i < 2100) std::cout<<fabs(jet_calo.Eta())<<" "<<mu<<" "<<npv<<" "<<DefaultMuRef<<" "<<DefaultNPVRef<<" "<<MuTerm.at(1)<<" "<<NPVTerm.at(1)<<" "<<residualoffset<<std::endl;
	      //std::cout<<"residualoffset: " << residualoffset << std::endl;
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),rho[r],residualoffset+constoffset,true);
	      AreaResidualConstL.at(r)->pt.push_back(area_pt);
	      AreaResidualConstL.at(r)->eta.push_back(area_eta);
	      AreaResidualConstL.at(r)->phi.push_back(area_phi);
	      AreaResidualConstL.at(r)->e.push_back(area_e);
	      AreaResidualConstL.at(r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaResidualConstL.at(r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualConstL.at(r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualConstL.at(r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualConstL.at(r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualConstL.at(r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaResidualConstL.at(r)->EW = w;
	      
	    }
	    //std::cout<<"*.....*"<<r<<std::endl;
 
	    if(doAreaResidualConstJES){//add rho
	      //std::cout<<"*.....*"<<r<<std::endl;
	      
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      float jes_pt, jes_eta, jes_phi, jes_e, jes_m;
	      float residualoffset = GetResidualOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),mu,npv,DefaultMuRef,DefaultNPVRef,MuTerm.at(r),NPVTerm.at(r),AbsEtaBinsAxis.at(r));
	      float constoffset = GetConstOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),ConstTerm.at(r),ConstTermAbsEtaBinsAxis.at(r));
	      //std::cout<<"*.....*"<<r<<std::endl;
  	      
	      //JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),rho[r],residualoffset+constoffset,true);
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),rho[r],residualoffset+constoffset,true);
	      //std::cout<<"*.....*"<<r<<std::endl;
  	      
	      //JESCorrection(); //VERIF
	      //std::cout<<".....open"<<std::endl;

	      //std::cout<<".....close"<<" "<<area_pt<<" "<<area_eta<<" "<<area_phi<<" "<<area_e<<" "<<area_m<<" "<<std::endl;	      
	      JESCorrection.at(r)->calibrateImpl(area_pt, area_eta, area_phi, area_e, area_m, jes_pt, jes_eta, jes_phi, jes_e, jes_m);
	      //std::cout<<".....close"<<" "<<jes_pt<<" "<<jes_eta<<" "<<jes_phi<<" "<<jes_e<<" "<<jes_m<<" "<<std::endl;

	      AreaResidualConstJESL.at(r)->pt.push_back(jes_pt);
	      AreaResidualConstJESL.at(r)->eta.push_back(jes_eta);
	      AreaResidualConstJESL.at(r)->phi.push_back(jes_phi);
	      AreaResidualConstJESL.at(r)->e.push_back(jes_e);
	      AreaResidualConstJESL.at(r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaResidualConstJESL.at(r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualConstJESL.at(r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualConstJESL.at(r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualConstJESL.at(r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualConstJESL.at(r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaResidualConstJESL.at(r)->EW = w;
	    
	      /*
	      AreaResidualConstJESL.at(Rhos.size()+r)->pt.push_back(area_pt);
	      AreaResidualConstJESL.at(Rhos.size()+r)->eta.push_back(area_eta);
	      AreaResidualConstJESL.at(Rhos.size()+r)->phi.push_back(area_phi);
	      AreaResidualConstJESL.at(Rhos.size()+r)->e.push_back(area_e);
	      AreaResidualConstJESL.at(Rhos.size()+r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaResidualConstJESL.at(Rhos.size()+r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualConstJESL.at(Rhos.size()+r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualConstJESL.at(Rhos.size()+r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualConstJESL.at(Rhos.size()+r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualConstJESL.at(Rhos.size()+r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaResidualConstJESL.at(Rhos.size()+r)->EW = w;
	      */
	    }
	  }
	  for(int r = 0; r < JetRhos.size(); ++r){
	    //std::cout<<"......."<<r<<std::endl;
  
	    if(doArea){//add rho
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),jetrho[r]->at(j),0.,true);
	      AreaL.at(Rhos.size()+r)->pt.push_back(area_pt);
	      AreaL.at(Rhos.size()+r)->eta.push_back(area_eta);
	      AreaL.at(Rhos.size()+r)->phi.push_back(area_phi);
	      AreaL.at(Rhos.size()+r)->e.push_back(area_e);
	      AreaL.at(Rhos.size()+r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaL.at(Rhos.size()+r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaL.at(Rhos.size()+r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaL.at(Rhos.size()+r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaL.at(Rhos.size()+r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaL.at(Rhos.size()+r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaL.at(Rhos.size()+r)->EW = w;
	  
	    }
	    if(doAreaResidual){//add rho
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      float residualoffset = GetResidualOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),mu,npv,DefaultMuRef,DefaultNPVRef,jetMuTerm.at(r),jetNPVTerm.at(r),jetAbsEtaBinsAxis.at(r));
	      //if(i> 2000 && i < 2100) std::cout<<fabs(jet_calo.Eta())<<" "<<mu<<" "<<npv<<" "<<DefaultMuRef<<" "<<DefaultNPVRef<<" "<<MuTerm.at(1)<<" "<<NPVTerm.at(1)<<" "<<residualoffset<<std::endl;
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),jetrho[r]->at(j),residualoffset,true);
	      AreaResidualL.at(Rhos.size()+r)->pt.push_back(area_pt);
	      AreaResidualL.at(Rhos.size()+r)->eta.push_back(area_eta);
	      AreaResidualL.at(Rhos.size()+r)->phi.push_back(area_phi);
	      AreaResidualL.at(Rhos.size()+r)->e.push_back(area_e);
	      AreaResidualL.at(Rhos.size()+r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaResidualL.at(Rhos.size()+r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualL.at(Rhos.size()+r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualL.at(Rhos.size()+r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualL.at(Rhos.size()+r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualL.at(Rhos.size()+r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaResidualL.at(Rhos.size()+r)->EW = w;
	      
	    }
	    if(doAreaResidualConst){//add rho
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      float residualoffset = GetResidualOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),mu,npv,DefaultMuRef,DefaultNPVRef,jetMuTerm.at(r),jetNPVTerm.at(r),jetAbsEtaBinsAxis.at(r));
	      float constoffset = GetConstOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),jetConstTerm.at(r),jetConstTermAbsEtaBinsAxis.at(r));
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),jetrho[r]->at(j),residualoffset+constoffset,true);
	      AreaResidualConstL.at(Rhos.size()+r)->pt.push_back(area_pt);
	      AreaResidualConstL.at(Rhos.size()+r)->eta.push_back(area_eta);
	      AreaResidualConstL.at(Rhos.size()+r)->phi.push_back(area_phi);
	      AreaResidualConstL.at(Rhos.size()+r)->e.push_back(area_e);
	      AreaResidualConstL.at(Rhos.size()+r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaResidualConstL.at(Rhos.size()+r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualConstL.at(Rhos.size()+r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualConstL.at(Rhos.size()+r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualConstL.at(Rhos.size()+r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualConstL.at(Rhos.size()+r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaResidualConstL.at(Rhos.size()+r)->EW = w;
	  
	    }
	    if(doAreaResidualConstJES){//add rho
	      //std::cout<<"*.....*"<<r<<std::endl;
	      
	      float area_pt, area_eta, area_phi, area_e, area_m;
	      float jes_pt, jes_eta, jes_phi, jes_e, jes_m;
	      std::cout<<"."<<std::endl;
	      float residualoffset = GetResidualOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),mu,npv,DefaultMuRef,DefaultNPVRef,MuTerm.at(r),NPVTerm.at(r),AbsEtaBinsAxis.at(r));
	      float constoffset = GetConstOffset(useOriginCorrection ? fabs(jet_orig.Eta()) : fabs(jet_calo.Eta()),ConstTerm.at(r),ConstTermAbsEtaBinsAxis.at(r));
	      //std::cout<<"*.....*"<<r<<std::endl;
  	      std::cout<<".."<<std::endl;
	      
	      //JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),rho[r],residualoffset+constoffset,true);
	      JetPileupCorrection(area_pt,area_eta,area_phi,area_e,area_m,jet_calo,useOriginCorrection ? jet_orig : jet_calo,a->at(j),jetrho[r]->at(j),residualoffset+constoffset,true);
	      //std::cout<<"*.....*"<<r<<std::endl;
  	      std::cout<<"...FF"<<std::endl;
	      
	      //JESCorrection(); //VERIF
	      //std::cout<<".....open"<<std::endl;

	      std::cout<<".....close"<<" "<<area_pt<<" "<<area_eta<<" "<<area_phi<<" "<<area_e<<" "<<area_m<<" "<<std::endl;	      
	      JESCorrection.at(r)->calibrateImpl(area_pt, area_eta, area_phi, area_e, area_m, jes_pt, jes_eta, jes_phi, jes_e, jes_m);
	      std::cout<<".....close"<<" "<<jes_pt<<" "<<jes_eta<<" "<<jes_phi<<" "<<jes_e<<" "<<jes_m<<" "<<std::endl;
	      
	      AreaResidualConstJESL.at(Rhos.size()+r)->pt.push_back(jes_pt);
	      AreaResidualConstJESL.at(Rhos.size()+r)->eta.push_back(jes_eta);
	      AreaResidualConstJESL.at(Rhos.size()+r)->phi.push_back(jes_phi);
	      AreaResidualConstJESL.at(Rhos.size()+r)->e.push_back(jes_e);
	      AreaResidualConstJESL.at(Rhos.size()+r)->m.push_back(jet_area.M());
	      
	      if(TruePt!="") AreaResidualConstJESL.at(Rhos.size()+r)->tpt.push_back(tpt->at(j));
	      if(TrueEta!="") AreaResidualConstJESL.at(Rhos.size()+r)->teta.push_back(teta->at(j));
	      if(TruePhi!="") AreaResidualConstJESL.at(Rhos.size()+r)->tphi.push_back(tphi->at(j));
	      if(TrueE!="") AreaResidualConstJESL.at(Rhos.size()+r)->te.push_back(te->at(j));
	      if(TrueM!="") AreaResidualConstJESL.at(Rhos.size()+r)->tm.push_back(tm->at(j));
	      if(EventWeight!="") AreaResidualConstJESL.at(Rhos.size()+r)->EW = w;
	      
	    }

	  }
	}
	//std::cout<<"size uncalibrated: " << UncalibratedL->e.size() <<std::endl;
	if(doUncalibrated){UncalibratedL->tree()->Fill(); UncalibratedL->clear();}
	for(int r = 0 ; r < Rhos.size()+JetRhos.size(); ++r){
	  if(doArea){AreaL.at(r)->tree()->Fill(); AreaL.at(r)->clear();}
	  if(doAreaResidual){AreaResidualL.at(r)->tree()->Fill(); AreaResidualL.at(r)->clear();}
	  if(doAreaResidualConst){AreaResidualConstL.at(r)->tree()->Fill(); AreaResidualConstL.at(r)->clear();}
	  if(doAreaResidualJES){AreaResidualJESL.at(r)->tree()->Fill(); AreaResidualJESL.at(r)->clear();}
	  if(doAreaResidualConstJES){AreaResidualConstJESL.at(r)->tree()->Fill(); AreaResidualConstJESL.at(r)->clear();}
	}
	event->Fill();
      }
      if(doUncalibrated) UncalibratedL->tree()->Write();
      for(int r = 0 ; r < Rhos.size()+JetRhos.size(); ++r){
	if(doArea) AreaL.at(r)->tree()->Write();
	if(doAreaResidual) AreaResidualL.at(r)->tree()->Write();
	if(doAreaResidualConst) AreaResidualConstL.at(r)->tree()->Write();
	if(doAreaResidualJES) AreaResidualJESL.at(r)->tree()->Write();
	if(doAreaResidualConstJES) AreaResidualConstJESL.at(r)->tree()->Write();
      }
      
      event->Write();
      //
    
      
    }
    
    OutputFileF->Close();
    InputFileF->Close();
    
    delete residualE;
  }
  
  //save the Tree heade; the file will be automatically closed
  //when going out of the function scope
}