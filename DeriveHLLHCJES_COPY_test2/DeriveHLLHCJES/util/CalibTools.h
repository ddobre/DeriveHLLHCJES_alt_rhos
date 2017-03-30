//Steve Alkire
//16.01.01

#ifndef CALIBTOOLS_H
#define CALIBTOOLS_H

#include <iostream>
#include <stdio.h>
#include "TCut.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TList.h"
#include "TEventList.h"
#include "TROOT.h"
#include "TKey.h"
#include "TFile.h"
#include "TSystem.h"
#include "TColor.h"
#include "TLegend.h"
#include "TF2.h"
#include "TFitResultPtr.h"
#include "TFitResult.h"
#include "TH3F.h"
#include "TLatex.h"

#define FOR_EACH(list,dostuff) for(int IiI = 0; IiI < list->GetSize();++IiI){dostuff}
#define RETURN_LIST_OF(list, things) TList *LiSt = new TList;		\
  for(int IiI = 0; IiI < list->GetSize();++IiI){LiSt->Add(things);}	\
  return LiSt;
#define UNIQUE_ID "_"+TString::Format("%i",IiI)

TString Devectorize(std::vector<TString> strs, TString sep = " ")
{
  if(strs.size()==0) return TString("");
  TString result(strs.at(0));
  for(int i = 1; i < strs.size(); ++i){
    result+=(sep+strs.at(i));
  }
  return result;
}

TString Devectorize(std::vector<double> vals, TString sep = " ")
{

  if(vals.size()==0) return TString("");
  TString result(TString::Format("%4.3f",vals.at(0)));
  for(int i = 1; i < vals.size(); ++i){
    result+=(sep+TString::Format("%4.3f",vals.at(i)));
  }
  return result;
}

std::vector<TString> Vectorize(TString str, TString sep = " ")
{
  std::vector<TString> result;
  TObjArray* tokens = str.Tokenize(sep);
  TIter istr(tokens);
  while (TObjString* os=(TObjString*)istr())
    result.push_back(os->GetString());
  delete tokens;
  return result;
}

std::vector<double> VectorizeD(TString str, TString sep = " ")
{
  std::vector<double> result;
  TObjArray* tokens = str.Tokenize(sep);
  TIter istr(tokens);
  while (TObjString* os=(TObjString*)istr())
    result.push_back(atof(os->GetString()));
  delete tokens;
  return result;
}


class PTools{
 public:
  static void CombineSquare(TH2* hout, TH2* h1, TH2* h2){
    for(int i = 1; i <= hout->GetXaxis()->GetNbins(); ++i){
      for(int j = 1; j <= hout->GetYaxis()->GetNbins(); ++j){
	hout->SetBinContent(i,j,sqrt(h1->Interpolate(hout->GetXaxis()->GetBinCenter(i),hout->GetYaxis()->GetBinCenter(j))*h1->Interpolate(hout->GetXaxis()->GetBinCenter(i),hout->GetYaxis()->GetBinCenter(j)) + h2->Interpolate(hout->GetXaxis()->GetBinCenter(i),hout->GetYaxis()->GetBinCenter(j))*h2->Interpolate(hout->GetXaxis()->GetBinCenter(i),hout->GetYaxis()->GetBinCenter(j)) ) );
      }
    }
  }

  static void CombineSquare(TH2* hout, TList* h_list){
    for(int i = 1; i <= hout->GetXaxis()->GetNbins(); ++i){
      for(int j = 1; j <= hout->GetYaxis()->GetNbins(); ++j){
	double squareterm = 0;
	FOR_EACH(h_list,squareterm+=(((TH2*)h_list->At(IiI))->Interpolate(hout->GetXaxis()->GetBinCenter(i),hout->GetYaxis()->GetBinCenter(j))*((TH2*)h_list->At(IiI))->Interpolate(hout->GetXaxis()->GetBinCenter(i),hout->GetYaxis()->GetBinCenter(j)));)
	  std::cout<<" i " << i<<" j " << j<<"square " << squareterm<<std::endl;
	hout->SetBinContent(i,j,sqrt(squareterm));
      }
    }
  }

  #define MAX_SPACE .6
  #define MIN_SPACE .1
  static Double_t max_space;
  static Double_t min_space;
  static bool min_zero;
  static void EstimateYaxis(TList* th1_list, int list_depth=1){
    double min = 1E+20;
    double max = -1E+20;
    if(list_depth<=1){
    FOR_EACH(th1_list, if(((TH1*)th1_list->At(IiI))->GetMaximum() > max) max = ((TH1*)th1_list->At(IiI))->GetMaximum();
	     if(((TH1*)th1_list->At(IiI))->GetMinimum() < min) min = ((TH1*)th1_list->At(IiI))->GetMinimum();)
    std::cout<<"min: "<<min-min_space*(max-min)<<std::endl;
    std::cout<<"max: "<<max+max_space*(max-min)<<std::endl;
    FOR_EACH(th1_list, ((TH1*)th1_list->At(IiI))->GetYaxis()->SetRangeUser(min_zero ? 0.0 : min-min_space*(max-min),max+max_space*(max-min)); )
    }else{
      FOR_EACH(th1_list, EstimateYaxis(((TList*)th1_list->At(IiI)),list_depth-1);)
    }
  }

  static void SetYRange(TList* th1_list, float low, float high, int list_depth=1){
    if(list_depth<=1){
      FOR_EACH(th1_list, ((TH1*)th1_list->At(IiI))->GetYaxis()->SetRangeUser(low,high); )
	}else{
      FOR_EACH(th1_list, SetYRange((TList*)th1_list->At(IiI),low,high,list_depth-1);)
      }
  }

  static void Normalize(TList* th1_list, int list_depth=1,Option_t* option = "width"){
    if(list_depth<=1){
      FOR_EACH(th1_list, if(((TH1*)th1_list->At(IiI))->Integral(option)!=0) ((TH1*)th1_list->At(IiI))->Scale(1./((TH1*)th1_list->At(IiI))->Integral(option));)
    }else{
      FOR_EACH(th1_list, Normalize(((TList*)th1_list->At(IiI)),list_depth-1,option);)
    }
  }

  static void Pause(const char* file){
    TDirectory *d = gDirectory;
    TFile *f = new TFile(file,"recreate");
    TDirectory *n = gDirectory;
    TList* l= d->GetList();
    TIter iter(l);
    TObject* obj= 0;
    int list_enumerator = 0;
    while( obj= iter.Next() ) {
      if( obj->InheritsFrom("TH1")){
	((TH1*)obj)->SetDirectory(n);
      }
    }
    f->Write();
    f->Close();
  }

  #define CHISQUARE_UNC 0 //1
  static float GetParError(TF1* fit, int par){
    if(CHISQUARE_UNC){
      std::cout<<"Doing chi-sq unc."<<std::endl;
      float X2PDF = fit->GetChisquare()/(float)fit->GetNDF();
      std::cout<<"X2PDF: "<<X2PDF<<std::endl;
      if(X2PDF > 1.) return fit->GetParError(par)*sqrt(X2PDF);
    }
    return fit->GetParError(par);
  }

  //makes all the value of bin
  static TH1D* Flatten(TH1D* h, int bin, bool noerrors = false){
      TH1D* h_flat = (TH1D*)h->Clone(TString(h->GetName())+"_flat");
      Int_t nbins = h->GetXaxis()->GetNbins();
      TString name(h->GetName());

      double val = h->GetBinContent(bin);
      double error = noerrors ? 0.0 : h->GetBinError(bin);
      for(int i = 1; i <= nbins; ++i){
	h_flat->SetBinContent(i, val);
	h_flat->SetBinError(i, error);
      }
      return h_flat;
  }

  static TList* Flatten(TList* h_list, int bin,int list_depth = 1, bool noerrors = false){
    if(list_depth<=1){
      RETURN_LIST_OF(h_list, Flatten((TH1D*)h_list->At(IiI),bin,noerrors))
    }else{
      RETURN_LIST_OF(h_list, Flatten((TList*)h_list->At(IiI),bin,list_depth-1,noerrors))
    }
  }

  static TH1* Divide(TH1* numerator,TH1* denominator){
    TH1* h_div = (TH1D*)numerator->Clone(TString(numerator->GetName())+"_d_"+TString(denominator->GetName()));
    h_div->Divide(denominator);
    return h_div;
  }

  static TList* Divide(TList* numerator, TList* denominator, int list_depth =1){
    if(list_depth<=1){
      RETURN_LIST_OF(numerator, Divide((TH1*)numerator->At(IiI),(TH1*)denominator->At(IiI)))
    }else{
      RETURN_LIST_OF(numerator, Divide((TList*)numerator->At(IiI),(TList*)denominator->At(IiI), list_depth-1))
    }
  }

  #define SKIP_SATURATED_PARAMS 1
  static TList* PlotFitParams(TList* fit_list, std::vector<float>& x_bounds, int nPar){

    TList* param_list = new TList;
    float xb[x_bounds.size()];
    for(unsigned i = 0 ; i < x_bounds.size(); ++i) xb[i] = x_bounds.at(i);
    for(int i = 0 ; i < nPar; ++i){
      param_list->Add(new TH1F(TString(((TF1*)fit_list->At(0))->GetName())+"_par"+TString::Format("%i",i),TString(((TF1*)fit_list->At(0))->GetName())+"_par"+TString::Format("%i",i),x_bounds.size()-1, &xb[0]));

    }

    for(int IiI =0; IiI < fit_list->GetSize(); ++IiI){
      bool saturated_parameter = false;
      for(int i = 0 ; i < nPar; ++i){
	Double_t min_sat = -999.9;
	Double_t max_sat = -999.9;
	((TF1*)fit_list->At(IiI))->GetParLimits(i,min_sat,max_sat);
	Double_t range = max_sat - min_sat;
	if(SKIP_SATURATED_PARAMS && min_sat != -999.9 && ((TF1*)fit_list->At(IiI))->GetParameter(i) > max_sat - range*0.05 && ((TF1*)fit_list->At(IiI))->GetParameter(i) < min_sat + range*.05){
	  std::cout<< "Skipping saturated param: " << ((TF1*)fit_list->At(IiI))->GetParameter(i) << std::endl;
	  saturated_parameter = true;
	}
      }

      if(((TF1*)fit_list->At(IiI))->GetParameter(0)!=-999.9 && !saturated_parameter){
	for(int i = 0 ; i < nPar; ++i){
	  ((TH1F*)param_list->At(i))->SetBinContent(IiI+1,((TF1*)fit_list->At(IiI))->GetParameter(i));
	  ((TH1F*)param_list->At(i))->SetBinError(IiI+1,GetParError(((TF1*)fit_list->At(IiI)),i));
	}
      }
    }
    return param_list;
  }

  static TList* PlotMultiFitParams(TList* fit_list, std::vector<float>& x_bounds, int nPar, int list_depth = 1){
    TList* param_list = new TList;
    float xb[x_bounds.size()];
    for(unsigned i = 0 ; i < x_bounds.size(); ++i) xb[i] = x_bounds.at(i);
    std::cout<<"PlotMultiFitParams returns list of depth"<<list_depth<<std::endl;
    if(list_depth<=1){
      for(int i = 0 ; i < nPar; ++i){
	TList* ilist = new TList; param_list->Add(ilist);
	for(int j = 0; j < fit_list->GetSize();++j){
	  TList* jfit_list = (TList*)fit_list->At(j);
	  TH1* h = new TH1F(TString(((TF1*)jfit_list->At(0))->GetName())+"_par"+TString::Format("%i",i),TString(((TF1*)jfit_list->At(0))->GetName())+"_par"+TString::Format("%i",i),x_bounds.size()-1, &xb[0]);
	  ilist->Add(h);

	  FOR_EACH(jfit_list,
		   bool saturated_parameter = false;
		   Double_t min_sat = -999.9;
		   Double_t max_sat = -999.9;
		   ((TF1*)jfit_list->At(IiI))->GetParLimits(i,min_sat,max_sat);
		   Double_t range = max_sat - min_sat;
		   if(SKIP_SATURATED_PARAMS && min_sat != -999.9 && ((TF1*)jfit_list->At(IiI))->GetParameter(i) > max_sat - range*0.05 && ((TF1*)jfit_list->At(IiI))->GetParameter(i) < min_sat + range*.05){
		     std::cout<< "Skipping saturated param: " << ((TF1*)jfit_list->At(IiI))->GetParameter(i) << std::endl;
		     saturated_parameter = true;
		   }

		   if(((TF1*)jfit_list->At(IiI))->GetParameter(0)!=-999.9 && !saturated_parameter){
		     h->SetBinContent(IiI+1,((TF1*)jfit_list->At(IiI))->GetParameter(i));
		     h->SetBinError(IiI+1,((TF1*)jfit_list->At(IiI))->GetParError(i));
		   }
		   )
	    }
      }
    }else if(list_depth==2){
      for(int i = 0 ; i < nPar; ++i){
	TList* ilist = new TList; param_list->Add(ilist);
	for(int j = 0; j < fit_list->GetSize();++j){
	  TList* jlist = new TList; ilist->Add(jlist);
	  TList* jfit_list = (TList*)fit_list->At(j);
	  for(int k = 0; k < jfit_list->GetSize();++k){
	    TList* kfit_list = (TList*)jfit_list->At(k);

	    TH1* h = new TH1F(TString(((TF1*)kfit_list->At(0))->GetName())+"_par"+TString::Format("%i",i),TString(((TF1*)kfit_list->At(0))->GetName())+"_par"+TString::Format("%i",i),x_bounds.size()-1, &xb[0]);
	    jlist->Add(h);
	    FOR_EACH(kfit_list,h->SetBinContent(IiI+1,((TF1*)kfit_list->At(IiI))->GetParameter(i));
		     h->SetBinError(IiI+1,((TF1*)kfit_list->At(IiI))->GetParError(i));)
	  }
	}
      }
    }
    return param_list;
  }

  static TString num_display;

  static TList* CutList(std::vector<float>& cut_bounds, const char* cut_name, TString num_disp = num_display){
    TList* cut_list = new TList;
    for(unsigned i = 1 ; i < cut_bounds.size(); ++i){
      cut_list->Add( new TCut(TString(cut_name) + " >= " + TString::Format(num_disp,cut_bounds.at(i-1)) + " && " + TString(cut_name) + " < " + TString::Format(num_disp,cut_bounds.at(i))));

    }
    return cut_list;
  }

  static TList* GausList(std::vector<float>& cut_bounds, const char* cut_name, TString num_disp = num_display){
    TList* cut_list = new TList;
    for(unsigned i = 1 ; i < cut_bounds.size(); ++i){
      float gaus_mean = (cut_bounds.at(i-1)+cut_bounds.at(i))/2.;
      float gaus_std = (cut_bounds.at(i) - cut_bounds.at(i-1))/2.;
      cut_list->Add( new TObjString("TMath::Gaus("+TString(cut_name) + ","+ TString::Format(num_disp,gaus_mean) + "," + TString::Format(num_disp,gaus_std)+")"));
    }
    return cut_list;
  }


  static TList* CutList(TList* cut_bounds, const char* cut_name){
    TList* cut_list = new TList;
    for(unsigned i = 1 ; i < cut_bounds->GetSize(); ++i){
      cut_list->Add( new TCut(TString(cut_name) + " >= " + ((TObjString*)cut_bounds->At(i-1))->GetString() + " && " + TString(cut_name) + " < " + ((TObjString*)cut_bounds->At(i))->GetString()));
    }
    return cut_list;
  }

  static TList* EqualList(std::vector<float>& cut_bounds, const char* cut_name, TString num_disp = num_display){
    TList* cut_list = new TList;
    for(unsigned i = 0 ; i < cut_bounds.size(); ++i){
      cut_list->Add( new TCut(TString(cut_name) + " == " + TString::Format(num_disp,cut_bounds.at(i))));

    }
    return cut_list;
  }



  static TList* Cut2Str(TList* cut_list){
    RETURN_LIST_OF(cut_list, new TObjString(((TCut*)cut_list->At(IiI))->GetTitle()))
  }

  static TString AddCondition(const char* selection, const char* condition){
    TString sel(selection);
    TString cond(condition);
    sel.ReplaceAll(" ","");
    if(sel.EndsWith(")")){
      sel.Insert(sel.Length()-1, TString("&&")+cond);
    }else{
      sel+=(TString("&&")+cond);
    }
    return sel;
  }




  static TString InsertVarExp(const char* varexp, const char* addition){
    TString hist(varexp);
    hist.Insert(hist.SubString(">>").Start(),addition);
    return hist;
  }

  static TString HistNameFromVarExp(const char* varexp){
    TString hist(varexp);
    hist.Replace(0,hist.SubString(">>").Start()+2,"");
    hist.Remove(hist.SubString("(").Start(),hist.Length()-hist.SubString("(").Start());
    return hist;
  }

  static TString AddSuffixToHistName(const char* varexp,const char* suffix){
    TString hist(varexp);
    hist.Insert(hist.SubString(">>").Start()+2+HistNameFromVarExp(varexp).Length(),suffix);
    return hist;
  }

  static TString PrettyCutFormat(TString cut){
    TString out(cut);
    TString out2(out(out.SubString(">=").Start()+2, out.Length()));
    out2 = out2.ReplaceAll("&&","#leq");
    std::cout<<out2<<std::endl;
    return out2;
  }
  static TList* PrettyCutFormat(TList* cut_list){
    RETURN_LIST_OF(cut_list, new TObjString(PrettyCutFormat(((TObjString*)cut_list->At(IiI))->GetString())))
  }

  static TString Translate(TCut* cut, TList* dict){
    TString cut_string = cut->GetTitle();
    TList* key = (TList*)dict->At(0);
    TList* val = (TList*)dict->At(1);
    FOR_EACH(key, cut_string.ReplaceAll(((TObjString*)key->At(IiI))->GetString(),((TObjString*)val->At(IiI))->GetString());)
    return cut_string+"   ";
  }

  static TList* Translate(TList* cut_list, TList* dict){
    RETURN_LIST_OF(cut_list, new TObjString(Translate((TCut*)cut_list->At(IiI),dict)))
  }

  static TString OverSimplify(const char* cut){
    TString cut_string(cut);
    cut_string.Replace(cut_string.First("&&"), cut_string.Length() - cut_string.First("&&"), "");
    cut_string.ReplaceAll(">=","=");
    cut_string.ReplaceAll(">","=");
    cut_string.ReplaceAll("<","=");
    cut_string.ReplaceAll("<=","=");
    cut_string.ReplaceAll("==","=");
    return cut_string;
  }

  static TList* OverSimplify(TList* cut_string_list){
    RETURN_LIST_OF(cut_string_list, new TObjString(OverSimplify(((TObjString*)cut_string_list->At(IiI))->GetString())))
  }

  static TString Simplify(const char* cut){
    TString cut_string(cut);
    cut_string.ReplaceAll(">=","=");
    cut_string.ReplaceAll(">","=");
    cut_string.ReplaceAll("<","=");
    cut_string.ReplaceAll("<=","=");
    cut_string.ReplaceAll("==","=");
    return cut_string;
  }
  static TList* Simplify(TList* cut_string_list){
    RETURN_LIST_OF(cut_string_list, new TObjString(Simplify(((TObjString*)cut_string_list->At(IiI))->GetString())))
  }

  static TAxis* AxisBy3DOption(TH3* h, Option_t* option = "zy"){
    TString optionS(option);
    if(optionS.Contains("x")) return optionS.Contains("y") ? h->GetZaxis() : h->GetYaxis();
    else return h->GetXaxis();
  }

  static TString AxisLetterBy3DOption(Option_t* option = "zy"){
    TString optionS(option);
    if(optionS.Contains("x")) return optionS.Contains("y") ? TString("z") : TString("y");
    else return TString("x");
  }

#define SKEW 1 //1
#define REFIT3_MAX_UNC 1 //1
#define REFIT3_ACUALLY_USE_MIDDLE_UNC 1 //tmeporary test
#define REFIT3_SIGMA_CUT 0.5
#define REFIT3_SCALAR_CUT 1.
#define REFIT3_VARIATION 5.0//0.1
#define BEST_TWO_OUT_OF_THREE 0 //run the fitting 3 times. If 2 agree replace the fit if it is not one of the 2.
#define BEST_TWO_SIGMA_CUT 2. //range for match
#define SKIP_LOW_STATS      10. //100. //Number of effective entries threshold.
#define REMOVE_TAILS        0
#define SIGMA_CUT           5.
#define DO_FIT_RANGE        1 //providing saturation limits to the fit params
#define TRUNCATED_MEAN_LOW  0
#define TRUNCATED_MEAN_HIGH 0
#define TRUNCATED_STD_LOW   0
#define TRUNCATED_STD_HIGH  0
  static Double_t truncated_mean_low;
  static Double_t truncated_mean_high;
  static Double_t truncated_std_low;
  static Double_t truncated_std_high;
  static Double_t skip_low_stats;
  static Double_t remove_tails;
  static bool     do_fit_range;
  static Double_t sigma_cut;
  static bool skew;
  static int unique_fit_id;

  static void FitSlicesY(TH2* h, TF1* f1 = 0, Int_t firstxbin = 0, Int_t lastxbin = -1, Int_t cut = 0, Option_t* option = "QNR", TObjArray* arr = 0){
    if(f1==0&&(remove_tails||BEST_TWO_OUT_OF_THREE||skew||DO_FIT_RANGE||REFIT3_MAX_UNC)){
      h->FitSlicesY(f1,firstxbin,lastxbin,cut,option,arr);
      TH1D* h_mean = (TH1D*)gDirectory->Get(TString(h->GetName())+"_1");
      TH1D* h_std  = (TH1D*)gDirectory->Get(TString(h->GetName())+"_2");
      TH1D* h_mean_clone = (TH1D*)h_mean->Clone(TString(h_mean->GetName())+"_clonage");
      TH1D* h_std_clone = (TH1D*)h_std->Clone(TString(h_std->GetName())+"_clonage");
      h_mean->Reset();
      h_std->Reset();
      Int_t nbins = h->GetXaxis()->GetNbins();
      TString name(h->GetName());
      TH2* h_clone = (TH2*)h->Clone(TString(h->GetName())+"_clonage");

      Double_t range_min = h->GetXaxis()->GetXmin();
      Double_t range_max = h->GetXaxis()->GetXmax();

      for(int i = 1; i <= nbins; ++i){

	TF1 *truncated_gaus;
	if(remove_tails){
	  truncated_gaus = skew ? new TF1(TString(h->GetName())+"_fit_"+TString::Format("%i",i-1),"[0]*exp(-0.5*pow((x-[1])/([2]+(x>[1])*[3]*(x-[1])),2))",h_mean_clone->GetBinContent(i)-sigma_cut*h_std_clone->GetBinContent(i),h_mean_clone->GetBinContent(i)+sigma_cut*h_std_clone->GetBinContent(i)) : new TF1(TString(h->GetName())+"_fit","gaus(0)",h_mean_clone->GetBinContent(i)-sigma_cut*h_std_clone->GetBinContent(i),h_mean_clone->GetBinContent(i)+sigma_cut*h_std_clone->GetBinContent(i));
	}else{
	  std::cout<<"NOT REMOVING TAILS"<<std::endl;
	  truncated_gaus = skew ? new TF1(TString(h->GetName())+"_fit_"+TString::Format("%i",i-1),"[0]*exp(-0.5*pow((x-[1])/([2]+(x>[1])*[3]*(x-[1])),2))",range_min,range_max) : new TF1(TString(h->GetName())+"_fit","gaus(0)",range_min,range_max);
	}

	truncated_gaus->SetParameter(0,1.);
	truncated_gaus->SetParameter(3,0.01);
	if(skew) truncated_gaus->SetParLimits(3,-0.001,0.25);
	if(DO_FIT_RANGE){
	  truncated_gaus->SetParLimits(1,truncated_mean_low,truncated_mean_high);
	  truncated_gaus->SetParLimits(2,truncated_std_low,truncated_std_high);
	  std::cout<<"Doing fit range."<<std::endl;
	}

	if(DO_FIT_RANGE && ( h_mean_clone->GetBinContent(i) < truncated_mean_low || h_mean_clone->GetBinContent(i) > truncated_mean_high)){
	  truncated_gaus->SetParameter(1,.5*truncated_mean_low+.5*truncated_mean_high);
	  std::cout<<"Outside fit range: "<< truncated_mean_low << " >? " << h_mean_clone->GetBinContent(i) << " >? " << truncated_mean_high << std::endl;
	}else{
	  std::cout<<"Inside fit range."<<std::endl;
	  truncated_gaus->SetParameter(1,h_mean_clone->GetBinContent(i));
	}

	if(DO_FIT_RANGE&& (h_std_clone->GetBinContent(i) < truncated_std_low || h_std_clone->GetBinContent(i) > truncated_std_high)){
	  truncated_gaus->SetParameter(1,.5*truncated_std_low+.5*truncated_std_high);
	}else{
	  truncated_gaus->SetParameter(2,h_std_clone->GetBinContent(i));
	}
	h_clone->GetXaxis()->SetRange(i,i+1);
	TH1* projection = h_clone->ProjectionY();
	projection->SetName(TString(projection->GetName())+"_"+TString::Format("%i",i-1));
	double effective_entries = projection->GetEffectiveEntries();
	Int_t status = 1;
	status = ((Int_t)projection->Fit(truncated_gaus, ((bool)remove_tails ? "QNR" : "WLM")))%10; //"WLM"
	bool b2oo3_replacement = false;
	bool new_fits_agree = false;
	bool old_fit_agrees_with_both = true;
	bool refit3_max_unc_converge = false;
	if(BEST_TWO_OUT_OF_THREE || REFIT3_MAX_UNC){
	  TF1* truncated_gaus2 = (TF1*)truncated_gaus->Clone(TString(truncated_gaus->GetName())+"b2oo32");
	  TF1* truncated_gaus3 = (TF1*)truncated_gaus->Clone(TString(truncated_gaus->GetName())+"b2oo33");
	  TF1* truncated_gaus4 = (TF1*)truncated_gaus->Clone(TString(truncated_gaus->GetName())+"b2oo34");

	  Int_t status2 = 1;
	  Int_t status3 = 1;
	  Int_t status4 = 1;
	  truncated_gaus2->SetParameter(1,truncated_gaus->GetParameter(1)+REFIT3_VARIATION);
	  truncated_gaus2->SetParameter(2,10.);
	  if(skew) truncated_gaus2->SetParameter(3,0.01);
	  truncated_gaus3->SetParameter(1,truncated_gaus->GetParameter(1)-REFIT3_VARIATION);
	  truncated_gaus3->SetParameter(2,10.);
	  if(skew) truncated_gaus3->SetParameter(3,0.01);
	  truncated_gaus4->SetParameter(1,truncated_gaus->GetParameter(1));
	  truncated_gaus4->SetParameter(2,10.);
	  if(skew) truncated_gaus4->SetParameter(3,0.01);
	  status2 = ((Int_t)projection->Fit(truncated_gaus2,((bool)remove_tails ? "QNR+" : "WLM+")))%10;
	  status3 = ((Int_t)projection->Fit(truncated_gaus3,((bool)remove_tails ? "QNR+" : "WLM+")))%10;
	  if(REFIT3_MAX_UNC) status4 = ((Int_t)projection->Fit(truncated_gaus4,((bool)remove_tails ? "QNR+" : "WLM+")))%10;


	  std::cout<<"histogram: "<<projection->GetName()<<std::endl;
	  std::cout<<"status:"<<status<<"status2: "<<(int)status2<<" status3: "<<(int)status3<<std::endl;
	  std::cout<<"par1: "<<truncated_gaus->GetParameter(1)<<"+/-"<<truncated_gaus->GetParError(1)<<std::endl;
	  std::cout<<"par1_2: "<<truncated_gaus2->GetParameter(1)<<"+/-"<<truncated_gaus2->GetParError(1)<<std::endl;
	  std::cout<<"par1_3: "<<truncated_gaus3->GetParameter(1)<<"+/-"<<truncated_gaus3->GetParError(1)<<std::endl;
	  if(BEST_TWO_OUT_OF_THREE && (!status2) && (!status3)){
	    new_fits_agree = (truncated_gaus2->GetParameter(1) < truncated_gaus3->GetParameter(1) + BEST_TWO_SIGMA_CUT*truncated_gaus3->GetParameter(2)) && (truncated_gaus2->GetParameter(1) > truncated_gaus3->GetParameter(1) - BEST_TWO_SIGMA_CUT*truncated_gaus3->GetParameter(2)) && (truncated_gaus3->GetParameter(1) < truncated_gaus2->GetParameter(1) + BEST_TWO_SIGMA_CUT*truncated_gaus2->GetParameter(2)) && (truncated_gaus3->GetParameter(1) > truncated_gaus2->GetParameter(1) - BEST_TWO_SIGMA_CUT*truncated_gaus2->GetParameter(2)) && (truncated_gaus2->GetParameter(1) < truncated_gaus3->GetParameter(1) + 2.) && (truncated_gaus2->GetParameter(1) > truncated_gaus3->GetParameter(1) - 2.) && (truncated_gaus3->GetParameter(1) < truncated_gaus2->GetParameter(1) + 2.) && (truncated_gaus3->GetParameter(1) > truncated_gaus2->GetParameter(1) - 2.);
	    old_fit_agrees_with_both = (truncated_gaus2->GetParameter(1) < truncated_gaus->GetParameter(1) + BEST_TWO_SIGMA_CUT*truncated_gaus->GetParameter(2)) && (truncated_gaus2->GetParameter(1) > truncated_gaus->GetParameter(1) - BEST_TWO_SIGMA_CUT*truncated_gaus->GetParameter(2)) && (truncated_gaus3->GetParameter(1) < truncated_gaus->GetParameter(1) + BEST_TWO_SIGMA_CUT*truncated_gaus->GetParameter(2)) && (truncated_gaus3->GetParameter(1) > truncated_gaus->GetParameter(1) - BEST_TWO_SIGMA_CUT*truncated_gaus->GetParameter(2)) && (truncated_gaus2->GetParameter(1) < truncated_gaus->GetParameter(1) + 2.) && (truncated_gaus2->GetParameter(1) > truncated_gaus->GetParameter(1) - 2.) && (truncated_gaus3->GetParameter(1) < truncated_gaus->GetParameter(1) + 2.) && (truncated_gaus3->GetParameter(1) > truncated_gaus->GetParameter(1) - 2.);
	  }
	  if(new_fits_agree && (!old_fit_agrees_with_both) && (!REFIT3_MAX_UNC)){
	    truncated_gaus = truncated_gaus2;
	    b2oo3_replacement = true;
	    std::cout<<"REPLACEMENT!"<<std::endl;
	  }

	  if(REFIT3_MAX_UNC){
	    bool converge23 = (truncated_gaus2->GetParameter(1) < truncated_gaus3->GetParameter(1) + REFIT3_SIGMA_CUT*truncated_gaus3->GetParameter(2)) && (truncated_gaus2->GetParameter(1) > truncated_gaus3->GetParameter(1) - REFIT3_SIGMA_CUT*truncated_gaus3->GetParameter(2)) && (truncated_gaus3->GetParameter(1) < truncated_gaus2->GetParameter(1) + REFIT3_SIGMA_CUT*truncated_gaus2->GetParameter(2)) && (truncated_gaus3->GetParameter(1) > truncated_gaus2->GetParameter(1) - REFIT3_SIGMA_CUT*truncated_gaus2->GetParameter(2)) && (truncated_gaus2->GetParameter(1) < truncated_gaus3->GetParameter(1) + REFIT3_SCALAR_CUT) && (truncated_gaus2->GetParameter(1) > truncated_gaus3->GetParameter(1) - REFIT3_SCALAR_CUT);
	    bool converge24 = (truncated_gaus2->GetParameter(1) < truncated_gaus4->GetParameter(1) + REFIT3_SIGMA_CUT*truncated_gaus4->GetParameter(2)) && (truncated_gaus2->GetParameter(1) > truncated_gaus4->GetParameter(1) - REFIT3_SIGMA_CUT*truncated_gaus4->GetParameter(2)) && (truncated_gaus4->GetParameter(1) < truncated_gaus2->GetParameter(1) + REFIT3_SIGMA_CUT*truncated_gaus2->GetParameter(2)) && (truncated_gaus4->GetParameter(1) > truncated_gaus2->GetParameter(1) - REFIT3_SIGMA_CUT*truncated_gaus2->GetParameter(2)) && (truncated_gaus2->GetParameter(1) < truncated_gaus4->GetParameter(1) + REFIT3_SCALAR_CUT) && (truncated_gaus2->GetParameter(1) > truncated_gaus4->GetParameter(1) - REFIT3_SCALAR_CUT);
	    bool converge34 = (truncated_gaus3->GetParameter(1) < truncated_gaus4->GetParameter(1) + REFIT3_SIGMA_CUT*truncated_gaus4->GetParameter(2)) && (truncated_gaus3->GetParameter(1) > truncated_gaus4->GetParameter(1) - REFIT3_SIGMA_CUT*truncated_gaus4->GetParameter(2)) && (truncated_gaus4->GetParameter(1) < truncated_gaus3->GetParameter(1) + REFIT3_SIGMA_CUT*truncated_gaus3->GetParameter(2)) && (truncated_gaus4->GetParameter(1) > truncated_gaus3->GetParameter(1) - REFIT3_SIGMA_CUT*truncated_gaus3->GetParameter(2)) && (truncated_gaus3->GetParameter(1) < truncated_gaus4->GetParameter(1) + REFIT3_SCALAR_CUT) && (truncated_gaus3->GetParameter(1) > truncated_gaus4->GetParameter(1) - REFIT3_SCALAR_CUT);
	    refit3_max_unc_converge = (!status2)&&(!status3)&&(!status4)&&converge23&&converge24&&converge34;
	    if(refit3_max_unc_converge){
	      if(!REFIT3_ACUALLY_USE_MIDDLE_UNC){
		truncated_gaus = truncated_gaus4;
		float max_mean_unc = truncated_gaus2->GetParError(1) > truncated_gaus3->GetParError(1) ? truncated_gaus2->GetParError(1) :  truncated_gaus3->GetParError(1);
		max_mean_unc = max_mean_unc > truncated_gaus4->GetParError(1) ? max_mean_unc : truncated_gaus4->GetParError(1);
		truncated_gaus->SetParError(1,max_mean_unc);
	      }else{
		truncated_gaus = truncated_gaus4;
		float max_mean_unc, two_mean_unc;
		if(truncated_gaus2->GetParError(1) > truncated_gaus3->GetParError(1)){
		  max_mean_unc = truncated_gaus2->GetParError(1);
		  two_mean_unc = truncated_gaus3->GetParError(1);
		}else{
		  two_mean_unc = truncated_gaus2->GetParError(1);
		  max_mean_unc = truncated_gaus3->GetParError(1);
		}
		if(max_mean_unc < truncated_gaus4->GetParError(1)){
		  two_mean_unc = max_mean_unc;
		  max_mean_unc = truncated_gaus4->GetParError(1);
		}else if(two_mean_unc < truncated_gaus4->GetParError(1)){
		  two_mean_unc = truncated_gaus4->GetParError(1);
		}
		truncated_gaus->SetParError(1,two_mean_unc);
	      }
	    }
	  }
	}
	std::cout<<"histogram: "<<projection->GetName()<<std::endl;
	std::cout<<"STATUS: "<<(int) status << std::endl;
	std::cout<<"EFFECTIVE ENTRIES: "<<effective_entries<<std::endl;
	if((BEST_TWO_OUT_OF_THREE&&(effective_entries > skip_low_stats && (((!status)  && old_fit_agrees_with_both) || b2oo3_replacement)))||(REFIT3_MAX_UNC&&refit3_max_unc_converge && (effective_entries > skip_low_stats))){
	  std::cout<<"setting bin content. "<<std::endl;
	  bool saturated_the_fit_range = false;
	  if(DO_FIT_RANGE){
	    saturated_the_fit_range = (truncated_std_low+.1 > truncated_gaus->GetParameter(2)) || (truncated_std_high-.1 < truncated_gaus->GetParameter(2)) || (truncated_mean_low+.1 > truncated_gaus->GetParameter(1)) || (truncated_mean_high-.1 < truncated_gaus->GetParameter(1));
	  }
	  if(!saturated_the_fit_range){
	    std::cout<<"SETCONTENT: "<<truncated_gaus->GetParameter(1)<<"+/-"<<truncated_gaus->GetParError(1)<<std::endl;
	    if(truncated_gaus->GetParError(1) < .5){
	      std::cout<<"FLAG: SMALL MEAN UNC "<<truncated_gaus->GetParError(1)<<std::endl;
	    }

	    h_mean->SetBinContent(i,truncated_gaus->GetParameter(1));
	    h_mean->SetBinError(i,truncated_gaus->GetParError(1));
	    h_std->SetBinContent(i,truncated_gaus->GetParameter(2));
	    h_std->SetBinError(i,truncated_gaus->GetParError(2));
	  }else{
	    std::cout<<"SATURATED THE FIT RANGE"<<std::endl;
	  }
	}else{
	  std::cout<<"Skipping low stats point: "<<effective_entries<<std::endl;
	}
      }
      delete h_clone;
      delete h_mean_clone;
      delete h_std_clone;
    }else h->FitSlicesY(f1,firstxbin,lastxbin,cut,option,arr);
  }



  static TF1* Fit(TH1* h, TF1* f){
    TF1* fit = (TF1*)f->Clone(TString(h->GetName())+"_myfit");
    Int_t status = 1;
    status = h->Fit(fit,"R");
    //don't restrict range
    //status = h->Fit(fit);
    if(status){
      fit->SetParameter(0, -999.9);
      std::cout<<"BADFIT!"<<std::endl;
    }
    return fit;
  }

  static TF1* Fit(TH1* h, TF1* f, Double_t (*fcn)(Double_t *, Double_t *)){
    Double_t xmin=0;
    Double_t xmax=1;
    f->GetRange(xmin,xmax);
    TF1* fit = new TF1(TString(h->GetName())+"_myfit", fcn, xmin,xmax,f->GetNpar());
    for(int i = 0; i < f->GetNpar(); ++i){
      Double_t parmin = 0;
      Double_t parmax = 0;
      f->GetParLimits(i,parmin,parmax);
      fit->SetParameter(i,f->GetParameter(i));
      fit->SetParError(i,f->GetParError(i));
      if(parmin||parmax) fit->SetParLimits(i,parmin,parmax);
    }

    Int_t status = 1;

    status = h->Fit(fit);

    if(status) fit->SetParameter(0, -999.9);
    return fit;
  }

  //limited fucntionality. Exists because I can't fuckin clone fits like this
  static TF1* Fit(TH1* h, TF1* f, Double_t (*fcn)(Double_t *, Double_t *), TFitResultPtr& fr){
    Double_t xmin=0;
    Double_t xmax=1;
    f->GetRange(xmin,xmax);
    TF1* fit = new TF1(TString(h->GetName())+"_myfit", fcn, xmin,xmax,f->GetNpar());
    for(int i = 0; i < f->GetNpar(); ++i){
      Double_t parmin = 0;
      Double_t parmax = 0;
      f->GetParLimits(i,parmin,parmax);
      fit->SetParameter(i,f->GetParameter(i));
      fit->SetParError(i,f->GetParError(i));
      if(parmin||parmax) fit->SetParLimits(i,parmin,parmax);
    }

    Int_t status = 1;

    fr = h->Fit(fit,"S");
    status = (Int_t) fr;
    if(status) fit->SetParameter(0, -999.9);
    return fit;
  }




  static TList* Fit(TList* h_list, TF1* f, int list_depth = 1){
    std::cout<<"h_list size:"<<h_list->GetSize()<<" "<<list_depth<<std::endl;

    if(list_depth<=1){
      TList* out_list = new TList;
      for(int IiI = 0; IiI < h_list->GetSize();++IiI){
	TF1* fit = Fit((TH1*)h_list->At(IiI), f);
	out_list->Add(fit);
      }
      return out_list;
    }else{
      TList* out_list = new TList;
      for(int IiI = 0; IiI < h_list->GetSize();++IiI){
	out_list->Add(Fit((TList*)h_list->At(IiI), f, list_depth-1));
      }
      return out_list;
    }
  }

  static TH1D* Fit3DExclusiveBin(TH3D* h, float lower, float upper, Option_t* option = "zy"){
    Int_t nbins = AxisBy3DOption(h,option)->GetNbins();
    TH1D* hproj = (TH1D*)h->Project3D(AxisLetterBy3DOption(option));
    TH1D* hout = new TH1D();
    hproj->Copy(*hout);
    hout->Reset();
    for(int i = 1; i <= nbins; ++i){
      AxisBy3DOption(h,option)->SetRange(i,i+1);
      TH2D* hslice = (TH2D*)(h->Project3D(option));
      hslice->Sumw2();
      FitSlicesY(hslice);
      TF1 *fit = new TF1("fit","[0]+[1]*x",lower,upper);
      TH1D* test = (TH1D*)gDirectory->Get(TString(hslice->GetName())+"_1");
      TFitResultPtr fp = test->Fit(fit,"S");
      if(!(Int_t)fp){
	hout->SetBinContent(i,fp->Parameter(1));
	hout->SetBinError(i,fp->ParError(1));
      }
      delete hslice;
      delete fit;
    }
    AxisBy3DOption(h,option)->SetRange(0,0);
    return hout;
  }


  #define GET_TRIM_LOW_STATS 1
  #define GET_TRIM_CUT 10

  static TH1D* GetMean(TH2* h, float differential = 0){
    TH2* h_clone = (TH2*)h->Clone(TString(h->GetName())+"_clone");
    Int_t nbins = h->GetXaxis()->GetNbins();
    TH1D* h_mean = h->ProjectionX(TString(h->GetName())+"_mean");
    h_mean->Reset();
    for(int i = 1; i <= nbins; ++i){
      std::cout<<"BINWIDTH: "<<h_mean->GetBinWidth(i)<<std::endl;
      h_clone->GetXaxis()->SetRange(i,i);//potentially change back to i,i+1?
      if(!GET_TRIM_LOW_STATS || h_clone->GetEffectiveEntries() > GET_TRIM_CUT){
	h_mean->SetBinContent(i,differential!=0 ? h_clone->GetMean(2)/ differential : h_clone->GetMean(2));
	h_mean->SetBinError(i,differential!=0 ? h_clone->GetMeanError(2)/ differential : h_clone->GetMeanError(2));
      }
    }
    delete h_clone;
    return h_mean;
  }

  static TH1D* GetMeanGausCut(TH2* h, float min, float max, float differential = 0){
    TH2* h_clone = (TH2*)h->Clone(TString(h->GetName())+"_clone");
    Int_t nbins = h->GetXaxis()->GetNbins();
    TH1D* h_mean = h->ProjectionX(TString(h->GetName())+"_meangauscut");
    h_mean->Reset();
    for(int i = 1; i <= nbins; ++i){
      h_clone->GetXaxis()->SetRange(i,i);
      if(!GET_TRIM_LOW_STATS || h_clone->GetEffectiveEntries() > GET_TRIM_CUT){
        TH1* projection = h_clone->ProjectionY();
	projection->SetName(TString(projection->GetName())+"_"+TString::Format("%i",i-1));
	Int_t status = 1;
	TF1* f =  new TF1(TString(h->GetName())+"_fit","gaus(0)",min,max);
	std::cout<<"mean: "<< h_clone->GetMean(2) << std::endl;
	f->SetParameter(1, h_clone->GetMean(2));
	f->SetParameter(0, 1.);
	f->SetParameter(2, 1.);

	status = ((Int_t)projection->Fit(f,"WLMR"))%10;
	if(!status){
	  h_mean->SetBinContent(i,f->GetParameter(1));
	  h_mean->SetBinError(i,f->GetParError(1));
	}
      }
    }
    delete h_clone;
    return h_mean;
  }

  static TH1D* GetMeanFitPeak(TH2* h, float differential = 0){
    TH2* h_clone = (TH2*)h->Clone(TString(h->GetName())+"_clone");
    Int_t nbins = h->GetXaxis()->GetNbins();
    TH1D* h_mean = h->ProjectionX(TString(h->GetName())+"_meanfitpeak");
    h_mean->Reset();
    for(int i = 1; i <= nbins; ++i){
      h_clone->GetXaxis()->SetRange(i,i);
      if(!GET_TRIM_LOW_STATS || h_clone->GetEffectiveEntries() > GET_TRIM_CUT){
        TH1* projection = h_clone->ProjectionY();
	projection->SetName(TString(projection->GetName())+"_"+TString::Format("%i",i-1));
	Int_t status = 1;
	float mean = h_clone->GetMean(2);
	float std = h_clone->GetRMS(2);
	TF1* f =  new TF1(TString(h->GetName())+"_fit","gaus(0)",mean - 1.6*std, mean+1.6*std);
	//std::cout<<"mean: "<< h_clone->GetMean(2) << std::endl;
	f->SetParameter(1, h_clone->GetMean(2));
	f->SetParameter(0, 1.);
	f->SetParameter(2, h_clone->GetRMS(2));

	status = ((Int_t)projection->Fit(f,"WLMR"))%10;
	if(!status){
	  h_mean->SetBinContent(i,f->GetParameter(1));
	  h_mean->SetBinError(i,f->GetParError(1));
	}
      }
    }
    delete h_clone;
    return h_mean;
  }

  static TList* GetMeanFitPeak(TList* h_list, int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, GetMeanFitPeak((TH2*)h_list->At(IiI)))
    }else{
      RETURN_LIST_OF(h_list, GetMeanFitPeak((TList*)h_list->At(IiI),list_depth-1))
    }
  }


  static TList* GetMeanGausCut(TList* h_list, float min, float max, int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, GetMeanGausCut((TH2*)h_list->At(IiI),min,max))
    }else{
      RETURN_LIST_OF(h_list, GetMeanGausCut((TList*)h_list->At(IiI),min,max,list_depth-1))
    }
  }


  static TList* GetMeanDiff(TList* h_list, std::vector<float>& cuts, int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, GetMean((TH2*)h_list->At(IiI),cuts.at(IiI+1)-cuts.at(IiI)))
    }else{
      RETURN_LIST_OF(h_list, GetMeanDiff((TList*)h_list->At(IiI),cuts, list_depth-1))
    }
  }
  static TList* GetMean(TList* h_list, int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, GetMean((TH2*)h_list->At(IiI)))
    }else{
      RETURN_LIST_OF(h_list, GetMean((TList*)h_list->At(IiI),list_depth-1))
    }
  }

  static TH1D* GetStd(TH2* h, float differential = 0){
    TH2* h_clone = (TH2*)h->Clone(TString(h->GetName())+"_clone");
    Int_t nbins = h->GetXaxis()->GetNbins();
    TH1D* h_mean = h->ProjectionX(TString(h->GetName())+"_std");
    h_mean->Reset();
    for(int i = 1; i <= nbins; ++i){
      std::cout<<"BINWIDTH: "<<h_mean->GetBinWidth(i)<<std::endl;
      h_clone->GetXaxis()->SetRange(i,i+1);
      if(!GET_TRIM_LOW_STATS || h_clone->GetEffectiveEntries() > GET_TRIM_CUT){
	h_mean->SetBinContent(i,differential!=0 ? h_clone->GetRMS(2)/ differential : h_clone->GetRMS(2));
	h_mean->SetBinError(i,differential!=0 ? h_clone->GetRMSError(2)/ differential : h_clone->GetRMSError(2));
      }
    }
    delete h_clone;
    return h_mean;
  }

  static TList* GetStd(TList* h_list, int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, GetStd((TH2*)h_list->At(IiI)))
    }else{
      RETURN_LIST_OF(h_list, GetStd((TList*)h_list->At(IiI), list_depth-1))
    }
  }

  static TList* GetStdDiff(TList* h_list, std::vector<float>& cuts, int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, GetStd((TH2*)h_list->At(IiI),cuts.at(IiI+1)-cuts.at(IiI)))
    }else{
      RETURN_LIST_OF(h_list, GetStdDiff((TList*)h_list->At(IiI),cuts, list_depth-1))
    }
  }


  //0 = x, 1 = y, 2 = z is the axis that the slices are along
  static TList* Slice3D(TH3D* h, Option_t* option = "zy"){
    std::cout<<"3D Slicing "<<h->GetName()<<std::endl;
    TList* slice_list = new TList;
    Int_t nbins = AxisBy3DOption(h,option)->GetNbins();
    TH1D* hproj = (TH1D*)h->Project3D(AxisLetterBy3DOption(option));
    TH1D* hout = new TH1D();
    hproj->Copy(*hout);
    hout->Reset();
    for(int i = 1; i <= nbins; ++i){
      AxisBy3DOption(h,option)->SetRange(i,i+1);
      TH2D* hslice = (TH2D*)(h->Project3D(TString(option)+TString::Format("%i",i-1)));
      hslice->Sumw2();
      slice_list->Add(hslice);
    }
    AxisBy3DOption(h,option)->SetRange(0,0);
    return slice_list;
  }
  static TList* Slice3D(TList* h_list, Option_t* option = "zy", int list_depth = 1){
    if(list_depth <=1){
      RETURN_LIST_OF(h_list, Slice3D((TH3D*)h_list->At(IiI), option))
    }else{
      RETURN_LIST_OF(h_list, Slice3D((TList*)h_list->At(IiI), option, list_depth-1))
    }
  }

  static void DumpListListList(TList* list){
    for(int i = 0; i < list->GetSize();++i){
      for(int j = 0; j < ((TList*)list->At(i))->GetSize();++j){
	for(int k = 0; k < ((TList*)((TList*)list->At(i))->At(j))->GetSize();++k){
	  std::cout<<"out:"<<i<<" mid:"<<j<<" in:"<<k<<" "<<((TH1*)((TList*)((TList*)list->At(i))->At(j))->At(k))->GetName()<<std::endl;
	}
      }
    }

  }

  static TList* MultiFitSlicesY(TList* inlist, TF1* f1 = 0, Int_t firstxbin = 0, Int_t lastxbin = -1, Int_t cut = 0, Option_t* option = "QNR", TObjArray* arr = 0){
    TList *list  = new TList;
    int n =3;
    for(int i = 0; i < n; ++i){
      TList *listinside = new TList;
      list->Add(listinside);
    }
    FOR_EACH(inlist, FitSlicesY(((TH2*)inlist->At(IiI)),f1,firstxbin,lastxbin,cut,option,arr);
	     for(int i = 0; i < n; ++i){
	       ((TList*)list->At(i))->Add(gDirectory->Get(TString(((TH1*)inlist->At(IiI))->GetName())+"_"+TString::Format("%i",i)));
	     }
	     )
    return list;
  }

  static TList* MultiMultiFitSlicesY(TList* th2_list_list,  TF1* f1 = 0, Int_t firstxbin = 0, Int_t lastxbin = -1, Int_t cut = 0, Option_t* option = "QNR", TObjArray* arr = 0){
    TList *list  = new TList;
    int n = 3;
    for(int i = 0; i < n; ++i){
      TList *listinside = new TList;
      list->Add(listinside);
      for(int j = 0; j < th2_list_list->GetSize();++j){
	((TList*)list->At(i))->Add(new TList);
      }
    }
    for(int j = 0; j < th2_list_list->GetSize();++j){
      FOR_EACH(((TList*)th2_list_list->At(j)), FitSlicesY(((TH2*)((TList*)th2_list_list->At(j))->At(IiI)),f1,firstxbin,lastxbin,cut,option,arr);
	       for(int i = 0; i < n; ++i){
		 ((TList*)((TList*)list->At(i))->At(j))->Add(gDirectory->Get(TString(((TH1*)((TList*)th2_list_list->At(j))->At(IiI))->GetName())+"_"+TString::Format("%i",i)));
		 ((TH1*)gDirectory->Get(TString(((TH1*)((TList*)th2_list_list->At(j))->At(IiI))->GetName())+"_"+TString::Format("%i",i)))->Draw();
	       }
	       )
    }
    return list;
  }

  static TList* MultiMultiMultiFitSlicesY(TList* th2_list_list_list,  TF1* f1 = 0, Int_t firstxbin = 0, Int_t lastxbin = -1, Int_t cut = 0, Option_t* option = "QNR", TObjArray* arr = 0){
    TList *list  = new TList;
    int n = 3;
    for(int i = 0; i < n; ++i){
      list->Add(new TList);
      for(int j = 0; j < th2_list_list_list->GetSize();++j){

	((TList*)list->At(i))->Add(new TList);
	for(int k = 0; k < ((TList*)th2_list_list_list->At(j))->GetSize();++k){
	  ((TList*)((TList*)list->At(i))->At(j))->Add(new TList);
	}
      }
    }
    PTools::DumpListListList(th2_list_list_list);
    for(int k = 0; k < th2_list_list_list->GetSize();++k){
      for(int j = 0; j < ((TList*)th2_list_list_list->At(k))->GetSize();++j){
	FOR_EACH(((TList*)((TList*)th2_list_list_list->At(k))->At(j)),
		 TList* ll1 = ((TList*)th2_list_list_list->At(k));
		 TList* ll2 = ((TList*)ll1->At(j));
		 TH2* hh = ((TH2*)ll2->At(IiI));
		 FitSlicesY(hh,f1,firstxbin,lastxbin,cut,option,arr);
		 for(int i = 0; i < n; ++i){
		   TList* l1 = ((TList*)list->At(i));
		   TList* l2 = ((TList*)l1->At(k));
		   TList* l3 = ((TList*)l2->At(j));
		   TList* la = ((TList*)th2_list_list_list->At(k));
		   TList* lb = ((TList*)la->At(j));
		   TH1* h = ((TH1*)lb->At(IiI));
		   TH1* h2 = (TH1*)gDirectory->Get(TString(h->GetName())+"_"+TString::Format("%i",i));
		   l3->Add(h2);
		 }
		 )
       }
    }
    return list;
  }

  //returns list of parameters as TH2s in the list list variables.
  static TList* FitMultiMultiTH1(TList* th1_list_list, std::vector<float>& x_bounds,std::vector<float>& y_bounds, TF1* fit, int nPar){
    TList* list = new TList;
    float xb[x_bounds.size()];
    float yb[y_bounds.size()];
    for(unsigned i = 0 ; i < x_bounds.size(); ++i){ xb[i] = x_bounds.at(i); std::cout<<"xb:"<<i<<":"<<xb[i]<<std::endl;}
    for(unsigned i = 0 ; i < y_bounds.size(); ++i){ yb[i] = y_bounds.at(i); std::cout<<"yb:"<<i<<":"<<yb[i]<<std::endl;}


    for(int i = 0 ; i < nPar; ++i){
      list->Add(new TH2F(TString(((TH1*)((TList*)th1_list_list->At(0))->At(0))->GetName())+"_fitmmth1_"+TString::Format("%i",i),TString(((TH1*)((TList*)th1_list_list->At(0))->At(0))->GetName())+"_fitmmth1_"+TString::Format("%i",i),x_bounds.size()-1, &xb[0],y_bounds.size()-1, &yb[0]));

    }
    for(int i =0 ; i < th1_list_list->GetSize(); ++i){
      for(int j = 0; j < ((TList*)th1_list_list->At(i))->GetSize(); ++j){
	TFitResultPtr fp = ((TH1*)((TList*)th1_list_list->At(i))->At(j))->Fit(fit,"S");
	if(!(Int_t)fp){
	  for(int k = 0; k < list->GetSize(); ++k){
	    ((TH2F*)list->At(k))->SetBinContent(j+1,i+1,fp->Parameter(k));
	    ((TH2F*)list->At(k))->SetBinError(j+1,i+1,fp->ParError(k));
	  }
	}
      }
    }
    return list;
  }
};

class PTreeTools{
 public:
  static TString AddCondition(const char* selection, const char* condition){
    TString sel(selection);
    TString cond(condition);
    sel.ReplaceAll(" ","");
    if(sel.EndsWith(")")){
      sel.Insert(sel.Length()-1, TString("&&")+cond);
    }else{
      sel+=(TString("&&")+cond);
    }
    return sel;
  }

  static TString AddFunctionWeight(const char* selection, const char* weight){
    TString sel(selection);
    TString w(weight);
    sel.ReplaceAll(" ","");
    if(sel.EndsWith(")")){
      sel = "("+w+")*"+sel;
      //sel.Insert(sel.Length()-1, TString("&&")+cond);
    }else{
      sel = "("+sel+")";
      sel = "("+w+")*"+sel;
    }
    return sel;
  }


  static TString InsertVarExp(const char* varexp, const char* addition){
    TString hist(varexp);
    hist.Insert(hist.SubString(">>").Start(),addition);
    return hist;
  }

  static TString HistNameFromVarExp(const char* varexp){
    TString hist(varexp);
    hist.Replace(0,hist.SubString(">>").Start()+2,"");
    hist.Remove(hist.SubString("(").Start(),hist.Length()-hist.SubString("(").Start());
    return hist;
  }

  static TString AddSuffixToHistName(const char* varexp,const char* suffix){
    TString hist(varexp);
    hist.Insert(hist.SubString(">>").Start()+2+HistNameFromVarExp(varexp).Length(),suffix);
    return hist;
  }

  static TList* AddSuffixToHistName(TList* varexp,const char* suffix){
    RETURN_LIST_OF(varexp, new TObjString(AddSuffixToHistName(((TObjString*)varexp->At(IiI))->GetString(), suffix)))
  }

#define FREEHISTO 1
  static TH1* Draw(TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    std::cout<<"Making Histogram--varexp: "<<varexp<<" w/ selection: "<<selection<<std::endl;
    tree->Print("all");
    std::cout<<"-------------------------DEBUGGING: PTREETOOLS DRAWING HIST OUT"<<std::endl;
    std::cout<< "varexp: " << varexp << " selection: " << selection << " TString(option: " << TString(option) << " nentries: " << nentries<< " firstentry: " << firstentry << std::endl;
    tree->Draw(varexp,selection,TString(option)+"goff",nentries,firstentry); // Potential issues
    TH1* offset = (TH1*)gDirectory->Get(HistNameFromVarExp(varexp));

    std::cout<<"Histogram is named: "<<offset->GetName()<<std::endl;
    std::cout<<"Sum of Weights: "<<offset->GetSumOfWeights()<<std::endl;
    if(FREEHISTO){
      TH1* offset_clone = (TH1*) offset->Clone(TString(offset->GetName())+"_clone");
      offset_clone->SetDirectory(0);
      return offset_clone;
    }
    return offset;
  }

  //cuts are TCuts
  static TList* Draw(TList* cuts, TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(cuts, Draw(tree, AddSuffixToHistName(varexp,UNIQUE_ID),AddCondition(selection,((TCut*)cuts->At(IiI))->GetTitle()),option,nentries,firstentry))
  }
//////////////////////////////////////////////////
// Potential issues here
///////////

  static TList* Draw(TList* outer_cuts, TList* inner_cuts, TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(outer_cuts, Draw(inner_cuts, tree, AddSuffixToHistName(varexp,UNIQUE_ID),AddCondition(selection,((TCut*)outer_cuts->At(IiI))->GetTitle()),option,nentries,firstentry))

  }

  static TList* DrawWithFunctionWeight(TList* function_weights, TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(function_weights, Draw(tree, AddSuffixToHistName(varexp,UNIQUE_ID),AddFunctionWeight(selection,((TObjString*)function_weights->At(IiI))->GetString()),option,nentries,firstentry))
 }

  static TList* DrawWithFunctionWeight(TList* function_weights, TList* inner_cuts, TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(function_weights, Draw(inner_cuts, tree, AddSuffixToHistName(varexp,UNIQUE_ID),AddFunctionWeight(selection,((TObjString*)function_weights->At(IiI))->GetString()),option,nentries,firstentry))
  }

  static TList* Draw(TList* outer_cuts, TList* middle_cuts, TList* inner_cuts, TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(outer_cuts, Draw(middle_cuts, inner_cuts, tree, AddSuffixToHistName(varexp,UNIQUE_ID),AddCondition(selection,((TCut*)outer_cuts->At(IiI))->GetTitle()),option,nentries,firstentry))
  }

//////////
// Potential issues
//////////////////////////////////////////////////


  //for multiple varexp tobjstrings
  static TList* Draw(TTree* tree, TList* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0, TString insideout=""){
    RETURN_LIST_OF(varexp,Draw(tree, AddSuffixToHistName(((TObjString*)varexp->At(IiI))->GetString(),insideout), selection, option,nentries, firstentry))
  }

  //for multiple varexp tobjstrings
  static TList* Draw(TList* cuts, TTree* tree, TList* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(varexp, Draw(cuts, tree, ((TObjString*)varexp->At(IiI))->GetString(),selection,option,nentries,firstentry))
  }
  //for multiple varexp tobjstrings
  static TList* DrawCutsOutside(TList* cuts, TTree* tree, TList* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(cuts, Draw(tree,varexp,TString(selection)+"&&"+TString(((TCut*)cuts->At(IiI))->GetTitle()),option,nentries,firstentry,UNIQUE_ID))
  }

  //for multiple varexp tobjstrings
  static TList* Draw(TList* outer_cuts, TList *inner_cuts, TTree* tree, TList* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    RETURN_LIST_OF(varexp, Draw(outer_cuts,inner_cuts, tree, ((TObjString*)varexp->At(IiI))->GetString(), selection,option,nentries,firstentry))
  }
};

class PPlotTools{
 public:
  static void CleanLargeUncertainties(TH1* h, float limit){
    int nbins = h->GetXaxis()->GetNbins();
    for(int bin = 1; bin <=nbins; ++bin){
      if(h->GetBinError(bin) > limit){
	h->SetBinError(bin,0.);
      }
    }
  }

  static Int_t bettercolor(int i, int n){
    if(i==0) return kBlue;
    if(i==1) return kViolet;
    if(i==2) return kRed;
    if(i==3) return kGreen+1;
    if(i==4) return kCyan+1;
    if(i==5) return kViolet-6;
    if(i==6) return kOrange+8;
    if(i==7) return kRed-2;
    if(i==8) return kYellow-2;
    if(i==9) return kGreen+3;
    if(i==10) return kCyan+3;
    return kBlack;
  }

  static Int_t bluered(int i, int n){
    Float_t r = ((float)i)/((float)n-1.);
    Float_t g = 0;
    Float_t b = ((float)n-(float)i-1.)/((float)n-1.);
    //return TColor::GetColor(r,g,b);
    return bettercolor(i,n);
  }

  static void ColorFitsBR(TList* h_list, int list_depth = 1){
    if(list_depth<=1){
      FOR_EACH(h_list,if(((TH1*)h_list->At(IiI))->GetFunction(TString(((TH1*)h_list->At(IiI))->GetName())+"_myfit")){
	  ((TH1*)h_list->At(IiI))->GetFunction(TString(((TH1*)h_list->At(IiI))->GetName())+"_myfit")->SetLineColor(bluered(IiI,h_list->GetSize()));
	  ((TH1*)h_list->At(IiI))->GetFunction(TString(((TH1*)h_list->At(IiI))->GetName())+"_myfit")->SetLineWidth(1);
	  ((TH1*)h_list->At(IiI))->GetFunction(TString(((TH1*)h_list->At(IiI))->GetName())+"_myfit")->SetLineStyle(3);
	})
    }else{
      FOR_EACH(h_list, ColorFitsBR(((TList*)h_list->At(IiI)),list_depth-1);)
    }
  }

  //in: TH1
#define SMART_RANGE 0//.2 //to turn off 0
#define STYLE_OFFSET 20
#define LINESTYLE_OFFSET 1
#define MARKERS_ON 1
#define LINESTYLE_ON 0
  static bool markers_on;
  static bool linestyle_on;

  static void DrawBR(TList* th1_list, Option_t* option = ""){
    if(th1_list->GetSize() < 1) return;
    ((TH1*)th1_list->At(0))->SetLineColor(bluered(0,th1_list->GetSize()));
    if(markers_on){
      ((TH1*)th1_list->At(0))->SetMarkerStyle(STYLE_OFFSET);
      ((TH1*)th1_list->At(0))->SetMarkerSize(1.0);
      ((TH1*)th1_list->At(0))->SetMarkerColor(bluered(0,th1_list->GetSize()));
    }
    if(linestyle_on){
      ((TH1*)th1_list->At(0))->SetLineStyle(LINESTYLE_OFFSET);
      ((TH1*)th1_list->At(0))->SetLineWidth(3);

    }
    ((TH1*)th1_list->At(0))->SetFillStyle(4000);
    if(SMART_RANGE) {
      TH1* copy = ((TH1*)th1_list->At(0))->DrawCopy(option);
      CleanLargeUncertainties(copy, SMART_RANGE*(copy->GetMaximum()-copy->GetMinimum()));
    }else{
      ((TH1*)th1_list->At(0))->Draw(option);

    }
    for(int i = 1; i < th1_list->GetSize(); ++i){
      ((TH1*)th1_list->At(i))->SetLineColor(bluered(i,th1_list->GetSize()));
      if(markers_on){
	((TH1*)th1_list->At(i))->SetMarkerStyle(STYLE_OFFSET+i);
	((TH1*)th1_list->At(i))->SetMarkerSize(1.0);
	((TH1*)th1_list->At(i))->SetMarkerColor(bluered(i,th1_list->GetSize()));
      }
      if(linestyle_on){
	((TH1*)th1_list->At(i))->SetLineStyle(LINESTYLE_OFFSET+i);
	((TH1*)th1_list->At(i))->SetLineWidth(3);

      }
      ((TH1*)th1_list->At(i))->SetFillStyle(4000);
      if(SMART_RANGE) {
	TH1* copy = ((TH1*)th1_list->At(i))->DrawCopy("same"+TString(option));
	CleanLargeUncertainties(copy, SMART_RANGE*(copy->GetMaximum()-copy->GetMinimum()));
      }else{
	((TH1*)th1_list->At(i))->Draw("same"+TString(option));
      }
    }
  }

  static TList* DrawMultiBR(TList* th1_list_list, Option_t* option = "", TPad* p = 0){
    //TList *list = PTreeTools::Draw(cuts, tree, varexp, selection, option, nentries, firstentry);
    if(!p) p = new TCanvas(TString(((TH1*)((TList*)th1_list_list->At(0))->At(0))->GetName())+"_c",TString(((TH1*)((TList*)th1_list_list->At(0))->At(0))->GetName())+"_c",400,400);
    p->cd();
    p->DivideSquare(th1_list_list->GetSize());
    TList *pad_list = new TList;
    for(int IiI = 0; IiI < th1_list_list->GetSize();++IiI){
      p->cd(IiI+1);
      pad_list->Add(gPad);
      DrawBR((TList*)th1_list_list->At(IiI), option);
    }
    return pad_list;
  }

  static TList* CutCanvas(const char* name, const char* title, Int_t ww, Int_t wh, std::vector<float>& cut_bounds, TString num_disp = "%3.0f", float factor = 1.){
   TList* cut_list = new TList;
   for(unsigned i = 1 ; i < cut_bounds.size(); ++i){
     cut_list->Add(new TCanvas(TString(name)+"cut"+TString::Format(num_disp,factor*cut_bounds.at(i-1)).ReplaceAll(" ",""),TString(title)+"cut"+TString::Format(num_disp,factor*cut_bounds.at(i-1)),ww,wh));
   }
   return cut_list;
  }

  static void CutSave(TList* canvas_list, const char* filename = "", const char* filename_ending = ".pdf", Option_t* option = ""){
    FOR_EACH(canvas_list, ((TCanvas*)canvas_list->At(IiI))->SaveAs(TString(filename)+((TCanvas*)canvas_list->At(IiI))->GetName()+TString(filename_ending),option);)
  }

  static TList* DrawMultiMultiBR(TList* th1_list_list_list, Option_t* option = "", TList* pads = 0){
    TList* p = new TList;
    if(!pads){
      //pads = new TList;
      FOR_EACH(th1_list_list_list,p->Add(DrawMultiBR((TList*)th1_list_list_list->At(IiI), option));)

    }else{
      FOR_EACH(th1_list_list_list,p->Add(DrawMultiBR((TList*)th1_list_list_list->At(IiI), option, ((TPad*)pads->At(IiI))));)
    }
    return p;
  }

  static void AddLeftLegend(TList* cut_names, TList* plots, Option_t* option = "lpe"){
    //float upper_bound  = (.20 + .05*cut_names->GetSize() > .50) ? .50 : (.20 + .05*cut_names->GetSize());
    float lower_bound  = (.60 - .05*cut_names->GetSize() < .30) ? .30 : (.60 - .05*cut_names->GetSize());

    TLegend* l = new TLegend(0.17,lower_bound,.94,.60);
    l->SetLineStyle(1);
    l->SetFillStyle(0);
    l->SetFillColor(kWhite);
    l->SetTextFont(42);
    l->SetBorderSize(0);
    //l->SetTextSize(text_size);
    //l->SetIndiceSize(.7);

    FOR_EACH(plots,l->AddEntry((TH1*)plots->At(IiI),((TObjString*)cut_names->At(IiI))->GetString(),option);)

      l->Draw();
  }


  static void AddBottomLegend(TList* cut_names, TList* plots, Option_t* option = "lpe"){
    float upper_bound  = (.20 + .05*cut_names->GetSize() > .50) ? .50 : (.20 + .05*cut_names->GetSize());

    TLegend* l = new TLegend(0.17,0.20,.94,upper_bound);
    l->SetLineStyle(1);
    l->SetFillStyle(0);
    l->SetFillColor(kWhite);
    //l->SetFillStyle(4000);
    l->SetTextFont(42);
    l->SetBorderSize(0);
    //l->SetTextSize(text_size);
    //l->SetIndiceSize(.7);

    FOR_EACH(plots,l->AddEntry((TH1*)plots->At(IiI),((TObjString*)cut_names->At(IiI))->GetString(),option);)

      l->Draw();
  }

  static void AddFatRightLegend(TList* cut_names, TList* plots, Option_t* option = "lpe"){
    float lower_bound  = (.70 - .05*cut_names->GetSize() < .40) ? .40 : (.70 - .05*cut_names->GetSize());

    TLegend* l = new TLegend(0.58,lower_bound,.94,.70);
    l->SetLineStyle(1);
    l->SetFillColor(kWhite);
    l->SetFillStyle(0);
    //l->SetFillStyle(4000);
    l->SetTextFont(42);
    l->SetBorderSize(0);
    //l->SetTextSize(text_size);
    //l->SetIndiceSize(.7);

    FOR_EACH(plots,l->AddEntry((TH1*)plots->At(IiI),((TObjString*)cut_names->At(IiI))->GetString(),option);)

      l->Draw();
  }


  static void AddCutLegend(TList* cut_names, TList* plots, Option_t* option = "lpe"){
    float lower_bound  = (.94 - .05*cut_names->GetSize() < .64) ? .64 : (.94 - .05*cut_names->GetSize());

    TLegend* l = new TLegend(0.68,lower_bound,.94,.94);
    l->SetLineStyle(1);
    l->SetFillColor(kWhite);
    l->SetFillStyle(0);
    //l->SetFillStyle(4000);
    l->SetTextFont(42);
    l->SetBorderSize(0);
    //l->SetTextSize(text_size);
    //l->SetIndiceSize(.7);

    FOR_EACH(plots,l->AddEntry((TH1*)plots->At(IiI),((TObjString*)cut_names->At(IiI))->GetString(),option);)

      l->Draw();
  }

  static void AddCutLegend(TList* cut_names, TList* plots, TPad* pad,  Option_t* option = "lpe"){
    pad->cd();
    AddCutLegend(cut_names, plots, option);
  }

  static void AddCutLegend(TList* cut_names, TList* plots, TList* pad_list, Option_t* option = "lpe"){
    FOR_EACH(pad_list, AddCutLegend(cut_names, plots, ((TPad*)pad_list->At(IiI)), option);)
  }

  //position is which plot on the canvas 0 to n-1 plots
  static void AddCutLegendOncePerCanvas(TList* cut_names, TList* plots, TList* pad_list_list, int position, Option_t* option = "lpe"){
    FOR_EACH(pad_list_list, AddCutLegend(cut_names, plots, ((TPad*)((TList*)pad_list_list->At(IiI))->At(position)), option);)
  }

  static void AddCutLegendAll(TList* cut_names, TList* plots, TList* pad_list_list, Option_t* option = "lpe"){
    FOR_EACH(pad_list_list, AddCutLegend(cut_names, plots, ((TList*)pad_list_list->At(IiI)), option);)
  }

  static void DrawBR(TList* cuts, TTree* tree, const char* varexp, const char* selection, Option_t* option = "", Long64_t nentries = 1000000000, Long64_t firstentry = 0){
    TList *list = PTreeTools::Draw(cuts, tree, varexp, selection, option, nentries, firstentry);
    new TCanvas();
    DrawBR(list, option);
  }

  static void Attire(TH1* th1, const char* x_title = "X", const char* y_title = "Y"){
    if(!th1) return;
    std::cout<<"in"<<std::endl;
    th1->GetXaxis()->SetTitle(x_title);
    th1->GetYaxis()->SetTitle(y_title);
  }

  static void Attire(TList* th1_list, const char* x_title = "X", const char* y_title = "Y",int layer = 0){
    if(!th1_list) return;
    std::cout<<"out"<<layer<<std::endl;
    if(layer==0){
      FOR_EACH(th1_list,Attire(((TH1*)th1_list->At(IiI)), x_title, y_title);)
    }else if(layer>0){
      FOR_EACH(th1_list,Attire(((TList*)th1_list->At(IiI)), x_title, y_title,layer-1);)
    }
  }

  static void AttireMulti(TList* th1_list_list_list, const char* x_title, TList* y_titles, int vary = 1){
    if(!th1_list_list_list) return;
    if(vary==2){
      FOR_EACH(th1_list_list_list,Attire(((TList*)th1_list_list_list->At(IiI)), x_title, ((TObjString*)y_titles->At(IiI))->GetString(),1);)
    }else if(vary == 1){
      for(int i = 0; i < th1_list_list_list->GetSize(); ++i){
	FOR_EACH(((TList*)th1_list_list_list->At(i)),Attire(((TList*)((TList*)th1_list_list_list->At(i))->At(IiI)), x_title, ((TObjString*)y_titles->At(IiI))->GetString());)
      }
    }
  }


  static void Title(TPad* pad, const char* title, Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    pad->cd();
    TPad* title_pad = (TPad*)gDirectory->Get(TString(pad->GetName())+"_title");
    if(!title_pad) title_pad = new TPad(TString(pad->GetName())+"_title","",0.19,0.64,.94,.94);
    title_pad->SetFillStyle(4000);
    title_pad->Draw();
    title_pad->cd();
    TLatex l;
    l.SetLineStyle(1);
    l.SetNDC();
    l.SetTextFont(42);
    l.SetTextSize(text_size);
    l.SetIndiceSize(.7);
    l.DrawLatex(x,y,title);
    pad->cd();
  }

  static void Title(const char* title1,const char* title2="",const char* title3="",const char* title4="", Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    TPad* pad = (TPad*)gPad;
    pad->SetLineStyle(1);
    Title(pad,title1,x,y,text_size);
    Title(pad,title2,x,y-text_size*1.3, text_size);
    Title(pad,title3,x,y-text_size*1.3*2, text_size);
    Title(pad,title4,x,y-text_size*1.3*3, text_size);
  }


  static void Title(TList* pad_list, TList* title_list,  Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    FOR_EACH(pad_list, Title(((TPad*)pad_list->At(IiI)),((TObjString*)title_list->At(IiI))->GetString(),x,y,text_size);)
  }

  static void Title(TList* pad_list, const char* title,  Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    FOR_EACH(pad_list, Title(((TPad*)pad_list->At(IiI)),title,x,y,text_size);)
  }

  static void Title(TList* pad_list, const char* title1, TList* title2_list,  Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    FOR_EACH(pad_list, Title(((TPad*)pad_list->At(IiI)),title1,x,y,text_size);
	     Title(((TPad*)pad_list->At(IiI)),((TObjString*)title2_list->At(IiI))->GetString(),x,y-text_size*1.3, text_size);)
  }

  static void Title(TList* pad_list, const char* title1, TList* title2_list,const char* title3,  Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){

    FOR_EACH(pad_list, Title(pad_list, title1, title2_list,x,y,text_size);
	     Title(((TPad*)pad_list->At(IiI)),title3,x,y-text_size*1.3*2, text_size);)
  }

  static void Title(TList* pad_list, const char* title1, TList* title2_list,const char* title3, const char* title4,  Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    FOR_EACH(pad_list, Title(pad_list, title1, title2_list, title3,x,y,text_size);
	     Title(((TPad*)pad_list->At(IiI)),title4,x,y-text_size*1.3*3, text_size);)
  }

  static void Title(TList* pad_list, TList* title1_list, const char* title2,  Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    FOR_EACH(pad_list, Title(((TPad*)pad_list->At(IiI)),((TObjString*)title1_list->At(IiI))->GetString(),x,y,text_size);
	     Title(((TPad*)pad_list->At(IiI)),title2,x,y-text_size*1.3, text_size);)
  }

  static void Title(TList* pad_list, TList* title1_list, const char* title2, const char* title3, Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    FOR_EACH(pad_list, Title(pad_list, title1_list, title2, title3,x,y,text_size);
	     Title(((TPad*)pad_list->At(IiI)),title2,x,y-text_size*1.3, text_size);)
  }

  //vary which title you want to vary per canvas
  static void TitleMulti(TList* pad_list_list, TList* title_list1, TList* title_list2, int vary = 1, Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    if(vary ==2){
     std::cout<<"Vary order==2, try 1 if crash."<<std::endl;
      FOR_EACH(pad_list_list, Title(((TList*)pad_list_list->At(IiI)),((TObjString*)title_list1->At(IiI))->GetString(), title_list2,x,y,text_size);)
    }else if(vary==1){
    std::cout<<"Vary order==1, try 2 if crash."<<std::endl;
      FOR_EACH(pad_list_list, Title(((TList*)pad_list_list->At(IiI)),title_list1, ((TObjString*)title_list2->At(IiI))->GetString(),x,y,text_size);)
    }
  }

  static void TitleMulti(TList* pad_list_list, TList* title_list1, TList* title_list2,const char* title3, int vary = 1, Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    TitleMulti(pad_list_list,title_list1,title_list2,vary,x,y,text_size);
    FOR_EACH(pad_list_list,Title(((TList*)pad_list_list->At(IiI)),title3,x,y-text_size*1.3*2,text_size);)
  }

  static void TitleMulti(TList* pad_list_list, const char* title1, TList* title_list2, TList* title_list3,const char* title4, int vary = 1, Double_t x = 0.0, Double_t y = 0.75, float text_size = .17){
    TitleMulti(pad_list_list,title_list2,title_list3,vary,x,y-text_size*1.3,text_size);
    FOR_EACH(pad_list_list,Title(((TList*)pad_list_list->At(IiI)),title1,x,y,text_size);
	     Title(((TList*)pad_list_list->At(IiI)),title4,x,y-text_size*1.3*3,text_size);)
  }
};

Double_t PTools::truncated_mean_low  = TRUNCATED_MEAN_LOW;
Double_t PTools::truncated_mean_high = TRUNCATED_MEAN_HIGH;
Double_t PTools::truncated_std_low   = TRUNCATED_STD_LOW;
Double_t PTools::truncated_std_high  = TRUNCATED_STD_HIGH;
Double_t PTools::skip_low_stats      = SKIP_LOW_STATS;
Double_t PTools::remove_tails        = REMOVE_TAILS;
bool     PTools::do_fit_range        = DO_FIT_RANGE;
Double_t PTools::sigma_cut           = SIGMA_CUT;
bool     PTools::skew                = SKEW;
int      PTools::unique_fit_id = 0;

Double_t PTools::min_space = MIN_SPACE;
Double_t PTools::max_space = MAX_SPACE;
bool PTools::min_zero = false;
TString PTools::num_display = "%4.2f";

bool PPlotTools::markers_on = MARKERS_ON;
bool PPlotTools::linestyle_on = LINESTYLE_ON;


#endif //CALIBTOOLS_H