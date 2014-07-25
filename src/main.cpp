#include <stdlib.h>   
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sapdecoder.h"

int main(int argc, char *argv[] )
{
	char *infilename= NULL, *outfilename = NULL;
	//surround effect,0 for virtual surrounding,1 for 5.1 blind surrounding
	//enviroment effect,0 for music,1 for live,2 for movie
	int nSurround = 0, nEffect = 0;
	int outChannel = 2,outEffect = 0;

	fprintf(stderr, "***** Spatial Acoustic Parameter Decoder Test*****\n");
	
	///arg operation
	if(argc == 7 && !strcmp(argv[1],"-i") && !strcmp(argv[3],"-o") && !strcmp(argv[5],"-s")) {
		infilename = argv[2];
		outfilename = argv[4];
		nSurround = atoi(argv[6]);
	}
	else if(argc == 9 && !strcmp(argv[1],"-i") && !strcmp(argv[3],"-o") && !strcmp(argv[5],"-s") && !strcmp(argv[7],"-e")){
		infilename = argv[2];
		outfilename = argv[4];
		nSurround = atoi(argv[6]);
		nEffect = atoi(argv[8]);
	}
	else{
		fprintf(stderr, "Command line switches:\n");
		fprintf(stderr, "-i <filename>   filename of the 2-channel input audio file\n");
		fprintf(stderr, "-o <filename>   filename of the 2/5-channel output audio file\n");
		fprintf(stderr, "-s <int>        surround effect,0 for virtual surrounding,1 for 5.1 blind surrounding\n");
		fprintf(stderr, "-e <int>        enviroment effect,0 for music,1 for live,2 for movie\n");
		fprintf(stderr, "Example: %s -i in.ogg -o out.wav -s 0 -e 0\n", argv[0]);
		fprintf(stderr, "Example: %s -i in.ogg -o out.wav -s 1 \n", argv[0]);
		return -1;
	}
	
	outEffect  = nEffect > 0 && nEffect < 3 ?  nEffect : 0;
	outChannel =   nSurround >0 ? 6 : 2;
	///init decoder
	SAPDecoder *decoder = new SAPDecoder();

	decoder->OpenDecoder();

	//set decoder effect
	decoder->SetDecodeParam(outChannel,outEffect);
	///pipe output if outfilename is null 
	decoder->ProcessDecoding(infilename,outfilename);

	///close the decoder;
	decoder->CloseDecoder();
	delete decoder;

	printf("Decoding finished successfully!\n");
	printf("Please input any key to exit!\n");
	getchar();
	return 1;
}


