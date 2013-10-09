#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "KeywordTree.h"

//second part of stream and count program
//assumes that the input pattern set has been preprocessed into a keyword tree and serialized to a file <%s_%d-mers_KWTREE_INFO>
//parameters - full path and name of the patterns file,  full path and prefix of numbered input files, input file index, value of k
int 
main(int argc, char *argv[])
{
	char patternFileName [MAX_PATH_LENGTH];
	char inputFileNamePrefix [MAX_PATH_LENGTH];
	int k, i;
	int fileID;
	KWTCounterManager manager; //bookkeper
	char currentFileName[MAX_PATH_LENGTH];
	KWTreeInfo newinfo[1];
	FILE *inputFP, *outputFP;
	int read, written;

	if(argc<4)
	{
		printf("./streamandcountonefile <patterns file name fullpath> <input file name prefix full path> <input file number> <size of k-mers> \n");
		return 1;
	}
	
	sprintf(patternFileName,"%s", argv[1]);
	sprintf(inputFileNamePrefix,"%s", argv[2]);
	fileID =atoi(argv[3]);
	k=atoi(argv[4]);

	//try to read KWtree info from file 
	sprintf(currentFileName, "%s_%d-mers_KWTREE_INFO", patternFileName, k);
	if(!(inputFP= fopen ( currentFileName , "rb" )))
	{
		printf("Could not open file \"%s\" for reading saved KWtree info \n", currentFileName);
		return 1;
	}
		
	read= fread (newinfo,sizeof(KWTreeInfo),1,inputFP);
	if(read!=1)
	{
		fclose(inputFP);
		printf("Error reading KWTinfo from file\n");
		return 1;
	}
	fclose(inputFP);

	//print new tree parameters read from file
	printf("Read from file %s the following: totalNodes = %d, k=%d, totalPatterns=%d\n",
		currentFileName,newinfo[0].treeSlotsNum,newinfo[0].k,newinfo[0].totalPatterns);

	//init general fields of the manager
	manager.treeSlotsNum = newinfo[0].treeSlotsNum;
	manager.k = newinfo[0].k;
	manager.totalPatterns = newinfo[0].totalPatterns;
	sprintf(manager.inputFileNamePrefix, "%s", inputFileNamePrefix); //to be able to process more than 1 file

	//allocate memory for KWT tree
	if(!(manager.KWTree =(KWTNode *)calloc(manager.treeSlotsNum,sizeof (KWTNode))))
	{
		printf("Unable to allocate memory to load keyword tree.\n");
		return 1;
	}

	//open file for reading a tree into RAM
	sprintf(currentFileName, "%s_%d-mers_KWTREE", patternFileName, manager.k);
	if(!(inputFP= fopen ( currentFileName , "rb" )))
	{
		printf("Could not open file \"%s\" for reading saved KWtree \n", currentFileName);
		return 1;
	}
	
	//read tree from file
	read= fread (&(manager.KWTree[0]),sizeof(KWTNode),manager.treeSlotsNum,inputFP);
	if(read!=manager.treeSlotsNum)
	{
		printf("Error reading KWT tree from file: wanted to read %d nodes but read %d\n", manager.treeSlotsNum, read);
		return 1;
	}
	fclose(inputFP);

	//allocate memory for counters
	if(!(manager.patternCounts =(UINT *)calloc(manager.totalPatterns,sizeof (UINT))))
	{
		printf("Unable to allocate memory to hold %d counters.\n",manager.totalPatterns );
		return 1;
	}

	//counting in a specific input file specified by program arguments
	manager.currentFileID = fileID;

	//now we are ready - KWtree is in memory
	//stream each line of the input file through kwtree and update counts
	if(streamAndCountOneFile(&manager)!=0)
		return 1;	

	//output counters to a file
	sprintf(currentFileName, "%s%d-file_%d-mers_COUNTS", manager.inputFileNamePrefix, manager.currentFileID,manager.k);
	if(!(outputFP= fopen ( currentFileName , "wb" )))
	{
		printf("Could not open file \"%s\" for writing counts \n", currentFileName);
		return 1;
	}

	written = fwrite (&(manager.patternCounts[0]) , sizeof(UINT), manager.totalPatterns, outputFP);

	if(written!=manager.totalPatterns)
	{
		printf("Not all counters have been written: wanted to write %d, and wrote %d \n",manager.totalPatterns,written);
		return 1;
	}
	fclose(outputFP);

	if(DEBUG_COUNT)
	{
		if(!(inputFP= fopen ( currentFileName , "rb" )))
		{
			printf("Could not open file \"%s\" for reading saved counts \n", currentFileName);
			return 1;
		}
	
		//read tree from file
		read= fread (&(manager.patternCounts[0]),sizeof(UINT),manager.totalPatterns,inputFP);
		if(read!=manager.totalPatterns)
		{
			printf("Error reading counts back from file: wanted to read %d nodes but read %d\n", manager.totalPatterns, read);
			return 1;
		}
		fclose(inputFP);

		//print counts
		for(i=1;i<manager.totalPatterns;i++)
			printf("Count for pattern %d: %u \n",i, manager.patternCounts[i]);
	}

	//free allocated memory
	free(manager.patternCounts);
	free(manager.KWTree);
	return 0;
}
