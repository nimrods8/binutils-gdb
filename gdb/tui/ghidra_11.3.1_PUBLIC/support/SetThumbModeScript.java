import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Program;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.lang.Language;
import ghidra.program.model.lang.Processor;

import java.math.BigInteger;
import ghidra.program.model.lang.Register;
import ghidra.program.model.lang.RegisterValue;
import ghidra.program.model.address.Address;
//import ghidra.program.model.address.AddressSetView;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.MemoryBlock;

import ghidra.program.model.lang.LanguageID;
import ghidra.program.model.lang.Language;



    x
public class SetThumbModeScript extends GhidraScript {

    @Override
    protected void run() throws Exception {
        // Get the current program object (this is automatically provided in the GhidraScript context)
        var program = this.getCurrentProgram();

        Language language = program.getLanguage();
        String processor = language.getProcessor().toString();
        String variant = language.getLanguageID().toString(); // e.g., "ARM:LE:32:v7"

        println("***************************************\n*******************************\nThis binary is: " + variant);

        // Check for ARM processor and specifically v7 variant
        if (!processor.equals("ARM") || !variant.contains("32")) {
            println("Skipping: This pre-script is only for ARMv7 binaries.");
            return;
        }

        // testing TMode set
        Register tmodeReg = program.getProgramContext().getRegister("TMode");
        if( tmodeReg == null) {
           System.err.println("Do specify the gdb PID");
           return;
        }
        RegisterValue tmodeValue = new RegisterValue(tmodeReg, BigInteger.ONE);
        //var thumbMode = RegisterValue(tmodeReg, BigInteger.ONE);
        //Address addr = currentAddress;
        //program.getProgramContext().setRegisterValue( addr, addr, tmodeValue);
        //setDefaultDisassemblyContext(thumbMode);
        //
        // Get entire address set of the program

        int count = 0;

        for (MemoryBlock block : program.getMemory().getBlocks()) 
        {
//            if (!block.isInitialized()) continue;

            // Heuristic: check if block is executable (likely code)
//            if (!block.isExecute()) continue;

            currentProgram.getProgramContext().setRegisterValue(
                block.getStart(),
                block.getEnd(),
                tmodeValue);
            count++;
        }

        println("Pre-script: TMode set to Thumb (1) for " + count + " executable blocks.");


//        AddressSetView allAddresses = program.getMemory().getAllInitializedAddressSet();
//        program.getProgramContext().setRegisterValue( allAddresses.getMinAddress(), allAddresses.getMaxAddress(), tmodeValue);
    }
}
