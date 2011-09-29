/**
 * Main program
 */
#include <cstdio>
#include <string>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cassert>

#include <Symbol.h>
#include <Module.h>
#include <Variable.h>
#include <InstructionDecoder.h>
//#include <Function.h>
//#include <Aggregate.h>
//#include <dyntypes.h>

using namespace std;
using namespace Dyninst;
using namespace SymtabAPI;
using namespace InstructionAPI;

class InstructionRewriter {
   private:
      // The main symtab object
      Symtab* symtab_obj;
      Module* module;
      Region* data_region;
      Region* text_region;
      string binary_name;
   public:
      InstructionRewriter(string binary_name);
};

InstructionRewriter::InstructionRewriter(string binary_name) {

   // Try opening and parsing the symbol table
   if (!Symtab::openFile(symtab_obj, binary_name)) {
      // Print message to screen
      fprintf(stderr, "> Couldn't open binary!\n");
      exit(EXIT_FAILURE);
   }
   assert(symtab_obj != NULL);

   // Try getting a handle on the DEFAULT_MODULE
   if (!symtab_obj->findModuleByName(module, "DEFAULT_MODULE")) {
      fprintf(stderr, "> Couldn't find DEFAULT_MODULE\n");
      exit(EXIT_FAILURE);
   }
   assert(module != NULL);

   // Try to find the '.data' region
   if (!symtab_obj->findRegion(data_region, ".data")) {
      fprintf(stderr, "> Couldn't find .data region!\n");
      exit(EXIT_FAILURE);
   }
   if (!symtab_obj->findRegion(text_region, ".text")) {
      fprintf(stderr, "> Couldn't find .text region!\n");
      exit(EXIT_FAILURE);
   }
   assert(data_region != NULL);
   assert(text_region != NULL);

   // Describe what we're looking at
   fprintf(stdout, "Data region:\n");
   fprintf(stdout, "\tgetRegionName: %s\n", data_region->getRegionName().c_str());
   fprintf(stdout, "\tgetRegionAddr: %p\n", data_region->getRegionAddr());
   fprintf(stdout, "\tgetRegionSize: %ld\n", data_region->getRegionSize());
   fprintf(stdout, "\n");

   fprintf(stdout, "Text region:\n");
   fprintf(stdout, "\tgetRegionName: %s\n", text_region->getRegionName().c_str());
   fprintf(stdout, "\tgetRegionAddr: %p\n", text_region->getRegionAddr());
   fprintf(stdout, "\tgetRegionSize: %ld\n", text_region->getRegionSize());
   fprintf(stdout, "\n");

   void* raw_text = text_region->getPtrToRawData();
   long unsigned int raw_text_size = text_region->getRegionSize();

   InstructionDecoder decoder(raw_text, raw_text_size, Arch_x86_64);

   Instruction::Ptr i = decoder.decode();
   while (i != NULL) {
      fprintf(stdout, "%i bytes > %s\n", i->size(), i->format().c_str());
      i = decoder.decode();
   }

   return;
}

int main(int argc, char** argv) {
   if (argc < 2) {
      fprintf(stdout, "Usage: ./program <binary to rewrite>\n");
      exit(EXIT_SUCCESS);
   }
   InstructionRewriter instr_rw(argv[1]);
   return 0;
}


