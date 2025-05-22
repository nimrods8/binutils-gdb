// Copyright (C) 2019 Guillaume Valadon <guillaume@valadon.net>
// This program is published under a GPLv2 license

/*
 * Decompile a function with Ghidra
 *
 * analyzeHeadless . Test.gpr -import $BINARY_NAME -postScript GhidraDecompiler.java $FUNCTION_ADDRESS -deleteProject -noanalysis
 *
 */

import java.io.*;
import java.util.*;

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
         			  System.out.println(e.getStackTrace());
                                } catch (Exception e) {
                                  // whatever
			          System.out.println(e.getStackTrace());
                                }
			}
		}
		catch (IOException e) {
			System.out.println(e.getStackTrace());
		}
	}

	private void exportToC2(DecompileResults results, String target_folder ) throws Exception {

			//DEBUG: println("Decompilation completed: " + results.decompileCompleted()); // DEBUG

    			DecompiledFunction df = results.getDecompiledFunction();
    			//DEBUG println(df.getC());

                        Function f = results.getFunction();


                        //FileWriter fw = new FileWriter("ghidra-output.r2");
			FileWriter fw     = new FileWriter( target_folder + "/" + results.getFunction().getName() + ".r2");

                        // *** open the sal.rx with append flag ***
			FileWriter fw_sal = new FileWriter( target_folder + "/sal.rx", true);


			// Print lines prepend with addresses
			//    PrettyPrinter pp = new PrettyPrinter(f, dr.getCCodeMarkup());
    			PrettyPrinter pp = new PrettyPrinter(f, results.getCCodeMarkup(), new IllegalCharCppTransformer());
    			List<ClangLine> lines = pp.getLines();

                        int lineNo = 0;

    			for( ClangLine line : lines) {
                            lineNo++;
      			    long minAddress = Long.MAX_VALUE;
      			    long maxAddress = 0;
      			    for( int i = 0; i < line.getNumTokens(); i++) {
        			if( line.getToken(i).getMinAddress() == null) {
          			   continue;
        		        } // endif
        			long addr = line.getToken(i).getMinAddress().getOffset();
        			minAddress = addr < minAddress ? addr : minAddress;
        			maxAddress = addr > maxAddress ? addr : maxAddress;

				String sr = String.format("0x%08x 0x%08x:%s:%d\n", minAddress, maxAddress, results.getFunction().getName() + ".c", lineNo);
                                //println( sr);
        			fw_sal.write( sr);

      			    } // endfor
      			    if( maxAddress == 0) {
        			// DEBUG: println(String.format("                      - %s", line.toString()));
        			String comment = line.toString().split(":", 2)[1];
      			    } else {
        			// DEBUG: println(String.format("0x%-8x 0x%-8x - %s", minAddress, maxAddress, line.toString()));
        			try {
          			   String comment = line.toString().split(":", 2)[1];
          			   // DEBUG: System.out.println(comment);
          			   String b64comment = Base64.getEncoder().encodeToString(comment.getBytes());
          			   fw.write(String.format("CCu base64:%s @ 0x%x\n", b64comment, minAddress));
        			} catch (Exception e) {
          			   System.out.println("ERROR: " + line.toString());
        			}
        			// 0x%-8x 0x%-8x - %s", minAddress, maxAddress, line.toString()));
      			   }
    			} // endfor
    			fw.close();
                        fw_sal.close();
  		} // endfunc


          private void sortSalFile( String target_folder ) throws Exception 
          {
                class FunctionEntry {
                      String function_name;
                      long address;
                      int lineNo;
                      // Constructor to initialize FunctionEntry object
                      public FunctionEntry(String function_name, long address, int lineNo) {
                           this.function_name = function_name;
                           this.address = address;
                           this.lineNo = lineNo;
                      }
                      public long getAddr1() {
                           return address;
                      }
                      public String getFunctionName() {
                           return function_name;
                      }
                      // toString method to output the line in the correct format
                      @Override
                      public String toString() {
                           return "0x" + Long.toHexString(address).toUpperCase() + " " + "0x" +
                                          Long.toHexString(address).toUpperCase() + ":" + function_name + ":" + lineNo;
                      }
                      // Override equals to compare function_name and addr2 for uniqueness
                      @Override
                      public boolean equals(Object o) {
                         if (this == o) return true;
                         if (o == null || getClass() != o.getClass()) return false;
                         FunctionEntry that = (FunctionEntry) o;
                         return address == that.address && function_name.equals(that.function_name);
                      }
                      // Override hashCode to match equals
                      @Override
                      public int hashCode() {
                        return Objects.hash(function_name, address);
                      }
                } // endclass
               // List<FunctionEntry> entries = new ArrayList<>();
                Set<FunctionEntry> entries = new HashSet<>();  // Use a Set to automatically remove duplicates
                BufferedReader reader = new BufferedReader(new FileReader( target_folder + "/sal.rx"));
                String line;
                while ((line = reader.readLine()) != null) {
                    // Parse the line: addr1 addr2:function_name:lineNo
                    String[] parts = line.split(":");
                    if( parts.length == 3) {
                       String[] addrParts = parts[0].split(" ");
                       if( addrParts.length == 2) {
                          long addr1 = Long.parseLong( addrParts[0].trim().substring(2), 16); // Remove '0x' and parse as hex
                          String function_name = parts[1].trim();
                          int lineNo = Integer.parseInt(parts[2].trim());
                          entries.add(new FunctionEntry( function_name, addr1, lineNo));
                       } // endif
                    } // endif
                } // endwhile
                reader.close();
                List<FunctionEntry> sortedEntries = new ArrayList<>(entries);  // Create and use FunctionEntry objects inside the method




                Collections.sort( sortedEntries, new Comparator<FunctionEntry>() {
                @Override
                   public int compare(FunctionEntry e1, FunctionEntry e2) {
                         // First compare by function_name
/*
                         int functionComparison = e1.getFunctionName().compareTo(e2.getFunctionName());
                         if (functionComparison != 0) {
                            return functionComparison;
                         }
*/
                         // If function_name is the same, compare by addr2 (hex comparison)
                         return Long.compare(e1.getAddr1(), e2.getAddr1());
                   } // endfunc compare
                });

		// remove all the helper ..lin.. for now... at least until I understand what is their role
                sortedEntries.removeIf( entry -> entry.getFunctionName().startsWith("..lin."));


//--                Collections.sort(sortedEntries, new Comparator<FunctionEntry>() {
//--                @Override
//--                   public int compare(FunctionEntry e1, FunctionEntry e2) {
//--                      int functionComparison = e1.getFunctionName().compareTo(e2.getFunctionName());
//--                      if (functionComparison != 0) {
//--                         return functionComparison;  // Group by function name
//--                      }
//--                      return Long.compare(e1.getAddr1(), e2.getAddr1());  // Sort by address within each group
//--                  }
//--                });



                // Output the entries
                BufferedWriter writer = new BufferedWriter(new FileWriter( target_folder + "/sal.rxx"));
                for( FunctionEntry entry : sortedEntries) {
                      writer.write(entry.toString());
                      writer.newLine();  // Write newline
                } // endfor
                writer.close();
// debug:
//                for( FunctionEntry entry : sortedEntries) {
//                    System.out.println(entry.toString());
//                } // endfor
//
         } // endfunc


	
	@Override
	public void run() throws Exception {

                // Stop after this headless script
                setHeadlessContinuationOption(HeadlessContinuationOption.ABORT);
		System.out.println("*******************\nGD2 v0.2\n*******************\n\n");

                // Get the function address from the script arguments
                String[] args = getScriptArgs();

                if( args.length == 0) {
                   System.err.println("Do specify a target folder for the function files");
                   return;
                }
                if( args.length != 2) {
                   System.err.println("Do specify the gdb PID");
                   return;
                }

		var program = this.getCurrentProgram();

		DecompInterface ifc = new DecompInterface();
		ifc.openProgram(program);
		for(var func : program.getFunctionManager().getFunctions(true)) {
		        var results = ifc.decompileFunction(func, 5, null);
		        exportToC(results, args[0]);
		}
                sortSalFile( args[0]);

                int pid = Integer.parseInt( args[1]); // Replace with the target process PID
                String signal = "-USR1"; // Use "-TERM", "-KILL", "-HUP", etc.

                ProcessBuilder pb = new ProcessBuilder("kill", signal, String.valueOf(pid));
                try {
                        Process process = pb.start();
                        process.waitFor();
                        System.out.println("Signal " + signal + " sent to PID " + pid);
                } catch (IOException | InterruptedException e) {
                        e.printStackTrace();
                }

	} // endfunc run
} // endclass 
