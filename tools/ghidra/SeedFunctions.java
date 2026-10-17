// Ghidra post-script: disable every analyzer except a small whitelist, create Thumb functions at each
// seed address listed in the file given as arg 0, then run analysis once.
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
import java.util.Set;
import ghidra.app.script.GhidraScript;
import ghidra.framework.options.Options;
import ghidra.program.model.address.Address;
import ghidra.program.model.lang.Register;
import ghidra.program.model.lang.RegisterValue;
import ghidra.program.model.listing.Function;
import java.math.BigInteger;

public class SeedFunctions extends GhidraScript {
    private static final Set<String> KEEP = Set.of(
        "ARM Constant Reference Analyzer",
        "Basic Constant Reference Analyzer",
        "Reference",
        "Subroutine References",
        "Data Reference",
        "Stack",
        "Call Convention ID",
        "ASCII Strings",
        "Shared Return Calls"
    );

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) throw new IllegalArgumentException("usage: SeedFunctions.java <seeds.txt>");

        Options opts = currentProgram.getOptions("Analyzers");
        for (String name : opts.getOptionNames()) {
            if (name.contains(".")) continue; // sub-options
            opts.setBoolean(name, KEEP.contains(name));
        }

        Register tmode = currentProgram.getRegister("TMode");
        List<String> lines = Files.readAllLines(Paths.get(args[0]));
        int created = 0, existing = 0, failed = 0;
        for (String line : lines) {
            line = line.trim();
            if (line.isEmpty()) continue;
            Address a = toAddr(Long.parseLong(line, 16));
            if (getFunctionAt(a) != null) { existing++; continue; }
            try {
                if (getInstructionAt(a) == null) {
                    currentProgram.getProgramContext().setRegisterValue(a, a, new RegisterValue(tmode, BigInteger.ONE));
                    disassemble(a);
                }
                Function f = createFunction(a, null);
                if (f != null) created++;
            } catch (Exception e) {
                failed++;
            }
        }
        println("seeded " + created + " functions (" + existing + " already present, " + failed + " failed)");
        analyzeAll(currentProgram);
        println("seed analysis done: " + currentProgram.getFunctionManager().getFunctionCount() + " functions");
    }
}
