// Ghidra post-script: dump every function (address, size, mode, name) to build/functions.csv
import java.io.FileWriter;
import java.io.PrintWriter;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.lang.RegisterValue;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class ExportFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String out = args.length > 0 ? args[0] : "functions.csv";
        Register tmode = currentProgram.getRegister("TMode");
        int n = 0, thumb = 0;
        try (PrintWriter w = new PrintWriter(new FileWriter(out))) {
            w.println("address,size,mode,name");
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            while (it.hasNext()) {
                Function f = it.next();
                long size = f.getBody().getNumAddresses();
                String mode = "arm";
                if (tmode != null) {
                    RegisterValue rv = currentProgram.getProgramContext().getRegisterValue(tmode, f.getEntryPoint());
                    if (rv != null && rv.getUnsignedValue() != null && rv.getUnsignedValue().intValue() == 1) {
                        mode = "thumb";
                        thumb++;
                    }
                }
                w.printf("%08x,%d,%s,%s%n", f.getEntryPoint().getOffset(), size, mode, f.getName());
                n++;
            }
        }
        println("exported " + n + " functions (" + thumb + " thumb) to " + out);
    }
}
