// Copyright (C) 2019 Guillaume Valadon <guillaume@valadon.net>
// This program is published under a GPLv2 license

/*
 * Decompile a function with Ghidra
 *
 * analyzeHeadless . Test.gpr -import $BINARY_NAME -postScript GhidraDecompiler.java $FUNCTION_ADDRESS -deleteProject -noanalysis
 *
 */

import java.io.*;

import ghidra.app.decompiler.ClangTokenGroup;
import ghidra.app.decompiler.ClangLine;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.decompiler.DecompiledFunction;
import ghidra.app.decompiler.PrettyPrinter;
import ghidra.app.util.headless.HeadlessScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Listing;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import ghidra.program.model.symbol.IllegalCharCppTransformer;

public class GhidraDecompiler2 extends HeadlessScript {


	private void exportToC(DecompileResults results ) {
		try {
			ClangTokenGroup grp = results.getCCodeMarkup();
			if(grp != null) {
				File tmpFile = new File("/tmp/ghidra" + 
						"/" + results.getFunction().getName() + ".c");
				PrintWriter writer = new PrintWriter(new FileOutputStream(tmpFile));

                                PrettyPrinter printer = new PrettyPrinter(results.getFunction(), grp, new IllegalCharCppTransformer());

//				PrettyPrinter printer = new PrettyPrinter(results.getFunction(), grp);
				DecompiledFunction decompFunc = printer.print();
				writer.write(decompFunc.getC());
				writer.close();

                                try {
                                   exportToC2( results);
                                } catch (IOException e) {
                                  //log/handle the exception
                                } catch (Exception e) {
                                  // whatever
                                }
			}
		}
		catch (IOException e) {
			System.out.println(e.getStackTrace());
		}
	}

	private void exportToC2(DecompileResults results ) throws Exception {

			println("Decompilation completed: " + results.decompileCompleted()); // DEBUG

    			DecompiledFunction df = results.getDecompiledFunction();
    			println(df.getC());

                        Function f = results.getFunction();


                        //FileWriter fw = new FileWriter("ghidra-output.r2");
                        //FileWriter fw_dec = new FileWriter("decompiled.c");
			FileWriter fw     = new FileWriter("/tmp/ghidra" + "/" + results.getFunction().getName() + ".r2");
			FileWriter fw_dec = new FileWriter("/tmp/ghidra" + "/" + results.getFunction().getName() + ".c");



			// Print lines prepend with addresses
			//    PrettyPrinter pp = new PrettyPrinter(f, dr.getCCodeMarkup());
    			PrettyPrinter pp = new PrettyPrinter(f, results.getCCodeMarkup(), new IllegalCharCppTransformer());
    			List<ClangLine> lines = pp.getLines();

    			for( ClangLine line : lines) {
      			    long minAddress = Long.MAX_VALUE;
      			    long maxAddress = 0;
      			    for( int i = 0; i < line.getNumTokens(); i++) {
        			if( line.getToken(i).getMinAddress() == null) {
          			   continue;
        		        } // endif
        			long addr = line.getToken(i).getMinAddress().getOffset();
        			minAddress = addr < minAddress ? addr : minAddress;
        			maxAddress = addr > maxAddress ? addr : maxAddress;
      			    } // endfor
      			    if( maxAddress == 0) {
        			println(String.format("                      - %s", line.toString()));
        			String comment = line.toString().split(":", 2)[1];
        			fw_dec.write(String.format("%s\n", comment));
      			    } else {
        			println(String.format("0x%-8x 0x%-8x - %s", minAddress, maxAddress, line.toString()));
        			try {
          			   String comment = line.toString().split(":", 2)[1];
          			   System.out.println(comment);
          			   String b64comment = Base64.getEncoder().encodeToString(comment.getBytes());
          			   fw.write(String.format("CCu base64:%s @ 0x%x\n", b64comment, minAddress));
          			   fw_dec.write(String.format("%s\n", comment));
        			} catch (Exception e) {
          			   System.out.println("ERROR: " + line.toString());
        			}
        			// 0x%-8x 0x%-8x - %s", minAddress, maxAddress, line.toString()));
      			   }
    			} // endfor
    			fw.close();
    			fw_dec.close();
  		} // endfunc


	
	@Override
	public void run() throws Exception {
		var program = this.getCurrentProgram();
		DecompInterface ifc = new DecompInterface();
		ifc.openProgram(program);
		for(var func : program.getFunctionManager().getFunctions(true)) {
		        var results = ifc.decompileFunction(func, 100000, null);
		        exportToC(results);
		}
	}
}
