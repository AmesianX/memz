/**
 * Main program
 */
#include <cstdio>
#include <string>
#include <algorithm>
#include <vector>

//#include <iostream>
//#include <cassert>

#include "rewriter.h"

InstructionRewriter::InstructionRewriter(string binary_name) {
   /**
    *
    */
   if (!Symtab::openFile(symtab_obj, binary_name)) {
      fprintf(stderr, "> Couldn't open binary!\n");
      exit(EXIT_FAILURE);
   }

   // Try getting a handle on the DEFAULT_MODULE
   if (!symtab_obj->findModuleByName(module, "DEFAULT_MODULE")) {
      fprintf(stderr, "> Couldn't find DEFAULT_MODULE\n");
      exit(EXIT_FAILURE);
   }

   // Try to find the '.data' region
   if (!symtab_obj->findRegion(data_region, ".data")) {
      fprintf(stderr, "> Couldn't find .data region!\n");
      exit(EXIT_FAILURE);
   }
   if (!symtab_obj->findRegion(text_region, ".text")) {
      fprintf(stderr, "> Couldn't find .text region!\n");
      exit(EXIT_FAILURE);
   }

   describeRegion(data_region);
   describeRegion(text_region);

   //map<unsigned int, struct DataRegionEntryInfo> originalAddressMap = describeData();
   //map<unsigned int, struct DataRegionEntryInfo>::const_iterator end = originalAddressMap.end();
   //for (map<unsigned int, struct DataRegionEntryInfo>::const_iterator it = originalAddressMap.begin();
   //      it != end;
   //      ++it) {
   //  fprintf(stderr, "%ld\n", (long) it->first);
   //}

   void* raw_text = text_region->getPtrToRawData();
   long unsigned int raw_text_size = text_region->getRegionSize();

   void* dotTextRawMuta = malloc(text_region->getRegionSize());
   memcpy(dotTextRawMuta, raw_text, text_region->getRegionSize());

   InstructionDecoder decoder(raw_text, raw_text_size, Arch_x86_64);
   Instruction::Ptr i = decoder.decode();

   long unsigned int currentTextOffset = 0;
   while (i != NULL) {

      //vector<Operand> operands;
      //i->getOperands(operands);
      //if (operands.size() > 0) {
      //  Expression::Ptr exp = operands[0].getValue();
      //  Result res = exp->eval();
      //  Immediate::makeImmediate(Result(u64, 2^32));
      //  fprintf(stderr, "results: %s\n", res.format().c_str());
      //}
      //fprintf(stdout, "%i bytes > %s\n", i->size(), i->format().c_str());

      unsigned char* dotTextRaw = (unsigned char*) i->ptr();

      if (dotTextRaw[0] == 0xa1 && i->size() == 9) {
         fprintf(stdout, "%i bytes > %s\n", i->size(), i->format().c_str());
         unsigned int* dataOperand = (unsigned int*)(dotTextRaw + 1);
         if (dataOperand[0] > 134518284) {
            //((unsigned int*)(((unsigned char*) __dot_text_muta) + underflow + 1))[0] = 134518520;
            ((unsigned int*)(((unsigned char*) dotTextRawMuta) + currentTextOffset + 1))[0] = 134518520;
         }
      }

      //fprintf(stderr, "<");
      //for (int byte_index = 0; byte_index < i->size(); byte_index++) {
      //  fprintf(stderr, " %x ", dotTextRaw[byte_index]);
      //}
      //fprintf(stderr, ">\n");

      currentTextOffset += i->size();
      i = decoder.decode();
   }

   // Assign the data region to point at the new buffer
   if (!text_region->setPtrToRawData((void*) dotTextRawMuta, text_region->getRegionSize())) {
      fprintf(stderr, "Failed to set pointer to raw text!\n");
      exit(EXIT_FAILURE);
   }

   // Emit the binary
   if (symtab_obj->emit(binary_name + "-mutated")) {
      fprintf(stderr, "Emit success\n");
   } else {
      fprintf(stderr, "Emit failure\n");
   }

   return;
}


map<unsigned int, struct DataRegionEntryInfo> InstructionRewriter::describeData() {
   /**
    *
    */
   map<unsigned int, struct DataRegionEntryInfo> originalAddressMap;

   VarList globals = collectGlobalVariables(data_region);

   // This is the current/original data region
   unsigned int* raw = NULL;
   unsigned int dataBase = 0;
   unsigned int dataOffset = 0;
   unsigned int rawIndex = 0;

   raw = (unsigned int*) data_region->getPtrToRawData();
   dataBase = data_region->getRegionAddr();

   fprintf(stderr, "Region offset = %p\n", (void*) dataBase);
   for (int i = 0; i < globals.size(); i++) {
      dataOffset = globals[i]->getOffset();
      // Raw data that we're interested in isn't always the first series of bytes,
      // so calculate the offset to the actual data each time.
      rawIndex = (dataOffset - dataBase) / 4;
      // Build up one of these and save it in the map
      struct DataRegionEntryInfo data;
      data.prettyName = globals[i]->getAllPrettyNames()[0];
      data.size = globals[i]->getSize();
      data.value = raw[rawIndex];
      originalAddressMap[dataOffset] = data;
      // Show what we did
      fprintf(stderr, "%p: %s (%d) size (%d)\n", dataOffset, data.prettyName.c_str(), data.value, data.size);
   }
   return originalAddressMap;

   // This is the current/original data region
   //unsigned int* raw = (unsigned int*) data_region->getPtrToRawData();

   //unsigned long regionSize = data_region->getRegionSize();

   //for (int i = 0; i < regionSize / 4; i++) {
   //  fprintf(stderr, "Data: %d\n", raw[i]);
   // }

   // We make a copy of it
   //char* orig_raw = (char*) malloc(sizeof(char) * regionSize);
   //memcpy(orig_raw, raw, data_region->getRegionSize());

   // This will be the new data region
   //char* new_raw = (char*) malloc (sizeof(char) * regionSize);
   //memcpy(new_raw, orig_raw, data_region->getRegionSize());
}


void InstructionRewriter::describeRegion(Region* reg) {
   /**
    *
    */
   if (reg != NULL) {
      fprintf(stderr, "Region (%s):\n", reg->getRegionName().c_str());
      fprintf(stderr, "\tgetRegionAddr(): %p\n", (void*) reg->getRegionAddr());
      fprintf(stderr, "\tgetRegionSize(): %ld\n", reg->getRegionSize());
      fprintf(stderr, "\tgetDiskOffset(): %p\n", (void*) reg->getDiskOffset());
      fprintf(stderr, "\tgetDiskSize(): %ld\n", reg->getDiskSize());
      fprintf(stderr, "\tgetPtrToRawData(): %p\n", (void*) reg->getPtrToRawData());
      fprintf(stderr, "\n");
   }
   else {
      fprintf(stderr, "Region is NULL\n");
   }
}


VarList InstructionRewriter::collectGlobalVariables(Region* reg) {
   /**
    *
    */
   VarList all_variables;
   VarList data_variables;
   if (symtab_obj->getAllVariables(all_variables)) {
      for (VarList::iterator it = all_variables.begin(); it != all_variables.end(); ++it) {
         Variable* var = *it;
         // If it is in the appropriate region (string comparison)
         if (var->getRegion() != NULL
               && var->getRegion()->getRegionName().compare(reg->getRegionName()) == 0) {
            // Save a reference to this variable
            data_variables.push_back(var);
         }
      }
   }
   return data_variables;
}



