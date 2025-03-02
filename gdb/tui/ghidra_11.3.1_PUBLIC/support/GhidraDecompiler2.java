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
import java.io.BufferedWriter;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import ghidra.program.model.symbol.IllegalCharCppTransformer;

public class GhidraDecompiler2 extends HeadlessScript {


	private void exportToC(DecompileResults results, String target_folder ) {
		try {
			ClangTokenGroup grp = results.getCCodeMarkup();
			if(grp != null) {
				File tmpFile = new File( target_folder + 
						"/" + results.getFunction().getName() + ".c");
				PrintWriter writer = new PrintWriter(new FileOutputStream(tmpFile));

                                PrettyPrinter printer = new PrettyPrinter(results.getFunction(), grp, new IllegalCharCppTransformer());

//				PrettyPrinter printer = new PrettyPrinter(results.getFunction(), grp);
				DecompiledFunction decompFunc = printer.print();
				writer.write(decompFunc.getC());
				writer.close();

                                try {
                                   exportToC2( results, target_folder);
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

	private void exportToC2(DecompileResults results, String target_folder ) throws Exception {

    			DecompiledFunction df = results.getDecompiledFunction();
    			//> this works quite nicely>> println(df.getC());

                        Function f = results.getFunction();

                        //FileWriter fw = new FileWriter("ghidra-output.r2");
                        //FileWriter fw_dec = new FileWriter("decompiled.c");

			FileWriter fw     = new FileWriter( target_folder + "/" + results.getFunction().getName() + ".r2");
//			FileWriter fw_dec = new FileWriter( target_folder + "/" + results.getFunction().getName() + ".c");

//                        FileWriter fw2    = new FileWriter( target_folder + "/" + results.getFunction().getName() + ".r3");
//                        fw2.write( df.getC());
//                        fw2.close();


         	        // Open the file for appending text
                        BufferedWriter writer = new BufferedWriter(new FileWriter( target_folder + "/sal.rx", true)); 


			// Print lines prepend with addresses
			//    PrettyPrinter pp = new PrettyPrinter(f, dr.getCCodeMarkup());
    			PrettyPrinter pp = new PrettyPrinter(f, results.getCCodeMarkup(), new IllegalCharCppTransformer());
    			List<ClangLine> lines = pp.getLines();

                        long lineNo = 0;

    			for( ClangLine line : lines) {
      			    long minAddress = Long.MAX_VALUE;
      			    long maxAddress = 0;

                            lineNo++;

      			    for( int i = 0; i < line.getNumTokens(); i++) {
        			if( line.getToken(i).getMinAddress() == null) {
          			   continue;
        		        } // endif
        			long addr = line.getToken(i).getMinAddress().getOffset();
        			minAddress = addr < minAddress ? addr : minAddress;
        			maxAddress = addr > maxAddress ? addr : maxAddress;
        			//String qdata = String.format("[1] 0x%-8x 0x%-8x:%s:%l", minAddress, maxAddress, results.getFunction().getName(), lineNo);
                                //writer.write( qdata);  // Append the text to the file

      			    } // endfor
      			    if( maxAddress == 0) {
        			//println(String.format("                      - %s", line.toString()));
        			String comment = line.toString().split(":", 2)[1];
        			//> fw_dec.write(String.format("%s\n", comment));
      			    } else {
        			// println(String.format("0x%-8x 0x%-8x - %s", minAddress, maxAddress, line.toString()));
        			try {
          			   String comment = line.toString().split(":", 2)[1];
          			   //System.out.println(comment);
          			   String b64comment = Base64.getEncoder().encodeToString(comment.getBytes());
          			   fw.write(String.format("CCu base64:%s @ 0x%x\n", b64comment, minAddress));
          			   //> fw_dec.write(String.format("%s\n", comment));

           			   String qdata = String.format("0x%08x 0x%08x:%s:%d\n", minAddress, maxAddress, results.getFunction().getName(), lineNo);
                                   writer.write( qdata);  // Append the text to the file
        			} catch (Exception e) {
          			   System.out.println("ERROR: " + line.toString());
        			}
        			// 0x%-8x 0x%-8x - %s", minAddress, maxAddress, line.toString()));
      			   }
    			} // endfor
    			fw.close();
                        writer.close();
    			//> fw_dec.close();
  		} // endfunc


	
	@Override
	public void run() throws Exception {

                // Stop after this headless script
                setHeadlessContinuationOption(HeadlessContinuationOption.ABORT);

                // Get the function address from the script arguments
                String[] args = getScriptArgs();
                println(String.format("Array length: %d", args.length)); // DEBUG

                if( args.length == 0) {
                   System.err.println("Please specify a target folder for the function files");
                   return;
                }


		var program = this.getCurrentProgram();
		DecompInterface ifc = new DecompInterface();
		ifc.openProgram(program);
		for(var func : program.getFunctionManager().getFunctions(true)) {
		        var results = ifc.decompileFunction(func, 5, null);
		        exportToC(results, args[0]);
		}
	}
}
