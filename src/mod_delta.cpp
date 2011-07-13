/*******************************************************************************
 Delta/Framediff module
*******************************************************************************/

#include "main.h"


/*******************************************************************************
 Encode stage
*******************************************************************************/
DeltaEncodeModule::DeltaEncodeModule(){

}

bool DeltaEncodeModule::process(vector<Instruction *> *list){
	//LOG("encode %d / %d\n", lastFrame.size(), list->size());
	
	if(lastFrame.size() == 0){
		//this must be the first frame
		mListResult = list;
		lastFrame = *list;
		LOG("Initial frame\n");
		return true;
	}
	
	LOG("Delta Start\n");
	
	vector<Instruction *> *result = new vector<Instruction *>();
		
	int sameCount = 0;
		
	for(int n=0;n<(int)list->size();n++){		
		Instruction *instr = (*list)[n];
		
		if((int)lastFrame.size() > n){		
			Instruction *last = lastFrame[n];
			
			if(instr->compare(last)){
				sameCount++;
				
				//LOG_INSTRUCTION(instr);
				//LOG_INSTRUCTION(last);
				//LOG("SAME\n\n");
				
			}else{
			
				//LOG_INSTRUCTION(instr);
				//LOG_INSTRUCTION(last);
				//LOG("DIFFERENT\n");				
				LOG("%d same\n", sameCount);
				sameCount = 0;
			}
		}else{
			//LOG("CompareOverflow: %d/%d\n", n, lastFrame.size());
		}
		
		result->push_back(instr);
		
		
		
	}
	
	if(sameCount){
		LOG("%d same (end)\n", sameCount);
	}
	
	LOG("Delta end\n");
	
	lastFrame = *list;
	
	mListResult = result;		
		
	return true;
}

//output
vector<Instruction *> *DeltaEncodeModule::resultAsList(){
	return mListResult;
}






/*******************************************************************************
 Decode stage
*******************************************************************************/
bool DeltaDecodeModule::process(vector<Instruction *> *i){
	LOG("decode\n");
	return true;
}

//output
vector<Instruction *> *DeltaDecodeModule::resultAsList(){

}



/*
//Now send the instructions
	int counter = 0;
	for(std::list<Instruction>::iterator iter = list.begin(), 
	    pIter = (*prevFrame).begin();iter != list.end(); iter++) {								
		Instruction *i = &(*iter);
		
		bool mustSend = false;

		for(int n=0;n<3;n++) {
			if(i->buffers[n].len > 0) {
				//If we expect a buffer back then we must send Instruction
				if(i->buffers[n].needReply) mustSend = true;
			}
		};

		if (useRepeat			 //value from config to enable/disable deltas
			&& i->id == pIter->id//if the instruction has the same id as previous
			&& !mustSend && i->id//mustSend is set when expecting a reply
		//	&& sameCount < 150//stops sameBuffer filling up indefinitely (is this needed?)
			&& i->id != 1499
		) {
			//assume the instruction is the same
			bool same = true;
			//compare all arguments
			for (int a=0;a<MAX_ARG_LEN;a++) {
				if (i->args[a]!=pIter->args[a]) {
					same = false;
					break;
				}
			}

			//if arguments the same, compare all buffers
			if (same) {
				for (int a=0;a<3;a++) {
					if (i->buffers[a].len > 0) {
						if(i->buffers[a].len != pIter->buffers[a].len) {
							same = false;
							break;
						}
						else if (i->buffers[a].needClear != pIter->buffers[a].needClear) {
							same = false;
							break;
						}
						else if (memcmp(i->buffers[a].buffer,pIter->buffers[a].buffer,i->buffers[a].len) != 0) {
							same = false;
							break;
						}
					}
				}
			}

			//if arguments and buffers match, the instruction is identical
			if (same) {
				sameCount++;
				if (pIter != (*prevFrame).end())
					pIter++;
				continue;
			}
		}
		//printf("same count: %d\n", sameCount);
		if (sameCount> 0) {		 // send a count of the duplicates before this instruction
			Instruction * skip = (Instruction *)malloc(sizeof(Instruction));
			if (skip == 0) {
				LOG("ERROR: Out of memory\n");
				exit(-1);
			}
			skip->id = CGL_REPEAT_INSTRUCTION;
			memcpy(skip->args, &sameCount, sizeof(uint32_t));
			//printf("sameCount: %d\n", sameCount);
			for(int i=0;i<3;i++) {
				skip->buffers[i].buffer = NULL;
				skip->buffers[i].len = 0;
				skip->buffers[i].needClear = false;
			}
			if(multicast) {
				netBytes += sizeof(Instruction);
			}
			else {
				netBytes += sizeof(Instruction) * numConnections;
			}
			if(myWrite(skip, sizeof(Instruction))!= sizeof(Instruction)) {
				LOG("Connection problem (didn't send instruction)!\n");
				return false;
			}
			sameCount = 0;		 // reset the count and free the memory
			free(skip);
		}

		// now send the new instruction
		netBytes += sizeof(Instruction) * numConnections;
		if(myWrite(i, sizeof(Instruction)) != sizeof(Instruction)) {
			LOG("Connection problem (didn't send instruction)!\n");
			return false;
		}

		//Now see if we need to send any buffers
		for(int n=0;n<3;n++) {
			int l = i->buffers[n].len;

			if(l > 0) {
				netBytes += l * numConnections;
				if(myWrite(i->buffers[n].buffer, l) != l) {
					LOG("Connection problem (didn't write buffer %d)!\n", l);
					return false;
				}
				//And check if we're expecting a buffer back in response
				if(i->buffers[n].needReply) {

					sendBuffer();
					if(int x = myRead(i->buffers[n].buffer, l) != l) {
						LOG("Connection problem NetClient (didn't recv buffer %d got: %d)!\n", l, x);
						return false;
					}
					//LOG("got buffer back!\n");
				}
			}
		}
		if (pIter != (*prevFrame).end()) pIter++;
		counter++;
	}

	//send any instructions that are remaining in the CGL_REPEAT_INSTRUCTION buffer
	if (sameCount> 0) {			 // send a count of the duplicates before this instruction
		Instruction * skip = (Instruction *)malloc(sizeof(Instruction));
		if (skip == 0) {
			LOG("ERROR: Out of memory\n");
			exit(-1);
		}
		skip->id = CGL_REPEAT_INSTRUCTION;
		skip->args[0] = (uint32_t) sameCount;
		//printf("sameCount: %d\n", sameCount);
		for(int i=0;i<3;i++) {
			skip->buffers[i].buffer = NULL;
			skip->buffers[i].len = 0;
			skip->buffers[i].needClear = false;
		}
		netBytes += sizeof(Instruction) * numConnections;
		if(myWrite(skip, sizeof(Instruction))!= sizeof(Instruction)) {
			LOG("Connection problem (didn't send instruction)!\n");
			return false;
		}
		sameCount = 0;			 // reset the count and free the memory
		free(skip);
	}
	sendBuffer();
*/