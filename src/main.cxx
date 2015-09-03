/**
 * @file      main.cxx
 * @brief     Main body of the system.
 * @author    Wooyoung Jang (wyjang)
 * @date      2015. 04. 01
 */
#include <iostream>
#include <cstring>

#ifndef __AMSINC__
#define __AMSINC__
#include "amschain.h"
#include "selector.h"
#endif

#ifndef __ACSOFTINC__
#define __ACSOFTINC__
#include "FileManager.hh"
#include "AMSRootSupport.hh"
#include "AMSRootParticleFactory.hh"
#endif

// Some global variables
char releaseName[16];
char softwareName[10] = "ACCTOFInt";
char versionNumber[5] = "0.01";

bool IsBadRun(AMSEventR* thisEvent);
bool IsGoodParticle(ParticleR* thisParticle);

/**
 * @brief This is main() function.
 * @return 0 : The program terminated properly. Otherwise : The program unintentionally terminated by error.
 */

int main(int argc, char* argv[])
{
  sprintf(releaseName, "%s%s", softwareName, versionNumber);

  /*************************************************************************************************************
   *
   * FILE OPENING PHASE
   *
   *************************************************************************************************************/

  // Unfold below to see the details of file opening
  /*{{{*/

  /*
   * 1. Test mode
   *   1-1. Skirmish test
   *     Use the designated run to test the code. No arguments are required. ( argc == 1)
   *   1-2. Skirmish test within given number of events
   *     User should pass the number of events to be analyzed as the program argument.
   *     ./a.out <number of events to be analyzed>  ( argc == 2 )
   *   1-3. Test using user-given run file within given number of events
   *     ./a.out <run file to be analyzed> <output file> <number of events to be analyzed> ( argc == 4 )
   *
   * 2. Job submission mode
   *   2-1. Standard job submission mode
   *     ./a.out <list file which contains list of files to be analyzed> <output file> ( argc == 3 )
   */

  // AMSChain
  AMSChain amsChain;

  //char skirmishRunPath[] = "root://eosams.cern.ch//eos/ams/Data/AMS02/2011B/ISS.B620/pass4/1323051106.00000001.root";   // Path of test run
  char skirmishRunPath[] = "root://eosams.cern.ch//eos/ams/Data/AMS02/2014/ISS.B950/pass6/1323051106.00000001.root";   // Path of test run
  char inputFileName[256];      // File path for single run. (Cat 3.)
  char outputFileName[256];     // The name of the output file to be stored in the disk.
  int  nEntries = 0;            // Number of entries to be analyzed.

  if( argc == 1 )
  {
    std::cout << "[" << releaseName << "] RUN MODE : Single Test Run (Cat. 1)" << endl;

    if(amsChain.Add(skirmishRunPath) != 1)
    {
      std::cerr << "[" << releaseName << "] ERROR    : File open error, [" << skirmishRunPath << "] can not be found!" << endl;
      return -1;
    }

    strcpy(outputFileName, "testrun.root");
    nEntries = amsChain.GetEntries();
  }
  else if( argc == 2 )
  {
    std::cout << "[" << releaseName << "] RUN MODE : Single Test Run (Cat. 2)" << endl;

    if(amsChain.Add(skirmishRunPath) != 1)
    {
      std::cerr << "[" << releaseName << "] ERROR    : File open error, [" << skirmishRunPath << "] can not be found!" << endl;
      return -1;
    }

    strcpy(outputFileName, "testrun.root");
    nEntries = atoi(argv[1]);
  }
  else if( argc == 3 )
  {
    std::cout << "[" << releaseName << "] RUN MODE : Batch-job Mode" << endl;

    char listFileName[256];       // The name of the list file.
    char inputFileName[256];

    FILE* fp;                     // File pointer to read list file.

    strcpy(listFileName, argv[1]);
    strcpy(outputFileName, argv[2]);

    if( ( fp = fopen( listFileName, "r") ) == NULL )
    {
      std::cerr << "[" << releaseName << "] ERROR     : Failed to open file [" << listFileName << "]!" << endl;
      return -1;
    }

    char* line_p;                 // Character pointer to filter out CRLF at the end of each line.
    while( fgets( inputFileName, 256, fp ) != NULL )
    {
      if( ( line_p = strchr(inputFileName, '\n') ) != NULL) *line_p = 0;  // Filter out \n at the end of lines.

      if(amsChain.Add( inputFileName ) != 1)
      {
        std::cerr << "[" << releaseName << "] ERROR     : Failed to open file [" << inputFileName << "]!" << endl;
        return -1;
      }
      else
      {
        std::cout << "[" << releaseName << "] The file [" << inputFileName << "] is added to the chain." << std::endl;
        std::cout << "[" << releaseName << "] Currently loaded events : " << amsChain.GetEntries() << std::endl;
      }
    }

    fclose(fp);

    nEntries = amsChain.GetEntries();
  }
  else if( argc >= 4 )
  {
    std::cout << "[" << releaseName << "] RUN MODE : Single Test Run (Cat. 3)" << endl;

    strcpy(inputFileName, argv[1]);
    strcpy(outputFileName, argv[2]);
    nEntries = atoi(argv[3]);

    if(amsChain.Add(inputFileName) != 1)
    {
      std::cerr << "[" << releaseName << "] ERROR   : File open error, [" << inputFileName << "] can not found!" << endl;
      return -1;
    }

    std::cout << "[" << releaseName << "] The file [" << inputFileName << "] is added to the chain." << std::endl;
  }/*}}}*/

  std::cout << "[" << releaseName << "] TOTAL EVENTS   : " << nEntries << endl;
  std::cout << "[" << releaseName << "] OUTPUT FILE NAME : " << outputFileName << endl;

  /***********************************************************************************************************************
   *
   *  ANALYSIS PHASE
   *
   ***********************************************************************************************************************/

  AMSSetupR::RTI::UseLatest();
  TkDBc::UseFinal();

  const bool setAMSRootDefaults = true;
  AMSRootSupport amsRootSupport(AC::ISSRun, setAMSRootDefaults);
  Analysis::EventFactory& eventFactory = amsRootSupport.EventFactory();
  Analysis::AMSRootParticleFactory& particleFactory = amsRootSupport.ParticleFactory();

  // Variable declaration which will be used for tree construction
  unsigned int  nCuts = 7;/*{{{*/           // Number of cuts
  unsigned int  nProcessed = 0;             // Number of processed events
  unsigned int  nRun;                       // Run number
  unsigned int  nEvent;                     // Event number
  unsigned int  nProcessedNumber;           // Number of processed events
  unsigned int  nLevel1;                    // Number of Level1 triggers
  unsigned int  nParticle;                  // Number of particles
  unsigned int  nCharge;                    // Number of charges
  unsigned int  nTrTrack;                   // Number of the Tracker tracks which are successfully reconstructed
  unsigned int  nTrdTrack;                  // Number of the TRD tracks which are successfully reconstructed
  unsigned int  nAntiCluster;               // Number of clusters on the ACC
  unsigned int  nTofClustersInTime;         // Number of in-time clusters on the TOF
  unsigned int  nRichRing;                  // Number of successfully reconstructed the RICH rings
  unsigned int  nRichRingB;                 // Number of successfully reconstructed the RICH rings with algorithm B
  unsigned int  nBeta;                      // Number of successfully estimated beta(v/c) values
  unsigned int  nBetaB;                     // Number of successfully estimated beta(v/c) values with algorithm B
  unsigned int  nBetaH;                     // Number of successfully estimated beta(v/c) values with algorithm H
  unsigned int  nShower;                    // Number of the ECAL shower objects
  unsigned int  nVertex;                    // Number of vertices in this event
  unsigned int  particleType;               // Type of ParticleR
  float         livetime;                   // Livetime fraction
  float         utcTime;                    // UTC time
  float         utcTimeCorrected;           // Corrected UTC time
  float         orbitAltitude;              // (cm) in GTOD coordinates system.
  float         orbitLatitude;              // (rad) in GTOD coordinates system.
  float         orbitLongitude;             // (rad) in GTOD coordinates system.
  float         orbitLatitudeM;             // (rad) in eccentric dipole coordinate system.
  float         orbitLongitudeM;            // (rad) in eccentric dipole coordinate system.
  float         velR;                       // Speed of the ISS in radial direction
  float         velTheta;                   // Angular speed of the ISS in polar angle direction
  float         velPhi;                     // Angular speed of the ISS in azimuthal angle direction
  float         yaw;                        // A parameter describing ISS attitude (tilted angle with respect to the x-axis)
  float         pitch;                      // A parameter describing ISS attitude (tilted angle with respect to the y-axis)
  float         roll;                       // A parameter describing ISS attitude (tilted angle with respect to the z-axis)
  float         gLongitude;                 // Galactic longitude of the incoming particle.
  float         gLatitude;                  // Galactic latitude of the incoming particle.
  int           gCoordCalcResult;           // Return value for galactic coordinate calculation.
  float         sunPosAzimuth;              // Azimuthal angle of the position of the Sun.
  float         sunPosElevation;            // Elevation angle of the position of the Sun.
  int           sunPosCalcResult;           // Return value for the Sun's position calculation.
  unsigned int  unixTime;                   // UNIX time
  int           isInShadow;                 // Value for check whether the AMS is in ISS solar panel shadow or not.
  unsigned int  ptlCharge;                  // ParticleR::Charge value
  float         ptlMomentum;                // ParticleR::Momentum value
  float         ptlTheta;                   // Direction of the incoming particle (polar angle)
  float         ptlPhi;                     // Direction of the incoming particle (azimuthal angle)
  float         ptlCoo[3];
  float         ptlCutOffStoermer;
  float         ptlCutOffDipole;
  float         ptlCutOffMax[2];
  float         showerEnergyD;
  float         showerEnergyE;
  float         showerBDT;
  float         showerCofG[3];
  float         showerCofGDist;
  float         showerCofGdX;
  float         showerCofGdY;
  int           tofNClusters;
  int           tofNUsedHits;
  float         tofBeta;
  int           isGoodBeta;
  int           isTkTofMatch;
  float         tofReducedChisqT;
  float         tofReducedChisqC;
  float         tofDepositedEnergyOnLayer[4];
  float         tofEstimatedChargeOnLayer[4];
  float         tofCharge;
  int           trkFitCodeMS;
  float         trkRigidityMS;
  float         trkReducedChisquareMS;
  int           trkFitCodeFS;
  float         trkRigidityFS;
  float         trkReducedChisquareFS;
  int           trkFitCodeInner;
  float         trkRigidityInner;
  float         trkReducedChisquareInner;
  int           trkEdepLayerJXSideOK[9];
  int           trkEdepLayerJYSideOK[9];
  float         trkEdepLayerJ[9];
  float         trkCharge;
  float         trkInnerCharge;
  int           richRebuild;
  int           richIsGood;
  int           richIsClean;
  int           richIsNaF;
  float         richRingWidth;
  int           richNHits;
  float         richBeta;
  float         richBetaError;
  float         richChargeSquared;
  float         richKolmogorovProbability;
  float         richTheta;
  float         richPhi;
  int           trdNClusters;
  int           trdNTracks;
  float         trdTrackPhi;
  float         trdTrackTheta;
  float         trdTrackChi2;
  int           trdTrackPattern;
  float         trdTrackCharge;
  float         trdDepositedEnergyOnLayer[20];
  int           trdQtNActiveLayer;
  int           trdQtIsValid;
  float         trdQtElectronToProtonLogLikelihoodRatio;
  float         trdQtHeliumToProtonLogLikelihoodRatio;
  float         trdQtElectronToHeliumLogLikelihoodRatio;
  int           trdKNRawHits;
  int           trdKIsReadAlignmentOK;
  int           trdKIsReadCalibOK;
  int           trdKNHits;
  int           trdKIsValid;
  float         trdKElectronToProtonLogLikelihoodRatio;
  float         trdKHeliumToProtonLogLikelihoodRatio;
  float         trdKElectronToHeliumLogLikelihoodRatio;
  float         trdKCharge;
  float         trdKChargeError;
  int           trdKNUsedHitsForCharge;
  float         trdKAmpLayer[20];
  float         trdKTotalPathLength;
  float         trdKElectronLikelihood;
  float         trdKProtonLikelihood;
  float         trdKHeliumLikelihood;
  /*}}}*/

  // Declare event counter
  TH1D* hEvtCounter = new TH1D("hEvtCounter", "Event Collecting Status", nCuts, 0., (float)nCuts);
  hEvtCounter->GetXaxis()->SetBinLabel(1, "Science-run check");
  hEvtCounter->GetXaxis()->SetBinLabel(2, "DAQ H/W status check");
  hEvtCounter->GetXaxis()->SetBinLabel(3, "Unbiased physics trigger check");
  hEvtCounter->GetXaxis()->SetBinLabel(4, "Multiple particles");
  hEvtCounter->GetXaxis()->SetBinLabel(5, "Tracker alignment test");
  hEvtCounter->GetXaxis()->SetBinLabel(6, "Good track test");
  hEvtCounter->GetXaxis()->SetBinLabel(7, "SAA rejection");

  // Make a TFile and TTree
  TFile* resultFile = new TFile(outputFileName, "RECREATE");
  TTree* tree  = new TTree(softwareName, releaseName);

  // Assign branches
  tree->Branch("nRun", &nRun, "nRun/i");/*{{{*/
  tree->Branch("nEvent", &nEvent, "nEvent/i");
  tree->Branch("nProcessedNumber", &nProcessedNumber, "nProcessedNumber/i");
  tree->Branch("nLevel1", &nLevel1, "nLevel1/i");
  tree->Branch("nParticle", &nParticle, "nParticle/i");
  tree->Branch("nCharge", &nCharge, "nCharge/i");
  tree->Branch("nTrTrack", &nTrTrack, "nTrTrack/i");
  tree->Branch("nTrdTrack", &nTrdTrack, "nTrdTrack/i");
  tree->Branch("nAntiCluster", &nAntiCluster, "nAntiCluster/i");
  tree->Branch("nTofClustersInTime", &nTofClustersInTime, "nTofClustersInTime/I");
  tree->Branch("nRichRing", &nRichRing, "nRichRing/i");
  tree->Branch("nRichRingB", &nRichRingB, "nRichRingB/i");
  tree->Branch("nBeta", &nBeta, "nBeta/i");
  tree->Branch("nBetaB", &nBetaB, "nBetaB/i");
  tree->Branch("nBetaH", &nBetaH, "nBetaH/i");
  tree->Branch("nShower", &nShower, "nShower/i");
  tree->Branch("nVertex", &nVertex, "nVertex/i");
  tree->Branch("particleType", &particleType, "particleType/i");
  tree->Branch("livetime", &livetime, "livetime/F");
  tree->Branch("utcTime", &utcTime, "utcTime/F");
  tree->Branch("utcTimeCorrected", &utcTimeCorrected, "utcTimeCorrected/F");
  tree->Branch("orbitAltitude", &orbitAltitude, "orbitAltitude/F");
  tree->Branch("orbitLatitude", &orbitLatitude, "orbitLatitude/F");
  tree->Branch("orbitLongitude", &orbitLongitude, "orbitLongitude/F");
  tree->Branch("orbitLatitudeM", &orbitLatitudeM, "orbitLatitudeM/F");
  tree->Branch("orbitLongitudeM", &orbitLongitudeM, "orbitLongitudeM/F");
  tree->Branch("velR", &velR, "velR/F");
  tree->Branch("velTheta", &velTheta, "velTheta/F");
  tree->Branch("velPhi", &velPhi, "velPhi/F");
  tree->Branch("yaw", &yaw, "yaw/F");
  tree->Branch("pitch", &pitch, "pitch/F");
  tree->Branch("roll", &roll, "roll/F");
  tree->Branch("gLongitude", &gLongitude, "gLongitude/F");
  tree->Branch("gLatitude", &gLatitude, "gLatitude/F");
  tree->Branch("gCoordCalcResult", &gCoordCalcResult, "gCoordCalcResult/I");
  tree->Branch("sunPosAzimuth", &sunPosAzimuth, "sunPosAzimuth/F");
  tree->Branch("sunPosElevation", &sunPosElevation, "sunPosElevation/F");
  tree->Branch("sunPosCalcResult", &sunPosCalcResult, "sunPosCalcResult/I");
  tree->Branch("unixTime", &unixTime, "unixTime/i");
  tree->Branch("isInShadow", &isInShadow, "isInShadow/I");
  tree->Branch("ptlCharge", &ptlCharge, "ptlCharge/i");
  tree->Branch("ptlMomentum", &ptlMomentum, "ptlMomentum/F");
  tree->Branch("ptlTheta", &ptlTheta, "ptlTheta/F");
  tree->Branch("ptlPhi", &ptlPhi, "ptlPhi/F");
  tree->Branch("ptlCoo", &ptlCoo, "ptlCoo[3]/F");
  tree->Branch("ptlCutOffStoermer", &ptlCutOffStoermer, "ptlCutOffStoermer/F");
  tree->Branch("ptlCutOffDipole", &ptlCutOffDipole, "ptlCutOffDipole/F");
  tree->Branch("ptlCutOffMax", &ptlCutOffMax, "ptlCutOffMax[2]/F");
  tree->Branch("showerEnergyD", &showerEnergyD, "showerEnergyD/F");
  tree->Branch("showerEnergyE", &showerEnergyE, "showerEnergyE/F");
  tree->Branch("showerBDT", &showerBDT, "showerBDT/F");
  tree->Branch("showerCofG", &showerCofG, "showerCofG[3]/F");
  tree->Branch("showerCofGDist", &showerCofGDist, "showerCofGDist/F");
  tree->Branch("showerCofGdX", &showerCofGdX, "showerCofGdX/F");
  tree->Branch("showerCofGdY", &showerCofGdY, "showerCofGdY/F");
  tree->Branch("tofNClusters", &tofNClusters, "tofNClusters/I");
  tree->Branch("tofNUsedHits", &tofNUsedHits, "tofNUsedHits/I");
  tree->Branch("tofBeta", &tofBeta, "tofBeta/F");
  tree->Branch("isGoodBeta", &isGoodBeta, "isGoodBeta/I");
  tree->Branch("isTkTofMatch", &isTkTofMatch, "isTkTofMatch/I");
  tree->Branch("tofReducedChisqT", &tofReducedChisqT, "tofReducedChisqT/F");
  tree->Branch("tofReducedChisqC", &tofReducedChisqC, "tofReducedChisqC/F");
  tree->Branch("tofDepositedEnergyOnLayer", &tofDepositedEnergyOnLayer, "tofDepositedEnergyOnLayer[4]/F");
  tree->Branch("tofEstimatedChargeOnLayer", &tofEstimatedChargeOnLayer, "tofEstimatedChargeOnLayer[4]/F");
  tree->Branch("tofCharge", &tofCharge, "tofCharge/F");
  tree->Branch("trkFitCodeMS", &trkFitCodeMS, "trkFitCodeMS/I");
  tree->Branch("trkRigidityMS", &trkRigidityMS, "trkRigidityMS/F");
  tree->Branch("trkReducedChisquareMS", &trkReducedChisquareMS, "trkReducedChisquareMS/F");
  tree->Branch("trkFitCodeFS", &trkFitCodeFS, "trkFitCodeFS/I");
  tree->Branch("trkRigidityFS", &trkRigidityFS, "trkRigidityFS/F");
  tree->Branch("trkReducedChisquareFS", &trkReducedChisquareFS, "trkReducedChisquareFS/F");
  tree->Branch("trkFitCodeInner", &trkFitCodeInner, "trkFitCodeInner/I");
  tree->Branch("trkRigidityInner", &trkRigidityInner, "trkRigidityInner/F");
  tree->Branch("trkReducedChisquareInner", &trkReducedChisquareInner, "trkReducedChisquareInner/F");
  tree->Branch("trkEdepLayerJXSideOK", &trkEdepLayerJXSideOK, "trkEdepLayerJXSideOK[9]/I");
  tree->Branch("trkEdepLayerJYSideOK", &trkEdepLayerJYSideOK, "trkEdepLayerJYSideOK[9]/I");
  tree->Branch("trkEdepLayerJ", &trkEdepLayerJ, "trkEdepLayerJ[9]/F");
  tree->Branch("trkCharge", &trkCharge, "trkCharge/F");
  tree->Branch("trkInnerCharge", &trkInnerCharge, "trkInnerCharge/F");
  tree->Branch("richRebuild", &richRebuild, "richRebuild/I");
  tree->Branch("richIsGood", &richIsGood, "richIsGood/I");
  tree->Branch("richIsClean", &richIsClean, "richIsClean/I");
  tree->Branch("richIsNaF", &richIsNaF, "richIsNaF/I");
  tree->Branch("richRingWidth", &richRingWidth, "richRingWidth/F");
  tree->Branch("richNHits", &richNHits, "richNHits/I");
  tree->Branch("richBeta", &richBeta, "richBeta/F");
  tree->Branch("richBetaError", &richBetaError, "richBetaError/F");
  tree->Branch("richChargeSquared", &richChargeSquared, "richChargeSquared/F");
  tree->Branch("richKolmogorovProbability", &richKolmogorovProbability, "richKolmogorovProbability/F");
  tree->Branch("richTheta", &richTheta, "richTheta/F");
  tree->Branch("richPhi", &richPhi, "richPhi/F");
  tree->Branch("trdNCluster", &trdNCluster, "trdNCluster/I");
  tree->Branch("trdNTracks", &trdNTracks, "trdNTracks/I");
  tree->Branch("trdTrackTheta", &trdTrackTheta, "trdTrackTheta/F");
  tree->Branch("trdTrackPhi", &trdTrackPhi, "trdTrackPhi/F");
  tree->Branch("trdTrackChi2", &trdTrackChi2, "trdTrackChi2/F");
  tree->Branch("trdTrackPattern", &trdTrackPattern, "trdTrackPattern/I");
  tree->Branch("trdTrackCharge", &trdTrackCharge, "trdTrackCharge/F");
  tree->Branch("trdDepositedEnergyOnLayer", &trdDepositedEnergyOnLayer, "trdDepositedEnergyOnLayer[2]/F");
  tree->Branch("trdQtNActiveLayer", &trdQtNActiveLayer, "trdQtNActiveLayer/I");
  tree->Branch("trdQtIsValid", &trdQtIsValid, "trdQtIsValid/I");
  tree->Branch("trdQtElectronToProtonLogLikelihoodRatio", &trdQtElectronToProtonLogLikelihoodRatio, "trdQtElectronToProtonLogLikelihoodRatio/F");
  tree->Branch("trdQtHeliumToProtonLogLikelihoodRatio", &trdQtHeliumToProtonLogLikelihoodRatio, "trdQtHeliumToProtonLogLikelihoodRatio/F");
  tree->Branch("trdQtElectronToHeliumLogLikelihoodRatio", &trdQtElectronToHeliumLogLikelihoodRatio, "trdQtElectronToHeliumLogLikelihoodRatio/F");
  tree->Branch("trdKNRawHits", &trdKNRawHits, "trdKNRawHits/I");
  tree->Branch("trdKIsReadAlignmentOK", &trdKIsReadAlignmentOK, "trdKIsReadAlignmentOK/I");
  tree->Branch("trdKIsReadCalibOK", &trdKIsReadCalibOK, "trdKIsReadCalibOK/I");
  tree->Branch("trdKNHits", &trdKNHits, "trdKNHits/I");
  tree->Branch("trdKIsValid", &trdKIsValid, "trdKIsValid/I");
  tree->Branch("trdKElectronToProtonLogLikelihoodRatio", &trdKElectronToProtonLogLikelihoodRatio, "trdKElectronToProtonLogLikelihoodRatio/F");
  tree->Branch("trdKHeliumToProtonLogLikelihoodRatio", &trdKHeliumToProtonLogLikelihoodRatio, "trdKHeliumToProtonLogLikelihoodRatio/F");
  tree->Branch("trdKElectronToHeliumLogLikelihoodRatio", &trdKElectronToHeliumLogLikelihoodRatio, "trdKElectronToHeliumLogLikelihoodRatio/F");
  tree->Branch("trdKCharge", &trdKCharge, "trdCharge/F");
  tree->Branch("trdKChargeError", &trdKChargeError, "trdKChargeError/F");
  tree->Branch("trdKNUsedHitsForCharge", &trdKNUsedHitsForCharge, "trdKNUsedHitsForCharge/F");
  tree->Branch("trdKAmpLayer", &trdKAmpLayer, "trdKAmpLayer[20]/F");
  tree->Branch("trdKTotalPathLength", &trdKTotalPathLength, "trdKTotalPathLength/F");
  tree->Branch("trdKElectronLikelihood", &trdKElectronLikelihood, "trdKElectronLikelihood/F");
  tree->Branch("trdKProtonLikelihood", &trdKProtonLikelihood, "trdKProtonLikelihood/F");
  tree->Branch("trdKHeliumLikelihood", &trdKHeliumLikelihood, "trdKHeliumLikelihood/F");
  /*}}}*/

  /**************************************************************************************************************************
   *
   * Begin of the event loop !!
   *
   **************************************************************************************************************************/

  unsigned int nProcessCheck = 10000;

  for(unsigned int e = 0; e < nEntries; e++)
  {
    AMSEventR* pev = NULL;
    pev = amsChain.GetEvent(e);

    // Basic cut processes
    if( IsBadRun(pev) )/*{{{*/
      break;
    if( !IsScienceRun(pev) ) continue;
    hEvtCounter->Fill(0);
    if( !IsHardwareStatusGood(pev) ) continue;
    hEvtCounter->Fill(1);
    if( IsUnbiasedPhysicsTriggerEvent(pev) ) continue;
    hEvtCounter->Fill(2);
    // Here we need to think about how to select a good particle among several particles.
    // In the study of deuteron flux, a particle need to be defined by the following several variables.
    //
    // 1. Momentum
    // 2. Charge
    //
    // To measure particle momentum correctly, track should be measured correctly.
    //
    // 04-02-15 : Currently, just use single particle case.
    if( pev->nParticle() != 1 ) continue;
    hEvtCounter->Fill(3);
    if( !IsTrkAlignmentGood(pev) ) continue;
    hEvtCounter->Fill(4);
    if( !IsGoodTrTrack(pev) ) continue;
    hEvtCounter->Fill(5);
    if( pev->IsInSAA() ) continue;
    hEvtCounter->Fill(6);/*}}}*/

    // ACSoft related lines
    particleFactory.SetAMSTrTrackR(trktrack);
    if( !amsRootSupport.SwitchToSpecificTrackFitById(id_maxspan) ) continue;
    Analysis::Event& event = amsRootSupport.BuildEvent(ams, pev);

    // Only do this if you need access to TRD segments/tracks and vertices
    eventFactory.PerformTrdTracking(event);
    eventFactory.PerformTrdVertexFinding(event);

    // If you want to use TrdQt, you should always add Analysis::CreateSplineTrack, so it can use the
    // Tracker track extrapolation to pick up the TRD hits in the tubes and calculate the path length
    // and also Analysis::FillTrdQt so that the likelihoods are calculated.
    // Additionally you shold pass Analysis::CreateTrdTrack if you want to examinge the TRD tracks
    // found by the previous PerformTrdTracking() call, via the Analysis::Particle interface.

    int productionSteps = 0;
    productionSteps |= Analysis::CreateSplineTrack;
    productionSteps |= Analysis::CreateTrdTrack;
    productionSteps |= Analysis::FillTrdQt;
    eventFactory.FillParticles(event, productionSteps);

    const Analysis::Particle* particle = event.PrimaryParticle();
    assert(particle);

    // Save data
    nRun            = pev->Run();
    nEvent          = pev->Event();
    nLevel1         = pev->nLevel1();
    nParticle       = pev->nParticle();
    nCharge         = pev->nCharge();
    nTrTrack        = pev->nTrTrack();
    nTrdTrack       = pev->nTrdTrack();
    nAntiCluster    = pev->nAntiCluster();
    nRichRing       = pev->nRichRing();
    nRichRingB      = pev->nRichRingB();
    nBeta           = pev->nBeta();
    nBetaB          = pev->nBetaB();
    nBetaH          = pev->nBetaH();
    nShower         = pev->nEcalShower();
    nVertex         = pev->nVertex();
    livetime        = pev->LiveTime();
    utcTime         = header->UTCTime(0);
    utcTimeError    = header->UTCTime(1);
    orbitAltitude   = header->RadS;
    orbitLatitude   = header->ThetaS;
    orbitLongitude  = header->PhiS;
    orbitLatitudeM  = header->ThetaM;
    orbitLongitudeM = header->PhiM;
    velR            = header->VelocityS;
    velTheta        = header->VelTheta;
    velPhi          = header->VelPhi;
    yaw             = header->Yaw;
    pitch           = header->Pitch;
    roll            = header->Roll;

    ParticleR* pParticle = pev->pParticle(0);
    ptlCharge       = (unsigned int)pParticle->Charge;
    ptlMomentum     = pParticle->Momentum;
    ptlTheta        = pParticle->Theta;
    ptlPhi          = pParticle->Phi;

    BetaHR* pBeta = pParticle->pBetaH();
    tofBeta = pBeta->GetBeta();
    if( pBeta->IsGoodBeta() == true ) isGoodBeta = 1;
    else isGoodBeta = 0;
    if( pBeta->IsTkTofMatch() == true ) isTkTofMatch = 11;
    else isTkTofMatch = 0;
    tofReducedChisqT = pBeta->GetNormChi2T();
    tofReducedChisqC = pBeta->GetNormChi2C();

    int ncls[4] = {0, 0, 0, 0};
    nTofClustersInTime = pev->GetNTofClustersInTime(pBeta, ncls);

    TrTrackR* pTrTrack = pParticle->pTrTrack();
    int id_maxspan     = pTrTrack->iTrTrackPar(1, 0, 0);
    int id_inner       = pTrTrack->iTrTrackPar(1, 3, 0);
    int id_fullspan    = pTrTrack->iTrTrackPar(1, 7, 0);

    // Tracker variables from maximum span setting
    trkFitCodeMS             = id_maxspan;
    trkRigidityMS            = pTrTrack->GetRigidity(id_maxspan);
    trkReducedChisquareMS    = pTrTrack->GetNormChisqX(id_maxspan);

    // Tracker variables from full span setting
    trkFitCodeFS             = id_fullspan;
    trkRigidityFS            = pTrTrack->GetRigidity(id_fullspan);
    trkReducedChisquareFS    = pTrTrack->GetNormChisqX(id_fullspan);

    // Tracker variables from inner tracker only setting
    trkFitCodeInner          = id_inner;
    trkRigidityInner         = pTrTrack->GetRigidity(id_inner);
    trkReducedChisquareInner = pTrTrack->GetNormChisqX(id_inner);

    TrRecHitR* pTrRecHit = NULL;          // This should be ParticleR associated hit.
    trkEdepLayerJ = 0;

    for(int ilayer = 0; ilayer < 9; ilayer++)
    {
      pTrRecHit = pTrTrack->GetHitLJ(ilayer);
      if(pTrTrack->GetEdep(0) != 0) trkEdepLayerJXSideOK[ilayer] = 1;
      else trkEdepLayerJXSideOK[ilayer] = 0;
      if(pTrTrack->GetEdep(1) != 0) trkEdepLayerJYSideOK[ilayer] = 1;
      else trkEdepLayerJYSideOK[ilayer] = 0;
      trkEdepLayerJ += pTrTrack->GetEdep(0) + pTrTrack->GetEdep(1);
    }
    trkCharge = pTrTrack->GetQ();
    trkInnerCharge = pTrTrack->GetInnerQ();

    RichRingR* richRing = pParticle->pRichRing();
    if(!richRing)
    {
      richRebuild               = -1;
      richIsGood                = -1;
      richIsClean               = -1;
      richIsNaF                 = -1;
      richRingWidth             = -1;
      richNHits                 = -1;
      richBeta                  = -1;
      richBetaError             = -1;
      richChargeSquared         = -1;
      richKolmogorovProbability = -1;
      richTheta                 = -9;
      richPhi                   = -9;
    }
    else
    {
      richRebuild   = (int)richRing->Rebuild();
      richIsGood    = (int)richRing->IsGood();
      richIsClean   = (int)richRing->IsClean();
      richIsNaF     = (int)richRing->IsNaF();
      richRingWidth = (float)richRing->RingWidth();
      richNHits     = richRing->getHits();
      richBeta      = richRing->getBeta();
      richBetaError = richRing->GetBetaError();
      richChargeSquared = richRing->getCharge2Estimate();
      richKolmogorovProbability = richring->getProb();
      richTheta = richRing->GetTrackTheta();
      richPhi = richRing->GetTrackPhi();
    }

    TrdTrackR* trdTrack = pParticle->pTrdTrack();
    trdNCluster = pev->nTrdCluster();
    trdNTracks  = pev->nTrdTrack();
    if(!trdTrack)
    {
      trdTrackTheta     = -9.;
      trdTrackPhi       = -9.;
      trdTrackPattern   = -9;
      trdTrackCharge    = -9;
      trdTrackEdep      = -9.;
    }
    else
    {
      trdTrackTheta = trdTrack->Theta;
      trdTrackPhi   = trdTrack->Phi;
      trdTrackPattern = trdTrack->Pattern;
      trdTrackCharge  = trdTrack->Q;
      trdTrackMeanDepositedEnergy = 0.;
      for(int i = 0; i < trdTrack->NTrdSegment(); i++)
      {
        for(int j = 0; j < trdTrack->pTrdSegment(i)->NTrdCluster(); j++)
        {
          trdTrackTotalDepositedEnergy += trdTrack->pTrdSegment(i)->pTrdCluster(j)->EDep;
        }
      }
    }

    const Analysis::TrdQt* trdQtFromTrackerTrack = particle->GetTrdQtInfo();
    trdQtIsCalibrationGood = trdQtFromTrackerTrack->IsCalibrationGood();
    trdQtIsSlowControlDataGood = trdQtFromTrackerTrack->IsSlowControlDataGood();
    trdQtIsInsideTrdGeometricalAcceptance = kTRUE;
    trdQtIsValid = 1;
    trdQtActiveStraws = 1;
    trdQtActiveLayers = 1;
    trdQtElectronToProtonLogLikelihoodRatio = -1.;
    trdQtHeliumToProtonLogLikelihoodRatio = -1.;
    trdQtElectronToHeliumLogLikelihoodRatio = -1.;

    const std::vector<Analysis::TrdHit>& TrdHit = trdQtFromTrackerTrack->GetAssignedHits();

    const std::vector<Analysis::TrdVertex>& verticesXZ = event.TrdVerticesXZ();
    const std::vector<Analysis::TrdVertex>& verticesYZ = event.TrdVerticesYZ();

    for( std::vector<Analysis::TrdVertex>::const_iterator xzIter = verticesXZ.begin(); xzIter != verticesXZ.end(); ++xzIter)
    {
      const Analysis::TrdVertex& xzVertex = *xzIter;
      for( std::vector<Analysis::TrdVertex>::const_iterator yzIter = verticesYZ.begin(); yzIter != verticesYZ.end(); ++yzIter)
      {
        const Analysis::TrdVertex& yzVertex = *yzIter;
        if( std::max(xzVertex.NumberOfSegments(), yzVertex.NumberOfSegments() ) < 3)
          continue;
        if( fabs(xzVertex.Z() - yzVertex.Z() ) < fabs(xzVertex.ErrorZ() + yzVertex.ErrorZ() ))
        {
          trdQtNTRDVertex++;
        }
      }
    }

    trdQtElectronToProtonLogLikelihoodRatio = (float)particle->CalculateElectronProtonLikelihood();
    trdQtHeliumToElectronLogLikelihoodRatio = (float)particle->CalculateHeliumElectronLikelihood();
    trdQtHeliumToProtonLogLikelihoodRatio = (float)particle->CalculateHeliumProtonLikelihood();

    if( e % nProcessCheck == 0 || e == nEntries - 1 )
      cout << "[" << releaseName << "] Processed " << e << " out of " << nEntries << " (" << (float)e/nEntries*100. << "%)" << endl;

    nProcessedNumber = nProcessed;
    tree->Fill();
    nProcessed++;
  }

  if( resultFile->Write() ) cout << "[" << releaseName << "] The result file [" << resultFile->GetName() << "] is successfully written." << endl;
  resultFile->cd("/");
  if( hEvtCounter->Write() ) cout << "[" << releaseName << "] The counter histogram is successfully written." << endl;
  resultFile->Close();

  cout << "[" << releaseName << "] The program is terminated successfully. " << nProcessed << " events are stored." << endl;
  return 0;
}

bool IsBadRun(AMSEventR* thisEvent)
{
  int tag = thisEvent->IsBadRun("");
  if( tag == 0 ) return false;
  if( tag == 1 ) return true;
  if( tag == 2 ) { printf("Run:%d Ev:%d -> Can not access to root_setup. Skipping this  event\n", (int)pev->Run(), (int)pev->Event() ); }
}

bool IsGoodParticle(ParticleR* thisParticle)
{
  return true;
}

unsigned int GetParticleType(ParticleR* thisParticle)
{
  // AMS Particle types : (from https://ams.cern.ch/AMS/Analysis/hpl3itp1/root02_v5/html/developmet/html/classParticleR.html)
  // - "Normal" Particle:
  //   a. Derived from ChargeR, BetaR and TrTrackR objects
  //   b. Has Charge, Rigidity, Velocity and DirCos properties set up
  //   c. Has fBeta, fCharge, fTrTrack set up
  //   d. Optionally has fTrdTrack set up in case TrdTrackR was found
  //   e. Optionally has fEcalShower set up in case EcalShowerR was found
  //   f. Optionally has fRichRing set up in case Rich was used in velocity determination
  // - Particle without TrTrackR:
  //   a. Derived from ChargeR, BetaR and optionally TrdTrack objects
  //   b. Has rigidity set up to 100000000 GeV (10^8 GeV)
  //   c. Has fBeta, fCharge set up
  //   d. fTrTrack set to -1
  //   e. Optionally has fTrdTrack set up in case TrdTrackR was found
  //   f. Optionally has fRichRing setted up in case Rich was used in velocity determination
  //   Optionally has fEcalShower set up in case EcalShowerR was found
  // - Particle based on EcalShower object:
  //   a. Derived from EcalShowerR (Momentum, DirCos);
  //   b. fBeta, fcharge, fTrTrack, fTrdTrack and fRichRing set to -1
  //   c. Velocity set to +/-1 depend on shower direction
  //   d. Two particles are in fact created with charge set to +/-1
  // - Particle based on VertexR (i.e. converted photon candidate or electron/positron ):
  //   a. fTrTrack set to -1
  //   b. fVertex set up
  //   c. Charge set to 0 or +/-1
  //   d. Velocity may or may not be set depending on fBeta index

  if(thisParticle->pCharge() && thisParticle->pBeta() && thisParticle->pTrTrack())
    return 1;  // Normal particle
  else if( thisParticle->pCharge() && thisParticle->pBeta() && !thisParticle->pTrTrack() )
    return 2;  // Particle without TrTrackR
  else if( thisParticle->pEcalShower() && !thisParticle->pCharge()
      && !thisParticle->pBeta() && !thisParticle->pTrTrack()
      && !thisParticle->pTrdTrack() && thisParticle->pRichRing() )
  {
    return 3;  // Particle based on EcalShower
  }
  else if( !thisParticle->pTrTrack() && thisParticle->pVertex() )
    return 4;  // Particle based on VertexR
}

bool IsHardwareStatusGood(AMSEventR* thisEvent)
{
  return false;
}

bool IsUnbiasedPhysicsTriggerEvent(AMSEventR* thisEvent)
{
  return false;
}

bool IsTrkAlignmentGood(AMSEventR* thisEvent)
{
  return false;
}

bool IsGoodTrTrack(AMSEventR* thisEvent)
{
  return false;
}

bool Initialize()
{
  richRebuild = -1;
  richIsGood = -1;
  richIsClean = -1;
  richIsNaF = -1;
  richRingWidth = -1;
  richNHits = -1;
  richBeta = -1;
  richBetaError = -1;
  richChargeSquared = -1;
  richKolmogorovProbatility = -1;
  richTheta = -9;
  richPhi = -9;

  trdTrackTheta = -99;
  trdTrackPhi = -99;
  trdTrackPattern = -9;
  trdTrackCharge = -9;
  trdTrackMeanDepositedEnergy = -9;
}
