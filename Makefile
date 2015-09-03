CXX = g++
CXXFLAGS = -W -Wall -Wno-unused-parameter -Wno-unknown-pragmas -Wno-extra -DAMS_ACQT_INTERFACE -D_PGTRACK_

# AMS Global Environment
CVMFS_AMS_OFFLINE = /cvmfs/ams.cern.ch/Offline

# CERN libraries
CERNLIBS = -lgeant321 -lpacklib -lmathlib -lkernlib
CERN_LEVEL = 2005.gcc64

ifndef CERNDIR
CERNDIR = $(CVMFS_AMS_OFFLINE)/CERN/$(CERN_LEVEL)
endif

CERNSRCDIR = $(CERNDIR)

ifndef AMSLIB
AMSLIB = /$(CVMFS_AMS_OFFLINE)/lib/linux/gcc64
endif

ifndef NAGDIR
NAGDIR = $(CVMFS_AMS_OFFLINE)/CERN/NagLib
endif
############ End of CERN library settings

# AMS Offline Software Related Includes
INCLUDES = -I. -I${ROOTSYS}/include -I${AMSWD}/include
NTUPLE_PG = $(AMSWD)/lib/linuxx8664gcc5.34/ntuple_slc6_PG.so
############ End of AMS Offline Software related Includes

# ROOT Related Settings
ROOTLIBS = $(shell root-config --libs) -lASImage -lRIO -lNet -lNetx -lMinuit -lTMVA -lMLP -lXMLIO -lTreePlayer
############ End of ROOT related settings

# ACSOFT Flags
ACSOFTFLAGS = `acsoft-config --definitions` `acsoft-config --cflags` `acsoft-config --auxcflags` `acsoft-config --libs` `acsoft-config --auxlibs`

TARGET = bin/main

all : $(TARGET)

$(TARGET) : obj/main.o obj/selector.o
	$(CXX) $(CXXFLAGS) $(ACSOFTFLAGS) $(ROOTLIBS) $(INCLUDES) -o $@ $^ $(NTUPLE_PG)

obj/selector.o : src/selector.cxx
	$(CXX) $(CXXFLAGS) $(ACSOFTFLAGS) $(ROOTLIBS) $(INCLUDES) -c -o $@ $^

obj/main.o : src/main.cxx
	$(CXX) $(CXXFLAGS) $(ACSOFTFLAGS) $(ROOTLIBS) $(INCLUDES) -c -o $@ $^

clean :
	rm -rf obj/*.o bin/main

