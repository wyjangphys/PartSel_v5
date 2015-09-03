#ifndef _ROOT_
#define _ROOT_
#include "root.h"
#endif

class Selector
{
  TH1F* hEventCounter;
public:
  Selector();
  virtual ~Selector();

  bool IsBadRun(AMSEventR*);
  bool IsScienceRun(AMSEventR*);
  bool IsHardwareStatusGood(AMSEventR*);
  bool IsUnbiasedPhysicsTriggerEvent(AMSEventR*);
  bool IsACCPatternGood(AMSEventR*);
  bool IsGoodBeta(AMSEventR*);
  bool IsGoodLiveTime(AMSEventR*);
  bool IsInSouthAtlanticAnomaly(AMSEventR*);
  bool IsInSolarArrays(AMSEventR*);
  bool IsGoodTrTrack(AMSEventR*);
  bool IsShowerTrackMatched(AMSEventR*);
  bool IsTrkAlignmentGood(AMSEventR*);
};
