#ifndef __AMSINC__
#define __AMSINC__
#include "amschain.h"
#include "selector.h"
#endif


/**
 * @brief   This function checks whether current event contained in bad-run.
 * @return  true : The event is contained in bad-run. (Therefore, it should be rejected.) / false : The event is good to analyze further.
 */
bool IsBadRun(AMSEventR* thisEvent)
{/*{{{*/
  unsigned int runNumber = (unsigned int)thisEvent->Run();
  if( thisEvent->fHeader.Run == 1306219312 || thisEvent->fHeader.Run == 1306219522 || thisEvent->fHeader.Run == 1306233745 )
  {
    printf("The program is aborted due to input run: %d is a bad run(1).\n", runNumber);
    exit(0);
  }
  else if( thisEvent->fHeader.Run >= 1307125541 && thisEvent->fHeader.Run <= 1307218054 )
  {
    printf("The program is aborted due to input run: %d is a bad run(2).\n", runNumber);
    exit(0);
  }
  else if( thisEvent->fHeader.Run == 132119816 )
  {
    printf("The program is aborted due to input run: %d is a bad run(3).\n", runNumber);
    exit(0);
  }
  else if( thisEvent->isBadRun(thisEvent->fHeader.Run) )
  {
    printf("The program is aborted due to input run: %d is a bad run(4).\n", runNumber);
    exit(0);
  }
  else
    return false;
}/*}}}*/



/**
 * @brief This function checks whether the run containing input event is science-run or not.
 * @return true : The event is contained in science-run / false : The run containing input event is not a science-run
 */
bool IsScienceRun(AMSEventR* thisEvent)
{/*{{{*/
  HeaderR* header = &(thisEvent->fHeader);
  if( !header ) return false;
  if( ( header->RunType >> 12 ) != 0xf ) return false;
  return true;
}/*}}}*/



/**
 * @brief This function returns hardware status.
 * @return true : Hardware status is good / false : Hardware status is not good
 */
bool IsHardwareStatusGood(AMSEventR* thisEvent)
{/*{{{*/
  int nDaqEvent = thisEvent->nDaqEvent();
  bool goodHW = true;
  bool error = false;

  for(int i = 0; i < nDaqEvent; i++)
  {
    DaqEventR* daqEvent = thisEvent->pDaqEvent(i);
    for( int iJINJ = 0; iJINJ < 4; iJINJ++ ) error |= (bool)( ( daqEvent->JINJStatus[iJINJ]>>8 ) & 0x7F );
    for( int iJErr = 0; iJErr < 24;iJErr++ ) error |= (bool)(   daqEvent->JError[iJErr] & 0x7F );
    if(error) goodHW &= false;
  }

  return goodHW;
}/*}}}*/



/**
 * @brief This function checks whether the event triggered by unbiased physics trigger or not.
 * @return true : The trigger is unbiased physics trigger / false : The trigger is physics trigger
 */
bool IsUnbiasedPhysicsTriggerEvent(AMSEventR* thisEvent)
{/*{{{*/
  int nLevel1 = thisEvent->nLevel1();
  bool unbiased = false;

  for(int i = 0; i < nLevel1; i++)
  {
    Level1R* pLevel1 = thisEvent->pLevel1(i);
    bitset<8> physicsBitPattern(pLevel1->PhysBPatt);

    if(physicsBitPattern.test(1) || physicsBitPattern.test(2) || physicsBitPattern.test(3) || physicsBitPattern.test(4) || physicsBitPattern.test(5))
      unbiased |= false;
    else
      unbiased |= true;
  }

  return unbiased;
}/*}}}*/


/**
 * @brief
 * @return
 */
bool IsACCPatternGood(AMSEventR* thisEvent)
{/*{{{*/
  int nACCHit = 0;
  int nLevel1 = thisEvent->nLevel1();

  for(int i = 0; i < nLevel1; i++)
  {
    Level1R* pLevel1 = thisEvent->pLevel1(i);
    for( int j = 0; j < 8; j++)
    {
      if(((pLevel1->AntiPatt>>i)&1)==1) nACCHit++;
    }
  }
  if( nACCHit > 5 ) return false;
  return true;
}/*}}}*/



/**
 * @brief
 * @return
 */
bool IsGoodBeta(AMSEventR* thisEvent)
{/*{{{*/
  ParticleR* pParticle = thisEvent->pParticle(0);
  BetaR* pBeta = pParticle->pBeta();

  if( pBeta->Pattern > 5 )
    return false;
  if( pBeta->Beta < 0.3 )
    return false;

  return true;
}/*}}}*/



/**
 * @brief
 * @return
 */
bool IsGoodLiveTime(AMSEventR* thisEvent)
{/*{{{*/
  return true;
}/*}}}*/



/**
 * @brief
 * @return
 */
bool IsInSouthAtlanticAnomaly(AMSEventR* thisEvent)
{/*{{{*/
  return false;
}/*}}}*/



/**
 * @brief
 * @return
 */
bool IsInSolarArrays(AMSEventR* thisEvent)
{/*{{{*/
  return false;
}/*}}}*/



/**
 * @brief
 * @return
 */
bool IsGoodTrTrack(AMSEventR* thisEvent)
{/*{{{*/
  bool debugMode = false;
  TrTrackR* pTrTrack = thisEvent->pParticle(0)->pTrTrack(); // Choose ParticleR associated TrTrack.

  if( !pTrTrack )
  {
    if( debugMode == true ) cerr << "No track pointer. Reject it." << endl;
    return false;
  }
  if( pTrTrack->IsFake() )
  {
    if( debugMode == true) cerr << "Found a fake track. Reject it." << endl;
    return false;
  }

  int id_maxspan, id_fullspan, id_inner;
  id_maxspan  = pTrTrack->iTrTrackPar(1, 0, 0);
  id_inner    = pTrTrack->iTrTrackPar(1, 3, 0);
  id_fullspan = pTrTrack->iTrTrackPar(1, 7, 0);

  if( id_fullspan < 0 )
  {/*{{{*/
    if( debugMode == true )
    {
      switch(id_fullspan)
      {
        case -1: cerr << "[FS]The requested fit cannot be performed on this track." << endl; break;
        case -2: cerr << "[FS]The requested fit it is not available without refitting." << endl; break;
        case -3: cerr << "[FS]The refit failed." << endl; break;
        case -4: cerr << "[FS]Should not happen!! Contact the developers!." << endl; break;
        case -5: cerr << "[FS]The refit failed because there was a problem retrieving dynamic alignment for Ext Layers" << endl; break;
        default: break;
      }
    }

    return false;
  }/*}}}*/

  if( id_maxspan < 0 )
  {/*{{{*/
    if( debugMode == true )
    {
      switch( id_maxspan )
      {
        case -1: cerr << "[MS]The requested fit cannot be performed on this track." << endl; break;
        case -2: cerr << "[MS]The requested fit it is not available without refitting." << endl; break;
        case -3: cerr << "[MS]The refit failed." << endl; break;
        case -4: cerr << "[MS]Should not happen!! Contact the developers!." << endl; break;
        case -5: cerr << "[MS]The refit failed because there was a problem retrieving dynamic alignment for Ext Layers" << endl; break;
        default: break;
      }
    }

    return false;
  }/*}}}*/

  if( id_inner < 0 )
  {/*{{{*/
    if( debugMode == true )
    {
      switch( id_inner )
      {
        case -1: cerr << "[IN]The requested fit cannot be performed on this track." << endl; break;
        case -2: cerr << "[IN]The requested fit it is not available without refitting." << endl; break;
        case -3: cerr << "[IN]The refit failed." << endl; break;
        case -4: cerr << "[IN]Should not happen!! Contact the developers!." << endl; break;
        case -5: cerr << "[IN]The refit failed because there was a problem retrieving dynamic alignment for Ext Layers" << endl; break;
        default: break;
      }
    }

    return false;
  }/*}}}*/

  bool hitOnLayerJ[9];
  for(int i = 0; i < 9; i++) hitOnLayerJ[i] = pTrTrack->TestHitBitsJ(i, id_fullspan);

  if( !hitOnLayerJ[1] ) return false;
  if( !(hitOnLayerJ[2] || hitOnLayerJ[3]) ) return false;
  if( !(hitOnLayerJ[4] || hitOnLayerJ[5]) ) return false;
  if( !(hitOnLayerJ[6] || hitOnLayerJ[7]) ) return false;

  return true;
}/*}}}*/


/**
 * @brief
 * @return
 */
bool IsShowerTrackMatched(AMSEventR* thisEvent)
{/*{{{*/
  return true;
}/*}}}*/



/**
 * @brief
 * @return
 */
bool IsTrkAlignmentGood(AMSEventR* thisEvent)
{/*{{{*/
  AMSPoint pn1, pn9, pd1, pd9;
  thisEvent->GetRTIdL1L9(0, pn1, pd1, thisEvent->UTime(), 60);
  thisEvent->GetRTIdL1L9(1, pn9, pd9, thisEvent->UTime(), 60);
  if(pd1.y() > 35 || pd9.y() > 45)
    return false;
  else
    return true;
}/*}}}*/
