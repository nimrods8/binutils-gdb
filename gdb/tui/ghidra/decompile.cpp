#include "database.hh"
#include "funcdata.hh"
#include "crc32.hh"
#include <ctype.h> 


#include "architecture.hh"
#include "grammar.hh"
//#include "test.hh"
#include <iostream>
#include <libdecomp.hh>


static Architecture *glb;
static TypeFactory *types;
static CastStrategy *strategy;
static Funcdata *dummyFunc;

Architecture *g;


// /home/nstoler/projects/rz-ghidra/ghidra/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/database.hh
int main( void)
{
//  startDecompilerLibrary( "/root/.local/lib/x86_64-linux-gnu/rizin/plugins/rz_ghidra_sleigh");
  startDecompilerLibrary( "./rz-ghidra/");

  //startDecompilerLibrary( "/home/nstoler/projects/rz-ghidra/");
  g = (Architecture *)0;


//  XmlArchitectureCapability xml = new XmlArchitectureCapability();


  ArchitectureCapability *xmlCapability = ArchitectureCapability::getCapability("xml");
  istringstream s(
      "<binaryimage arch=\"x86:LE:64:default:gcc\"></binaryimage>"
  );
  DocumentStorage store;
  Document *doc = store.parseDocument(s);
  store.registerTag( doc->getRoot());

  g = xmlCapability->buildArchitecture("", "", &cout);
  g->init(store);

  glb = g;
  types = glb->types;
  strategy = glb->print->getCastStrategy();
  Address addr(glb->getDefaultCodeSpace(),0x1000);
  dummyFunc = glb->symboltab->getGlobalScope()->addFunction(addr, "dummy")->getFunction();
  dummyFunc->setHighLevel();
} // ENDFUNC MAIN

#if 0
Datatype *parse(const string &text) {
  istringstream s(text);
  string unused;
  return parse_type(s,unused,glb);

} // ENDFUNC 

#endif

/*
Funcdata *RizinScope::findFunction(const Address &addr) const

	FunctionSymbol *sym;
	sym = dynamic_cast<FunctionSymbol *>(queryRizin(addr, false));
	if(sym)
		return sym->getFunction();
*/

