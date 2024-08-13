#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/5257efb8/ht_main.o \
	${OBJECTDIR}/_ext/511e4115/hash_file.o \
	${OBJECTDIR}/_ext/511e4115/hp_file.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

# Test Object Files
TESTOBJECTFILES= \
	${TESTDIR}/_ext/5257efb8/hp_main.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../lib/libbf.so

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netbeans_project

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netbeans_project: ../lib/libbf.so

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netbeans_project: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netbeans_project ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/_ext/5257efb8/ht_main.o: ../examples/ht_main.c
	${MKDIR} -p ${OBJECTDIR}/_ext/5257efb8
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5257efb8/ht_main.o ../examples/ht_main.c

${OBJECTDIR}/_ext/511e4115/hash_file.o: ../src/hash_file.c
	${MKDIR} -p ${OBJECTDIR}/_ext/511e4115
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511e4115/hash_file.o ../src/hash_file.c

${OBJECTDIR}/_ext/511e4115/hp_file.o: ../src/hp_file.c
	${MKDIR} -p ${OBJECTDIR}/_ext/511e4115
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511e4115/hp_file.o ../src/hp_file.c

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-tests-subprojects .build-conf ${TESTFILES}
.build-tests-subprojects:

${TESTDIR}/TestFiles/f1: ${TESTDIR}/_ext/5257efb8/hp_main.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.c} -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS}   ../lib/libbf.so 


${TESTDIR}/_ext/5257efb8/hp_main.o: ../examples/hp_main.c 
	${MKDIR} -p ${TESTDIR}/_ext/5257efb8
	${RM} "$@.d"
	$(COMPILE.c) -g -Wall -I../include -I. -MMD -MP -MF "$@.d" -o ${TESTDIR}/_ext/5257efb8/hp_main.o ../examples/hp_main.c


${OBJECTDIR}/_ext/5257efb8/ht_main_nomain.o: ${OBJECTDIR}/_ext/5257efb8/ht_main.o ../examples/ht_main.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/5257efb8
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/5257efb8/ht_main.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Wall -I../include -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/5257efb8/ht_main_nomain.o ../examples/ht_main.c;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/5257efb8/ht_main.o ${OBJECTDIR}/_ext/5257efb8/ht_main_nomain.o;\
	fi

${OBJECTDIR}/_ext/511e4115/hash_file_nomain.o: ${OBJECTDIR}/_ext/511e4115/hash_file.o ../src/hash_file.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/511e4115
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/511e4115/hash_file.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Wall -I../include -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511e4115/hash_file_nomain.o ../src/hash_file.c;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/511e4115/hash_file.o ${OBJECTDIR}/_ext/511e4115/hash_file_nomain.o;\
	fi

${OBJECTDIR}/_ext/511e4115/hp_file_nomain.o: ${OBJECTDIR}/_ext/511e4115/hp_file.o ../src/hp_file.c 
	${MKDIR} -p ${OBJECTDIR}/_ext/511e4115
	@NMOUTPUT=`${NM} ${OBJECTDIR}/_ext/511e4115/hp_file.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -g -Wall -I../include -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/511e4115/hp_file_nomain.o ../src/hp_file.c;\
	else  \
	    ${CP} ${OBJECTDIR}/_ext/511e4115/hp_file.o ${OBJECTDIR}/_ext/511e4115/hp_file_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} -r ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libbf.so
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/netbeans_project

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
